/*
 * handler_proc.h
 *
 *  Created on: 06.07.2016
 *      Author: irina
 */

#ifndef HANDLER_PROC_H_
#define HANDLER_PROC_H_

#include <iostream>
#include <signal.h>
#include <sys/time.h>
#include <pthread.h>
#include "clientmessage.h"
#include "log.h"
#include "handler_proc.h"

/*
 * Компилировать:
 * g++ -D_REENTRANT -o threads threads.c -lpthread
 */

namespace handler_proc
{
extern pthread_t 	  message_handler_thread;


void  		 track_timeout(int value);
void 		*handler_client_thread(void *m_clientfd);
void		 create_client_thread(int *clientfd);

void 		*handler_message_thread(void*);
void		 create_message_handler_thread();
}
#endif /* HANDLER_PROC_H_ */
