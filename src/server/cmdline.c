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
#include <pthread.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "cmdline.h"
#include "game-server.h"

#define	FD_READ		0
#define	FD_WRITE	1

static	int					pipe_stdin[2];
static	int					pipe_stdout[2];
static	bool				run_flag = false;
static	pthread_t			cmd_thread_id;
static	pthread_mutex_t		mutex;
static	bool				mc_mode;

static	void*				cmdline_thread(void* args);
static	bool				execute_command(char* cmd);
static	char**				analyse_command(char* cmd, int* p_argc);
static	void				jump_space(char**p);
static	void				exec_server_cmd(int argc, char* argv[]);
static	void				exec_mc_cmd(char* cmd);

bool cmdline_init()
{
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

	run_flag = false;
	pthread_mutex_init(&mutex, NULL);
	return true;
}

void cmdline_destroy()
{
	cmdline_stop();
	close(pipe_stdin[0]);
	close(pipe_stdin[1]);
	close(pipe_stdout[0]);
	close(pipe_stdout[1]);
	pthread_mutex_destroy(&mutex);

	return;
}

bool cmdline_start()
{
	pthread_mutex_lock(&mutex);

	if(!run_flag) {
		mc_mode = false;
		run_flag = true;

		if(pthread_create(&cmd_thread_id, NULL, cmdline_thread, NULL) != 0) {
			run_flag = false;
			pthread_mutex_unlock(&mutex);
			return false;
		}
	}

	pthread_mutex_unlock(&mutex);
	return true;
}

void cmdline_stop()
{
	void* ret;
	pthread_mutex_lock(&mutex);

	if(run_flag) {
		run_flag = false;
		write(pipe_stdin[FD_WRITE], "\r", 1);
		pthread_join(cmd_thread_id, &ret);
	}

	pthread_mutex_unlock(&mutex);
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
		if(mc_mode) {
			cmd = readline("\nminecraft> ");

		} else {
			cmd = readline("server> ");
		}

		//Excute command
		if(execute_command(cmd)) {
			add_history(cmd);
		}

		free(cmd);
	}

	UNREFERRED_PARAMETER(args);
	return NULL;
}

bool execute_command(char* cmd)
{
	char** args;
	int argc;
	int i;

	if(mc_mode) {
		exec_mc_cmd(cmd);

	} else {
		//Analyse command
		args = analyse_command(cmd, &argc);

		if(args == NULL) {
			return false;
		}

		exec_server_cmd(argc, args);

		for(i = 0; args[i] != NULL; i++) {
			free(args[i]);
			free(args);
		}
	}

	return true;
}


char** analyse_command(char* cmd, int* p_argc)
{
	bool quote_flag;
	char** ret;
	char** p_args;
	int argc;
	char* p_src;
	char* p_dest;
	char* buf;

	quote_flag = false;

	//Get arg num
	argc = 0;
	p_src = cmd;
	quote_flag = false;
	jump_space(&p_src);

	if(*p_src == '\0') {
		return NULL;
	}

	while(true) {
		if(*p_src == '\'' || *p_src == '\"') {
			quote_flag = !quote_flag;
			p_src++;

		} else if(((*p_src == ' ' || *p_src == '\t') && !quote_flag)
		          || *p_src == '\0') {
			argc++;
			jump_space(&p_src);

			if(*p_src == '\0') {
				break;
			}

		} else {
			p_src++;
		}
	}

	//Allocate memory
	ret = malloc(sizeof(char*) * (argc + 1));
	buf = malloc(strlen(cmd) + 1);

	//Split arguments
	p_dest = buf;
	p_src = cmd;
	p_args = ret;

	while(true) {
		if(*p_src == '\'' || *p_src == '\"') {
			quote_flag = !quote_flag;
			p_src++;

		} else if(((*p_src == ' ' || *p_src == '\t') && !quote_flag)
		          || *p_src == '\0') {
			*p_dest = '\0';
			*p_args = malloc(strlen(buf) + 1);
			strcpy(*p_args, buf);
			p_dest = buf;
			p_args++;
			jump_space(&p_src);

			if(*p_src == '\0') {
				break;
			}

		} else {
			*p_dest = *p_src;
			p_src++;
			p_dest++;
		}
	}

	*p_args = NULL;

	free(buf);
	*p_argc = argc;
	return ret;
}

void jump_space(char**p)
{
	while(**p == ' '
	      ||**p == '\t') {
		(*p)++;
	}

	return;
}

void exec_server_cmd(int argc, char* argv[])
{
	if(strcmp(argv[0], "start") == 0) {
	} else if(strcmp(argv[0], "username") == 0) {

	} else if(strcmp(argv[0], "passwd") == 0) {

	} else if(strcmp(argv[0], "server") == 0) {

	} else if(strcmp(argv[0], "exit") == 0) {

	} else if(strcmp(argv[0], "status") == 0) {

	}

	return;
}

void exec_mc_cmd(char* cmd)
{
	if(strcmp("stop", cmd) == 0) {

	} else if(strcmp("exit", cmd) == 0) {

	} else {

	}
}
