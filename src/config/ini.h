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

#ifndef	INI_H_INCLUDE
#define	INI_H_INCLUDE

#include "../common/common.h"

typedef	struct	_ini_key {
	char*			key_name;
	char*			value;
} ini_key, *pini_key;

typedef	struct	_ini_section {
	char*	section_name;
	list_t	keys;
} ini_section, *pini_section;

typedef struct	_ini_file_info {
	list_t	sections;
} ini_file_info, *pini_file_info;

pini_file_info		ini_open(char* path);

//Return the size of value.If the size > buf_size,
//you should get the value wth a bigger buf again.
size_t				ini_get_key_value(pini_file_info p_file,
                                      char* section,
                                      char* key,
                                      char* buf,
                                      size_t buf_size);
void				ini_close(pini_file_info p_file);

#endif	//!	INI_H_INCLUDE
