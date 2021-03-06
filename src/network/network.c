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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "../common/common.h"
#include "../log/log.h"
#include "../config/config.h"
#include "../server/server.h"
#include "network.h"
#include "ssh.h"
#include "session.h"

#define	MAX_TRY_TIME	5
#define	SYS_RSA_KEY		"/etc/ssh/ssh_host_rsa_key"
#define	WAIT_TEXT		"Other administrators has logged in,please wait until they logged off.\r\n"
#define	URGENCY_TEXT	"\r\nOther administrators are waitting for you!\r\n"

static queue_t			session_queue;
static volatile	bool	run_flag;
static pthread_mutex_t	mutex;
static pthread_mutex_t	socket_mutex;
static pthread_t		gc_thread_id;
static pthread_t		send_thread_id;
static pthread_cond_t	cond;
static pthread_t		main_thread;

static	void			session_destroy_callback(void* p_item, void* args);
static	int				pty_request(ssh_session session, ssh_channel channel,
                                    const char *term, int cols, int rows,
                                    int py, int px, void *userdata);
static	int				shell_request(ssh_session session, ssh_channel channel,
                                      void *userdata);
static	ssh_channel		channel_open(ssh_session session, void *userdata);
static	int				conn_auth(ssh_session session, const char *user,
                                  const char *pass, void *userdata);
static	int				conn_receive(ssh_session session, ssh_channel channel,
                                     void *data, uint32_t len, int is_stderr,
                                     void *userdata);

static	void*			session_thread(void* args);
static	void*			send_thread(void* args);
static	void*			gc_thread(void* args);
static	void			sigroutine(int dunno);
static	void			sigterm(int dunno);

void network_init()
{
	queue_init(session_queue);
	pthread_mutex_init(&mutex, NULL);
	pthread_mutex_init(&socket_mutex, NULL);
	pthread_cond_init(&cond, NULL);
	return;
}

void network_destroy()
{
	queue_destroy(session_queue, session_destroy_callback, NULL);
	pthread_mutex_destroy(&mutex);
	pthread_mutex_destroy(&socket_mutex);
	pthread_cond_destroy(&cond);
	return;
}

int network_main()
{
	psession_t p_session;
	ssh_bind bind;
	int port;
	struct sigaction action;
	struct sigaction term_action;
	int status;

	main_thread = pthread_self();

	//Setup awake signal
	action.sa_handler = sigroutine;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGCONT, &action, NULL);

	//Setup awake signal
	term_action.sa_handler = sigterm;
	sigemptyset(&term_action.sa_mask);
	term_action.sa_flags = 0;
	sigaction(SIGTERM, &term_action, NULL);

	printlog(LOG_CONN, "Starting ssh service...\n");

	ssh_init();
	bind = ssh_bind_new();

	//Bind port
	port = cfg_get_port();
	status = ssh_bind_options_set(bind, SSH_BIND_OPTIONS_BINDPORT, &port);

	if(status != 0) {
		printlog(LOG_CONN, "%s\n", ssh_get_error((void*)bind));
		ssh_bind_free(bind);
		return -1;
	}

	//RSA key
	status = ssh_bind_options_set(bind, SSH_BIND_OPTIONS_RSAKEY,
	                              SYS_RSA_KEY);

	if(status != 0) {
		printlog(LOG_CONN, "%s\n", ssh_get_error((void*)bind));
		ssh_bind_free(bind);
		return -1;
	}

	run_flag = true;

	//Create gc thread
	if(pthread_create(&gc_thread_id,
	                  NULL, gc_thread, NULL) != 0) {
		printlog(LOG_CONN, "Failed to create gc thread!\n");
		ssh_bind_free(bind);
		return -1;
	}

	//Create send thread
	if(pthread_create(&send_thread_id,
	                  NULL, send_thread, NULL) != 0) {
		printlog(LOG_CONN, "Failed to create send thread!\n");
		pthread_cancel(gc_thread_id);
		pthread_join(gc_thread_id, NULL);
		ssh_bind_free(bind);
		return -1;
	}

	//Listen for new connection
	ssh_bind_set_blocking(bind, 1);
	printlog(LOG_CONN, "Start listen on port: %d\n", port);

	while(run_flag) {
		status = ssh_bind_listen(bind);

		if(status != 0) {
			if(run_flag) {
				printlog(LOG_CONN, "%s\n", ssh_get_error((void*)bind));
			}

			continue;
		}

		p_session = session_new();
		status = ssh_bind_accept(bind, p_session->session);

		if(status != 0) {
			if(run_flag) {
				printlog(LOG_CONN, "%s\n", ssh_get_error((void*)bind));
			}

			session_free(p_session);
			continue;
		}

		//Create thread
		if(pthread_create(&(p_session->thread),
		                  NULL, session_thread, p_session) != 0) {
			printlog(LOG_CONN, "Failed to create session thread!\n");
			session_free(p_session);
			continue;
		}

		pthread_mutex_lock(&mutex);
		queue_push(session_queue, p_session);

		if(p_session == queue_front(session_queue)) {
			p_session->status = SESSION_ACTIVE;
			server_refresh();
		}

		pthread_mutex_unlock(&mutex);
	}

	//Join gc thread
	pthread_join(gc_thread_id, NULL);
	pthread_join(send_thread_id, NULL);

	//Destroy queue
	queue_destroy(session_queue, session_destroy_callback, NULL);
	ssh_bind_free(bind);
	ssh_finalize();
	printlog(LOG_CONN, "ssh service stopped.\n");
	return 0;
}

