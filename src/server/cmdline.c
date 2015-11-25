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
#include <stdio_ext.h>
#include <readline/readline.h>
#include <readline/history.h>

#include "../config/config.h"
#include "cmdline.h"
#include "game-server.h"
#include "../network/network.h"
#include "../log/log.h"

#define	FD_READ		0
#define	FD_WRITE	1

#define	ECHOFLAGS		(ECHO | ECHOE | ECHOK | ECHONL)

static	int					pipe_stdin[2];
static	int					pipe_stdout[2];
static	bool				run_flag = false;
static	pthread_t			cmd_thread_id;
static	pthread_t			read_thread_id;
static	pthread_mutex_t		mutex;
static	bool				mc_mode;

static	bool				execute_command(char* cmd);
static	char**				analyse_command(char* cmd, int* p_argc);
static	void				jump_space(char**p);
static	void				exec_server_cmd(int argc, char* argv[]);
static	void				exec_mc_cmd(char* cmd);

static	char*				get_line_input(char* buf, size_t size, bool echo);

static	void*				cmdline_thread(void* args);
static	void*				read_thread(void* args);

//Commands
static	void				cmd_server_start(int argc, char* argv[]);
static	void				cmd_server_username(int argc, char* argv[]);
static	void				cmd_server_passwd(int argc, char* argv[]);
static	void				cmd_server_server(int argc, char* argv[]);
static	void				cmd_server_exit(int argc, char* argv[]);
static	void				cmd_server_status(int argc, char* argv[]);
static	void				cmd_server_exitserver(int argc, char* argv[]);

static	void				cmd_mc_stop();
static	void				cmd_mc_exit();

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

	if(pthread_create(&read_thread_id, NULL, read_thread, NULL) != 0) {
		close(pipe_stdin[0]);
		close(pipe_stdin[1]);
		close(pipe_stdout[0]);
		close(pipe_stdout[1]);
		pthread_mutex_destroy(&mutex);

		return false;
	}

	return true;
}

void cmdline_destroy()
{
	cmdline_stop();
	pthread_cancel(read_thread_id);
	pthread_join(read_thread_id, NULL);
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
	pthread_mutex_lock(&mutex);

	if(run_flag) {
		run_flag = false;
		pthread_cancel(cmd_thread_id);
		pthread_join(cmd_thread_id, NULL);
	}

	pthread_mutex_unlock(&mutex);
	return;
}

size_t cmdline_read(char* buf, size_t buf_size)
{
	ssize_t ret;

	ret = read(pipe_stdout[FD_READ], buf, buf_size);

	if(ret >= 0) {
		return ret;
	}

	return 0;
}

