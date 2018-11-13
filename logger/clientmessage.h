#ifndef CLIENTMESSAGE_H
#define CLIENTMESSAGE_H

#include "common.h"
#include "tlogmsg.h"

class TClientMessage
{
    TClientMessage(Logger_namespace::tcStore store, ulong, uint);
    void        AddMessage(TLogMsg *m);
    inline      msgevent::tcEvent Cmd()
                { return m_cmd; }
    inline void ClearCmd()
                { m_cmd = msgevent::evEmpty; }
    bool        TransmitFromLocalLog();
    bool        TransmitFromLocalLog(bool);

    ~TClientMessage();
private:
    v_messagelist   message_list;

    unsigned short  MaxStoreListSize;
    Logger_namespace::tcStore         CategoryStore;
    msgevent::tcEvent   m_cmd;
    uint            CheckPeriod;
    volatile  bool  FTransmitFromLocalLog;
    //QTimer         *pTimerTramsmitMessage;

    inline    void  set_transmit_timer(bool);
    inline    void  OnReadyTransmitMessage();
};
extern TClientMessage *pClientMessage;

#endif // CLIENTMESSAGE_H
