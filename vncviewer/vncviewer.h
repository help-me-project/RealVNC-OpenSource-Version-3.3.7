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

/*
 * vncviewer.h
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <X11/Xmd.h>
#include <X11/keysym.h>
#include <X11/Xatom.h>
#include <X11/Xmu/StdSel.h>
#include <rfb/rfbproto.h>

extern int endianTest;

#define Swap16IfLE(s) \
    (*(char *)&endianTest ? ((((s) & 0xff) << 8) | (((s) >> 8) & 0xff)) : (s))

#define Swap32IfLE(l) \
    (*(char *)&endianTest ? ((((l) & 0xff000000) >> 24) | \
			     (((l) & 0x00ff0000) >> 8)  | \
			     (((l) & 0x0000ff00) << 8)  | \
			     (((l) & 0x000000ff) << 24))  : (l))

#define MAX_ENCODINGS 10

#define FLASH_PORT_OFFSET 5400
#define LISTEN_PORT_OFFSET 5500
#define SERVER_PORT_OFFSET 5900


/* argsresources.c */

typedef struct {
  Bool shareDesktop;
  Bool viewOnly;
  Bool fullScreen;

  Bool autoDetect;

  String encodingsString;

  Bool useBGR233;
  int nColours;
  Bool useSharedColours;
  Bool forceOwnCmap;
  Bool forceTrueColour;
  int requestedDepth;

  Bool useShm;

  int wmDecorationWidth;
  int wmDecorationHeight;

  char *passwordFile;
  Bool passwordDialog;

  int rawDelay;
  int copyRectDelay;

  Bool debug;

  int popupButtonCount;

  int bumpScrollTime;
  int bumpScrollPixels;

} AppData;

extern AppData appData;

extern char *fallback_resources[];
extern char vncServerHost[];
extern int vncServerPort;
extern Bool listenSpecified;
extern int listenPort, flashPort;

extern XrmOptionDescRec cmdLineOptions[];
extern int numCmdLineOptions;

extern void GetArgsAndResources(int argc, char **argv);
extern void SetFullScreenState(Widget w, XEvent *event, String *params,
			       Cardinal *num_params);
extern void SetBGR233State(Widget w, XEvent *event, String *params,
                           Cardinal *num_params);
extern void SetAutoState(Widget w, XEvent *event, String *params,
                         Cardinal *num_params);
extern void ToggleAuto(Widget w, XEvent *event, String *params,
                       Cardinal *num_params);
extern void ToggleBGR233(Widget w, XEvent *event, String *params,
                         Cardinal *num_params);

/* colour.c */

extern unsigned long BGR233ToPixel[];

extern Colormap cmap;
extern Visual *vis;
extern unsigned int visdepth, visbpp;
extern Bool usingBGR233;

extern void GetVisualAndCmap();
extern void SetMyFormatFromVisual();

/* desktop.c */

extern Atom wmDeleteWindow;
extern Widget form, viewport, desktop;
extern Window desktopWin;
extern GC gc;
extern GC srcGC, dstGC;
extern Dimension dpyWidth, dpyHeight;

extern void DesktopInitBeforeRealization();
extern void DesktopInitAfterRealization();
extern void SendRFBEvent(Widget w, XEvent *event, String *params,
			 Cardinal *num_params);
extern void CopyDataToScreen(char *buf, int x, int y, int width, int height);
extern void SynchroniseScreen();

/* dialogs.c */

extern void ServerDialogDone(Widget w, XEvent *event, String *params,
			     Cardinal *num_params);
extern char *DoServerDialog();
extern void PasswordDialogDone(Widget w, XEvent *event, String *params,
			     Cardinal *num_params);
extern char *DoPasswordDialog();

/* fullscreen.c */

extern void ToggleFullScreen(Widget w, XEvent *event, String *params,
			     Cardinal *num_params);
extern Bool BumpScroll(XEvent *ev);
extern void FullScreenOn();
extern void FullScreenOff();

/* listen.c */

extern void listenForIncomingConnections();

/* misc.c */

extern void ToplevelInitBeforeRealization();
extern void ToplevelInitAfterRealization();
extern Time TimeFromEvent(XEvent *ev);
extern void Pause(Widget w, XEvent *event, String *params,
		  Cardinal *num_params);
extern void Quit(Widget w, XEvent *event, String *params,
		 Cardinal *num_params);
extern void Cleanup();

/* popup.c */

extern Widget popup;
extern void ShowPopup(Widget w, XEvent *event, String *params,
		      Cardinal *num_params);
extern void HidePopup(Widget w, XEvent *event, String *params,
		      Cardinal *num_params);
extern void CreatePopup();

/* rfbproto.c */

extern Bool canUseCoRRE;
extern Bool canUseHextile;
extern char *desktopName;
extern rfbPixelFormat myFormat;
extern Bool pendingFormatChange;
extern rfbServerInitMsg si;
extern char *serverCutText;
extern Bool newServerCutText;

extern Bool InitialiseRFBConnection();
extern Bool SendSetPixelFormat();
extern Bool SendSetEncodings();
extern void UpdateNeeded(int x, int y, int w, int h);
extern Bool CheckUpdateNeeded();
extern Bool SendPointerEvent(int x, int y, int buttonMask);
extern Bool SendKeyEvent(CARD32 key, Bool down);
extern Bool SendClientCutText(char *str, int len);
extern Bool HandleRFBServerMessage();

extern void PrintPixelFormat(rfbPixelFormat *format);

/* selection.c */

extern void InitialiseSelection();
extern void SelectionToVNC(Widget w, XEvent *event, String *params,
			   Cardinal *num_params);
extern void SelectionFromVNC(Widget w, XEvent *event, String *params,
			     Cardinal *num_params);

/* shm.c */

extern XImage *CreateShmImage();
extern void ShmCleanup();

/* sockets.cxx */

extern Bool sameMachine;

extern Bool ConnectToRFBServer(const char *hostname, int port);
extern Bool SetRFBSock(int sock);
extern void StartTiming();
extern void StopTiming();
extern int KbitsPerSecond();
extern int TimeWaitedIn100us();
extern Bool ReadFromRFBServer(char *out, unsigned int n);
extern Bool WriteToRFBServer(char *buf, int n);
extern int ConnectToTcpAddr(const char* hostname, int port);
extern int ListenAtTcpPort(int port);
extern int AcceptTcpConnection(int listenSock);

extern int StringToIPAddr(const char *str, unsigned int *addr);

/* vncviewer.c */

extern char *programName;
extern XtAppContext appContext;
extern Display* dpy;
extern Widget toplevel;

/* zrle.cxx */

extern Bool zrleDecode(int x, int y, int w, int h);
