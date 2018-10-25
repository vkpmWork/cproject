/*
 * mqueue.cpp
 *
 *  Created on: 05.07.2018
 *      Author: irina
 */

#include "mqueue.h"
#include <sys/msg.h>
#include <string.h>
#include <sys/stat.h>

namespace mgueue
{

  tmqueue::tmqueue(char* key_name, int m_pid)
          :  msg_len(0)
          ,  qexist(false)

  {
    key_t msgkey   = ftok(key_name, m_pid);
    this->queue_id = msgget(msgkey, 0777 | IPC_CREAT | S_IREAD | S_IWRITE);
    if (queue_id != -1)
    {
        qexist   = true;
        msg_len  = sizeof(message_buf) - sizeof(long); /* Length of the message */
    }

  }

  tmqueue::~tmqueue()
  {
      if (qexist) msgctl(queue_id, IPC_RMID, 0);
  }

  bool tmqueue::msg_send(char* m_msg, int m_len)
  {
    /* Sends the message */
    if (!qexist) return false;

    message_buf sent;
    sent.msg_type = MSG_TYPE;

    memset(sent.msg, '\0', MSG_SIZE);
    memcpy(sent.msg, m_msg, m_len);
    int err = msgsnd(queue_id, &sent, msg_len, IPC_NOWAIT | MSG_NOERROR);

    return err != -1;
  }

  char* tmqueue::msg_recv(int &m_len)
  {

    m_len = 0;
    if (!qexist) return NULL;

    message_buf received;
    memset(received.msg, '\0', MSG_SIZE);
    /* Receives the message */
    int err = msgrcv(queue_id, &received, msg_len, MSG_TYPE, IPC_NOWAIT | MSG_NOERROR);

    if (err != -1) m_len = strlen(received.msg) + 1;

    return  err != -1 ? (char*)received.msg : NULL;
  }

  int   tmqueue::qcount()
  {
    if (!qexist) return 0;

    struct msqid_ds qstatus;
    if(msgctl(queue_id, IPC_STAT, &qstatus) < 0) return 0;

    return qstatus.msg_qnum;
  }


} /* namespace mgueue */
