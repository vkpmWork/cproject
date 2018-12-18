/*
 * tclientsocket.h
 *
 *  Created on: 16.02.2016
 *      Author: irina
 */

#ifndef TCLIENTSOCKET_H_
#define TCLIENTSOCKET_H_

#include <string>
#include "loggermutex.h"
#include "log.h"

#define SOCKET_BLOCK    0
#define SOCKET_NONBLOCK 1

class tclient_socket
{

public :
        /* server */
    tclient_socket(char *m_host, int m_port);
	bool  do_accept();
	void  do_close();
    std::string  net_recv_string(int m_socket, int &m_res);

	/* client */
    tclient_socket(char *m_host, int m_port, int m_retry_interval);
    bool    net_connect();
    inline  char *get_host()                        { return host;            }
    inline  int  get_port()                         { return port;            }

        /* common */
	virtual ~tclient_socket();
	bool	net_send(const char *buf, int lehgth);
        bool    bsend   (const char *buf, int lehgth);
        bool    bsend  (int m_socket, char *buf, int lehgth);

        void    net_close();
	bool    net_recv(char*);
	void    net_recv();
	std::string  receive(int &sz);
	std::string  recv_file();
	std::string  net_recv_string();
	inline  bool is_connected()			{ return f_connected;	  }
	inline  bool get_error()                        { return error;           }

	void    close_clientfd(int m_pid);
private:
	char *host;
	int   port;
	int   sock;
	bool  f_connected;
	bool  error;
	int   sock_pid;
	void  set_error()                               { error = true; };
    int   retry_interval;



};
extern tclient_socket *ptrSocket;
#endif /* TCLIENTSOCKET_H_ */
