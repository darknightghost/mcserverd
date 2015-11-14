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

/*
An sample of config file

[minecraft]
workdir=/
command=java -Xms1024M -Xmx1024M -jar minecraft_server.jar nogui

[ssh]
port=2048
username=admin
password=admin
max_connection=5

[log]
log-file-size=2MB
log-file-num=10


*/

#ifndef	CONFIG_H_INCLUDE
#define	CONFIG_H_INCLUDE

#include "../common/common.h"

#define	MAX_USERNAME_LEN		15
#define	MAX_PASSWD_LEN			31

#define	CFG_OPEN_SUCCESS		0
#define	CFG_OPEN_FAILED			1
#define	CFG_OPEN_EAGAIN			2

bool				cfg_init();
void				cfg_destroy();

//Write config file
bool				cfg_write();

//Server command line
size_t				cfg_get_mcserver_dir(char* buf, size_t buf_size);
size_t				cfg_get_mcserver_cmd_line(char* buf, size_t buf_size);

//SSH port
unsigned short		cfg_get_port();
void				cfg_set_port(unsigned short new_port);

//Login username
size_t				cfg_get_username(char* buf, size_t buf_size);
void				cfg_set_username(char* new_usrname);

//SSH password
size_t				cfg_get_passwd(char* buf, size_t buf_size);
void				cfg_set_passwd(char* new_passwd);

//Maxium number of login users
int					cfg_get_max_connect();
void				cfg_set_max_connect(int num);

size_t				cfg_get_log_file_size();

int					cfg_get_log_file_num();

#endif	//!	CONFIG_H_INCLUDE
