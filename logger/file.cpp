#include "file.h"
#include <dirent.h>
#include <sys/stat.h>
#include "clientmessage.h"
#include "loggermutex.h"
#include "logsystemsetup.h"
#include <pwd.h>
#include <grp.h>
#include <QDebug>


string fileName;

std::string::size_type GetPos(string s, const char c, std::string::size_type n)
{
    std::string::size_type value = std::string::npos;

    if (n) value = s.find_last_of(c, n);
    if (value == std::string::npos) value = s.find_last_of(c);

    return  value;
}

int sel (const struct dirent *d)
{
    if ( !strcmp(".", d->d_name) || !strcmp("..", d->d_name)) return 0;
    string dname = d->d_name;

    std::string::size_type V_fileName = GetPos(fileName, '.', 0);
    if (V_fileName == std::string::npos) return 0;

    std::string::size_type V_dname = GetPos(dname, '.', 0);
    if (V_dname == std::string::npos) return 0;

    if (fileName.compare(V_fileName+1, fileName.length() - V_fileName, dname.substr(V_dname+1)) != 0) return 0;

    V_fileName = GetPos(fileName, '.', V_fileName-1);

    return fileName.compare(0, V_fileName, dname.substr(0, GetPos(dname, '.', V_dname-1))) == 0;
}

//-----------------------------------------------------

filethread::filethread(v_messagelist m_list, QObject *parent, tabstractfile *):
    QObject(parent)
  , tabstractfile()
  , msg_list(m_list)
{
    pthread_mutex_init(&msg_lock, NULL);

    CategoryStore  = ptrLoggerSetup->CurrentStoreType;

    Path           = ptrLoggerSetup->Folder();
    MaxFileSize    = ptrLoggerSetup->max_size();
    fl_changeowner = false;
    Mode_Dir       = ptrLoggerSetup->mode_dir_local();
    Mode_File      = ptrLoggerSetup->mode_file_local();
    Max_archive_count = ptrLoggerSetup->max_archive_count();

    ClearMemo();

    fl_changeowner = CheckOwner(ptrLoggerSetup->owner_user_local(), ptrLoggerSetup->owner_group_local());

    SomeError             = !MakeDirectory(Path, Mode_Dir, Owner_user, Owner_group);
    Default_base_fileName = DEFAULT_BASE_FILE_NAME;

    Default_base_filePath = Path+Default_base_fileName;
    if (CategoryStore == LOCAL_STORE) connect(this, SIGNAL(continue_work()), this, SLOT(RunWork()), Qt::QueuedConnection);

}

filethread :: ~filethread()
{

    pthread_mutex_destroy(&msg_lock);
}

void filethread::SetLock()
{
    pthread_mutex_lock(&msg_lock);
}

void filethread::SetUnlock()
{
    pthread_mutex_unlock(&msg_lock);
}

void filethread :: onMsg_list_append(v_messagelist m_list)
{
    v_messagelist m = m_list;
    if (!m.size()) return;

    SetLock();
    msg_list.insert(msg_list.end(), m.begin(), m.end());
    SetUnlock();
    RunWork();
}

bool filethread ::empty_msg_list()
{
    bool fl = true;

    SetLock();
    fl = msg_list.empty();
    SetUnlock();

    return fl;
}

void filethread :: RunWork()
{
    SetLock();
     v_messagelist msg = msg_list;
     msg_list.clear();
     MasCounter -= msg.size();
    SetUnlock();

    TLogMsg m;
    string  s  = msg.front();
    m.Message(s);

    UpdateMemoInfo(&m);
    while (msg.size())
    {
        s = msg.front();
        msg.pop_front();

        m.Message(s);

        TryToWrite(&m);

    }
   Write();

   if (empty_msg_list()) emit finished();
   else RunWork();
}

bool filethread :: RunLocal(v_messagelist m_list, v_messagelist::iterator it, v_messagelist::iterator it_last)
{
        bool return_value = false;

        string s, m;
        s.clear();
        m.clear();
        ulong sz = 0;

        char *ch = (char*)malloc(2);

        while(it != it_last)
        {

            m = *it;
            memset(ch, 0, 2);

            sz = m.size() + 1 /* MARKER_END */;

            memmove(ch, &sz, sizeof(ushort));
            s.append(ch, 2);
            s.append(m);
            s.append(MARKER_END);

            sz = s.size();
            it++;

            if (sz >= MAX_MESSAGE_SIZE || (it == it_last))
            {

                int fd = OpenFileForAppend(Default_base_filePath);
                if (fd > -1)
                {

                    WriteFile(fd, s);
                    FileClose(fd);
                    //FsyncFile(fd);
                    s.clear();
                    if (!return_value) return_value = true;
                }

            }
        }

        free(ch);
        m_list.clear();

        return return_value;
}


