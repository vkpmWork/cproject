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

    bool     StartPlayTrack_MutexLock()         	{ return MutexLock  (&mu_start_play_track);		 }
    bool     StartPlayTrack_MutexUnlock()       	{ return MutexUnlock(&mu_start_play_track);		 }

    bool     StopPlayTrack_MutexLock()         		{ return MutexLock  (&mu_stop_play_track);		 }
    bool     StopPlayTrack_MutexUnlock()       		{ return MutexUnlock(&mu_stop_play_track);		 }

    bool     Change_MutexLock()         			{ return MutexLock  (&mu_change_playlist);   }
    bool     Change_MutexUnlock()       			{ return MutexUnlock(&mu_change_playlist);   }

    bool     Playlist_MutexLock()         			{ return MutexLock  (&mu_playlist);			 }
    bool     PlayList_MutexUnlock()       			{ return MutexUnlock(&mu_playlist);   		 }

    bool     ChangePlaylist_MutexLock()         	{ return (MutexLock  (&mu_playlist) && MutexLock  (&mu_change_playlist)) ;}
    bool     ChangePlayList_MutexUnlock()       	{ return (MutexUnlock(&mu_playlist) && MutexUnlock(&mu_change_playlist)) ;   		 }

    ~TLoggerMutex();
private:

    pthread_mutex_t mu_start_play_track;
    pthread_mutex_t mu_stop_play_track;

    pthread_mutex_t mu_change_playlist;
    pthread_mutex_t mu_playlist;

    bool     SetSomeError(int);
    bool     MutexLock(pthread_mutex_t*  );
    bool     MutexUnlock(pthread_mutex_t*);
};

extern TLoggerMutex *pLoggerMutex;
//extern bool InitLoggerMutex();
//extern void DeleteLogMutex();

#endif // LOGGERMUTEX_H
