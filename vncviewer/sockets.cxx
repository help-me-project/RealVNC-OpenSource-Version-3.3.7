/*
 *  Copyright (C) 2002-2003 RealVNC Ltd.
 *  Copyright (C) 1999 AT&T Laboratories Cambridge.  All Rights Reserved.
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * sockets.cxx - functions to deal with sockets.
 */

#include <unistd.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <assert.h>
extern "C" {
#include "vncviewer.h"
}

#include <rdr/FdInStream.h>
#include <rdr/FdOutStream.h>
#include <rdr/Exception.h>

extern "C" { void PrintInHex(char *buf, int len); }


int rfbsock;
rdr::FdInStream* fis;
rdr::FdOutStream* fos;
Bool sameMachine = False;

static Bool rfbsockReady = False;

extern "C" {
static void rfbsockReadyCallback(XtPointer clientData, int *fd, XtInputId *id)
{
  rfbsockReady = True;
  XtRemoveInput(*id);
}
}

static void ProcessXtEvents(void*)
{
  rfbsockReady = False;
  XtAppAddInput(appContext, rfbsock, (XtPointer)XtInputReadMask,
		rfbsockReadyCallback, NULL);
  while (!rfbsockReady) {
    XtAppProcessEvent(appContext, XtIMAll);
    if (!XtAppPending(appContext))
      CheckUpdateNeeded();
  }
}


/*
 * ConnectToRFBServer.
 */

Bool ConnectToRFBServer(const char *hostname, int port)
{
  int sock = ConnectToTcpAddr(hostname, port);

  if (sock < 0) {
    fprintf(stderr,"Unable to connect to VNC server\n");
    return False;
  }

  return SetRFBSock(sock);
}

Bool SetRFBSock(int sock)
{
  try {
    rfbsock = sock;
    fis = new rdr::FdInStream(rfbsock, ProcessXtEvents);
    fos = new rdr::FdOutStream(rfbsock);

    struct sockaddr_in peeraddr, myaddr;
    VNC_SOCKLEN_T addrlen = sizeof(struct sockaddr_in);

    getpeername(sock, (struct sockaddr *)&peeraddr, &addrlen);
    getsockname(sock, (struct sockaddr *)&myaddr, &addrlen);

    sameMachine = (peeraddr.sin_addr.s_addr == myaddr.sin_addr.s_addr);

    return True;
  } catch (rdr::Exception& e) {
    fprintf(stderr,"initialiseInStream: %s\n",e.str());
  }
  return False;
}

void StartTiming()
{
  fis->startTiming();
}

void StopTiming()
{
  fis->stopTiming();
}

int KbitsPerSecond()
{
  //fprintf(stderr," kbps %d     \r",fis->kbitsPerSecond());
  return fis->kbitsPerSecond();
}

int TimeWaitedIn100us()
{
  return fis->timeWaited();
}

Bool ReadFromRFBServer(char *out, unsigned int n)
{
  try {
    fis->readBytes(out, n);
    return True;
  } catch (rdr::Exception& e) {
    fprintf(stderr,"ReadFromRFBServer: %s\n",e.str());
  }
  return False;
}


/*
 * Write an exact number of bytes, and don't return until you've sent them.
 */

Bool WriteToRFBServer(char *buf, int n)
{
  try {
    fos->writeBytes(buf, n);
    fos->flush();
    return True;
  } catch (rdr::Exception& e) {
    fprintf(stderr,"WriteExact: %s\n",e.str());
  }
  return False;
}


/*
 * ConnectToTcpAddr connects to the given host and port.
 */

int ConnectToTcpAddr(const char* hostname, int port)
{
  int sock;
  struct sockaddr_in addr;
  int one = 1;
  unsigned int host;

  if (!StringToIPAddr(hostname, &host)) {
    fprintf(stderr,"Couldn't convert '%s' to host address\n", hostname);
    return -1;
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = host;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    fprintf(stderr,programName);
    perror(": ConnectToTcpAddr: socket");
    return -1;
  }

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fprintf(stderr,programName);
    perror(": ConnectToTcpAddr: connect");
    close(sock);
    return -1;
  }

  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		 (char *)&one, sizeof(one)) < 0) {
    fprintf(stderr,programName);
    perror(": ConnectToTcpAddr: setsockopt");
    close(sock);
    return -1;
  }

  return sock;
}



/*
 * ListenAtTcpPort starts listening at the given TCP port.
 */

int ListenAtTcpPort(int port)
{
  int sock;
  struct sockaddr_in addr;
  int one = 1;

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = INADDR_ANY;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
    fprintf(stderr,programName);
    perror(": ListenAtTcpPort: socket");
    return -1;
  }

  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
		 (const char *)&one, sizeof(one)) < 0) {
    fprintf(stderr,programName);
    perror(": ListenAtTcpPort: setsockopt");
    close(sock);
    return -1;
  }

  if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    fprintf(stderr,programName);
    perror(": ListenAtTcpPort: bind");
    close(sock);
    return -1;
  }

  if (listen(sock, 5) < 0) {
    fprintf(stderr,programName);
    perror(": ListenAtTcpPort: listen");
    close(sock);
    return -1;
  }

  return sock;
}


/*
 * AcceptTcpConnection accepts a TCP connection.
 */

int AcceptTcpConnection(int listenSock)
{
  int sock;
  struct sockaddr_in addr;
  VNC_SOCKLEN_T addrlen = sizeof(addr);
  int one = 1;

  sock = accept(listenSock, (struct sockaddr *) &addr, &addrlen);
  if (sock < 0) {
    fprintf(stderr,programName);
    perror(": AcceptTcpConnection: accept");
    return -1;
  }

  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
		 (char *)&one, sizeof(one)) < 0) {
    fprintf(stderr,programName);
    perror(": AcceptTcpConnection: setsockopt");
    close(sock);
    return -1;
  }

  return sock;
}


/*
 * StringToIPAddr - convert a host string to an IP address.
 */

Bool StringToIPAddr(const char *str, unsigned int *addr)
{
  struct hostent *hp;

  if (strcmp(str,"") == 0) {
    *addr = 0; /* local */
    return True;
  }

  *addr = inet_addr(str);

  if (*addr != (unsigned int)-1)
    return True;

  hp = gethostbyname(str);

  if (hp) {
    *addr = *(unsigned int *)hp->h_addr;
    return True;
  }

  return False;
}


/*
 * Print out the contents of a packet for debugging.
 */

void PrintInHex(char *buf, int len)
{
  int i, j;
  char c, str[17];

  str[16] = 0;

  fprintf(stderr,"ReadExact: ");

  for (i = 0; i < len; i++)
    {
      if ((i % 16 == 0) && (i != 0)) {
	fprintf(stderr,"           ");
      }
      c = buf[i];
      str[i % 16] = (((c > 31) && (c < 127)) ? c : '.');
      fprintf(stderr,"%02x ",(unsigned char)c);
      if ((i % 4) == 3)
	fprintf(stderr," ");
      if ((i % 16) == 15)
	{
	  fprintf(stderr,"%s\n",str);
	}
    }
  if ((i % 16) != 0)
    {
      for (j = i % 16; j < 16; j++)
	{
	  fprintf(stderr,"   ");
	  if ((j % 4) == 3) fprintf(stderr," ");
	}
      str[i % 16] = 0;
      fprintf(stderr,"%s\n",str);
    }

  fflush(stderr);
}
