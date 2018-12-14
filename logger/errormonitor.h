#ifndef ERRORMONITOR_H
#define ERRORMONITOR_H

#include "common.h"
#include <map>

struct tDomInfo
{
    int         m_counter;
    time_t      tm;
    bool        m_overflow;
    bool        m_first_owerflow;
    string      last_msg;
    int         msg_box_size;
    common::vmsg_box    msg_box;
} typedef TDomInfo;

typedef map<string, TDomInfo> map_domain;

class TErrorMonitor
{

//signals:
//    void     send_email (vmsg_box);
public:
    explicit TErrorMonitor(int, int, double, double, uint);
    ~TErrorMonitor();

    void     onAdd_error(int, char*, char*);
private:

    struct
    {
        bool        IsNeedToCheckError;
        int         m_error_counter;         /* счетчик ошибок, превышающих заданный уровень     */
        int         m_registered_error_level;/* уровень ошибки, с которой начинается регистрация */
        double      m_reset_error_timeout;   /* timeout для сброса счетчика ошибок, в минутах    */
        double      m_email_error_timeout;   /* timeout для повторной передачи почты, в минутах    */
        int         m_email_volume;          /* объем передаваемых сообщений*/
    } ErrMonitor;

    map_domain  domain;

    inline bool counter_overflow(int);
    inline bool counter_overflow(bool, int&);
    inline bool exceeded_email_timeout(time_t, bool);
    inline bool exceeded_email_timeout(TDomInfo&);
    inline void set_mutex(bool);

//private slots  :
//        void     exceeded_reset_timeout();
//public  slots  :
};

extern TErrorMonitor *pErrorMonitor;
extern void create_error_monitor_thread();
extern pthread_t error_thread;

#endif // ERRORMONITOR_H