void network_quit()
{
	run_flag = false;
	pthread_kill(main_thread, SIGCONT);
	pthread_cancel(gc_thread_id);
	pthread_cancel(send_thread_id);
	return;
}

void network_logoff()
{
	psession_t p_session;

	pthread_mutex_lock(&mutex);
	p_session = (psession_t)queue_front(session_queue);
	ssh_channel_free(p_session->channel);
	p_session->channel = NULL;
	pthread_mutex_unlock(&mutex);
	return;
}

void session_destroy_callback(void* p_item, void* args)
{
	psession_t p_session = (psession_t)p_item;

	if(p_session->status != SESSION_EXIT) {
		pthread_cancel(p_session->thread);
	}

	pthread_join(p_session->thread, NULL);
	session_free((psession_t)p_item);
	UNREFERRED_PARAMETER(args);
	return;
}

int pty_request(ssh_session session, ssh_channel channel,
                const char *term, int cols, int rows, int py, int px,
                void *userdata)
{
	UNREFERRED_PARAMETER(session);
	UNREFERRED_PARAMETER(channel);
	UNREFERRED_PARAMETER(term);
	UNREFERRED_PARAMETER(cols);
	UNREFERRED_PARAMETER(rows);
	UNREFERRED_PARAMETER(py);
	UNREFERRED_PARAMETER(px);
	UNREFERRED_PARAMETER(userdata);
	return SSH_OK;
}

int shell_request(ssh_session session, ssh_channel channel,
                  void *userdata)
{
	UNREFERRED_PARAMETER(session);
	UNREFERRED_PARAMETER(channel);
	UNREFERRED_PARAMETER(userdata);
	return SSH_OK;
}

ssh_channel channel_open(ssh_session session, void *userdata)
{
	psession_t p_session;

	p_session = (psession_t)userdata;
	p_session->channel = ssh_channel_new(session);
	return p_session->channel;
}

