/*
 * mqueue.h
 *
 *  Created on: 05.07.2018
 *      Author: irina
 */

#ifndef MQUEUE_H_
#define MQUEUE_H_


#define MSG_SIZE      4096
#define MSG_TYPE      1

namespace mgueue
{

  typedef struct msgbuf
  {
      long msg_type;
      char msg[MSG_SIZE];
  } message_buf;

  class tmqueue
  {
  public:
    tmqueue(char* key_name /* application name */, int m_pid /* parent PID*/);
    virtual  ~tmqueue();

    bool  msg_send(char*, int);
    char* msg_recv(int &);
    bool  qempty()      { return qcount() == 0; }
    bool  qexists()     { return qexist;   }
  private:
    int  queue_id;
    int  msg_len;
    int  qcount();
    bool qexist;
 };

} /* namespace mgueue */
#endif /* MQUEUE_H_ */
