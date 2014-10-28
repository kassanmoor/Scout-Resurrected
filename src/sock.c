/**
 * SCOUT socket library
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
#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

#include <setup.h> 
 
#include <sock.h>
#include <joedog/joedog.h>
#include <fcntl.h>

#ifdef  HAVE_UNISTD_H
# include <unistd.h>
#endif/*HAVE_UNISTD_H*/

#ifdef  HAVE_ARPA_INET_H
# include <arpa/inet.h>
#endif/*HAVE_ARPA_INET_H*/
 
#ifdef  HAVE_SYS_SOCKET_H
# include <sys/socket.h>
#endif/*HAVE_SYS_SOCKET_H*/ 

#ifdef  HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif/*HAVE_NETINET_IN_H*/
 
#ifdef  HAVE_NETDB_H
# include <netdb.h>
#endif/*HAVE_NETDB_H*/

#ifdef  HAVE_SSL
# include <openssl/rand.h>
#endif/*HAVE_SSL*/

/** 
 * local prototypes 
 */
int mknblock( int socket, int nonblock );
struct hostent *SCOUTgethostbyname( const char *hostname );
ssize_t socket_write( int sock, const void *vbuf, size_t len );  
ssize_t ssl_socket_write( CONN *C, const void *vbuf, size_t len );

/**
 * SCOUTsocket
 * returns int, socket handle
 */
