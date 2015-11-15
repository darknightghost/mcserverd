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

#include "cmdline.h"
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#define	FD_READ		0
#define	FD_WRITE	1

static	int					pipe_stdin[2];
static	int					pipe_stdout[2];
static	bool				run_flag = false;
static	pthread_t			cmd_thread_id;
static	pthread_mutex_t		mutex;
static	char*				prompt = "mcserverd> ";

static	void*				cmdline_thread(void* args);
static	void				execute_command(char* cmd);

bool cmdline_init()
{
	if(!run_flag) {
		//Create pipe
		if(pipe(pipe_stdin) == -1) {
			return false;
		}

		if(pipe(pipe_stdout) == -1) {
			close(pipe_stdin[0]);
			close(pipe_stdin[1]);
			return false;
		}

		//Replace stdin,stdout,stderr
		//stdin
		if(dup2(pipe_stdin[FD_READ], STDIN_FILENO) == -1) {
			close(pipe_stdin[0]);
			close(pipe_stdin[1]);
			close(pipe_stdout[0]);
			close(pipe_stdout[1]);
			return false;
		}

		//stdout
		if(dup2(pipe_stdout[FD_WRITE], STDOUT_FILENO) == -1) {
			close(pipe_stdin[0]);
			close(pipe_stdin[1]);
			close(pipe_stdout[0]);
			close(pipe_stdout[1]);
			return false;
		}

		//stderr
		if(dup2(pipe_stdout[FD_WRITE], STDERR_FILENO) == -1) {
			close(pipe_stdin[0]);
			close(pipe_stdin[1]);
			close(pipe_stdout[0]);
			close(pipe_stdout[1]);
			return false;
		}

		run_flag = true;
		pthread_mutex_init(&mutex, NULL);

		if(pthread_create(&cmd_thread_id, NULL, cmdline_thread, NULL) != 0) {
			run_flag = false;
			close(pipe_stdin[0]);
			close(pipe_stdin[1]);
			close(pipe_stdout[0]);
			close(pipe_stdout[1]);
			pthread_mutex_destroy(&mutex);
			return false;
		}
	}

	return true;
}

void cmdline_destroy()
{
	void* ret;

	if(run_flag) {
		run_flag = false;
		close(pipe_stdin[0]);
		close(pipe_stdin[1]);
		close(pipe_stdout[0]);
		close(pipe_stdout[1]);
		pthread_mutex_destroy(&mutex);
		pthread_join(cmd_thread_id, &ret);
	}

	return;
}

size_t cmdline_read(char* buf, size_t buf_size)
{
	ssize_t ret;

	if(run_flag) {
		pthread_mutex_lock(&mutex);
		ret = read(pipe_stdout[FD_READ], buf, buf_size);
		pthread_mutex_unlock(&mutex);

		if(ret >= 0) {
			return ret;
		}
	}

	return 0;
}

size_t cmdline_write(char* buf, size_t size)
{
	ssize_t ret;

	if(run_flag) {
		pthread_mutex_lock(&mutex);
		ret = write(pipe_stdin[FD_WRITE], buf, size);
		pthread_mutex_unlock(&mutex);

		if(ret >= 0) {
			return ret;
		}

	}

	return 0;
}

void* cmdline_thread(void* args)
{
	char* cmd;

	while(run_flag) {
		cmd = readline(prompt);
		add_history(cmd);

		//Excute command
		execute_command(cmd);

		free(cmd);
	}

	UNREFERRED_PARAMETER(args);
	return NULL;
}

void execute_command(char* cmd)
{
	UNREFERRED_PARAMETER(cmd);
}
