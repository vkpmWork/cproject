/*
 * url.h
 *
 *  Created on: 30.06.2016
 *      Author: irina
 */

#ifndef URL_H_
#define URL_H_

#include <string>
#include <algorithm>
#include "log.h"

namespace url
{


class turl
{
private :
	struct Url
	{
		std::string QueryString, Path, Protocol, Host;
		int  Port;
	} 	url_data;

	bool	error_url;
	char	*url_string;
	char	*url_query;
	void	Init();
	void	Parse(std::string);

public  :
	turl(char*);
	turl();
	~turl();

	void	Parse();
	void    set_url_string(char*);
	bool	get_error_url();
	char   *get_host();
	int	    get_port();
	char   *get_query();
	int		get_query_size();

};

extern std::string query_url(char*);
extern void		   transmit(char* m_host, int m_port, int m_count, char *m_data, int m_data_len);

} /* namespace url */
#endif /* URL_H_ */
