#include "clientmessage.h"
#include "remotethread.h"
#include "log.h"
#include <signal.h>
#include <pthread.h>

/*---------------------------------------------*/
pthread_t 	  message_thread;

void Msg_to_local_store (Mmessagelist ml, msgevent::tcEvent ev_code)
{
	std::cout <<"Msg_to_local_store\n";

	filethread *pWrk_file = new filethread(ml, ev_code);
	if (pWrk_file)
	{
		ev_code == msgevent::evMsg ? pWrk_file->RunWork() : pWrk_file->TryToDeleteFile();
		delete pWrk_file; pWrk_file = NULL;
	}

}

void *handler_message_thread(void*)
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

	/* Задаём время ожидания 1 с */
	struct timespec tv;
	tv.tv_sec  = 60;
	tv.tv_nsec = 0;

	/*
	 * Ждём доставки сигнала. Мы ожидаем 2 сигнала,
	 * SIGRTMIN и SIGRTMIN+1. Цикл завершится, когда
	 * будут приняты оба сигнала
	 */

	siginfo_t siginfo;
    int 	  recv_sig;

	string s;
    while(true)
	{
//      std::cout << "TimeOut\n";
	  if ((recv_sig = sigtimedwait(&mask, &siginfo, &tv)) == -1)
	  {
	    if (errno == EAGAIN) continue;

	    perror("sigtimedwait");
        break;
      }

	  printf("signal %d received. Code %i\n",  recv_sig,  siginfo._sifields._rt.si_sigval.sival_int);

	  union sigval svalue;
	  svalue.sival_int =  siginfo._sifields._rt.si_sigval.sival_int;

	  if (svalue.sival_int != msgevent::evMsg && svalue.sival_int != msgevent::evDelete) continue;

	  if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexLock();
	  Mmessagelist mml =  pClientMessage->message_list;
	  pClientMessage->ClearMessageList();
	  if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexUnlock();

	  //pConfig->CurrentStoreType == Logger_namespace::LOCAL_STORE ? Msg_to_local_store (mml, (msgevent::tcEvent)svalue.sival_int) : Msg_to_remote_store(mml);
	  Msg_to_local_store (mml, (msgevent::tcEvent)svalue.sival_int);
	}
    std::cout << "4\n";
    return NULL;
}

void	create_message_handler_thread()
{
	   //if ( pthread_create(&message_thread, NULL, handler_message_thread, NULL) != 0) std::cout << "In handler_timer : Thread creation failed\n";

//       return;

       pthread_attr_t thread_attr;

	   std::ostringstream m;
	   if (pthread_attr_init(&thread_attr) != 0)
	   {
		   m.str("Attribute creation failed (create_message_handler_thread(v_messagelist m_message)\n");
		   logger::write_log(m);
		   return;
	   }

	    // переводим в отсоединенный режим
	   if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) != 0)
	    {
	    	m.str("Setting detached attribute failed (create_message_handler_thread(v_messagelist m_message))\n");
	    	logger::write_log(m);
	    	return;
	    }

	    if ( pthread_create(&message_thread, &thread_attr, handler_message_thread, NULL) != 0) std::cout << "In  : Thread creation failed (create_message_handler_thread(v_messagelist m_message))\n";

	    (void)pthread_attr_destroy(&thread_attr);
}

/*---------------------------------------------*/
TClientMessage *pClientMessage = NULL;
//pthread_t logger_thread;

TClientMessage::TClientMessage(pthread_t m_msg_thread, pthread_t m_error_thread, Logger_namespace::tcStore store , ulong maxStoreListSize, uint checkPeriod, uint m_error_level)
  	  :   msg_thread  (m_msg_thread)
		, error_thread(m_error_thread)
		, MaxStoreListSize(maxStoreListSize)
		, CategoryStore(store)
		, m_cmd(msgevent::evEmpty)
		, CheckPeriod(checkPeriod)
		, ErrorLevel(m_error_level)

{
    FTransmitFromLocalLog = false;
}

void TClientMessage::OnReadyTransmitMessage()
{
	if (message_list.size() < MaxStoreListSize) return;

	union sigval value;
   	value.sival_int = msgevent::evMsg;

   	pthread_sigqueue(CategoryStore == Logger_namespace::LOCAL_STORE ? msg_thread : remote_thread, SIGUSR2, value);
}

void TClientMessage::OnDeleteMessage(/*std::string m_msg*/)
{
	union sigval value;
   	value.sival_int = (int) msgevent::evDelete;
	pthread_sigqueue(msg_thread, SIGUSR2, value);

	std::cout << "OnDeleteMessage(std::string m_msg) " << "value.sival_int = " << value.sival_int << endl;
}

inline void  TClientMessage::OnTransmitError(uint m_error, std::string m_domain, std::string m_msg)
{
	union sigval value;

	TMess *mess = new TMess();
	mess->value = m_error;
	mess->domain = strdup(m_domain.c_str());
	mess->msg    = strdup(m_msg.c_str());

   	value.sival_ptr = mess;
	pthread_sigqueue(error_thread, SIGUSR2, value);

}

void    TClientMessage::AddMessage(TLogMsg *m)
{
   m_cmd   = msgevent::evEmpty;
   msgevent::tcEvent ev = m->Event();

   switch(ev)
   {
        case msgevent::evMsg     :
        case msgevent::evDelete  : if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexLock();
        						   if (ev == msgevent::evMsg)
        						   {
        							   message_list[m->GlobalFileName()].push_back(CategoryStore == Logger_namespace::LOCAL_STORE ? m->Message() : m->msg());
        							   OnReadyTransmitMessage();
        							   if (0 < ErrorLevel && (uint)m->error() > ErrorLevel)
        								   	   OnTransmitError(m->error(), m->domain(), m->Message());
        						   }
        						   else
        						   {
    		   	   	   	   	   	   	   message_list[m->domain()].push_back(m->fullfilename());
        							   OnDeleteMessage();
        						   }

         						   if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexUnlock();
                                   break;
        case msgevent::evConfig  : m_cmd = msgevent::evConfig;break;
        case msgevent::evExit    : kill(getpid(), SIGTERM);   break;
        default                  : break;
   }
}
bool TClientMessage::TransmitFromLocalLog()
{
    if (pLoggerMutex) pLoggerMutex->RemoteRunnable_MutexLock();

    bool value = FTransmitFromLocalLog;

    if (pLoggerMutex) pLoggerMutex->RemoteRunnable_MutexUnlock();

    return value;
}

bool TClientMessage::TransmitFromLocalLog(bool value)
{
    if (pLoggerMutex) pLoggerMutex->RemoteRunnable_MutexLock();

    if (value != FTransmitFromLocalLog)  FTransmitFromLocalLog = value;

    if (pLoggerMutex) pLoggerMutex->RemoteRunnable_MutexUnlock();

    return FTransmitFromLocalLog;
}

void TClientMessage::ClearMessageList()
{
	for (Mmessagelist::iterator it = message_list.begin(); it != message_list.end(); it++ ) ((*it).second).clear();
	message_list.clear();
}


TClientMessage::~TClientMessage()
{
	std::ostringstream s;
	s.str("Destructor TClientMessage - Ok");
	wmsg(s, common::levDebug);
}

void  TClientMessage::set_transmit_timer(bool value)
{
	/*
    bool aa = pTimerTramsmitMessage->isActive();
    if ( aa != value)
    {
        if (value) pTimerTramsmitMessage->start();
        else
            pTimerTramsmitMessage->stop();
    }
    */
}
