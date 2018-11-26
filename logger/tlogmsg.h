#ifndef TLOGMSG_H
#define TLOGMSG_H
#include "common.h"
//#include <queue>
#include <list>

#define HEADER_DOMAIN   "<domain>"
#define HEADER_FILENAME "<filename>"
#define HEADER_MESSAGE  "<message>"
#define HEADER_COMMAND  "<command>"
#define HEADER_ERROR    "<error>"


//typedef deque<string> v_messagelist;

namespace msgevent
{
    enum tcEvent { evEmpty = 0, evMsg, evDelete, evConfig, evExit};
}

enum tcErrorHeadersValue
{
    OK = 0,                    /* */
    ERROR_EMPTY_DOMAIN          = 0x01, /*отсутствует или пустой домен */
    ERROR_EMPTY_FILENAME        = 0x02, /*отсутствует или пустое имя файла */
    ERROR_EMPTY_MESSAGE         = 0x04, /*отсутствует или пустое сообщение при наличии заголовка message*/
    ERROR_EMPTY_COMMAND         = 0x08, /*отсутствует или пустая команда при наличии заголовка command  */
    ERROR_EMPTY_STRING          = ERROR_EMPTY_DOMAIN|ERROR_EMPTY_FILENAME|ERROR_EMPTY_MESSAGE|ERROR_EMPTY_COMMAND,
    ERROR_LENGTH_MESSAGE        = 0x10, /* ошибка приема сообщения по длине */
    ERROR_READ_SOCKET           = 0xFF, /* таймфаут при чтении данных из сокета */
    ERROR_HEADER_FORMAT         = ERROR_EMPTY_DOMAIN |ERROR_EMPTY_FILENAME
};

class TLogMsg
{
public:
    void   Message(string);
    string Message();
    string domain();
    string filename();
    string fullfilename();
    string msg();
    string cmd();
    int    error();
    string GlobalFileName();
    string FilePath();
    void   SetEvent(string);
    size_t Size();
    inline msgevent::tcEvent Event() { return m_Event;}

    unsigned short ErrorHeadersValue() { return m_ErrorHeadersValue; }

    TLogMsg():m_ErrorHeadersValue(0), m_Event(msgevent::evEmpty) {}
    TLogMsg(string str);
    bool operator < (const TLogMsg &param)const
    {
        if (strcmp(m_domain.c_str(),  param.m_domain.c_str())   < 0) return true;
        if (strcmp(m_domain.c_str(),  param.m_domain.c_str())   >=0) return false;
        if (strcmp(m_filename.c_str(),param.m_filename.c_str()) < 0) return true;
        return false;
    }

    bool operator != (const TLogMsg &param) const
    {
        if (strcmp(m_domain.c_str(),  param.m_domain.c_str())   != 0) return true;
        if (strcmp(m_filename.c_str(),param.m_filename.c_str()) != 0) return true;
        return false;
    }

private:
    string  GetHeader(char* str, string KeyHeader);
    string  m_domain,
            m_filename,
            m_cmd,
            m_msg,
            m_error,
            m_string;
    int     m_ierror;

    size_t  m_size;
    unsigned short m_ErrorHeadersValue;
    msgevent::tcEvent   m_Event;
    unsigned short GetHeadersValue();
    void           ParseString(string);
};

typedef std::list<TLogMsg> mList;

#endif // TLOGMSG_H
