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

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "ini.h"
#include "config.h"

#define	CFG_FILE			"/etc/mcserverd.conf"
#define	DEFAULT_USERNAME	"admin"
#define	DEFAULT_PASSWD		"admin"

static	char*				mcserver_cmd_line = NULL;
static	unsigned short		port = 2048;
static	char*				username = NULL;
static	char*				passwd = NULL;
static	int					max_connect = 5;
static	pini_file_info_t	p_cfg_file = NULL;


bool cfg_init()
{
	size_t len;
	char* buf;
	size_t buf_len;

	//Test if the file exists
	if(access(CFG_FILE, R_OK | W_OK) != 0) {
		return false;
	}

	//Open config file
	p_cfg_file = ini_open(CFG_FILE);

	if(p_cfg_file == NULL) {
		return false;
	}

	//Get config files
	//Minecraft config
	//Sever command line
	len = ini_get_key_value(p_cfg_file, "minecraft", "command", NULL, 0);

	if(len == 0) {
		ini_close(p_cfg_file);
		return false;
	}

	mcserver_cmd_line = malloc(len);
	ini_get_key_value(p_cfg_file, "minecraft", "command",
	                  mcserver_cmd_line, len);

	//SSH
	buf_len = 64;
	buf = malloc(buf_len);

	//Port
	len = ini_get_key_value(p_cfg_file, "ssh", "port",
	                        buf, buf_len);

	if(len != 0) {
		if(len > buf_len) {
			free(buf);
			buf_len = len;
			buf = malloc(buf_len);
			ini_get_key_value(p_cfg_file, "ssh", "port",
			                  buf, buf_len);
		}

		port = (unsigned short)atoi(buf);
	}

	//Username
	len = ini_get_key_value(p_cfg_file, "ssh", "username",
	                        NULL, 0);

	if(len != 0) {
		username = malloc(len);
		ini_get_key_value(p_cfg_file, "ssh", "username",
		                  username, len);

	} else {
		username = malloc(strlen(DEFAULT_USERNAME) + 1);
		strcpy(username, DEFAULT_USERNAME);
	}

	//Password
	len = ini_get_key_value(p_cfg_file, "ssh", "password",
	                        NULL, 0);

	if(len != 0) {
		passwd = malloc(len);
		ini_get_key_value(p_cfg_file, "ssh", "password",
		                  username, len);

	} else {
		passwd = malloc(strlen(DEFAULT_PASSWD) + 1);
		strcpy(passwd, DEFAULT_PASSWD);
	}

	//Max connection
	len = ini_get_key_value(p_cfg_file, "ssh", "max_connection",
	                        buf, buf_len);

	if(len != 0) {
		if(len > buf_len) {
			free(buf);
			buf_len = len;
			buf = malloc(buf_len);
			ini_get_key_value(p_cfg_file, "ssh", "max_connection",
			                  buf, buf_len);
		}

		max_connect = atoi(buf);
	}

	free(buf);

	return true;
}

void cfg_destroy()
{
	if(p_cfg_file != NULL) {
		ini_close(p_cfg_file);
		free(mcserver_cmd_line);
		free(username);
		free(passwd);
	}

	return;
}

bool cfg_write()
{
	return ini_sync(p_cfg_file);
}

size_t				cfg_get_mcserver_cmd_line(char* buf, size_t buf_size);
unsigned short		cfg_get_port();
void				cfg_set_port();
size_t				cfg_get_username(char* buf, size_t buf_size);
void				cfg_set_username(char* username);
size_t				cfg_get_passwd(char* buf, size_t buf_size);
void				cfg_set_passwd(char* passwd);
int					cfg_get_max_connect();
void				cfg_set_max_connect(int num);