size_t cmdline_write(char* buf, size_t size)
{
	ssize_t ret;

	if(run_flag) {
		ret = write(pipe_stdin[FD_WRITE], buf, size);

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

void* read_thread(void* args)
{
	char buf[4096];
	size_t len;

	while(1) {
		len = game_read(buf, 4096);

		if(len > 0 && run_flag) {
			fwrite(buf, 1, len, stdout);
			fflush(stdout);
		}
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
		cmd_server_start(argc, argv);

	} else if(strcmp(argv[0], "username") == 0) {
		cmd_server_username(argc, argv);

	} else if(strcmp(argv[0], "passwd") == 0) {
		cmd_server_passwd(argc, argv);

	} else if(strcmp(argv[0], "server") == 0) {
		cmd_server_server(argc, argv);

	} else if(strcmp(argv[0], "exit") == 0) {
		cmd_server_exit(argc, argv);

	} else if(strcmp(argv[0], "status") == 0) {
		cmd_server_status(argc, argv);

	} else if(strcmp(argv[0], "exitserver") == 0) {
		cmd_server_exitserver(argc, argv);

	} else {
		printf("Unknow command!\n");
	}

	UNREFERRED_PARAMETER(argc);

	return;
}

void exec_mc_cmd(char* cmd)
{
	size_t len;
	size_t written;
	char* log_buf;

	if(strcmp("/stop", cmd) == 0) {
		cmd_mc_stop();

	} else if(strcmp("exit", cmd) == 0) {
		cmd_mc_exit();

	} else {
		len = strlen(cmd);

		for(written = 0; written < len;) {
			written += game_write(cmd + written, len - written);
		}

		game_write("\n", 1);

		//Write log
		log_buf = malloc(len + 2);
		memcpy(log_buf, cmd, len);
		*(log_buf + len) = '\n';
		*(log_buf + len + 1) = '\0';
		printlog(LOG_SERVER, "%s", log_buf);
		free(log_buf);
	}
}

char* get_line_input(char* buf, size_t size, bool echo)
{
	size_t len_read;
	char* p;


	setbuf(stdin, NULL);
	p = buf;
	fread(p, 1, 1, stdin);

	for(len_read = 0;
	    len_read < size;
	    len_read++, p++) {
		fread(p, 1, 1, stdin);

		if(*p == '\r' || *p == '\n') {
			setbuf(stdin, NULL);
			break;

		} else if((*p < '0' || *p > '9')
		          && (*p < 'a' || *p > 'z')
		          && (*p < 'A' || *p > 'Z')) {
			printf("\nUnknow character:%c!\n", *p);
			return NULL;

		} else if(echo) {
			printf("%c", *p);
			fflush(stdout);

		}
	}

	*p = '\0';
	printf("\n");
	fflush(stdout);

	return buf;
}

void cmd_server_start(int argc, char* argv[])
{
	if(game_start()) {
		printf("Service started.\n");

	} else {
		printf("Failed to start service.\n");
	}

	UNREFERRED_PARAMETER(argc);
	UNREFERRED_PARAMETER(argv);
	return;
}

void cmd_server_username(int argc, char* argv[])
{
	char usrname[MAX_USERNAME_LEN + 5];

	printf("New username:");
	fflush(stdout);

	if(get_line_input(usrname, MAX_USERNAME_LEN + 5, true) == NULL) {
		printf("\n");
		return;
	}

	if(strlen(usrname) > MAX_USERNAME_LEN) {
		printf("The length of username must below %d!\n", MAX_USERNAME_LEN);

	} else {
		cfg_set_username(usrname);
		printlog(LOG_CONN, "Username changed.\n");
	}

	UNREFERRED_PARAMETER(argc);
	UNREFERRED_PARAMETER(argv);
	return;
}

void cmd_server_passwd(int argc, char* argv[])
{
	char passwd[MAX_PASSWD_LEN + 5];
	char passwd_again[MAX_PASSWD_LEN + 5];

	//Get new password
	printf("New password:");
	fflush(stdout);

	if(get_line_input(passwd, MAX_PASSWD_LEN + 5, false) == NULL) {
		printf("\n");
		return;
	}

	if(strlen(passwd) > MAX_PASSWD_LEN) {
		printf("The length of password must below %d!\n", MAX_PASSWD_LEN);

	} else {
		//Again
		printf("Retype new password:");
		fflush(stdout);

		if(get_line_input(passwd_again, MAX_PASSWD_LEN + 5, false) == NULL) {
			printf("\n");
			return;
		}

		if(strcmp(passwd, passwd_again) == 0) {
			cfg_set_passwd(passwd);
			printlog(LOG_CONN, "Password changed.\n");

		} else {
			printf("Sorry, passwords do not match.\n");
		}
	}

	UNREFERRED_PARAMETER(argc);
	UNREFERRED_PARAMETER(argv);
	return;
}

void cmd_server_server(int argc, char* argv[])
{
	mc_mode = true;
	UNREFERRED_PARAMETER(argc);
	UNREFERRED_PARAMETER(argv);
	return;
}

void cmd_server_exit(int argc, char* argv[])
{
	printf("\n");
	network_logoff();
	UNREFERRED_PARAMETER(argc);
	UNREFERRED_PARAMETER(argv);
	return;
}

void cmd_server_status(int argc, char* argv[])
{
	if(game_is_running()) {
		printf("Service started.\n");

	} else {
		printf("Service stopped.\n");
	}

	UNREFERRED_PARAMETER(argc);
	UNREFERRED_PARAMETER(argv);
	return;
}

void cmd_server_exitserver(int argc, char* argv[])
{
	printf("\n");
	game_stop();
	network_quit();

	UNREFERRED_PARAMETER(argc);
	UNREFERRED_PARAMETER(argv);
	return;
}

void cmd_mc_stop()
{
	game_stop();
	return;
}

void cmd_mc_exit()
{
	mc_mode = false;
	return;
}
