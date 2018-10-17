/*
 * parameters.h
 *
 *  Created on: 12.07.2016
 *      Author: irina
 */

#ifndef PARAMETERS_H_
#define PARAMETERS_H_

#include "common.h"
#include "loggermutex.h"
#include <list>

typedef std::list<char*> Lplaylist;

class Tparameters
{
public:
	explicit Tparameters(bool, char*, char*, char*, char*);
	virtual ~Tparameters();

	void	set_start_play_track(bool);
	bool	get_start_play_track();

	void	set_stop_play_track(bool);
	bool	get_stop_play_track();

	void	set_change_playlist(char*);

	void 	set_new_playlist(bool);
	bool 	get_new_playlist();

	void    set_start();
	void    update_playlist(Lplaylist &);

	bool	get_playfile_exists();

	bool    get_playfile_default()    { return playfile_default; }
        void    clear_playfile_default()  { playfile_default = false;}

        int     playlist_size()           { return playlist.size();  }

private :
	    TLoggerMutex *pLoggerMutex;

	    bool		aac_streamtype;
		char		*playlist_path;
		char            *playfile;
		char		*np_file;
		bool 		start_play_track;
		bool		stop_play_track;
		bool 		last_track;
		bool 		new_playlist;
		bool 		get_titrs;
		bool		playfile_exists;
		bool            playfile_default;
		char            *jingle_path;

		Lplaylist   	    playlist;
		Lplaylist::iterator it_start;

		void 		playlist_clear();
		void 		check_playlist();
};

extern Tparameters *pParameters;

#endif /* PARAMETERS_H_ */
