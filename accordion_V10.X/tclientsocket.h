/*
 * tclientsocket.h
 *
 *  Created on: 16.02.2016
 *      Author: irina
 */

#ifndef TCLIENTSOCKET_H_
#define TCLIENTSOCKET_H_
#include <string>

#define SOCKET_BLOCK    0
#define SOCKET_NONBLOCK 1

class tclient_socket
{
public :
	tclient_socket(char *m_host, int m_port,  int m_connection, int m_sosket_type, int m_reconnection_time);
	bool 	connection_attempt() 			{ return connection_counter >= max_conections; };
	virtual ~tclient_socket();
	bool 	net_connect();
	bool	net_send(const char *buf, int lehgth);
	void    net_close();
	bool    net_recv(char*);
	void    net_recv();
	std::string net_recv_string();
	inline  void clear_connection_counter() { if (connection_counter) connection_counter = 0; }
	inline  bool is_connected()				{ return f_connected;	  }
private:
	char *host;
	int   port;
	int   max_conections;
	int   connection_counter;
	int   sock;
	bool  f_connected;
	int   socket_type;
	int   reconnection_time;
};

#endif /* TCLIENTSOCKET_H_ */
