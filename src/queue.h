/**
 * URL Queue
 *
 * Copyright (C) 2002 Jeffrey Fulmer <jdfulmer@armstrong.com>
 * This file is part of Scout
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * --
 *
 */

#ifndef QUEUE_H
#define QUEUE_H

struct NODE
{
  int         index;
  int         used;
  int         code;
  char        *url;
  struct NODE *next;
} NODE;
 
EXTERN struct NODE *head;

void add_node( char *value );
char *pop_queue();
int  find_node( char *url );
int  set_node( char *url, int code ); 
void display_list();
void write_queue();

#endif /* QUEUE_H */
