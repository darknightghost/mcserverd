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
#include <pthread.h>

#include "ini.h"
#include "config.h"

#define	CFG_FILE			"/etc/mcserverd.conf"

static	char*				mcserver_dir = NULL;
static	char*				mcserver_cmd_line = NULL;
static	unsigned short		port;
static	char*				username;
static	char*				passwd;
static	int					max_connect;
static	size_t				log_file_size;
static	int					log_file_num;
static	pini_file_info_t	p_cfg_file = NULL;

static	pthread_mutex_t		mutex;

static	size_t				get_size(char* buf);

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
	//Working directory
	len = ini_get_key_value(p_cfg_file, "minecraft", "workdir", NULL, 0);

	if(len == 0) {
		ini_close(p_cfg_file);
		return false;
	}

	mcserver_dir = malloc(len);
	ini_get_key_value(p_cfg_file, "minecraft", "workdir",
	                  mcserver_dir, len);

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

	} else {
		ini_close(p_cfg_file);
		return false;
	}

	//Username
	len = ini_get_key_value(p_cfg_file, "ssh", "username",
	                        NULL, 0);

	if(len != 0) {
		username = malloc(len);
		ini_get_key_value(p_cfg_file, "ssh", "username",
		                  username, len);

	}  else {
		ini_close(p_cfg_file);
		return false;
	}

	//Password
	len = ini_get_key_value(p_cfg_file, "ssh", "password",
	                        NULL, 0);

	if(len != 0) {
		passwd = malloc(len);
		ini_get_key_value(p_cfg_file, "ssh", "password",
		                  passwd, len);

	}  else {
		ini_close(p_cfg_file);
		return false;
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

	} else {
		ini_close(p_cfg_file);
		return false;
	}

	//Log
	//Log file size
	len = ini_get_key_value(p_cfg_file, "log", "log-file-size",
	                        buf, buf_len);

	if(len != 0) {
		if(len > buf_len) {
			free(buf);
			buf_len = len;
			buf = malloc(buf_len);
			ini_get_key_value(p_cfg_file, "log", "log-file-size",
			                  buf, buf_len);
		}

		log_file_size = get_size(buf);

	} else {
		ini_close(p_cfg_file);
		return false;
	}

	//Log file num
	len = ini_get_key_value(p_cfg_file, "log", "log-file-num",
	                        buf, buf_len);

	if(len != 0) {
		if(len > buf_len) {
			free(buf);
			buf_len = len;
			buf = malloc(buf_len);
			ini_get_key_value(p_cfg_file, "log", "log-file-num",
			                  buf, buf_len);
		}

		log_file_num = atoi(buf);

	} else {
		ini_close(p_cfg_file);
		return false;
	}

	free(buf);

	//Initialize mutex
	pthread_mutex_init(&mutex, NULL);

	return true;
}

void cfg_destroy()
{
	if(p_cfg_file != NULL) {
		cfg_write();
		ini_close(p_cfg_file);
		free(mcserver_cmd_line);
		free(username);
		free(passwd);
		pthread_mutex_destroy(&mutex);
	}

	return;
}

bool cfg_write()
{
	return ini_sync(p_cfg_file);
}

size_t cfg_get_mcserver_dir(char* buf, size_t buf_size)
{
	size_t size;

	pthread_mutex_lock(&mutex);
	size = strlen(mcserver_dir) + 1;

	if(buf_size < size) {
		pthread_mutex_unlock(&mutex);
		return size;

	} else {
		strcpy(buf, mcserver_dir);
		pthread_mutex_unlock(&mutex);
		return size;
	}
}

size_t cfg_get_mcserver_cmd_line(char* buf, size_t buf_size)
{
	size_t size;

	pthread_mutex_lock(&mutex);
	size = strlen(mcserver_cmd_line) + 1;

	if(buf_size < size) {
		pthread_mutex_unlock(&mutex);
		return size;

	} else {
		strcpy(buf, mcserver_cmd_line);
		pthread_mutex_unlock(&mutex);
		return size;
	}
}

unsigned short cfg_get_port()
{
	return port;
}

void cfg_set_port(unsigned short new_port)
{
	char buf[64];

	pthread_mutex_lock(&mutex);
	port = new_port;
	sprintf(buf, "%u", port);
	ini_set_key_value(p_cfg_file, "ssh", "port", buf);
	pthread_mutex_unlock(&mutex);
	return;
}

size_t cfg_get_username(char* buf, size_t buf_size)
{
	size_t size;

	pthread_mutex_lock(&mutex);
	size = strlen(username) + 1;

	if(buf_size < size) {
		pthread_mutex_unlock(&mutex);
		return size;

	} else {
		strcpy(buf, username);
		pthread_mutex_unlock(&mutex);
		return size;
	}
}

void cfg_set_username(char* new_usrname)
{
	size_t size;

	pthread_mutex_lock(&mutex);
	size = strlen(new_usrname) + 1;
	free(username);
	username = malloc(size);
	strcpy(username, new_usrname);
	ini_set_key_value(p_cfg_file, "ssh",
	                  "username", username);
	pthread_mutex_unlock(&mutex);
	return;
}

size_t cfg_get_passwd(char* buf, size_t buf_size)
{
	size_t size;

	pthread_mutex_lock(&mutex);
	size = strlen(passwd) + 1;

	if(buf_size < size) {
		pthread_mutex_unlock(&mutex);
		return size;

	} else {
		strcpy(buf, passwd);
		pthread_mutex_unlock(&mutex);
		return size;
	}
}

void cfg_set_passwd(char* new_passwd)
{
	size_t size;

	pthread_mutex_lock(&mutex);
	size = strlen(new_passwd) + 1;
	free(passwd);
	passwd = malloc(size);
	strcpy(passwd, new_passwd);
	ini_set_key_value(p_cfg_file, "ssh",
	                  "password", passwd);
	pthread_mutex_unlock(&mutex);
	return;
}

int cfg_get_max_connect()
{
	return max_connect;
}

void cfg_set_max_connect(int num)
{
	char buf[64];
	max_connect = num;

	pthread_mutex_lock(&mutex);
	sprintf(buf, "%u", max_connect);
	ini_set_key_value(p_cfg_file, "ssh", "max_connection", buf);
	pthread_mutex_unlock(&mutex);
	return;
}

size_t cfg_get_log_file_size()
{
	return log_file_size;
}

int cfg_get_log_file_num()
{
	return log_file_num;
}

size_t get_size(char* buf)
{
	char* p;
	size_t size;

	size = atoi(buf);

	for(p = buf; *p != '\0'; p++) {
		if(*p > '9' || *p < '0') {
			if(*p == 'G') {
				size *= 1024 * 1024 * 1024;
				break;

			} else if(*p == 'M') {
				size *= 1024 * 1024;
				break;

			} else if(*p == 'K') {
				size *= 1024;
				break;

			} else if(*p == 'B') {
				break;

			} else {
				break;
			}
		}
	}

	return size;
}