void filethread :: TryToWrite(TLogMsg* m)
{
    if (CategoryStore == LOCAL_STORE)
    {
        if (m->Event() == msgevent::evMsg)
        {   if (CompareFileName(Path + m->GlobalFileName()) == false)
            {
                Write();
                UpdateMemoInfo(m);
            }
        }
        else if (m->Event() == msgevent::evDelete)
             {
                  TryToDeleteFile(m);
                  return;
             }
    }

    AppendStrInMemo(CategoryStore == LOCAL_STORE ? m->msg() : m->Message());
}

void filethread :: Write()
{
    if (Memo.StrInMemory.empty()) return;
    if (!CheckFileDirectory())    return;

    if (pLoggerMutex) pLoggerMutex->Write_MutexLock();

    int  pFile = OpenFileForAppend(Path+Memo._FileName()); //CheckFileForWriting(Path+Memo._FileName());

    if (pFile >= 0)
    {
        WriteFile(pFile, Memo.StrInMemory);

        size_t sz = tabstractfile::FileSize(pFile);

        FileClose(pFile);

        if (CategoryStore == LOCAL_STORE && sz > (size_t)MaxFileSize) RecreateFileList();
    }

    if (pLoggerMutex) pLoggerMutex->Write_MutexUnlock();
}

bool filethread :: CompareFileName(string str)
{
      return strcmp((Path+Memo._FileName()).c_str(), str.c_str()) == 0;
}

void filethread :: UpdateMemoInfo(TLogMsg* m)
{
    ClearMemo();

    if (CategoryStore == LOCAL_STORE)
    {
        Memo.FileName = m->filename();
        Memo.FilePath = m->FilePath();
    }
    else
    {
        Memo.FileName = Default_base_fileName;
        Memo.FilePath = "";
    }
    SomeError = Memo.FileName.empty() || Memo.FilePath.empty();
}

void filethread :: AppendStrInMemo(string str)
{
    /* Это тоже надо как-то реализовать if (Memo.StrInMemory.size()+str.length() >= MaxFileSize) Memo.StrInMemory.clear();*/

    Memo.StrInMemory.append(str);
    Memo.StrInMemory.append(MARKER_END);
}

string filethread :: CheckFileName(string str)
{
    if (str.empty()) str = DEFAULT_BASE_FILE_NAME;

    return str;
}

bool filethread::CheckFileDirectory()
{
    SomeError = !MakeDirectory(Path, Memo.FilePath, Mode_Dir, Owner_user, Owner_group);
    return SomeError == false;
}
/*
size_t filethread :: FileSize  (int fd)
{
    return tabstractfile::FileSize(fd);
}
*/

int    filethread :: OpenFileForAppend(string s)  /* открываем файл для дозаписи */
{
    bool file_exists = FileExists(s);
    int fd = tabstractfile::FileOpen(s, AppendFileMode());
    if (fd > 0)
    {
        if (!file_exists)
        {
            ChangeOwner(fd);
        	ChangeMode(fd);
        }
    }
    return fd;
}

int    filethread :: OpenFileForTruncate(string s)  /* */
{
    return tabstractfile::FileOpen(s.c_str(), TruncateFileMode());
}

void     filethread :: RecreateFileList()
{
    string dir     = Path + Memo.FilePath,
                     fileExt = "";

    fileName       = Memo.FileName;
//    struct stat      buf;
    struct dirent ** entry;

    uint n = scandir(dir.c_str(), &entry, sel, alphasort);

    if (!n) return;

    fileExt  = fileName.substr(GetPos(fileName, '.', 0)+1); /* маска расширения */
    fileName = fileName.substr(0, GetPos(fileName, '.', GetPos(fileName, '.', 0)-1)); /* маска имени файла */

    char f[FILENAME_MAX];

    string str;
    ulong index = n < Max_archive_count ? 0 : n - Max_archive_count;

    for (uint i = 0;i < n; i++)
    {
        str = dir+entry[i]->d_name;

        if (i < index) FileDelete(str);
        else
        {
            sprintf(f, "%s%s.%05lu.%s", dir.c_str(), fileName.c_str(), i - index, fileExt.c_str());

            if (rename(str.c_str(), f) != 0)
            {   sprintf(f, "Не могу переименовать файл %s\n", str.c_str());
                LOG_OPER(f);
            }

        }
    }
    for (uint i = 0; i < n; free(entry[i++]));
    free(entry);
}
/*
int  filethread :: CheckFileForWriting(string s)
{
    return OpenFileForAppend(s);
}
*/
inline void filethread :: TryToDeleteFile(TLogMsg *m)
{
    list<string>ListFile;
    string str = m->fullfilename();

    while (true)
    {
        size_t pos = str.find_first_of(';');

        string s = pos != string::npos ? str.substr(0, pos) : str;
        if (!s.empty())
        {
            if (s[0] != DELIMITER) s = DELIMITER + s;
            ListFile.push_back(s);
        }
        if (pos != string::npos) str = str.substr(pos+1);
        else break;
    }

    if (!ListFile.size()) return;

    while(ListFile.size())
    {
        str = ListFile.front();
        ListFile.pop_front();

        if (str.empty())  return;

        str = Path + m->domain() + str;
        FileDelete(str);
    }

}

