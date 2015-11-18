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
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include "../common/common.h"
#include "../log/log.h"
#include "../config/config.h"
#include "network.h"
#include "ssh.h"
#include "session.h"
#include "connections.h"

#define	MAX_TRY_TIME	5
#define	SYS_RSA_KEY		"/etc/ssh/ssh_host_rsa_key"

static	queue_t			session_queue;
static	bool			run_flag;
static	pthread_mutex_t	mutex;
static	pthread_mutex_t	socket_mutex;
static	pthread_mutex_t	cond_mutex;
static	pthread_t		gc_thread_id;
static	pthread_cond_t	cond;
static	pthread_t		main_thread;

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
	sh_bind bind;
	int num;
	int port;
	void* ret;
	struct sigaction action;

	main_thread = pthread_self();

	//Setup awake signal
	action.sa_handler = sigroutine;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGCONT, &action, NULL);

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

	//Create gc thread
	if(pthread_create(&gc_thread_id,
	                  NULL, gc_thread, NULL) != 0) {
		printlog(LOG_CONN, "Failed to create gc thread!\n");
		ssh_bind_free(bind);
		return -1;
	}

	//Listen for new connection
	ssh_bind_set_blocking(bind, 1);
	printlog(LOG_CONN, "Start listen on port: %d\n", port);
	run_flag = true;

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
		queue_push(p_session);
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
	}

	//Join gc thread
	pthread_join(gc_thread_id, &ret);

	//Destroy queue
	queue_destroy(session_queue, session_destroy_callback, NULL);
	ssh_bind_free(bind);
	ssh_finalize();
	return 0;
}

void network_quit()
{
	run_flag = false;
	pthread_kill(SIGCONT, main_thread);
	return;
}

void session_destroy_callback(void* p_item, void* args)
{
	void* ret;

	pthread_join(((psession_t)p_item)->thread, &ret);
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
	return channel;
}

void* session_thread(void* args)
{
	psession_t p_session;
	struct sockaddr addr;
	int socket_fd;

	p_session = (psession_t)args;

	p_session->server_callbacks = {
		.userdata = p_session,
		.auth_password_function = conn_auth,
		.channel_open_request_session_function = channel_open
	};

	p_session->channel_callbacks = {
		.channel_pty_request_function = pty_request,
		.channel_pty_window_change_function = NULL,
		.channel_shell_request_function = shell_request,
		.channel_exec_request_function = NULL,
		.channel_data_function = conn_receive,
		.channel_subsystem_request_function = NULL
	};

	//Print log
	socket_fd = ssh_get_fd(p_session->session);
	get_peername(socket_fd, &addr, sizeof(addr));
	pthread_mutex_lock(&socket_mutex);
	printlog(LOG_CONN, "Host %s has been accepted.\n",
	         inet_ntoa((struct in_addr)addr));
	pthread_mutex_unlock(&socket_mutex);

	//Server callbacks
	ssh_callbacks_init(&(p_session->server_callbacks));
	ssh_callbacks_init(&(p_session->channel_callbacks));

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
			pthread_mutex_lock(&socket_mutex);
			printlog(LOG_CONN, "Connection to %s has been lost.\n",
			         inet_ntoa((struct in_addr)addr));
			pthread_mutex_unlock(&socket_mutex);
			pthread_mutex_lock(&mutex);
			p_session->status = SESSION_EXIT;
			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&mutex);
			return NULL;
		}

		if(!p_session->tried_time > MAX_TRY_TIME) {
			pthread_mutex_lock(&socket_mutex);
			printlog(LOG_CONN, "Host %s has tried too much times.\n",
			         inet_ntoa((struct in_addr)addr));
			pthread_mutex_unlock(&socket_mutex);
			pthread_mutex_lock(&mutex);
			p_session->status = SESSION_EXIT;
			pthread_cond_signal(&cond);
			pthread_mutex_unlock(&mutex);
			return NULL;
		}
	}

	if(ssh_set_channel_callbacks(channel, &channel_cb) != SSH_OK) {
		pthread_mutex_lock(&mutex);
		p_session->status = SESSION_EXIT;
		printlog(LOG_CONN, "Failed to set channel callback.\n");
		pthread_cond_signal(&cond);
		pthread_mutex_unlock(&mutex);
		return NULL;
	}

	//Create send thread

	while(ssh_channel_is_open(channel)) {
		if(ssh_event_dopoll(event, -1) == SSH_ERROR) {
			break;
		}
	}

	pthread_mutex_lock(&socket_mutex);
	printlog(LOG_CONN, "Host %s has logged off.\n",
	         inet_ntoa((struct in_addr)addr));
	pthread_mutex_unlock(&socket_mutex);
	pthread_mutex_lock(&mutex);
	p_session->status = SESSION_EXIT;
	pthread_cond_signal(&cond);
	pthread_mutex_unlock(&mutex);

	return NULL;
}

void* gc_thread(void* args)
{
	plist_node_t p_node;
	plist_node_t p_next_node;
	psession_t p_session;
	pthread_mutex_lock(&mutex);
	void* ret;
	struct sockaddr addr;
	int socket_fd;

	while(run_flag) {
		//Suspend thread
		pthread_cond_wait(&cond, &mutex);

		//Release exited sessions
		p_node = session_queue;

		if(p_node != NULL) {
			do {
				p_session = (psession_t)(p_node->p_item);

				if(p_session->status == SESSION_EXIT) {
					//Release the session
					p_next_node = p_node->p_next;
					list_remove(&session_queue, p_node);
					pthread_join(p_session->thread, &ret);
					session_free(p_session);
					p_node = p_next_node;
					continue;
				}

				p_node = p_node->p_next;
			} while(p_node != session_queue);

			//Active first session
			p_session = (psession_t)queue_front(session_queue);

			if(p_session->status == SESSION_WAITING) {
				p_session->status = SESSION_ACTIVE;
				socket_fd = ssh_get_fd(p_session->session);
				get_peername(socket_fd, &addr, sizeof(addr));
				pthread_mutex_lock(&socket_mutex);
				printlog(LOG_CONN, "Host %s actived.\n",
				         inet_ntoa((struct in_addr)addr));
				pthread_mutex_unlock(&socket_mutex);
			}
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
		server_write(data, len);
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
