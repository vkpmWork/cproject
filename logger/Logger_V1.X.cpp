//============================================================================
// Name        : X.cpp
// Author      : 
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
#include "common.h"
#include "config.h"
#include <linux/limits.h>
#include "log.h"
#include "loggermutex.h"
#include "tclientsocket.h"

#define LOCAL

#ifdef LOCAL
const char *gLockFilePath           = "./";
#else
const char *gLockFilePath           = "/var/run/";
#endif

const char *version                 = "Application name: %s.  Version 1.0 от 08.11.2018\n";
char       *pidfile_name            = NULL;

enum tcCmd {cEmpty = 0, cStart, cVersion, cInfo, cStop, cState, cPid, cReconfig, cPause, cResume};

tcCmd   GetCmd(char *s);
int     ConfigureSignalHandlers(void);
void    FatalSigHandler(int sig);
void    Sig_Handler(int sig);
void    FreeMemory();
int 	main(int argc, char* argv[])
{

  if (argc < 2)
  {
      std::cout << "It isn't enough arguments!\n";
      return EXIT_FAILURE;
  }

  common::application_name = strdup(argv[0]);
  char     *f_pid  = strrchr(common::application_name, DELIMITER)+1;
  if (!f_pid)
  {
              printf("Something wrong with application name.\n");
              return -1;
  }

//  pConfig = NULL;

  pidfile_name = (char*)malloc(NAME_MAX);

  strcpy(pidfile_name, gLockFilePath);
  strcat(pidfile_name, f_pid);
  strcat(pidfile_name, ".pid");

  tcCmd  cmd = GetCmd(argv[1]);

  if (cmd != cStart)
  {
      pid_t m_pid  = common::getPid(pidfile_name);
      int m_signal = 0;

      switch (cmd)
      {
        case    cState    : m_signal = SIGUSR1; break;
        case    cStop     : m_signal = SIGTERM; break;
        case    cVersion  :
        case    cPid      : printf("Process PID : %i %s", m_pid, common::application_name); break;
        case    cReconfig : m_signal = SIGHUP;  break;
        case    cPause    : m_signal = SIGTSTP; break;
        case    cResume   : m_signal = SIGCONT; break;
        default 		  : m_signal = 0;		break;
      }

      if (m_signal)
      {
          kill(m_pid, m_signal);
          std::cout << "Attempt to send a signal " << argv[1] << " to PID " << m_pid << ". Result: " << strerror(errno) << std::endl;
      }
      return EXIT_SUCCESS;
  }


  if (cmd != cStart)
  {
      free(pidfile_name);
      exit (EXIT_FAILURE);
  }

  if (!common::FileExists(argv[2]))
  {
          printf("Configuration file '%s' not found!\n", argv[2]);
          exit (EXIT_FAILURE);
  }


  if (!InitLogSystemSetup(argv[2]))
  {
       std::cout << "Something wrong with cfg-file" << endl;
       FreeMemory();

       exit (EXIT_FAILURE);
  }

  ConfigureSignalHandlers();

  if (pConfig->isdaemon())
  {
                   if( common::BecomeDaemonProcess() < 0)
                   {
                           std::cout << "Failed to become daemon process\n";
                           FreeMemory();
                           return EXIT_FAILURE;
                   }
   }

  logger::ptr_log = new logger::TInfoLog((char*)pConfig->get_logfile().c_str(), pConfig->get_logsize(), pConfig->get_loglevel(), (char*) pConfig->get_header().c_str());
  std::ostringstream s;
  s << "Hello World";
  winfo(s);

  InitLoggerMutex();

  tclient_socket *Socket = new tclient_socket((char*)pConfig->LocalAddress().c_str(), pConfig->LocalPort(), 300);
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

  FreeMemory();
  return EXIT_SUCCESS;
}

tcCmd  GetCmd(char *ss)
{
        char *s = strdup(ss);

        s = common::xstrlower(s);

        tcCmd c = cEmpty;


        if (strcmp(s, "-c") == 0) c = cStart;
        else    if (strcmp(s, "stop") == 0 || strcmp(s, "exit") == 0) c = cStop;
                else if (strcmp(s, "-v") == 0 || strcmp(s, "version") == 0) c = cVersion;
                     else if (strcmp(s, "-i") == 0 || strcmp(s, "info") == 0) c = cInfo;
                          else if (strcmp(s, "-s") == 0 || strcmp(s, "state") == 0) c = cState;
                               else if (strcmp(s, "-p") == 0 || strcmp(s, "pid") == 0) c = cPid;
                                    else if (strcmp(s, "reconfig") == 0) c = cReconfig;
                                         else if (strcmp(s, "pause") == 0) c = cPause;
                                              else if (strcmp(s, "resume") == 0) c = cResume;
                                                   else c = cEmpty;

        free(s);

        return c;
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

    signal(SIGUSR2, SIG_IGN);
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
    signal(SIGCHLD, SIG_IGN);

    /* these signals mainly indicate fault conditions and should be logged.
        Note we catch SIGCONT, which is used for a type of job control that
        is usually inapplicable to a daemon process. We d      if (argc > 1) {
            int fd, len;
            pid_t pid;
            char pid_buf[16];c /home/user14/workspace/RemoteLogger/RemoteLogger.conf
            if ((fd = open(gLockFilePath, O_RDONLY)) < 0)
            {
                  perror("Lock file not found. May be th-ce server is not running?");
                  exit(fd);
            }
            len = read(fd, pid_buf, 16);
            pid_buf[len] = 0;
            pid = atoi(pid_buf);
            if(!strcmp(argv[1], "stop")) {
           -c       kill(pid, SIGUSR1);
                  exit(EXIT_SUCCESS);
            }
            if(!strcmp(argv[1], "restart")) {
                  kill(pid, SIGHUP);isn't established
                  exit(EXIT_SUCCESS);c /home/user14/workspace/RemoteLogger/RemoteLogger.conf
            }
            printf ("usage %s [stop|restart]\n", argv[0]);-c
            exit (EXIT_FAILURE);-c
        }on't do anyting to
        SIGSTOP since this signal can't be caught or ignored. SIGEMT is not
        supported under Linux as of kernel v2.4 */

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
    sigaction(SIGTSTP, &sigtermSA, 0);
    sigaction(SIGCONT, &sigtermSA, 0);
    sigaction(SIGUSR1, &sigtermSA, 0);
    return 0;
}

void FatalSigHandler(int sig)
{
    char s[100];
    printf(s, "Falat error! Received signal is %i", sig);
 //   DisposeProcess();
    _exit(sig);
}

void Sig_Handler(int sig)
{
  std::cout << "SigTerm = " << sig << endl;
  /*   if (pMainDispatcher)
    {
        switch (sig)
        {
            case  SIGTERM :  DisposeProcess();
                             exit(EXIT_SUCCESS);
                             break ;
            case  SIGHUP  :  pMainDispatcher->Set_SIGHUP();
                             break;
            case  SIGTSTP :  pMainDispatcher->Set_SIGTSTP();
                             break;
            case  SIGCONT :  pMainDispatcher->Set_SIGCONT();
                             break;
            case  SIGUSR1 :  pMainDispatcher->Set_SIGUSR1();
            default       :  sig = 0;
        }
    }
 */
}

void    FreeMemory()
{
  DeleteLogSystemSetup();
  DeleteLogMutex();

  if (logger::ptr_log)
  {
	  delete logger::ptr_log;
	  logger::ptr_log = NULL;
  }

  unlink(pidfile_name);
  free(pidfile_name);
}