inline int filethread :: FileDelete(string f)
{
    if (f.at(0) != DELIMITER) f.insert(0, STRDELIMITER);

    int err = tabstractfile::FileDelete(f);

    if (!err) return 0;
    else if (err != EISDIR)
         {
                pInternalLog->LOG_OPER( levWarning, strerror(err) + f);
                return 0;
         }

    if (f.at(f.size()-1) != DELIMITER) f.append(STRDELIMITER);

    //size_t sz = f.length();
    char *s = new char[PATH_MAX];
    if (s)
    {
        memset(s, 0, PATH_MAX);
        strcpy(s, f.c_str());
        f = ClearDirectory(s);
        delete [] s; s = NULL;
        pInternalLog->LOG_OPER(levWarning, f);
    }
    return 0;
}

void filethread ::UnlinkLocalFile()
{
    FileDelete(Default_base_filePath);
}

/*void filethread :: ExecuteCommand(TLogMsg* m)
{
    TryToDeleteFile(m);
}
*/


bool filethread ::CheckFileSize(string s)
{
    struct stat st;
    bool   szOk = true;

    if (FileStat(s, st))  szOk = tabstractfile::FileSize(st) < (size_t) MaxFileSize;
    return szOk;
}

int filethread ::FileOpen(string fname, int flag)
{

    int pFile = -1;

    if (FileExists(fname)) pFile = tabstractfile::FileOpen(fname.c_str(), flag);
    return pFile;
}

bool filethread ::CheckOwner(string owneruser, string ownergroup)
{
    if (owneruser.empty() && ownergroup.empty() ) return false;

    struct passwd *pw = getpwnam(owneruser.c_str()); /* узнаем uid пользователя по его имени */

    if (pw) Owner_user  = pw->pw_uid;
    else
    {    if (pInternalLog) pInternalLog->LOG_OPER("User named not found : " + owneruser);
         Owner_user  = 0;
    }

    struct group *gr = getgrnam(ownergroup.c_str());
    if (gr) Owner_group = gr->gr_gid;
    else
    {    if (pInternalLog) pInternalLog->LOG_OPER("Group named not found : " + ownergroup);
         Owner_group = 0;
    }

    return (Owner_user != 0 || Owner_group != 0);
}

void filethread ::ChangeOwner(int fd)
{
    if (fd == -1 || fl_changeowner == false) return;

    struct stat bufstat;
    if (FileStat(fd, bufstat) )
    {
        uid_t ow_user  = 0;
        gid_t ow_group = 0;

        if (bufstat.st_uid != Owner_user)  ow_user  = Owner_user;
        if (bufstat.st_gid != Owner_group) ow_group = Owner_group;

        if (ow_user != 0 || ow_group != 0)
             if (fchown(fd, ow_user, ow_group) == -1 && pInternalLog) pInternalLog->LOG_OPER(QString("Can't' change LogFile.log owner or group. %1").arg(strerror(errno)).toStdString().c_str());
    }
}

void filethread ::ChangeMode(int fd)
{
    if (fd == -1 || Mode_File == 0) return;

    struct stat bufstat;
    if (FileStat(fd, bufstat) )
        if (bufstat.st_mode != Mode_File)
             if (fchmod(fd, Mode_File) == -1 && pInternalLog)
             {
                 string s = "ChangeMode(int fd). Can't' change file mode.";
                 s.append(strerror(errno));
                 pInternalLog->LOG_OPER(s);
             }
}


int filethread ::DeleteFile(int pFile, string s)
{
    if (FileExists(pFile))
    {
            pFile = FileClose(pFile);
            FileDelete(s /*Default_base_fileName*/);
    }
    return pFile;
}



