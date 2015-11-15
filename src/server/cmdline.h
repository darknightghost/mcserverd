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

#ifndef	CMDLINE_H_INCLUDE
#define	CMDLINE_H_INCLUDE

#include "../common/common.h"
#include <stdlib.h>

bool		cmdline_init();
void		cmdline_destroy();
bool		cmdline_start();
void		cmdline_stop();
size_t		cmdline_read(char* buf, size_t buf_size);
size_t		cmdline_write(char* buf, size_t size);

#endif	//!	CMDLINE_H_INCLUDE
