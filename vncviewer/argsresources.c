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
 * argsresources.c - deal with command-line args and resources.
 */

#include "vncviewer.h"
#include <X11/Xaw/Toggle.h>

/*
 * fallback_resources - these are used if there is no app-defaults file
 * installed in one of the standard places.
 */

char *fallback_resources[] = {

  "Vncviewer.title: VNC: %s",

  "Vncviewer.translations:\
    <Enter>: SelectionToVNC()\\n\
    <Leave>: SelectionFromVNC()",

  "*form.background: black",

  "*viewport.allowHoriz: True",
  "*viewport.allowVert: True",
  "*viewport.useBottom: True",
  "*viewport.useRight: True",
  "*viewport*Scrollbar*thumb: None",

  "*desktop.baseTranslations:\
     <Key>F8: ShowPopup()\\n\
     <ButtonPress>: SendRFBEvent()\\n\
     <ButtonRelease>: SendRFBEvent()\\n\
     <Motion>: SendRFBEvent()\\n\
     <KeyPress>: SendRFBEvent()\\n\
     <KeyRelease>: SendRFBEvent()",

  "*serverDialog.dialog.label: VNC server:",
  "*serverDialog.dialog.value:",
  "*serverDialog.dialog.value.translations: #override\\n\
     <Key>Return: ServerDialogDone()",

  "*passwordDialog.dialog.label: Password:",
  "*passwordDialog.dialog.value:",
  "*passwordDialog.dialog.value.AsciiSink.echo: False",
  "*passwordDialog.dialog.value.translations: #override\\n\
     <Key>Return: PasswordDialogDone()",

  "*popup.title: VNC popup",
  "*popup*background: grey",
  "*popup*font: -*-helvetica-bold-r-*-*-16-*-*-*-*-*-*-*",
  "*popup.buttonForm.Command.borderWidth: 0",
  "*popup.buttonForm.Toggle.borderWidth: 0",

  "*popup.translations: #override <Message>WM_PROTOCOLS: HidePopup()",
  "*popup.buttonForm.translations: #override\\n\
     <KeyPress>: SendRFBEvent() HidePopup()",

  "*popupButtonCount: 9",

  "*popup*button1.label: Dismiss popup",
  "*popup*button1.translations: #override\\n\
     <Btn1Down>,<Btn1Up>: HidePopup()",

  "*popup*button2.label: Quit viewer",
  "*popup*button2.translations: #override\\n\
     <Btn1Down>,<Btn1Up>: Quit()",

  "*popup*button3.label: Full screen",
  "*popup*button3.type: toggle",
  "*popup*button3.translations: #override\\n\
     <Visible>: SetFullScreenState()\\n\
     <Btn1Down>,<Btn1Up>: ToggleFullScreen() HidePopup()",

  "*popup*button4.label: Clipboard: local -> remote",
  "*popup*button4.translations: #override\\n\
     <Btn1Down>,<Btn1Up>: SelectionToVNC(always) HidePopup()",

  "*popup*button5.label: Clipboard: local <- remote",
  "*popup*button5.translations: #override\\n\
     <Btn1Down>,<Btn1Up>: SelectionFromVNC(always) HidePopup()",

  "*popup*button6.label: Send ctrl-alt-del",
  "*popup*button6.translations: #override\\n\
     <Btn1Down>,<Btn1Up>: SendRFBEvent(keydown,Control_L)\
                          SendRFBEvent(keydown,Alt_L)\
                          SendRFBEvent(key,Delete)\
                          SendRFBEvent(keyup,Alt_L)\
                          SendRFBEvent(keyup,Control_L)\
                          HidePopup()",

  "*popup*button7.label: Send F8",
  "*popup*button7.translations: #override\\n\
     <Btn1Down>,<Btn1Up>: SendRFBEvent(key,F8) HidePopup()",

  "*popup*button8.label: Auto select format/encoding",
  "*popup*button8.type: toggle",
  "*popup*button8.translations: #override\\n\
     <Visible>: SetAutoState()\\n\
     <Btn1Down>,<Btn1Up>: ToggleAuto() HidePopup()",

  "*popup*button9.label: Use 8-bit colour",
  "*popup*button9.type: toggle",
  "*popup*button9.translations: #override\\n\
     <Visible>: SetBGR233State()\\n\
     <Btn1Down>,<Btn1Up>: ToggleBGR233() HidePopup()",

  NULL
};


