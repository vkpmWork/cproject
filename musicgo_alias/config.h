/*
 * config.h
 *
 *  Created on: 29.01.2016
 *      Author: irina
 */

#ifndef CONFIG_H_
#define CONFIG_H_
#include "common.h"
#include "map"

typedef std::map<std::string, std::string> tpath_media;
typedef std::map<std::string, int> MHost;
typedef enum {tpProxy, tpLoader} tpTracts;

class tconfig
{
public:
	tconfig(int argc, char**argv);
	virtual ~tconfig();

	bool    get_cfg_error()		{ return (bool)cfg_error; }
	char*	get_config_file();

	char*   get_logfile()		{ return logfile;       }
	int     get_logsize()		{ return logsize*1024;  }
	common::tcLogLevel get_loglevel();

	char*   get_host()			{ return host; 		}
	int     get_port()			{ return port;		}

/*	char*   get_client_host()	{ return client_host; 		}
	int     get_client_port()	{ return client_port;		}
*/
	char*   get_proxy_host()	{ return proxy_host;    }
	int     get_proxy_port()	{ return proxy_port;    }

	MHost   get_proxy()             { return ProxyHost;     }

	bool    get_daemon()		{ return is_daemon;	}
	std::string get_header()	{ return m_header;	}

	bool    isLoader()              { return rejime == tpLoader; }
        bool    isProxy ()              { return rejime == tpProxy;  }
        tpTracts  get_rejime()          { return rejime;             }

        int     get_transmit_timeout()  { return transmit_timeout * 1000;}
        tpath_media   get_path_media()  { return m_pathmedia;   }

        int     get_start_position()    { return start_position;}
        int     get_duration()          { return duration;      }
        int     get_token_lifetime()    { return token_lifetime;}

private:
	char* cfg_file;
	int   cfg_error;

	char *host;
	int   port;

	char *client_host;
	int   client_port;

	MHost ProxyHost;

	char *proxy_host;
	int   proxy_port;

	char *logfile;
	int   logsize;
	common::tcLogLevel   loglevel;

	bool  is_daemon;

	std::string m_header;

	tpTracts rejime;

	int   transmit_timeout;
//	char  *path_media;

	int   start_position; // in seconds
	int   duration;       // in seconds
	int   token_lifetime;

	tpath_media m_pathmedia;

	void ReadIni();
	bool make_host(char*);
};

extern tconfig *pConfig;
#endif /* CONFIG_H_ */
