/*
 * handler_proc.cpp
 *
 *  Created on: 06.07.2016
 *      Author: irina
 */

#include "handler_proc.h"

#include "url.h"
#include "config.h"
#include "common.h"
#include <assert.h>


//#ifdef _DEBUG
#include <fstream>
#include <stdio.h>
#include <time.h>
#include "base64.h"

//#endif

namespace handler_proc
{

/* Prototypes */
void handler_timer(int);
void  handler_get_playlist(char*);
void *handler_playlist_thread(void *arg);
void *handler_playlist_sign(void*);
bool  flag_timer;
/* Prototypes */


void handler_timer(int signo)
{

	   pthread_t 	  a_thread;
	   pthread_attr_t thread_attr;

	   std::ostringstream m;
	   if (pthread_attr_init(&thread_attr) != 0)
	   {
		   m.str("Attribute creation failed (in handler_timer: pthread_attr_init(thread_attr))\n");
		   logger::write_log(m);
		   return;
	   }

	    // переводим в отсоединенный режим
	   if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) != 0)
	    {
	    	m.str("Setting detached attribute failed (in handler_timer)\n");
       	        logger::write_log(m);
	    	return;
	    }

	    if ( pthread_create(&a_thread, &thread_attr, handler_playlist_sign, NULL) != 0) std::cout << "In handler_timer : Thread creation failed\n";

	    (void)pthread_attr_destroy(&thread_attr);
	    flag_timer = true;
}

void   track_timeout(int value)
{
	struct itimerval tval;

	timerclear(&tval.it_interval); /* нулевой интервал означает не сбрасывать таймер */
	timerclear(&tval.it_value);

	div_t d = div(value, 1000);


	tval.it_value.tv_sec     = d.quot; /* тайм-аут до срабтывания, секунд */
	tval.it_value.tv_usec    = d.rem*1000; /* тайм-аут до срабтывания, микросекунды */

	tval.it_interval.tv_sec  = 0; /* интервал срабатывания,  секунд */

    (void)setitimer(ITIMER_REAL, &tval, NULL);
    (void)signal(SIGALRM, NULL);

}

void *handler_playlist_thread(void *arg)
{
     std::ostringstream m;
     m << time(0) << " Playlist Request" << endl;
     logger::write_marker(m);

     int cnt = 0;
     std::string m_playlist;
     while(++cnt < 3)
     {
           m_playlist = url::query_url((char*)arg);

           if (m_playlist.size()) break;
           sleep(1);
     }

     if (m_playlist.size())
	 {

#ifdef _DEBUG
    	 std::ofstream outfile("new_pl.txt", std::ofstream::binary);
    	 outfile.write((char*)arg, strlen((char*)arg));

    	 outfile.write(m_playlist.c_str(), m_playlist.size());
	 	 outfile.close();
#endif

	 	 pParameters->set_change_playlist((char*)m_playlist.c_str());
	 }
     else
     {
 		m.str("");
                m << ERROR_PLAYLIST_QUERY << " " << time(0) << " Playlist didn't receive. Request " << (char*)arg << endl;
 		logger::write_info(m);
 		logger::write_log(m);
     }

    return NULL;
}

void  handler_get_playlist(char* message)
{
#ifdef _DEBUG
      std::cout << "Handler_get_playlist\n";
#endif

	   pthread_t 	  a_thread;
	   if ( pthread_create(&a_thread, NULL, handler_playlist_thread, (void *)message) != 0) std::cout << "In handler_timer : Thread creation failed\n";

       pthread_join(a_thread, NULL);
#ifdef _DEBUG
       std::cout << "Handler_get_playlist exit\n";
#endif
}