/*
 * vncServerHost and vncServerPort are set either from the command line or
 * from a dialog box.
 */

char vncServerHost[256];
int vncServerPort = 0;


/*
 * appData is our application-specific data which can be set by the user with
 * application resource specs.  The AppData structure is defined in the header
 * file.
 */

AppData appData;

static XtResource appDataResourceList[] = {
  {"shareDesktop", "ShareDesktop", XtRBool, sizeof(Bool),
   XtOffsetOf(AppData, shareDesktop), XtRImmediate, (XtPointer) False},

  {"viewOnly", "ViewOnly", XtRBool, sizeof(Bool),
   XtOffsetOf(AppData, viewOnly), XtRImmediate, (XtPointer) False},

  {"fullScreen", "FullScreen", XtRBool, sizeof(Bool),
   XtOffsetOf(AppData, fullScreen), XtRImmediate, (XtPointer) False},

  {"passwordFile", "PasswordFile", XtRString, sizeof(String),
   XtOffsetOf(AppData, passwordFile), XtRImmediate, (XtPointer) 0},

  {"passwordDialog", "PasswordDialog", XtRBool, sizeof(Bool),
   XtOffsetOf(AppData, passwordDialog), XtRImmediate, (XtPointer) False},

  {"autoDetect", "AutoDetect", XtRBool, sizeof(Bool),
   XtOffsetOf(AppData, autoDetect), XtRImmediate, (XtPointer) True},

  {"encodings", "Encodings", XtRString, sizeof(String),
   XtOffsetOf(AppData, encodingsString), XtRImmediate, (XtPointer) 0},

  {"useBGR233", "UseBGR233", XtRBool, sizeof(Bool),
   XtOffsetOf(AppData, useBGR233), XtRImmediate, (XtPointer) False},

  {"nColours", "NColours", XtRInt, sizeof(int),
   XtOffsetOf(AppData, nColours), XtRImmediate, (XtPointer) 256},

  {"useSharedColours", "UseSharedColours", XtRBool, sizeof(Bool),
   XtOffsetOf(AppData, useSharedColours), XtRImmediate, (XtPointer) True},

  {"forceOwnCmap", "ForceOwnCmap", XtRBool, sizeof(Bool),
   XtOffsetOf(AppData, forceOwnCmap), XtRImmediate, (XtPointer) False},

  {"forceTrueColour", "ForceTrueColour", XtRBool, sizeof(Bool),
   XtOffsetOf(AppData, forceTrueColour), XtRImmediate, (XtPointer) False},

  {"requestedDepth", "RequestedDepth", XtRInt, sizeof(int),
   XtOffsetOf(AppData, requestedDepth), XtRImmediate, (XtPointer) 0},

  {"useSharedMemory", "UseSharedMemory", XtRBool, sizeof(Bool),
   XtOffsetOf(AppData, useShm), XtRImmediate, (XtPointer) True},

  {"wmDecorationWidth", "WmDecorationWidth", XtRInt, sizeof(int),
   XtOffsetOf(AppData, wmDecorationWidth), XtRImmediate, (XtPointer) 4},

  {"wmDecorationHeight", "WmDecorationHeight", XtRInt, sizeof(int),
   XtOffsetOf(AppData, wmDecorationHeight), XtRImmediate, (XtPointer) 24},

  {"popupButtonCount", "PopupButtonCount", XtRInt, sizeof(int),
   XtOffsetOf(AppData, popupButtonCount), XtRImmediate, (XtPointer) 0},

  {"debug", "Debug", XtRBool, sizeof(Bool),
   XtOffsetOf(AppData, debug), XtRImmediate, (XtPointer) False},

  {"rawDelay", "RawDelay", XtRInt, sizeof(int),
   XtOffsetOf(AppData, rawDelay), XtRImmediate, (XtPointer) 0},

  {"copyRectDelay", "CopyRectDelay", XtRInt, sizeof(int),
   XtOffsetOf(AppData, copyRectDelay), XtRImmediate, (XtPointer) 0},

  {"bumpScrollTime", "BumpScrollTime", XtRInt, sizeof(int),
   XtOffsetOf(AppData, bumpScrollTime), XtRImmediate, (XtPointer) 25},

  {"bumpScrollPixels", "BumpScrollPixels", XtRInt, sizeof(int),
   XtOffsetOf(AppData, bumpScrollPixels), XtRImmediate, (XtPointer) 20},
};


