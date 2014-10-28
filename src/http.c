/**
 * HTTP/HTTPS protocol support 
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
#include <setup.h>
#include <http.h>
#include <auth.h>
#include <cookie.h>
#include <string.h>
/**
 * returns int, ( < 0 == error )
 * formats and sends an HTTP/1.0 request
 */
void
SCOUThttp_send( CONN *C, char *host, char *path )
{
  int rlen;
  char *protocol; 
  char *keepalive;
  char cookie[MAX_COOKIE_SIZE];
  char authwww[128];
  char authpxy[128];
  char request[1024]; 

  /* Request path based on proxy settings */
  char fullpath[2048];
  if( my.proxy.required ){
    sprintf( fullpath, "%s://%s%s", C->prot == 0?"http":"https", host, path );
  } 
  else{
    sprintf( fullpath, "%s", path );
  }

  memset( cookie,  0, sizeof( cookie ));
  memset( request, 0, sizeof( request ));

  /* HTTP protocol string */
  protocol  = (my.protocol == TRUE)?"HTTP/1.1":"HTTP/1.0";
  keepalive = (my.keepalive == TRUE)?"keep-alive":"close";
  get_cookie( 1, host, cookie ); 
  if( C->auth.www ){
    rlen = snprintf(
      authwww, sizeof(authwww),
      "Authorization: %s %s\015\012",
      (C->auth.type.www==BASIC)?"Basic":"Digest", my.auth.encode
    );
  }
  if( C->auth.proxy ){
    rlen = snprintf(
      authpxy, sizeof(authpxy),
      "Proxy-Authorization: %s %s\015\012",
      (C->auth.type.www==BASIC)?"Basic":"Digest", my.proxy.encode
    );
  }

  /** 
   * build a request string to pass to the server      
   */
  rlen = snprintf(
    request, sizeof( request ),
    "GET %s %s\015\012"
    "Host: %s\015\012"
    "%s"
    "%s"
    "Cookie: %s\015\012"
    "Accept: */*\015\012"
    "Accept-Encoding: * \015\012"
    "User-Agent: %s\015\012%s"
    "Connection: %s\015\012\015\012",
    fullpath, protocol, host,
    (C->auth.www==TRUE)?authwww:"",
    (C->auth.proxy==TRUE)?authpxy:"",
    cookie, my.uagent, my.extra, keepalive
  ); 
  
  
  if( my.debug ){ printf("%s\n", request); fflush(stdout); }
  if( rlen<0 || rlen>sizeof(request)  ){ joe_fatal("http_send: request buffer overrun!"); }

  if(( SCOUTsocket_check( C, WRITE )) < 0 ){
    joe_error( "SCOUTsocket: not writeable" );
    return;
  } 

  SCOUTsocket_write( C, request, rlen );

  return;
}

/**
 * returns int, ( < 0 == error )
 * formats and sends an HTTP/1.0 request
 */
void
SCOUThttp_post( CONN *C, char *host, char *path, char *data, size_t len )
{
  int  rlen;
  char authwww[128];
  char authpxy[128]; 
  char request[1024]; 
  char *protocol; 
  char *keepalive;

  /* cookie value  */
  char cookie[MAX_COOKIE_SIZE];

  /* Request path based on proxy settings */
  char fullpath[2048];
  if( my.proxy.required ){
    sprintf( fullpath, "%s://%s%s", C->prot == 0?"http":"https", host, path );
  }
  else{
    sprintf( fullpath, "%s", path );
  }

  memset( cookie,  0, sizeof( cookie ));
  memset( request, 0, sizeof( request ));

  /* HTTP protocol string */
  protocol  = (my.protocol  == TRUE)?"HTTP/1.1":"HTTP/1.0";
  keepalive = (my.keepalive == TRUE)?"keep-alive":"close";
  get_cookie( 1, host, cookie );
  if( C->auth.www ){
    rlen = snprintf(
      authwww, sizeof(authwww),
      "Authorization: %s %s\015\012",
      (C->auth.type.www==BASIC)?"Basic":"Digest", my.auth.encode
    );
  }
  if( C->auth.proxy ){
    rlen = snprintf(
      authpxy, sizeof(authpxy),
      "Proxy-Authorization: %s %s\015\012",
      (C->auth.type.www==BASIC)?"Basic":"Digest", my.proxy.encode
    );
  } 
    
  /** 
   * build a request string to pass to the server       
   */
  rlen = snprintf(
    request, sizeof( request ),
    "POST %s %s\015\012"
    "Host: %s\015\012"
    "%s"
    "%s"
    "Cookie: %s\015\012"
    "Accept: */*\015\012"
    "Accept-Encoding: * \015\012"
    "User-Agent: %s\015\012%s"
    "Connection: %s\015\012"
    "Content-type: application/x-www-form-urlencoded\015\012"
    "Content-length: %d\015\012\015\012"
    "%*.*s\015\012",
    fullpath, protocol, host,
    (C->auth.www==TRUE)?authwww:"",
    (C->auth.proxy==TRUE)?authpxy:"",
    cookie, my.uagent, my.extra, keepalive,
    len, len, (int)len, data
  ); 

  if( my.debug ){ printf("%s\n", request); fflush(stdout); }
  if( rlen<0 || rlen>sizeof(request) ){ joe_fatal("http_post: request buffer overrun!"); }

  if(( SCOUTsocket_check( C, WRITE )) < 0 ){
    joe_error( "SCOUTsocket: not writeable" );
    return;
  } 

  SCOUTsocket_write( C, request, rlen );

  return;
}

