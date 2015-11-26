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

#include "ini.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static	bool			read_buf_line(FILE* fp, char** p_buf, size_t* p_len);
static	void			jump_space(char**p);
static	bool			analyse_line(pini_file_info_t p_ini_file,
                                     char* line, pini_section_t* p_p_section);
static	pini_section_t	get_section(pini_file_info_t p_ini_file, char* section,
                                    bool create);
static	pini_key_t		get_key(pini_section_t p_section, char* key,
                                bool create);
static	void			add_comment(plist_t p_list, char* comment);
static	void			add_blank(plist_t p_list);
static	void			section_destroy_callback(void* p_item, void* args);
static	void			key_destroy_callback(void* p_item, void* args);

pini_file_info_t ini_open(char* path)
{
	pini_file_info_t p_info;
	char* line_buf;
	size_t len;
	pini_section_t p_section;

	p_info = malloc(sizeof(ini_file_info_t));

	//Open ini file
	p_info->fp = fopen(path, "r+");

	if(p_info->fp == NULL) {
		free(p_info);
		return NULL;
	}

	p_info->path = malloc(strlen(path) + 1);
	strcpy(p_info->path, path);

	//Analyse ini file
	len = 256;
	line_buf = malloc(256 * sizeof(char));
	list_init(p_info->sections);
	p_section = NULL;

	while(read_buf_line(p_info->fp, &line_buf, &len)) {
		if(!analyse_line(p_info, line_buf, &p_section)) {
			ini_close(p_info);
			return NULL;
		}
	}

	free(line_buf);

	return p_info;
}

size_t ini_get_key_value(pini_file_info_t p_file,
                         char* section,
                         char* key,
                         char* buf,
                         size_t buf_size)
{
	pini_section_t p_section;
	pini_key_t p_key;
	size_t len;

	p_section = get_section(p_file, section, false);

	if(p_section == NULL) {
		return 0;
	}

	p_key = get_key(p_section, key, false);

	if(p_key == NULL) {
		return 0;
	}

	len = strlen(p_key->value) + 1;

	if(len <= buf_size) {
		strcpy(buf, p_key->value);
	}

	return len;
}

void ini_set_key_value(pini_file_info_t p_file,
                       char* section,
                       char* key,
                       char* value)
{
	pini_section_t p_section;
	pini_key_t p_key;
	plist_node_t p_node;


	if(value != NULL) {
		p_section = get_section(p_file, section, true);
		p_key = get_key(p_section, key, true);

		if(p_key->value != NULL) {
			free(p_key->value);
		}

		p_key->value = malloc(strlen(value) + 1);
		strcpy(p_key->value, value);

	} else {
		//Remove the key
		p_section = get_section(p_file, section, false);

		if(p_section == NULL) {
			return;
		}

		p_node = p_section->keys;

		if(p_node != NULL) {
			do {
				p_key = p_node->p_item;

				if(strcmp(p_key->key_name, key) == 0) {
					list_remove(&(p_section->keys), p_node);

					if(p_key->value != NULL) {
						free(p_key->value);
					}

					free(p_key->key_name);
					free(p_key);
					break;
				}

				p_node = p_node->p_next;
			} while(p_node != p_section->keys);
		}
	}

	return;
}

