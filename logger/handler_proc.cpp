/*
 * handler_proc.cpp
 *
 *  Created on: 06.07.2016
 *      Author: irina
 */

#include "handler_proc.h"

#include "config.h"
#include "common.h"
#include "log.h"
#include <assert.h>


#include <fstream>
#include <stdio.h>
#include <time.h>
#include "base64.h"
#include <sys/socket.h>

namespace handler_proc
{

/* Prototypes */
void handler_timer(int signo)
{

	   pthread_t 	  a_thread;
	   pthread_attr_t thread_attr;

	   std::ostringstream m;
	   if (pthread_attr_init(&thread_attr) != 0)
	   {
		   m.str("Attribute creation failed (in handler_timer: pthread_attr_init(thread_attr))\n");
		   logger::write_log(m);
		   return;
	   }

	    // переводим в отсоединенный режим
	   if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) != 0)
	    {
	    	m.str("Setting detached attribute failed (in handler_timer)\n");
       	        logger::write_log(m);
	    	return;
	    }

//	    if ( pthread_create(&a_thread, &thread_attr, handler_playlist_sign, NULL) != 0) std::cout << "In handler_timer : Thread creation failed\n";

	    (void)pthread_attr_destroy(&thread_attr);
//	    flag_timer = true;
}

void   track_timeout(int value)
{
	struct itimerval tval;

	timerclear(&tval.it_interval); /* нулевой интервал означает не сбрасывать таймер */
	timerclear(&tval.it_value);

	div_t d = div(value, 1000);


	tval.it_value.tv_sec     = d.quot; /* тайм-аут до срабтывания, секунд */
	tval.it_value.tv_usec    = d.rem*1000; /* тайм-аут до срабтывания, микросекунды */

	tval.it_interval.tv_sec  = 0; /* интервал срабатывания,  секунд */

    (void)setitimer(ITIMER_REAL, &tval, NULL);
    (void)signal(SIGALRM, NULL);

}

void *handler_playlist_thread(void *m_clientfd)
{
	int m;
	m = *(int*)m_clientfd;

    pid_t m_child = getpid();

    usleep(200*1000);

    std::string str;

     int   sz          = 0;
     char* buf         = (char*)calloc(50001, sizeof(char));
     if (buf)
     {
             do
             {
                     sz = recv(m, buf, 50000, 0);

                     if (sz > 0)
                     {
                             str.append(buf, sz);
                             std::cout << "Size: " << sz << " Buffer: " << buf << endl;
                     }
                     else
                     {
                             if (errno == EAGAIN) continue;
                             std::ostringstream message;
                             message << errno << " " << time(0) << " net_recv(): " << buf << " Received : " << strerror(errno) << std::endl;
                             if (logger::ptr_log->get_loglevel() == common::levDebug) logger::write_log(message);
                     }
             }
             while (sz > 0);
             free(buf);
     }

    /* Clean up the client socket */
    shutdown (m, SHUT_RDWR);
    close(m);

    std::ostringstream ss;
    ss << m_child << str << endl;
    wmsg(ss, common::levDebug);
    std::cout << "Exit(0)" << endl;

	return NULL;
}

void	on_client_connect(int *m_clientfd)
{
	   pthread_t 	  a_thread;
	   pthread_attr_t thread_attr;

	   std::ostringstream m;
	   if (pthread_attr_init(&thread_attr) != 0)
	   {
		   m.str("Attribute creation failed (on_client_connect(int *clientfd))\n");
		   logger::write_log(m);
		   return;
	   }

	    // переводим в отсоединенный режим
	   if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) != 0)
	    {
	    	m.str("Setting detached attribute failed (on_client_connect(int *clientfd))\n");
    	    logger::write_log(m);
	    	return;
	    }

	    if ( pthread_create(&a_thread, &thread_attr, handler_playlist_thread, (void*)m_clientfd) != 0) std::cout << "In  : Thread creation failed\n";

	    (void)pthread_attr_destroy(&thread_attr);

}


} /* namespace common */




