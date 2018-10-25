/*
 * Parameters.cpp
 *
 *  Created on: 21.02.2018
 *      Author: irina
 */

#include "Parameters.h"
#include "time.h"
#include <sys/sendfile.h>
#include <sys/stat.h>
#include "aacbitrate.h"
#include "mp3bitrate.h"
#include <iostream>

#define STATUS_OK	   0
#define STATUS_EMPTY	0x01
#define STATUS_ID	0x02
#define STATUS_MODULE	0x04
#define STATUS_NOFILE	0x08
#define STATUS_NOOPEN	0x10
#define STATUS_NOREAD   0x20
#define STATUS_SEND     0x40
#define STATUS_MP3      0x80

#define MARKER_ID       "id="
#define MARKER_MODULE   "module="
#define MARKER_Q	'&'
#define MARKER_PARAM    '?'

#define MARKER_EMPTY    "-"
#define EMPTY_LEN       strlen(MARKER_EMPTY)

#define MARKER_TAB      ' '

#define TOKEN_SIZE        32

//#define RESPONSE        "HTTP/1.1 200 OK\nAccept-Ranges: bytes\nConnection: keep-alive\nAccess-Control-Allow-Origin:*\nContent-Range: bytes %i-%i/%i\nContent-Length: %i\nContent-Type: audio/%s;q=0.9,audio/mpeg,*/*;q=0.8\nDate:%s GMT\nCache-Control: no-cache, no-store, max-age=0\nPragma: no-cache\nKeep-Alive: timeout=3\n\r\n\r\n"
//#define RESPONSE206     "HTTP/%s 206 Partial Content\nDate: %s GMT\nContent-Range: bytes %i-%i/%i\nContent-Length: %i\nConnection: close\nContent-Type: audio/%s;q=0.9,audio/mpeg,*/*;q=0.8\nAccess-Control-Allow-Origin: *\nCache-Control: no-cache, no-store, max-age=0\nPragma: no-cache\n\n"
#define RESPONSE206     "HTTP/%s 206 Partial Content\nDate: %s GMT\nContent-Range: bytes %i-%i/%i\nContent-Length: %i\nConnection: keep-alive\nContent-Type: audio/%s;q=0.9,audio/mpeg,*/*;q=0.8\nAccess-Control-Allow-Origin: *\nCache-Control: no-cache, no-store, max-age=0\nPragma: no-cache\nKeep-Alive: timeout=3\n\n"

#define BADREQUEST      "HTTP/%s 405 Method Not Allowed\n\n"
#define NOTFOUND        "HTTP/%s 404 Not Found\n\n"
#define FORBIDDEN       "HTTP/%s 403 Forbidden\n\n"

#define AAC             ".aac"
#define MP3             ".mp3"

#define AAC_TYPE        "aac"
#define MPEG_TYPE       "mpeg"

#define DEFAULT_ALIAS   "101.ru"

