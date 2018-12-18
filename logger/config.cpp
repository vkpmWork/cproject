/*
 * config.cpp
 *
 *  Created on: 29.01.2016
 *      Author: irina
 */

#include "config.h"
#include "INIReader.h"
#include "base64.h"
#include <string.h>

const int         DEFAULT_LOGSIZE = 10;

#define DEFAULT_FOLDER  "./"
#define DEFAULT_BASE_FILE_NAME     "LogFile.log"

#define DEFAULT_PORT               3333
#define DEFAULT_CHECK_PERIOD       5
#define DEFAULT_MAX_MSG_PER_SECOND 0
#define DEFAULT_MAX_LIST_SIZE      500
#define DEFAULT_BUFFER_SEND_RATE   1
#define DEFAULT_RETRY_INTERVAL     2
#define DEFAULT_SOCKET_TIMEOUT_MS  2
#define DEFAULT_MAX_SIZE           200000
#define DEFAULT_LOCAL_SIZE         3000000000
#define DEFAULT_MAX_WRITE_SIZE     100000
#define DEFAULT_CONF_FILE_LOCATION "./"
#define DEFAULT_MAX_ARCHIVE_COUNT  10        /* кол-во "архивных" файлов одного типа */
#define DEFAULT_LOGLEVEL           levError
#define DEFAULT_CLOSE_DESCRIPTORS_PERIOD  60 /* период в минутах закрытия всех открытых файловых дескрипторов*/
#define DEFAULT_LOCALLOG_LIMIT     false
#define DEFAULT_CLEAR_INTERNALLOG  false
#define DEFAULT_ERROR_COUNTER      0
#define DEFAULT_REGISTERED_ERROR_LEVEL    0
#define DEFAULT_RESET_ERROR_TIMEOUT       10
#define DEFAULT_EMAIL_ERROR_TIMEOUT       30
#define DEFAULT_EMAIL_ERROR_VOLUME        20


#define REGEX_MASK  "[.\\-_a-z0-9]+@([a-z0-9][\\-a-z0-9]+\\.)+[a-z]{2,6}"

tconfig *pConfig = NULL;

bool InitLogSystemSetup(string fconfig)
{
	pConfig = new tconfig((char*)fconfig.c_str());

    if (pConfig && pConfig->ConfigStatus == Logger_namespace::EMPTY_STORE)
    {
    	DeleteLogSystemSetup();
    	std::cout << "No type of store found: not LOCAL STORE & not REMOTE STORE" << endl;
    }

    return pConfig ? pConfig->ConfigStatus : false;
}

void DeleteLogSystemSetup()
{
    if (pConfig)
        try
        {  delete pConfig;
           pConfig = NULL;
        }
        catch(...) {}
}

/* -------------------------------------------------------- */
tconfig::tconfig(char *c_file)
		: cfg_file(NULL)
		, cfg_error(0)
        , is_daemon(false)
{
	if (c_file)
	{
		cfg_file = strdup(c_file);
		ReadIni();
	}
	else cfg_error = 1;
}

tconfig::~tconfig()
{
	free(cfg_file);
}