int
SCOUTsocket( CONN *C, const char *hn, int port )
{
  int conn;
  int serr; 
  struct timeval timeout;
  int res;
  int herrno;
  struct linger      ling;
  struct sockaddr_in cli; 
  struct hostent     *hp;
  struct hostent     hent; 
#if defined(_AIX)
  char *aixbuf;
  int  rc;
#endif/*_AIX*/ 
  char buf[1024]; 
  char hbf[9000]; 
  C->sock = -1;
#ifdef  HAVE_SSL
  C->ssl  = NULL;
  C->ctx  = NULL;
#endif/*HAVE_SSL*/
  
  if( C->prot == HTTPS ){
    #ifdef HAVE_SSL
      SSL_load_error_strings();
      SSLeay_add_ssl_algorithms();
      C->ctx = SSL_CTX_new( SSLv3_client_method());
      if( C->ctx == NULL ){ joe_warning( "SSL: ctx is NULL" ); }
      /* http://www.openssl.org/support/faq.html#USER1
       * Perhaps someday I'll learn to read the FAQ
       * first and then code second, but probably not.
       * Not all OSes have /dev/urandom, we must seed
       * the PRNG
       */
      memset( buf, 0, sizeof( buf ));
      RAND_seed( buf, sizeof( buf ));
      C->ssl = SSL_new( C->ctx );
      SSL_set_connect_state( C->ssl );
    #else
      return -1;
    #endif /* HAVE_SSL */
  }

  /* create a socket, return -1 on failure */
  if(( C->sock = socket( AF_INET, SOCK_STREAM, 0 )) < 0 ){
    joe_error( "SCOUTsocket: failed to create socket" );
    return -1;
  }
 
#if defined(__GLIBC__)
  /* for systems using GNU libc */
  if(( gethostbyname_r( hn, &hent, hbf, sizeof(hbf), &hp, &herrno ) < 0 )){
    hp = NULL;
  }
#elif defined(sun)
  /* Solaris 5++ */
  hp = gethostbyname_r( hn, &hent, hbf, sizeof(hbf), &herrno );
#elif defined(_AIX)
  aixbuf = (char*)xmalloc( 9000 );
  rc  = gethostbyname_r(hn, (struct hostent *)aixbuf,
                       (struct hostent_data *)(aixbuf + sizeof(struct hostent)));
  hp = (struct hostent*)aixbuf;
#elif ( defined(hpux) || defined(__osf__) )
  hp = gethostbyname( hn );
  herrno = h_errno;
#else
  /* simply hoping that gethostbyname is thread-safe */
  hp = gethostbyname( hn );
  herrno = h_errno;
#endif/*OS SPECIFICS*/ 

  if( hp == NULL ){ return -1; } 
  bzero( &cli, sizeof( cli ));
  bcopy( (char*)hp->h_addr, (char*)&cli.sin_addr, hp->h_length );
  cli.sin_family = AF_INET;
  cli.sin_port = htons( port );

  ling.l_onoff  = 1;
  ling.l_linger = 0;
  if( setsockopt( C->sock, SOL_SOCKET, SO_LINGER, &ling, sizeof( ling )) < 0 ){ 
    joe_error( "SCOUTsocket: socket option error" );
    return -1;    
  }

  if(( mknblock( C->sock, TRUE )) < 0 ){
    joe_error( "SCOUTsocket: unable to set socket to non-blocking." );
    return -1; 
  }

  /**
   * connect to the host 
   * evaluate the server response and check for
   * readability/writeability of the socket....
   */ 
  conn = connect( C->sock, (struct sockaddr *)&cli, sizeof(struct sockaddr_in));
  if( conn < 0 && errno != EINPROGRESS ){
    /**
     * handle the error...
     */
    switch( errno ){
    case EACCES:
      joe_error( "EACCES");
      SCOUTclose( C );
      return( -1 );
    case EADDRNOTAVAIL:
      joe_error( "SCOUTsocket: address is unavailable.");
      SCOUTclose( C );
      return( -1 );
    case ETIMEDOUT:
      joe_error( "SCOUTsocket: connection timed out.");
      SCOUTclose( C );
      return( -1 );
    case ECONNREFUSED:
      joe_error( "SCOUTsocket: connection refused.");
      SCOUTclose( C );
      return( -1 );
    case ENETUNREACH:
      joe_error( "SCOUTsocket: network is unreachable.");
      SCOUTclose( C );
      return( -1 );
    default:
      joe_error( "SCOUTsocket: unknown network error." );
      SCOUTclose( C );
      return -1;
    } 
  }
  else{
    /** 
     * we have a connection, set the
     * socket to non-blocking and test
     * its integrity.
     */
    FD_ZERO(&C->rset);
    FD_ZERO(&C->wset);
    FD_SET( C->sock, &C->rset );    
    FD_SET( C->sock, &C->wset );    
    memset((void *)&timeout, '\0', sizeof( struct timeval ));
    /**
     * the default timeout is set in init.c, it's
     * value can be changed by the user in .siegerc,
     * but we'll still use a ternary condition since
     * you can't be too safe....
     */ 
    timeout.tv_sec  = (my.timeout > 0)?my.timeout:30;
    timeout.tv_usec = 0;

    if(( res = select( C->sock+1, &C->rset, &C->wset, NULL, &timeout )) < 1 ){
      joe_error( "SCOUTsocket: connection timed out." );
      close( C->sock );
      return -1; 
    }
    else{ 
      C->status = S_READING; 
    }
  } /* end of connect conditional */

  /**
   * make the socket blocking again.
   */
  if(( mknblock( C->sock, FALSE )) < 0 ){
    joe_error( "SCOUTsocket: unable to set socket to non-blocking." );
    return -1;
  } 
 
  /* if requested, encrypt the transaction  */
  if( C->prot == HTTPS ){
    #ifdef HAVE_SSL
      /* currently a fatal error, should it be? */
      if(( SSL_set_fd( C->ssl, C->sock )) < 0 ){
        joe_fatal( "unable to create secure socket!" );
      }
      if(( serr = SSL_connect( C->ssl )) < 0 ){
        int problem = SSL_get_error( C->ssl, serr );
        fprintf( stderr, "SSL_connect: want to %s more...\n",
               ( problem == SSL_ERROR_WANT_READ) ? "read" : "write");
        return -1;
      }
      SSL_get_cipher( C->ssl );
    #else
      return -1;
    #endif /* HAVE_SSL */
  }
  
  return( C->sock );
}

/**
 * local function
 * returns hostent based on OS specifics
 */
