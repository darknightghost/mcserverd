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

#include <stdlib.h>

#include "session.h"

psession_t session_new()
{
	psession_t p_session;

	p_session = malloc(sizeof(session_t));
	p_session->session = ssh_new();
	p_session->event = ssh_event_new();
	p_session->channel = NULL;
	p_session->status = SESSION_WAITING;
	p_session->tried_time = 0;
	p_session->authed = false;

	return p_session;
}

void session_free(psession_t p_session)
{
	if(p_session->channel != NULL) {
		ssh_channel_free(p_session->channel);
	}

	ssh_event_free(p_session->event);
	ssh_free(p_session->session);
	free(p_session);

	return;
}
