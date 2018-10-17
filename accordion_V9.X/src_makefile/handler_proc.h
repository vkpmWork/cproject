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
#include "parameters.h"

/*
 * Компилировать:
 * g++ -D_REENTRANT -o threads threads.c -lpthread
 */

namespace handler_proc
{

struct Targ_
{
	char *m_file;
	int   play_time;
};

typedef struct Targ_ DATA;
extern void  handler_timer(int);
extern void  handler_get_playlist(char*);
extern void  query_playlist_detach(char*);

extern void *handler_playlist_thread(void *arg);
extern void *handler_metadata_thread(void *arg);
extern void *handler_playlist_sign(void*);

void  		 track_timeout(int value);
}
#endif /* HANDLER_PROC_H_ */
