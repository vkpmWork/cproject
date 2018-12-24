#ifndef REMOTETHREAD_H
#define REMOTETHREAD_H

#include "clientmessage.h"
#include "file.h"

class RemoteThread
{
public:
    explicit RemoteThread(string, ulong, string, size_t);
    ~RemoteThread   ();
    bool 			empty_msg_list() {return msg_list.empty();}
    void            RunWork(Mmessagelist);
    void			SaveRemoteFile();
private:

    Vmessage	    msg_list;

    string          Default_base_fileName;
    string          m_Address;
    ulong           m_Port;
    string          m_folder;
    filethread      *pfilethread;
    bool            fl_local_exists;
    size_t          m_size_in_memo;
    filethread      *pWrk_file;

    void			append(Mmessagelist);
};
extern void create_remote_thread();

#endif // REMOTETHREAD_H
