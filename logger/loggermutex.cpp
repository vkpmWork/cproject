#include "loggermutex.h"
//#include "common.h"
#include "log.h"
TLoggerMutex *pLoggerMutex = NULL;

void DeleteLogMutex()
{
    if (pLoggerMutex)
    {  delete pLoggerMutex;
       pLoggerMutex = NULL;
    }
}

bool InitLoggerMutex()
{
    pLoggerMutex = new TLoggerMutex;
    if (pLoggerMutex && pLoggerMutex->SomeError) DeleteLogMutex();
    return pLoggerMutex != NULL;
}

TLoggerMutex::TLoggerMutex()
{
    SomeError = SetSomeError(pthread_mutex_init (&lockDir,   NULL));
    SomeError = SetSomeError(pthread_mutex_init (&lockWrite, NULL));
    SomeError = SetSomeError(pthread_mutex_init (&lockLocallog,NULL));
    SomeError = SetSomeError(pthread_mutex_init (&lockLocal, NULL));
    SomeError = SetSomeError(pthread_mutex_init (&lockSetup, NULL));
    SomeError = SetSomeError(pthread_mutex_init (&lockClientMessage,  NULL));

    SomeError = SetSomeError(pthread_mutex_init (&lockSocketRunnable, NULL));
    SomeError = SetSomeError(pthread_mutex_init (&lockFileRunnable,   NULL));
    SomeError = SetSomeError(pthread_mutex_init (&lockRemoteRunnable, NULL));

    SomeError = SetSomeError(pthread_mutex_init (&lockErrorMonitor, NULL));
}

TLoggerMutex::~TLoggerMutex()
{

    SomeError = SetSomeError(pthread_mutex_destroy (&lockDir));
    SomeError = SetSomeError(pthread_mutex_destroy (&lockWrite));
    SomeError = SetSomeError(pthread_mutex_destroy (&lockLocallog));
    SomeError = SetSomeError(pthread_mutex_destroy (&lockLocal));
    SomeError = SetSomeError(pthread_mutex_destroy (&lockSetup));
    SomeError = SetSomeError(pthread_mutex_destroy (&lockClientMessage));

    SomeError = SetSomeError(pthread_mutex_destroy (&lockSocketRunnable));
    SomeError = SetSomeError(pthread_mutex_destroy (&lockSocketRunnable));
    SomeError = SetSomeError(pthread_mutex_destroy (&lockSocketRunnable));

    SomeError = SetSomeError(pthread_mutex_destroy (&lockErrorMonitor));
}

bool TLoggerMutex::SetSomeError(int value)
{
    string str = "";
    switch(value)
    {
        case  ENOMEM :  str = "Insufficient memory exists to initialise the mutex";
                        break;
        case  EINVAL :  str = "The value specified by attr(mutex, if destroy) is invalid";
                        break;
        case  EBUSY  :  str = "The implementation has detected an attempt to destroy the object referenced by mutex while it is locked or referenced";
                        break;
    }

    if (str.length())
    {
    	std::ostringstream s;
    	s << str;
    	winfo(s);
    }
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

bool TLoggerMutex::Local_MutexLock()
{
   return MutexLock(&lockLocal);
}

bool TLoggerMutex::Local_MutexUnlock()
{
    return MutexUnlock(&lockLocal);
}


