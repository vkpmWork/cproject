/*
 * parameters.cpp
 *
 *  Created on: 12.07.2016
 *      Author: irina
 */

#include "parameters.h"
#include "log.h"
#include <linux/limits.h>
#include <fstream>
#include <iomanip>
#include <dirent.h>


Tparameters *pParameters = NULL;

/* ============================================================== */
std::string  get_metadata_begin(std::string str)
{
	if (str.empty()) return 0;

	size_t sz = str.find("finish");

	if (sz != std::string::npos) str = str.substr(sz, str.length());
	else str.clear();

	return str;
}

char*   get_endl(char* v)
{
	if (!v) return NULL;

	v = strchr(v,'\n');
	return  v ? v + 1 : v;
}
/* ============================================================== */

Tparameters::Tparameters(bool m_aac_streamtype, char* m_playlist_path, char *m_playfile, char *m_npfile, char *m_jingle)
	: aac_streamtype  (m_aac_streamtype)
	, start_play_track(false)
	, stop_play_track (false)
	, last_track	  (false)
	, new_playlist	  (false)
	, get_titrs	  (false)
	, playfile_exists (false)
        , playfile_default(false)
{
	playlist_path = strdup(m_playlist_path);
	playfile	  = strdup(m_playfile);
	np_file		  = strdup(m_npfile);
	pLoggerMutex  = new TLoggerMutex;
	it_start      = playlist.end();
	jingle_path   = strdup(m_jingle);

	check_playlist();
}

Tparameters::~Tparameters()
{
	playlist_clear();
	free(playfile);
	free(playlist_path);
	free(jingle_path);

	if (pLoggerMutex)
    {  delete pLoggerMutex;
       pLoggerMutex = NULL;
    }

}

void	Tparameters::set_start_play_track(bool value)
{
	pLoggerMutex->StartPlayTrack_MutexLock();
	start_play_track = value;
	pLoggerMutex->StartPlayTrack_MutexUnlock();
}

bool	Tparameters::get_start_play_track()
{
	bool value;

	pLoggerMutex->StartPlayTrack_MutexLock();
	value = start_play_track;
	pLoggerMutex->StartPlayTrack_MutexUnlock();

	return value;
}

void	Tparameters::set_stop_play_track(bool value)
{
	pLoggerMutex->StopPlayTrack_MutexLock();
	stop_play_track = value;
	pLoggerMutex->StopPlayTrack_MutexUnlock();
}

bool	Tparameters::get_stop_play_track()
{
	bool value;

	pLoggerMutex->StopPlayTrack_MutexLock();
	value = stop_play_track;
	pLoggerMutex->StopPlayTrack_MutexUnlock();

	return value;
}

