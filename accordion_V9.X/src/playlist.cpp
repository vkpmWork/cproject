/*
 * playlist.cpp
 *
 *  Created on: 02.02.2016
 *      Author: irina
 */
#include "playlist.h"
#include "aac.h"
#include "mpx.h"
#include "log.h"
#include <math.h>
#include "msg_error.h"
#include <wchar.h>
#include <iomanip>
#include "parameters.h"

using namespace std;
using namespace msg_error;

char message[] = "GET / HTTP/1.1\r\n\r\n";
char buf[sizeof(message)];

const size_t Buf_size   = 100000;
const int b_size 	= 2000;

TPlaylist   *pPlaylist  = NULL;
TPlaylist   *p_pl       = NULL;



/* Захватить сигнал и зарегистрировать факт его обработки */

TPlaylist::TPlaylist(tconfig *ptr_config) :
		 ptr_play_time(NULL), ptr_file_info(NULL), fmt_Free(NULL), stream_buffer(NULL), fs(NULL),  counter(0), ptr_client_socket(NULL)
{

        sigemptyset(&sset);
	p_pl = this;

	pConfig = ptr_config;

        ptr_event     = (tevent*) malloc(sizeof(tevent));
        ptr_event->ptr_flag     = (tflag*)malloc(sizeof(tflag));
        ptr_event->timeout      = 0;
        ptr_event->ptr_flag->fl = 0;

        ptr_file_info = new tfile_info;
        if (ptr_file_info) memset(ptr_file_info, 0, sizeof(tfile_info));

        ptr_play_time = (tplay_time*) malloc(sizeof(tplay_time));
        memset(ptr_play_time, 0, sizeof(tplay_time));


        config.contenttype = NULL;
	config.playlist    = strdup(ptr_config->get_playlist());

	config.query_playlist_interval = ptr_config->get_query_playlist_interval();

	config.stream_type = pConfig->aac_streamtype();
	config.log_level   = pConfig->get_loglevel();
	config.reconnection_time = pConfig->get_reconectgion_time();

	playlist_clear();

	bool flag = false;
	int v;
	config.error = load_playlist(flag, v);
	if (config.error)
	{
		cout << "Empty playlist " << config.playlist << endl;
		return;
	}

	if (config.stream_type)
	{
		fmt_Create = aac_create;
		fmt_Free = aac_free;
		fmt_GetFileInfo = aac_GetFileInfo;
		fmt_GetFrames = aac_GetFrames;
		fmt_start_play = aac_StartPlay;
		fmt_stop_play = aac_StopPlay;
		fmt_GetMetadata = aac_Metadata;
		config.contenttype = strdup("audio/aacp");
	}
	else
	{
		fmt_Create = mpx_create;
		fmt_Free = mpx_free;
		fmt_GetFileInfo = mpx_GetFileInfo;
		fmt_GetFrames = mpx_GetFrames;
		fmt_start_play = mpx_StartPlay;
		fmt_stop_play = mpx_StopPlay;
		fmt_GetMetadata = mpx_Metadata;
		config.contenttype = strdup("audio/mpeg");
	}

	create_log_header();
}

void TPlaylist::create_log_header()
{
     char buffer [1024];
     time_t rawtime;

     time (&rawtime);
     struct tm * timeinfo = localtime (&rawtime);

     string	headers	  = pConfig->get_header();

     logger::message.str("");
     logger::message.flags ( std::ios::left);

     strftime (buffer,122,"%c ", timeinfo);
     logger::message << endl << pConfig->get_impotant_info_marker() << " " << time(0) << " Started at "  << buffer;

     if (common::application_name)
     {
    	memset(buffer, 0, 100);
    	sprintf(buffer,common::version, common::application_name);
    	logger::message << buffer << endl;
     }

	const int w = 20;

	logger::message << setw(w) << "Pid" 				<< " : " << getpid()     << endl;
	logger::message << setw(w) << "Log      file name"  << " : " << logger::get_log_name() << endl;
	logger::message << setw(w) << "Config   file name"  << " : " << pConfig->get_config_file() << endl;
	logger::message << setw(w) << "Playlist file name"  << " : " << pConfig->get_playlist()    << endl;
	logger::message << setw(w) << "Mount point" 		<< " : " << pConfig->get_mount()       << endl;
	logger::message << setw(w) << "Stream name" 		<< " : " << pConfig->get_name()        << endl;
	logger::message << setw(w) << "Address"		        << " : " << pConfig->get_host() << endl;
	logger::message << setw(w) << "Port " 				<< " : " << pConfig->get_port() << endl;
	logger::message << setw(w) << "Loglevel" 			<< " : " << (pConfig->get_loglevel() == 0 ? "debug" : pConfig->get_loglevel() == 1 ? "warning" : "error") << endl << endl;

	logger::header_log(logger::message, (void*)logger::ptr_log);
	logger::header_log(logger::message, (void*)logger::titr_log);

	logger::message.unsetf ( std::ios::showbase );

}

