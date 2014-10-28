/**
 * HTTP/HTTPS client support
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
 * 
 */
#include <setup.h>
#include <client.h>
#include <signal.h>
#include <sock.h>
#include <http.h>
#include <auth.h>
#include <url.h>
#include <queue.h>

/**
 * local prototypes
 */ 
int http_request( CLIENT *client, URL *U ); 
void clean_up(); 
static void signal_handler( int sig );
static void signal_init();

static void
signal_handler( int sig )
{
  return;
}
 
static void
signal_init()
{
  struct sigaction sa;
 
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);
}
   

/**
 * thread entry point for cancellable friendly
 * operating systems.
 */
void * 
http_client( CLIENT *client )
{
  URL U;
  char *tmp;
  int x, y;

  U = add_url( my.url );
  my.hostname = U.hostname;
  
  http_request( client, &U ); 

  y = my.count;
  for( x = 0; x < y; x++ ){
    URL  uri;
    char *str;
    str = pop_queue(); 
    if( !str ) return NULL;
    uri = add_url( str );
    if( uri.hostname != NULL )
      http_request( client, &uri );
    else
      continue;
    y = my.count;
  } 

  return( NULL );
} /*END CANCEL_CLIENT*/

int
http_request( CLIENT *client, URL *U )
{
  int retry = 0, okay = 0;    /* flow of control vars    */
  int bytes, code, fail;      /* statistics quantifiers  */
  float etime;                /* trans. elapsed time     */
  clock_t start, stop;        /* time structs for elaps. */
  struct tms t_start, t_stop; /* time structs for elaps. */
  CONN    *C;                 /* connection structure    */
  HEADERS *head;              /* HTTP header structure   */ 
  URL redirect_url;           /* URL in redirect request */ 
  PAGE    *page;

  C = malloc( sizeof( CONN ));
  C->pos_ini         = 0;
  C->inbuffer        = 0;
  C->auth.www        = client->auth.www;
  C->auth.proxy      = client->auth.proxy;
  C->auth.type.www   = client->auth.type.www;
  C->auth.type.proxy = client->auth.type.proxy; 

  if( U->protocol == UNSUPPORTED ){ 
    if( my.verbose ){
      printf(
        "%s %d %6.2f secs: %7d bytes ==> %s\n",
        "UNSPPRTD", 501, 0.00, 0, "PROTOCOL NOT SUPPORTED BY SCOUT" 
      );
    } /* end if my.verbose */
  }

  C->prot = U->protocol;

  /* record transaction start time */
  start = times( &t_start );  
 
  if( my.debug ){
    printf(
      "attempting connection to %s:%d\n",
      (my.proxy.required==TRUE)?my.proxy.hostname:U->hostname,
      (my.proxy.required==TRUE)?my.proxy.port:U->port
    );
    fflush( stdout );
  } 
  
  do{
    if( my.proxy.required ){
      C->sock = SCOUTsocket( C, my.proxy.hostname, my.proxy.port );
    }
    else{
      C->sock = SCOUTsocket( C, U->hostname, U->port );
    }

    if( C->sock < 0 ){
      if( my.debug ){ printf( "connection failed.\n" ); fflush( stdout ); } 
      retry ++; 
      okay = 0;
      fail = 1;
      C->sock = 0;
    }  
    else{ okay = 1; }
  } while( !okay && retry < 3 );
  if(  !okay   ){ return -1; }
  if( my.debug ){ printf( "connention made.\n" ); fflush( stdout ); } 
 
  /**
   * write to socket with a POST or GET
   */
  if( U->calltype == URL_POST ) 
    SCOUThttp_post( 
      C, U->hostname, U->pathname, U->postdata, U->postlen 
    );
  else
    SCOUThttp_send( 
      C, U->hostname, U->pathname
    );
  
  /**
   * read from socket and collect statistics.
   */
  head      =  SCOUThttp_read_headers( C, U->hostname ); 
  if(!head) {  SCOUTclose( C ); free( C ); return -1; } 
  page      =  SCOUThttp_read( C, 0 ); 
  if(!page) {  SCOUTclose( C ); free( C ); free( head ); return -1; }
  stop      =  times( &t_stop ); 
  etime     =  elapsed_time( stop - start );  
  code      =  (head->code <  400 || head->code == 401 || head->code == 407) ? 1 : 0;
  fail      =  (head->code >= 400 && head->code != 401 && head->code != 407) ? 1 : 0; 
  U->status =  head->code;
  set_node( U->abs_url, head->code ); 

  /**
   * verbose output, print statistics to stdout
   */
  if(( my.verbose )&&( our.shutting_down != TRUE )&&( !my.debug )){
    printf(
      "%s %d %6.2f secs: %7d bytes ==> %s\n",
      head->head, head->code, etime, page->bytes, U->pathname 
    );
  }
  parse_text( U->pathname, page->page );

  /**
   * close the socket and free memory.
   */
  SCOUTclose( C );
  free( C ); 

  /**
   * deal with HTTP > 300
   */
  switch( head->code ){
  case 301:
  case 302:
    if( head->redirection[0] ){
      if( my.debug ){ printf("parsing redirection URL %s\n", head->redirection); }
      if (protocol_length(head->redirection) == 0) {
          memcpy(&redirect_url, &U, sizeof(U));
          redirect_url.pathname = head->redirection;
      } else {
        redirect_url = add_redirect( head->redirection, U->port );
      }
      http_request( client, &redirect_url );
    }
    break; 
  case 401:
    /**
     * WWW-Authenticate challenge from the WWW server
     */
    client->auth.www = (client->auth.www==0)?1:client->auth.www;
    if(( client->auth.bids.www++ ) < my.bids - 1 ){
      if( head->auth.type.www == DIGEST ){
        if( my.debug ){fprintf(stderr,"oops! digest...\n"); fflush(stderr);}
        client->auth.type.www =  DIGEST;
        break;
      }
      if( head->auth.type.www == BASIC ){
        client->auth.type.www =  BASIC;
        set_authorization(WWW, head->auth.realm.www);
      }
      http_request( client, U );
    }
    break;
  case 407:
    /**
     * Proxy-Authenticate challenge from the proxy server.
     */
    client->auth.proxy = (client->auth.proxy==0)?1:client->auth.proxy;
    if(( client->auth.bids.proxy++ ) < my.bids - 1 ){
      if( head->auth.type.proxy == DIGEST ){
        client->auth.type.proxy =  DIGEST;
        break;
      }
      if( head->auth.type.proxy == BASIC  ){
        client->auth.type.proxy =  BASIC;
        set_authorization(PROXY, head->auth.realm.proxy);
      }
      http_request( client, U );
    }
    break;
  default:
    break;
  } 
  
  return 0;
}

