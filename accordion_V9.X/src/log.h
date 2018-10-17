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

namespace logger
{

class Tlog
{
protected:
	char* 		        log_file;
//	std::ostringstream  message;
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

extern TInfoLog *ptr_log;
extern TTitrLog *titr_log;
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