v_messagelist filethread ::GetLocalMsgList()
{

    v_messagelist m;

     m.clear();


     if (FileExists(Default_base_filePath))
     {
            int pFile = -1;

            if (pLoggerMutex) pLoggerMutex->RemoteRunnable_MutexLock();

            pFile = FileOpen(Default_base_filePath, ReadFileMode());

            if (pLoggerMutex) pLoggerMutex->RemoteRunnable_MutexUnlock();

            if (pFile > -1)
            {
                char sz_short = MSG_SIZE;
                size_t flength   = FileSize(pFile);
                string  str   = "";
                if (flength> 0)
                {
                    char *buf = (char*)malloc(flength+1);
                    memset(buf, 0, flength);

                    if (buf)
                    {
                        flength = read(pFile, buf, flength);

                        if (flength > (size_t)sz_short)
                        {
                            int    pos   = 0;
                            ushort sz    = 0;

                            while ((size_t)pos < flength)
                            {
                                memcpy(&sz, buf + pos, sz_short);

                                pos += sz_short;
                                if (sz)
                                {
                                    str.assign(buf + pos, (sz-1));
                                    pos += sz;
                                    if (str.size()) m.push_back(str);
                                    //str.clear();
                                }

                            }
                        }
                        free(buf);
                    }
                }
                pFile = FileClose(pFile);
            }
     }
     return m;
}

/* --------------------------------------------------- */
/* --------------------------------------------------- */
/* --------------------------------------------------- */

tlocalfile::tlocalfile(string path, tabstractfile *)
    :  tabstractfile()
    ,  Default_base_fileName(path + DEFAULT_BASE_FILE_NAME)
    ,  LastFilePos(0)
    ,  pFile(-1)
    ,  MaxTransferSize(200000)
    ,  LocalFileBufferPos(0)
    ,  m_MessageSize(0)

{
    plocalfilebuffer = new char[MaxTransferSize];
}

tlocalfile::~tlocalfile()
{
    Clearfilebuffer();
}

bool tlocalfile::FileExists(int fd)
{
    bool value = fd == -1 ? false : tabstractfile::FileExists(fd);
    return value;
}

int tlocalfile::FileOpen  (string s)
{
    if (pLoggerMutex) pLoggerMutex->RemoteRunnable_MutexLock();
    if (!FileExists(pFile))
    {
        LastFilePos = m_BufferSize = 0;

        pFile = tabstractfile::FileOpen(s, ReadFileMode());
    }
    if (pLoggerMutex) pLoggerMutex->RemoteRunnable_MutexUnlock();

    return pFile;
}

void  tlocalfile::DeleteFile()
{
    FileClose(pFile);
    pFile = -1;
    FileDelete(Default_base_fileName);
}

bool tlocalfile::NextBuffer()
{
    LastFilePos += m_BufferSize;
    size_t fsize = FileSize(pFile);
    return (fsize && ((off_t)fsize > LastFilePos));

}

bool tlocalfile::GetNextBuffer()
{
 //   struct stat statbuf;

    long flength = MaxTransferSize;//MAX_MESSAGE_SIZE;

    size_t fsize = FileSize(pFile);
    if (fsize == 0) return 0;

    //if (FileSize !=(flength - shift) < sz_short fsize) FileSize = fsize;

//    if ((fsize - (size_t)LastFilePos) < flength) flength = fsize;

    if(!plocalfilebuffer) return 0;

    memset(plocalfilebuffer, 0, MaxTransferSize);
    m_BufferSize = 0;

    int off = lseek(pFile, LastFilePos, SEEK_SET);

    if (off == -1)
        return 0;

    char *fbuf = new char[flength+1];
    memset(fbuf, '\0', flength+1);

    char sz_short  = MSG_SIZE;

    flength = read(pFile, fbuf, flength);

    if (flength <= sz_short)
    {
        delete [] fbuf; fbuf = NULL;
        return 0;
    }

    long shift = 0;
    ushort sz = 0;
    while ((flength - shift) > 2)
    {

            memcpy(&sz, fbuf + shift, sz_short);

            if ((flength - (shift + sz + sz_short)) < 0)
                break;

            shift += (sz + sz_short);
    }

    memcpy(plocalfilebuffer, fbuf, m_BufferSize = shift);
    delete [] fbuf; fbuf = NULL;

    LocalFileBufferPos = m_MessageSize = 0;
    LastFilePos       += m_BufferSize;
    return m_BufferSize > 0;
}

void tlocalfile::Clearfilebuffer()
{
    if (plocalfilebuffer)
    {
        delete [] plocalfilebuffer;
        plocalfilebuffer = NULL;
    }
}

char* tlocalfile::Getlocalfilebuffer()
{
    char *pos          = NULL;

    if (plocalfilebuffer)
        string          PrepareForSend(v_messagelist &);
    {
        size_t sz_short  = MSG_SIZE;
        ushort sz = 0;
        LocalFileBufferPos += m_MessageSize;

        if ((LocalFileBufferPos + sz_short) < m_BufferSize)
        {
            memcpy(&sz, plocalfilebuffer + LocalFileBufferPos, sz_short);
            sz_short += sz;
            if (LocalFileBufferPos + sz_short <= m_BufferSize)
            {    m_MessageSize = sz_short;
                 pos =  plocalfilebuffer + LocalFileBufferPos;
            }
            else m_MessageSize = 0;
        }

    }
    return pos;
}

