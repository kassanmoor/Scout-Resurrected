/**
 * Utility Functions
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
 * --
 *
 */
#include <setup.h>

/*
  parses the -t/--time option for a timed
  interval.  The option requires a modifier,
  H, M, or S, hours, minutes or seconds    */
#if 0
void
parse_time( char *p )
{
  int x = 0;
  my.time = my.secs = 0;
  while( ISDIGIT( p[x] ))
    x++;
  my.time = atoi( substring( p, 0, x ));

  for( ; x < strlen( p ); x ++ )
    switch( TOLOWER( p[x] )){
      case 's':
        my.secs = my.time;
        my.time = 1;
        return;
      case 'm':
        my.secs = my.time * 60;
        my.time = 1;
        return;
      case 'h':
        my.secs = my.time * 3600;
        my.time = 1;
        return;
      default:
        break;
    }
  if(( my.time > 0 ) && ( my.secs <= 0 )){
    my.secs = my.time * 60;
  }

  return;
}
#endif

