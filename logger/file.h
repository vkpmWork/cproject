#ifndef FILE_H
#define FILE_H

#include "clientmessage.h"
#include "abstractfile.h"

class tlocalfile : public tabstractfile
{
public  :
    explicit   tlocalfile(string, tabstractfile *pfile = 0);
    ~tlocalfile();
    bool       FileExists(int);
    int        FileOpen  (string);
    void       DeleteFile();
    bool       NextBuffer();
    bool       GetNextBuffer();
    char*      Getlocalfilebuffer();
    inline size_t   GetBufferSize() {return m_MessageSize;}

private :
    string     Default_base_fileName;
    off_t      LastFilePos;
    int        pFile;
    size_t     m_BufferSize;
    void       Clearfilebuffer();
    long       MaxTransferSize;
    char       *plocalfilebuffer;
    off_t      LocalFileBufferPos;
    size_t     m_MessageSize;
};

class filethread : public tabstractfile
{
public :

    explicit    filethread(Mmessagelist m_list);
    ~filethread();


//    bool          RunLocal(Mmessagelist, Mmessagelist::iterator, Mmessagelist::iterator) ;
    bool          someError() { return SomeError; }
    int           DeleteFile(int, string);
    void          UnlinkLocalFile();
    bool          SomeError;
    bool          empty_msg_list();
    Mmessagelist  GetLocalMsgList() {}
    void          RunWork();

private:
    string          Default_base_fileName,
                    Default_base_filePath;
    struct
    {
       string       StrInMemory;
       string       FileName,
                    FilePath;
       string       _FileName() { return FilePath + FileName;}
    } Memo;
    string          Path;
    ulong           MaxFileSize;
    Logger_namespace::tcStore         CategoryStore;
    ulong           Max_archive_count;

    uid_t           Owner_user;
    gid_t           Owner_group;
    bool            fl_changeowner;
    mode_t          Mode_Dir;
    mode_t          Mode_File;

    Mmessagelist    msg_list;
    pthread_mutex_t msg_lock;

    bool            CompareFileName(string);
    void            Write(std::string m_f, std::string s);
    void            TryToWrite(TLogMsg*);
    void            UpdateMemoInfo(TLogMsg*);
    void            AppendStrInMemo(string);

    int             OpenFileForAppend(string);
    int             OpenFileForTruncate(string);  /* */
//    inline   int    CheckFileForWriting(string);
    bool            CheckFileDirectory(string m_file);
    string          CheckFileName(string);
    void            ClearMemo()             { Memo.FileName.clear(); Memo.StrInMemory.clear(); Memo.FilePath.clear();}
    void            RecreateFileList(string m_f);

    inline   void  TryToDeleteFile(TLogMsg*);
    inline   int   FileDelete(string f);
    inline   bool  CheckFileSize(string);
    int            FileOpen(string, int);
    bool           CheckOwner(string, string);
    void           ChangeOwner(int);
    void           ChangeMode(int);
    inline void    SetLock();
    inline void    SetUnlock();

    void           onMsg_list_append(Mmessagelist);

/*
public slots:
     void           RunWork();
signals     :
    void            finished();
    void            continue_work();
*/
};

#endif // FILE_H
