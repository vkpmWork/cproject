/*
 * Parameters.h
 *
 *  Created on: 21.02.2018
 *      Author: irina
 */

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string>
#include "log.h"
#include "config.h"

#include <sys/wait.h> /* waitpid */
#include <netdb.h> /* getaddrinfo */
#include "common.h"


namespace parameters
{
typedef bool (*sendParameters)(int, char*, int, pid_t);
typedef bool (*GetFileSegment)(int, int&, int&);

typedef std::map<int, int> MRange;

typedef struct tConfigInfo
{
  common::tcLogLevel  m_log;
  char               *path_media,
                     *host;
  int                 start_position,
                      duration,
                      token_lifetime;
  tpath_media         aliases;

  void TConfigInfo()  { m_log = common::levDebug; path_media = NULL; start_position = 10;  duration = 40; token_lifetime = 30000; }
} TConfigInfo;


typedef struct tMsg_data
{
   char *ip;
   char *username;
   char *userpass;
   char *request_time;
   char *request;
   int   response;
   int   response_size;
   char *user_agent;
   float response_time; // seconds
   char *referer;

   int   header_size;
   int   file_size;
   int   range_start;
   int   range_stop;


} TMsg_data;

class TParameters
{
public:
	TParameters(std::string m_str, int m_clientfd = -1, pid_t m_child = -1, common::tcLogLevel m_log = common::levDebug, int m_TO = 10, char *m_path = NULL);
        TParameters(std::string m_str, int m_clientfd, pid_t m_child, TConfigInfo m);
	virtual ~TParameters();
	int 	Status()	        { return pid_status; }
        bool        create_answer (parameters::sendParameters f);
        bool        create_answer1(parameters::sendParameters f);
        std::string get_log_str();
private:
        TConfigInfo ConfigInfo;
        struct
        {
          bool  is_aac;
          bool  is_mp3;

          char *http;

          int   id;
          char *module;

          char *date;
          char *hash;
          int   range_f;
          int   range_l;

          int   start_position;
          int   duration;
          char  *token;
          char  *alias;

        } mData;

        int 		clientfd;       /* client's socket */
	pid_t		pid_child;
	char            *recv_str;
	int     	pid_status;
	common::tcLogLevel log_level;
	MRange          mRange;
	int             transmit_timeout;
//	std::string     path_media;
	TMsg_data       msg_log;

	GetFileSegment  pFileSegment;

	void get_file_data(char* v);
        std::string sparse();
        bool bsend(char *buf, int length);
        bool bsendfile(int fd, int m_begin, int length);
};

extern void query_parameters(int m_clientfd, char *, parameters::sendParameters, pid_t m_pid, common::tcLogLevel m_log, int m_TO, char* m_path);
extern std::string query_parameters(int m_clientfd, char *, parameters::sendParameters, pid_t m_pid, TConfigInfo m);
} /* namespace parameters */

#endif /* PARAMETERS_H_ */
