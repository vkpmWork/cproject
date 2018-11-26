#ifndef LOGGERMUTEX_H
#define LOGGERMUTEX_H
#include <pthread.h>

class TLoggerMutex
{
public :
    bool     SomeError;
    explicit TLoggerMutex();
    bool     Local_MutexLock();
    bool     Local_MutexUnlock();

    bool     Dir_MutexLock()               { return MutexLock  (&lockDir);   }
    bool     Dir_MutexUnlock()             { return MutexUnlock(&lockDir);   }

    bool     Locallog_MutexLock()          { return MutexLock  (&lockLocallog);}
    bool     Locallog_MutexUnlock()        { return MutexUnlock(&lockLocallog);}

    bool     Write_MutexLock()             { return MutexLock  (&lockWrite); }
    bool     Write_MutexUnlock()           { return MutexUnlock(&lockWrite); }

    bool     Setup_MutexLock()             { return MutexLock  (&lockSetup); }
    bool     Setup_MutexUnlock()           { return MutexUnlock(&lockSetup); }

    bool     ClientMessage_MutexLock()     {return MutexLock    (&lockClientMessage); }
    bool     ClientMessage_MutexUnlock()   {return MutexUnlock  (&lockClientMessage); }


    bool     SocketRunnable_MutexLock()    { return MutexLock   (&lockSocketRunnable);}
    bool     SocketRunnable_MutexUnlock()  { return MutexUnlock (&lockSocketRunnable);}

    bool     FileRunnable_MutexLock()      { return MutexLock   (&lockFileRunnable);  }
    bool     FileRunnable_MutexUnlock()    { return MutexUnlock (&lockFileRunnable);  }

    bool     RemoteRunnable_MutexLock()    { return MutexLock   (&lockRemoteRunnable);}
    bool     RemoteRunnable_MutexUnlock()  { return MutexUnlock (&lockRemoteRunnable);}

    bool     ErrorMonitor_MutexLock()      { return MutexLock   (&lockErrorMonitor);}
    bool     ErrorMonitor_MutexUnlock()    { return MutexUnlock (&lockErrorMonitor);}

    ~TLoggerMutex();
private:

    pthread_mutex_t lockSetup;
    pthread_mutex_t lockDir;
    pthread_mutex_t lockLocallog;
    pthread_mutex_t lockWrite;
    pthread_mutex_t lockLocal;
    pthread_mutex_t lockClientMessage;

    pthread_mutex_t lockSocketRunnable;
    pthread_mutex_t lockFileRunnable;
    pthread_mutex_t lockRemoteRunnable; /* запись/чтение из LocalLog.log, когда удаленный сервер отвалился */

    pthread_mutex_t lockErrorMonitor;



    bool     SetSomeError(int);
    bool     MutexLock(pthread_mutex_t*  );
    bool     MutexUnlock(pthread_mutex_t*);
};

extern TLoggerMutex *pLoggerMutex;
extern bool InitLoggerMutex();
extern void DeleteLogMutex();

#endif // LOGGERMUTEX_H
