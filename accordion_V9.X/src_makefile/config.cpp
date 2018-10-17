/*
 * config.cpp
 *
 *  Created on: 29.01.2016
 *      Author: irina
 */

#include "config.h"
#include <cstdlib>
#include <string>
#include "INIReader.h"
#include "base64.h"
#include "signal.h"
#include "log.h"

const std::string SECTION_SERVER  ("server");
const std::string SECTION_STREAM  ("stream");
const std::string SECTION_CHANNEL ("channel");
const std::string SECTION_PERSONAL ("personal");
const std::string SECTION_PLAYLIST("playlist");
const std::string SECTION_MISC	  ("misc");

const std::string DEFAULT_SERVER  ("icecast");
const std::string DEFAULT_HOST	  ("127.0.0.1");
const int  		  DEFAULT_PORT	 	  =	8000;
const std::string DEFAULT_MOUNT	  ("");
const std::string DEFAULT_PASSWORD("11111");
const int   	  DEFAULT_CONNECT	 	  = 5;
const int	  DEFAULT_PLAYLIST_INTERVAL       = 7200;
const int         DEFAULT_LOGSIZE                 = 10;//= 5000;
const int	  DEFAULT_RECONNECT_TIME          = 5;

tconfig     *pConfig    = NULL;

tconfig::tconfig(char** argv, int argc)
		: m_cfg_file(NULL)
		, m_cfg_error(0)
		, pConfig(NULL)
{

	config_param[(char*)"host"]        = "";
	config_param[(char*)"port"]        = "";
	config_param[(char*)"mount"]       = "";
	config_param[(char*)"streamtype"]  = "";
	config_param[(char*)"channelid"]   = "";
	config_param[(char*)"channeltype"] = "";

	if (argc > 1)
	{
		char *s;
		char *c = NULL;
		string str;

		for (int i = 2; i < argc; i++)
		{
			for (tconfig_param::iterator it = config_param.begin(); it != config_param.end(); it++)
			{
				s = (*it).first;
				c = strstr(argv[i], s);

				if (c)
				{
					str.clear();
					c = strchr(c, '=');
					if (c)
					{
						str.append(c+1);
						config_param[s] = str;
					}
					break;
				}
			}

		}
	}

	m_cfg_file = strdup(argv[1]);
	if (m_cfg_file)
	{
		pConfig = new TConfig;
		if (pConfig)
		{
			memset(pConfig, 0, sizeof(TConfig));
			ReadIni();
		}
		else m_cfg_error = 1;
	}
	else m_cfg_error = 1;
}

tconfig::~tconfig()
{
	if (pConfig)
	{
		free(pConfig->content_type);
		free(pConfig->streamtype);
		free(pConfig->server);
		free(pConfig->host );
		free(pConfig->mount);
		free(pConfig->password);
		free(pConfig->playlist);
		free(pConfig->npfile);
		free(pConfig->logfile);
		free(pConfig->loghistory);
		free(pConfig->playfile);
		free(pConfig->scriptfile);
		free(pConfig->name);
		free(pConfig->descr);
		free(pConfig->url);
		free(pConfig->genre);
		free(pConfig->url_sign_change_playlist);
		free(pConfig->url_get_playlist);
		free(pConfig->url_send_track_data);
		free(pConfig->jingle_path);

		delete pConfig;
		pConfig = NULL;

	}

	free(m_cfg_file);
}


