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

#define _GNU_SOURCE

#include <fcntl.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../config/config.h"
#include "../log/log.h"
#include "game-server.h"

#define	FD_READ		0
#define	FD_WRITE	1

static	bool				running_flag = false;
static	int					pipe_input[2];
static	int					pipe_output[2];
static	char				server_dir[PATH_MAX];
static	char				server_cmd[PATH_MAX];
static	int					server_pid = -1;
static	pthread_mutex_t		fd_mutex;
static	pthread_t			wait_thread_id;

static	int					run_server();
static	void				exec_command(char* cmd);
static	void*				wait_thread(void* args);

void game_init()
{
	pthread_mutex_init(&fd_mutex, NULL);
	return;
}

void game_destroy()
{
	game_stop();
	pthread_mutex_destroy(&fd_mutex);
	return;
}

bool game_start()
{
	pthread_mutex_lock(&fd_mutex);

	if(!running_flag) {
		//Get config
		cfg_get_mcserver_dir(server_dir, PATH_MAX);
		cfg_get_mcserver_cmd_line(server_cmd, PATH_MAX);
		running_flag = true;

		//Run server
		if(pthread_create(&wait_thread_id, NULL, wait_thread, NULL) != 0) {
			running_flag = false;
			pthread_mutex_unlock(&fd_mutex);
			return false;
		}

	}

	pthread_mutex_unlock(&fd_mutex);

	return true;
}

void game_stop()
{
	void* p_null;

	pthread_mutex_lock(&fd_mutex);

	if(running_flag) {
		running_flag = false;
		write(pipe_input[FD_WRITE], "\nstop\n", 6);
		pthread_join(wait_thread_id, &p_null);
	}

	pthread_mutex_unlock(&fd_mutex);

	return;
}

size_t game_read(char* buf, size_t buf_size)
{
	ssize_t ret;
	pthread_mutex_lock(&fd_mutex);

	if(running_flag) {
		ret = read(pipe_output[FD_READ], buf, buf_size);
		pthread_mutex_unlock(&fd_mutex);

		if(ret > 0) {
			return ret;

		} else {
			return 0;
		}
	}

	pthread_mutex_unlock(&fd_mutex);

	return 0;
}

void game_write(char* buf, size_t size)
{
	pthread_mutex_lock(&fd_mutex);

	if(running_flag) {
		write(pipe_input[FD_WRITE], buf, size);
	}

	pthread_mutex_unlock(&fd_mutex);

	return;
}


int run_server()
{
	pid_t pid;

	//Create pipes
	if(pipe(pipe_input) < 0) {
		return -1;
	}

	if(pipe(pipe_output) < 0) {
		close(pipe_input[0]);
		close(pipe_input[1]);
		return -1;
	}

	//Fork
	pid = fork();

	if(pid == -1) {
		close(pipe_input[0]);
		close(pipe_input[1]);
		close(pipe_output[0]);
		close(pipe_output[1]);
		return -1;

	} else if(pid != 0) {
		//Parent process
		close(pipe_input[FD_READ]);
		close(pipe_output[FD_WRITE]);
		return pid;
	}

	//Child process
	close(pipe_input[FD_WRITE]);
	close(pipe_output[FD_READ]);

	//Replaces stdin stdou&stderr
	//stdin
	if(dup2(pipe_input[FD_READ], 0) == -1) {
		exit(-1);
	}

	//stdout
	if(dup2(pipe_output[FD_WRITE], 1) == -1) {
		exit(-1);
	}

	//stderr
	if(dup2(pipe_output[FD_WRITE], 2) == -1) {
		exit(-1);
	}

	//Change work directory
	if(chdir(server_dir) != 0) {
		exit(-1);
	}

	//Exec
	exec_command(server_cmd);
	exit(-1);
	return -1;
}

void exec_command(char* cmd)
{
	bool quote_flag;
	char** args;
	int arg_num;
	char* buf;
	char* p;
	char** p_args;

	//Copy command
	buf = malloc(strlen(cmd) + 1);
	strcpy(buf, cmd);

	//Compute number of arguments
	for(quote_flag = false, arg_num = 1, p = buf;
	    *p != '\0';
	    p++) {
		if(*p == '\"' || *p == '\'') {
			quote_flag = !quote_flag;

		} else if(*p == ' ' && !quote_flag) {
			arg_num++;
		}
	}

	//Spilt arguments
	args = malloc(sizeof(char*) * (arg_num + 1));
	*args = buf;

	for(p_args = args + 1, quote_flag = false, p = buf;
	    *p != '\0';
	    p++) {
		if(*p == '\"' || *p == '\'') {
			quote_flag = !quote_flag;

		} else if(*p == ' ' && !quote_flag) {
			*p = '\0';
			p++;
			*p_args = p;
			p_args++;
		}

	}

	*p_args = NULL;
	execvp(buf, args);
	return;
}

void* wait_thread(void* args)
{
	int status;

	while(running_flag) {
		//Start server
		pthread_mutex_lock(&fd_mutex);
		server_pid = run_server();

		if(server_pid == -1) {
			close(pipe_input[FD_WRITE]);
			close(pipe_output[FD_READ]);
			pthread_mutex_unlock(&fd_mutex);
			printlog(LOG_SERVER, "Failed to run server!\n");
			continue;
		}

		pthread_mutex_unlock(&fd_mutex);
		printlog(LOG_SERVER, "Server started.\n");

		//Wait
		waitpid(server_pid, &status, 0);
	}

	UNREFERRED_PARAMETER(args);
	return NULL;
}
