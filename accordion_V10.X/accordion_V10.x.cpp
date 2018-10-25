/*
 * Версия 9.1 - на всякий случай разблокировала сигнал SIGTERM
 *            - в функции запуска демона закомментировала проверку и закрытие файловых дескрипторов текущего процесса. Эта проверка занимала почти 600 мс
 *
 * Версия 9.2  04.10.2018 - отправка фреймов net_send добавила строку if (errno == EAGAIN)...
 * Версия 9.3  16.10.2018 - убрано ограничение на размер файла в 5 секунд (aac_GetFileInfo)
 *                        - три запроса плейлиста, при необходимости
 *
 * Версия 10.0 25.10.2018 - добавлена команда check, которая возвращает ноль (SUCCESS), если канал запущен и 1, если канал не запущен
 *                        - добавлен лог на неудачные запуски аккордеона
 *                        - в конфиге добавлен пуль для логов неудачных запусков
*/


#include <signal.h>
#include "main_dispatcher.h"
#include <linux/limits.h>
#include "stdio.h"
#include "log.h"

#ifdef _DEBUG
//const char *gLockFilePath    = "/var/run/";
#include "stdio.h"
	const char *gLockFilePath    = "./";
#else
	const char *gLockFilePath    = "./";
#endif

//const char *str_process		 = "ps --sort=start -axo pid,command | grep channelid=%i | grep channeltype=%i | grep streamtype=%i | grep -v grep";
const char *str_process			 = "ps --sort=start_time -axo pid,command | grep %s | grep %s | grep %s | grep -v grep";

/*  Управление курсором в консоли
 *

    //    \033[#A передвинуть курсор вверх на # строк
    //    \033[#B передвинуть курсор вниз на # строк
    //    \033[#С передвинуть курсор вправо на # столбцов
    //    \033[#D передвинуть курсор влево на # столбцов
    //    \033[#E передвинуть курсор вниз на # строк и поставить в начало строки
    //    \033[#F передвинуть курсор вверх на # строк и поставить в начало строки
    //    \033[#G переместить курсор в указанный столбец текущей строки
    //    \033[#;#H задает абсолютные координаты курсора
    //    \033]2;BLA_BLA\007   Заголовок окна xterm...
 */

/*
Обновление glibc-6 выше версии 2.13
sudo -s
echo 'deb http://ftp.us.debian.org/debian/ testing main contrib non-free' >> /etc/apt/sources.list && apt-get update && apt-get install -t testing libc6

If you were able to build the project but you still get the "binary not found" message
then the issue might be that a default launch configuration is not being created for the project. In that case do this:

Right click project > Run As > Run Configurations... >
Then create a new configuration under the "C/C++ Application" section > Enter the full path to the executable file
(the file that was created in the build step and that will exist in either the Debug or Release folder)

mplayer -cache 5000 -cache-min 29 http://localhost:8000/a236

echo $? - код завершения процесса
*/
using namespace common;

enum tcCmd {cEmpty = 0, cStop, cMake, cStatus, cCheck};
/* =================== prototypes ================= */
void   ClearMemory();
int    ConfigureSignalHandlers(void);
void   FatalSigHandler(int sig);
void   Sig_Handler(int sig);
tcCmd  GetCmd(char *s);
//pid_t  GetPid(MProcess, char*);
/* ================================================ */
/* Зависимости
 * ldd -v accordion_V3.X
 *
 * http://127.0.0.1:8000
 * /etc/init.d/icecast2 start
 * http://101.ru/?an=meta - скрипт для отладки точки icecast2 : test1 - test10
 *
 * Тестовый канал http://101.ru/radio/channel/236/show/1
 *
 * /home/irina/Eclipseworkspace/accordion_V3.X/Debug/test1.ini
 *
 * To add pthread library to your non-makefile project, do following steps (in eclipse):
 * right click on the project in the project explorer. Select properties -> c/c++ general -> Paths and Symbols -> libraries -> add -> type 'pthread' in text box -> ok -> ok -> rebuild
 *
 * Код завершения операции. Посмотреть, что возвращается в консоль: echo $?
 */

tcCmd  GetCmd(char *s)
{
	char *ss = strdup(s);

	ss = common::xstrlower(ss);

	tcCmd c = cEmpty;

	if (strcmp(s, "stop") == 0 || strcmp(s, "exit") == 0) c = cStop;
	else  if (strcmp(s, "make") == 0) c = cMake;
		  else  if (strcmp(s, "status") == 0) c = cStatus;
		        else  if (strcmp(s, "check") == 0) c = cCheck;

	free(ss);

	return c;
}