void tconfig::ReadIni()
{
	std::string s;

	tconfig_param::iterator it;

	INIReader reader(m_cfg_file);

	if (reader.ParseError())
	{
		m_cfg_error = 1;

		std::cout << "Something Wrong with Configuration File\n";
    	return;
	}

	/* CHANNEL */
	it = config_param.find((char*)"channelid");
	if (it == config_param.end() || (*it).second.empty() == true) pConfig->channel_id = reader.GetInteger(SECTION_CHANNEL, "channelid" , 0);
	else pConfig->channel_id = atoi((*it).second.c_str());
	if (!pConfig->channel_id)
	{
		m_cfg_error = 1;

		std::cout << "Configuration file :  [channel] channelid = 0\n";
    	return;
	}

	it = config_param.find((char*)"channeltype");
	if (it == config_param.end() || (*it).second.empty() == true)
			s = reader.Get(SECTION_CHANNEL, "channeltype",	"");
	else s = (*it).second;

	if ((s.empty() == true) || (s != "channel" && s != "personal"))
	{	pConfig->channel_type	= NULL;
		m_cfg_error = 1;

		std::cout << "Configuration file :  [channel] type = empty\n";
		return;
	}
	else pConfig->channel_type	= strdup(s.c_str());

	/* SERVER */
	pConfig->server 		= strdup(reader.Get(SECTION_SERVER, "server", 				DEFAULT_SERVER).c_str());

	it = config_param.find((char*)"host");
	if (it == config_param.end() || (*it).second.empty() == true) pConfig->host = strdup(reader.Get(SECTION_SERVER, "host", DEFAULT_HOST).  c_str());
	else pConfig->host = strdup((*it).second.c_str());

	it = config_param.find((char*)"port");
	if (it == config_param.end() || (*it).second.empty() == true) pConfig->port = reader.GetInteger(SECTION_SERVER, "port", DEFAULT_PORT);
	else pConfig->port = atoi((*it).second.c_str());

	it = config_param.find((char*)"mount");
	if (it == config_param.end() || (*it).second.empty() == true) pConfig->mount = strdup(reader.Get(SECTION_SERVER, "mount", DEFAULT_MOUNT).	c_str());
	else pConfig->mount     = strdup((*it).second.c_str());

	if (pConfig->mount == NULL || strlen(pConfig->mount) == 0)
	{
		char *b = (char*)calloc(50, sizeof(char));

		sprintf(b, "%c%i", strcmp(pConfig->channel_type, "channel") == 0 ? 'a':'p', pConfig->channel_id);
		pConfig->mount = strdup(b);
		free(b);
	}

	pConfig->password	  	= strdup(reader.Get(SECTION_SERVER, "password",  			DEFAULT_PASSWORD).c_str());
	pConfig->connattempts 	= reader.GetInteger(SECTION_SERVER, "connectionattempts",   DEFAULT_CONNECT);
	pConfig->reconnection_time = reader.GetInteger(SECTION_SERVER, "reconnectiontime",  DEFAULT_RECONNECT_TIME);

	/* STREAM */
	it = config_param.find((char*)"streamtype");
	if (it == config_param.end() || (*it).second.empty() == true) pConfig->streamtype = strdup(reader.Get(SECTION_STREAM, "streamtype",  			"aac").c_str());
	else pConfig->streamtype = strdup((*it).second.c_str());

#ifdef _DEBUG
	s.append("_");
	s.append(pConfig->mount);
	pConfig->name			= strdup(s.c_str());
#else
	pConfig->name			= strdup(reader.Get(SECTION_STREAM, "name",  				"lost in wilderness accordion").c_str());
#endif
	pConfig->descr			= strdup(reader.Get(SECTION_STREAM, "description",  		" ").c_str());
	pConfig->url			= strdup(reader.Get(SECTION_STREAM, "url",  				"www.101.ru").c_str());
	pConfig->genre			= strdup(reader.Get(SECTION_STREAM, "genre",  				" ").c_str());
	pConfig->is_public		= reader.GetBoolean(SECTION_STREAM, "public",   			false);

	/* PLAYLIST */
	char *str 				= strdup(reader.Get(SECTION_PLAYLIST, "playlisttype", 		"internal").c_str());
	pConfig->playlist_type  = strcmp("lua", str) != 0 ? common::TYPE_INTERNAL : common::TYPE_LUA;
	pConfig->content_type   = strcmp("aac", pConfig->streamtype) == 0 ? strdup("audio/aacp") : strdup("audio/mpeg");
	free(str);

	pConfig->playlist		= strdup(reader.Get(SECTION_PLAYLIST, "playlist",  			"./playlist.txt").c_str());
	pConfig->play_ramdom	= reader.GetBoolean(SECTION_PLAYLIST, "playrandom",   		false);
	pConfig->query_playlist_interval = reader.GetInteger(SECTION_PLAYLIST, "query_playlist_interval" , DEFAULT_PLAYLIST_INTERVAL);

	s = reader.Get(SECTION_PLAYLIST, "url_get_sign_change_playlist", "");
	if (!s.empty())
	{
		char *b = (char*)calloc(256, sizeof(char));
		if (b)
		{
				sprintf(b, "/%i/%s", pConfig->channel_id,  pConfig->channel_type);
				s.append(b);

				pConfig->url_sign_change_playlist= strdup(s.c_str());
		}
		free(b);
	}
	else pConfig->url_sign_change_playlist = NULL;

	s = reader.Get(SECTION_PLAYLIST, "url_get_playlist", "");
	if (!s.empty())
	{
		char *b = (char*)calloc(256, sizeof(char));
		if (b)
		{

			sprintf(b, "/%i/%s/make", pConfig->channel_id,  pConfig->channel_type);
			s.append(b);

			pConfig->url_get_playlist= strdup(s.c_str());
		}
		free(b);
	}
	else pConfig->url_get_playlist = NULL;

	s = reader.Get(SECTION_PLAYLIST, "url_send_track_data", "");
	if (!s.empty())
	{
		char *b = (char*)calloc(256, sizeof(char));
		if (b)
		{
				sprintf(b, "/%i/%s", pConfig->channel_id,  pConfig->channel_type);
				s.append(b);

				pConfig->url_send_track_data= strdup(s.c_str());
		}
		free(b);
	}
	else pConfig->url_send_track_data = NULL;

	if (pConfig->url_sign_change_playlist == NULL || pConfig->url_get_playlist == NULL || pConfig->url_send_track_data == NULL)
	{
		if (!pConfig->url_sign_change_playlist) std::cout << "Url_get_sign_change_playlist not found in ini-file\n";
		if (!pConfig->url_get_playlist) 		std::cout << "Url_get_playlist not found in ini-file\n";
		if (!pConfig->url_send_track_data) 		std::cout << "Url_send_track_data not found in ini-file\n";

		m_cfg_error = 1;
		return;
	}

	pConfig->jingle_path            = strdup(reader.Get(SECTION_PLAYLIST, "jingle_path", "").c_str());


	/* MISK */
	pConfig->buffersize		= reader.GetInteger(SECTION_MISC, 	"buffersize",   		3)*1000;
	pConfig->update_meta_data=reader.GetBoolean(SECTION_MISC,   "updatemetadata",   		false);
	pConfig->scriptfile		= strdup(reader.Get(SECTION_MISC,   "playlisttype", 		"script.lua").c_str());

	s = reader.Get(SECTION_MISC,   "npfile", 				"/tmp/np/");
	s.append(common::GetSubPath(pConfig->channel_id));
	s.append("/");
	s.append(pConfig->mount);
	s.append(".txt");
	pConfig->npfile			= strdup(s.c_str());

#ifdef _DEBUG
	s = reader.Get(SECTION_MISC,   "logfile", "./log");
#else
	s = reader.Get(SECTION_MISC,   "logfile", "/var/log/accord");
#endif
	pConfig->logfile		= strdup(s.c_str());

#ifdef _DEBUG
	s = reader.Get(SECTION_MISC,   "loghistory", "./history/");
#else
	s = reader.Get(SECTION_MISC,   "loghistory", "/var/log/meta/");
#endif
	pConfig->loghistory		= strdup(s.c_str());

	pConfig->logsize                = reader.GetInteger(SECTION_MISC, "logsize",  DEFAULT_LOGSIZE);

	s = reader.Get(SECTION_MISC,   "playfile", "/tmp/playlist/");
	s.append(common::GetSubPath(pConfig->channel_id));
	s.append("/");
	s.append(pConfig->mount);
	s.append(".txt");
	pConfig->playfile		= strdup(s.c_str());

	pConfig->impotant_info_marker   = strdup(reader.Get(SECTION_MISC, "impotant_info_marker", (char*)"$").c_str());

	pConfig->loglevel		= (common::tcLogLevel)reader.GetInteger(SECTION_MISC, 	"loglevel",   		common::levError);
	pConfig->is_deamon              = reader.GetBoolean(SECTION_MISC,   "deamon",   		false);

	m_header.clear();
	m_header.append("SOURCE /");
	m_header.append(pConfig->mount);
	m_header.append(" HTTP/1.0\n");

	m_header.append("Content-Type: ");
	m_header.append(pConfig->content_type);
	m_header.append("\n");

	m_header.append("Authorization: Basic ");

	s = "source:";
	s.append(pConfig->password);
	int len = s.length();

	unsigned char *b = (unsigned char *) malloc(len+1);
	memcpy(b, s.c_str(), len);

	m_header.append(base64_encode(b, s.length()));
	m_header.append("\n");
	free(b);

	m_header.append("User-Agent: http_client/Version 1.0\n");

	m_header.append("ice-name: ");
	m_header.append(pConfig->name);
	m_header.append("\n");

	m_header.append("ice-public: ");
	m_header.append(pConfig->is_public == true ? "1" : "0" );
	m_header.append("\n");

	m_header.append("ice-url: ");
	m_header.append(pConfig->url);
	m_header.append("\n");

	m_header.append("ice-genre:");
	m_header.append(pConfig->genre);
	m_header.append("\n");

	m_header.append("ice-description:");
	m_header.append(pConfig->descr);
	m_header.append("\n");

	m_header.append("ice-audio-info: ice-bitrate=%i;ice-channels=%i;ice-samplerate=%i\n\n");
}