/**
 * returns HEADERS struct
 * reads from http/https socket and parses
 * header information into the struct.
 */
HEADERS *
SCOUThttp_read_headers( CONN *C, char *host )
{ 
  int  x;           /* while loop index      */
  int  n;           /* assign socket_read    */
  char c;           /* assign char read      */
  char line[512];   /* assign chars read     */
  HEADERS *h;       /* struct to hold it all */
  h = (HEADERS*)malloc( sizeof(HEADERS));
  memset( h, 0, sizeof( HEADERS ));

  if(( SCOUTsocket_check( C, READ )) < 0 ){
    joe_error( "SCOUTsocket: not readable" );
    return NULL;
  } 

  h->redirection[0]=0;

  while( TRUE ){
    x = 0;
    memset( &line, 0, sizeof( line ));
    while(( n = SCOUTsocket_read( C, &c, 1 )) == 1 ){
      line[x] = c; 
      if(( line[0] == '\n' ) || ( line[1] == '\n' )){ 
        return h;
      }
      /* if( line[x] == '\n' ){ *line=0; break; } */
      if( line[x] == '\n' ){ break; }
      x ++;
    }
    if( strncasecmp( line, "http", 4 ) == 0 ){
      strncpy( h->head, line, 8 );
      h->code = atoi( line + 9 ); 
    }
    if( strncasecmp( line, "content-length: ", 16 ) == 0 ){ 
      h->length = atol( line + 16 ); 
    }
    if( strncasecmp( line, "set-cookie: ", 12 ) == 0 ){
      if( my.cookies ){
        memset( h->cookie, 0, sizeof( h->cookie ));
        strncpy( h->cookie, line+12, strlen( line ));
        add_cookie( 1, host, h->cookie );
      }
    }
    if( strncasecmp( line, "connection: ", 12 ) == 0 ){
      if ( strncasecmp( line+12, "keep-alive", 10 ) == 0 ){
        h->keepalive = 1;
      }
      else if( strncasecmp( line+12, "close", 5 ) == 0 ){
        h->keepalive = 0;
      }
    }
    if( strncasecmp(line, "location: ", 10) == 0) {
      if (strlen(line) - 10 > sizeof( h->redirection ) - 1) {
        joe_warning( "redirection URL too long, ignored");
      }
      else {
        strncpy( h->redirection, line+10,  sizeof( h->redirection ));
      }
    }
    if( strncasecmp(line, "www-authenticate: ", 18 ) == 0 ){
      char *tmp;
      h->auth.www = TRUE;
      if( strncasecmp( line+18, "digest", 6 ) == 0 ){
        h->auth.type.www = DIGEST;
      }
      else{
        h->auth.type.www = BASIC;
      }
      tmp = strchr( line, '=' );
      tmp++;
      if( tmp[0] == '"' ){ tmp++; tmp[strlen(tmp)-1] = '\0'; }
      strncpy( h->auth.realm.www, tmp, strlen( tmp ));
    }
    if( strncasecmp(line, "proxy-authenticate: ", 20 ) == 0 ){
      char *tmp;
      h->auth.proxy = TRUE;
      if( strncasecmp( line+18, "digest", 6 ) == 0 ){
        h->auth.type.proxy = DIGEST;
      }
      else{
        h->auth.type.proxy = BASIC;
      }
      tmp = strchr( line, '=' );
      tmp++;
      if( tmp[0] == '"' ){ tmp++; tmp[strlen(tmp)-1] = '\0'; }
      strncpy( h->auth.realm.proxy, tmp, strlen( tmp ));
    } 

    if( n <  0 ){ 
      joe_error("SCOUThttp_read_headers"); 
      return( NULL ); 
    } /* socket closed */
  } /* end of while TRUE */

  return h;
}

/**
 * returns int
 * reads a http/https socket
 * ( you know what I mean :)
 */
PAGE *
SCOUThttp_read( CONN *C, int len )
{ 
  int  n;
  size_t bytes;
  char body[MAXFILE];
  PAGE *p; 
  p = (PAGE*)malloc( sizeof(PAGE));
  memset( p, 0, sizeof( PAGE )); 

  if(( SCOUTsocket_check( C, READ )) < 0 ){
    joe_error( "SCOUTsocket: not readable" );
    return NULL;
  } 

  memset( body, 0, MAXFILE );  
  while( TRUE ){
    if(( n = SCOUTsocket_read( C, body, MAXFILE)) == 0 ){
      break;
    } 
    bytes = n;
  }
  p->bytes = bytes;
  p->page  = strdup( body ); 
  return p;
}

