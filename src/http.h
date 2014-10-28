/**
 * SCOUT http header file
 *
 * Copyright (C) 2000, 2001, 2002 by
 * Jeffrey Fulmer - <jdfulmer@armstrong.com>
 * This file is distributed as part of Scout
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
 */

#ifndef HTTP_H
#define HTTP_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif /* HAVE_CONFIG_H */

#include <stdio.h>
#include <stdlib.h>
#include <sock.h>
#include <queue.h>
#include <joedog/joedog.h>

#ifndef HAVE_SNPRINTF
# define portable_snprintf  snprintf
# define portable_vsnprintf vsnprintf
#endif

typedef struct headers
{
  char                head[64];          /* http response       */
  int                 code;              /* http return code    */
  unsigned long int   length;
  char                cookie[128];       /* set-cookie data     */
  char                redirection[128];  /* redirection URL     */
  struct{
    int   www;                           /* www auth challenge  */
    int   proxy;                         /* proxy auth challenge*/
    struct{
      char  www[64];                     /* www auth realm      */
      char  proxy[64];                   /* proxy auth realm    */
    } realm;
    struct{
      TYPE  www;
      TYPE  proxy;
    } type;
  } auth;
  int                 keepalive;         /* http keep-alive     */
} HEADERS;

typedef struct page
{ 
  int                 bytes;
  char                *page;
} PAGE;

struct result
{
  long total;
};

/**
 * construct a HTTP GET request with "my" 
 * data and send it to the server host.
 */
void SCOUThttp_send( CONN *C, char *host, char *path );

/**
 * construct a HTTP POST request with "my"
 * data and send it to server host.
 */
void SCOUThttp_post( CONN *C, char *host, char *path, char *data, size_t len );

/**
 * read http headers
 * int socket
 */
HEADERS * SCOUThttp_read_headers( CONN *C, char * );

/**
 * read http content
 * int socket
 */
PAGE * SCOUThttp_read( CONN *C, int len );

#endif /* HTTP_H */

