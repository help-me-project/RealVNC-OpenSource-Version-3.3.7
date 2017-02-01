/*
 * httpd.c - a simple HTTP server
 */

/*
 *  Copyright (C) 2002 RealVNC Ltd.
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <pwd.h>

#include "rfb.h"

#define NOT_FOUND_STR "HTTP/1.0 404 Not found\r\n\r\n" \
    "<HEAD><TITLE>File Not Found</TITLE></HEAD>\n" \
    "<BODY><H1>File Not Found</H1></BODY>\n"

#define OK_STR "HTTP/1.0 200 OK\r\n\r\n"

static void httpProcessInput();
static Bool compareAndSkip(char **ptr, const char *str);

int httpPort = 0;
char *httpDir = NULL;

int httpListenSock = -1;
int httpSock = -1;

#define BUF_SIZE 32768

static char buf[BUF_SIZE];
static int bufLen;


/*
 * httpInitSockets sets up the TCP socket to listen for HTTP connections.
 */

void httpInitSockets()
{
    static Bool done = FALSE;

    if (done)
	return;

    done = TRUE;

    if (!httpDir)
	return;

    if (httpPort == 0) {
	httpPort = 5800 + atoi(display);
    }

    rfbLog("Listening for HTTP connections on TCP port %d\n", httpPort);

    rfbLog("  URL http://%s:%d\n",rfbThisHost,httpPort);

    if ((httpListenSock = ListenOnTCPPort(httpPort)) < 0) {
	rfbLogPerror("ListenOnTCPPort");
	exit(1);
    }

    AddEnabledDevice(httpListenSock);
}


/*
 * httpCheckFds is called from ProcessInputEvents to check for input on the
 * HTTP socket(s).  If there is input to process, httpProcessInput is called.
 */

void httpCheckFds()
{
    int nfds;
    fd_set fds;
    struct timeval tv;
    struct sockaddr_in addr;
    unsigned int addrlen = sizeof(addr);
    int flags;

    if (!httpDir)
	return;

    FD_ZERO(&fds);
    FD_SET(httpListenSock, &fds);
    if (httpSock >= 0) {
	FD_SET(httpSock, &fds);
    }
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    nfds = select(max(httpSock,httpListenSock) + 1, &fds, NULL, NULL, &tv);
    if (nfds == 0) {
	return;
    }
    if (nfds < 0) {
	rfbLogPerror("httpCheckFds: select");
	return;
    }

    if ((httpSock >= 0) && FD_ISSET(httpSock, &fds)) {
	httpProcessInput();
    }

    if (FD_ISSET(httpListenSock, &fds)) {

	if (httpSock >= 0) close(httpSock);

	if ((httpSock = accept(httpListenSock,
			       (struct sockaddr *)&addr, &addrlen)) < 0) {
	    rfbLogPerror("httpCheckFds: accept");
	    return;
	}

	flags = fcntl(httpSock, F_GETFL);

	if (flags < 0 || fcntl(httpSock, F_SETFL, flags | O_NONBLOCK) == -1) {
	    rfbLogPerror("httpCheckFds: fcntl");
	    close(httpSock);
	    httpSock = -1;
	    return;
	}

	AddEnabledDevice(httpSock);
        bufLen = 0;
    }
}


static void httpCloseSock()
{
    close(httpSock);
    RemoveEnabledDevice(httpSock);
    httpSock = -1;
}


/*
 * httpProcessInput is called when input is received on the HTTP socket.  We
 * read lines from the HTTP client until we get a blank line (the end of an
 * HTTP request).  The socket is non-blocking so we return if there's no more
 * to read and will carry on where we left off when more data is available.
 */

