/*
 * tclientsocket.cpp
 *
 *  Created on: 16.02.2016
 *      Author: irina
 *
 */

#include "tclientsocket.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <iostream>
#include <cerrno>
#include <cstring>
#include "stdlib.h"
#include <unistd.h>
#include <fcntl.h>
#include "mqueue.h"
#include <netdb.h>
#include <sys/wait.h>

//#include <msgpack.hpp>

//#include <iostream>
//#include <sstream>

#define MAXLEN   4096

tclient_socket  *ptrSocket = NULL;
//mgueue::tmqueue *pMQ       = NULL;

void sighandler(int sig)
{

      pid_t p;
      int status;

      /* loop as long as there are children to process */
      while (true)
      {

         /* retrieve child process ID (if any) */
         p = waitpid(-1, &status, WNOHANG);

         /* check for conditions causing the loop to terminate */
         if (p == -1)
         {
             /* continue on interruption (EINTR) */
             if (errno == EINTR) continue;

             /* break on anything else (EINVAL-неверный аргумент  or ECHILD-нет процессов потомков according to manpage) */
             break;
         }
         else if (p == 0) /* no more children to process, so break */
                   break;
         /* valid child process ID retrieved, process accordingly */


      }

}


bool   retransmit(int m_socket, std::string m_str)
{
  bool fl_ok = false;
  if (ptrSocket)
  {
      char *b = (char*)m_str.c_str();
      fl_ok = ptrSocket->bsend(m_socket, b, m_str.length());
  }
  return fl_ok;
}

bool retransmit(int m_socket, char *m_str, int m_len, pid_t m_pid)
{
  bool fl_ok = false;
  if (ptrSocket)
  {
        fl_ok = ptrSocket->bsend(m_socket, m_str, m_len);
        if (fl_ok)
          {
            std::ostringstream ss;
            ss  << m_pid << " Buffer: " << m_str << endl;
            wmsg(ss, common::levError);

          }
  }
  return fl_ok;
}

tclient_socket::tclient_socket(char *m_host, int m_port, int m_TO)
				: port(m_port)
				, sock(-1)
				, f_connected(false)
                , error(false)
                , transmit_timeout(m_TO)
{
/*	msgpack::type::tuple<int, bool, std::string> src(1, true, "example");

	    // serialize the object into the buffer.
	    // any classes that implements write(const char*,size_t) can be a buffer.
	    std::stringstream buffer;
	    msgpack::pack(buffer, src);

	    // send the buffer ...
	    buffer.seekg(0, buffer.beg);

	    // deserialize the buffer into msgpack::object instance.
	    std::string str(buffer.str());

	    msgpack::object_handle oh =
	        msgpack::unpack(str.data(), str.size());

	    // deserialized object is valid during the msgpack::object_handle instance is alive.
	    msgpack::object deserialized = oh.get();

	    // msgpack::object supports ostream.
	    std::cout << deserialized << std::endl;

	    // convert msgpack::object instance into the original type.
	    // if the type is mismatched, it throws msgpack::type_error exception.
	    msgpack::type::tuple<int, bool, std::string> dst;
	    deserialized.convert(dst);

	    // or create the new instance
	    msgpack::type::tuple<int, bool, std::string> dst2 =
	        deserialized.as<msgpack::type::tuple<int, bool, std::string> >();
*/

//++++++++++++++++++++++++++++++++++++++++
	    //++++++++++++++++++++++++++++++++++++++++
	    //++++++++++++++++++++++++++++++++++++++++
	    std::ostringstream msg;

        sock_pid = getpid();
        host = strdup(m_host);

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == -1)
        {
            msg << "Couldn't create a socket: " << strerror(errno) << endl;
            wmsg(msg, common::levError);
            set_error();
            return;
        }

        /* Prevents those dreaded "Address already in use" errors */
        int yes = 1; // used in setsockopt(2)
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&yes, sizeof(int)) == -1)
        {
                msg << "Couldn't setsockopt: " << strerror(errno) << endl;
                wmsg(msg, common::levError);
                if (sock != -1) close(sock);
                set_error();
                return;
        }

        /* fixed for OS X, would not bind otherwise */
        struct addrinfo ai_hint;
        memset(&ai_hint, 0, sizeof(ai_hint));
        ai_hint.ai_family = AF_INET;
        ai_hint.ai_socktype = SOCK_STREAM;
        ai_hint.ai_flags = AI_PASSIVE;

        /* Fill the address info struct (host + port) -- getaddrinfo(3) */
        struct addrinfo *m_addrinfo;
        string s_port = atos(port);
        if (getaddrinfo(host, s_port.c_str(), &ai_hint, &m_addrinfo) != 0)
        {
            msg << "Couldn't get address: " << strerror(errno) << endl;
            wmsg(msg, common::levError);
            if (sock != -1) close(sock);
            set_error();
            return;
        }

        /* Assign address to this socket's fd */
        if (bind(sock, m_addrinfo->ai_addr, m_addrinfo->ai_addrlen) != 0)
        {
            msg << "Couldn't bind socket to address: " << strerror(errno) << endl;
            wmsg(msg, common::levError);
            if (sock != -1) close(sock);
            set_error();
            return;
        }

        /* Free the memory used by our address info struct */
        freeaddrinfo(m_addrinfo);
        /* Mark this socket as able to accept incoming connections */
        //std::cout <<  "SOMAXCONN: " << SOMAXCONN << endl;
        if (listen(sock, SOMAXCONN) == -1)
        {
                msg << "Couldn't make socket listen: " << strerror(errno) << endl;
                if (sock != -1) close(sock);
                set_error();
                return;
        }
        else
        {
                std::ostringstream ss;
                ss << "The server is listening on network interfaces! Host: " << host << " Port: "<< port;
                wmsg(ss, common::levError);
        }

        if (!error) ptrSocket = this;
