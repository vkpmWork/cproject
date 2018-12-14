
#include "errormonitor.h"
#include "loggermutex.h"
#include "pthread.h"
#include "log.h"
#include "config.h"


#ifdef _DEBUG
#include  <iostream>
#endif
TErrorMonitor *pErrorMonitor = NULL;
pthread_t 	   error_thread;
//----------------------------------------
void* handler_error_monitor_thread(void*)
{

    pErrorMonitor = new TErrorMonitor(pConfig->error_counter()
                                      , pConfig->registered_error_level()
                                      , pConfig->reset_error_timeout()
                                      , pConfig->email_error_timeout()
                                      , pConfig->email_volume()
                                      );

	sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR2);

	/*
	 * Блокируем доставку сигналов SIGUSR2
	 * После возвращения предыдущее значение
	 * заблокированной сигнальной маски хранится в oldmask
	 */

	sigprocmask(SIG_BLOCK, &mask, &oldmask);

	/* Задаём время ожидания 1 мин */
	struct timespec tv;
	tv.tv_sec  = 30;
	tv.tv_nsec = 0;

	/*
	 * Ждём доставки сигнала. Мы ожидаем 2 сигнала,
	 * SIGRTMIN и SIGRTMIN+1. Цикл завершится, когда
	 * будут приняты оба сигнала
	 */

	siginfo_t siginfo;
    int 	  recv_sig;

	string s;
    while(true)
	{
//      std::cout << "Error TimeOut \n";
	  if ((recv_sig = sigtimedwait(&mask, &siginfo, &tv)) == -1)
	  {
	    if (errno == EAGAIN) continue;

	    perror("sigtimedwait");
        break;
      }

	  printf("Error signal %d received\n",  recv_sig);

	  struct TMess
	  {
			int   value;
			char  *domain;
			char *str;
	  }*mess;

	  union sigval svalue;
 	  svalue.sival_ptr =  siginfo._sifields._rt.si_sigval.sival_ptr;

 	  mess =  (TMess*)svalue.sival_ptr;
 	  if (!mess)
 	  {	  std::cout << "Mess is empty\n";
 	  	  continue;
 	  }

 	 std::cout << "Error Monitor: level=" << mess->value << "; domain=" << mess->domain << ": msg=" << mess->str << endl;

//	  if (pErrorMonitor) pErrorMonitor->onAdd_error(mess->value, mess->domain, mess->str);
 	  free(mess->domain);
 	  free(mess->str);
 	  free(mess);

	}
    return NULL;
}

void create_error_monitor_thread()
{
	   pthread_attr_t thread_attr;

	   std::ostringstream m;
	   if (pthread_attr_init(&thread_attr) != 0)
	   {
		   m.str("Attribute creation failed (create_error_monitor_thread())\n");
		   logger::write_log(m);
		   return;
	   }

	    // переводим в отсоединенный режим
	   if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) != 0)
	    {
	    	m.str("Setting detached attribute failed (create_error_monitor_thread())\n");
	    	logger::write_log(m);
	    	return;
	    }

	    if ( pthread_create(&error_thread, &thread_attr, handler_error_monitor_thread, NULL)) std::cout << "In  : Thread creation failed (create_error_monitor_thread())\n";

	    (void)pthread_attr_destroy(&thread_attr);

	    std::cout << "ErrorMonitor Created\n";

}
//----------------------------------------

TErrorMonitor::TErrorMonitor(   int    m_error_counter
                              , int    m_registered_error_level
                              , double m_reset_error_timeout
                              , double m_email_error_timeout
                              , uint   m_email_volume
                            )
{
    ErrMonitor.m_error_counter          = m_error_counter;
    ErrMonitor.m_registered_error_level = m_registered_error_level;
    ErrMonitor.m_reset_error_timeout    = m_reset_error_timeout;
    ErrMonitor.m_email_error_timeout    = m_email_error_timeout;
    ErrMonitor.m_email_volume           = m_email_volume*1024;


}

TErrorMonitor::~TErrorMonitor()
{
}

void TErrorMonitor::onAdd_error(int m_error, char* dm, char* msg)
{
    if (!m_error) return;

    if ((ErrMonitor.IsNeedToCheckError == false) || (m_error <= ErrMonitor.m_registered_error_level)) return;

    TDomInfo info;
    bool     fl_email = false;

    set_mutex(true);


    if (domain.count(dm))
    {
        info            = domain[dm];
        info.m_overflow = counter_overflow(info.m_overflow, info.m_counter);
        fl_email        = exceeded_email_timeout(info);
    }
    else
    {
        info.m_counter   = info.msg_box_size = 0;
        info.m_overflow  = false;
        info.m_first_owerflow = true;
        time(&info.tm);

        struct tm * timeinfo = localtime (&info.tm);
        timeinfo->tm_year -= 10;
        info.tm = mktime ( timeinfo );
    }

    int m_msg_box_size = strlen(msg);
    if (m_msg_box_size + info.msg_box_size > ErrMonitor.m_email_volume)
    {
        common::vmsg_box::iterator it = info.msg_box.begin();

        if (it != info.msg_box.end())
        {
            info.msg_box_size    -= strlen(*it);

            free(*it);
            info.msg_box.erase(it);
        }
    }

    info.msg_box.push_back(strdup(msg));

    if (fl_email)
    {
        common::vmsg_box box;
        for (common::vmsg_box::iterator it = info.msg_box.begin(); it != info.msg_box.end(); it++)
        {
            box.push_back(strdup(*it));
            free(*it);
        }

        info.msg_box.clear();
        info.m_counter = info.msg_box_size = m_msg_box_size = 0;

//        emit send_email(box);
    }
    else
    {
        info.m_counter++;
        info.msg_box_size += m_msg_box_size;
    }
    domain[dm] = info;



    set_mutex(false);

}

bool TErrorMonitor::counter_overflow(int value)
{
    return value >= ErrMonitor.m_error_counter;
}

bool TErrorMonitor::counter_overflow(bool is_overflow, int &value)
{
    if (is_overflow || (value >= ErrMonitor.m_error_counter)) value = 0;
    return value == 0;
}

bool TErrorMonitor::exceeded_email_timeout(time_t tm, bool first_overflow)
{
    if (first_overflow) return true;

    time_t now;
    time(&now);

    double _difftime = difftime(now, tm);
    return _difftime >= ErrMonitor.m_email_error_timeout;
}

bool TErrorMonitor::exceeded_email_timeout(TDomInfo &m)
{
    bool fl_email = false;
    time_t now;
    time(&now);

    //if (m.m_first_owerflow || - так было!!!
//    if ((m.m_overflow && m.m_first_owerflow) || (m.m_overflow && (difftime(now, m.tm) >= ErrMonitor.m_email_error_timeout)))
//    if (m.m_first_owerflow || (m.m_overflow && (difftime(now, m.tm) >= ErrMonitor.m_email_error_timeout)))
    if (m.m_overflow && (difftime(now, m.tm) >= ErrMonitor.m_email_error_timeout))
    {
        m.m_overflow  =  m.m_first_owerflow = false;
        fl_email      = true;
        time(&m.tm);
    }

    return fl_email;
}

void TErrorMonitor::set_mutex(bool lock_status)
{

    if (pLoggerMutex)
    {
            if (lock_status) pLoggerMutex->ErrorMonitor_MutexLock();
            else pLoggerMutex->ErrorMonitor_MutexUnlock();
    }
}
