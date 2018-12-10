/*
 * config.h
 *
 *  Created on: 29.01.2016
 *      Author: irina
 */

#ifndef CONFIG_H_
#define CONFIG_H_

#include "common.h"
#include <map>
#include <vector>
#include <stdint.h>

#define KEY_LOCAL_STORE            "LOCAL"
#define KEY_REMOTE_STORE           "REMOTE"
#define KEY_COMMON_GROUP           "COMMON"

#define DEFAULT_RESET_ERROR_TIMEOUT       10
#define DEFAULT_EMAIL_ERROR_TIMEOUT       30
#define DEFAULT_EMAIL_ERROR_VOLUME        20

typedef struct TLoggerSettings
{
    ulong   port;
    string  Address;
} LoggerSettings;

typedef std::vector<string>m_emails;

class tconfig
{
public:
	Logger_namespace::tcStore CurrentStoreType;
    bool	ConfigStatus;

    tconfig(char*);
	virtual ~tconfig();

	bool    get_cfg_error()		{ return (bool)cfg_error; }
	char*	get_config_file();

    string  ConfigFile;
    tconfig(string);

    LoggerSettings  STORE_LOCAL() {return GetStore(KEY_LOCAL_STORE);}
    LoggerSettings  STORE_REMOTE(){return GetStore(KEY_REMOTE_STORE);}

    bool            ExistsSTORE_LOCAL()    {return IsStoreExists(KEY_LOCAL_STORE);}
    bool            ExistsIs_STORE_REMOTE(){return IsStoreExists(KEY_REMOTE_STORE);}

    static  string  settingsPath();

    ulong   LocalPort();
    string  LocalAddress();

    ulong   RemotePort();
    string  RemoteAddress();

    uint    CheckPeriod();
    string  Folder();
    ulong   max_list_size();
    uint    retry_interval();
    size_t  max_size();
    size_t  max_local_size();
    string  base_file_name()                {return m_base_file_name;}
    string  get_logfile();
    uint    max_archive_count();
    common::tcLogLevel get_loglevel();
    ulong   close_descriptors_period();
    int     get_logsize();
    string  owner_user_local();
    string  owner_group_local();
    mode_t  mode_dir_local();
    mode_t  mode_file_local();

    /* Регистрация ошибок */
    uint    error_counter()                 { return m_error_counter;         } /* счетчик ошибок, превышающих заданный уровень        */
    uint    registered_error_level()        { return m_registered_error_level;} /* уровень ошибки, с которой начинается регистрация    */

    double  reset_error_timeout()           { /* timeout для сброса счетчика ошибок, в секундах    */
                                                if (!m_reset_error_timeout) m_reset_error_timeout = DEFAULT_RESET_ERROR_TIMEOUT;
                                                return m_reset_error_timeout*60.0;
                                            }

    double  email_error_timeout()           {   /* timeout для повторной передачи почты, в секундах  */
                                                if (!m_email_error_timeout) m_email_error_timeout = DEFAULT_EMAIL_ERROR_TIMEOUT;
                                                return m_email_error_timeout*60.0;
                                            }
    bool     IsNeedToCheckErrors();          /* необходимо регистрировать ошибки, все параметры заданы */
    m_emails get_error_emails()             {return m_emails_error;	}
    uint     email_volume()                 {return m_email_volume;	}
//  ===================

    bool	 isdaemon()						{return is_daemon;		}
    std::string get_header()				{return m_header;		}

private:
	    char* cfg_file;
	    int   cfg_error;

        map<string, LoggerSettings> Category_Store;
	    map<string, LoggerSettings>::iterator it;

	    ulong       m_max_list_size;        /* максимальное кол-во сообщений в очереди приема */
	    string      m_base_file_name;       /* имя внутреннего Log-файла */
	    string      m_logfile;			    /* путь ко внутреннему Log-файлу */
	    string      m_folder;               /* директория для записы Log-файлов*/
	    size_t      m_max_file_size,        /* максимальный размер одного Log-файла */
	                m_max_local_size;       /* максимальный размер LocalLog.log (при потере связи с удаленным сервером) */
	    uint        m_max_archive_count;    /* кол-во архивных файлов одного типа */
	    uint        m_checkperiod;          /* in seconds периодичность выдачи сигнала на передачу/запись полученных сообщений */
	    common::tcLogLevel  m_loglevel;             /* уровень логирования - для работы с объектами */
	    uint        m_close_descriptors_period; /* время проверки открытых файловых дискрипторов в  минутах*/
	    uint32_t      m_retry_interval;       /* периодичность попыток связи с удаленным сервером при разрыве связи */
	    int	        m_logsize;		        /* признак ограничения длинны внутреннего Log-файла */
	    string      m_owner_law;            /* права владельца локально создаваемых лог-файлов */

	    string      m_owner_user_local;     /* владелец локально создаваемых лог-файлов */
	    string      m_owner_group_local;    /* группа локально создаваемых лог-файлов */

	    string      m_mode_dir_local;        /* права локально создаваемых директорий */
	    string      m_mode_file_local;       /* права локально создаваемых файлов */

	    /* Регистрация ошибок */
	    uint        m_error_counter;         /* счетчик ошибок, превышающих заданный уровень     */
	    uint        m_registered_error_level;/* уровень ошибки, с которой начинается регистрация */
	    uint        m_reset_error_timeout;   /* timeout для сброса счетчика ошибок, в минутах    */
	    uint        m_email_error_timeout;   /* timeout для повторной передачи почты, в минутах    */
	    m_emails    m_emails_error;          /* перечень адресов для уведомлений о превышении кол-ва ошибок */
	    uint        m_email_volume;          /* объем критических сообщений для одного домена приотпрапвке по почте */

	    std::string m_header;
	    bool        is_daemon;

	void				ReadIni();

    LoggerSettings		GetStore(string);
    bool        		IsStoreExists(string);
    string      		FileExists(string);
    common::tcLogLevel  SetLogLevel(string);
    void        		GetSetupInfo();
    m_emails    		emails_error(string);
    bool        		is_valid_email(string);

};

extern tconfig *pConfig;
extern bool InitLogSystemSetup(string);
extern void DeleteLogSystemSetup();

#endif /* CONFIG_H_ */