void tconfig::ReadIni()
{
	INIReader reader(cfg_file);

	int v = reader.ParseError();
	if (v)
	{
		cfg_error = 1;
		std::cout << "Something Wrong with Configuration File. Error: " << v << endl;
    	return;
	}

	    LoggerSettings ls;

	    /* LOCAL */
	    ls.port             = reader.GetInteger(KEY_LOCAL_STORE, "port",   0);
	    ls.Address          = reader.Get(KEY_LOCAL_STORE, "address","");
	    m_max_file_size     = reader.GetInteger(KEY_LOCAL_STORE, "max_file_size",     DEFAULT_MAX_SIZE);
	    m_max_archive_count = reader.GetInteger(KEY_LOCAL_STORE, "max_archive_count", DEFAULT_MAX_ARCHIVE_COUNT);

	    /* Регистрация ошибок */
	    m_error_counter             = reader.GetInteger(KEY_LOCAL_STORE,"error_counter", DEFAULT_ERROR_COUNTER);
	    m_registered_error_level    = reader.GetInteger(KEY_LOCAL_STORE, "registered_error_level",  DEFAULT_REGISTERED_ERROR_LEVEL);

	    m_reset_error_timeout       = reader.GetInteger(KEY_LOCAL_STORE, "reset_error_timeout",     DEFAULT_RESET_ERROR_TIMEOUT);
	    m_email_error_timeout       = reader.GetInteger(KEY_LOCAL_STORE, "email_error_timeout",     DEFAULT_EMAIL_ERROR_TIMEOUT);

	    std:string sss = reader.Get(KEY_LOCAL_STORE, "emails_error", "");
	    m_emails_error  = emails_error(sss);
	    if (m_emails_error.empty() || m_emails_error.size()==0)   m_registered_error_level = m_error_counter = 0;

	    m_email_volume  = reader.GetInteger(KEY_LOCAL_STORE, "error_volume", DEFAULT_EMAIL_ERROR_VOLUME);
	    if (m_email_volume > DEFAULT_EMAIL_ERROR_VOLUME) m_email_volume = DEFAULT_EMAIL_ERROR_VOLUME;

	    if ((ls.port == 0) || ls.Address.empty())
	    {
	        std::cout << "Error configuration.  Section LOCAL: port = " << ls.port << "; address = " << ls.Address << endl;
	        cfg_error = 1;
	        return;
	    }
	    Category_Store[KEY_LOCAL_STORE] = ls;

	    /* KEY_REMOTE_STORE */
	    if (reader.SectionExists(KEY_REMOTE_STORE))
	    {
				ls.port             = reader.GetInteger(KEY_REMOTE_STORE, "port", 0);
				ls.Address          = reader.Get(KEY_REMOTE_STORE,"address",        "");
				m_retry_interval    = reader.GetInteger(KEY_REMOTE_STORE, "retry_interval", DEFAULT_RETRY_INTERVAL);
				m_max_local_size    = reader.GetInteger(KEY_REMOTE_STORE, "max_local_size", DEFAULT_LOCAL_SIZE);

				if (ls.port && ls.Address.empty() == false) Category_Store[KEY_REMOTE_STORE] = ls;
				else
				{
						std::cout << "Error configuration.  Section REMOTE:  port = " << ls.port << "; address = " << ls.Address << endl;
						cfg_error = 1;
				}


	    }
            /* KEY_COMMON_GROUP */
        is_daemon           = reader.GetBoolean(KEY_COMMON_GROUP, "daemon", false);
	    m_folder            = reader.Get(KEY_COMMON_GROUP, "folder",  DEFAULT_FOLDER);
	    m_logfile			= reader.Get(KEY_COMMON_GROUP, "logfile",   "");

	    m_max_list_size     = reader.GetInteger(KEY_COMMON_GROUP, "max_list_size",       DEFAULT_MAX_LIST_SIZE);
	    m_checkperiod       = reader.GetInteger(KEY_COMMON_GROUP, "check_period",        DEFAULT_CHECK_PERIOD);
	    m_loglevel          = (common::tcLogLevel)reader.GetInteger(KEY_COMMON_GROUP, "loglevel",  common::levError);
	    m_close_descriptors_period = reader.GetInteger(KEY_COMMON_GROUP, "check_descriptors_period", DEFAULT_CLOSE_DESCRIPTORS_PERIOD);
	    m_logsize 		    = reader.GetBoolean(KEY_COMMON_GROUP, "logsize",  DEFAULT_LOCALLOG_LIMIT);
	    m_owner_user_local  = reader.Get(KEY_COMMON_GROUP, "owner_user", "");
	    m_owner_group_local = reader.Get(KEY_COMMON_GROUP, "owner_group", "");

	    m_mode_dir_local    = reader.Get(KEY_COMMON_GROUP, "mode_dir",       DEFAULT_DIR_MODE );
	    m_mode_file_local   = reader.Get(KEY_COMMON_GROUP, "mode_file",      DEFAULT_FILE_MODE);

	    Logger_namespace::tcStore store =  IsStoreExists(KEY_REMOTE_STORE) == true ? Logger_namespace::REMOTE_STORE : Logger_namespace::LOCAL_STORE;
	    if (CurrentStoreType != Logger_namespace::EMPTY_STORE && CurrentStoreType != store)
	    {
	        // !!!! if (pInternalLog) pInternalLog->LOG_OPER("Невозможно изменить тип системы (Category_Store)!");
	        ConfigStatus = (CurrentStoreType != Logger_namespace::EMPTY_STORE);
	        return;
	    }

	    CurrentStoreType = store;
	    ConfigStatus     = (CurrentStoreType != Logger_namespace::EMPTY_STORE);

//	    GetSetupInfo();


/*	ostringstream str;
	str << "INI FILE   : " << cfg_file << endl;
	str << "SERVER     : host = " << host << "  port = " << port << endl;
//        str << "REJIME     : " << ( isLoader() ?  SSD_REJIME : PROXY_REJIME ) << endl;
        str << "DOMAIN     :"  << endl;
        for (tpath_media::iterator it = m_pathmedia.begin(); it != m_pathmedia.end(); it++)
          {
            str << (*it).first << "="  << (*it).second << endl;
          }

        if (isProxy())
          str << "PROXY    : proxy host =" << host << "  proxy port =" << port << endl;

	str << "LOG FILE   : name=" << logfile << "  logsize=" << logsize << "  loglevel=" << loglevel << endl;
        str << "DAEMON     : " <<  (is_daemon ? "Yes" : "No") << endl;
        str << "PID file   : " <<  common::pidfile_name << endl;
        str << "PID        : " <<  getpid() << endl << endl;

	m_header.append(str.str());
	cout << m_header;
*/
    m_header = "Hello World\n";
}