//        queue_id =  create_queue_id();

}

bool  tclient_socket::do_accept()
{
         /* Fork some child processes. */
          struct sigaction sa;
          sa.sa_handler = &sighandler;
          sigemptyset(&sa.sa_mask);
          sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
          if (sigaction(SIGCHLD, &sa, 0) == -1)
          {
            perror(0);
            exit(1);
          }

         int flags = 0;
         struct sockaddr_in *client;
         socklen_t client_len;

         std::ostringstream ss;

         for (;;) // Run forever ...
         {
                         /* Necessary initialization for accept(2) */
                         client_len = sizeof client;

                         /* Blocks! */
                         int clientfd = accept(sock, (struct sockaddr *)&client, &client_len);
                         if (clientfd == -1)
                         {
                             ss.str("");
                             ss  << "Accept: "<< strerror(errno) << endl;
                             wmsg(ss, common::levError);
                             continue;
                         }

                         flags = fcntl(clientfd, F_GETFL, 0);
                         if (fcntl(clientfd, F_SETFL, flags | O_NONBLOCK | FD_CLOEXEC) == -1)
                           {
                               ss.str("");
                               ss << "fcntl(clientfd, F_SETFL, flags | O_NONBLOCK | FD_CLOEXEC): "<< strerror(errno) << endl;
                               wmsg(ss, common::levError);
                               continue;
                           }


/*                                 char host[NI_MAXHOST]; // to store resulting string for ip
                                 memset(host, 0, NI_MAXHOST);
                                 if (getnameinfo((sockaddr*)&client, client_len,
                                                   host, NI_MAXHOST, // to try to get domain name don't put NI_NUMERICHOST flag
                                                   NULL, 0,          // use char serv[NI_MAXSERV] if you need port number
                                                   NI_NUMERICHOST    // | NI_NUMERICSERV
                                                ) == 0)
                                 {
                                     //printf("Connected to: %s\n", host);
                                 }
*/

                         handler_proc::create_client_thread(&clientfd);
                         //handler_proc::handler_client_thread(&clientfd);
         }

         return true;
}

void  tclient_socket::do_close()
{
  /* Sit back and wait for all child processes to exit */
  pid_t m_pid;
  do {
        m_pid = waitpid(-1, NULL, 0);
        if (m_pid == -1) break;

        std::ostringstream ss;
        ss << "PID = " << m_pid << " Завершние процесса (do_close)"  << endl;
        wmsg(ss, common::levDebug);
  } while (true);

  while (waitpid(-1, NULL, 0) > 0)

  /* Close up our socket */
  if (sock != -1)
  {
      close(sock);
      sock = -1;
  }
}

tclient_socket::~tclient_socket()
{
    free(host);
}

bool tclient_socket::net_connect()
{
    std::ostringstream message;
    message.str("");
    struct sockaddr_in serv_addr;
    struct hostent     *srv;
    int  ret_value     = -1;


    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
    	message << time(0) << " net_connect Socket(AF_INET, SOCK_STREAM, 0) : " << strerror(errno) << std::endl;
    	winfo(message);
        return ret_value;
    }

//    int flags = fcntl(sock, F_GETFL, 0);
//    if(fcntl(sock, F_SETFL, flags | O_NONBLOCK) == -1)
    {
      /*
       * Не получилось...
       */
    }

        srv = gethostbyname(host);
        if (!srv)
        {
            message << time(0) << " net_connect Gethostbyname(host); : " << hstrerror(errno) << std::endl;
            winfo(message);
            return ret_value;
        }
        bzero((char *) &serv_addr, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port   = htons((uint16_t)port);
        bcopy( (char*)srv->h_addr, (char*)&serv_addr.sin_addr, srv->h_length );

        ret_value = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));


    f_connected = (int)(ret_value != -1);
    return f_connected;
}
bool tclient_socket::bsend(const char *buf, int length)
{
  int total = 0;
  while(total < length)
  {
                  total += send(sock, buf+total, length, 0);
                  if(total == -1)
                  {
                          std::ostringstream msg;
                          msg << time(0) <<  " : " << "tclient_socket::send(const char *buf, int length) " << strerror(errno) << "data:  " << buf << std::endl;
                          logger::write_log(msg);
                          break;
                  }

                  length -=total;
  }

  return total != -1;
}