bool ini_sync(pini_file_info_t p_file)
{
	plist_node_t p_sec_node;
	plist_node_t p_key_node;
	pini_section_t p_section;
	pini_key_t p_key;
	pini_comment_t p_comment;

	if(p_file->fp != NULL) {
		p_file->fp = freopen(p_file->path, "w+", p_file->fp);

	} else {
		p_file->fp = fopen(p_file->path, "w+");
	}

	if(p_file->fp == NULL) {
		return false;
	}

	//Write ini file
	//Sections
	if(p_file->sections != NULL) {
		p_sec_node = p_file->sections;

		do {
			p_section = p_sec_node->p_item;

			if(p_section->line.type == INI_LINE_SECTION) {
				//Write section name
				fprintf(p_file->fp, "[%s]\n", p_section->section_name);

				//Keys
				if(p_section->keys != NULL) {
					p_key_node = p_section->keys;

					do {
						p_key = p_key_node->p_item;

						if(p_key->line.type == INI_LINE_KEY) {
							//Write key
							fprintf(p_file->fp, "%s=%s\n",
							        p_key->key_name, p_key->value);

						} else if(p_key->line.type == INI_LINE_COMMENT) {
							//Write comment
							p_comment = p_key_node->p_item;
							fprintf(p_file->fp, ";%s\n", p_comment->comment);

						} else if(p_key->line.type == INI_LINE_BLANK) {
							fprintf(p_file->fp, "\n");
						}

						p_key_node = p_key_node->p_next;
					} while(p_key_node != p_section->keys);
				}

			} else if(p_section->line.type == INI_LINE_COMMENT) {
				//Write comment
				p_comment = p_sec_node->p_item;
				fprintf(p_file->fp, ";%s\n", p_comment->comment);

			} else if(p_section->line.type == INI_LINE_BLANK) {
				fprintf(p_file->fp, "\n");
			}


			p_sec_node = p_sec_node->p_next;
		} while(p_sec_node != p_file->sections);
	}

	fflush(p_file->fp);

	return true;
}

void ini_close(pini_file_info_t p_file)
{
	list_destroy(&(p_file->sections), section_destroy_callback, NULL);
	free(p_file->path);

	if(p_file->fp != NULL) {
		fclose(p_file->fp);
	}

	return;
}

bool read_buf_line(FILE* fp, char** p_buf, size_t* p_len)
{
	char* p;
	char* old_buf;

	//Read line
	if(fgets(*p_buf, *p_len, fp) == NULL) {
		return false;
	}

	//If the size of bufer is not enough
	for(p = *p_buf + strlen(*p_buf) - 1;
	    *p != '\n';) {

		//Allocate a bigger buffer
		old_buf = *p_buf;
		(*p_len) *= 2;
		*p_buf = malloc(*p_len * 2);
		strcpy(*p_buf, old_buf);
		free(old_buf);

		//Read file
		p = *p_buf + strlen(*p_buf);

		if(fgets(p, *p_len / 2, fp) == NULL) {
			p = *p_buf + strlen(*p_buf) - 1;
			break;
		}

	}

	if(*p == '\n') {
		*p = '\0';
	}

	return true;
}

void jump_space(char**p)
{
	while(**p == ' '
	      ||**p == '\t') {
		(*p)++;
	}

	return;
}

bool analyse_line(pini_file_info_t p_ini_file,
                  char* line, pini_section_t* p_p_section)
{
	char* p;
	char* p_end;
	pini_key_t p_key;

	p = line;
	jump_space(&p);

	if(*p == '\0') {
		if(*p_p_section == NULL) {
			add_blank(&(p_ini_file->sections));

		} else {
			add_blank(&((*p_p_section)->keys));
		}

		return true;

	} else if(*p == '[') {
		//Section
		p++;

		for(p_end = p; *p_end != ']'; p_end++) {
			if(*p_end == '\0' || *p_end == ';' || *p_end == '['
			   || *p_end == '=') {
				return false;
			}
		}

		*p_end = '\0';
		*p_p_section = get_section(p_ini_file, p, true);

	} else if(*p == ';') {
		//Comment
		p++;

		if(*p_p_section == NULL) {
			add_comment(&(p_ini_file->sections), p);

		} else {
			add_comment(&((*p_p_section)->keys), p);
		}

	} else if(*p != ';' && *p != '=' && *p != '[' && *p != ']') {
		//Key
		if(*p_p_section == NULL) {
			return false;
		}

		for(p_end = p;
		    *p_end != ' ' && *p_end != '\t' && *p_end != '=';
		    p_end++) {
			if(*p_end == '\0') {
				return false;
			}
		}

		//Value
		if(*p_end != '=') {
			*p_end = '\0';
			jump_space(&p_end);
			p_end++;

			if(*p_end != '=') {
				return false;
			}

			*p_end = '\0';

		} else {
			*p_end = '\0';
		}

		p_end++;
		jump_space(&p_end);

		//Get key
		if(*p == '\0') {
			return false;
		}

		p_key = get_key(*p_p_section, p, true);

		//Set value
		p = p_end;

		if(p_key->value != NULL) {
			free(p_key->value);
		}

		p_key->value = malloc(strlen(p) + 1);
		strcpy(p_key->value, p);

	} else {
		return false;
	}

	return true;
}

