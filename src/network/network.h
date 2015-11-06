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

#ifndef	NETWORK_H_INCLUDE
#define	NETWORK_H_INCLUDE

#include "../common/common.h"

#define	NETWORK_READY		0
#define	NETWORK_NOTREADY	1
#define	NETWORK_RUNNING		2

void			network_init();
bool			network_start();
void			network_stop();
void			network_destroy();
unsigned int	network_status();

#endif	//!	NETWORK_H_INCLUDE
