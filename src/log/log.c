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
#include <errno.h>
#include <stdarg.h>
#include <time.h>

#include "log.h"
#include "../config/config.h"

#define	LOG_DIR				"/var/log/mcserverd"
#define	SSH_LOG_NAME		"/var/log/mcserverd/ssh-%d.log"
#define	MC_LOG_NAME			"/var/log/mcserverd/minecraft-%d.log"

static	pthread_mutex_t		ssh_mutex;
static	pthread_mutex_t		mc_mutex;
static	size_t				ssh_size = 0;
static	size_t				mc_size = 0;
static	int					ssh_log_num = 0;
static	int					mc_log_num = 0;
static	size_t				max_size;
static	int					max_num;

static	FILE*				ssh_fp = NULL;
static	FILE*				minecraft_fp = NULL;

static	FILE*				open_log_file(char* fmt,
        int* p_num,
        size_t* p_size);
static	void				next_log(char* fmt, int* p_num);

bool log_init()
{
	//Creat log directory
	if(mkdir(LOG_DIR,
	         S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IROTH) != 0) {
		if(errno != EEXIST) {
			return false;
		}
	}

	//Open log file
	ssh_fp = open_log_file(SSH_LOG_NAME, &ssh_log_num, &ssh_size);

	if(ssh_fp == NULL) {
		return false;
	}

	minecraft_fp = open_log_file(MC_LOG_NAME, &mc_log_num, &mc_size);

	if(minecraft_fp == NULL) {
		fclose(ssh_fp);
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
	FILE** p_fp;
	pthread_mutex_t* p_mutex;
	size_t* p_size;
	int out_size;
	int time_size;
	va_list args;
	char* file_fmt;
	int* p_num;
	time_t secs;
	struct tm st_tm;

	//Get file to write
	switch(log) {
	case LOG_CONN:
		p_fp = &ssh_fp;
		p_mutex = &ssh_mutex;
		p_size = &ssh_size;
		file_fmt = SSH_LOG_NAME;
		p_num = &ssh_log_num;
		break;

	case LOG_SERVER:
		p_fp = &minecraft_fp;
		p_mutex = &mc_mutex;
		p_size = &mc_size;
		file_fmt = MC_LOG_NAME;
		p_num = &mc_log_num;
		break;

	default:
		return;
	}

	pthread_mutex_lock(p_mutex);

	//Print file
	va_start(args, fmt);

	secs = time((time_t)0);
	localtime_r(&secs, &st_tm);
	time_size = fprintf(*p_fp, "[%d-%d-%d %d:%d:%d] ",
	                    st_tm.tm_mon,
	                    st_tm.tm_mday,
	                    st_tm.tm_year,
	                    st_tm.tm_hour,
	                    st_tm.tm_min,
	                    st_tm.tm_sec);
	out_size = vfprintf(*p_fp, fmt, args);

	if(out_size > 0) {
		(*p_size) += out_size;
	}

	if(time_size > 0) {
		(*p_size) += time_size;
	}

	//Check file size
	if(*p_size >= max_size) {
		//Open new file
		fclose(*p_fp);

		do {
			*p_fp = open_log_file(file_fmt, p_num, p_size);
		} while(*p_fp == NULL);

	}

	pthread_mutex_unlock(p_mutex);

	return;
}


FILE* open_log_file(char* fmt, int* p_num, size_t* p_size)
{
	char name[64];
	struct stat file_status;
	FILE* ret;

	if(max_num == 0 || max_size == 0) {
		return NULL;
	}

	//Get last log file
	for(; *p_num < max_num; (*p_num)++) {
		sprintf(name, fmt, *p_num);

		if(access(name, F_OK) != 0) {
			if(*p_num > 0) {
				(*p_num)--;
			}

			break;
		}
	}

	//Get file status
	sprintf(name, fmt, *p_num);

	if(stat(name, &file_status) == 0) {
		if(file_status.st_size >= max_size) {
			next_log(fmt, p_num);
			sprintf(name, fmt, *p_num);
		}

	} else {
		if(errno != ENOENT) {
			return NULL;
		}

	}

	//Open file
	ret = fopen(name, "a");

	if(ret != NULL) {
		*p_size = ftell(ret);
	}

	return ret;
}

void next_log(char* fmt, int* p_num)
{
	int i;
	char name[64];
	char new_name[64];

	if(*p_num >= max_num) {
		//Remove oldest file
		sprintf(name, fmt, 0);

		if(unlink(name) != 0
		   && errno != ENOENT) {
			return;
		}

		//Rename files
		for(i = 1; i <= *p_num; i++) {
			sprintf(name, fmt, i);
			sprintf(new_name, fmt, i - 1);
			rename(name, new_name);
		}

	} else {
		(*p_num)++;
	}

	return;
}