/*
 * The cmdLineOptions array specifies how certain app resource specs can be set
 * with command-line options.
 */

XrmOptionDescRec cmdLineOptions[] = {
  {"-shared",     "*shareDesktop",      XrmoptionNoArg,  "True"},
  {"-viewonly",   "*viewOnly",          XrmoptionNoArg,  "True"},
  {"-fullscreen", "*fullScreen",        XrmoptionNoArg,  "True"},
  {"-passwd",     "*passwordFile",      XrmoptionSepArg, 0},
  {"-noauto",     "*autoDetect",        XrmoptionNoArg,  "False"},
  {"-encodings",  "*encodings",         XrmoptionSepArg, 0},
  {"-bgr233",     "*useBGR233",         XrmoptionNoArg,  "True"},
  {"-8bit",       "*useBGR233",         XrmoptionNoArg,  "True"},
  {"-owncmap",    "*forceOwnCmap",      XrmoptionNoArg,  "True"},
  {"-truecolor",  "*forceTrueColour",   XrmoptionNoArg,  "True"},
  {"-truecolour", "*forceTrueColour",   XrmoptionNoArg,  "True"},
  {"-depth",      "*requestedDepth",    XrmoptionSepArg, 0},
};

int numCmdLineOptions = XtNumber(cmdLineOptions);


/*
 * actions[] specifies actions that can be used in widget resource specs.
 */

static XtActionsRec actions[] = {
    {"SendRFBEvent", SendRFBEvent},
    {"ShowPopup", ShowPopup},
    {"HidePopup", HidePopup},
    {"ToggleFullScreen", ToggleFullScreen},
    {"SetFullScreenState", SetFullScreenState},
    {"ToggleBGR233", ToggleBGR233},
    {"SetBGR233State", SetBGR233State},
    {"ToggleAuto", ToggleAuto},
    {"SetAutoState", SetAutoState},
    {"SelectionFromVNC", SelectionFromVNC},
    {"SelectionToVNC", SelectionToVNC},
    {"ServerDialogDone", ServerDialogDone},
    {"PasswordDialogDone", PasswordDialogDone},
    {"Pause", Pause},
    {"Quit", Quit},
};


/*
 * usage() prints out the usage message.
 */

void
usage()
{
  fprintf(stderr,"\n"
	  "usage: %s [<options>] <host>:<display#>\n"
	  "       %s [<options>] -listen [<display#>]\n"
	  "\n"
	  "<options> are standard Xt options, or:\n"
	  "              -shared\n"
	  "              -viewonly\n"
	  "              -fullscreen\n"
	  "              -passwd <passwd-file>\n"
	  "              -noauto\n"
	  "              -encodings <encoding-list> (e.g. \"raw copyrect\")\n"
	  "              -bgr233\n"
	  "              -owncmap\n"
	  "              -truecolour\n"
	  "              -depth <depth>\n"
	  ,programName,programName);
  exit(1);
}


