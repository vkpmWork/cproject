#ifndef REMOTETHREAD_H
#define REMOTETHREAD_H

#include "clientmessage.h"
#include "file.h"

class RemoteThread
{
public:
    explicit RemoteThread(Mmessagelist, string, ulong, string);
    ~RemoteThread   ();
    bool empty_msg_list();
    void            RunWork();

private:

    Vmessage	    msg_list;
    Mmessagelist    append_list;

    string          Default_base_fileName;
    string          m_Address;
    ulong           m_Port;
    string          m_folder;
    filethread      *pfilethread;
    bool            fl_local_exists;
    int             local_counter;
    filethread      *pWrk_file;

    bool            SendToServer(int, string);
    bool            SendToServer(int, char*, size_t);
    ushort          UpdateFromLocalLog(char*);
    string          PrepareForSend(const Vmessage &, int &, int);

    void            finished();

    void            onMsg_list_append(Mmessagelist);

    Vmessage 		GetLocalMsgList();

};
extern void create_remote_thread();
extern pthread_t remote_thread;

#endif // REMOTETHREAD_H
