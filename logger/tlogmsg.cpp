#include "tlogmsg.h"
#include <algorithm>

TLogMsg::TLogMsg(string Str):
          m_ErrorHeadersValue(0)
        , m_Event(msgevent::evEmpty)
{
    ParseString(Str);
}

void    TLogMsg::Message(string Str)
{
    ParseString(Str);
}

void    TLogMsg::ParseString(string Str)
{
    if (Str.empty())
    {    m_ErrorHeadersValue =  ERROR_EMPTY_STRING;
         return;
    }

    char *str = strdup(Str.data());
    if (str)
    {
        std::cout << "STR: " << str << endl;

        m_domain   = GetHeader(str, HEADER_DOMAIN);
        m_cmd      = GetHeader(str, HEADER_COMMAND);
        std::cout << "CMD: " << m_cmd << endl;
        m_filename = GetHeader(str, HEADER_FILENAME);
        m_msg      = GetHeader(str, HEADER_MESSAGE);
        m_error    = GetHeader(str, HEADER_ERROR);
        m_ierror   = m_error.empty() ? 0 : atoi(m_error.c_str());

        m_ErrorHeadersValue = GetHeadersValue();
        free(str);
        str = NULL;
    }
    SetEvent(m_cmd);

    m_size   = Str.size();
    m_string = Str;
}

string    TLogMsg::GetHeader(char* str, string KeyHeader)
{
    string s = "";

    char *pHeader = strstr(str, KeyHeader.c_str());
    if (!pHeader) return s;

    pHeader = strchr(pHeader, '>');
    char *pStopInfo = KeyHeader == HEADER_MESSAGE ?  pHeader + strlen(pHeader): strchr(pHeader+1, '<');

    char *m_str = new char[strlen(pHeader)+1];
    if (m_str)
    {   memset(m_str, '\0', strlen(pHeader));

        if (pStopInfo) strncpy(m_str, pHeader+1, pStopInfo - pHeader - 1);
        else strcpy(m_str, pHeader+1);

        s = m_str;
        delete [] m_str; m_str = NULL;
    }
/*
#ifdef _DEBUG
    if (KeyHeader == HEADER_MESSAGE)
    {   time_t seconds = time(NULL);
        tm* timeinfo   = localtime(&seconds);
        s.insert(0, asctime(timeinfo));
    }
#endif
*/
    return s;
}

string   TLogMsg::domain()
{
    return m_domain;
}


string   TLogMsg::fullfilename()
{
    return m_filename;
}

string   TLogMsg::filename()
{
    size_t vLastDelimiter = m_filename.find_last_of(DELIMITER);
    string str = vLastDelimiter == std::string::npos ? m_filename : m_filename.substr(vLastDelimiter+1);
    return str;
}

string   TLogMsg::msg()
{
    string s = m_msg;
    s.append(MARKER_END);
    return s;
}

string   TLogMsg::cmd()
{
    return m_cmd;
}

int   TLogMsg::error()
{
    return m_ierror;
}


unsigned short  TLogMsg::GetHeadersValue()
{
    if (m_msg.empty() && m_cmd.empty()) return ERROR_EMPTY_MESSAGE|ERROR_EMPTY_COMMAND;

    ushort eHeadersValue = OK;
    if (m_msg.empty() == false)
    {
        if (m_domain.empty())   eHeadersValue  = (int)ERROR_EMPTY_DOMAIN;
        if (m_filename.empty()) eHeadersValue |= (int)ERROR_EMPTY_FILENAME;
    }
    return eHeadersValue;
}

string  TLogMsg::Message()
{
 /*   string str;
    str.append(HEADER_DOMAIN);
    str.append(m_domain);
    if (!m_error.empty())
    {   str.append(HEADER_ERROR);
        str.append(m_error);
    }
    str.append(HEADER_FILENAME);
    str.append(m_filename);
    if (!m_msg.empty())
    {   str.append(HEADER_MESSAGE);
        str.append(m_msg);
    }
    if (!m_cmd.empty())
    {   str.append(HEADER_COMMAND);
        str.append(m_cmd);
    }
    //str.append(MARKER_END);

    return str;
*/
 m_string.append(MARKER_END);
 return m_string;
}

string  TLogMsg::GlobalFileName()
{
   string str = FilePath();
   str.append(filename());
   return str;
}

string  TLogMsg::FilePath()
{
    string str = "";
    size_t vLastDelimiter = m_filename.find_last_of(DELIMITER);

    if (vLastDelimiter != std::string::npos)
    {
        str = m_filename.substr(0, vLastDelimiter);
        if (str[0] == DELIMITER)
        {
            size_t vFirstDelimiter = m_filename.find_first_of(DELIMITER);
            str = str.substr(vFirstDelimiter != std::string::npos ? vFirstDelimiter+1 : 0);
        }
    }
    if (!str.empty()) str += DELIMITER;

    return m_domain + DELIMITER + str;
}

void TLogMsg::SetEvent(string ev)
{
    std::transform(ev.begin(), ev.end(), ev.begin(), ::toupper);

    if (ev.empty()) m_Event = msgevent::evMsg;
    else if (ev == COMMAND_DELETE)  m_Event = msgevent::evDelete;
         else if (ev == COMMAND_RECONFIG) m_Event = msgevent::evConfig;
              else  if (ev == COMMAND_EXIT) m_Event = msgevent::evExit;
                    else m_Event = msgevent::evEmpty;
}

size_t TLogMsg::Size()
{
    return m_size;
}
