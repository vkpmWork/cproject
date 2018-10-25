/*
 * common.h
 *
 *  Created on: 26.01.2016
 *      Author: irina
 */

#ifndef COMMON_H_
#define COMMON_H_
#include <string.h>
#include <stdio.h>
#include <iostream>

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <cerrno>
#include <string>
#include <sstream>
#include <vector>
#include <sys/time.h>

// #define _DEBUG

#define DELIMITER     '/'
#define BUFFER_SIZE  1024

using namespace std;

template<typename T>
string atos(T a)
{
    stringstream s;
    s << a;
    return s.str();
}

namespace common
{
	typedef std::vector<char*> MProcess;

	enum tcLogLevel { levDebug = 0, levWarning, levError };

	extern const int 	TYPE_INTERNAL;
	extern const int 	TYPE_LUA;

	extern const char       *version;
	extern char	        *application_name;
	extern char             *pidfile_name;


	extern bool		flag_working;

	bool 			FileExists(char* str);
	bool 			FileStatExists(char* str);
	char			*pathname(char*); /* возвращает указатель на распределенную память внутри функии. Требует free(распределенной памяти)*/

	void			ApplicationVersion(char*);
	bool     		MakeDirectory(char*);
	void 			DisposeCommon();
	void     		LOG_OPER(string format_string, ...);

	int 			BecomeDaemonProcess();
	char 			*xstrlower(char *);
	char 			*xstrupper(char *);
	char			*xtrim(char*);

	string 			urlEncode(string);

//	int    			mtime();
	unsigned int  	        mtime();
	time_t			time_now();
	bool			up_time(int /* интервал */, time_t& /* начало отсчета интервала */); /* true - время вышло */
	std::string             str_time();
	bool 			MakeDirectory(char*);
	pid_t 			getPid();

	std::string 	        getCmdOutput1(const std::string& mStr);
	MProcess 	        getCmdOutput(const std::string& mStr);
	pid_t    		GetPid(MProcess m);

	std::string		GetSubPath(int m_id);
	float                   time_between   (struct timeval);
}
#endif /* COMMON_H_ */