namespace parameters
{

void TParameters::get_file_data(char* v)
{
    char *pdata = NULL, *pch = NULL;

    std::ostringstream msg;
    bool on_start    = true;

    while (true)
    {
          if (on_start)
          {
              pch = strtok (v, " ?&");
              on_start = false;
          }

          else pch = strtok (NULL, " ?&");

          if (!pch) break;

          pdata = strstr(pch, "token=");
          if (pdata)
          {
              mData.token = strdup(pdata+strlen("token="));
              if (mData.token && (strlen(mData.token) < TOKEN_SIZE))
              {    free(mData.token);
                   mData.token = NULL;
              }
              continue;
          }

          pdata = strstr(pch, "start=");
          if (pdata)
          {
              mData.start_position = atoi(pdata+strlen("start="));
              continue;
          }

          pdata = strstr(pch, "id=");
          if (pdata)
          {
              mData.id = atoi(pdata+strlen("id="));
              mData.is_aac = true;
              continue;
          }

          pdata = strstr(pch, "module=");
          if (pdata)
          {  mData.module = strdup(pdata+strlen("module="));
             continue;
          }

          pdata = strstr(pch, "date=");
          if (pdata)
          {
             std::string ds;
             ds.append(pdata+strlen("date="));

             std::size_t found = ds.find("%2F");
             if (found != std::string::npos) ds.replace(found, 3, "/");

             mData.date = strdup(ds.c_str());
             mData.is_mp3 = true;
             continue;
          }

          pdata = strstr(pch, "hash=");
          if (pdata)
          {  mData.hash = strdup(pdata+strlen("hash="));
             continue;
          }

          pdata = strstr(pch, "bytes=");
          if (pdata)
          {
                pdata += strlen("bytes=");
                if (pdata)
                {
                      mData.range_f = mData.range_l = 0;

                      char *pEnd;
                      mData.range_f = strtol(pdata, &pEnd, 10);
                      if (pEnd != NULL) mData.range_l  = abs(strtol(pEnd+1, NULL, 10));
                      mRange[mData.range_f] = mData.range_l;
                }
                continue;
          }

          pdata = strstr(pch, "alias=");
          if (pdata)
          {  mData.alias = strdup(pdata+strlen("alias="));
             continue;
          }


          pdata = strstr(pch, "login=");
          if (pdata)
          {  msg_log.username = strdup(pdata+strlen("login="));
             continue;
          }

          pdata = strstr(pch, "password=");
          if (pdata)
          {  msg_log.userpass = strdup(pdata+strlen("password="));
             continue;
          }

          pdata = strstr(pch, "HTTP/");
          if (pdata) mData.http = strdup(pdata+strlen("HTTP/"));

    }

}

void query_parameters(int m_clientfd, char *m_str, parameters::sendParameters f, pid_t m_pid, common::tcLogLevel m_log, int m_TO, char* m_path)
{
  TParameters *p = new TParameters(m_str, m_clientfd, m_pid, m_log, m_TO, m_path);
  if (p)
  {
      p->create_answer(f);

      delete p; p = NULL;
  }
}

std::string query_parameters(int m_clientfd, char *m_str, parameters::sendParameters f, pid_t m_pid, TConfigInfo m)
{
  std::string str = "";
  TParameters *p = new TParameters(m_str, m_clientfd, m_pid, m);
  if (p)
  {
      p->create_answer(f);
      str = p->get_log_str();
      delete p; p = NULL;
  }
  return str;
}


TParameters::TParameters(std::string m_str, int m_clientfd, pid_t m_child, TConfigInfo m)
: clientfd(m_clientfd)
, pid_child(m_child)
, pid_status(0)
, transmit_timeout(0)

{
    ConfigInfo.m_log = log_level = m.m_log;

/*    if (log_level == -1)
    {
        std::ostringstream msg;
        msg << pid_child << " Запрос: " << m_str     << endl;
        wmsg(msg, common::levError);
    }
*/
    mData.is_aac = false;
    mData.is_mp3 = false;

    mData.http   = NULL;
    mData.id     = 0;
    mData.module = NULL;

    mData.date   = NULL;
    mData.hash   = NULL;
    mData.token  = NULL;
    mData.alias  = NULL;

    mData.range_f= 0;
    mData.range_l= 0;

//    path_media.append(m.path_media);
    recv_str = strdup(m_str.c_str());

    ConfigInfo.start_position = m.start_position;
    ConfigInfo.duration       = m.duration;
    ConfigInfo.token_lifetime = m.token_lifetime;
    ConfigInfo.aliases        = m.aliases;

    msg_log.ip           = strdup(m.host);
    msg_log.request      = NULL;
    msg_log.response     = 206;
    msg_log.response_size= 0;
    msg_log.response_time= 0;
    msg_log.user_agent   = NULL;
    msg_log.username     = NULL;
    msg_log.userpass     = NULL;
    msg_log.referer      = NULL;
    msg_log.request_time = strdup(common::str_time().c_str());

    msg_log.file_size    = msg_log.header_size = msg_log.range_start = msg_log.range_stop = 0;
}


TParameters::TParameters(std::string m_str, int m_clientfd, pid_t m_child, common::tcLogLevel m_log, int m_TO, char *m_path)
			: clientfd(m_clientfd)
			, pid_child(m_child)
			, pid_status(0)
                        , log_level(m_log)
                        , transmit_timeout(m_TO)

{
/*        if (log_level == -1)
        {
            std::ostringstream msg;
            msg << pid_child << " Запрос: " << m_str     << endl;
            wmsg(msg, common::levError);
        }
*/
        mData.is_aac = false;
        mData.is_mp3 = false;

        mData.http   = NULL;
        mData.id     = 0;
        mData.module = NULL;

        mData.date   = NULL;
        mData.hash   = NULL;
        mData.token  = NULL;
        mData.alias  = NULL;

        mData.range_f= 0;
        mData.range_l= 0;

//        path_media.append(m_path);
        recv_str = strdup(m_str.c_str());
}

TParameters::~TParameters()
{
      free(msg_log.ip);
      free(msg_log.request);
      free(msg_log.request_time);
      free(msg_log.user_agent);
      free(msg_log.username);
      free(msg_log.userpass);
      free(msg_log.referer);
      free(mData.alias);

      free(recv_str);

      free(mData.http);
      free(mData.module);

      free(mData.date);
      free(mData.hash);
      free(mData.token);

}

std::string TParameters::sparse()
{
  std::ostringstream m;

  char *pshift = recv_str;
  char *pch    = strtok (recv_str,"\r\n");
  char *pdata  = NULL;

  while (pch != NULL)
  {
    pshift += (strlen(pch)+2);

    pdata = strstr(pch, "GET /");
    if (pdata)
    {
        msg_log.request = strdup(pdata);
        get_file_data(pdata);
    }
    else
    {
        pdata = strstr(pch, "Range:");
        if (pdata) get_file_data(pdata);
        else
        {
            pdata = strstr(pch, "User-Agent:");
            if (pdata) msg_log.user_agent = strdup(pdata);
            else
            {
                pdata = strstr(pch, "Referer:");
                if (pdata) msg_log.referer = strdup(pdata);
            }
        }
    }

    pch = strtok (pshift, "\r\n");
  }

  if (!mData.http) mData.http = strdup("1.1");

  if (mData.is_aac == false && mData.is_mp3 == false)
  {
    /*  m.str("");
      m  << pid_child << " AAC и МР3 формат: Не найден параметр id и date: recv_str = " <<  recv_str << endl;
      wmsg(m, common::levError);
    */
      pid_status = STATUS_EMPTY;
      return "";
  }

  if (mData.is_aac == true)
  {
      if (mData.id == 0 || mData.module == NULL)
      {
        /*  m.str("");
          m  << pid_child << " AAC формат: Не найден параметр id или module: recv_str = " <<  recv_str << endl;
          wmsg(m, common::levError);
        */
          pid_status |= (STATUS_ID | STATUS_MODULE);
          return "";
      }

      if (!mRange.size()) mRange[0] = 0;
  }

  if (mData.is_mp3 && (mData.date == NULL || mData.hash == NULL))
    {
     /* m.str("");
      m  << pid_child << "MP3 формат: Не найден параметр date или hash: recv_str = " << recv_str << endl;
      wmsg(m, common::levError);
     */
      pid_status |= (STATUS_ID | STATUS_MODULE);
      return "";

    }

  if (mData.alias == NULL) mData.alias = (char*)DEFAULT_ALIAS;

  std::ostringstream m_file;
  if (mData.is_aac)
    m_file << ConfigInfo.aliases[mData.alias] << mData.module << '/' << common::GetSubPath(mData.id) << '/' << mData.id << AAC;
  else
    m_file << ConfigInfo.aliases[mData.alias] << mData.date << '/' << mData.hash << MP3;

  return m_file.str();

}

bool TParameters::create_answer(parameters::sendParameters f)
{
        std::string file   = sparse();


        std::ostringstream msg;
	if (pid_status || file.empty())
	{
            char *data   = (char*)calloc(1024, sizeof(char));
            sprintf(data, BADREQUEST, mData.http);

            msg_log.response_size = strlen(data);
            bsend(data, msg_log.response_size) ;

            msg_log.response = 405;

            free(data);
	    return false;
	}

        if (!common::FileExists((char*)file.c_str()))
        {
              char *data   = (char*)calloc(1024, sizeof(char));
              sprintf(data, NOTFOUND, mData.http);

              msg_log.response_size = strlen(data);
              bsend(data, msg_log.response_size);
              free(data);

              msg_log.response = 404;

              return false;
        }

        if (log_level == common::levDebug)
        {
            msg  << pid_child << " File to play: "<< file;
            wmsg(msg, common::levDebug);
        }

        int mf = open(file.c_str(), O_RDONLY | O_NONBLOCK);
	if (mf == -1)
	{

            char *data   = (char*)calloc(1024, sizeof(char));
            sprintf(data, FORBIDDEN, mData.http);

            if (log_level == common::levDebug)
            {
                msg.str("");
                msg  << pid_child << " " << file << " : " << strerror(errno) << std::endl;
                msg  << pid_child << " " << data << endl;
                wmsg(msg, common::levError);
            }

	    pid_status |= STATUS_NOOPEN;

            msg_log.response_size = strlen(data);
	    bsend(data, msg_log.response_size);
	    free(data);

            msg_log.response = 403;

	    return false;
	}

	int m_len = lseek(mf, 0, SEEK_END);

	MRange::iterator it = mRange.begin();

	int data_begin = (*it).first;
	int data_end   = (*it).second;

        msg_log.file_size   = m_len;
        msg_log.range_start = data_begin;
        msg_log.range_stop  = data_end;

	if (log_level == common::levDebug)
	{
	    msg.str("");
            msg  << pid_child << " Range. Data begin: " << data_begin << " Data end: " << data_end << endl;
            wmsg(msg, common::levError);
	}

	if (lseek(mf, 0, SEEK_SET) == -1)
	{
            if (log_level == common::levDebug)
            {
                msg.str("");
                msg  << pid_child << " fseek " << endl;
                wmsg(msg, common::levError);
            }
	}

	int start_pos = 0, stop_pos = 0;

	if (mData.token == NULL)
	{
	    pFileSegment = mData.is_aac ?  aac::get_file_segment : mp3::get_file_segment;
	    if (pFileSegment(mf, start_pos = ConfigInfo.start_position, stop_pos = ConfigInfo.duration))
	    {
	        if (m_len > stop_pos) m_len = stop_pos;
	    }
	    else
	        msg  << pid_child << " pFileSegment = false" << endl;

	}

        if (!data_end) data_end = m_len-1;

        if (lseek(mf, start_pos + data_begin, SEEK_SET) == -1)
        {
            if (log_level == common::levDebug)
            {
                msg.str("");
                msg  << pid_child << " fseek " << endl;
                wmsg(msg, common::levError);
            }
        }

	char *data   = (char*)calloc(BUFFER_SIZE+1, sizeof(char));

        /* Передача заголовка */
        sprintf(data, RESPONSE206, mData.http, common::str_time().c_str(), data_begin, data_end, m_len, (data_end-data_begin)+1, mData.is_aac ? AAC_TYPE : MPEG_TYPE );

        if (log_level == common::levDebug)
        {
            msg.str("");
            msg << pid_child << " Заголовок ответа: " << data << endl;
            wmsg(msg, common::levError);
        }

        struct timeval t1;
        gettimeofday(&t1, NULL);

        msg_log.header_size = strlen(data);
        if (!bsend(data, msg_log.header_size))
        {
            msg_log.header_size = 0;

            msg.str("");
            msg  << pid_child << " Не могу отправить заголовок клиенту" << endl;
            wmsg(msg, common::levError);

            free(data);
            close(mf);

            return false;
        }

        memset(data, 0, BUFFER_SIZE+1);

        data_end++;
        msg_log.response_size = 0;
        while ( 0 != (m_len = read( mf, data, BUFFER_SIZE))  )
	{
	    if (m_len > data_end) m_len = data_end;

            if (!bsend(data, m_len))
	    {
	        if (log_level == common::levDebug)
	        {
	            msg.str("");
	            msg << pid_child << " Клиент закрыл соединение" << endl;
                    wmsg(msg, common::levDebug);
	        }
	        break;
	    }

            data_end -= m_len;
            msg_log.response_size  += m_len;
            if (!data_end) break;
	}

        msg_log.response_time = common::time_between(t1);

    //    bsend((char*)"\n\n", strlen("\n\n"));
        free(data);
	close(mf);

	return pid_status == 0;
}


/*----------------------------------------------*/
bool TParameters::create_answer1(parameters::sendParameters f)
{
        std::string file   = sparse();

        std::ostringstream msg;
        if (pid_status || file.empty())
        {
            char *data   = (char*)calloc(1024, sizeof(char));
            sprintf(data, BADREQUEST, mData.http);

            bsend(data, strlen(data)) ;
            msg  << pid_child << " "<< data << endl;
            wmsg(msg, common::levError);

            free(data);
            return false;
        }

        if (!common::FileExists((char*)file.c_str()))
        {
              char *data   = (char*)calloc(1024, sizeof(char));
              sprintf(data, NOTFOUND, mData.http);

              bsend(data, strlen(data));

              msg  << pid_child << " " << file << " "<< data << endl;
              wmsg(msg, common::levError);

              free(data);
              return false;
        }

        msg  << pid_child << " File to play: "<< file;
        wmsg(msg, common::levWarning);

        int mf = open(file.c_str(), O_RDONLY | O_NONBLOCK);
        if (mf == -1)
        {

            char *data   = (char*)calloc(1024, sizeof(char));
            sprintf(data, FORBIDDEN, mData.http);

            msg.str("");
            msg  << pid_child << " "<< file << " : " << strerror(errno) << std::endl;
            msg  << pid_child << " "<< data << endl;
            wmsg(msg, common::levError);

            pid_status |= STATUS_NOOPEN;

            bsend(data, strlen(data));

            free(data);
            return false;
        }

        struct stat st;
        fstat(mf, &st);
        int m_len = st.st_size;

        MRange::iterator it = mRange.begin();
        int data_begin = (*it).first;

        char *data   = (char*)calloc(BUFFER_SIZE+1, sizeof(char));

        /* Передача заголовка */
        sprintf(data, RESPONSE206, mData.http, common::str_time().c_str(), data_begin, m_len-1, m_len, m_len-data_begin, mData.is_aac ? AAC_TYPE : MPEG_TYPE );

        if (log_level == common::levDebug)
        {
            msg.str("");
            msg << pid_child << " Заголовок ответа: " << data << endl;
            wmsg(msg, common::levError);
        }

        if (!bsend(data, strlen(data)))
        {
            msg.str("");
            msg  << pid_child << " Не могу отправить заголовок клиенту" << endl;
            wmsg(msg, common::levError);

            free(data);
            close(mf);
            return false;
        }

        bsendfile(mf, data_begin, m_len);
        free(data);
        close(mf);
        return pid_status == 0;
}
/*----------------------------------------------*/
std::string TParameters::get_log_str()
{
    std::ostringstream str;

    if (!msg_log.username)      msg_log.username        = strdup((char*)MARKER_EMPTY);
    if (!msg_log.userpass)      msg_log.userpass        = strdup((char*)MARKER_EMPTY);;
    if (!msg_log.request_time)  msg_log.request_time    = strdup((char*)MARKER_EMPTY);;
    if (!msg_log.referer)       msg_log.referer         = strdup((char*)MARKER_EMPTY);;
    if (!msg_log.user_agent)    msg_log.user_agent      = strdup((char*)MARKER_EMPTY);;

    str << msg_log.ip << MARKER_TAB << msg_log.username << MARKER_TAB << msg_log.userpass << MARKER_TAB;
    str << '[' << msg_log.request_time << ']' << MARKER_TAB << msg_log.request << MARKER_TAB << msg_log.response << MARKER_TAB << msg_log.header_size + msg_log.response_size << MARKER_TAB;
    str << msg_log.referer << MARKER_TAB << msg_log.user_agent << MARKER_TAB << msg_log.response_time << MARKER_TAB;
    str << "( " << msg_log.header_size << MARKER_TAB << msg_log.response_size << MARKER_TAB << msg_log.file_size << MARKER_TAB << msg_log.range_start << MARKER_TAB << msg_log.range_stop << ")" << endl;

    return str.str();
}

bool TParameters::bsend(char *buf, int length)
{
  int total = 0, m_out = 0;
  while(total < length)
  {
                  m_out = send(clientfd, buf+total, length-total, MSG_NOSIGNAL);
                  if(m_out == -1)
                  {
                          if (errno == EAGAIN)
                          {
                              usleep(1000);
                              continue;
                          }

                          if (log_level == common::levDebug)
                          {
                              std::ostringstream msg;
                              msg << pid_child << " tclient_socket::send(const char *buf, int length) " << strerror(errno) << std::endl;
                              wmsg(msg, common::levWarning);
                          }
                          break;
                  }
                  total += m_out;
  }
  return m_out != -1;
}


bool TParameters::bsendfile(int mf, int m_begin, int m_length)
{
  off_t   m_off  = (off_t)m_begin;
  size_t  m_len  = (size_t)(m_length-m_begin);
  ssize_t total  = 0, m_out = 0;

  while (total < (ssize_t)m_len)
  {
      m_out = sendfile(clientfd, mf, &m_off, m_len - total);
      if(m_out == -1)
      {
              if (errno == EAGAIN) continue;
              std::ostringstream msg;
              msg << pid_child << " tclient_socket::send(const char *buf, int length) " << strerror(errno) << std::endl;
              wmsg(msg, common::levWarning);
              break;
      }
      m_off -= m_out;
      total += m_out;
  }
  return m_out != -1;
}

} /* namespace parameters */
