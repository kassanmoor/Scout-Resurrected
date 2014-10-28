#include <stdio.h>
#include <auth.h>
#include <base64.h> 
#include <setup.h>
#include <joedog/joedog.h> 

void
add_authorization( SERVICE service, char *username, char *password, char *realm )
{
  struct LOGIN *tail = NULL;
  tail = (struct LOGIN*)malloc(sizeof(struct LOGIN));
  tail->username = strdup( username );
  tail->password = strdup( password );
  tail->realm    = (realm!=NULL&&strlen(realm)>1)?strdup( realm ):"all";
  switch( service ){
  case WWW:
    tail->next   = my.auth.head;
    my.auth.head = tail;
    break;
  case PROXY:
    tail->next   = my.proxy.head;
    my.proxy.head= tail;
    break;
  default:
    break;
  }
  return;
} 

int
display_authorization( SERVICE service )
{
  struct LOGIN *li = (service==WWW)?my.auth.head:my.proxy.head;

  while( li != NULL ){
    printf("%s:%s [%s]\n", li->username, li->password, li->realm );
    li = li->next; 
  }
  return 0;
}

int
set_authorization( SERVICE service, char *realm ) 
{
  char buf[64];
  struct LOGIN *li     = (service==WWW)?my.auth.head:my.proxy.head;
 
  while( li != NULL ){
    if( !strncasecmp( li->realm, realm, strlen( realm ))){ 
      snprintf( 
        buf, sizeof( buf ), 
        "%s:%s", 
        (li->username!=NULL)?li->username:"", (li->password!=NULL)?li->password:"" 
      ); 
      if( service==WWW ){
        if(( base64_encode( buf, strlen(buf), &my.auth.encode ) < 0 ))
          return -1;
      }
      else {
        if(( base64_encode( buf, strlen(buf), &my.proxy.encode ) < 0 ))
          return -1;
      }
      return 0;
    } 
    li = li->next;
  }
  /* failed to match, attempting default */
  li = (service==WWW)?my.auth.head:my.proxy.head; 
  if( li == NULL )
    return -1;
  snprintf(
    buf, sizeof( buf ),
    "%s:%s",
    (li->username!=NULL)?li->username:"", (li->password!=NULL)?li->password:""
  ); 
  if( service==WWW ){
    if(( base64_encode( buf, strlen(buf), &my.auth.encode ) < 0 ))
      return -1;
  }
  else{
    if(( base64_encode( buf, strlen(buf), &my.proxy.encode ) < 0 ))
      return -1;
  }
  return 0;
}

