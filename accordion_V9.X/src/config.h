/*
 * config.h
 *
 *  Created on: 29.01.2016
 *      Author: irina
 */

#ifndef CONFIG_H_
#define CONFIG_H_
#include <string>
#include <map>
#include "common.h"

class tconfig
{
public:
	tconfig(char**, int);
	virtual ~tconfig();

	bool    get_cfg_error() { return (bool)m_cfg_error; }

	char*	get_config_file();
	char*   get_playlist();
	int 	get_playlist_type();
	char*	get_npfile();
	char*   get_logfile();
	char*   get_impotant_info_marker()              { return pConfig->impotant_info_marker; }
	char*   get_loghistory();
	char*   get_playfile();
	common::tcLogLevel get_loglevel();

	bool    aac_streamtype();
	bool    mpx_streamtype();
	char*   get_host();
	int     get_port();

	char*   get_mount();
	int     get_connattempts();
	char*	get_password();
	char*   get_server();
	char*   get_name();
	bool    get_ispublic()				{ return pConfig->is_public;			}
	char*   get_url()      				{ return pConfig->url; 				}
	char*   get_genre()	   			{ return pConfig->genre;     			}
	char*   get_descr()				{ return pConfig->descr;     			}
	int     get_buffersize()			{ return pConfig->buffersize; 			}
	bool    is_update_meta_data()      	        { return pConfig->update_meta_data;      	}
	bool    get_deamon()                            { return pConfig->is_deamon;			}
	int     get_query_playlist_interval() 	        { return pConfig->query_playlist_interval; 	}
	char	*get_url_sign_change_playlist();
	char	*get_url_get_playlist();
	char    *get_url_send_track_data();

	std::string get_header()                        { return m_header; 		                }
	std::string get_metadata_header(std::string);

	int	get_channel_id();
	char	*get_channel_type();
	char    get_channel_prefix();
	int     get_logsize()				{ return pConfig->logsize*1024;			}
	int     get_reconectgion_time()                 { return pConfig->reconnection_time; 	        }

	char    *get_jingle_path()                      { return pConfig->jingle_path;                  }


private:
	char* m_cfg_file;
	int   m_cfg_error;
	struct TConfig
	{
		char *streamtype;
		char *server;
		char *host;
		int   port;
		char *mount;
		int   connattempts;
		char *password;
		int	  buffersize;
		char *playlist;
		int   playlist_type;
		char *npfile;
		char *logfile;
		char *loghistory;
		char *playfile;
		char *scriptfile;
		common::tcLogLevel   loglevel;
		bool  play_ramdom;
		bool  update_meta_data;

		char *name;
		char *descr;
		char *url;
		char *genre;
		bool  is_public;
		bool  is_deamon;
		char *content_type;
		int   query_playlist_interval;
		char *url_sign_change_playlist;
		char *url_get_playlist;
		char *url_send_track_data;

		int   channel_id;
		char *channel_type;
		int   logsize;

		int   reconnection_time;
		char *jingle_path;

		char *impotant_info_marker;

	} *pConfig;

	std::string m_header;

	typedef std::map<char*, std::string> tconfig_param;
	tconfig_param	config_param;

	void ReadIni();
};

extern tconfig     *pConfig;

#endif /* CONFIG_H_ */