std::string tconfig::get_metadata_header(std::string s)
{
	std::string str = "GET /admin/metadata?mode=updinfo&mount=/";
	str.append(pConfig->mount);
	str.append("&song=");
//	std::string str = "GET /admin/metadata?mount=";
//	str.append(pConfig->mount);
//	str.append("&mode=updinfo&song=");
	str.append(common::urlEncode(s));
	str.append(" HTTP/1.0\n");
	str.append("User-Agent: accordion/Version 5.0\n");
//	str.append(common::version);
	str.append("Authorization: Basic ");

	std::string ss = "source:";
	ss.append(pConfig->password);
	int len = ss.length();

	unsigned char *b = (unsigned char *) malloc(len+1);
	memcpy(b, ss.c_str(), len);

	str.append(base64_encode(b, ss.length()));
	str.append("\n");
	str.append("\n");
	free(b);

	return str;
}

char*	tconfig::get_config_file()
{
	return this->m_cfg_file;
}

char*   tconfig::get_playlist()
{
	return pConfig->playlist;
}

int 	tconfig::get_playlist_type()
{
	return pConfig->playlist_type;
}

char*	tconfig::get_npfile()
{
	return pConfig->npfile;
}

bool	tconfig::aac_streamtype()
{
	return   strcmp(pConfig->streamtype, "aac") == 0;
}