struct hostent *
SCOUTgethostbyname( const char *hn )
{
#define RESBUF  9000

  struct hostent *hp;
  char *buf = (char*)malloc( RESBUF );

  /* OS specific; for simplicity, use same ifdef structure as in code */
#if defined(__GLIBC__)
#elif defined(sun)
  int    herrno;
  struct hostent hent;
  char hbf[RESBUF];
#elif defined(_AIX)
  int  rc;
#elif ( defined(hpux) || defined(__osf__) )
  int    herrno;
#else
  int    herrno;
#endif/*OS SPECIFICS*/

#if defined(__GLIBC__)
  /* for systems using GNU libc */
  if(( gethostbyname_r( hn,
                        (struct hostent *)buf,
                        buf + sizeof(struct hostent),
                        RESBUF - sizeof(struct hostent),
                        &hp,
                        &h_errno )) < 0 )
    hp = NULL;
#elif defined(sun)
  /* Solaris 5++ */
  if(( hp = gethostbyname_r( hn, &hent, hbf, sizeof(hbf), &herrno )) < 0 )
    hp = NULL;
#elif defined(_AIX)
  rc  = gethostbyname_r(hn, (struct hostent *)buf,
                       (struct hostent_data *)(buf + sizeof(struct hostent)));
  hp = (struct hostent*)buf;
#elif ( defined(hpux) || defined(__osf__) )
  hp = gethostbyname( hn );
  herrno = h_errno;
#else
  /* simply hoping that gethostbyname is thread-safe */
  hp = gethostbyname( hn );
  herrno = h_errno;
#endif/*OS SPECIFICS*/
  if( hp == NULL ){ perror( "SCOUTgethostbyname" ); }
 
  free( buf );
  return( hp );
} 

/**
 * makes the socket non-blocking,
 * calls select with timeout and
 * returns the socket to blocking.
 */
int
SCOUTsocket_check( CONN *C, SDSET test )
{
  int res;
  struct timeval timeout;
 
  FD_ZERO(&C->rset);
  FD_ZERO(&C->wset);
  FD_SET( C->sock, &C->rset );
  FD_SET( C->sock, &C->wset );
  memset((void *)&timeout, '\0', sizeof( struct timeval ));

  if(( mknblock( C->sock, TRUE )) < 0 ){
    joe_error( "SIMBOTsocket: unable to set socket to non-blocking." );
    return -1;
  }
 
  timeout.tv_sec  = (my.timeout > 0)?my.timeout:15;
  timeout.tv_usec = 0;
 
  switch( test ){
  case READ:
    if(( res = select( C->sock+1, &C->rset, NULL, NULL, &timeout )) < 1 ){
      close( C->sock );
      return -1;
    }
    break;
  case WRITE:
    if(( res = select( C->sock+1, NULL, &C->wset, NULL, &timeout )) < 1 ){
      close( C->sock );
      return -1;
    }
    break;
  case RDWR:
    if(( res = select( C->sock+1, &C->rset, &C->wset, NULL, &timeout )) < 1 ){
      close( C->sock );
      return -1;
    }
    break;
  }
 
  if(( mknblock( C->sock, FALSE )) < 0 ){
    joe_error( "SIMBOTsocket: unable to set socket to non-blocking." );
    return -1;
  }
  FD_CLR( C->sock, &C->rset );
 
  return 0;
}

/**
 * local function
 * set socket to non-blocking
 */
int
mknblock( int sock, int nonblock )
{
#if HAVE_FCNTL_H 
  int flags;
  int retval;

  flags = fcntl( sock, F_GETFL, 0 );
  if( flags < 0 ){
    joe_error("fcntl");
    return -1;
  }
  if( nonblock ){ 
    flags |=  O_NDELAY;
  }
  else{
    flags &= ~O_NDELAY;
  }
  retval = fcntl( sock, F_SETFL, flags);
  if( retval < 0 ){
    perror("fcntl");
    return -1;
  } 
  return retval;

#elif defined( FIONBIO )
  ioctl_t status;
 
  status = nb ? 1 : 0;
  return ioctl( sd, FIONBIO, &status );
#endif
}  

