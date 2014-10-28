/**
 * Scout HTML Parser
 *
 * Copyright (C) 2002 by
 * Jeffrey Fulmer - <jdfulmer@armstrong.com>
 * Ken Jones      - <linkcheck@inter7.com>
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
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <setup.h>
#include <parser.h>
#include <url.h>
#include <queue.h>
#include <joedog/joedog.h>

#define BUF     512
#define URLBUF 1024 

void *html_lower(char *html_text);
void *assemble_url( char *ref, char *req ); 
void *parse_control( char *path, char *token ); 
char *strstart( char *sstr, char *tstr ); 

int
test_string( char *s )
{
  while( *s ){ 
    if( !ISASCII( *s ) ) return 0;
    printf( " %d[%s] ", *s, s );
    s++; 
  }
  return 1;
}

char *
url_trim( char *str )
{
  register char c;
  int  x  = 0;
  char *a;
  char *s = (char *) str; 

  if( str == NULL ) return NULL;
 
  /** 
   * advance the ptr omitting any
   * HTML anchors...
   */
  do {
    c = *s;
    switch((char)*s){
    case '\n':
    case '\r':
    case '#':
    case '\\':
    case '\'':
      break;
    case '/':
      if( x > 6 )
        if( a[x-1]=='/' )
          break;
      a[x] = c;
      x++; 
      break;
    default:
      a[x] = c;
      x++; 
      break;
    } 
    s++;
  } while( *s && *s != '#' );
  a[x] = '\0'; 
  
  return (char*)a; /* UNIX friendly */
} 

void *
parse_text( char *path, char *html_text )
{
  int  i;
  char *tmp;
  char buf[MEDFILE];

  memset( buf, 0, MEDFILE );  
  html_lower( html_text );
  tmp = html_text;
  while( *tmp != (char)NULL ) {
    if( *tmp == '<' ){
        tmp++;
 
      /**
       * ignore comments
       */
      if( tmp == strstart( tmp, "!--" )){
        tmp++;
        while( *tmp != (char)NULL ){
          if ( tmp == strstart(tmp, "-->")){
            tmp += 3;
            break;
          }
          tmp++;
        }
      }

      /**
       * scripts
       */
      else if( tmp == strstart( tmp,"script" )) {
        i = 0;
        tmp++;
        while( *tmp != (char)NULL && i < ( MEDFILE-1 )) {
          if ( tmp == strstart( tmp, "</script>" )){
            tmp+=9;
            break;
          }
          else {
            buf[i] = *tmp;
            i++;
            tmp++;
          }
        }
        buf[i] = 0;
        parse_control( path, buf );
      }

      /**
       * standard html
       */
      else{
        i = 0;
        while( *tmp != (char)NULL && *tmp != '>' && i < ( MEDFILE-1 )) {
          buf[i] = *tmp;
          i++;
          tmp++;
        }
        buf[i] = 0;
        parse_control( path, buf );
      }
    } /* end tmp!=NULL  */
    if ( *tmp != (char)NULL ) {
      tmp++;
    }
  }   /* end while loop */
  return;
}

char *
strstart( char *sstr, char *tstr )
{
  char *ret_str;
 
  ret_str = sstr;
  if( sstr == NULL || tstr == NULL ){
    return(NULL);
  }
  while( *sstr != 0 && *tstr != 0 ){
    if( *sstr != *tstr ){
      return(NULL);
    }
    ++sstr;
    ++tstr;
  }
  if( *tstr == 0 ){
    return(ret_str);
  }
  return(NULL);
 
} 
void *
parse_control( char *path, char *token )
{
  char *tmp;
  char **dirs;
  int  count;
  char buf[BUF];
  char url[URLBUF];

  tmp = strtok(token, TOKENS);
  while ( tmp != NULL ) {
    /* search for "link href=tokens_plus" */
    if( !strncmp( tmp, "link", 3 )){
      tmp = strtok(NULL, TOKENS);
      if( tmp != NULL ){
        if( !strncmp( tmp, "href", 3 )){
          tmp = strtok( NULL, TOKENS_PLUS );
          if ( tmp != NULL ) {
            assemble_url( path, tmp );
          }
        }
      }
    } 
    /* search for "a href=tokens_plus" */
    else if( !strncmp( tmp, "href", 4 )){
      tmp = strtok( NULL, TOKENS_PLUS );
      if( tmp != NULL ){
        assemble_url( path, tmp );
      }
      return;
    }
    else if( !strncmp( tmp, "location.href", 13 )){
      tmp = strtok(NULL, TOKENS_PLUS);
      if ( tmp != NULL ) {
        assemble_url( path, tmp );
      }
    }
    else if( !strncmp( tmp, "img", 3 )){
      if( my.images ){
        tmp = strtok(NULL, TOKENS);
        if( tmp != NULL ){
          if( !strncmp( tmp, "src", 3 )){
            tmp = strtok( NULL, TOKENS_PLUS );
            if ( tmp != NULL ) {
              assemble_url( path, tmp );
            }
          }
        }
      }
      /* user doesn't want images */
      else{ return; } 
    }
    else if( !strncmp( tmp, "frame", 5 )){
      tmp = strtok(NULL, TOKENS);
      while( tmp != NULL ){
        if( !strncmp( tmp, "src", 3 )){
          tmp = strtok(NULL, TOKENS_PLUS);
          if( tmp != NULL ){
            assemble_url( path, tmp ); 
          }
        }
        tmp = strtok(NULL, TOKENS);
      }
    }
    else if( !strncmp( tmp, "background", 10 )){
      tmp = strtok(NULL, TOKENS_PLUS);
      if ( tmp != NULL ) {
        assemble_url( path, tmp );
      }
    }
    tmp = strtok(NULL, TOKENS);
  }
}  

