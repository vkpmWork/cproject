/*
 * main_dispatcher.h
 *
 *  Created on: 08.02.2016
 *      Author: irina
 */

#ifndef MAIN_DISPATCHER_H_
#define MAIN_DISPATCHER_H_
#include <iostream>
#include "log.h"
#include "common.h"
#include "config.h"

extern int dispatcher(char**, int);
extern std::string get_info();
extern void Set_SIGUSR1();
extern void Set_SIGDELETE();
#endif /* MAIN_DISPATCHER_H_ */
