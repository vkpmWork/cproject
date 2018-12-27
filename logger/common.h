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
#include <map>

//#define _DEBUG
//#define LOCAL
//#define DEAMON
#define MARKER_END                 "\n"

#define DELIMITER     		'/'
#define DEFAULT_DIR_MODE    "775"
#define DEFAULT_FILE_MODE   "665"

#define COMMAND_DELETE             "DELETE"
#define COMMAND_RECONFIG           "RECONFIG"
#define COMMAND_EXIT               "EXIT"

#define STRDELIMITER               "/"
#define END_STR                    "\0"
#define MSG_SIZE       sizeof(short)

using namespace std;

template<typename T>
string atos(T a)
{
    stringstream s;
    s << a;
    return s.str();
}

namespace msgevent
{
    enum tcEvent { evEmpty = 0, evMsg, evDelete, evConfig, evExit, evError};
}

namespace Logger_namespace
{

    enum tcLogLevel { levDebug = 0, levWarning, levError };
    enum tcStore {EMPTY_STORE = 0, LOCAL_STORE, REMOTE_STORE, POSTPONE_STORE /* восстановление связи с сервером */} ;

    inline bool IsFileExists(std::string str)
    {
        return (access(str.c_str(), 0) == 0);
    }

    union u_eventdelete
    {
        /* find_first_of */
        struct
        {
            char  file_delete_error : 1;
            char  dir_delete_error  : 1;
            char  file_not_found    : 1;
            char  dir_not_found     : 1;
            char  reserv            : 4;
        } s_eventdelete;

        unsigned char eventdelete;
    };
}

namespace common
{
	typedef std::map<string,string> tdata_map;
	typedef std::vector<char*> MProcess;

	enum tcLogLevel { levDebug = 0, levWarning, levError };

	extern const int 	TYPE_INTERNAL;
	extern const int 	TYPE_LUA;

	extern const char   *version;
	extern char	      	*application_name;

	extern bool			flag_working;
	typedef std::vector<char*> vmsg_box;

	bool 			FileExists(char* str);
	bool 			FileStatExists(char* str);
	bool            FileStatExists(char* str, std::string &s);
	char			*pathname(char*); /* возвращает указатель на распределенную память внутри функии. Требует free(распределенной памяти)*/

	void			ApplicationVersion();
	bool     		MakeDirectory(char*);
	void 			DisposeCommon();
	void     		LOG_OPER(string format_string, ...);

	int 			BecomeDaemonProcess();
	char 			*xstrlower(char *);
	char 			*xstrupper(char *);
	char			*xtrim(char*);

	string 			urlEncode(string);

	extern unsigned int    	time_start, time_stop;
	unsigned int            mtime();
	time_t			time_now();
	bool			up_time(int /* интервал */, time_t& /* начало отсчета интервала */); /* true - время вышло */
        bool                    up_time(struct tm& /* начало отсчета интервала */); /* true - время вышло */
	bool 			MakeDirectory(char*);
	pid_t 			getPid();
    pid_t           getPid(char*);

	std::string     getCmdOutput1(const std::string& mStr);
	MProcess 	    getCmdOutput(const std::string& mStr);
	pid_t    		GetPid(MProcess m);

	std::string		GetSubPath(int m_id);
	long long int 	ConvertToDecFromOct(string s);

	extern string   ClearDirectory(char *);


}
#endif /* COMMON_H_ */
