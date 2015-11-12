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
#include <stdio.h>

#define		INI_LINE_SECTION	0
#define		INI_LINE_KEY		1
#define		INI_LINE_COMMENT	2
#define		INI_LINE_BLANK		3

typedef	struct	_ini_line {
	int	type;
} ini_line_t, *pini_line_t;

typedef	struct	_ini_key {
	ini_line_t	line;
	char*		key_name;
	char*		value;
} ini_key_t, *pini_key_t;

typedef	struct	_ini_section {
	ini_line_t	line;
	char*		section_name;
	list_t		keys;
} ini_section_t, *pini_section_t;

typedef	struct	_ini_comment {
	ini_line_t	line;
	char*		comment;
} ini_comment_t, *pini_comment_t;

typedef struct	_ini_file_info {
	char*	path;
	list_t	sections;
	FILE*	fp;
} ini_file_info_t, *pini_file_info_t;

//Load file data
pini_file_info_t		ini_open(char* path);

//Return the size of value.If the size > buf_size,
//you should get the value wth a bigger buf again.
//If the key does not exists,the key wiil be create.
size_t				ini_get_key_value(pini_file_info_t p_file,
                                      char* section,
                                      char* key,
                                      char* buf,
                                      size_t buf_size);

//If value == NULL,the key will be removed
void				ini_set_key_value(pini_file_info_t p_file,
                                      char* section,
                                      char* key,
                                      char* value);

//Wrte ini file
bool				ini_sync(pini_file_info_t p_file);

//Free file data
void				ini_close(pini_file_info_t p_file);

#endif	//!	INI_H_INCLUDE
