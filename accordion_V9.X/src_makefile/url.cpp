/*
 * url.cpp
 *
 *  Created on: 30.06.2016
 *      Author: irina
 */

#include "url.h"
#include <cctype>
#include <cstring>
#include <functional>
#include <stdio.h>
#include <iostream>
#include "tclientsocket.h"
#include <unistd.h>

const int MAX_URL      = 2048;
const int DEFAULT_PORT = 80;

namespace url
{

std::string query_url(char* u)
{
	std:: string ret_string;

    url::turl *url = new url::turl(u);
	  if (url)
	  {
			tclient_socket *Sock = new tclient_socket(url->get_host(), url->get_port(), 3, SOCKET_NONBLOCK, 3);
			if (Sock)
			{
				if (Sock->net_connect())
				{
					sleep(1);
					char * aa = url->get_query();
					int    bb = url->get_query_size();

					/*
					std::ostringstream m_message;
					m_message << aa << endl;
					logger::write_log(m_message);
					*/

					if (Sock->net_send(aa, bb))
					{
						sleep(2);
						ret_string =  Sock->net_recv_string();
					}

					Sock->net_close();
				}
				delete Sock;
				Sock = NULL;
			}

			delete url;
			url = NULL;
	  }

	return ret_string;
}

void transmit(char* m_host, int m_port, int m_count, char *m_data, int m_data_len)
{
	tclient_socket *Sock   = NULL;
	Sock = new tclient_socket(m_host, m_port, m_count, SOCKET_NONBLOCK, 3);

	if (Sock)
		if (Sock->net_connect())
		{
			usleep(2000);
			Sock->net_send(m_data, m_data_len);
			Sock->net_close();

			delete Sock;
			Sock = NULL;
		}
}
/* ------------------------------------------------------------- */
turl::turl(char* uri)
{
	Init();

	if (uri && strlen(uri))
	{
		url_string = strdup(uri);
		Parse();
	}
	else error_url = false;
}

turl::turl()
{
	Init();
}

void turl::Init()
{
	url_data.Host 		 = "";
	url_data.Path 		 = "";
	url_data.Protocol    = "";
	url_data.QueryString = "";
	url_data.Port 		 = DEFAULT_PORT;

	error_url 			 = false;

	url_string 			 = NULL;

	url_query 			 = (char*)malloc(MAX_URL+1);
}

turl::~turl()
{
	free(url_string);
	free(url_query);
}

void    turl::set_url_string(char* str)
{

	Init();

	if (str && strlen(str))
	{
		url_string = strdup(str);
		Parse();
	}
	else error_url = false;

}

void	turl::Parse()
{
	Parse(url_string);
}

void  turl::Parse(std::string uri)
{
    if (uri.length() == 0)
    {
        error_url = true;
    	return;
    }

    typedef std::string::const_iterator iterator_t;

    error_url = false;

    iterator_t uriEnd = uri.end();

    // get query start

    iterator_t queryStart = find(uri.begin(), uri.end(), '?');

    // protocol
    iterator_t protocolStart = uri.begin();
    iterator_t protocolEnd = find(protocolStart, uriEnd, ':');            //"://");

    if (protocolEnd != uriEnd)
    {
        std::string prot = &*(protocolEnd);
        if ((prot.length() > 3) && (prot.substr(0, 3) == "://"))
        {
            url_data.Protocol = std::string(protocolStart, protocolEnd);
            protocolEnd += 3;   //      ://
        }
        else
            protocolEnd = uri.begin();  // no protocol
    }
    else
        protocolEnd = uri.begin();  // no protocol

    // host
    iterator_t hostStart = protocolEnd;
    iterator_t pathStart = find(hostStart, uriEnd, '/');  // get pathStart

    iterator_t hostEnd   = find(protocolEnd, (pathStart != uriEnd) ? pathStart : queryStart, ':');  // check for port

    url_data.Host = std::string(hostStart, hostEnd);

    // port
    if ((hostEnd != uriEnd) && ((&*(hostEnd))[0] == L':'))  // we have a port
    {
        hostEnd++;
        iterator_t portEnd = (pathStart != uriEnd) ? pathStart : queryStart;
        url_data.Port 	   = atoi(std::string(hostEnd, portEnd).c_str());

        if (!url_data.Port)  url_data.Port = DEFAULT_PORT;
    }

    // path
    if (pathStart != uriEnd)
        url_data.Path = std::string(pathStart, queryStart);

    // query
    if (queryStart != uriEnd)
    		url_data.QueryString = std::string(queryStart, uriEnd);

}   // Parse

bool turl::get_error_url()
{
	return error_url;
}

char   *turl::get_host()
{
	return (char*)url_data.Host.c_str();
}

int	    turl::get_port()
{
	return url_data.Port;
}
char   *turl::get_query()
{
	memset(url_query, 0, MAX_URL);
        sprintf(url_query, "GET %s HTTP/1.1\nHost: %s\r\nUser_Agent: radio-accordion\r\nAccept-Language: en-us\r\n\r\n", url_data.Path.c_str(), url_data.Host.c_str());

	return url_query;
}

int		turl::get_query_size()
{
		return url_query ? strlen(url_query) : 0;
}


} /* namespace url */