void *
assemble_url( char *ref, char *req )
{
  char buf[BUF];
  char url[URLBUF]; 

  memset( buf, 0, BUF );
  memset( url, 0, URLBUF );
  strncpy( buf, req, BUF-1 );
  if( !strncmp( buf, "#", 1 ))
    return; /* we're not a browswer... */
  else if( has_protocol( buf )){
    if( is_supported( buf ))
      snprintf( url, URLBUF, "%s", buf );
    else
      return;
  }
  else if( !strncmp( buf, "/", 1 )){
    snprintf( url, URLBUF, "http://%s%s", my.hostname, buf );
    if( url ){
      url_trim( url );
      if( !find_node( url )){
        add_node( url );
      }
    }
    else
      return;
  } 
  else if(( !strncmp( buf, ".", 1 )) || ( isalpha( buf[0] ))){
    int  x, y, r1, r2;
    char **reference;
    char **request;
    char uri[256];
    reference = split( '/', ref, &r1 );
    if( !strstr( buf, "/" )){
      /* top level, no refernce to '/' */
      strncpy( uri, "/", sizeof( uri ));
      strncat( uri, buf, sizeof( uri ));
      snprintf( url, URLBUF, "http://%s%s", my.hostname, uri );
      url_trim( url );
      if( !find_node( url )){
        add_node( url );
      }
      return;
    }
    else{
      request   = split( '/', buf,  &r2 );
    }
    strncpy( uri, "", sizeof( uri )); 
    for( x = 0; x < r2; x ++ ){
      if( !strncmp( request[x], "..", 2 )){
        strncat( uri, "/", sizeof( uri ));
        strncat( uri, reference[x], sizeof( uri ));
      }
      else if( !strncmp( request[x], ".", 1 )){
        for( y = 0; y < r1 - 1; y++ ){
          strncat( uri, "/", sizeof( uri ));
          strncat( uri, reference[y], sizeof( uri ));
        }
      }
      else if( !strncmp( request[x], "/", 1 )){
        strncat( uri, request[x], sizeof( uri ));
      }
      else{
        if( strncmp( uri, "/", strlen( uri )))
          strncat( uri, "/", sizeof( uri ));
        strncat( uri, request[x], sizeof( uri ));
      }
    }
    if( uri[0] == '/' )
      snprintf( url, URLBUF, "http://%s%s", my.hostname, uri );
    else
      snprintf( url, URLBUF, "http://%s/%s", my.hostname, uri );
    if( url ){
      url_trim( url );
      if( !find_node( url )){
        add_node( url );
      }
    }
    else
      return;
  }
  else{
    snprintf( url, URLBUF, "http://%s/%s", my.hostname, buf );
    if( url ){
      url_trim( url );
      if( !find_node( url )){
        add_node( url );
      }
    }
    else
      return;
  }
} /* end of assemble_url */

void *
html_lower(char *html_text)
{
  char *tmp;
  int in_angle;
  int in_quotes;
  int i;
 
  tmp    = html_text;
  in_angle  = 0;
  in_quotes = 0;
  i = 0;
  while( *tmp != 0 ) {
    if ( in_angle == 1 ) {
      if ( in_quotes == 1 ) {
        if ( *tmp == '"' ) {
          in_quotes = 0;
        }
      } else if ( *tmp == '>' ) {
        in_angle = 0;
      } else if ( *tmp == '"') {
        in_quotes = 1;
      } else if ( isupper(*tmp) ) {
        *tmp = tolower(*tmp);
      } else if ( *tmp == '\n' ) {
        *tmp = ' ';
      }
    } else if ( *tmp == '<' ) {
      in_angle = 1;
    }
    i++;
    tmp++;
  }
} 

char *
cleanupName(char *name)
{
  char *buf, *p, *q;
    
  buf = strdup( name );
  p = buf;
  q = name;
  while (*q != '\0') {
    if( strncmp(p, "/../", 4) == 0) {       /* foo/bar/../FOO */
      if( p - 2 == buf && strncmp(p - 2, "..", 2) == 0) {
        /* ../../       */
        p += 3;
        q += 3;
      }
      else if (p - 3 >= buf && strncmp(p - 3, "/..", 3) == 0) {
        /* ../../../    */
        p += 3;
                q += 3;
            }
            else {
                while (p != buf && *--p != '/') ;       /* ->foo/FOO */
                *p = '\0';
                q += 3;
                strcat(buf, q);
            }
        }
        else if (strcmp(p, "/..") == 0) {       /* foo/bar/..   */
            if (p - 2 == buf && strncmp(p - 2, "..", 2) == 0) {
                /* ../..        */
            }
            else if (p - 3 >= buf && strncmp(p - 3, "/..", 3) == 0) {
                /* ../../..     */
            } 
            else {
                while (p != buf && *--p != '/') ;       /* ->foo/ */
                *++p = '\0';
            }
            break;
        }
        else if (strncmp(p, "../", 3) == 0){
          while (p != buf && *--p != '/') ;       /* ->foo/ */
          *++p = '\0';
        }
        else if (strncmp(p, "/./", 3) == 0) {   /* foo/./bar */
            *p = '\0';          /* -> foo/bar           */
            q += 2;
            strcat(buf, q);
        }
        else if (strcmp(p, "/.") == 0) {        /* foo/. */
            *++p = '\0';        /* -> foo/              */
            break;
        }
        else if (strncmp(p, "//", 2) == 0) {    /* foo//bar */
            /* -> foo/bar           */
            *p = '\0';
            q++;
            strcat(buf, q);
        }
        else {
            p++;
            q++;
        }
    }
    return buf;
}
