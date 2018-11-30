#include "clientmessage.h"
#include "loggermutex.h"
#include "log.h"
#include <signal.h>
#include <pthread.h>

TClientMessage *pClientMessage = NULL;
pthread_t logger_thread;
TClientMessage::TClientMessage(pthread_t m_msg_thread, Logger_namespace::tcStore store , ulong maxStoreListSize, uint checkPeriod)
  	  :   msg_thread(m_msg_thread)
		, MaxStoreListSize(maxStoreListSize)
		, CategoryStore(store)
		, m_cmd(msgevent::evEmpty)
		, CheckPeriod(checkPeriod)
{
    FTransmitFromLocalLog = false;
}

void TClientMessage::OnReadyTransmitMessage()
{
	if (message_list.size() < MaxStoreListSize) return;

	union sigval value;
   	value.sival_int = MSG_TYPE;
   	pthread_sigqueue(msg_thread, SIGUSR2, value);
   	std::cout << "OnReadyTransmitMessage()\n";
}

void TClientMessage::OnDeleteMessage(std::string m_msg)
{
	union sigval value;
   	char *p = (char*)m_msg.c_str();
   	value.sival_ptr = p;
   	pthread_sigqueue(msg_thread, SIGUSR2, value);
   	p = (char*)value.sival_ptr;
   	std::cout << "OnDeleteMessage(std::string m_msg) " << "value.sival_ptr = " << p << endl;

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
        							   message_list[m->GlobalFileName()].push_back(m->Message());
        							   OnReadyTransmitMessage();
        						   }
        						   else OnDeleteMessage(m->GlobalFileName());

         						   if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexUnlock();
                                   break;
        case msgevent::evConfig  : m_cmd = msgevent::evConfig;break;
        case msgevent::evExit    : kill(getpid(), SIGTERM);   break;
        default                  : break;
   }

  // emit add_error(m->error(), (char*)m->domain().c_str(), (char*)m->Message().c_str());

//   if (message_list.size() >= MaxStoreListSize) OnReadyTransmitMessage();

//   if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexUnlock();

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
