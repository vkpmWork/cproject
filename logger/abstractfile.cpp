#include "abstractfile.h"
#include "log.h"
#include <fcntl.h>
#include <sys/stat.h>

tabstractfile::tabstractfile()
{
//    int aa = 5;
}

int tabstractfile ::ReadFileMode ()
{
    return O_RDONLY;
}

int tabstractfile ::AppendFileMode ()
{
    return O_CREAT | O_APPEND | O_WRONLY; /* | O_SYNC  проверить флаг O_SYNC*/
}

int tabstractfile ::TruncateFileMode ()
{
    return O_CREAT | O_TRUNC | O_WRONLY;
}

int tabstractfile ::FileClose(int pFile)
{
    if (pFile < 0) return pFile;

    if (close(pFile) == 0) pFile = -1;
    else
    {
        	 std::ostringstream m;
        	 m << "Close file error: " << strerror(errno);
        	 winfo(m);
    }
    return pFile;
}

int tabstractfile ::FileOpen(string fname, int flag)
{

    int pFile = open(fname.c_str(), flag, S_IWUSR | S_IWGRP); 

    if (pFile == -1)
    {
		 std::ostringstream m;
		 m << "Open file error: %s " << fname << " Error code: "<< strerror(errno);
		 winfo(m);
    }
    return pFile;
}

bool tabstractfile ::FileStat(int fd, struct stat &bufstat)
{
    if (fd < 0) return false;

    bool isOk = (fstat(fd, &bufstat) == 0) && (bufstat.st_mode & S_IFREG);
    if (!isOk)
    {
		 std::ostringstream m;
		 m << "File stat error FileStat(int...) " << strerror(errno);
		 winfo(m);
    }
    return isOk;
}

bool tabstractfile ::FileStat(string s, struct stat &bufstat)
{
    return stat(s.c_str(), &bufstat) == 0 && (bufstat.st_mode & S_IFREG);
}

size_t tabstractfile ::FileSize(int fd)
{
    size_t sz = 0;

    if (fd > -1)
    {
        struct stat bufstat;
        if (FileStat(fd, bufstat)) sz = bufstat.st_size;
    }
    return sz;
}

size_t tabstractfile ::FileSize(struct stat bufstat)
{
    return bufstat.st_size;
}

int tabstractfile :: FileDelete(string f)
{
    int err = 0;
    if (unlink(f.c_str()) == 0)
    {
        f.append(" - file successfuly deleted");
        //pInternalLog->LOG_OPER( levWarning, f);
    }
    else err = errno;

    return err;
}

void tabstractfile ::WriteFile(int pFile, string s)
{
    if (pFile < 0) return;

    size_t sz  = s.size();

    try
    {
        if (write(pFile, s.c_str() , sz) != (long)sz)
        {
			 std::ostringstream m;
			 m << "File write error: " << strerror(errno);
			 winfo(m);
        }
    }
    catch(...) {}

}

void tabstractfile ::FsyncFile(int fd)
{
    if (fd > -1) fsync(fd);
}

bool  tabstractfile :: FileExists(int fd)
{
    bool isOk = false;

    struct stat bufstat;
    if ((fd > 0) && FileStat(fd, bufstat)) isOk = bufstat.st_nlink != 0; /* 0 - Нет жестких ссылок - висячий дискриптор*/

    return isOk;
}

bool  tabstractfile :: FileExists(struct stat bufstat)
{
    return bufstat.st_nlink != 0;  /* 0 - Нет жестких ссылок - висячий дискриптор*/
}

bool  tabstractfile :: FileExists(string f)
{
    struct stat bufstat;
    bool isOk = stat(f.c_str(), &bufstat) == 0 && (bufstat.st_mode & S_IFREG);

    if (!isOk)
    {
        f.append("- File not Exists");
 //       //pInternalLog->LOG_OPER( levWarning, f);
    }
    return isOk;
}

bool tabstractfile :: MakeDirectory(string dir, mode_t mode, uid_t uid, gid_t gid)
{
        if (dir.empty()) return false;
        if(dir[dir.size()-1] != DELIMITER) dir += DELIMITER;

        bool mdret = mkdir(dir.c_str(), ConvertToDecFromOct(DEFAULT_DIR_MODE)) == 0 || errno == EEXIST;
	
	if (errno == EEXIST) return true; // 09.12.14
	
        if (!mdret)
        {
            
            char s[100];
            sprintf(s,"Couldn't' create directory %s. Permission denied!", dir.c_str());
            LOG_OPER(s);
            return false;
        }

       string str;
       if (uid != -1 || gid != -1)
            if (chown(dir.c_str(), uid, gid) == -1 && pInternalLog)
            {
                str = "tabstractfile :: MakeDirectory. Couldn't change directory user name or group name.";
                str.append(strerror(errno));
            }

        if (mode > 0)
           if (chmod(dir.c_str(), mode) == -1 && pInternalLog)
           {
               str.append("\ntabstractfile :: MakeDirectory. Couldn't change directory mode.");
               str.append(strerror(errno));
           }

        if (!str.empty()) pInternalLog->LOG_OPER(levWarning, str);
        return (bool) mdret;
}

bool tabstractfile :: MakeDirectory(string sf, string s, mode_t mode, uid_t uid, gid_t gid)
{
    if (s.empty() || MakeDirectory(sf, mode, uid, gid) == false) return false;

    size_t pre=0,pos;
    string dir;
    bool mdret;

    //if(s.at(0) != DELIMITER && s.at(0) != '.') s = DELIMITER + s;
    if(s[s.size()-1] != DELIMITER) s += DELIMITER;

    while((pos=s.find_first_of(DELIMITER,pre))!=std::string::npos)
    {
        dir=sf + s.substr(0,pos++);
		
        pre=pos;

        if(!dir.size()) continue; // if leading / first time is 0 length

        mdret = mkdir(dir.c_str(), ConvertToDecFromOct(DEFAULT_DIR_MODE)) == 0 || errno == EEXIST;

	if (errno == EEXIST) continue; // 09.12.14

        if (!mdret)
        {
            char s[100];
            sprintf(s,"Couldn't' create directory %s. Permission denied!", dir.c_str());
            LOG_OPER(s);
            break;
        }

        string str;
        if (uid != -1 || gid != -1)
            if (chown(dir.c_str(), uid, gid) == -1 && pInternalLog)
            {
                str = "tabstractfile :: MakeDirectory. Couldn't change directory user name or group name.";
                str.append(strerror(errno));
            }

        if (mode > 0)
           if (chmod(dir.c_str(), mode) == -1 && pInternalLog)
           {
               str.append("\ntabstractfile :: MakeDirectory. Couldn't change directory mode.");
               str.append(strerror(errno));
           }

        if (!str.empty()) pInternalLog->LOG_OPER(levWarning, str);
    }
    return (bool) mdret;
}