bool tclient_socket::bsend(int m_socket, char *buf, int length)
{
  int total = 0;
  int m_out = 0;
  while(total < length)
  {
                  m_out = send(m_socket, buf+total, length-total, 0);
                  if(m_out == -1)
                  {
                          std::ostringstream msg;
                          msg << time(0) <<  " : " << "tclient_socket::send(const char *buf, int length) " << strerror(errno) << "data:  " << buf << std::endl;
                          wmsg(msg, common::levError);
                          break;
                  }
                  if (m_out == 0)
                    {
                      std::ostringstream msg;
                      msg << time(0) <<  " : " << "tclient_socket::send(const char *buf, int length) " << strerror(errno) << "m_out == 0" << buf << std::endl;
                      wmsg(msg, common::levError);
                      break;
                    }
                  total += m_out;
                  //length -=total;
  }

  return m_out != -1;
}

bool tclient_socket::net_send(const char *buf, int length)
{
    if (!f_connected) return false;

    int total = MAXLEN/*FD_SETSIZE*/;
    int n     = 0;
    int counter = 0;
    int m_errno = 0;

    long t = common::mtime();

    while(length >= total)
    {
    	n = send(sock, buf+counter, total, 0);
        if(n == -1)
        {
        	std::ostringstream message;
			message << errno << " " << time(0) <<  " : " << "net_send(const char *buf, int length) Transmit error while(length >= total): " << strerror(m_errno = errno) << std::endl;
   	        logger::write_log(message);

        	break;
        }

        length  -= n;
    	counter += n;
    }

	total = 0;
	while(n != -1 && total < length)
	{
			n = send(sock, buf+ counter + total, length-total, 0);
			if(n == -1)
			{
						std::ostringstream message;
	        			message << errno << " " << time(0) <<  " : " << "net_send(const char *buf, int length) Transmit error while(... total < length): " << strerror(m_errno = errno) << std::endl;
						logger::write_log(message);
						break;
			}

			total += n;
	}

   if(n == -1/* && m_errno != EAGAIN*/)
   {
	   std::ostringstream message;
	   message.str("net_close()\n");
	   logger::write_info (message);
    	net_close();
   }

   t = common::mtime() - t;

   if (t > 10 || n == -1)
   {
	   std::ostringstream message;
	   message << "0 " << time(0) <<  " : " << "Timeout on Send : " << t <<  " milliseconds" << std::endl;
	   logger::write_log(message);
   }

    return (n==-1 ? false : true);
}

bool  tclient_socket::net_recv(char *s)
{
	int ret_value = 0;

	char* buf 	  = (char*)malloc(65535);
	if (buf)
	{
		bzero((char *) buf, 65535);
		ret_value = recv(sock, buf, 65535, 0);
	}

	if (ret_value <= 0)
	{
		std::ostringstream message;
		message << errno << " " << time(0) << " net_recv(char*): " << buf << " " << strerror(errno) << std::endl;
		logger::write_log (message);
		net_close();
		return ret_value;
	}

	if (strstr(buf, s)) ret_value = true;
	else
	{
		std::ostringstream message;
		message << errno << " " << time(0) << " net_recv(char*): " << buf << " Received from : " << strerror(errno) << std::endl;
		logger::write_log(message);

		ret_value = false;
	}

//	ret_value == (bool)(strstr(buf, s) != NULL);
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
		std::ostringstream message;
		message << errno << " " << time(0) << " net_recv(): " << buf << " Received : " << strerror(errno) << std::endl;
		logger::write_log(message);
		net_close();
	}

	free(buf);
}

std::string  tclient_socket::receive(int &m_size)
{
        std::string str;
        char* buf = (char*)calloc(MAXLEN, 1);
        m_size = recv(sock, buf, MAXLEN, 0);

        if (m_size) str.append(buf, m_size);
        free(buf);

        return str;
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
		}
		while (sz > 0);
		free(buf);
	}

	#ifdef DEBUG
	std::ostringstream s;
	s << "Pid = " << sock_pid << "Получен файл: " << str;
	wmsg(s, common::levError);
	#endif

	return str;
}


std::string tclient_socket::net_recv_string(int m_socket, int &m_res)
{
        std::string str;
        int   sz          = 0;
        char* buf         = (char*)malloc(50001);
        if (buf)
        {
                do
                {
                        sz = recv(m_socket, buf, 50000, 0);

                        if (sz > 0)
                        {
                                str.append(buf, sz);
                        }
                }
                while (sz > 0);
                free(buf);
        }

        m_res = sz;
        return str;
}

std::string  tclient_socket::recv_file()
{
  std::string str;

  int   sz          = 0;
  char* buf         = (char*)malloc(MAXLEN);
  if (buf)
  {
          do
          {
                  sz = recv(sock, buf, MAXLEN, 0);

                  if (sz > 0)
                  {
                          str.append(buf, sz);
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
		shutdown(sock, SHUT_RDWR); /* запретить чтение из сокета +  запретить запись в сокет */
		close   (sock);
		sock = -1;
	}
	f_connected = false;
}

