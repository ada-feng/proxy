#include <stdio.h>
#include "csapp.h"
#include <stdlib.h>

/* Recommended max cache and object sizes */
//#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";

struct cache_line
{
    char key[MAXLINE];
    char value[MAX_OBJECT_SIZE];
    int valid;
    int timestamp;
};

int timer = 0;

struct cache_line cache[ 10 ];

void cache_init()
{
    int i;
    for( i = 0; i < 10; ++i )
      { cache[ i ].valid = 0;
        //cache[ i ].value=(char*)malloc(MAX_OBJECT_SIZE);
      }
}

int cache_lookup( char *key, char *value )
{
    ++timer;
    int i;
    for( i = 0; i < 10; ++i )
    {  if( cache[ i ].valid && (!strcmp( key, cache[ i ].key )) )
        {   strcpy(value, cache[ i ].value);
            cache[ i ].timestamp = timer;
            return 1;
        }
    }
    return 0;
}

int cache_find(char* key){
  ++timer;
  int i;
  for( i = 0; i < 10; ++i )
  {if( cache[ i ].valid && (!strcmp( key,  cache[ i ].key )) )
    { cache[ i ].timestamp = timer;
      return 1;
    }
  }   return 0;
}

void cache_update( char* key, char* value )
{   if(sizeof(value)>MAX_OBJECT_SIZE)
        return;
        //we know the object is too large we will not return it
    ++timer;
    if( cache_find( key ) )
        return;
    // We know key is NOT in cache
    int i;
    for( i = 0; i < 10; ++i )
    {
        if( !cache[ i ].valid )
        {
            strcpy (cache[ i ].key , key);
            strcpy (cache[ i ].value , value);
            cache[ i ].valid = 1;
            cache[ i ].timestamp = timer;
            return;
        }
    }
    // We know there are ZERO empty cache lines
    int oldest_time = timer;
    int oldest_indx = 0;
    for( i = 0; i < 10; ++i )
    {
        if( oldest_time > cache[ i ].timestamp )
        {
            oldest_time = cache[ i ].timestamp;
            oldest_indx = i;
        }
    }
    strcpy( cache[ oldest_indx ].key, key);
    strcpy( cache[ oldest_indx ].value, value);
    cache[ oldest_indx ].timestamp = timer;
}


int parse_uri( char *uri, char *hostname, char *pathname, char *port )
{
    char *hostbegin;
    char *hostend;
    char *pathbegin;
    size_t len;

    if( strncasecmp( uri, "http://", 7 ) != 0 ) {
        hostname[0] = '\0';
        return -1;
    }

    /* Extract the host name */
    hostbegin = uri + 7;
    hostend = strpbrk( hostbegin, " :/\r\n\0" );
    len = hostend - hostbegin;
    strncpy( hostname, hostbegin, len );
    hostname[ len ] = '\0';



    /* Extract the path */
    pathbegin = strchr( hostbegin, '/' );
    if( pathbegin == NULL ) {
        pathname[ 0 ] = '\0';
    }
    else {
        *pathbegin=0;
        pathbegin++;
        strcpy( pathname, pathbegin );
    }
    /* Extract the port number */
    strcpy(port,"80"); /* default */
    if( *hostend == ':' )
        strcpy(port,hostend + 1) ;

    return 0;
}

int parse_request_host(char* hostline, char* hostname){
  char * hostend;
  hostend=index(hostline,':');
  *hostend=0;
  if(!strcmp(hostline, hostname)){
    strcpy(hostname,hostline);
  }

  return 0;
}

void initrequest(char* request, char *method, char* path, char*hostname, char* port){
  sprintf(request, "%s", method);
  sprintf(request,"%s /%s", request, path);
  sprintf(request,"%s HTTP/1.0\r\n",request);
  sprintf(request, "%sHostname: %s\r\n", request, hostname);
  sprintf(request, "%sPort: %s\r\n", request, port);
  //path can sometimes be empty
  if(*path){
    sprintf(request, "%sPath and query: %s\r\n", request, path);
  }
  sprintf(request, "%s%s", request, user_agent_hdr);
  sprintf(request, "%sConnection: close\r\n",request);
  sprintf(request, "%sProxy-Connection: close\r\n", request);
}

int check_modified(char *header){
  char* portheader="Port:";
  char* pathheader="Path and query:";
  char* useheader="User-Agent:";
  char* connheader="Connection:";
  char* proconheader="Proxy-Connection:";
  char title[MAXLINE];
  title[0]=0;
  sscanf(header,"%s", title);
  if((!strcmp(title,portheader))|(!strcmp(title,pathheader))|(!strcmp(title,useheader))|(!strcmp(title,connheader))|(!strcmp(title,proconheader))){
    return 0;
  }
  return 1;
}

