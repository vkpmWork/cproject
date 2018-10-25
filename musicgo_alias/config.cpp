/*
 * config.cpp
 *
 *  Created on: 29.01.2016
 *      Author: irina
 */

#include "config.h"
#include "INIReader.h"
#include "base64.h"
#include <string.h>

#define SECTION_SERVER	"server"
#define SECTION_PROXY	"proxy"
#define SECTION_TRACK   "track"
#define SECTION_COMMON	"common"
#define SECTION_PATH    "PATH_MEDIA"

#define DEFAULT_HOST	"127.0.0.1:4545"
#define DEF_PATH_MEDIA  "/ssd/"

const std::string PROXY_REJIME    = "proxy";
const std::string SSD_REJIME      = "ssd";
const int  	  DEFAULT_PORT	  = 80;
const int         DEFAULT_LOGSIZE = 10;
const int         DEF_TRANSMIT_TO = 10;


std::string parse_host(char *p, int &m_port)
{
  std::string s;
  int i  = 0;
  m_port = 0;

  i = strcspn(p, ":");
  if (!i) return s;

  s.append(p, i);

  m_port = atoi(p+i+1);
  return s;
}

/* -------------------------------------------------------- */
tconfig *pConfig = NULL;
tconfig::tconfig(int argc, char**argv)
		: cfg_file(NULL)
		, cfg_error(0)
                //, rejime(tpLoader)
                //, path_media(NULL)
{

	cfg_file = strdup(argv[1]);
	std::ostringstream s;
	s << endl << common::str_time() << " " << argv[0] << endl;
	m_header.append(s.str());
	ReadIni();
}

tconfig::~tconfig()
{
	free(proxy_host );
	free(host);
	free(logfile);
	free(cfg_file);
	//free(path_media);
}


void tconfig::ReadIni()
{
	INIReader reader(cfg_file);

	int v = reader.ParseError();
	if (v)
	{
		cfg_error = 1;
		std::cout << "Something Wrong with Configuration File. Error: " << v << endl;
    	return;
	}

	/* SERVER */
	std::string m_host = reader.Get(SECTION_SERVER, "host", DEFAULT_HOST);
	host = strdup(parse_host((char*)m_host.c_str(), port).c_str());
	if (host == NULL || port == 0)
	{
	    cfg_error = 1;
	    return;
	}
        transmit_timeout = reader.GetInteger(SECTION_SERVER, "transmit_timeout", DEF_TRANSMIT_TO);

	/* PROXY, если существует
        m_host  = reader.Get(SECTION_PROXY, "host", "").  c_str();
        if (make_host((char*)m_host.c_str()))
        {
              rejime = tpProxy;
              proxy_port  = reader.GetInteger(SECTION_PROXY, "port", DEFAULT_PORT);
        }
        */

        /* TRACK */
        start_position  = reader.GetInteger(SECTION_TRACK, "start_position", 10    /* seconds from beginning */);
        duration        = reader.GetInteger(SECTION_TRACK, "duration",       40    /* seconds play duration  */);
        token_lifetime  = reader.GetInteger(SECTION_TRACK, "token_lifetime", 180   /* token lifetime in seconds */);

	/* COMMON */
	logfile  = strdup(reader.Get(SECTION_COMMON, "logfile", "./log/log.txt").  c_str());
	logsize	 = reader.GetInteger(SECTION_COMMON, "logsize", DEFAULT_LOGSIZE);
	loglevel = (common::tcLogLevel)reader.GetInteger(SECTION_COMMON, "loglevel", common::levError);
//	path_media = strdup(reader.Get(SECTION_COMMON, "path_media", DEF_PATH_MEDIA).  c_str());

	is_daemon   = reader.GetBoolean(SECTION_COMMON, "daemon", false);

	/* aliases path */
	m_pathmedia = reader.GetSection(SECTION_PATH);
	if (!m_pathmedia.size())
	  {
	    std::cout << "В конфигурационном файле отсутствует настройка путей до аудиотреков!" << endl;
	    cfg_error = 1;
	  }

	ostringstream str;
	str << "INI FILE   : " << cfg_file << endl;
	str << "SERVER     : host = " << host << "  port = " << port << endl;
//        str << "REJIME     : " << ( isLoader() ?  SSD_REJIME : PROXY_REJIME ) << endl;
        str << "ALIASES    :"  << endl;
        for (tpath_media::iterator it = m_pathmedia.begin(); it != m_pathmedia.end(); it++)
          {
            str << (*it).first << "="  << (*it).second << endl;
          }

        if (isProxy())
          str << "PROXY    : proxy host =" << host << "  proxy port =" << port << endl;

	str << "LOG FILE   : name=" << logfile << "  logsize=" << logsize << "  loglevel=" << loglevel << endl;
        str << "DAEMON     : " <<  (is_daemon ? "Yes" : "No") << endl;
        str << "PID file   : " <<  common::pidfile_name << endl;
        str << "PID        : " <<  getpid() << endl << endl;

	m_header.append(str.str());
	cout << m_header;
}

common::tcLogLevel tconfig::get_loglevel()
{
	if (loglevel > common::levError) loglevel = common::levError;
	return loglevel;
}

bool tconfig::make_host(char *m_str)
{
  if (!m_str) return false;

  std::string s;
  int   m_port = 0;

  char *p = strtok(m_str, "\n");
  while(p)
  {

      s = parse_host(p, m_port);

      if (!s.empty() && m_port > 0)
        ProxyHost[s] = m_port;

      p = strtok(NULL, "\n");
  }

  return ProxyHost.empty() == false;
}
