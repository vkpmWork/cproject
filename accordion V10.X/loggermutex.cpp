#include "loggermutex.h"
#include <string>
#include <errno.h>
#include <iostream>
#include "log.h"

TLoggerMutex *pLoggerMutex = NULL;

TLoggerMutex::TLoggerMutex()
{
    SomeError = SetSomeError(pthread_mutex_init (&mu_start_play_track,      NULL));
    SomeError = SetSomeError(pthread_mutex_init (&mu_stop_play_track,      NULL));
    SomeError = SetSomeError(pthread_mutex_init (&mu_change_playlist, NULL));
    SomeError = SetSomeError(pthread_mutex_init (&mu_playlist, NULL));
}

TLoggerMutex::~TLoggerMutex()
{

    SomeError = SetSomeError(pthread_mutex_destroy (&mu_start_play_track));
    SomeError = SetSomeError(pthread_mutex_destroy (&mu_stop_play_track));
    SomeError = SetSomeError(pthread_mutex_destroy (&mu_change_playlist));
    SomeError = SetSomeError(pthread_mutex_destroy (&mu_playlist));
}

bool TLoggerMutex::SetSomeError(int value)
{
	logger::message.str().clear();
	switch(value)
    {
        case  ENOMEM :  logger::message.str("Insufficient memory exists to initialise the mutex\n");
                        break;
        case  EINVAL :  logger::message.str("The value specified by attr(mutex, if destroy) is invalid");
                        break;
        case  EBUSY  :  logger::message.str("The implementation has detected an attempt to destroy the object referenced by mutex while it is locked or referenced");
                        break;
    }

    if (value) logger::write_log(logger::message);
    return value != 0;
}

bool TLoggerMutex::MutexLock(pthread_mutex_t *mutex)
{

    return pthread_mutex_lock(mutex) == 0;
}

bool TLoggerMutex::TLoggerMutex::MutexUnlock(pthread_mutex_t *mutex)
{
    return pthread_mutex_unlock(mutex) == 0;
}



