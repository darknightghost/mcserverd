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

#ifndef COMMON_H_INCLUDE
#define	COMMON_H_INCLUDE

#define	UNREFERRED_PARAMETER(x)		((void)(x))

#ifndef	bool

	typedef	int		bool;
	#define	true	1
	#define	false	0

#endif	//!	bool

#include "list.h"
#include "queue.h"

#endif	//!	COMMON_H_INCLUDE