pini_section_t get_section(pini_file_info_t p_ini_file, char* section,
                           bool create)
{
	pini_section_t ret;
	plist_node_t p_node;
	pini_section_t p_sec;

	ret = NULL;

	//Look for section
	p_node = p_ini_file->sections;

	if(p_node != NULL) {
		do {
			p_sec = p_node->p_item;

			if(p_sec->line.type == INI_LINE_SECTION
			   && strcmp(p_sec->section_name, section) == 0) {
				ret = p_sec;
				break;
			}

			p_node = p_node->p_next;
		} while(p_node != p_ini_file->sections);
	}

	if(ret == NULL && create) {
		//Create new section
		ret = malloc(sizeof(ini_section_t));
		ret->section_name = malloc(strlen(section) + 1);
		ret->line.type = INI_LINE_SECTION;
		strcpy(ret->section_name, section);
		list_init(ret->keys);
		list_insert_after(&(p_ini_file->sections), NULL, ret);
	}

	return ret;
}

pini_key_t get_key(pini_section_t p_section, char* key, bool create)
{
	pini_key_t p_key;
	pini_key_t ret;
	plist_node_t p_node;

	ret = NULL;

	//Look for key
	p_node = p_section->keys;

	if(p_node != NULL) {
		do {
			p_key = p_node->p_item;

			if(p_key->line.type == INI_LINE_KEY
			   && strcmp(p_key->key_name, key) == 0) {
				ret = p_key;
				break;
			}

			p_node = p_node->p_next;
		} while(p_node != p_section->keys);
	}

	if(ret == NULL && create) {
		//Create new section
		ret = malloc(sizeof(ini_key_t));
		ret->key_name = malloc(strlen(key) + 1);
		ret->line.type = INI_LINE_KEY;
		strcpy(ret->key_name, key);
		list_insert_after(&(p_section->keys), NULL, ret);
		ret->value = NULL;
	}

	return ret;
}

void add_comment(plist_t p_list, char* comment)
{
	pini_comment_t p_comment;

	p_comment = malloc(sizeof(ini_comment_t));
	p_comment->line.type = INI_LINE_COMMENT;
	p_comment->comment = malloc(strlen(comment) + 1);
	strcpy(p_comment->comment, comment);

	list_insert_after(p_list, NULL, p_comment);
	return;
}

void add_blank(plist_t p_list)
{
	pini_line_t p_line;

	p_line = malloc(sizeof(ini_line_t));
	p_line->type = INI_LINE_BLANK;

	list_insert_after(p_list, NULL, p_line);
	return;
}

void section_destroy_callback(void* p_item, void* args)
{
	pini_section_t p_section;
	pini_comment_t p_comment;

	p_section = p_item;

	if(p_section->line.type == INI_LINE_SECTION) {
		list_destroy(&(p_section->keys), key_destroy_callback, NULL);
		free(p_section->section_name);
		free(p_section);

	} else if(p_section->line.type == INI_LINE_COMMENT) {
		p_comment = p_item;
		free(p_comment->comment);
		free(p_comment);
	}

	UNREFERRED_PARAMETER(args);
	return;
}

void key_destroy_callback(void* p_item, void* args)
{
	pini_key_t p_key;
	pini_comment_t p_comment;

	p_key = p_item;

	if(p_key->line.type == INI_LINE_KEY) {
		if(p_key->value != NULL) {
			free(p_key->value);
		}

		free(p_key->key_name);
		free(p_key);

	} else if(p_key->line.type == INI_LINE_COMMENT) {
		p_comment = p_item;
		free(p_comment->comment);
		free(p_comment);
	}

	UNREFERRED_PARAMETER(args);
	return;
}