TPlaylist::~TPlaylist()
{
	free(ptr_event->ptr_flag);
	free(ptr_event);

	if (ptr_client_socket)
	{
		delete ptr_client_socket;

		ptr_client_socket = NULL;
	}

	free(ptr_play_time);
	if (ptr_file_info)
	{
		delete ptr_file_info;
		ptr_file_info = NULL;
	}

	if (fmt_Free) fmt_Free();
	free(stream_buffer);
	free(config.playlist);
	free(config.contenttype);

	for (Lplaylist::iterator it = play_list.begin(); it != play_list.end(); it++) free(*it);

	if (logger::ptr_log->get_loglevel() == common::levDebug)
	{
	    logger::message.str("");
            logger::message << "Delete pPlayList\n";
            logger::write_log(logger::message);
	}

}

void TPlaylist::advance_iterator()
{
	Lplaylist::iterator it = play_list.begin();

	if (pConfig->get_npfile())
	{
		int idx = load_idx();

		if (idx && (++idx < (int) play_list.size()))
		{
			it = play_list.begin();
			std::advance(it, idx);
		}
	}

	it_playlist = it;
}

void TPlaylist::start()
{
	if (play_list.empty())
	{
		logger::message.str("");
		logger::message << ERROR_PLAYLIST_EMPTY << " " << time(0) << " Empty\n";
		logger::write_info(logger::message);
		return;
	}

	if (!fmt_Create(Buf_size)) 	return;

	ptr_client_socket = new tclient_socket(pConfig->get_host(), pConfig->get_port(), pConfig->get_connattempts(), SOCKET_BLOCK, config.reconnection_time);

	if (!ptr_client_socket)
	{
	    logger::message.str(get_error(config.error = ERR_SOCKET_CREATE));
	    logger::write_log(logger::message);
	    return;
	}

	if (!pParameters->get_playfile_exists())
		it_playlist = play_list.begin(); /* начинаем проигрывание сначала */
	else
		advance_iterator(); 		 /* продолжаем с точки завершения приложения */


	save_idx(distance_iterator());

        common::time_stop = common::mtime();
        logger::message.str("");
        logger::message << "Before connection (stop - start), mc = " << common::time_stop - common::time_start << endl;
        logger::write_log(logger::message);

	if (ptr_client_socket->net_connect())
	{
	        logger::message.str("");
	        logger::message << time(0) << " Connection set on " << pConfig->get_host() << ":" << pConfig->get_port() << std::endl;
	        logger::write_marker(logger::message);

		ptr_event->ptr_flag->bit.ev_header =  1;
		ptr_file_info->icecast_sample_rate = -1;
		ptr_client_socket->clear_connection_counter();
	}
	else
	{
		playlist_clear();
		logger::message.str("On start : ptr_client_socket->is_connected() == false\n ");
		logger::write_log(logger::message);
		return;
	}

	int buffer_size = pConfig->get_buffersize();
	stream_buffer = (char*) malloc(Buf_size);

	char 	*filename = NULL;
	int 	res		  = -1;
	fs 				  = NULL;


	bool 	is_metadata      = pConfig->is_update_meta_data();
	string	headers	  		 = pConfig->get_header();


        time_t tm;
        time(&tm);
	config.playlist_time = *localtime(&tm);

	int m_pl_size = pParameters->playlist_size();

	while (!play_list.empty())
	{
		if (common::up_time(config.playlist_time)) handler_proc::handler_timer(0);

		memset(ptr_play_time, 0, sizeof(tplay_time));

		if (!filename) free(filename);

		filename = strdup(get_current_file());
		if (!filename) break;

		ptr_play_time->time_begin = common::mtime();

		res = fmt_GetFileInfo(filename, ptr_file_info->bitrate,
			ptr_file_info->spf, ptr_file_info->simple_frequency,
			ptr_file_info->frames, ptr_file_info->channel_configuration,
			ptr_file_info->frames_to_read, ptr_file_info->play_time);

		if (res == -1 || fmt_start_play() == false)
		{
			it_playlist = erase_idx();

			logger::message.str("");
			logger::message << "File " << filename << " not found and erased from play_list\n";
			logger::write_log(logger::message);
			continue;
		}

		if (ptr_file_info->icecast_sample_rate == -1)
		  {
		        ptr_file_info->icecast_sample_rate = ptr_file_info->simple_frequency;

		        common::time_stop = common::mtime();
		        logger::message.str("");
		        logger::message << "After header sent (stop - start), mc = " << common::time_stop - common::time_start << endl;
		        logger::write_log(logger::message);

		  }
		else
			if (ptr_file_info->icecast_sample_rate != ptr_file_info->simple_frequency)
			{
					it_playlist = erase_idx();

					logger::message.str("");
					logger::message << ERR_SAMPLE_RATE << " " << time(0) <<  " " << filename << " : " << "Sample rate error" << endl;
					logger::write_info(logger::message);
					continue;
			}


		logger::message.str("");
                logger::message << time(0) << " Index: " << distance_iterator()+1 << " of " << m_pl_size << "; " << filename << std::endl;
                logger::write_marker(logger::message);
		res = 0;
		while (true)
		{

			if (!ptr_client_socket->is_connected())
			{
                            if (!ptr_event->ptr_flag->bit.ev_header) ptr_event->ptr_flag->bit.ev_header = 1;
				ptr_file_info->icecast_sample_rate = -1;
				ptr_client_socket->clear_connection_counter();
			}

			if (ptr_client_socket->net_connect())
			{
					if (ptr_event->ptr_flag->bit.ev_header)
					{
							char *v = (char*) malloc(headers.size() + 300);
							sprintf(v, headers.c_str(), (int) ptr_file_info->bitrate,
									ptr_file_info->channel_configuration,
									ptr_file_info->simple_frequency);

							if (config.log_level == common::levDebug)
							{
								logger::message.str(v);
								logger::write_log(logger::message);
							}

							bool fl_send = ptr_client_socket->net_send(v, strlen(v));
							usleep(10*1000);
							free(v);

							if (!fl_send) continue;

							if (ptr_client_socket->net_recv((char*) "200") == false)
							{
								logger::message.str("");
								logger::message << ERR_CONNECT_200 << " " << time(0) <<  " ptr_client_socket->net_recv(200) == false" <<  endl;
								logger::write_info(logger::message);

								playlist_clear();
								break;
							}

							ptr_event->ptr_flag->bit.ev_header = 0; // заголовок успешно передан!
					}


					if (res == 0)
					{
						if (ptr_client_socket->is_connected() && is_metadata) transmit_metadata(filename, ptr_file_info->play_time);
						it_playlist = get_next_iterator();
					}

					ptr_play_time->send_begin = common::mtime();

					res = fmt_GetFrames(stream_buffer);
					if (res)
					{
						if (ptr_client_socket->net_send(stream_buffer, res))
						      ptr_play_time->frames_sent += ptr_file_info->frames_to_read;
					}

					timing(res, buffer_size);

		        	if (!res) break;

			}//    	if (ptr_client_socket->net_connect())
			else break;

		} // while (true)

		fmt_stop_play();
		free(filename);
		filename = NULL;

		if (!ptr_client_socket->is_connected()) break;

		if (pParameters->get_new_playlist())
		{
		    bool flag = false;
		    load_playlist(flag, m_pl_size);
		}

	} //while (!play_list.empty())

}


