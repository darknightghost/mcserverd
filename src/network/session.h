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

#include "../common/common.h"
#include "ssh.h"

typedef struct _session_t {
	char		username[32];
	ssh_session	session;
	ssh_event	event;
	ssh_channel	channel;
	int			status;
} session_t, *psession_t;

psession_t	session_new();
void		session_free(psession_t p_session);
int			session_add(psession_t p_session);


#endif	//!	SESSION_H_INCLODE
