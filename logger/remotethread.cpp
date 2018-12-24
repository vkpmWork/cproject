#include "tclientsocket.h"
#include "remotethread.h"
#include "config.h"
#include "file.h"
#include "log.h"
#include <pthread.h>

#define ATTEMPT_COUNTER 2

//----------------------------------------
void* handler_remote_thread(void*)
{
	Mmessagelist mml;
    RemoteThread *pWrk_Remote = new RemoteThread(pConfig->RemoteAddress(), pConfig->RemotePort(), pConfig->Folder(), pConfig->remote_size_in_memo());
    if (!pWrk_Remote) return NULL;

	sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR2);

	/*
	 * Блокируем доставку сигналов SIGUSR2
	 * После возвращения предыдущее значение
	 * заблокированной сигнальной маски хранится в oldmask
	 */

	sigprocmask(SIG_BLOCK, &mask, &oldmask);

	/* Задаём время ожидания X с */
	struct timespec tv;
	tv.tv_sec  = pConfig->retry_interval();
	tv.tv_nsec = 0;

	/*
	 * Ждём доставки сигнала. Мы ожидаем SIGUSR2,
	 * SIGRTMIN и SIGRTMIN+1. Цикл завершится, когда
	 * будут приняты оба сигнала
	 */

	siginfo_t siginfo;
    int 	  recv_sig;

    if (!pWrk_Remote->empty_msg_list()) raise(SIGUSR2);

    while(true)
	{
    	if ((recv_sig = sigtimedwait(&mask, &siginfo, &tv)) == -1)
    	{
    		    if (errno != EAGAIN)
    		    {
    		    	perror("sigtimedwait");
    		    	break;
    		    }
    	}

  	   printf("Remote signal %d received\n",  recv_sig);

  	   if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexLock();
	   mml =  pClientMessage->message_list;
	   pClientMessage->ClearMessageList();
       if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexUnlock();

	   if (pWrk_Remote) pWrk_Remote->RunWork(mml);

       if (siginfo._sifields._rt.si_sigval.sival_int == 0xFF) break;
    }

    if (pWrk_Remote)
    {   pWrk_Remote->SaveRemoteFile();
    	delete pWrk_Remote; pWrk_Remote = NULL;
    }
    std::cout << "Remote Exit" << endl;
    sleep(2);
    return NULL;
}

void create_remote_thread()
{
	   pthread_attr_t thread_attr;

	   std::ostringstream m;
	   if (pthread_attr_init(&thread_attr) != 0)
	   {
		   m.str("Attribute creation failed (create_remote_thread())\n");
		   logger::write_log(m);
		   return;
	   }

	    // переводим в отсоединенный режим
	   if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) != 0)
	    {
	    	m.str("Setting detached attribute failed (create_remote_thread())\n");
	    	logger::write_log(m);
	    	return;
	    }

	    if ( pthread_create(&remote_thread, &thread_attr, handler_remote_thread, NULL)) std::cout << "In  : Thread creation failed (create_remote_thread())\n";

	    (void)pthread_attr_destroy(&thread_attr);

	    std::cout << "Remote thread Created\n";

}
//----------------------------------------

RemoteThread::RemoteThread(string m_address, ulong m_port, string mfolder, size_t m_size)
	: m_Address(m_address)
	, m_Port(m_port)
	, m_folder(mfolder)
  	, fl_local_exists(false)
    , m_size_in_memo(m_size)

{
     pWrk_file = new filethread();
     if (pWrk_file) msg_list = pWrk_file->GetLocalMsgList();

     fl_local_exists = (bool)msg_list.size();
     cout << "msg_list = " << msg_list.size() << std::endl;
}

void RemoteThread ::append(Mmessagelist m_list)
{
	if (m_list.empty()) return;

	Vmessage    msg;
	int 		sz = 0;
	for (Mmessagelist::iterator it = m_list.begin(); it != m_list.end(); it++ )
	{

			msg = (*it).second;
        	sz  = msg_list.size();
			if (sz > (int)m_size_in_memo)
			{
				sz = (sz*3)/4;
				msg_list.erase(msg_list.begin(), msg_list.begin() + sz);
			}
			msg_list.insert(msg_list.end(), msg.begin(), msg.end());
	}

}

void RemoteThread::RunWork(Mmessagelist m_list)
{
	append(m_list);
	if (!msg_list.size()) return;

    bool   is_ok = false;

    tclient_socket *Socket  = new tclient_socket((char*)pConfig->RemoteAddress().c_str(), pConfig->RemotePort(), pConfig->retry_interval());
    if (!Socket) return;

    if (Socket->net_connect())
    {

    	char *buf = 0;
    	int   len = 0;
    	int   cnt = 0;
    	for (Vmessage::iterator it = msg_list.begin(); it != msg_list.end(); it++)
    	{
    		len = (*it).length();

    		buf = (char*)calloc(len, sizeof(char));
    		if (buf)
    		{
    	    	memmove(buf, (*it).c_str(), len);
    			is_ok = Socket->net_send(buf, len);
    			free(buf);
    			sleep(1);
    			cnt++;

    		//	if (!is_ok) break;
    		}
    	}
    	Socket->net_close();
    	cout << "Cnt = " << cnt << endl;
    }
    delete Socket;
    Socket = NULL;

    if (is_ok)
    {
         msg_list.clear();
         if (fl_local_exists && pWrk_file) pWrk_file->UnlinkLocalFile();
    }
    else
    {
    	SaveRemoteFile();
        msg_list.clear();
        //bool fl_local_exists = pWrk_file->RunLocal(msg_list);

    }
}


void RemoteThread::SaveRemoteFile()
{
	if (pWrk_file)
	{
		pWrk_file->RunLocal(msg_list);
	}
}

RemoteThread :: ~RemoteThread()
{

    if (pWrk_file)
    {
        delete pWrk_file;
        pWrk_file = NULL;
    }

}
