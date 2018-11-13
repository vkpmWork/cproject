/*
 * common.cpp
 *
 *  Created on: 26.01.2016
 *      Author: irina
 */
#include "common.h"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
//#include <cstring>
//using namespace std;

namespace common
{

#define STRDELIMITER  "/"

/* =================== prototypes ================= */
int    CheckLockPIDExists(const char *const lockFileName);
int    SetLock(int, short);

/* ================================================ */

const int 	TYPE_INTERNAL    = 1;
const int 	TYPE_LUA		 = 2;

char	   *application_name = NULL;
char 	   *pidfile_name	 = NULL;
/* Особенности врсии:
 * 1. Переход на reconnect, усди связь с icecast прервалась
 * 2. В команду ps добавлена опция -a для отображения процессов по маске для всех пользователей
 * */
const char *version          = "Application name: %s  Version 10.2 от 08.11.2018";
bool        flag_working     = false;

unsigned int time_start = 0;
unsigned int time_stop  = 0;

bool FileExists(char* str)
{
 	bool v = (access(str, 0) == 0);
 	if (!v)
 	{
/* 		logger::message.str("");
		logger::message << errno << " " << (unsigned int)time(0) << " " << str << " : " << strerror(errno) << std::endl;
 		logger::write_info(logger::message, levWarning); */
 	}
	return v;
}

bool FileStatExists(char* str)
{
//        logger::message.str("");
	struct stat sb;

	if (stat(str, &sb) == -1)
	{
//	    logger::message << errno << " " << (unsigned int)time(0) << " " << str << " : " << strerror(errno) << std::endl;
//	    logger::write_info(logger::message);
	    return false;
	}


	bool v = (sb.st_size != 0);
 	if (!v)
 	{
 //	    logger::message << ERR_FILE_EMPTY << " " <<  str << " : Something wrong (file is empty) with file" << endl;
//	    logger::write_info(logger::message);
 	}
	return v;
}

bool FileStatExists(char* str, std::string &s)
{

        struct stat sb;
        std::ostringstream m;

        if (stat(str, &sb) == -1)
        {
            m << errno << " " << (unsigned int)time(0) << " " << str << " : " << strerror(errno) << std::endl;
            s.append(m.str());
            return false;
        }


        bool v = (sb.st_size != 0);
        if (!v)
        {
  //          m << ERR_FILE_EMPTY << " " <<  str << " : Something wrong (file is empty) with file" << endl;
            s.append(m.str());
        }
        return v;
}

char *pathname(char* m_file)
{ /* возвращает указатель на распределенную память внутри функии. Требует free(распределенной памяти)*/

	char *a = (char*) calloc(strlen(m_file), sizeof(char));
	char *b = basename(m_file);

	strncpy(a, m_file, (b-m_file));

	return a;
}

void ApplicationVersion()
{
	if (application_name) printf(version, application_name);
//	else printf("Какая-то фигня!");
}

void DisposeCommon()
{
//	unlink(pidfile_name);
	std::cout << printf("The End!\n");
	free(application_name);
	free(pidfile_name);

}

pid_t getPid()
{
    return getPid(pidfile_name);
}

pid_t getPid(char* m_pidfile_name)
{
    if (!m_pidfile_name) return 0;

    pid_t  m_pid = 0;

    char   *f_pid = strdup(m_pidfile_name);

    int lockFD=open(f_pid,O_RDONLY);
    free(f_pid); /* больше он не нужен */

    if (lockFD < 0)
    {
        printf("PID-file isn't found. Probably the application isn't started.\n");
        exit(lockFD);
    }

    char *pid_buf =  (char*)calloc(16, sizeof(char));

    if (pid_buf)
    {
        read(lockFD, pid_buf, 16);
        m_pid = atoi(pid_buf);
        free(pid_buf);
    }
    close(lockFD);

    return m_pid;
}
int BecomeDaemonProcess()
{
    int curPID = fork();

    switch(curPID)
    {
        case 0: /* we are the child process */
                  LOG_OPER("\nDeamon Started");
                  break;

        case -1:  printf("Error fork()!!! ");
                  return -1;
                  break;

        default:  /* We are the parent, so exit */
                  exit(0);
                  break;
    }

    if (setsid() < 0)
    {
        printf("setsid() < 0\n");
        return -1;    /* создаем новый сеанс, делаем лидером, не связываем с терминалом */
    }

    /* закрываем все файловые дескрипторы, открытые в нашем процессе */
/*
    long numFiles = sysconf(_SC_OPEN_MAX);  // how many file descriptors?
    for ( long i = numFiles - 1; i >= 0; --i)
    {
        close(i);
    }
*/
    umask(0);

//    int stdioFD=open("/dev/null",O_RDWR); /* fd 0 = stdin */
//    if (stdioFD != -1)
//    {
//        dup(stdioFD);/* fd 1 = stdout */
//        dup(stdioFD); /* fd 2 = stderr */
//    }

    /* put server into its own process group. If this process now spawns
        child processes, a signal sent to the parent will be propagated
        to the children */
    setpgrp();
    return 0;
}


int    CheckLockPIDExists(const char *const lockFileName)
{
    int lockFD=open(lockFileName,O_RDONLY,0777);

    if (lockFD != -1)
    {
        char	pidBuf[17];
        if (read(lockFD, pidBuf, 16) > 0)
        {

            unsigned long lockPID = strtoul(pidBuf, (char**)0, 10);

            if (kill(lockPID,0) == 0) printf("PID-file %s is found. Deamon with PID = %i is already started\n" ,  lockFileName, (int)lockPID);
            else if (errno == ESRCH)
                 {
                    printf("PID-file %s is found. Process with PID = %i could stopped abnormally.\n" , lockFileName, (int)lockPID);
                    ssize_t aa = SetLock(lockFD, F_UNLCK);
                    close(lockFD);

                    if (aa != -1)
                    {    if (unlink(lockFileName) == 0) printf("PID-file %s successfully removed\n" , lockFileName);
                         else printf("Remove PID-file %s\n" , lockFileName);
                    }
                    return -1;
                 }
                 else printf("PID-file %s is found. But any trouble with it!\n" , lockFileName);

        }
        else printf("Can't' read file %s\n" , lockFileName);
        close(lockFD);
    }

    return lockFD;
}

int SetLock(int lockfd, short ltype)
{
    struct flock exclusiveLock;
    exclusiveLock.l_type   = ltype;//  F_UNLCK - сброс блокировки, F_WRLCK;                 /* exclusive write lock */
    exclusiveLock.l_whence = SEEK_SET;                /* use start and len */
    exclusiveLock.l_len    = exclusiveLock.l_start=0; /* whole file */
    exclusiveLock.l_pid    = 0;                       /* don't care about this */

    /* F_SETLK = Установить блокировку сегмента файла в соответствии со значением структуры типа flock */
    return fcntl(lockfd,F_SETLK,&exclusiveLock);
}

void     		LOG_OPER(string format_string, ...)
{

}


char *xstrlower(char *s)
{
	char *ptr = s;

	while(*ptr)
	{
		*ptr = tolower(*ptr);
		ptr++;
	}
	return s;
}

char * xstrupper(char *s)
{
	char *ptr = s;

	while(*ptr)
	{
		*ptr = toupper(*ptr);
		ptr++;
	}
	return s;
}

string urlEncode(string str)
{
    string new_str = "";

    unsigned char  c;
    const    char* chars = str.c_str();
    char     bufHex[6];

     while( *chars )
    {
        c = (unsigned char) *chars;//[i];
        // uncomment this if you want to encode spaces with +
        /*if (c==' ') new_str += '+';
        else */if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') new_str += c;
        else
        {
        	sprintf(bufHex,"%02X",c);

            new_str += "%";
            new_str.append(bufHex, 2);
        }
        chars++;
    }
    return new_str;
 }

char *xtrim(char *str)
{
	char *ptr = str;
	int  i = 0;

	while(*ptr)
	{
		if (*ptr != ' ')
		{
			 i = ptr-str;
		 	 break;
		}
		ptr++;
	}

	int len = strlen(str);
	if (i)
	{
		memmove(str, str+i, len - i);
		for (int j = len - i; j < len; str[j++] = 0);

		len = strlen(str);
	}

	ptr = strrchr(str, ' ');

	if (ptr < (str + len)) {}
	else
	{
		while(*ptr)
		{
			if (*ptr == ' ') *ptr = 0;
			else break;
			ptr--;
		}
	}

	return str;
}

unsigned int mtime()
{
  struct timespec t;

  clock_gettime(/*CLOCK_REALTIME*/ 0, &t);
  return (int)t.tv_sec * 1000 + t.tv_nsec / 1000000;
}

time_t	time_now()
{
	time_t  t;
	time(&t);

	return t;
}

bool up_time(struct tm &tm_begin/* начало отсчета интервала */) /* true - время вышло */
{
        time_t now = time_now();
        struct tm *tm_now = localtime (&now);

        bool ret_value = (tm_now->tm_mday > tm_begin.tm_mday) || (tm_now->tm_mon > tm_begin.tm_mon) || (tm_now->tm_year > tm_begin.tm_year);

        if(ret_value)
        {
#ifdef _DEBUG
                char buffer1[80];
                char buffer2[80];

                strftime (buffer1, 80, "Tm begin = %I:%M%p", &tm_begin);

                strftime (buffer2, 80, "Tm now = %I:%M%p"  , tm_now);

                std::cout << buffer1 << " VS " << buffer2 << std::endl;
#endif
                tm_begin = *tm_now;
        }
        return ret_value;

}

bool up_time(int /* интервал */tm_between, time_t& tm_begin/* начало отсчета интервала */) /* true - время вышло */
{
	time_t now = time_now();
	double seconds = difftime(now, tm_begin);

	bool ret_value = seconds >= (double)tm_between;

	if(ret_value)
	{
#ifdef _DEBUG
		char buffer1[80];
		char buffer2[80];

		struct tm *timeinfo = localtime (&tm_begin);
		strftime (buffer1, 80, "Tm begin = %I:%M%p", timeinfo);

		timeinfo = localtime (&now);
		strftime (buffer2, 80, "Tm now = %I:%M%p", timeinfo);

		std::cout << buffer1 << " VS " << buffer2 << std::endl;
#endif
		tm_begin = now;
	}
	return ret_value;

}

bool MakeDirectory(char* str)
{
    if (!str) return false;

    std::string s;
    s.append(str);

    size_t pre=0,pos;
    string dir;
    bool mdret;

    if(s.at(0) != DELIMITER && s.at(0) != '.') s = DELIMITER + s;
    if(s[s.size()-1] != DELIMITER) s += DELIMITER;

    while((pos=s.find_first_of(DELIMITER,pre))!=std::string::npos)
    {
        dir=s.substr(0,pos++);
        pre=pos;

        if(!dir.size()) continue; // if leading / first time is 0 length

        mdret = mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0 || errno == EEXIST;
        if (!mdret)
        {
//            logger::message.str("");
//            logger::message << errno << " " << (unsigned int)time(0) << " " << dir << " : " << strerror(errno) << std::endl;
//    	    logger::write_log(logger::message);

            break;
        }

    }
    return (bool) mdret;
}

std::string getCmdOutput1(const std::string& mStr)
{
    std::string result, s;

    FILE* pipe = popen(mStr.c_str(), "r");

    char *buffer = (char*)calloc(256, sizeof(char));

    while(fgets(buffer, 256, pipe) != NULL)
    {
        s = buffer;
    	result.append(buffer);
    }

    pclose(pipe);
    free(buffer);

#ifdef _DEBUG
   std::cout << result << endl;
#endif
    return result;
}

MProcess getCmdOutput(const std::string& mStr)
{
	MProcess m;

    FILE* pipe = popen(mStr.c_str(), "r");

    char *buffer = (char*)calloc(5001, sizeof(char));

    while(fgets(buffer, 5000, pipe) != NULL)
    {
        char *r = strdup(buffer);
    	m.push_back(r);
#ifdef _DEBUG
    	std::cout << "Строка = " << r << endl;
#endif
    }

    pclose(pipe);
    free(buffer);

    return m;
}

bool is_console_cmd(char* m)
{
	bool is_cmd  = false;
	char *buf  = (char*) calloc(256, sizeof(char));
	if (!buf) return is_cmd;

	sscanf(m, "%*i %s %*s *s", buf);

	std::string s = "which ";
	s.append(buf);
//	std::cout << "Console cmd :" << s << endl;

	MProcess pr = getCmdOutput(s);

	if (pr.size())
	{

			for (MProcess::iterator it = pr.begin(); it != pr.end(); it++)
			{
				std::cout << " Cmd :" << *it << endl;
				if (strstr(*it, "accord" ) == NULL || strstr(*it, "stop" ) != NULL || strstr(*it, "exit" ) != NULL) is_cmd  = true;
                                //if (strstr(*it, "accord" )) is_cmd  = true;
				free(*it);
				*it = NULL;
			}
	}
	free(buf);

	return is_cmd;
}

pid_t    GetPid(MProcess m)
{
    pid_t m_pid = 0;
    int sz = m.size()-1;

    if (sz <= 0) return m_pid;

    for (int i = 0; i < sz; i++)
    {
    		 if (is_console_cmd(m[i])) continue;

    		 m_pid = atoi(m[i]);
   		 break;
    }

	return m_pid;

}

std::string		GetSubPath(int m_id)
{
		return atos(div(m_id, 10000).quot);
}

long long int ConvertToDecFromOct(string s)
{
	char *ptr;
	return strtoll(s.c_str(), &ptr, 8);
}

}/* namespace */


