#include "tclientsocket.h"
#include "remotethread.h"
#include "clientmessage.h"
#include "config.h"
#include "file.h"
#include "log.h"
#include <pthread.h>

#define ATTEMPT_COUNTER 2
//----------------------------------------
void* handler_remote_thread(void*)
{
	sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR2);

	/*
	 * Блокируем доставку сигналов SIGUSR2
	 * После возвращения предыдущее значение
	 * заблокированной сигнальной маски хранится в oldmask
	 */

	sigprocmask(SIG_BLOCK, &mask, &oldmask);

	/*
	 * Ждём доставки сигнала. Мы ожидаем 2 сигнала,
	 * SIGRTMIN и SIGRTMIN+1. Цикл завершится, когда
	 * будут приняты оба сигнала
	 */

    int 	  recv_sig;
    while(true)
	{
	   sigwait(&mask, &recv_sig);

       if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexLock();
	   Mmessagelist mml =  pClientMessage->message_list;
	   pClientMessage->ClearMessageList();
	   if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexUnlock();

	   RemoteThread *pWrk_Remote = new RemoteThread(mml, pConfig->RemoteAddress(), pConfig->RemotePort(), pConfig->Folder());
	   if (pWrk_Remote)
	   {
		   pWrk_Remote->RunWork();
		   delete pWrk_Remote; pWrk_Remote = NULL;
	   }


      continue;
    }


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

RemoteThread::RemoteThread(Mmessagelist m_mml, string m_address, ulong m_port, string mfolder)
	: append_list(m_mml)
	, m_Address(m_address)
	, m_Port(m_port)
	, m_folder(mfolder)
  	, fl_local_exists(false)
    , local_counter(0)

{
     pWrk_file = new filethread();
     if (pWrk_file)
     {
       	 	    msg_list = pWrk_file->GetLocalMsgList();
       	 	    Vmessage    msg;
    	    	for (Mmessagelist::iterator it = append_list.begin(); it != append_list.end(); it++ )
    	    	{

    	    			msg    = (*it).second;
    	    			msg_list.insert(msg_list.end(), msg.begin(), msg.end());
    	    	}
     }

}

void RemoteThread :: RunWork()
{
    if (!msg_list.size()) return;

    int msg_counter = msg_list.size();
    int count       = 0;               /* кол-во сообщений, передаваемых в одной строке*/

    string ss    = PrepareForSend(msg_list, count /* индекс в очереди */, msg_counter); /* строка для передачи на удаленный сервер */

    bool   is_ok = false;
    int    m_attempt_counter = 0, m_fact_counter = 0;


    tclient_socket *Socket  = new tclient_socket((char*)pConfig->RemoteAddress().c_str(), pConfig->RemotePort(), pConfig->retry_interval());
    if (!Socket) return;

    if (Socket->net_connect())
    {
        is_ok = true;
        while(is_ok && ss.size())
        {
            is_ok  = SendToServer(socket, ss);
            if (/*is_ok && */ socket->state() == QAbstractSocket::ConnectedState && socket->waitForBytesWritten(WAIT_EVENT)) // *2
            {
                if (socket->waitForReadyRead(WAIT_EVENT)) socket->readAll();
                else
                {

                    is_ok = false;
                    pInternalLog->LOG_OPER("RemoteThread :: SendToServer(string str) Нет квитанции от Logger-сервера!");
                    pInternalLog->APPEND_LOG(socket->errorString().toStdString());
                }
            }
            else
            {   is_ok = false;
                pInternalLog->LOG_OPER("RemoteThread :: waitForBytesWritten == false");
                pInternalLog->APPEND_LOG(socket->errorString().toStdString());
            }

            if (is_ok)
            {
                m_fact_counter = count;
                ss = PrepareForSend(msg_list, count, msg_counter);
            }
            else
                if (++m_attempt_counter >= ATTEMPT_COUNTER) break;

        }
    }

    socket->close();
    socket->disconnectFromHost();
    delete socket;
    socket = NULL;

    if (is_ok)
    {
        SetLock(msg_lock);
           msg_list.clear();
           local_counter = 0;
        SetUnlock(msg_lock);

        emit finished();
        if (fl_local_exists && pWrk_file) pWrk_file->UnlinkLocalFile();
    }
    else
    {
         int  v = local_counter + m_fact_counter;

         if ((v < msg_counter) && pWrk_file)
         {
                        SetLock(msg_lock);
                        fl_local_exists = pWrk_file->RunLocal(msg_list, msg_list.begin() + v, msg_list.end());
                        SetUnlock(msg_lock);
                        local_counter = msg_counter;
         }
         QTimer::singleShot(300, this, SLOT(RunWork()));
    }
}



RemoteThread :: ~RemoteThread()
{
    pthread_mutex_destroy(&append_lock);
    pthread_mutex_destroy(&msg_lock);

    if (pWrk_file)
    {
        delete pWrk_file;
        pWrk_file = NULL;
    }

}

string RemoteThread :: PrepareForSend(const Vmessage &msg, int &count /* индекс в очереди */, int msg_size /* размер очереди */)
{
    if (!msg_size) return "";

    string str,  Str = "";
    int    szcounter = 0;

    char    ch[sizeof(ushort)];
    ushort  sz;

    while(count < msg_size && szcounter < 100/*MAX_MESSAGE_SIZE*/)
    {
        str = msg[count++];

        sz  = str.size();
        memcpy(ch, &sz, sizeof(ushort));

        Str.append(ch, 2);
        Str.append(str);
        szcounter += (sz+2);
    }
    return Str;
}

bool RemoteThread :: SendToServer(int socket, string str)
{
    bool   rez_value = false;
    ushort sz        = str.size();

    char *buf = (char*)malloc(sz);
    if (buf && sz)
    {
        memmove(buf, str.c_str(), sz);
        rez_value = SendToServer(socket, buf, sz);
        free(buf);
    }
    return rez_value;
}

bool RemoteThread :: SendToServer(int socket, char *str, size_t sz)
{
    bool ok   = false;
    try
    {
        size_t v = socket->write(str, sz);
        ok = (sz == v);
        if (!ok)
        {
            pInternalLog->LOG_OPER("RemoteThread :: SendToServer - Transmit to Server didn't compleate");
        }
    }
    catch(...) { }

    return ok;
}


void RemoteThread::SetLock(pthread_mutex_t m_mutex)
{
    pthread_mutex_lock(&m_mutex);
}

void RemoteThread::SetUnlock(pthread_mutex_t m_mutex)
{
    pthread_mutex_unlock(&m_mutex);
}

void RemoteThread :: onMsg_list_append(Mmessagelist m_list)
{
    SetLock(append_lock);
    append_list.insert(append_list.end(), m_list.begin(), m_list.end());
    SetUnlock(append_lock);
}

bool RemoteThread ::empty_msg_list()
{
    bool fl = true;
    SetLock(msg_lock);
    fl = msg_list.empty();
    SetUnlock(msg_lock);

    return fl;
}

Vmessage RemoteThread::GetLocalMsgList()
{
	Vmessage m;
    return m;
}
