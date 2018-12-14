#include "clientmessage.h"
#include "loggermutex.h"
#include "log.h"
#include <signal.h>
#include <pthread.h>

TClientMessage *pClientMessage = NULL;
pthread_t logger_thread;
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

   	pthread_sigqueue(msg_thread, SIGUSR2, value);
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
        							   message_list[m->GlobalFileName()].push_back(m->Message());
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
