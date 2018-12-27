#ifndef TLOGMSG_H
#define TLOGMSG_H
#include "common.h"

#define HEADER_DOMAIN   "domain"
#define HEADER_FILENAME "filename"
#define HEADER_MESSAGE  "message"
#define HEADER_COMMAND  "command"
#define HEADER_ERROR    "error"


enum tcErrorHeadersValue
{
    OK = 0,                    /* */
    ERROR_EMPTY_DOMAIN          = 0x01, /*отсутствует или пустой домен */
    ERROR_EMPTY_FILENAME        = 0x02, /*отсутствует или пустое имя файла */
    ERROR_EMPTY_MESSAGE         = 0x04, /*отсутствует или пустое сообщение при наличии заголовка message*/
    ERROR_EMPTY_COMMAND         = 0x08, /*отсутствует или пустая команда при наличии заголовка command  */
    ERROR_EMPTY_STRING          = ERROR_EMPTY_DOMAIN|ERROR_EMPTY_FILENAME|ERROR_EMPTY_MESSAGE|ERROR_EMPTY_COMMAND,
    ERROR_LENGTH_MESSAGE        = 0x10, /* ошибка приема сообщения по длине */
    ERROR_DATA_TYPE             = 0x20, /* отсутствует тип сообщения */
    ERROR_READ_SOCKET           = 0xFF, /* таймфаут при чтении данных из сокета */
    ERROR_HEADER_FORMAT         = ERROR_EMPTY_DOMAIN |ERROR_EMPTY_FILENAME
};

class TLogMsg
{
public:
	int	   CheckLoggerData(common::tdata_map dm);

    string Message();
    string domain()						{ return m_domain;   }
    string filename()					{ return m_filename; }
    string fullfilename()				{ return m_filename; }
    string msg();
    string cmd()						{ return m_cmd;		 }
    int    error()						{ return m_ierror;	 }
    string GlobalFileName();
    string FilePath();
    void   SetEvent(string);
    inline msgevent::tcEvent Event() 	{ return m_Event;	 }

    unsigned short ErrorHeadersValue() 	{ return m_ErrorHeadersValue; }
    void           ParseData();

    TLogMsg():m_ErrorHeadersValue(0), m_Event(msgevent::evEmpty) {}
    TLogMsg(common::tdata_map dm, string m_str);

private:

    string  m_domain,
            m_filename,
            m_cmd,
            m_msg,
            m_error;
    int     m_ierror;
    size_t  m_size;
    unsigned short 		m_ErrorHeadersValue;
    msgevent::tcEvent   m_Event;
    common::tdata_map 	DM;
    string	m_string;
    unsigned short 		GetHeadersValue();
};

#endif // TLOGMSG_H
