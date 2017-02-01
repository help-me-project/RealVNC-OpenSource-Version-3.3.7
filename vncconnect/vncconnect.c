/*
 * vncconnect.c
 */

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

static char *programName;

static void usage()
{
    fprintf(stderr,"\nusage: %s [-display Xvnc-display] host[:port]\n"
            "       %s [-display Xvnc-display] -away\n"
            "       %s [-display Xvnc-display] -deferupdate <time>\n\n"
            "Tells Xvnc to connect to a listening VNC viewer on the given"
            " host and port\n"
            "or to disconnect from all viewers with the -away flag.\n"
            "The -deferupdate flag changes the server's defer update time"
            " in milliseconds.\n",
	    programName, programName, programName);
    exit(1);
}

int main(argc, argv)
    int argc;
    char **argv;
{
    char *displayname = NULL;
    Display *dpy;
    int i;
    Atom VNC_CONNECT, VNC_DEFER_UPDATE;
    Bool away = False;
    char *hostAndPort = "";
    char *deferUpdate = 0;

    programName = argv[0];
    
    for (i = 1; i < argc; i++) {
	if (argv[i][0] != '-')
	    break;

        if (strcmp(argv[i],"-display") == 0) {
            if (++i >= argc) usage();
	    displayname = argv[i];
        } else if (strcmp(argv[i], "-away") == 0) {
            away = True;
        } else if (strcmp(argv[i], "-deferupdate") == 0) {
            if (++i >= argc) usage();
	    deferUpdate = argv[i];
        } else {
	    usage();
	}
    }

    if (away) {
        if (argc != i)
            usage();
    } else if (deferUpdate) {
    } else {
        if (argc != i+1)
            usage();
        hostAndPort = argv[i];
    }

    if (!(dpy = XOpenDisplay(displayname))) {
	fprintf(stderr,"%s: unable to open display \"%s\"\n",
		programName, XDisplayName (displayname));
	exit(1);
    }

    if (deferUpdate) {
      VNC_DEFER_UPDATE = XInternAtom(dpy, "VNC_DEFER_UPDATE", False);
      XChangeProperty(dpy, DefaultRootWindow(dpy), VNC_DEFER_UPDATE, XA_STRING,
                      8, PropModeReplace, (unsigned char *)deferUpdate,
                      strlen(deferUpdate));
    } else {
      VNC_CONNECT = XInternAtom(dpy, "VNC_CONNECT", False);
      XChangeProperty(dpy, DefaultRootWindow(dpy), VNC_CONNECT, XA_STRING, 8,
                      PropModeReplace, (unsigned char *)hostAndPort,
                      strlen(hostAndPort));
    }

    XCloseDisplay(dpy);

    return 0;
}