void TPlaylist::timing(int res, int buffer_size)
{
	int time_between_tracks = 0;

	unsigned int m_time = common::mtime();
	logger::message.str("");

	if (res)
	{
		try
		{
			ptr_play_time->time_elapsed = m_time - ptr_play_time->time_begin;

			ptr_play_time->time_sent = round(
					(((float) ptr_play_time->frames_sent * ptr_file_info->spf)
							/ ptr_file_info->simple_frequency) * 1000.0);

			ptr_play_time->buffer_sent = ptr_play_time->time_elapsed > ptr_play_time->time_sent ? 0 : ptr_play_time->time_sent - ptr_play_time->time_elapsed;

			ptr_play_time->send_lag = m_time - ptr_play_time->send_begin;

			if (config.log_level == common::levDebug &&  ptr_play_time->time_elapsed > 1500)
			{

			/*	logger::message << "Frames : " << ptr_play_time->frames_sent << "/"
						<< ptr_file_info->frames << " Time : "
						<< round(ptr_play_time->time_elapsed / 1000.) << "/"
						<< round(ptr_play_time->time_sent / 1000.) << "s Buffer : "
						<< ptr_play_time->buffer_sent << "ms Bps : " << res << '\r';
			*/
			}

			if (ptr_play_time->buffer_sent < (buffer_size - 100))
					ptr_play_time->time_pause = 900 - ptr_play_time->send_lag;
			else if (ptr_play_time->buffer_sent > buffer_size)
						ptr_play_time->time_pause = 1100 - ptr_play_time->send_lag;
				else
						ptr_play_time->time_pause = 975 - ptr_play_time->send_lag;

			time_between_tracks = ptr_play_time->time_pause;
		}
		catch(...) {time_between_tracks = 500; }
	}
	else
	{
		try
		{
			logger::message << "Frames : " << ptr_file_info->frames << " Time : "
					<< round(ptr_play_time->time_elapsed / 1000.) << "/"
					<< round(ptr_play_time->time_sent / 1000.) << "s... Finished.  ";

			time_between_tracks = round(
					(((float) ptr_file_info->frames * ptr_file_info->spf)
							/ ptr_file_info->simple_frequency) * 1000)
					- (common::mtime() - ptr_play_time->time_begin);
		}
		catch(...)
		{
			time_between_tracks = 500;
		}

		logger::message << "Pause between tracks =  " << time_between_tracks << "ms...\n\n";
	}

	if (time_between_tracks > 3000)
	{
				time_between_tracks = 2000;
				logger::message << "		Changing: Pause between tracks  = 2000\n";
	}

	if (pConfig->get_loglevel() == common::levWarning) logger::write_log(logger::message);

	int sig 	  = -1;
	if (time_between_tracks > 0)
	{
		sigaddset(&sset, SIGALRM);
		sigprocmask(SIG_BLOCK, &sset, NULL);

		handler_proc::track_timeout((suseconds_t) (time_between_tracks));

		sigwait(&sset, &sig);

		sigdelset(&sset, SIGALRM);
		sigprocmask(SIG_BLOCK, &sset, NULL);
	}
}

