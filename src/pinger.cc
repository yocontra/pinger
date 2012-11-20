#include <v8.h>
#include <node.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/time.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <iostream>
#include <assert.h>
#include "node_defs.h"

using namespace v8;
using namespace node;
using namespace std;

typedef struct {
  int ret;
  in_addr ip;
} PingResponse;

// This ICMP code was written by somebody else
// Slight modifications were made by me to remove logging/integrate with V8
// Found it on a forum - no license was attached. 
// Will try to google for it again later
uint16_t in_cksum(uint16_t *addr, unsigned len);

#define DEFDATALEN  (64-ICMP_MINLEN)  /* default data length */
#define MAXIPLEN  60
#define MAXICMPLEN  76
#define MAXPACKET (65536 - 60 - ICMP_MINLEN)/* max packet size */

PingResponse ping(const char* targ)
{

  int s, i, cc, packlen, datalen = DEFDATALEN;
  struct hostent *hp;
  struct sockaddr_in to, from;
  struct ip *ip;
  u_char *packet, outpack[MAXPACKET];
  char hnamebuf[MAXHOSTNAMELEN];
  string hostname;
  struct icmp *icp;
  int fromlen, hlen;
  PingResponse ret;
  fd_set rfds;
  struct timeval tv;
  int retval;
  struct timeval start, end;
  int end_t;
  bool cont = true;

  to.sin_family = AF_INET;

  // try to convert as dotted decimal address, else if that fails assume it's a hostname
  to.sin_addr.s_addr = inet_addr(targ);
  if (to.sin_addr.s_addr != (u_int)-1)
  {
    hostname = string(targ);
    ret.ip = to.sin_addr;
  }
  else
  {
    hp = gethostbyname(targ);
    if (!hp)
    {
      ret.ret = -1;
      return ret;
    }
    to.sin_family = hp->h_addrtype;
    bcopy(hp->h_addr, (caddr_t)&to.sin_addr, hp->h_length);
    ret.ip = to.sin_addr;
    strncpy(hnamebuf, hp->h_name, sizeof(hnamebuf) - 1);
    hostname = hnamebuf;
  }
  packlen = datalen + MAXIPLEN + MAXICMPLEN;
  if ( (packet = (u_char *)malloc((u_int)packlen)) == NULL)
  {
    cerr << "malloc error\n";
    ret.ret = -1;
    return ret;
  }

  if ( (s = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) < 0)
  {
    perror("socket"); /* probably not running as superuser */
    ret.ret = -1;
    return ret;
  }

  icp = (struct icmp *)outpack;
  icp->icmp_type = ICMP_ECHO;
  icp->icmp_code = 0;
  icp->icmp_cksum = 0;
  icp->icmp_seq = 12345;  /* seq and id must be reflected */
  icp->icmp_id = getpid();


  cc = datalen + ICMP_MINLEN;
  icp->icmp_cksum = in_cksum((unsigned short *)icp,cc);

  gettimeofday(&start, NULL);

  i = sendto(s, (char *)outpack, cc, 0, (struct sockaddr*)&to, (socklen_t)sizeof(struct sockaddr_in));
  if (i < 0 || i != cc)
  {
    if (i < 0)
      perror("sendto error");
  }
  
  // Watch stdin (fd 0) to see when it has input.
  FD_ZERO(&rfds);
  FD_SET(s, &rfds);
  // Wait up to one seconds.
  tv.tv_sec = 1;
  tv.tv_usec = 0;

  while(cont)
  {
    retval = select(s+1, &rfds, NULL, NULL, &tv);
    if (retval == -1)
    {
      perror("select()");
      ret.ret = -1;
      close(s);
      return ret;
    }
    else if (retval)
    {
      fromlen = sizeof(sockaddr_in);
      if ( (ret.ret = recvfrom(s, (char *)packet, packlen, 0,(struct sockaddr *)&from, (socklen_t*)&fromlen)) < 0)
      {
        perror("recvfrom error");
        ret.ret = -1;
        close(s);
        return ret;
      }

      // Check the IP header
      ip = (struct ip *)((char*)packet); 
      hlen = sizeof( struct ip ); 
      if (ret.ret < (hlen + ICMP_MINLEN)) 
      { 
        ret.ret = -1;
        close(s);
        return ret;
      } 

      // Now the ICMP part 
      icp = (struct icmp *)(packet + hlen); 
      if (icp->icmp_type == ICMP_ECHOREPLY)
      {
        if (icp->icmp_seq != 12345)
        {
          continue;
        }
        if (icp->icmp_id != getpid())
        {
          continue;
        }
        cont = false;
      }
      else
      {
        continue;
      }
  
      gettimeofday(&end, NULL);
      end_t = 1000000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);

      if(end_t < 1)
        end_t = 1;
      close(s);
      ret.ret = end_t;
      return ret;
    }
    else
    {
      ret.ret = 0;
      close(s);
      return ret;
    }
  }
  ret.ret = 0;
  close(s);
  return ret;
}

uint16_t in_cksum(uint16_t *addr, unsigned len)
{
  uint16_t answer = 0;
  /*
   * Our algorithm is simple, using a 32 bit accumulator (sum), we add
   * sequential 16 bit words to it, and at the end, fold back all the
   * carry bits from the top 16 bits into the lower 16 bits.
   */
  uint32_t sum = 0;
  while (len > 1)  {
    sum += *addr++;
    len -= 2;
  }

  // mop up an odd byte, if necessary
  if (len == 1) {
    *(unsigned char *)&answer = *(unsigned char *)addr ;
    sum += answer;
  }

  // add back carry outs from top 16 bits to low 16 bits
  sum = (sum >> 16) + (sum & 0xffff); // add high 16 to low 16
  sum += (sum >> 16); // add carry
  answer = ~sum; // truncate to 16 bits
  return answer;
}

Handle<Value> Ping(const Arguments &args)
{
    HandleScope scope;

    // Check args
    ARG_CHECK_STRING(0, 'host');
    ARG_CHECK_OPTIONAL_FUNCTION(1, 'callback');

    String::Utf8Value host(args[0]->ToString());
    const char* ac = *host;

    // Do ping
    PingResponse ret;
    Unlocker ul;
    ret = ping(ac);
    Locker l;
    Handle<Boolean> res;
    if (ret.ret > 0) {
        res = True();
    } else {
        res = False();
    }

    if (args.Length() > 1){
      //Set up callback
      Local<Function> cb = Local<Function>::Cast(args[1]);
      unsigned cbargc = 3;
      Handle<Value> cbargv[cbargc];
      cbargv[0] = res;
      cbargv[1] = Local<Value>::New(Number::New(ret.ret));
      char* host = inet_ntoa(ret.ip);
      cbargv[2] = Local<Value>::New(String::New(host));
      //Run callback
      cb->Call(Context::GetCurrent()->Global(), cbargc, cbargv);
      return scope.Close(Undefined());
    } else {
      return scope.Close(res);
    }
}

extern "C" {
    static void init (Handle<Object> target)
    {
        HandleScope scope;
        target->Set(NODE_SYMBOL("ping"), FunctionTemplate::New(Ping)->GetFunction());
    }

    NODE_MODULE(pinger, init);
}