bool tconfig::IsStoreExists(string storetype)
{
    return (bool)Category_Store.count(storetype);
}


LoggerSettings tconfig::GetStore(string storetype)
{
    LoggerSettings s;
    s.port   = 0;
    s.Address = "";

    it = Category_Store.find(storetype);
    if (it != Category_Store.end()) s = (*it).second ;
    return s;
}

string tconfig::settingsPath()
{
	return "./";
	//  return QFileInfo(s.fileName()).absolutePath();
}

ulong   tconfig::LocalPort()
{
    return  STORE_LOCAL().port;
}

ulong   tconfig::RemotePort()
{
    return  STORE_REMOTE().port;
}

string  tconfig::LocalAddress()
{
    return  STORE_LOCAL().Address;
}

string  tconfig::RemoteAddress()
{
    return  STORE_REMOTE().Address;
}

uint tconfig::CheckPeriod()
{
    return  m_checkperiod*1000;
}

string  tconfig::Folder()
{
    if (m_folder[0] != DELIMITER && m_folder[0] != '.') m_folder =  DELIMITER + m_folder;
    if (m_folder[m_folder.size()-1] != DELIMITER) m_folder += DELIMITER;
    return  m_folder;
}

ulong   tconfig::max_list_size()
{
    return (ulong)m_max_list_size;
}

uint    tconfig::retry_interval()
{
    return m_retry_interval*1000;
}

size_t tconfig::max_size()
{
    return m_max_file_size;
}

size_t tconfig::max_local_size()
{
    return m_max_local_size;
}

uint tconfig::max_archive_count()
{
    return m_max_archive_count > 0 ? m_max_archive_count : DEFAULT_MAX_ARCHIVE_COUNT;
}

string tconfig::FileExists(string str)
{
    char *s = (char*)str.c_str();
	if (common::FileExists(s) == false)
    {
         str.insert(0, "Errort : Cohfiguration file is absent or is damaged!");
        // if (pInternalLog) pInternalLog->LOG_OPER(str.c_str());
         str.clear();
    }
    return  str;
}

string  tconfig::get_logfile()
{
    return m_logfile;
}

common::tcLogLevel tconfig::get_loglevel()
{
	if (m_loglevel > common::levError) m_loglevel = common::levError;
	return m_loglevel;
}

ulong tconfig::close_descriptors_period()
{
    return (ulong)m_close_descriptors_period*60*1000;
}

int tconfig::get_logsize()
{
    return m_logsize;
}

string tconfig::owner_user_local()
{
    return m_owner_user_local;
}

string tconfig::owner_group_local()
{
    return m_owner_group_local;
}

mode_t tconfig::mode_dir_local()
{

    return (mode_t) common::ConvertToDecFromOct(m_mode_dir_local);
}

mode_t tconfig::mode_file_local()
{
    return (mode_t) common::ConvertToDecFromOct(m_mode_file_local);
}

bool   tconfig::IsNeedToCheckErrors()          /* необходимо регистрировать ошибки, все параметры заданы */
{
    return (m_error_counter > 0 && m_registered_error_level > 0 && (m_emails_error.empty() == false));
}

m_emails tconfig::emails_error(string m_mails)
{
    m_emails emails;
    string   s;
    int      s_len = m_mails.length();
    char    *ss = (char*) malloc(255);

    if (!m_mails.empty())
    {
        size_t pos = 0, len = 0;

        while (true)
        {

            len = m_mails.find_first_of(' ', pos);
            if (len == string::npos) len = s_len;

            s = m_mails.substr(pos, len-pos);
            if(!s.empty())
            {
                if (is_valid_email(s)) emails.push_back(s);
                else
                {
                    sprintf(ss, "Error email address format: %s\n", s.c_str());
                    std::cout << ss;
                }

            }
            pos = len+1;
            if ((int)pos >= s_len) break;

        }
    }

    if (ss) free(ss);
    return emails;
}

bool tconfig::is_valid_email(string value)
{

    return true;
	//return m_validMailRegExp.QRegExp::exactMatch(QString::fromStdString(value));

/*    if (value.empty()) return false;

    boost::regex re(REGEX_MASK);
    return boost::regex_match(value,re);
*/
}

