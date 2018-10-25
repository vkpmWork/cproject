
#include "tclientsocket.h"
#include "common.h"
#include <unistd.h>
#include <linux/limits.h>
//#include "string.h"
using namespace std;

//#define gLockFilePath    "/var/run/"
#define gLockFilePath    "./"

int readline(int fd, char *buf, int maxlen); // forward declaration

/*

file:///home/irina/Eclipseworkspace/HTML/player.html
file:///home/irina/Eclipseworkspace/HTML/Тестовая страница.html
file:///home/irina/Eclipseworkspace/HTML/test.html
file:///home/irina/Eclipseworkspace/HTML/audiotags.html
http://r1.101.ru:8100/?id=562347&module=mdb
http://r1.101.ru:8100/?id=350899&module=mdb
http://101.ru/mp3_test

http://101.ru/audiotags
Текущее состояние числа открытых файлов можно узнать так: cat /proc/sys/fs/file-nr
ps --sort=start_time -x -o pid,command | grep musicgo | grep -v grep

Зависимости: ldd -v musicgo

If you were able to build the project but you still get the "binary not found" message
then the issue might be that a default launch configuration is not being created for the project. In that case do this:

Right click project > Run As > Run Configurations... >
Then create a new configuration under the "C/C++ Application" section > Enter the full path to the executable file
(the file that was created in the build step and that will exist in either the Debug or Release folder)

netstat -an | grep 8100 - показать все соединения на порту 8100
netstat -an | wc        - показать счетчики соединения
netstat -an             - показать все соединения
pkill -9 -f musicgo     - грохнуть все по маске
telnet  r1.101.ru 8100  - проверка соединения

*/


enum tcCmd {cEmpty = 0, cStop};
/* =================== prototypes ================= */
int    ConfigureSignalHandlers(void);
void   FatalSigHandler(int sig);
void   Sig_Handler(int sig);
tcCmd  GetCmd(char *s);
void   OnSigTerm();



tclient_socket *Socket = NULL;
int main(int argc, char**argv)
{
            if (argc < 2)
	    {
	        std::cout << "It isn't enough arguments!\n";
                exit (EXIT_FAILURE);
	    }


            common::application_name = strdup(argv[0]);
            char     *f_pid  = strrchr(common::application_name, DELIMITER)+1;
            if (!f_pid)
            {
                        printf("Something wrong with application name.\n");
                        return -1;
            }

            pConfig = NULL;

            common::pidfile_name = (char*)malloc(NAME_MAX);

            strcpy(common::pidfile_name, gLockFilePath);
            strcat(common::pidfile_name, f_pid);
            strcat(common::pidfile_name, ".pid");

            tcCmd  cmd = GetCmd(argv[1]);

            if (cmd == cStop)
            {
                  pid_t m_pid = common::getPid();

                  if (m_pid)
                  {
                          kill(m_pid, SIGTERM);
                          std::cout << "Command " << argv[1] << " received!\n";
                  }
                  common::DisposeCommon();
                  exit(EXIT_SUCCESS);
            }

            if (!common::FileExists(argv[1]))
            {
                printf("Configuration file '%s' not found!\n", argv[1]);
                common::DisposeCommon();
                exit (EXIT_FAILURE);
            }

            ConfigureSignalHandlers();

            pConfig = new tconfig(argc, argv);
            if (!pConfig || pConfig->get_cfg_error())
            {
                std::cout << "pConfig = NULL Проблемы с cfg-файлом" << endl;
                common::DisposeCommon();
                if (pConfig)
                {  delete pConfig;
                   pConfig = NULL;
                }
                exit (EXIT_FAILURE);
            }

            if (pConfig->get_daemon())
            {
                            if( common::BecomeDaemonProcess() < 0)
                            {
                                    std::cout << "Failed to become daemon process\n";
                                    if (pConfig)
                                    {  delete pConfig;
                                       pConfig = NULL;
                                    }
                                    common::DisposeCommon();
                                    return EXIT_FAILURE;
                            }
            }

	    logger::ptr_log = new logger::TInfoLog(pConfig->get_logfile(), pConfig->get_logsize(), pConfig->get_loglevel(), (char*) pConfig->get_header().c_str());

	    Socket = new tclient_socket(pConfig->get_host(), pConfig->get_port(), pConfig->get_transmit_timeout());
	    if (Socket)
	    {
	        if(Socket->get_error() == 0)
	        {
	          Socket->do_accept();
	          Socket->do_close();
	        }

	        delete Socket;
	        Socket = NULL;
	    }
////////////////////////////////////////////////
	    common::DisposeCommon();
	    logger::delete_log();
	    return 0;
}

