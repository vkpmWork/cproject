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

/*
 * Компилировать:
 * g++ -D_REENTRANT -o threads threads.c -lpthread
 */

namespace handler_proc
{


void  		 track_timeout(int value);
void *handler_playlist_thread(void *m_clientfd);
void		 on_client_connect(int *clientfd);
}
#endif /* HANDLER_PROC_H_ */
