#ifndef CLIENTMESSAGE_H
#define CLIENTMESSAGE_H

#include "common.h"
#include "tlogmsg.h"
#include <vector>
#include <map>

typedef vector<string> Vmessage; /* список сообщений от клиентов */
typedef map<string, Vmessage> Mmessagelist; /* domain/filename + сообщение */

#define MSG_TYPE	1
#define DEL_TYPE	2
#define CFG_TYPE	3

class TClientMessage
{
public:
    Mmessagelist    message_list;

    TClientMessage(pthread_t m_msg_thread, Logger_namespace::tcStore store, ulong, uint);
    void        AddMessage(TLogMsg *m);
    inline      msgevent::tcEvent Cmd()
                { return m_cmd; }
    inline void ClearCmd()
                { m_cmd = msgevent::evEmpty; }
    bool        TransmitFromLocalLog();
    bool        TransmitFromLocalLog(bool);

    void 		ClearMessageList();

    ~TClientMessage();
private:
    pthread_t 	    msg_thread;

    unsigned short  MaxStoreListSize;
    Logger_namespace::tcStore         CategoryStore;
    msgevent::tcEvent   m_cmd;
    uint            CheckPeriod;
    volatile  bool  FTransmitFromLocalLog;
    //QTimer         *pTimerTramsmitMessage;

    inline    void  set_transmit_timer(bool);
    inline    void  OnReadyTransmitMessage();
    inline    void  OnDeleteMessage(std::string);
};
extern TClientMessage *pClientMessage;
extern pthread_t logger_thread;
#endif // CLIENTMESSAGE_H
