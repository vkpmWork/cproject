#include "tlogmsg.h"
#include <algorithm>

TLogMsg::TLogMsg(common::tdata_map dm, string m_str):
          m_ErrorHeadersValue(0)
        , m_Event(msgevent::evEmpty)
		, DM(dm)
		, m_string(m_str)
{
    if (DM.empty())
    {	m_ErrorHeadersValue =  ERROR_EMPTY_STRING;
    	return;
    }

    if (!DM.count("data_type"))
    {	m_ErrorHeadersValue =  ERROR_DATA_TYPE;
    	return;
    }

    string s = DM["data_type"];

    if (s.compare("logger") == 0) m_ErrorHeadersValue = CheckLoggerData(dm);
}

int	TLogMsg::CheckLoggerData(common::tdata_map dm)
{
	if ((dm.count(HEADER_MESSAGE) == 0)  && (dm.count(HEADER_COMMAND) == 0) ) return ERROR_EMPTY_MESSAGE|ERROR_EMPTY_COMMAND;

    ushort eHeadersValue = OK;
    if (dm.count(HEADER_MESSAGE))
    {
        if (dm.count(HEADER_DOMAIN)   == 0) eHeadersValue  = (int)ERROR_EMPTY_DOMAIN;
        if (dm.count(HEADER_FILENAME) == 0) eHeadersValue |= (int)ERROR_EMPTY_FILENAME;
    }
    return eHeadersValue;
}

void    TLogMsg::ParseData()
{
    m_domain   = DM.count(HEADER_DOMAIN)   ? DM.find(HEADER_DOMAIN)->  second : "";
    m_cmd      = DM.count(HEADER_COMMAND)  ? DM.find(HEADER_COMMAND)-> second : "";
    m_filename = DM.count(HEADER_FILENAME) ? DM.find(HEADER_FILENAME)->second : "";
    m_msg      = DM.count(HEADER_MESSAGE)  ? DM.find(HEADER_MESSAGE)-> second : "";
    m_error    = DM.count(HEADER_ERROR  )  ? DM.find(HEADER_ERROR)->   second : "";
    m_ierror   = m_error.empty() ? 0 : atoi(m_error.c_str());

    m_ErrorHeadersValue = GetHeadersValue();

    SetEvent(m_cmd);
}
/*
string   TLogMsg::filename()
{
    size_t vLastDelimiter = m_filename.find_last_of(DELIMITER);
    string str = vLastDelimiter == std::string::npos ? m_filename : m_filename.substr(vLastDelimiter+1);
    return str;
}
*/

string   TLogMsg::msg()
{
    string s = m_msg;
    s.append(MARKER_END);
    return s;
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
