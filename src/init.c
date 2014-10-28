/**
 * Scout environment initialization.
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
#include <init.h>
#include <setup.h>
#include <base64.h>
#include <util.h>
#include <auth.h>
#include <fcntl.h>
char seti[128];

int
init_config( void )
{
  char *e;
  char filename[256];
  char buf[256];
  memset( &my,  0, sizeof( struct CONFIG )); /* from setup.h */
  memset( &our, 0, sizeof( struct STATUS )); /* from setup.h */
  my.signaled = 0;
  our.shutting_down = 0;
  our.total_threads = 0;

  if(( e = getenv("SCOUTRC")) != NULL){
    snprintf(filename, sizeof( filename), e);
  }
  else{
    snprintf( filename, sizeof( filename), "%s/.scoutrc", getenv( "HOME" ));
  }

  strcpy( my.file, SCOUT_HOME ); 
  strcat( my.file, "/etc/urls.txt" );
  my.debug          = FALSE;
  my.config         = FALSE;
  my.cookies        = TRUE;
  my.images         = FALSE;
  my.timeout        = 30;
  my.extra[0]       = 0;
  my.showcodes      = FALSE;
  my.bids           = 5;
  my.auth.encode    = "";
  my.proxy.encode   = "";
  my.proxy.required = FALSE;
  my.proxy.port     = 80;    
  if( load_conf( filename ) < 0 ){
    fprintf( stderr, "****************************************************\n" );
    fprintf( stderr, "scout: could not open %s\n", filename );
    fprintf( stderr, "run \'scout.config\' to generate a new .scoutrc file\n" );
    fprintf( stderr, "****************************************************\n" );
    return -1;
  }

  if( strlen(my.uagent) < 1 ) 
    snprintf( 
      my.uagent, sizeof( my.uagent ),
      "Mozilla/4.73 [en] (X11; I; Linux 2.2.16 i586)" 
    );

  /**
   * DEPRECATED!! username and password are being
   * phased out in favor of my.auth.head
   */
  if(( my.username && strlen(my.username) > 0 ) &&
    (  my.password && strlen(my.password) > 0 )){
    add_authorization( WWW, my.username, my.password, "all" );
  }  

  if( my.proxy.hostname && strlen(my.proxy.hostname) > 0 ){
    my.proxy.required = TRUE;
  }

  return 0;  
}

int
show_config( int EXIT )
{
  printf( "CURRENT  SIEGE  CONFIGURATION\n" );
  printf( "Edit the resource file to change the settings.\n" );
  printf( "----------------------------------------------\n" );
  printf( "user-agent:              %s\n", my.uagent );
  printf( "version:                 %s\n", version_string );
  printf( "verbose:                 %s\n", my.verbose?"true":"false" );
  printf( "debug:                   %s\n", my.debug?"true":"false" );
  printf( "protocol:                %s\n", my.protocol?"HTTP/1.1":"HTTP/1.0" );
  if( my.proxy.required ){
    printf( "proxy-host:                     %s\n", my.proxy.hostname );
    printf( "proxy-port:                     %d\n", my.proxy.port );
  }  
  printf( "connection:              %s\n", my.keepalive?"keep-alive":"close" );
  printf( "named URL:               %s\n", my.url==NULL?"none":my.url );
  printf( "URLs file:               %s\n", 
    !strcasecmp(my.file, seti)?(strlen(seti)>0?seti:CNF_FILE):CNF_FILE );
  printf( "return codes in file:    %s\n", my.showcodes?"yes":"no" );
  printf( "logging:                 %s\n", my.logging?"true":"false" );
  printf( "check images:            %s\n", my.images?"true":"false" );
  printf( "log file:                %s\n", LOG_FILE );
  printf( "resource file:           %s/.siegerc\n", getenv( "HOME" ));
  printf( "\n" );

  if( EXIT ) exit( 0 );
  else return 0;
}

/**
 * chops the white space from
 * the beginning of a char *
 */
#if 0
static char
*chomp( char *str )
{
  while( *str == ' ' || *str == '\t') {
    str++;
  }
  return str;
}   
#endif

static char
*get_line( FILE *fp )
{
  char *ptr;
  char *new;
  char tmp[256];
 
  ptr = malloc( 1 );
 
  do{
    if(( fgets( tmp, sizeof( tmp ), fp )) == NULL ) return( NULL );
    if( ptr == NULL ) ptr = strdup( tmp );
    else{
      if(( ptr = xrealloc( ptr, strlen(ptr) + strlen(tmp) + 1 )) == NULL ) break;
      strcat( ptr, tmp );
    }
  } while(( new = strchr( ptr, '\n')) == NULL );
  if( new != NULL ) *new = '\0';
 
  return ptr;
} 