char* TPlaylist::get_next_file() {
	bool is_modify = false;

	int aa;
	load_playlist(is_modify, aa);

	if (is_modify)
		it_playlist = play_list.end();

	char *str = NULL;

	it_playlist = get_next_iterator();
	if (it_playlist == play_list.end()) {
		cout << get_error(ERR_PLAYLIST_SZ);
		return str;
	}
	str = *it_playlist;

	save_idx(distance_iterator());
	return str;
}

char* TPlaylist::get_current_file()
{
	char *str = NULL;

	if (it_playlist == play_list.end())
	{
		cout << get_error(ERR_PLAYLIST_SZ);
		return str;
	}
	str = *it_playlist;
	save_idx(distance_iterator());

	return str;
}


Lplaylist::iterator TPlaylist::get_next_iterator()
{
	Lplaylist::iterator it = it_playlist;
	if (++it == play_list.end())
	{
		/* обновлене плейлиста!!! */
		handler_proc::query_playlist_detach(pConfig->get_url_get_playlist());
		it = play_list.begin();
	}
	return it;
}

void TPlaylist::save_idx(int idx)
{
	char *fl = pConfig->get_npfile();
	char *a = common::pathname(fl);

	common::MakeDirectory(a);
	free(a);


	FILE *f = fopen(pConfig->get_npfile(), "w+");
	if (f)
	{
		fprintf(f, "%d", idx);
		fclose(f);
	}

}

int TPlaylist::load_idx()
{
	int index = 0;
	if (common::FileExists(pConfig->get_npfile()))
	{
		FILE *f = fopen(pConfig->get_npfile(), "r+");
		char *s = (char*) calloc(255, sizeof(char));
		if (f)
		{
			if (fgets(s, 255, f)) sscanf(s, "%d", &index);
			fclose(f);
		}
		free(s);
	}
	return index;
}

Lplaylist::iterator TPlaylist::erase_idx()
{
	it_playlist = play_list.erase(it_playlist);

	if (it_playlist == play_list.end())
		it_playlist = play_list.begin();

	return it_playlist;
}

int TPlaylist::load_playlist(bool &is_modified, int &m_sz)
{
	playlist_clear();
	pParameters->update_playlist(play_list);
	config.error = play_list.size() == 0;
	it_playlist = play_list.begin();

	is_modified = true;
	m_sz        = play_list.size();
	return config.error;
}

void TPlaylist::playlist_clear()
{
	for (Lplaylist::iterator it = play_list.begin(); it != play_list.end(); it++) free(*it);
	play_list.clear();
}

void TPlaylist::transmit_metadata(char *m_file, int m_time)
{
	   pthread_t 	  a_thread;
	   pthread_attr_t thread_attr;

	   handler_proc::DATA *m_arg = (handler_proc::DATA *)calloc(1, sizeof(handler_proc::DATA));
	   m_arg->m_file 	 = strdup(m_file);
	   m_arg->play_time = m_time;

	   if (pthread_attr_init(&thread_attr) != 0)
	   {
		   logger::message.str("Attribute creation failed (in handler_timer: pthread_attr_init(thread_attr))\n");
		   logger::write_log(logger::message);
		   return;
	   }

	   if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED) != 0) // переводим в отсоединенный режим
       {
		   logger::message.str("Setting detached attribute failed (in handler_timer)\n");
		   logger::write_log(logger::message);
	       return;
	   }

	   if ( pthread_create(&a_thread, &thread_attr, handler_proc::handler_metadata_thread, m_arg) != 0)
	   {
		   logger::message.str("on_play_thread : Thread creation failed\n");
		   logger::write_log(logger::message);
	   }

	   (void)pthread_attr_destroy(&thread_attr);
}
