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

#include "server.h"
#include "cmdline.h"
#include "game-server.h"

bool server_init()
{
	//Start mincraft server
	game_init();

	//Initialize shell
	if(!cmdline_init()) {
		game_destroy();
		return false;
	}

	return true;
}

void server_destroy()
{
	cmdline_destroy();
	game_destroy();
	return;
}

bool server_start()
{
	return game_start();
}

void server_stop()
{
	game_stop();
	return;
}

size_t server_read(char* buf, size_t buf_size)
{
	return cmdline_read(buf, buf_size);
}

void server_write(char* buf, size_t size)
{
	cmdline_write(buf, size);
	return;
}
