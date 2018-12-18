
#include "errormonitor.h"
#include "pthread.h"
#include "log.h"
#include "config.h"
#include "email.h"


#ifdef _DEBUG
#include  <iostream>
#endif
pthread_t 	   error_thread;
//----------------------------------------
void* handler_error_monitor_thread(void*)
{

	TErrorMonitor *pErrorMonitor = new TErrorMonitor(pConfig->error_counter()
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
	tv.tv_sec  = pConfig->reset_error_timeout();
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
	  if ((recv_sig = sigtimedwait(&mask, &siginfo, &tv)) == -1)
	  {
	    if (errno == EAGAIN)
	    {
	    	pErrorMonitor->exceeded_reset_timeout();
	    	continue;
	    }

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

 	  if (pErrorMonitor) pErrorMonitor->onAdd_error(mess->value, mess->domain, mess->str);

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

inline void TErrorMonitor::send_email(common::vmsg_box m_msg_box)
{
    if (m_msg_box.size())
    {
    	TEMail      *wrk_email = new TEMail(pConfig->get_error_emails(), m_msg_box);
        if (wrk_email)
        {
        	wrk_email->send_mail();
        	delete wrk_email;
        	wrk_email = NULL;
        }
    }

}


void TErrorMonitor::onAdd_error(int m_error, char* dm, char* msg)
{
	if (!m_error) return;

    if (m_error <= ErrMonitor.m_registered_error_level) return;

    TDomInfo info;
    bool     fl_email = false;

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

        send_email(box);
    }
    else
    {
        info.m_counter++;
        info.msg_box_size += m_msg_box_size;
    }
    domain[dm] = info;
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

inline void TErrorMonitor::exceeded_reset_timeout()
{

    time_t now;
    time(&now);

    map_domain  dm;
    TDomInfo    info;
    string      s;

    std::cout << "Domain " << domain.size() << endl;
    dm = domain;

    vmsg_box::iterator it_box;

    for (map_domain::iterator it = dm.begin(); it != dm.end(); it++)
    {
        s    = (*it).first;
        info = (*it).second;

        if (difftime(now, info.tm) >= ErrMonitor.m_reset_error_timeout)
        {
            it_box = info.msg_box.begin();

            if (domain.count(s))
            {
                while(it_box != info.msg_box.end())
                {
                    free((*it_box));
                    it_box = info.msg_box.erase(it_box);
                }

                domain.erase(s);
            }
        }
    }

    std::cout << "Domain " << domain.size() << endl;
}