std::size_t get_content_pos(std::string m_metadata)
{
    std::ostringstream m;

    std::size_t found = m_metadata.find("Content-Length");
    if (found == std::string::npos)
    {
    	m << "Metadata error. Content-Length not found : " << m_metadata << std::endl;
    	logger::write_log(m);
 	logger::write_info(m);
      	return found;

    }

    found = m_metadata.find_first_of (":", found+1);
    if (found == std::string::npos)
    {
    	m << "Metadata error. <:> after Content-Length not found : " << m_metadata << std::endl;
    	logger::write_log(m);
 	logger::write_info(m);
      	return found;

    }

    found = atoi(&m_metadata[found+1]);
    if (!found)
    {
    	m << "Metadata error. Content-Length isn't valid : " << m_metadata << std::endl;
    	logger::write_log(m);
 	logger::write_info(m);
        found = std::string::npos;
    }
    return found;
}
/*
long mtime()
{
  struct timespec t;

  clock_gettime(0, &t);
  long mt = (long)t.tv_sec * 1000 + t.tv_nsec / 1000000;
  return mt;
}
*/
void *handler_metadata_thread(void *arg)
{
#ifdef LOCAL
	return NULL;
#endif
	//long t = mtime();

    std::ostringstream  me;
    if (logger::ptr_log->get_loglevel() == common::levDebug)
    {
		me << "Let's get metadata!\n";
		logger::write_log(me);
    }

	DATA *a = (DATA*)arg;

	int pos0 = strlen(pConfig->get_playlist());
	char *a0 = a->m_file + pos0;
	if (!a0) return NULL;

	char *f = basename(a->m_file) - 1;
    if (!f) return NULL;

	int track_id = atoi(f+1);
	if (track_id == 0) return NULL;

	char *c  = NULL;
	char *aC = a->m_file;

	while (*aC)
	{
 		c = strchr(aC, '/');

		if (c && (c != f))
		   aC = c+1;
		else
		   break;
	}


   pos0 = aC - a0;

   if (pos0 <= 0) return NULL;

   char* ss = (char*)calloc(pos0+1, sizeof(char));
   memcpy(ss, a0, aC - a0);

   char *s = (char*)calloc(1000, sizeof(char));

   sprintf(s, "%s/%i%s%i/", pConfig->get_url_send_track_data(), track_id, ss, a->play_time);

   std::string m_metadata = url::query_url(s);

   if (logger::ptr_log->get_loglevel() == common::levDebug)
   {
	   me.str().clear();
	   me << "Url Query metadata " << s << std::endl << m_metadata << std::endl;
	   logger::write_log(me);
   }

   /* сообщение для отчета РАО */
   memset(s, 0, sizeof(1000));
   string sss;
   sss.append(ss+1, strlen(ss) - 2);
   sprintf(s, "%u\t%s\t%i\t%i\t%i\n", (unsigned int)time(0), sss.c_str(), track_id, a->play_time, pConfig->get_channel_id());
   logger::message.str(s);
   logger::write_titrs(logger::message);

   free(s);
   free(ss);

   free(a->m_file);
   free(a);

   int md_size = m_metadata.size();
   if (!md_size) return NULL;

   std::ostringstream m;
//	std::size_t found = m_metadata.find("Content-Length");

    std::size_t found_200OK  = m_metadata.find("200");
    std::size_t found = m_metadata.find("{");

    if (found_200OK == std::string::npos || found == std::string::npos)
    {
    		if (found_200OK == std::string::npos)
    		{
    			m << ERR_METADATA << " " << time(0) << " Metadata error  : " << m_metadata << std::endl;
    			logger::write_info(m);
    		}
        	return NULL;
    }

/*    found = m_metadata.find_first_of (":", found+1);
    if (found == std::string::npos)
      {
    		logger::message << "Metadata error. <:> after Content-Length not found : " << m_metadata << std::endl;
    		logger::write_log(logger::message);
        	return NULL;

    }

    found = atoi(&m_metadata[found+1]);
    if (!found)
    {
    		logger::message << "Metadata error. Content-Length isn't valid : " << m_metadata << std::endl;
    		logger::write_log(logger::message);
        	return NULL;

    }
*/
      m_metadata = m_metadata.substr(found, m_metadata.length());

      if (m_metadata.length())
      {
    	  m_metadata = pConfig->get_metadata_header(m_metadata);
    	  url::transmit(pConfig->get_host(), pConfig->get_port(), pConfig->get_connattempts(), (char*)m_metadata.c_str(), m_metadata.length());
      }
      else
      {
  		m << "Metadata not found" << std::endl;
  		logger::write_log(m);
      }

     if (pConfig->get_loglevel() == common::levDebug)
  	 {
  		m.str("Exit Query Metadata\n");
  		logger::write_log(m);
  	 }
/*
      t = mtime() - t;

      logger::message.str("");
      logger::message << "Send Metadata : " << t <<  " milliseconds.\n";
      logger::write_log(logger::message);
*/
      return NULL;

}

void *handler_playlist_sign(void* aa)
{

#ifdef _DEBUG
	std::cout << "New Thread handler_playlist_sign\n";
#endif
	std::string m_metadata = url::query_url(pConfig->get_url_sign_change_playlist());

	std::size_t found 	   = get_content_pos(m_metadata);
	if (found == std::string::npos) return NULL;

	if (m_metadata.find("true", m_metadata.length() - found) != std::string::npos)
        {
		handler_playlist_thread(pConfig->get_url_get_playlist());
        }
        return NULL;
}

void query_playlist_detach(char *arg)
{
	   pthread_t 	  a_thread;
	   pthread_attr_t thread_attr;

	   std::ostringstream m;
	   if (pthread_attr_init(&thread_attr) != 0)
	   {
		   m.str("Attribute creation failed (in query_playlist_detach: pthread_attr_init(thread_attr))\n");
		   logger::write_log(m);
		   return;
	   }

	    // переводим в отсоединенный режим
	   if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) != 0)
	    {
	    	m.str("Setting detached attribute failed (in query_playlist_detach)\n");
    	        logger::write_log(m);
	    	return;
	    }

	    if ( pthread_create(&a_thread, &thread_attr, handler_playlist_thread, (void*)arg) != 0) std::cout << "In  : Thread creation failed\n";

	    (void)pthread_attr_destroy(&thread_attr);

}


} /* namespace common */




