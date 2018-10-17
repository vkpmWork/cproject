/*
 * msg_error.cpp
 *
 *  Created on: 29.03.2016
 *      Author: irina
 */

#include "msg_error.h"

namespace msg_error
{
	std::string get_error(int value)
	{
		if (!value) return "";

		std::string str;
		switch(value)
		{
			case ERR_PLAYLIST_SZ	: 	str = "Playlist is Empty";				break;
			case ERR_OBJ_CREATE		: 	str = "Error fmt_Create(Buf_size)";		break;
			case ERR_SOCKET_CREATE	:   str = "Error Socket Create";			break;
		}
		if (!str.empty()) str.append("\n");

		return str;
	}

} /* namespace */


