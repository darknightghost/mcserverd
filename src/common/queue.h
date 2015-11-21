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

#ifndef QUEUE_H_INCLUDE
#define	QUEUE_H_INCLUDE

#include "list.h"
#include <stdlib.h>

typedef	list_t	queue_t, *pqueue_t;

#define	queue_init(queue)						list_init(queue)

#define	queue_front(queue)						((queue) == NULL\
        ?NULL\
        :(queue)->p_item)

#define	queue_back(queue)						((queue) == NULL\
        ?NULL\
        :(queue)->p_prev->p_item)

#define	queue_push(queue,p_item)				list_insert_after(&(queue),\
        NULL,\
        (p_item))

#define	queue_pop(queue,_macro_ret)						({\
		if((queue) == NULL){\
			_macro_ret = NULL;\
		}else{\
			_macro_ret = (queue)->p_item;\
			list_remove(&(queue),(queue));\
		}\
		_macro_ret;\
	})

#define	queue_destroy(queue,callback,p_args)	list_destroy(&(queue),\
        (callback),\
        (p_args))

#endif	//!	QUEUE_H_INCLUDE
