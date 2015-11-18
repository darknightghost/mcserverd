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
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#include "common/common.h"
#include "config/config.h"
#include "log/log.h"
#include "server/server.h"
#include "network/network.h"

#define	LOCK_FILE	"/tmp/mcserverd.lck"

int main(int argc, char* argv[])
{
	int lock_fd;

	//Check privilege
	if(geteuid() != 0) {
		printf("This daemon can only run as root!\n");
		return -1;
	}

	//Check lock
	lock_fd = open(LOCK_FILE, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);

	if(lock_fd == -1) {
		printf("Unable to open lock file!\n");
		return -1;
	}

	if(lockf(lock_fd, F_TLOCK, 0) != 0) {
		close(lock_fd);
		return 0;
	}

	//Load config file
	if(!cfg_init()) {
		printf("Failed to load config file!\n");
		lockf(lock_fd, F_ULOCK, 0);
		close(lock_fd);
		return -1;
	}

	//Initialize logging
	if(!log_init()) {
		printf("Failed to initialize logging!\n");
		cfg_destroy();
		lockf(lock_fd, F_ULOCK, 0);
		close(lock_fd);
		return -1;
	}

	//Change to daemon process
	if(daemon(0, 1) != 0) {
		printf("Failed to run in background!\n");
		log_close();
		cfg_destroy();
		lockf(lock_fd, F_ULOCK, 0);
		close(lock_fd);
		return -1;
	}

	//Initialize server
	if(!server_init()) {
		printlog(LOG_SERVER, "Failed to initialize server!\n");
		log_close();
		cfg_destroy();
		lockf(lock_fd, F_ULOCK, 0);
		close(lock_fd);
		return -1;
	}

	if(!server_start()) {
		printlog(LOG_SERVER, "Failed to start server!\n");
		server_destroy();
		log_close();
		cfg_destroy();
		lockf(lock_fd, F_ULOCK, 0);
		close(lock_fd);
		return -1;
	}

	//Initialize network
	//Server main

	cfg_destroy();
	lockf(lock_fd, F_ULOCK, 0);
	close(lock_fd);

	UNREFERRED_PARAMETER(argc);
	UNREFERRED_PARAMETER(argv);

	return 0;
}
