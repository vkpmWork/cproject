/*
 * filemutex.h
 *
 *  Created on: 12.03.2018
 *      Author: irina
 */

#ifndef FILEMUTEX_H_
#define FILEMUTEX_H_

#include "pthread.h"
class tfile_mutex
{
public:
  tfile_mutex()         { m_error = pthread_mutex_init(&mutex, NULL) == 0;
                          m_error = pthread_mutex_init(&mutex1, NULL) == 0;
                        }

  bool     error()      { return    m_error;                                           }
  bool     lock()       { m_error = pthread_mutex_lock(&mutex)   == 0; return m_error; }
  bool     unlock()     { m_error = pthread_mutex_unlock(&mutex) == 0; return m_error; }

  bool     lock1()       { m_error = pthread_mutex_lock(&mutex1)   == 0; return m_error; }
  bool     unlock1()     { m_error = pthread_mutex_unlock(&mutex1) == 0; return m_error; }

  virtual ~tfile_mutex(){ pthread_mutex_destroy(&mutex);
                          pthread_mutex_destroy(&mutex1);
                        }

private:
  pthread_mutex_t mutex;
  pthread_mutex_t mutex1;
  bool   m_error;
};
//extern tfile_mutex *pFileMutex;

#endif /* FILEMUTEX_H_ */
