/*
 * log.h
 *
 *  Created on: 18.08.2016
 *      Author: irina
 */

#ifndef LOG_H_
#define LOG_H_

#include "common.h"
#include <fstream>

#define ERR_FILE_EMPTY	    300	 /*Track file size = 0*/
#define ERR_SAMPLE_RATE	    301	 /*Track file size = 0*/

#define ERR_CONFIG_FILE   	350

#define ERROR_PLAYLIST_EMPTY 400 /* Playlist is empty */
#define ERROR_PLAYLIST_QUERY 401 /* Playlist didn't receive */
#define ERR_METADATA		 402 /* что-то неправильно в информации о ткере */

#define ERR_CONNECT_200		450
#define ERR_FINISHED		500

#define COMMENT_INFO   "#ERROR "

#define CODE_ARG       " 1 "
#define CODE_STARTED   " 2 "
#define CODE_CFG       " 3 "

namespace logger
{

class Tlog
{
protected:
	char* 		        log_file;
	bool			log_error;
	common::tcLogLevel 	log_level;
	std::ostringstream  header;

	void 				check_file(FILE*);

public:
	Tlog(char* m_path = NULL, common::tcLogLevel m_level = common::levError);
	virtual ~Tlog();
	void 	 write(std::string);
	FILE*    open();
	void     close(FILE*);
	bool     error()					{ return log_error; }
	common::tcLogLevel  get_loglevel()  { return log_level; }
	void     set_header(std::ostringstream &msg);
	void  	 check_log_size(int);
	char*    get_log_name() { return log_file; }

};

class TStartLog :public Tlog
{
private:
  bool is_error;
public:
  TStartLog(char* m_file);

  void     write_arg    (std::string str)
  {
      if (is_error || str.empty()) return;
      char *c_tm = str_time();

      std::ostringstream m;
      m << c_tm << CODE_ARG << "It isn't enough arguments: " << str << endl;

      free(c_tm);

      Tlog::write(m.str());
  }

  void     write_started(std::string str)
  {
    if (is_error || str.empty()) return;
    char *c_tm = str_time();

    std::ostringstream m;
    m << c_tm << " Channel "  << str << " is already started" << endl;

    free(c_tm);

    Tlog::write(m.str());
  }

  void     write_cgf    (std::string str)
  {
    if (is_error || str.empty()) return;
    char *c_tm = str_time();

    std::ostringstream m;
    m << c_tm << CODE_CFG << "Configuration file " << str << " not found!\n";

    free(c_tm);

    Tlog::write(m.str());
  }

private:

  char*     str_time()
  {
    char *stime = (char*)calloc(26, 1);

    time_t rawtime;
    time (&rawtime);

    ctime_r(&rawtime, stime);
    char *c = strchr(stime,'\n');
    *c = ' ';

    return stime;
  }
};

class TTitrLog : public Tlog
{
	private:
	char*				path;
	char				channel_type;
	int 				channel_id;

	public:
	TTitrLog(char* m_file, char m_channel_type, int m_channel_id, common::tcLogLevel m_level);
	virtual ~TTitrLog();

	void 	 write_comment(std::ostringstream &msg);
	void 	 write(std::string str);

	private:
	void  	set_log_name  (struct tm* _tm);
	bool  	check_log_date(struct tm* &);
};

class TInfoLog : public Tlog
{
	private:
	char*	path;
	char	channel_type;
	int 	channel_id;
	int 	log_size;
	char    *info_marker;

	public:
	TInfoLog(char* m_file, char m_channel_type, int m_channel_id, int logsize, common::tcLogLevel m_level, char* m_info_marker);
	virtual ~TInfoLog();

	void 	 write_info  (std::ostringstream &msg);
	void     write_marker(std::ostringstream &msg);
	void 	 write(std::string str);

	private:
	void  	set_log_name  ();
};

extern TInfoLog  *ptr_log;
extern TTitrLog  *titr_log;

extern void write_titrs(std::ostringstream &);
extern void write_log(std::ostringstream &);
extern void write_log(std::ostringstream &, common::tcLogLevel lev);
extern void write_info(std::ostringstream &);
extern void write_marker(std::ostringstream &);
extern void write_info(std::ostringstream &, common::tcLogLevel lev);
extern void header_log(std::ostringstream &);
extern void header_log(std::ostringstream &, void*);
extern void write_comment(std::ostringstream &msg);
extern char* get_log_name();
extern void delete_log();

extern std::ostringstream message;
} /* namespace log */
#endif /* LOG_H_ */