bool	tconfig::mpx_streamtype()
{
	return strcmp(pConfig->streamtype, "mpeg") == 0;
}

char*   tconfig::get_logfile()
{
	return pConfig->logfile;
}

char*   tconfig::get_loghistory()
{
	return pConfig->loghistory;
}

char*   tconfig::get_playfile()
{
	return pConfig->playfile;
}

common::tcLogLevel tconfig::get_loglevel()
{
	if (pConfig->loglevel > common::levError) pConfig->loglevel = common::levError;
	return pConfig->loglevel;
}

char*   tconfig::get_host()
{
	return pConfig->host;
}
int     tconfig::get_port()
{
	return pConfig->port;
}
int     tconfig::get_connattempts()
{
	return pConfig->connattempts;
}
char*	tconfig::get_password()
{
	return pConfig->password;
}

char*	tconfig::get_mount()
{
	return pConfig->mount;
}

char*	tconfig::get_server()
{
	return pConfig->server;
}

char*   tconfig::get_name()
{
	return pConfig->name;
}
char* tconfig::get_url_sign_change_playlist()
{
	return pConfig->url_sign_change_playlist;
}
char* tconfig::get_url_get_playlist()
{
	return pConfig->url_get_playlist;
}
char* tconfig::get_url_send_track_data()
{
	return pConfig->url_send_track_data;
}
int	  tconfig::get_channel_id()
{
	return pConfig->channel_id;
}

char* tconfig::get_channel_type()
{
	return pConfig->channel_type;
}

char  tconfig::get_channel_prefix()
{
	if (strcmp(pConfig->channel_type, "channel") == 0) /* профессиональный канал */
		return  strcmp(pConfig->streamtype, "aac") == 0 ? 'a' : 'm';

	if (strcmp(pConfig->channel_type, "personal") == 0) /* персональный канал */
		return  strcmp(pConfig->streamtype, "aac") == 0 ? 'p' : 'z';
}


