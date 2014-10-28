/**
 * FIFO queue.
 *
 * Copyright (C) 2001 Jeffrey Fulmer <jdfulmer@armstrong.com>
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
#include <setup.h>
#include <queue.h>


struct NODE *head = NULL;

/**
 * insert values into list
 */
void
add_node( char *url )
{
  struct NODE *tail = NULL;

  my.count ++;
  tail = xmalloc( sizeof( struct NODE ));
  tail->index = my.count;
  tail->url   = strdup( url );
  tail->used  = FALSE;
  tail->next  = head;
  head = tail;
}

/**
 * delete NODE whose value is *value
 */
void
delete_NODE( char *url )
{
  struct NODE *tail = NULL;
  struct NODE *p, *prev;
  p = head;
  if( strncmp( head->url, url, strlen( url )) == 0 ){
    head = head->next;
    free(p); 
  }
  if ( head == NULL ){ 
      tail = NULL;
      return;
  }
  prev = head;
  p = head->next;
  while ( p->next != NULL ){
    if( strncmp( p->url, url, strlen( url )) == 0 ){
      prev->next = p->next;
      free(p);
      return;
    }
    p = p->next;
    prev = prev->next;
  }
  
  if( p->url == url ){
    prev->next = NULL;
    tail = prev;
    free( p );
  }
}

int
find_node( char *url ) 
{
  struct NODE *p;

  if( head == NULL ){
    return 0;
  }

  p = head;
  while ( p->next != NULL ){
    if( strncmp( p->url, url, strlen( url )) == 0 ){
      return 1;
    }
    p = p->next;
  }
  return 0;
}

int
set_node( char *url, int code )
{
  struct NODE *p;
 
  if( head == NULL ){
    return 0;
  }
 
  p = head;
  while( p->next != NULL ){
    if( strncmp( p->url, url, strlen( url )) == 0 ){
      p->code = code;
      return 0;
    }
    p = p->next;
  }
  return -1;
}

char *
pop_queue()
{
  struct NODE *p = head;
  char *url;

  while( p != NULL ){
    if( p->used == FALSE ){
      url = strdup( p->url );
      p->used = TRUE;
      return( url );
    }      
    p = p->next;
  }
  return NULL;
} 
  
 
/**
 * Helper util, displays the contents of head
 */ 
void
display_queue()
{
  struct NODE *p = head;
  while( p != NULL ){
    printf("%d -> %s\n",p->used, p->url );
    p = p->next;
  }
} 

void
write_queue()
{
  char *mode;
  char tmpbuf[128];
  char myfile[64];
  FILE *fs;
  struct NODE *p = head;
 
  mode = "w";
#if 0
  mode = ( newbool > 0 ) ? "w" : "a";
  if( filebool ){
      fs = fopen( filename, mode );
  }
  else{
      fs = fopen(CNF_FILE, mode );
  }
#endif
  fs = fopen("urls.txt", mode );
 
  while( p != NULL ){
    if( p->code < 400 )
      fprintf( fs, "%s\n", p->url );
    p = p->next;
  }
 
  fclose(fs);
} 

