/*
 * obj_timer.h
 *
 *  Created on: 12.12.2014
 *      Author: irina
 */

#ifndef OBJ_TIMER_H_
#define OBJ_TIMER_H_

#include <stdlib.h>
#include <math.h>
#include <time.h>
//#include <iostream>

using namespace std;

namespace php_rotator
{

#define HOUR_INTERVAL 3600 /* секунды в одном часе */
#define DEFAULT_TRACK_DURATION	     "00:03:00" /* длительность трека , мин*/
#define DEFAULT_DAY   24 	

class TTimer
{
public:

	TTimer(char *str) : s_time(NULL)
				{
				  m_remained_interval  	   = HOUR_INTERVAL;
				  m_default_track_duration = get_seconds((char*)DEFAULT_TRACK_DURATION);
				  clear_playlist_duration();
				  set_general_duration(str);
				}

	~TTimer()	{ if (s_time) { free(s_time); s_time = NULL; } }

    inline void		set_interval(int counter)  			{ m_remained_interval = HOUR_INTERVAL * counter;}       /* определяем длительность ротации одного  списка правил ORDER       */

    inline long int     remained_interval(char* stime)  		{ return remained_interval(get_seconds(stime)); /*m_remained_interval -= get_seconds(stime);*/} 	/* остаток время ротации после добавления трека, длительностью char* */

    inline bool         interval_completed()	   			{ return m_remained_interval <= 0;} 					/* true -  время ротации вышло                                       */

    long int 		append_playlist_duration(char* stime) 	 	{
									  if (!stime) return get_playlist_duration();
									  long int value    = get_seconds(stime);
									  return append_playlist_duration(value); /*m_playlist_duration += value;*/

									}	/* увеличить длительность плейлиста на время трека 					 */

    inline void 	clear_playlist_duration() 			{ m_playlist_duration = 0; }

    inline long int     get_playlist_duration()				{ return m_playlist_duration; }
    inline long int     is_24hours_completed()				{ return m_playlist_duration >= m_24hours; }

    inline long int     get_general_duration()				{ return m_24hours; }
    inline void		set_general_duration(char* str)			{ m_24hours = str == NULL ? HOUR_INTERVAL*DEFAULT_DAY : get_seconds(str); }

    void		set_default_track_duration(char *str)           { if (str) m_default_track_duration = get_seconds(str);	}

    int get_current_hour()
    {
	time_t now;
	struct tm t;

	time(&now);
	t = *localtime(&now);

	return t.tm_hour;
    }

    long int define_playlist_duration(int current_hour)
    {
	long int value = this->seconds_between_times(current_hour);
	this->remained_interval(value);

	value = this->seconds_between_times(0);
	return this->append_playlist_duration(value);
    }		

private:

    long int			m_remained_interval;
    long int 			m_playlist_duration;				/* длительность плейлиста в секундах */
    long int 			m_24hours;
    long int 			m_default_track_duration;
    char			*s_time;

    long int remained_interval(int m_time)			{ return m_remained_interval -= m_time; } 
    long int append_playlist_duration(int m_sec)		{ return m_playlist_duration += m_sec;}

    long int seconds_between_times(int  m_hour)
    {
	/* определим, сколько секунд уже прошло с начала интервала ротации */
	time_t now;
	struct tm rem_time, m_now;

	time(&now);

	rem_time = *localtime(&now);
	rem_time.tm_hour = m_hour;
	rem_time.tm_min  = 0;
	rem_time.tm_sec  = 0;

	double seconds = difftime(now, mktime(&rem_time));

	return (long int) seconds;
    }

    long int 			get_seconds(char* stime)
    {
    	long int value = 0;
    	if (stime)
        {
        	char *s = strdup(stime);

        	value = atoi(s)*3600; /* hours */

        	char *s1 = strchr(s, ':')+1;

        	if (s1)
        	{
        		value += atoi(s1)*60;     /* minuts */

        		s1 = strchr(s1, ':')+1;
        		if (s1) value += atoi(s1);
        	}
        	free(s);
        }
    	if (!value) value = m_default_track_duration;
        return value;
    }

    char  *convert_seconds_to_char(long int t)
    {
    	if (s_time) { delete [] s_time; s_time = NULL; }

    	s_time = (char*)malloc(9); /* 00:00:00 +1 */
    	sprintf(s_time, "%02li:%02li:%02li", t/3600, (t/60) % 60, t % 60);

    	return s_time;
    }

};

} /* namespace rotator */
#endif /* OBJ_TIMER_H_ */
