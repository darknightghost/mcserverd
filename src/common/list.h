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

#ifndef LIST_H_INCLUDE
#define	LIST_H_INCLUDE

#include "common.h"

//void	item_destroy_callback(void* p_item,void* args);
typedef		void	(*item_destroy_callback)(void*, void*);

typedef	struct	_list_node_t {
	struct _list_node_t*	p_prev;
	struct _list_node_t*	p_next;
	void*					p_item;
} list_node_t, *plist_node_t, *list_t, **plist_t;

#define			list_init(a)	(a) = NULL
plist_node_t	list_insert_before(
    plist_t p_list,
    plist_node_t position,		//Null if at the start of the list_t
    void* p_item);
plist_node_t	list_insert_after(
    plist_t p_list,
    plist_node_t position,		//Null if at the end of the list_t
    void* p_item);
void			list_remove(plist_t p_list, plist_node_t p_node);
plist_node_t	list_get_node_by_item(list_t list, void* p_item);
void			list_destroy(plist_t p_list,
                             item_destroy_callback callback,
                             void* p_arg);

#endif	//!	LIST_H_INCLUDE
