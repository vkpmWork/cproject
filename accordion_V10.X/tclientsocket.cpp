/*
 * tclientsocket.cpp
 *
 *  Created on: 16.02.2016
 *      Author: irina
 */

#include "tclientsocket.h"
//#include <sys/socket.h> VS <netdb.h>
//#include <netinet/in.h> VS <netdb.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include "stdlib.h"
#include <unistd.h>
#include <fcntl.h>
#include "log.h"


tclient_socket::tclient_socket(char *m_host, int m_port,  int m_connection, int m_socket_type, int m_reconnection_time)
				: port(m_port)
				, max_conections(m_connection)
				, connection_counter(0)
				, sock(-1)
				, f_connected(false)
				, socket_type(m_socket_type)
				, reconnection_time(m_reconnection_time)
{
	host   = strdup(m_host);
}

tclient_socket::~tclient_socket()
{
    net_close();

    if (logger::ptr_log->get_loglevel() == common::levDebug)
    {
        logger::message.str("");
        logger::message << " Socket closed"  << std::endl;
        logger::write_log(logger::message);
    }
    free(host);
}

bool tclient_socket::net_connect()
{
	if (f_connected) return true;

	/*  -1 - error socket or host
	 *   0 - connections limit attempt
	 *   1 - Ok!
	 */

    struct sockaddr_in serv_addr;
    struct hostent     *srv;
    int  ret_value     = -1;


    std::ostringstream m;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
    	m << errno << " " << time(0) << " Socket error : " << strerror(errno) << std::endl;
    	logger::write_log(m);
    	logger::write_info(m);
        return ret_value;
    }

/*
    if (socket_type == SOCKET_NONBLOCK)
    {
		struct timeval tv;
		tv.tv_sec  = 1;
		tv.tv_usec = 0;  // mks Not init'ing this can cause strange errors
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv,sizeof(struct timeval));

		tv.tv_sec  = 120;
		tv.tv_usec = 0; //0;  // mks Not init'ing this can cause strange errors
		setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv,sizeof(struct timeval));
    }
*/

    struct timeval tv;
    tv.tv_sec  = 10; //60
    tv.tv_usec = 0;  //10*1000; //0;  // mks Not init'ing this can cause strange errors
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv,sizeof(struct timeval));

    srv = gethostbyname(host);
    if (!srv)
    {
    	m << h_errno << " " << time(0) << " Socket error : " << hstrerror(errno) << std::endl;
    	logger::write_log(m);
    	logger::write_info(m);
    	return ret_value;
    }
    bzero((char *) &serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port   = htons((uint16_t)port);
    bcopy( (char*)srv->h_addr, (char*)&serv_addr.sin_addr, srv->h_length );

    connection_counter = 0;

    int m_errno = 0;
    while (connection_counter < max_conections)
    {
    	ret_value = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));

    	if (ret_value != -1) break;
    	else
    	{
    		m_errno = errno;
    	        if (m_errno == EISCONN)
    		{
    		    ret_value++;
    		    break;
    		}

    		if (reconnection_time) sleep(reconnection_time);
    	        else sleep(5);

    		connection_counter++;
                m.str("");
                m << "Connection attempt number " << connection_counter << " " << host << ":" << port << endl;
    		logger::write_marker(m);
    	}
    }


    if(ret_value == -1)
    {
            m << m_errno << " " << time(0) << " Connection error : " << strerror(m_errno) << " on host : " << host << " port : " << port << std::endl;
            logger::write_log(m);
            logger::write_info(m);
            return  f_connected = false;
    }


    if (socket_type == SOCKET_NONBLOCK)
    {
                        if (connection_counter)  connection_counter = 0;

                        long socketFlags = fcntl(sock, F_GETFL, NULL);
			if (socketFlags < 0)
			{
					m << errno << " " << time(0) << " Impossible to retrieve socket descriptor flags : " << strerror(errno) << std::endl;
					logger::write_log(m);
					logger::write_info(m);
					//return f_connected = false;
			}

			socketFlags |= O_NONBLOCK;
			if (fcntl(sock, F_SETFL, socketFlags) < 0 )
			{
					m << errno << " " << time(0) << " Impossible to update socket descriptor flags : " << strerror(errno) << std::endl;
					logger::write_log(m);
					logger::write_info(m);
					//return f_connected = false;
			}

    }

    f_connected = (int)(ret_value != -1);
    return f_connected;
}

