#ifndef CLIENTMESSAGE_H
#define CLIENTMESSAGE_H

#include "common.h"
#include "tlogmsg.h"
#include "loggermutex.h"

typedef vector<string> Vmessage; /* список сообщений от клиентов */
typedef map<string, Vmessage> Mmessagelist; /* domain/filename + сообщение */

#define MSG_TYPE	1
#define DEL_TYPE	2
#define CFG_TYPE	3

class TClientMessage
{
public:
    Mmessagelist    message_list;

    TClientMessage(pthread_t m_msg_thread, pthread_t m_error_thread, pthread_t m_remote_thread,
    			   Logger_namespace::tcStore store, ulong, uint, uint);
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
    pthread_t 	    msg_thread,
    				error_thread,
    				remote_thread;


    unsigned short  MaxStoreListSize;
    Logger_namespace::tcStore         CategoryStore;
    msgevent::tcEvent   m_cmd;
    uint            CheckPeriod;
    volatile  bool  FTransmitFromLocalLog;
    uint			ErrorLevel;

	struct TMess
	{
		uint  value;
		char *domain;
		char *msg;
	};

    inline    void  OnReadyTransmitMessage();
    inline    void  OnDeleteMessage(/*std::string*/);
    inline 	  void  OnTransmitError(uint, std::string, std::string);
    inline    int   get_messagelist_size();
};
extern pthread_t 	message_thread;
extern pthread_t 	remote_thread;

extern TClientMessage *pClientMessage;
void   create_message_handler_thread();
#endif // CLIENTMESSAGE_H
