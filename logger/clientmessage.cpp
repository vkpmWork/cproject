#include "clientmessage.h"
#include "loggermutex.h"
#include "log.h"
#include <signal.h>

TClientMessage *pClientMessage = NULL;

TClientMessage::TClientMessage(Logger_namespace::tcStore store , ulong maxStoreListSize, uint checkPeriod)
  : MaxStoreListSize(maxStoreListSize)
  , CategoryStore(store)
  , m_cmd(msgevent::evEmpty)
  , CheckPeriod(checkPeriod)
{
    message_list.clear();

/*    pTimerTramsmitMessage = new QTimer();
    if (pTimerTramsmitMessage)
    {
        pTimerTramsmitMessage->setInterval(CheckPeriod);
        connect(pTimerTramsmitMessage, SIGNAL(timeout()),       SLOT(onTransmit_timer()),             Qt::DirectConnection);

        pTimerTramsmitMessage->start();
    }
*/
    FTransmitFromLocalLog = false;
}
/*
void TClientMessage::onTransmit_timer()
{

    if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexLock();
    OnReadyTransmitMessage();
    if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexUnlock();

}
*/

void TClientMessage::OnReadyTransmitMessage()
{
   // set_transmit_timer(false); какой-то косяк в самом Qt. В новой версии можно открыть :)
    if (message_list.size())
    {
//        emit ready_to_save(message_list);
        message_list.clear();
    }
  //  set_transmit_timer(true);
}

void    TClientMessage::AddMessage(TLogMsg *m)
{
   if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexLock();

   m_cmd = msgevent::evEmpty;
   switch(m->Event())
   {
        case msgevent::evMsg     : message_list.push_back(m->Message());
                                   break;
        case msgevent::evDelete  : message_list.push_front(m->Message());
                                   break;
        case msgevent::evConfig  : m_cmd = msgevent::evConfig;break;
        case msgevent::evExit    : kill(getpid(), SIGTERM);   break;
        default                  : break;
   }

 //  emit add_error(m->error(), (char*)m->domain().c_str(), (char*)m->Message().c_str());

   if (message_list.size() >= MaxStoreListSize) OnReadyTransmitMessage();

   if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexUnlock();

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


TClientMessage::~TClientMessage()
{
/*
    if (pTimerTramsmitMessage)
    {
        pTimerTramsmitMessage->stop();
        delete pTimerTramsmitMessage;
        pTimerTramsmitMessage = NULL;
    }
*/
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
