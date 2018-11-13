/*
 * log.cpp
 *
 *  Created on: 18.08.2016
 *      Author: irina
 */


#include "log.h"
#include <string>       // std::string
#include <sstream>      // std::ostringstream
#include <stdio.h>
#include <sys/stat.h>
#include <string>         // std::string
#include <limits.h>

#define  COMMENT_MARKER	"#	"
#define  COMMENT_INFO	"#ERROR "

namespace logger
{

TInfoLog *ptr_log  = NULL;
TTitrLog *titr_log = NULL;

char* get_log_name()
{
	if (ptr_log) return ptr_log->get_log_name();

	return NULL;
}

void write_titrs(std::ostringstream &msg)
{
	if (titr_log) titr_log->write(msg.str());
}

void write_comment(std::ostringstream &msg)
{
	if (titr_log) titr_log->write_comment(msg);
}

void write_log(std::ostringstream &msg)
{

	if (ptr_log)
	{
              time_t rawtime;
              time (&rawtime);

              char *stime = (char*)calloc(26, 1);

              ctime_r(&rawtime, stime);
              char *c = strchr(stime,'\n');
              *c = ':';

              std::ostringstream s;
              s << stime << "  " << msg.str() << endl;

              free(stime);

              ptr_log->write(s.str());
	}
}

void write_info(std::ostringstream &msg)
{
	if (ptr_log) ptr_log->write_info(msg);
}

void write_log(std::ostringstream &msg, common::tcLogLevel lev)
{
	if ((ptr_log != NULL) && (lev >= ptr_log->get_loglevel())) write_log(msg);
}

void write_info(std::ostringstream &msg, common::tcLogLevel lev)
{
	if ((ptr_log != NULL) && (lev >= ptr_log->get_loglevel())) ptr_log->write_info(msg);
}

void header_log(std::ostringstream &msg, void* ptr)
{
	TTitrLog *t = dynamic_cast<TTitrLog*>((Tlog*)ptr);
	if (t)
	{
		std::string str = msg.str();
		t->set_header(msg);
		t->write_comment(msg);
	}
	else
	{
		TInfoLog *t = dynamic_cast<TInfoLog*>((Tlog*)ptr);
		if (t)
		{
			t->set_header(msg);
			t->write(msg.str());
		}

	}

}

void delete_log()
{
	if (ptr_log)
	{
		delete ptr_log;
		ptr_log = NULL;
	}

	if (titr_log)
	{
		delete titr_log;
		titr_log = NULL;
	}

}

/* ========================= Tlog ================== */

Tlog::Tlog(char* m_file, common::tcLogLevel m_level, char *m_header) :
	  log_file(NULL)
	, log_error(true)
	, log_level(m_level)
{
  header = strdup(m_header);
}

Tlog::~Tlog()
{
	free(header);
	free(log_file);
}

FILE* Tlog::open()
{
	char *a = common::pathname(log_file);

	log_error = !common::MakeDirectory(a);
	free(a);

	if (log_error) return NULL;

	FILE *outfile = fopen(log_file, "a+");
	if (!outfile)
	{
		std::cout << "Can't open file " << log_file << " "<< strerror(errno) << std::endl;
		log_error = true;
	}
	return outfile;
}

void Tlog::write(std::string str)
{
	if (str.empty()) return;

//	pFileMutex->lock();
	FILE *outfile = this->open();
	if (outfile)
	{
		fwrite(str.c_str(), sizeof(char), str.length(), outfile);
		fclose(outfile);
	}
//        pFileMutex->unlock();
}

void Tlog::close(FILE *outfile)
{
	if (outfile) fclose(outfile);
}


void Tlog::set_header(std::ostringstream &msg)
{
}

void Tlog::check_log_size(int fsize)
{
	if (!fsize) return ;

	struct stat buf;

	if (stat(log_file, &buf) == 0)
	{
		if (buf.st_size >= fsize)
		{
			char *b = NULL, *e = NULL;
			int sz  = 3*512;//2*1024;

			FILE *outfile = this->open();
			if (fseek(outfile, (-1)*sz, SEEK_END) == 0)
			{
				b = (char*)calloc(sz, sizeof(char));
				if (b)
				{
					sz = fread(b, 1, sz, outfile);
					if (sz)
						e = strstr(b, COMMENT_INFO);
				}
			}

			outfile = freopen(log_file, "w", outfile);
			fwrite(header, 1/*sizeof(char)*/, strlen(header), outfile);
			if (e)
				fwrite(e, 1/*sizeof(char)*/, sz, outfile);

			if (b) free(b);
			close(outfile);
		}
	}
}

/* ========================= TTitrLog ================== */
TTitrLog::TTitrLog(char* m_file, char m_channel_type, int m_channel_id, common::tcLogLevel m_level) : Tlog(m_file, m_level)
	,	path(NULL)
	,	channel_type(m_channel_type)
	,	channel_id  (m_channel_id)
{

		if (!m_file) return;

		path  = strdup(m_file);

		time_t rawtime;
		time (&rawtime);

		set_log_name(localtime (&rawtime));

		FILE *outfile = open();
		if (outfile) close(outfile);
}

TTitrLog::~TTitrLog()
{
	free(path);
}

void TTitrLog::write_comment(std::ostringstream &msg)
{

			std::ostringstream s;
			time_t rawtime;
			time (&rawtime);

			char *stime = (char*)calloc(26, 1);

			ctime_r(&rawtime, stime);
			char *c = strchr(stime,'\n');
			*c = ':';

			s << COMMENT_MARKER << stime << "  " << msg.str();
			std::string ss = s.str();

			size_t pos = ss.find("\n");
			while (pos != std::string::npos && pos < (ss.length()-1))
			{
					ss.insert(pos+1, COMMENT_MARKER, strlen(COMMENT_MARKER));
					pos = ss.find("\n", pos+strlen(COMMENT_MARKER));
			}
			write(ss);
}

void  TTitrLog::write(std::string str)
{
	struct tm  *timeinfo;
	if (check_log_date(timeinfo)) set_log_name(timeinfo);

	Tlog::write(str);
}

bool TTitrLog::check_log_date(struct tm* &timeinfo)
{
	struct stat buf;
	bool ret_value = false;


	if (stat(log_file, &buf) == 0)
	{
		time_t 		rawtime, filetime;
		struct tm  *fileinfo;

		time (&rawtime);
		timeinfo = localtime (&rawtime);

		filetime = buf.st_ctime;
		fileinfo = localtime (&filetime);

		ret_value = (timeinfo->tm_mon != fileinfo->tm_mon) || (timeinfo->tm_year != fileinfo->tm_year);
	}

	return ret_value;
}

void TTitrLog::set_log_name(struct tm* timeinfo)
{
	if (log_file) free(log_file);
	char b[80];
	strftime(b, 80, "%y%m", timeinfo);

	char *buffer = (char*)calloc(PATH_MAX, sizeof(char));

	int len = strlen(path);
	if (path[len-1] == DELIMITER) sprintf(buffer, "%s%s/t%c%i_%s.log", path, common::GetSubPath(channel_id).c_str(), channel_type, channel_id, b);
	else sprintf(buffer, "%s/%s/t%c%i_%s.log", path, common::GetSubPath(channel_id).c_str(), channel_type, channel_id, b);

	log_file  = strdup(buffer);
	free(buffer);
}

/* ========================= TInfoLog ================== */
TInfoLog::TInfoLog(char* m_file, int logsize, common::tcLogLevel m_level, char *m_header) : Tlog(m_file, m_level, m_header)
	,	path(NULL)
	, 	log_size(logsize)

{
		if (!m_file) return;

		time_t rawtime;
		time (&rawtime);

		log_file  = strdup(m_file);

		write(header);
}

TInfoLog::~TInfoLog()
{
	free(path);
}

void 	 TInfoLog::write(std::string str)
{
    check_log_size(log_size);
	Tlog::write(str);

}

void 	 TInfoLog::write_info(std::ostringstream &msg)
{
	std::string str;
	str.append(msg.str());

	write(str);
}

} /* namespace log */