static void httpProcessInput()
{
    struct sockaddr_in addr;
    unsigned int addrlen = sizeof(addr);
    char fullFname[256];
    char *fname;
    int maxFnameLen;
    int fd;
    Bool performSubstitutions = FALSE;
    char str[256];
    struct passwd *user = getpwuid(getuid());
    int n;

    if (strlen(httpDir) > 200) {
	rfbLog("-httpd directory too long\n");
	httpCloseSock();
	return;
    }
    strcpy(fullFname, httpDir);
    fname = &fullFname[strlen(fullFname)];
    maxFnameLen = 255 - strlen(fullFname);

    while (1) {
        if (bufLen >= BUF_SIZE-1) {
            rfbLog("httpProcessInput: HTTP request is too long\n");
            httpCloseSock();
            return;
        }

        n = read(httpSock, buf + bufLen, BUF_SIZE - bufLen - 1);

        if (n <= 0) {
            if (n < 0) {
                if (errno == EAGAIN) return;
                rfbLogPerror("httpProcessInput: read");
            } else {
                rfbLog("httpProcessInput: connection closed\n");
            }
            httpCloseSock();
            return;
        }

        bufLen += n;
        buf[bufLen] = 0;

	if (strstr(buf, "\r\r") || strstr(buf, "\n\n") ||
	    strstr(buf, "\r\n\r\n") || strstr(buf, "\n\r\n\r"))
            break;
    }

    if (strncmp(buf, "GET ", 4) != 0) {
	rfbLog("httpProcessInput: first line wasn't a GET?\n");
	httpCloseSock();
	return;
    }

    buf[strcspn(buf, "\r\n")] = 0; /* only want first line */

    if (strlen(buf) > maxFnameLen) {
	rfbLog("httpProcessInput: GET line too long\n");
	httpCloseSock();
	return;
    }

    if (sscanf(buf, "GET %s HTTP", fname) != 1) {
	rfbLog("httpProcessInput: couldn't parse GET line\n");
	httpCloseSock();
	return;
    }

    if (fname[0] != '/') {
	rfbLog("httpProcessInput: filename didn't begin with '/'\n");
	WriteExact(httpSock, NOT_FOUND_STR, strlen(NOT_FOUND_STR));
	httpCloseSock();
	return;
    }

    if (strchr(fname+1, '/') != NULL) {
	rfbLog("httpProcessInput: asking for file in other directory\n");
	WriteExact(httpSock, NOT_FOUND_STR, strlen(NOT_FOUND_STR));
	httpCloseSock();
	return;
    }

    getpeername(httpSock, (struct sockaddr *)&addr, &addrlen);
    rfbLog("httpd: get '%s' for %s\n", fname+1,
	   inet_ntoa(addr.sin_addr));

    /* If we were asked for '/', actually read the file index.vnc */

    if (strcmp(fname, "/") == 0) {
	strcpy(fname, "/index.vnc");
	rfbLog("httpd: defaulting to '%s'\n", fname+1);
    }

    /* Substitutions are performed on files ending .vnc */

    if (strlen(fname) >= 4 && strcmp(&fname[strlen(fname)-4], ".vnc") == 0) {
	performSubstitutions = TRUE;
    }

    /* Open the file */

    if ((fd = open(fullFname, O_RDONLY)) < 0) {
	rfbLogPerror("httpProcessInput: open");
	WriteExact(httpSock, NOT_FOUND_STR, strlen(NOT_FOUND_STR));
	httpCloseSock();
	return;
    }

    WriteExact(httpSock, OK_STR, strlen(OK_STR));

    while (1) {
	int n = read(fd, buf, BUF_SIZE-1);
	if (n < 0) {
	    rfbLogPerror("httpProcessInput: read");
	    close(fd);
	    httpCloseSock();
	    return;
	}

	if (n == 0)
	    break;

	if (performSubstitutions) {

	    /* Substitute $WIDTH, $HEIGHT, etc with the appropriate values.
	       This won't quite work properly if the .vnc file is longer than
	       BUF_SIZE, but it's reasonable to assume that .vnc files will
	       always be short. */

	    char *ptr = buf;
	    char *dollar;
	    buf[n] = 0; /* make sure it's null-terminated */

	    while ((dollar = strchr(ptr, '$'))) {
		WriteExact(httpSock, ptr, (dollar - ptr));

		ptr = dollar;

		if (compareAndSkip(&ptr, "$WIDTH")) {

		    sprintf(str, "%d", rfbScreen.width);
		    WriteExact(httpSock, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$HEIGHT")) {

		    sprintf(str, "%d", rfbScreen.height);
		    WriteExact(httpSock, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$APPLETWIDTH")) {

		    sprintf(str, "%d", rfbScreen.width);
		    WriteExact(httpSock, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$APPLETHEIGHT")) {

		    sprintf(str, "%d", rfbScreen.height + 32);
		    WriteExact(httpSock, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$PORT")) {

		    sprintf(str, "%d", rfbPort);
		    WriteExact(httpSock, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$DESKTOP")) {

		    WriteExact(httpSock, desktopName, strlen(desktopName));

		} else if (compareAndSkip(&ptr, "$DISPLAY")) {

		    sprintf(str, "%s:%s", rfbThisHost, display);
		    WriteExact(httpSock, str, strlen(str));

		} else if (compareAndSkip(&ptr, "$USER")) {

		    if (user) {
			WriteExact(httpSock, user->pw_name,
				   strlen(user->pw_name));
		    } else {
			WriteExact(httpSock, "?", 1);
		    }

		} else {
		    if (!compareAndSkip(&ptr, "$$"))
			ptr++;

		    if (WriteExact(httpSock, "$", 1) < 0) {
			close(fd);
			httpCloseSock();
			return;
		    }
		}
	    }
	    if (WriteExact(httpSock, ptr, (&buf[n] - ptr)) < 0)
		break;

	} else {

	    /* For files not ending .vnc, just write out the buffer */

	    if (WriteExact(httpSock, buf, n) < 0)
		break;
	}
    }

    close(fd);
    httpCloseSock();
}


static Bool
compareAndSkip(char **ptr, const char *str)
{
    if (strncmp(*ptr, str, strlen(str)) == 0) {
	*ptr += strlen(str);
	return TRUE;
    }

    return FALSE;
}