bool tclient_socket::net_send(const char *buf, int length)
{
	if (!f_connected) return false;
	int total = 0, m_out = 0;
	std::ostringstream m;

	while(total < length)
	{
	                  m_out = send(sock, buf+total, length-total, MSG_NOSIGNAL);
	                  if(m_out == -1)
	                  {
	                          if (errno == EAGAIN)
	                          {   usleep(200);
	                              continue;
	                          }

	                          m.str("");
	                          m << errno << " " << time(0) <<  " : " << "net_send(const char *buf, int length) Error : " << strerror(errno) << std::endl;
	                          logger::write_log(m);
	                          logger::write_info(m);
	                          net_close();
	                          //f_connected = false;
	                          break;
	                  }
	                  total += m_out;
	  }
	  return (m_out ==-1 ? false : true);
}

bool  tclient_socket::net_recv(char *s)
{
	int ret_value = 0;
	std::ostringstream m;

	char* buf 	  = (char*)malloc(65535);
	if (buf)
	{
		bzero((char *) buf, 65535);
		ret_value = recv(sock, buf, 65535, 0);
	}

	if (ret_value <= 0)
	{
		m << errno << " " << time(0) << " net_recv(char*): " << buf << " " << strerror(errno) << std::endl;
		logger::write_log (m);
		logger::write_info(m);
		net_close();
		return ret_value;
	}

	if (strstr(buf, s)) ret_value = true;
	else
	{
		m << errno << " " << time(0) << " net_recv(char*): Received buffer: " << buf << std::endl;
		logger::write_log(m);
 		logger::write_info(m);

		ret_value = false;
	}

	free(buf);

	return ret_value;
}


void  tclient_socket::net_recv()
{
	sleep(1);

	char* buf 	  = (char*)malloc(65535);
	int ret_value = -1;
	if (buf)
	{
		bzero((char *) buf, 65535);
		ret_value = recv(sock, buf, 65535, 0);
	}

	if (!ret_value)
	{
		logger::message.str("");
		logger::message << errno << " " << time(0) << " net_recv(): " << buf << " Received : " << strerror(errno) << std::endl;

		if (logger::ptr_log->get_loglevel() == common::levDebug) logger::write_log(logger::message);
 		logger::write_info(logger::message);
		net_close();
	}

	free(buf);
}

std::string tclient_socket::net_recv_string()
{
	sleep(1);
	std::string str;

	int   sz 	  = 0;
	char* buf 	  = (char*)malloc(50001);
	if (buf)
	{
		do
		{
			sz = recv(sock, buf, 50000, 0);

			if (sz > 0)
			{
				str.append(buf, sz);
			}
			else
			{
		                if (errno == EAGAIN) continue;
			        logger::message.str("");
		                logger::message << errno << " " << time(0) << " net_recv(): " << buf << " Received : " << strerror(errno) << std::endl;
		                if (logger::ptr_log->get_loglevel() == common::levDebug) logger::write_log(logger::message);
		                logger::write_info(logger::message);
			}
		}
		while (sz > 0);
		free(buf);
	}

	if (str.empty()) net_close();

	return str;
}

void tclient_socket::net_close()
{
	if (f_connected)
	{
		shutdown(sock, 2); /* запретить чтение из сокета +  запретить запись в сокет */
		close   (sock);
		sock = -1;
	}
	f_connected = false;
}


