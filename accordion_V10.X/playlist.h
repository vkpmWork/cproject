/*
 * playlist.h
 *
 *  Created on: 02.02.2016
 *      Author: irina
 */

#ifndef PLAYLIST_H_
#define PLAYLIST_H_

#include "config.h"
#include "tclientsocket.h"
#include "common.h"
#include "handler_proc.h"

/* ---------------------------------- */
typedef 		 bool (*Tfmt_Create) 	  (size_t b_size);
typedef 		 void (*Tfmt_Free)	 	  ();
typedef 		 int  (*Tfmt_GetFileInfo) (char *filename, float &br, int &spf, int &sr, int &frames, int &ch, int &m_frames_to_read, int &m_play_time);
typedef unsigned int  (*Tfmt_GetFrames)   (char * &sbuf);
typedef 		 bool (*Tfmt_start_play)  ();
typedef 		 void (*Tfmt_stop_play)   ();
typedef 		 std::string (*Tfmt_GetMetadata) (char *filemame);

/* ---------------------------------- */

class TPlaylist
{
public:
	std::string np;

	TPlaylist(tconfig*);
	virtual ~TPlaylist();

    int     error()				{ return config.error; }
    void    start();
private :

    struct tplay_time
    {
    	unsigned  int time_begin;	/* время от начала передачи */
    	unsigned  int time_sent;    /* время на отправку одного фрейма  */
    	unsigned  int send_begin;
    	unsigned  int time_elapsed; /* время, затраченное на отправку */
    	unsigned  int send_lag;	    /* вычисление времени задержки */
    	unsigned int  time_pause;
		int frames_sent;
		int buffer_sent;
    } *ptr_play_time;

    struct tfile_info
    {
    		int	spf;
    		int	simple_frequency;
    		int 	frames;
    		float	bitrate;
    		int 	channel_configuration;
    		int     frames_to_read;
        	int     play_time;
    		int 	icecast_sample_rate;
    } *ptr_file_info;

    tconfig   				*pConfig;
    Tfmt_Create		  	  	fmt_Create;
	Tfmt_Free			fmt_Free;
	Tfmt_GetFileInfo		fmt_GetFileInfo;
	Tfmt_GetFrames			fmt_GetFrames;
	Tfmt_start_play			fmt_start_play;
	Tfmt_stop_play			fmt_stop_play;
	Tfmt_GetMetadata        fmt_GetMetadata;

	Lplaylist 				play_list;
	Lplaylist::iterator 	it_playlist;
//	handler_proc::tsocket_data socket_data;
	char				    *stream_buffer;
    FILE 					*fs;
	struct
	{
		char *			playlist;
		int			stream_type;
		int 			error;
		char        		*contenttype;
		int			query_playlist_interval;
		struct tm		playlist_time;
		common::tcLogLevel      log_level;
		int 				reconnection_time;
	} config;

	int counter;
	tclient_socket  *ptr_client_socket;

    union tflag
    {
    	struct t_flag
    	{
    		unsigned int ev_dispose : 1;
    		unsigned int ev_new     : 1;
    		unsigned int ev_header  : 1;
    		unsigned int ev_send	   : 1;
    		unsigned int ev_next	   : 1;
    	}bit;

    	unsigned int fl;
    };

    struct tevent
    {
    	tflag *ptr_flag;
    	int    timeout;
    } *ptr_event;

    sigset_t sset;

	void 			create_log_header();

    int				load_playlist(bool &, int & /*кол-во треков в плейлисте */);
    Lplaylist::iterator         erase_idx();
    Lplaylist::iterator         get_next_iterator();
	inline bool 	        modified();
	void			save_idx(int idx);
	inline int	  	load_idx();
//	void 			send_metadata(std::string);
	void 			advance_iterator();
	inline int 		distance_iterator() { return std::distance(play_list.begin(), it_playlist); }
	void		        playlist_clear();
	void			timing(int, int);
    char*		        get_next_file();
    char*		        get_current_file();
    void 			transmit_metadata(char*, int);

};

extern  TPlaylist   *pPlaylist;

#endif /* PLAYLIST_H_ */