/**
 * local function
 * returns ssize_t
 * writes vbuf to sock
 */
ssize_t
socket_write( int sock, const void *vbuf, size_t len )
{
  size_t      n;
  ssize_t     w;
  const char *buf;
 
  buf = vbuf;
  n   = len;
  while( n > 0 ){
    if(( w = write( sock, buf, n )) <= 0 ){
      if( errno == EINTR )
        w = 0;
      else
        return( -1 );
    }
    n   -= w;
    buf += w;
  }
  return( len );
}

/**
 * local function
 * returns ssize_t
 * writes vbuf to sock
 */
ssize_t
ssl_socket_write( CONN *C, const void *vbuf, size_t len )
{
  size_t      n;
  ssize_t     w;
  const char *buf;

#ifdef HAVE_SSL
  buf = vbuf;
  n   = len;
  while( n > 0 ){
    if(( w = SSL_write( C->ssl, buf, n )) <= 0 ){
      if( errno == EINTR )
        w = 0;
      else
        return( -1 );
    }
    n   -= w;
    buf += w;
  }
  return( len );
#else
  joe_error( "protocol not supported" );
  return -1;
#endif/*HAVE_SSL*/
}

ssize_t
SCOUTsocket_read( CONN *C, void *vbuf, size_t len )
{
  int type;
  size_t      n;
  ssize_t     r;
  char *buf;
  
  buf = vbuf;
  n   = len;
  
  if( C->prot == HTTPS ){
  #ifdef HAVE_SSL
    while( n > 0 ){ 
      if(( r = SSL_read( C->ssl, buf, n )) < 0 ){
        if( errno == EINTR )
          r = 0;
        else
          return( -1 );
      }
      else if( r == 0 ) break; 
      n   -= r;
      buf += r;
    }   /* end of while    */
  #endif/*HAVE_SSL*/
  }  
  else{
    while( n > 0 ){
      if( C->inbuffer <  n ){
        int lidos;
        memmove(C->buffer,&C->buffer[C->pos_ini],C->inbuffer);
        C->pos_ini = 0;
        lidos = read( C->sock, &C->buffer[C->inbuffer], sizeof(C->buffer)-C->inbuffer );
        if( lidos< 0 ){
          joe_error( "SCOUTread_byte: read error %d", C->sock );
          if(errno==EINTR)
            lidos = 0;
          else
            return -1;
        }
        C->inbuffer += lidos;
      }
      if( C->inbuffer >= n ){
        r = n;
      }
      else{
        r = C->inbuffer;
      }
      if( r == 0 ) break;
      memmove(buf,&C->buffer[C->pos_ini],r);
      C->pos_ini+=r;
      C->inbuffer-=r;
      n   -= r;
      buf += r;
    } /* end of while */ 
  }   /* end of else  */

  return( len - n );  
}  

/**
 * returns void
 * socket_write wrapper function.
 */
void
SCOUTsocket_write( CONN *C, const void *buf, ssize_t len )
{
  ssize_t bytes;
  int type;

  if( C->prot == HTTPS ){
    /* handle HTTPS protocol */
    #ifdef HAVE_SSL
    if(( bytes = ssl_socket_write( C, buf, len )) != len ){
      joe_error( "SCOUTssl_socket_write: ERROR" );
    }
    #else
      joe_error( "SCOUTssl_socket_write: protocol NOT supported" );
    #endif/*HAVE_SSL*/
  }
  else{
    /* assume HTTP */
    if(( bytes = socket_write( C->sock, buf, len )) != len ){
      joe_error( "SCOUTsocket_write: ERROR" );
    }
  }

  return;
} 

/**
 * returns void
 * frees ssl resources if using ssl and
 * closes the connection and the socket.
 */
void
SCOUTclose( CONN *C )
{
  int type;

  #ifdef  HAVE_SSL
    if( C->prot == HTTPS ){
      SSL_shutdown( C->ssl );
    }
    SSL_free( C->ssl );
    SSL_CTX_free( C->ctx );
  #endif/*HAVE_SSL*/
  close( C->sock ); 

  return;
} 