int main(int argc, char **argv)
{

  if (argc <= 4)
   {
      std::cout << "It isn't enough arguments!\n";
      exit (EXIT_FAILURE);
    }

#ifdef _DEBUG
    printf("Argument %s\n", argv[1]);
    ApplicationVersion();
#endif

    time_start = mtime();

    application_name = strdup(argv[0]);
    char     *f_pid  = strrchr(application_name, DELIMITER);
    if (!f_pid)
    {
    	    	printf("Something wrong with application name.\n");
    	    	return -1;
    }

    char *cmd_out = (char*) calloc(1024, sizeof(char));

    sprintf(cmd_out, str_process, argv[2], argv[3], argv[4]);

    MProcess m     = getCmdOutput(cmd_out);     /* список процессов по маске */
    pid_t    m_pid = common::GetPid(m);		/* PID запущенного канала, 0 - если канал не запущен */
    tcCmd    cmd   = GetCmd(argv[1]);		/* команда в строке запуска  */

    if (cmd != cEmpty)
    {
        	if (m_pid && (cmd != cStatus && cmd != cCheck))
        	{
        		m_pid = kill(m_pid, cmd == cStop ? SIGTERM : SIGUSR1);
                        std::cout << "Attempt to send a signal " << argv[1] << " to PID " << m_pid << ". Result: " << strerror(errno) << std::endl;

        		if (cmd == cStop)
        		{
                                  sleep(1);

                                  m.clear();
                                  m = getCmdOutput(cmd_out);
                                  m_pid = common::GetPid(m);
        		}
        	}
    		free(cmd_out);

    		int ret_value = 0;
    		switch ((int)cmd)
    		{
    		  case  cStatus :  ret_value =  (m_pid != 0);   break; /* процесс существует = true */
    		  case  cMake   :  ret_value =  (m_pid == 0);   break; /* отправка сигнала прошла успешно SUCCESS */
    		  case  cCheck  :  ret_value =   m_pid == 0;    break;  /* процесс существует = SUCCESS(0)  */
    		  case  cStop   :  ret_value =  (m_pid == 0);   break; /* процесс остановлен = true */
    		}

    		exit(ret_value); /* процесс остановлен = true */

    }

    for (MProcess::iterator it = m.begin(); it != m.end(); it++)
    {
		free(*it);
		*it = NULL;
    }
    free(cmd_out);

    if (m_pid)
    {
        std::cout << " Channel with <" <<  argv[2] << ' ' << argv[3] << ' ' << argv[4] << "> is already started!" << endl;

        tconfig *p   = new tconfig(argv[1]);
        if (p)
        {
            logger::TStartLog start_log(p->get_log_on_start());

            std::stringstream m;
            for (int i = 1; i < argc; i++) m << " " << argv[i];
            start_log.write_started(m.str());
            delete p;
        }
        exit (0);
    }

    if (!FileExists(argv[1]))
    {
        printf("Configuration file '%s' not found!\n", argv[1]);
        exit (EXIT_FAILURE);
    }

	ConfigureSignalHandlers();


        int ret_value = dispatcher(argv, argc);

	ClearMemory();
	return ret_value;
}

void ClearMemory()
{
	DisposeCommon();
}

int ConfigureSignalHandlers(void)
{

  sigset_t currentset, news;
  sigemptyset(&news);
  sigaddset(&news, SIGTERM);
  sigaddset(&news, SIGTSTP);
  sigaddset(&news, SIGUSR1);

  if (sigprocmask(SIG_UNBLOCK, &news, &currentset) < 0) std::cout << "Ошибка вызова функции sigprocmask" << endl;
  if (sigprocmask(0, NULL, &currentset) < 0) std::cout << "Ошибка вызова функции sigprocmask" << endl;
  else
  {
      std::cout << "Список сигналов процесса: ";
      if (sigismember(&currentset, SIGKILL))  std::cout << "SIGKILL ";
      if (sigismember(&currentset, SIGTERM)) std::cout << "SIGTERM ";
      if (sigismember(&currentset, SIGINT))  std::cout << "SIGINT ";
      if (sigismember(&currentset, SIGHUP))  std::cout << "SIG ";
      if (sigismember(&currentset, SIGSTOP)) std::cout << "SIGSTOP ";
      if (sigismember(&currentset, SIGALRM)) std::cout << "SIGALRM ";
      if (sigismember(&currentset, SIGUSR1)) std::cout << "SIGUSR1 ";
      if (sigismember(&currentset, SIGUSR2)) std::cout << "SIGUSR2 ";
      std::cout << endl;
  }

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
    signal(SIGIO, SIG_IGN);     // POSIX (B.3.3.1.3) запрещает установку действия для сигнала SIGCHLD на SIG_IGN
//    signal(SIGCHLD, SIG_IGN);  /* игнорирование сигнала "гибель потомка" */
    signal(SIGUSR2, SIG_IGN);
    signal(SIGHUP, SIG_IGN);

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
            len = read(fd, pid_buf, 16);channelid=
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
    sigaction(SIGSTOP,  &sigtermSA, 0);
    sigaction(SIGTSTP, &sigtermSA, 0);
    sigaction(SIGCONT, &sigtermSA, 0);
    sigaction(SIGUSR1, &sigtermSA, 0);

   return 0;
}

void FatalSigHandler(int sig)
{
    char s[100];
    sprintf(s, "Falat error! Received signal is %i", sig);

    //LOG_OPER(s);
    ClearMemory();
    _exit(sig);
}



void Sig_Handler(int sig)
{
    	std::cout << "SigTerm = " << sig << endl;
	switch (sig)
        {
	     case  SIGSTOP :
	     case  SIGTERM : Set_SIGDELETE();
            		     ClearMemory();
                             exit(EXIT_SUCCESS);
                             break ;
            case  SIGTSTP :  ;
                             break;
            case  SIGCONT :  ;
                             break;
            case  SIGUSR1 :  Set_SIGUSR1(); break;
            default       :  sig = 0;
            				 break;
        }
}




