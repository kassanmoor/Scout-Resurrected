/**
 * Scout
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
 * 
 *
 */
#define  INTERN  1

#ifdef  HAVE_CONFIG_H
# include <config.h>
#endif/*HAVE_CONFIG_H*/

/*LOCAL HEADERS*/
#include <setup.h>
#include <client.h>
#include <getopt.h>
#include <util.h>
#include <log.h>
#include <init.h>
#include <cfg.h>
#include <url.h>
#include <cookie.h>
#include <queue.h>
#include <signal.h>

int fd         =    0;    /* tmp file, for user defined URL          */
int count      =    0;    /* count messages processes, ie. transacts */
int code       =    0;    /* count HTTP successes, i.e., <  400      */
int fail       =    0;    /* count HTTP failures,  i.e., >= 400      */ 
clock_t start, stop;      /* process start and stop times.           */
char temp[32];            /* container for tmp file name.            */

struct tms t_start, t_stop;

/**
 * long options, std options struct
 */
static struct option long_options[] =
{
  { "version",    no_argument,       NULL, 'V' },
  { "help",       no_argument,       NULL, 'h' },
  { "verbose",    no_argument,       NULL, 'v' },
  { "config",     no_argument,       NULL, 'C' },
  { "debug",      no_argument,       NULL, 'D' },
  { "header",     required_argument, NULL, 'H' },
  { "images",      no_argument,       NULL, 'i' }
};

/**
 * display_version   
 * displays the version number and exits on boolean false. 
 * continue running? TRUE=yes, FALSE=no
 * return void
 */
void 
display_version( int i )
{
  /**
   * version_string is defined in version.c 
   * adding it to a separate file allows us
   * to parse it in configure.  
   */
  if( my.debug )
    printf( "Scout %s: debugging enabled\n", version_string );
  else 
    printf( "Scout %s\n", version_string );
  /**
   * if TRUE, exit 
   */
  if( i == 1 ){ exit( EXIT_SUCCESS ); }
}  /* end of display version */

/**
 * display_help 
 * displays the help section to STDOUT and exits
 */ 
void 
display_help()
{
  /**
   * call display_version, but do not exit 
   */
  display_version( FALSE ); 
  printf("Usage: scout [options] host.domain.com\n");
  printf("Options:\n"                    );
  puts("  -V, --version         VERSION, prints version number to screen.");
  puts("  -h, --help            HELP, prints this section.");
  puts("  -C, --config          CONFIGURATION, show the current configuration.");
  puts("  -v, --verbose         VERBOSE, prints notification to screen.");
  puts("  -f, --file=FILE       FILE, change the configuration file to file." );
  puts("  -H, --header=\"text\"   Add a header to request (can be many)" ); 
  puts("  -i, --images          IMAGES, parse server for images.");
  /**
   * our work is done, exit nicely
   */
  exit( EXIT_SUCCESS );
}

/**
 * parses command line arguments and assigns
 * values to run time variables. relies on GNU
 * getopts included with this distribution.  
 */ 
void 
parse_cmdline( int argc, char *argv[] )
{
  int c = 0;
  int nhosts;

  while(( c = getopt_long( argc, argv, "VhvCDH:i", long_options, (int *)0)) != EOF ){
  switch( c ){
      case 'V':
        display_version( TRUE );
        break;
      case 'h':
        display_help();
        exit( EXIT_SUCCESS );
      case 'v':
        my.verbose = TRUE;
        break;
      case 'C':
        my.config = TRUE;
        break;
      case 'D':
        my.debug = TRUE;
        break;
      case 'i':
        my.images = TRUE;
        break;
      case 'H':
        {
          int ll;
          if(!strchr(optarg,':')) joe_fatal("no ':' in http-header");
          if((strlen(optarg) + strlen(my.extra) + 3) > 512)
              joe_fatal("too many headers");
          strcat(my.extra,optarg);
          strcat(my.extra,"\015\012");
        }
        break; 
    } /** end of switch( c )           **/
  }   /** end of while c = getopt_long **/
  nhosts = argc - optind;
  if( my.config ){ show_config( TRUE );   }
  if( !nhosts )  { display_help( TRUE );  }
  else{ my.url = strdup( argv[argc-1] );  }
} /* end of parse_cmdline */

void
signal_handler()
{
  int ret;
  write_queue( my.showcodes );
 
  exit( ret );
} 

/**
 * scout main
 */  
int 
main( int argc, char *argv[] )
{
  int x, result;                /* index, func. result*/
  struct sigaction  action;   
  CLIENT           *client;     /* defined in setup.h */  
 
  init_config();                /* defined in init.h */
  parse_cmdline(argc, argv);    /* defined above     */
  
  if( my.config ){
    /* see: init.h */  
    show_config( TRUE );    
  }

  memset( &action, 0, sizeof( action ));
  action.sa_handler = signal_handler;
  if( sigaction( SIGHUP, &action, NULL ))
    joe_error( "sigaction" );
  if( sigaction( SIGINT, &action, NULL))
    joe_error( "sigaction" );
  if( sigaction( SIGTERM, &action, NULL))
    joe_error( "sigaction" );

  /* cookie is an EXTERN, defined in setup */ 
  cookie = (COOKIE*)malloc( sizeof(COOKIE)); 
  if( !cookie ){ joe_fatal( "memory exhausted" ); }
  cookie->first = NULL;
  client = (CLIENT*)malloc(sizeof(CLIENT));
  if( !client ){ joe_fatal( "application memory exhausted" ); } 

  http_client( client );
  write_queue( my.showcodes );
  /**
   * exit program.
   */
  exit( EXIT_SUCCESS );	
} /* end of int main **/