/*
 * GetArgsAndResources() deals with resources and any command-line arguments
 * not already processed by XtVaAppInitialize().  It sets vncServerHost and
 * vncServerPort and all the fields in appData.
 */

void
GetArgsAndResources(int argc, char **argv)
{
  int i;
  char *vncServerName = 0;

  /* Turn app resource specs into our appData structure for the rest of the
     program to use */

  XtGetApplicationResources(toplevel, &appData, appDataResourceList,
			    XtNumber(appDataResourceList), 0, 0);

  if (appData.encodingsString || appData.useBGR233) {
    appData.autoDetect = False;
  }

  if (appData.autoDetect) {
    appData.useBGR233 = True;
  }

  /* Add our actions to the actions table so they can be used in widget
     resource specs */

  XtAppAddActions(appContext, actions, XtNumber(actions));

  /* Check any remaining command-line arguments.  If -listen was specified
     there should be none.  Otherwise the only argument should be the VNC
     server name.  If not given then pop up a dialog box and wait for the
     server name to be entered. */

  if (listenSpecified) {
    if (argc != 1) {
      fprintf(stderr,"\n%s -listen: invalid command line argument: %s\n",
	      programName, argv[1]);
      usage();
    }
    return;
  }

  if (argc == 1) {
    vncServerName = DoServerDialog();
    appData.passwordDialog = True;
  } else if (argc != 2) {
    usage();
  } else {
    vncServerName = argv[1];

    if (vncServerName[0] == '-')
      usage();
  }

  if (strlen(vncServerName) > 255) {
    fprintf(stderr,"VNC server name too long\n");
    exit(1);
  }

  for (i = 0; vncServerName[i] != ':' && vncServerName[i] != 0; i++);

  strncpy(vncServerHost, vncServerName, i);
  vncServerHost[i] = 0;

  if (vncServerName[i] == ':') {
    vncServerPort = atoi(&vncServerName[i+1]);
  } else {
    vncServerPort = 0;
  }

  if (vncServerPort < 100)
    vncServerPort += SERVER_PORT_OFFSET;
}


/*
 * SetXXXState is an action which sets the "state" resource of a toggle
 * widget to reflect the value of a boolean flag.
 */

void SetFullScreenState(Widget w, XEvent *ev, String *params, Cardinal *num)
{
  XtVaSetValues(w, XtNstate, appData.fullScreen, NULL);
}
void SetBGR233State(Widget w, XEvent *ev, String *params, Cardinal *num)
{
  XtVaSetValues(w, XtNstate, appData.useBGR233, NULL);
}
void SetAutoState(Widget w, XEvent *ev, String *params, Cardinal *num)
{
  XtVaSetValues(w, XtNstate, appData.autoDetect, NULL);
}

/*
 * ToggleAuto() toggles the use of autoDetect, if appropriate.
 */

void ToggleAuto(Widget w, XEvent *event, String *params, Cardinal *num)
{
  if (vis->class == TrueColor) {
    /* only makes sense if using a TrueColor visual */
    appData.autoDetect = !appData.autoDetect;
  } else {
    XBell(dpy,100);
  }
  SetAutoState(w, event, 0, 0);
}

/*
 * ToggleBGR233() toggles the use of bgr233, if appropriate.
 */

void ToggleBGR233(Widget w, XEvent *event, String *params, Cardinal *num)
{
  if (vis->class == TrueColor) {
    /* only makes sense if using a TrueColor visual */
    appData.useBGR233 = !appData.useBGR233;
    if (appData.useBGR233)
      appData.autoDetect = False; /* otherwise autoDetect may just turn it off
                                     again */
    pendingFormatChange = True;
  } else {
    XBell(dpy,100);
  }
  SetBGR233State(w, event, 0, 0);
}

