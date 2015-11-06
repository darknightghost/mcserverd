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

#include "list.h"
#include <stdlib.h>

plist_node_t list_insert_before(
    plist_t p_list,
    plist_node_t position,		//Null if at the start of the list_t
    void* p_item)
{
	plist_node_t p_node, p_new_node;

	//Allocate memory
	p_new_node = malloc(sizeof(list_node_t));

	if(p_new_node == NULL) {
		return NULL;
	}

	p_new_node->p_item = p_item;

	if(*p_list == NULL) {
		//Insert the new item as the first item
		p_new_node->p_prev = p_new_node;
		p_new_node->p_next = p_new_node;
		*p_list = p_new_node;

	} else if(position == NULL) {
		//Insert the new item as the first item
		p_node = *p_list;
		p_new_node->p_prev = p_node->p_prev;
		p_new_node->p_next = p_node;
		p_node->p_prev->p_next = p_new_node;
		p_node->p_prev = p_new_node;
		*p_list = p_new_node;

	} else {
		//Insert the new item before position
		p_node = position;
		p_new_node->p_prev = p_node->p_prev;
		p_new_node->p_next = p_node;
		p_node->p_prev->p_next = p_new_node;
		p_node->p_prev = p_new_node;

	}

	return p_new_node;
}

plist_node_t list_insert_after(
    plist_t p_list,
    plist_node_t position,		//Null if at the end of the list_t
    void* p_item)
{
	plist_node_t p_node, p_new_node;

	//Allocate memory
	p_new_node = malloc(sizeof(list_node_t));

	if(p_new_node == NULL) {
		return NULL;
	}

	p_new_node->p_item = p_item;

	if(*p_list == NULL) {
		//Insert the new item as the first item
		p_new_node->p_prev = p_new_node;
		p_new_node->p_next = p_new_node;
		*p_list = p_new_node;

	} else {
		if(position == NULL) {
			p_node = (*p_list)->p_prev;

		} else {
			p_node = position;
		}

		//Insert p_new_node after p_node
		p_new_node->p_next = p_node->p_next;
		p_new_node->p_prev = p_node;
		p_node->p_next->p_prev = p_new_node;
		p_node->p_next = p_new_node;

	}

	return p_new_node;
}

void list_remove(plist_t p_list, plist_node_t p_node)
{
	if(*p_list == NULL) {
		return;

	} else if(*p_list == p_node) {
		if(p_node->p_prev == p_node) {
			free(p_node);
			*p_list = NULL;
			return;

		} else {
			*p_list = p_node->p_next;
		}
	}

	p_node->p_prev->p_next = p_node->p_next;
	p_node->p_next->p_prev = p_node->p_prev;
	free(p_node);
	return;
}

plist_node_t list_get_node_by_item(list_t list, void* p_item)
{
	plist_node_t p_node;

	if(list == NULL) {
		return NULL;
	}

	p_node = list;

	do {
		if(p_node->p_item == p_item) {
			return p_node;
		}

		p_node = p_node->p_next;
	} while(p_node != list);

	return NULL;
}

void list_destroy(plist_t p_list,
                  item_destroy_callback callback,
                  void* p_arg)
{
	while(*p_list != NULL) {
		if(callback != NULL) {
			callback((*p_list)->p_item, p_arg);
		}

		rtl_list_remove(p_list, *p_list);
	}

	return;
}

