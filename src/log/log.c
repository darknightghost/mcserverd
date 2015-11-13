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

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "../config/config.h"

#define	LOG_DIR				"/var/log/mcserverd"
#define	SSH_LOG_NAME		"ssh-%d.log"
#define	MC_LOG_NAME			"minecraft-%d.log"

static	pthread_mutex_t		ssh_mutex;
static	pthread_mutex_t		mc_mutex;
static	size_t				file_size = 0;
static	int					ssh_log_num = 0;
static	int					mc_log_num = 0;
static	size_t				max_size;
static	int					max_num;

static	FILE*				ssh_fp = NULL;
static	FILE*				minecraft_fp = NULL;

static	bool				open_log_file();

bool log_init()
{
	//Open log file
	if(!open_log_file()) {
		return false;
	}

	//Read config
	max_size = cfg_get_log_file_size();
	max_num = cfg_get_log_file_num();

	//Initialize mutex
	pthread_mutex_init(&ssh_mutex, NULL);
	pthread_mutex_init(&mc_mutex, NULL);
	return true;
}

void log_close()
{
	if(ssh_fp != NULL) {
		fclose(ssh_fp);
	}

	if(minecraft_fp != NULL) {
		fclose(minecraft_fp);
	}

	pthread_mutex_destroy(&ssh_mutex);
	pthread_mutex_destroy(&mc_mutex);
	return;
}

void printlog(int log, char* fmt, ...)
{
	return;
}


bool open_log_file()
{
	char ssh_name[64];
	char mc_name[64];
	struct stat file_status;

	if(max_num == 0 || max_size == 0) {
		return false;
	}

	//Get last log file
	//SSH
	for(; ssh_log_num < max_num; ssh_log_num++) {
		sprintf(ssh_name, SSH_LOG_NAME, ssh_log_num);

		if(access(ssh_name, F_OK) != 0) {
			if(ssh_log_num > 0) {
				ssh_log_num--;
			}

			break;
		}
	}

	//Get file status
	sprintf(ssh_name, SSH_LOG_NAME, ssh_log_num);

	if(stat(ssh_name & file_status) == 0) {

	} else {

	}

	//Minecraft
	for(; mc_log_num < max_num; mc_log_num++) {
		sprintf(mc_name, MC_LOG_NAME, mc_log_num);

		if(access(mc_name, F_OK) != 0) {
			if(mc_log_num > 0) {
				mc_log_num--;
			}

			break;
		}
	}


}