int hostline(char *header){
  char *hostheader="Host:";
    char title[MAXLINE];
    title[0]=0;
  sscanf(header,"%s", title);
  if(!strcmp(title,hostheader)){
    return 1;
  }
  return 0;
}

void handleError(char *msg){
  fprintf(stderr, "%s: %s\n", msg, strerror(errno));
}

int main(int argc, char **argv)
{
    if (argc != 2) {
      fprintf(stderr, "usage: %s <port>\n", argv[0]);
      exit(1);
    }
    //connect to client
    char* listenport=argv[1];
    int listenfd=Open_listenfd(listenport);

    while(1){
      struct sockaddr_storage clientaddr;
      socklen_t clientlen = sizeof(clientaddr);
      int connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
      rio_t rio;
      Rio_readinitb(&rio, connfd);
      //form request line and the first few headers
      char requestline[MAXLINE];
      requestline[0]=0;

      if(rio_readlineb(&rio, requestline, MAXLINE)<0){
        handleError("rio_readlineb Error");
      }
      char method[MAXLINE];
      method[0]=0;
      char uri[MAXLINE];
      uri[0]=0;
      sscanf(requestline,"%s %s", method, uri);
      char hostname[MAXLINE];
      hostname[0]=0;
      char pathname[MAXLINE];
      pathname[0]=0;
      char port[MAXLINE];
      port[0]=0;
      if(parse_uri( uri, hostname, pathname, port)){
        printf("Cannot Handle such protocol\n");
      }

      //build modified headers
      char newrequest[MAXLINE];
      newrequest[0]=0;
      initrequest(newrequest, method, pathname, hostname, port);
      //forward other headers unchanged
      char header[MAXLINE];
      header[0]=0;
      
      while(strcmp(header,"\r\n")){
        if(rio_readlineb(&rio, header, MAXLINE)<0) {
          handleError("rio_readlineb Error");
        }
        //check if host is not the same as in requestline, if so update
        if(hostline(header)){
          parse_request_host(header, hostname);
          if(!(*hostname))
            printf("Need Hostname");
        }
        else if(check_modified(header)){
          strcat(newrequest, header);
        }
      }

      printf("%s", newrequest);

      //caching check

      //response cached
       char response[MAX_OBJECT_SIZE];
       response[0]=0;
       if(cache_lookup(uri, response)){
         if(rio_writen(connfd, response, sizeof(response))!=sizeof(response)){
           handleError("rio_writen Error");
         }
         printf("from cache\n");
       }
       //first time
       else{
          //connect to server, send the request
         int svconnfd = Open_clientfd(hostname, port);
         size_t bytes = sizeof(newrequest);
         Rio_writen(svconnfd, newrequest, bytes);

         //read the response
         rio_t rio_s;
         Rio_readinitb(&rio_s, svconnfd);
         char newresponse[MAX_OBJECT_SIZE];
         newresponse[0]=0;
         int first=1;
         size_t size=0;
         int stopcache=1;
         while(1){
           if(first){
             char buff[MAXLINE];
             buff[0]=0;
             ssize_t num_bytes=Rio_readnb(&rio_s, buff, MAXLINE);
             if(rio_writen(connfd, buff, num_bytes)!=num_bytes){
               handleError("rio_writen Error");
             }
             size+=sizeof(buff);
             if (size>MAX_OBJECT_SIZE)
                stopcache=0;
             if (stopcache)
                strcat(newresponse, buff);
             first=0;
           }

           else{
             char buff[MAXLINE];
             buff[0]=0;
              ssize_t num_bytes=Rio_readnb(&rio_s, buff, MAXLINE);
                if(rio_writen(connfd, buff, num_bytes)!=num_bytes){
                   handleError("rio_writen Error");
                   break;
                }
                size+=sizeof(buff);
                if (size>MAX_OBJECT_SIZE)
                  stopcache=0;
                if (stopcache)
                 strcat(newresponse, buff);
              //num_bytes=MAXLINE when not EOF, otherwise<MAXLINE
                if(num_bytes<MAXLINE)
                   break;


             }
        } if(stopcache){
            cache_update(uri, newresponse);
            printf("%s", newresponse);
        }

        Close(svconnfd);
       }
       Close(connfd);
    }





  //  return 0;

}
