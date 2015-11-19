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

#ifndef	SERVER_H_INCLUDE
#define	SERVER_H_INCLUDE

#include "../common/common.h"
#include <stdlib.h>

bool		server_init();
void		server_destroy();
bool		server_start();
void		server_stop();
void		server_refresh();
size_t		server_read(char* buf, size_t buf_size);
size_t		server_write(char* buf, size_t size);

#endif	//!	SERVER_H_INCLUDE
