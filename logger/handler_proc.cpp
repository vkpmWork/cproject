/*
 * handler_proc.cpp
 *
 *  Created on: 06.07.2016
 *      Author: irina
 */

#include "handler_proc.h"

#include "config.h"
#include "common.h"
#include <assert.h>
#include <signal.h>
#include "loggermutex.h"
#include "file.h"

#include <fstream>
#include <stdio.h>
#include <time.h>
#include "base64.h"
#include <sys/socket.h>
#include <msgpack.hpp>
#include <map>

namespace handler_proc
{
pthread_t 	  message_handler_thread;
/* Prototypes */

void Msg_to_local_store (Mmessagelist ml, msgevent::tcEvent ev_code)
{
	std::cout <<"Msg_to_local_store\n";

	filethread *pWrk_file = new filethread(ml, ev_code);
	if (pWrk_file)
	{
		ev_code == msgevent::evMsg ? pWrk_file->RunWork() : pWrk_file->TryToDeleteFile();
		delete pWrk_file; pWrk_file = NULL;
	}

}

void Msg_to_remote_store(Mmessagelist ml)
{
	std::cout << " Msg_to_remote_store\n";
}

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

//	timerclear(&tval.it_interval); /* нулевой интервал означает не сбрасывать таймер */
//	timerclear(&tval.it_value);

	div_t d = div(value, 1000);


	tval.it_value.tv_sec     = d.quot; /* тайм-аут до срабтывания, секунд */
	tval.it_value.tv_usec    = d.rem*1000; /* тайм-аут до срабтывания, микросекунды */

	tval.it_interval.tv_sec  = 0; /* интервал срабатывания,  секунд */

    (void)setitimer(ITIMER_REAL, &tval, NULL);
    (void)signal(SIGALRM, NULL);

}

void *handler_client_thread(void *m_clientfd)
{
    ushort ErrorHeadersValue   = 0;
	int m = *(int*)m_clientfd;

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
                             std::cout << " Size: " << sz << " Buffer: " << str << endl;
                     }
                     else
                     {
                             if (errno == EAGAIN) continue;
                             std::ostringstream message;
                             message << errno << " " << time(0) << " net_recv(): " << buf << " Received : " << strerror(errno) << std::endl;
                             logger::write_log(message);
                     }
             }
             while (sz > 0);
             free(buf);
     }

    /* Clean up the client socket */
    shutdown (m, SHUT_RDWR);
    close(m);


    std::cout << str.data() << std::endl;    // ["Hello," "World!"]

    // Deserialize the serialized data.
    msgpack::object_handle oh = msgpack::unpack(str.data(), str.size());
    msgpack::object obj = oh.get();

    // Print the deserialized object to stdout.
    std::cout << obj << std::endl;    // ["Hello," "World!"]

    // Convert the deserialized object to staticaly typed object.
    std::map<std::string, int> result;
    obj.convert(result);

    for (std::map<std::string, int>::iterator it = result.begin(); it != result.end(); it++)
    {
    	std::cout << (*it).first << " : " << (*it).second << endl;
    }
    return NULL;

    std::ostringstream ss;
    ss << m_child << " " << str << endl;
    wmsg(ss, common::levDebug);
    std::cout << "Exit(0)" << endl;


    TLogMsg SocketMessage(str);
    ErrorHeadersValue |= SocketMessage.ErrorHeadersValue();

    if ((ErrorHeadersValue & ERROR_HEADER_FORMAT) == 0) pClientMessage->AddMessage(&SocketMessage);
    if (ErrorHeadersValue)
    {
        ss.str("");
        ss << "SocketRunnable::Message ErrorHeadersValue = " + atos(ErrorHeadersValue);
        wmsg(ss, common::levWarning);
    }

	return NULL;
}

void	create_client_thread(int *m_clientfd)
{
	   pthread_t 	  a_thread;
	   pthread_attr_t thread_attr;

	   std::ostringstream m;
	   if (pthread_attr_init(&thread_attr) != 0)
	   {
		   m.str("Attribute creation failed (create_client_thread(int *m_clientfd))\n");
		   logger::write_log(m);
		   return;
	   }

	    // переводим в отсоединенный режим
	   if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) != 0)
	    {
	    	m.str("Setting detached attribute failed (create_client_thread(int *m_clientfd))\n");
    	    logger::write_log(m);
	    	return;
	    }

	    if ( pthread_create(&a_thread, &thread_attr, handler_client_thread, (void*)m_clientfd) != 0) std::cout << "In  : Thread creation failed (create_thread_connect ( int *m_clientfd))\n";

	    (void)pthread_attr_destroy(&thread_attr);


}

void *handler_message_thread(void*)
{
	sigset_t mask, oldmask;
	sigemptyset(&mask);
	sigaddset(&mask, SIGUSR2);

	/*
	 * Блокируем доставку сигналов SIGUSR2
	 * После возвращения предыдущее значение
	 * заблокированной сигнальной маски хранится в oldmask
	 */

	sigprocmask(SIG_BLOCK, &mask, &oldmask);

	/* Задаём время ожидания 1 с */
	struct timespec tv;
	tv.tv_sec  = 100;
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
      std::cout << "TimeOut\n";
	  if ((recv_sig = sigtimedwait(&mask, &siginfo, &tv)) == -1)
	  {
	    if (errno == EAGAIN) continue;

	    perror("sigtimedwait");
        break;
      }

	  printf("signal %d received. Code %i\n",  recv_sig,  siginfo._sifields._rt.si_sigval.sival_int);

	  union sigval svalue;
	  svalue.sival_int =  siginfo._sifields._rt.si_sigval.sival_int;

	  if (svalue.sival_int != msgevent::evMsg && svalue.sival_int != msgevent::evDelete) continue;

	  if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexLock();
	  Mmessagelist mml =  pClientMessage->message_list;
	  pClientMessage->ClearMessageList();
	  if (pLoggerMutex) pLoggerMutex->ClientMessage_MutexUnlock();

	  pConfig->CurrentStoreType == Logger_namespace::LOCAL_STORE ? Msg_to_local_store (mml, (msgevent::tcEvent)svalue.sival_int) : Msg_to_remote_store(mml);
	}
    std::cout << "4\n";
    return NULL;
}


void	create_message_handler_thread()
{
	   if ( pthread_create(&message_handler_thread, NULL, handler_message_thread, NULL) != 0) std::cout << "In handler_timer : Thread creation failed\n";

       return;

       pthread_attr_t thread_attr;

	   std::ostringstream m;
	   if (pthread_attr_init(&thread_attr) != 0)
	   {
		   m.str("Attribute creation failed (create_message_handler_thread(v_messagelist m_message)\n");
		   logger::write_log(m);
		   return;
	   }

	    // переводим в отсоединенный режим
	   if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) != 0)
	    {
	    	m.str("Setting detached attribute failed (create_message_handler_thread(v_messagelist m_message))\n");
	    	logger::write_log(m);
	    	return;
	    }

	    if ( pthread_create(&message_handler_thread, &thread_attr, handler_message_thread, NULL) != 0) std::cout << "In  : Thread creation failed (create_message_handler_thread(v_messagelist m_message))\n";

	    (void)pthread_attr_destroy(&thread_attr);
}

} /* namespace common */