void* session_thread(void* args)
{
	volatile psession_t p_session;
	struct sockaddr addr;
	int socket_fd;
	socklen_t len;
	int status;
	char ip_buf[17];

	p_session = (psession_t)args;

	memset(&(p_session->server_callbacks), 0,
	       sizeof(struct ssh_server_callbacks_struct));
	memset(&(p_session->channel_callbacks), 0,
	       sizeof(struct ssh_channel_callbacks_struct));

	p_session->server_callbacks.userdata = p_session;
	p_session->server_callbacks.auth_password_function = conn_auth;
	p_session->server_callbacks.channel_open_request_session_function = channel_open;

	p_session->channel_callbacks.userdata = p_session;
	p_session->channel_callbacks.channel_pty_request_function = pty_request;
	p_session->channel_callbacks.channel_pty_window_change_function = NULL;
	p_session->channel_callbacks.channel_shell_request_function = shell_request;
	p_session->channel_callbacks.channel_exec_request_function = NULL;
	p_session->channel_callbacks.channel_data_function = conn_receive;
	p_session->channel_callbacks.channel_subsystem_request_function = NULL;

	//Print log
	socket_fd = ssh_get_fd(p_session->session);
	len = sizeof(addr);
	getpeername(socket_fd, &addr, &len);
	pthread_mutex_lock(&socket_mutex);
	strcpy(ip_buf, inet_ntoa(((struct sockaddr_in*)(&addr))->sin_addr));
	printlog(LOG_CONN, "Host %s has been accepted.\n",
	         ip_buf);
	pthread_mutex_unlock(&socket_mutex);

	//Server callbacks
	ssh_callbacks_init(&(p_session->server_callbacks));
	ssh_callbacks_init(&(p_session->channel_callbacks));
	ssh_set_server_callbacks(p_session->session, &(p_session->server_callbacks));

	//Exchange key
	status = ssh_handle_key_exchange(p_session->session);

	if(status != SSH_OK) {
		pthread_mutex_lock(&mutex);
		p_session->status = SESSION_EXIT;
		printlog(LOG_CONN, "%s\n", ssh_get_error((void*)(p_session->session)));
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
		return NULL;
	}

	//Event
	status = ssh_event_add_session(p_session->event, p_session->session);

	if(status != SSH_OK) {
		pthread_mutex_lock(&mutex);
		p_session->status = SESSION_EXIT;
		printlog(LOG_CONN, "%s\n", ssh_get_error((void*)(p_session->session)));
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
		return NULL;
	}

	//Auth
	while(!p_session->authed
	      || p_session->channel == NULL) {
		if(ssh_event_dopoll(p_session->event, -1) == SSH_ERROR) {
			printlog(LOG_CONN, "Connection to %s has been lost.\n",
			         ip_buf);
			pthread_mutex_lock(&mutex);
			p_session->status = SESSION_EXIT;
			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&mutex);
			return NULL;
		}

		if(!p_session->tried_time > MAX_TRY_TIME) {
			__asm__ __volatile__("":::"memory");
			printlog(LOG_CONN, "Host %s has tried too much times.\n",
			         ip_buf);
			pthread_mutex_lock(&mutex);
			p_session->status = SESSION_EXIT;
			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&mutex);
			return NULL;
		}
	}

	if(ssh_set_channel_callbacks(p_session->channel,
	                             &(p_session->channel_callbacks)) != SSH_OK) {
		pthread_mutex_lock(&mutex);
		p_session->status = SESSION_EXIT;
		printlog(LOG_CONN, "Failed to set channel callback.\n");
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
		return NULL;
	}

	while(ssh_channel_is_open(p_session->channel)) {
		if(ssh_event_dopoll(p_session->event, -1) == SSH_ERROR) {
			break;
		}
	}

	printlog(LOG_CONN, "Host %s has logged off.\n",
	         ip_buf);
	pthread_mutex_lock(&mutex);
	p_session->status = SESSION_EXIT;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);

	return NULL;
}

void* send_thread(void* args)
{
	char buf[1024];
	size_t size;
	struct timespec sleep_time;
	psession_t p_session;
	char* p_start;
	char* p_end;

	sleep_time.tv_sec = 0;
	sleep_time.tv_nsec = 10;

	while(run_flag) {
		//Read console
		size = server_read(buf, 1024);

		if(size > 0) {
			pthread_mutex_lock(&mutex);

			while(queue_front(session_queue) == NULL
			      || ((psession_t)queue_front(session_queue))->status
			      == SESSION_EXIT
			      || !((psession_t)queue_front(session_queue))->authed) {
				pthread_mutex_unlock(&mutex);

				if(!run_flag) {
					break;
				}

				nanosleep(&sleep_time, NULL);

				pthread_mutex_lock(&mutex);
			}

			p_session = (psession_t)queue_front(session_queue);

			for(p_start = buf, p_end = buf;
			    p_end - buf < size;
			    p_end++) {
				if(*p_end == '\n') {
					ssh_channel_write(p_session->channel, p_start,
					                  p_end - p_start);
					ssh_channel_write(p_session->channel, "\r",
					                  1);
					p_start = p_end;
				}
			}

			ssh_channel_write(p_session->channel, p_start,
			                  p_end - p_start);
			pthread_mutex_unlock(&mutex);
		}

	}

	UNREFERRED_PARAMETER(args);
	return NULL;
}