void	Tparameters::set_change_playlist(char *m_playlist)
{
	Lplaylist pl;

	if (!m_playlist) return;

	int  m_received_counter = 0;

        char *ext  =  aac_streamtype ? (char*)"aac" : (char*)"mp3";

	char *data = (char*) calloc(PATH_MAX, sizeof(char));

	char *ptr_s = strdup(get_metadata_begin(m_playlist).c_str());
	char *s 	= ptr_s;

	if (s) s = get_endl(s);
	if (s)
	{	  //s = get_endl(s);

	  	  char *id   = (char*) calloc(100, sizeof(char));
	  	  char *path = (char*) calloc(100, sizeof(char));
	  	  int   idi  = 0;

	  	  while(*s)
	  	  {
	  		  if (sscanf(s, "%[^|]%*c%[^|]", id, path) > 0)
	  		  {
					  idi = atoi(id);

					  if (idi && strlen(path) && data)
					  {
							sprintf(data, "%s/%s/%s/%s.%s", playlist_path, path, common::GetSubPath(idi).c_str(), id, ext);

							if (common::FileStatExists(data)) pl.push_back(strdup(data));

							m_received_counter++;
					  }
	  		  }
	  		  s = get_endl(s);
	  		  if (!s) break;
	  	  }

	  	  free(data);
	  	  free(ptr_s);
	  	  free(path);
	  	  free(id);
	}

	std::ostringstream m;
	if (!pl.size())
	{
		new_playlist = false;

	        m << time(0) << " Playlist: received size: " << m_received_counter << "; real size: " << playlist.size() << endl;
	        logger::write_marker(m);

	        m.clear();
	        m << ERROR_PLAYLIST_EMPTY << " " << time(0) << " Нет треков для ротации канала (playlist is empty)" << endl;;
		logger::write_info(m);
		logger::write_log(m, common::levWarning);
		return;
	}


	pLoggerMutex->ChangePlaylist_MutexLock();

	char *a = common::pathname(playfile);

	common::MakeDirectory(a);
	free(a);

	std::ofstream outfile(playfile, std::ofstream::binary);

	playlist_clear();

	for (Lplaylist::iterator it = pl.begin(); it != pl.end(); it++)
	{
		playlist.push_back(*it);

		if (outfile)
		{
			outfile. write(*it, strlen(*it));
			outfile.put('\n');
		}
	}

	if (outfile) outfile.close();

        m.str("");
        m << time(0) << " Playlist: received size: " << m_received_counter << "; real size: " << playlist.size() << endl;
        logger::write_marker(m);

	new_playlist = true;
	if (playfile_exists) playfile_exists = false;

        pLoggerMutex->ChangePlayList_MutexUnlock();
}

void 	Tparameters::check_playlist()
{
	  /* ппроверим существование файла с плейлистом */

      char *str = (char*)calloc(256, sizeof(char));
      if (!str) return;

      std::ostringstream m;
      std::string        s;

      char *ext  =  aac_streamtype ? (char*)".aac" : (char*)".mp3";

	  std::filebuf fb;
	  if (fb.open (playfile,std::ios::in))
	  {
	    std::istream is(&fb);

	    while (is)
	    {
	    	is.getline(str, 256);
	    	if (str)
	    	{
	    		if (strstr(str, ext) && common::FileStatExists(str, s))
	    			playlist.push_back(strdup(str));
	    	}
	    }
	    fb.close();
	  }

	  if (!s.empty())
	  {
	        m.str("");
	        m << s;
	        logger::write_info(m);
	  }

	  playfile_default = playlist.empty();
	  if (playfile_default)
          {
	        DIR *dir = opendir(jingle_path);
	        if(dir)
	        {
	                 struct dirent *ent;
	                 while((ent = readdir(dir)) != NULL)
	                 {
	                      if (strstr(ent->d_name, ext))
	                      {
	                          memset(str, 0, 255);
	                          sprintf(str, "%s/%s", jingle_path, ent->d_name);
	                          playlist.push_back(strdup(str));
	                      }
	                  }
	        }
	    }

	  playfile_exists = !playlist.empty();
	  free(str);
}

void 	Tparameters::set_new_playlist(bool value)
{
	pLoggerMutex->Change_MutexLock();
	new_playlist = value;
	pLoggerMutex->Change_MutexUnlock();

}

bool 	Tparameters::get_new_playlist()
{
	bool value = true;

	pLoggerMutex->Change_MutexLock();
	value = new_playlist && playlist.size();
	pLoggerMutex->Change_MutexUnlock();

	return value;
}

void    Tparameters::set_start()
{
	set_start_play_track(true);
	set_stop_play_track (false);
}

void 	Tparameters::playlist_clear()
{
	for (Lplaylist::iterator it = playlist.begin(); it != playlist.end(); it++) free(*it);
	playlist.clear();
}

void    Tparameters::update_playlist(Lplaylist &pl)
{
	pLoggerMutex->Playlist_MutexLock();

//	int cnt = 0;
	for (Lplaylist::iterator it = playlist.begin(); it != playlist.end(); it++)
	{
		pl.push_back(strdup(*it));
	}
	set_new_playlist(false);
	pLoggerMutex->PlayList_MutexUnlock();
}

bool	Tparameters::get_playfile_exists()
{
	return playfile_exists;
}
