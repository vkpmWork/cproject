/*
 * msg_error.h
 *
 *  Created on: 29.03.2016
 *      Author: irina
 */

#ifndef MSG_ERROR_H_
#define MSG_ERROR_H_

#include <string>

namespace msg_error
{

#define  ERR_PLAYLIST_SZ	1
#define  ERR_OBJ_CREATE		3
#define  ERR_SOCKET_CREATE	4


	std::string get_error(int);
}


#endif /* MSG_ERROR_H_ */