static char
*chomp_line( FILE *fp, char **mystr, int *line_num )
{
  char *ptr;
  while( TRUE ){
    if(( *mystr = get_line( fp )) == NULL) return( NULL );
    (*line_num)++;
    ptr = chomp( *mystr );
    /* exclude comments */
    if( *ptr != '#' && *ptr != '\0' ){
      return( ptr );
    }
  }
} 

int
load_conf( char *filename )
{
  FILE *fp;
  int  fd;
  char temp[32]; 
  int  line_num = 0;
  char *line;
  char *option;
  char *value;
 
  if (( fp = fopen(filename, "r")) == NULL ) {
    return -1;
  } 

  while(( line = chomp_line( fp, &line, &line_num )) != NULL ){
    option = line;
    while( *line && !ISSPACE( (int)*line ) && !ISSEPARATOR( *line ))
      line++;
    *line++=0;
    while( ISSPACE( (int)*line ) || ISSEPARATOR( *line ))
      line++;
    value  = line;
    while( *line )
      line++;  
    if( !strncasecmp( option, "verbose", 7   )){
      if( !strncasecmp( value, "true", 4 ))
        my.verbose = TRUE;
      else
        my.verbose = FALSE;
    } 
    if( !strncasecmp( option, "logging", 7 )){
      if( !strncasecmp( value, "true", 4 ))
        my.logging = TRUE;
      else
        my.logging = FALSE;
    }
    if( !strncasecmp( option, "show-logfile", 12 )){
      if( !strncasecmp( value, "true", 4 ))
        my.shlog = TRUE;
      else
        my.shlog = FALSE;
    }
    if( !strncasecmp( option, "cookies", 7 )){
      if( !strncasecmp( value, "true", 4 ))
        my.cookies = TRUE;
      else
        my.cookies = FALSE;
    }
    if( !strncasecmp( option, "timeout", 7 )){
      my.timeout = atoi( value );
    }
    if( !strncasecmp( option, "debug", 5 )){
      if( !strncasecmp( value, "true", 4 ))
        my.debug = TRUE;
      else
        my.debug = FALSE;
    }
    if( !strncasecmp( option, "file", 5 )){
      strncpy( my.file, value, sizeof( my.file ));
      strncpy( seti, value, sizeof( seti ));
    }
    if( !strncasecmp( option, "url", 3 )){
      my.url = (char*)strdup( value );
    }
    if( !strncasecmp( option, "login", 5 )){
      if( !strncasecmp( option, "login-url", 9 )){
        /* login URL */
        my.login = TRUE;
        my.loginurl = value;
      }
      else{
        /* user login info */
        char *usr, *pwd, *rlm;
        usr = value;
        while( *value && *value != ':' && *value != '\0' )
          value++;
        *value++=0; pwd = value;
        while( *value && *value != ':' && *value != '\0' )
        value++;
        *value++=0; rlm = value;
        add_authorization( WWW, usr, pwd, rlm );
      }
    }
    if( !strncasecmp( option, "user-agent", 10 )){
      strncpy( my.uagent, value, sizeof( my.uagent ));
    }
    if( !strncasecmp( option, "username", 8 )){
      my.username = value;
    }
    if( !strncasecmp( option, "password", 8 )){
      my.password = value;
    }
    if( !strncasecmp( option, "connection", 10 )){
      if( !strncasecmp( value, "keep-alive", 10 ))
        my.keepalive = TRUE;
      else
        my.keepalive = FALSE; 
    }
    if( !strncasecmp( option, "protocol", 8 )){
      if( !strncasecmp( value, "HTTP/1.1", 8 ))
        my.protocol = TRUE;
      else
        my.protocol = FALSE; 
    }
    if( !strncasecmp( option, "proxy-host", 10 )){
      my.proxy.hostname = tchomp( value );
    }
    if( !strncasecmp( option, "proxy-port", 10 )){
      my.proxy.port = atoi( value );
    }
    if( !strncasecmp( option, "proxy-login", 10 )){
      char *usr, *pwd, *rlm;
      usr = value;
      while( *value && *value != ':' && *value != '\0' )
        value++;
      *value++=0; pwd = value;
      while( *value && *value != ':' && *value != '\0' )
      value++;
      *value++=0; rlm = value;
      add_authorization( PROXY, usr, pwd, rlm );
    } 
    if( !strncasecmp( option, "show-codes", 10 )){
      if( !strncasecmp( value, "true", 4 ))
        my.showcodes = TRUE;
      else
        my.showcodes = FALSE; 
    }
    if( !strncasecmp( option, "images", 6 )){
      if( !strncasecmp( value, "true", 4 ))
        my.images = TRUE;
      else
        my.images = FALSE;
    }
    if( !strncasecmp( option, "header", 6 )){
	int ll;
	if(!strchr(value,':')) joe_fatal("no ':' in http-header");
	if((strlen(value) + strlen(my.extra) + 3) > 512) joe_fatal("too many headers");
	strcat(my.extra,value);
	strcat(my.extra,"\015\012");
    }

  } /* end of while line=chomp_line */

  return 0;
}