/**
 * Simple utility function that reads a line from a file descriptor fd,
 * up to maxlen bytes -- ripped from Unix Network Programming, Stevens.
 */
int readline(int fd, char *buf, int maxlen)
{
    int n, rc;
    char c;

    for (n = 1; n < maxlen; n++)
    {
    	if ((rc = read(fd, &c, 1)) == 1)
    	{
    		*buf++ = c;
    		if (c == '\n') break;
    	}
    	else if (rc == 0)
    	     {
    			if (n == 1) return 0; // EOF, no data read
    			else break; // EOF, read some data
    		 }
    		else return -1; // error
    }

    *buf = '\0'; // null-terminate
    return n;
}

tcCmd  GetCmd(char *s)
{
        char *ss = strdup(s);

        ss = common::xstrlower(ss);

        tcCmd c = cEmpty;

        if (strcmp(s, "stop") == 0 || strcmp(s, "exit") == 0) c = cStop;

        free(ss);

        return c;
}

void FatalSigHandler(int sig)
{
    char s[100];
    sprintf(s, "Falat error! Received signal is %i", sig);

    //LOG_OPER(s);
    // ClearMemory();
    _exit(sig);
}

int ConfigureSignalHandlers(void)
{
    struct sigaction sigtermSA;

    /* ignore several signals because they do not concern us. In a
        production server, SIGPIPE would have to be handled as this
        is raised when attempting to write to a socket that has
        been closed or has gone away (for example if the client has

        crashed). SIGURG is used to handle out-of-band data. SIGIO
        is used to handle asynchronous I/O. SIGCHLD is very important
        if the server has forked any child processes. */

    signal(SIGPIPE, SIG_IGN);
    signal(SIGALRM, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGURG, SIG_IGN);
    signal(SIGXCPU, SIG_IGN);
    signal(SIGXFSZ, SIG_IGN);
    signal(SIGVTALRM, SIG_IGN);
    signal(SIGPROF, SIG_IGN);
    signal(SIGIO, SIG_IGN);
//    signal(SIGCHLD, SIG_IGN);  /* игнорирование сигнала "гибель потомка" */
    signal(SIGQUIT,   FatalSigHandler);
    signal(SIGILL,    FatalSigHandler);
    signal(SIGTRAP,   FatalSigHandler);
    signal(SIGABRT,   FatalSigHandler);
    signal(SIGIOT,    FatalSigHandler);
    signal(SIGBUS,    FatalSigHandler);
    signal(SIGFPE,    FatalSigHandler);
    signal(SIGSEGV,   FatalSigHandler);
    signal(SIGSTKFLT, FatalSigHandler);
    signal(SIGCONT,   FatalSigHandler);
    signal(SIGPWR,    FatalSigHandler);
    signal(SIGSYS,    FatalSigHandler);
    /* these handlers are important for control of the daemon process */

    /* TERM  - shut down immediately */

    sigtermSA.sa_handler=Sig_Handler;
    sigemptyset(&sigtermSA.sa_mask);
    sigtermSA.sa_flags=0;

    sigaction(SIGTERM, &sigtermSA, 0);
    sigaction(SIGHUP,  &sigtermSA, 0);
//    sigaction(SIGTSTP, &sigtermSA, 0);
    sigaction(SIGCONT, &sigtermSA, 0);
    sigaction(SIGUSR1, &sigtermSA, 0);
    sigaction(SIGUSR2, &sigtermSA, 0);

    return 0;
}

void   OnSigTerm()
{
  if (Socket)
  {
      Socket->do_close();
      delete Socket;
      Socket = NULL;
  }

  if (pConfig)
  {
      delete pConfig;
      pConfig = NULL;
  }

  if (logger::ptr_log)
  {
      delete logger::ptr_log;
      logger::ptr_log  = NULL;
  }
  common::DisposeCommon();
}
void Sig_Handler(int sig)
{
        std::cout << "SigTerm\n";
                switch (sig)
        {
            case  SIGTERM :  //Set_SIGDELETE();
                             OnSigTerm();
                             exit(EXIT_SUCCESS);
                             break ;
            case  SIGHUP  :  break;
            case  SIGTSTP :  break;
            case  SIGCONT :  break;
            case  SIGUSR1 :  break;
            case  SIGUSR2 :  break;
            default       :  sig = 0;
                             break;
        }
}