void* gc_thread(void* args)
{
	volatile psession_t p_session;
	pthread_mutex_lock(&mutex);
	struct sockaddr addr;
	socklen_t len;
	int socket_fd;
	void* macro_ret;

	while(run_flag) {
		//Suspend thread
		pthread_cond_wait(&cond, &mutex);

		//Release exited sessions
		while(session_queue != NULL) {
			p_session = (psession_t)queue_front(session_queue);

			if(p_session->status == SESSION_EXIT) {
				p_session = (psession_t)queue_pop(session_queue, macro_ret);
				session_free(p_session);

			} else {
				break;
			}
		}

		//Active first session
		if(session_queue != NULL) {
			p_session = (psession_t)queue_front(session_queue);

			if(p_session->status == SESSION_WAITING) {
				p_session->status = SESSION_ACTIVE;
				socket_fd = ssh_get_fd(p_session->session);
				len = sizeof(addr);
				getpeername(socket_fd, &addr, &len);
				pthread_mutex_lock(&socket_mutex);
				printlog(LOG_CONN, "Host %s actived.\n",
				         inet_ntoa(((struct sockaddr_in*)(&addr))->sin_addr));
				pthread_mutex_unlock(&socket_mutex);
				server_refresh();
			}

		} else {
			server_close_cmd();
		}
	}

	pthread_mutex_unlock(&mutex);
	UNREFERRED_PARAMETER(args);
	return NULL;
}

int conn_auth(ssh_session session, const char *user,
              const char *pass, void *userdata)
{
	char username[MAX_USERNAME_LEN];
	char passwd[MAX_PASSWD_LEN];
	psession_t p_session;

	p_session = (psession_t)userdata;

	cfg_get_username(username, MAX_USERNAME_LEN);
	cfg_get_passwd(passwd, MAX_PASSWD_LEN);

	if(strcmp(username, user) != 0) {
		p_session->tried_time++;
		return SSH_AUTH_DENIED;
	}

	if(strcmp(passwd, pass) != 0) {
		p_session->tried_time++;
		return SSH_AUTH_DENIED;
	}

	p_session->authed = true;

	UNREFERRED_PARAMETER(session);
	return SSH_AUTH_SUCCESS;
}

int conn_receive(ssh_session session, ssh_channel channel,
                 void *data, uint32_t len, int is_stderr,
                 void *userdata)
{
	psession_t p_session;

	p_session = (psession_t)userdata;

	if(p_session->status == SESSION_ACTIVE) {
		if(*((char*)data) == '\r') {
			server_write("\n", 2);

		} else {
			server_write(data, len);
		}


	} else {
		pthread_mutex_lock(&mutex);
		ssh_channel_write(channel, WAIT_TEXT, sizeof(WAIT_TEXT) - 1);
		p_session = (psession_t)(session_queue->p_item);

		if(p_session->status == SESSION_ACTIVE) {
			ssh_channel_write(p_session->channel, URGENCY_TEXT,
			                  sizeof(URGENCY_TEXT) - 1);
		}

		pthread_mutex_unlock(&mutex);
	}

	UNREFERRED_PARAMETER(session);
	UNREFERRED_PARAMETER(channel);
	UNREFERRED_PARAMETER(is_stderr);

	return len;
}

void sigroutine(int dunno)
{
	UNREFERRED_PARAMETER(dunno);
	return;
}

void sigterm(int dunno)
{
	network_quit();
	UNREFERRED_PARAMETER(dunno);
	return;
}
