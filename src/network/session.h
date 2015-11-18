/*
	Copyright 2015,暗夜幽灵 <darknightghost.cn@gmail.com>

	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SESSION_H_INCLODE
#define	SESSION_H_INCLUDE

#include <pthread.h>

#include "../common/common.h"
#include "ssh.h"

#define	SESSION_ACTIVE		0
#define	SESSION_WAITING		1
#define	SESSION_EXIT		2

typedef struct _session_t {
	ssh_session								session;
	ssh_event								event;
	ssh_channel								channel;
	int										status;
	int										tried_time;
	bool									authed;
	pthread_t								thread;
	pthread_t								send_thread;
	struct ssh_server_callbacks_struct		server_callbacks;
	struct ssh_channel_callbacks_struct		channel_callbacks;
} session_t, *psession_t;

psession_t	session_new();
void		session_free(psession_t p_session);


#endif	//!	SESSION_H_INCLODE
