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

#include "./config/config.h"

#define	LOCK_FILE	"/tmp/mcserverd.lck"

static	void	run_as_daemon();

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
	//Change to daemon process
	//Initialize server
	//Initialize network
	//Server main

	cfg_destroy();
	lockf(lock_fd, F_ULOCK, 0);
	close(lock_fd);

	return 0;
}

void run_as_daemon()
{

}
