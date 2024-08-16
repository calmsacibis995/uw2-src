/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	NOIDENT
#ident	"@(#)xterm:Strings.h	1.19"
#endif

/* Strings (names and types) */

#ifndef _XTERM_STRINGS_H_
#define _XTERM_STRINGS_H_

#define OleCOlClientXtermMsgs (OLconst char *)"xterm_msgs"

/* Tekproc.c */
#define OleNperror (OLconst char *)"perror"
#define OleTreason (OLconst char *)"reason"

#define OleNioctl (OLconst char *)"ioctl"
#define OleTwinSz (OLconst char *)"winsize"
#define OleNfont (OLconst char *)"font"
#define OleTbadFont (OLconst char *)"badfont"
#define OleNcreate (OLconst char *)"create"
#define OleTbadWindow1 (OLconst char *)"badwindow1"

#define OleTtekMode (OLconst char *)"tekmode"

/* charproc.c */
#define OleNuname (OLconst char *)"uname"
#define OleTbadUname (OLconst char *)"baduname"

#define OleNtitle (OLconst char *)"title"
#define OleTuntitled (OLconst char *)"untitled"

#define OleNname (OLconst char *)"name"
#define OleTnotLocal (OLconst char *)"notlocal"

#define OleNspace (OLconst char *)"space"
#define OleTbuffer (OLconst char *)"buffer"

#define OleTtruncate (OLconst char *)"truncate"

#define OleNread (OLconst char *)"read"
#define OleTbadConsole (OLconst char *)"badconsole"

/* main.c */
#define OleTxterm (OLconst char *)"xterm"


#define OleNtty (OLconst char *)"tty"
#define OleTbadTty (OLconst char *)"badtty"


#define OleNdup2 (OLconst char *)"dup2"

#define OleNusage (OLconst char *)"usage"
#define OleTmsg1a (OLconst char *)"msg1a"
#define OleTmsg2a (OLconst char *)"msg2a"
#define OleTmsg3a (OLconst char *)"msg3a"
#define OleTmsg7a (OLconst char *)"msg7a"
#define OleTmsg7b (OLconst char *)"msg7b"
#define OleTmsg7c (OLconst char *)"msg7c"
#define OleTmsg7d (OLconst char *)"msg7d"
#define OleTmsg7e (OLconst char *)"msg7e"
#define OleTmsg7f (OLconst char *)"msg7f"
#define OleTmsg8a (OLconst char *)"msg8a"
#define OleTmsg19a (OLconst char *)"msg19a"
#define OleTmsg19b (OLconst char *)"msg19b"
#define OleTmsg25 (OLconst char *)"msg25"
#define OleTmsg26 (OLconst char *)"msg26"
#define OleTmsg27 (OLconst char *)"msg27"
#define OleTmsg28 (OLconst char *)"msg28"
#define OleTmsg29 (OLconst char *)"msg29"

#define OleNpty (OLconst char *)"pty"
#define OleTnoAvail (OLconst char *)"noavail"

#define OleNsignal (OLconst char *)"signal"
#define OleTsighup (OLconst char *)"sighup"

#define OleTopen (OLconst char *)"open"
#define OleTbadDup2Msg2 (OLconst char *)"baddup2msg2"

#define OleNexec (OLconst char *)"exec"
#define OleTbadExecvp (OLconst char *)"badexecvp"

#define OleTbadExeclp (OLconst char *)"badexeclp"

#define OleNstrindex (OLconst char *)"strindx"
#define OleTco (OLconst char *)"co"
#define OleTli (OLconst char *)"li"

#define OleNopen (OLconst char *)"open"
#define OleNhelp (OLconst char *)"help"
#ifndef DTM_HELP
#define OleThelpString (OLconst char *)"helpstring"
#endif /* DTM_HELP */
/*
 *****************************************
 * Menu.c ...
 */
#define OleNlabel	(OLconst char *)"label"
#define OleNmnemonic	(OLconst char *)"mnem"

/* menu labels and mnemonics */
/* #define OleTedit	"ed" */
#define OleTredraw	(OLconst char *)"rd"
#define OleTsoftReset	(OLconst char *)"sR"
#define OleTfullReset	(OLconst char *)"fR"
#define OleTproperties	(OLconst char *)"prop"
#define OleTshowTekWin	(OLconst char *)"stw"
#define OleTinterrupt	(OLconst char *)"intt"
#define OleThangup	(OLconst char *)"hang"
#define OleTterminate	(OLconst char *)"term"
#define OleTkill	(OLconst char *)"kil"

#define OleTpage	(OLconst char *)"pg"
#define OleTreset	(OLconst char *)"rst"
#define OleTcopy2	(OLconst char *)"cp2"


#define OleTsend	(OLconst char *)"snd"
/* #define OleTpaste	"pst" */
/* #define OleTcopy	"cp" */
/* #define OleTcut		"ct" */

#define OleThideTek	(OLconst char *)"hidetek"
#define OleTshowTek	(OLconst char *)"showtek"
#define OleThideVt	(OLconst char *)"hidevt"
#define OleTshowVt	(OLconst char *)"showvt"

#define OleNcheckbox	(OLconst char *)"checkbox"
#define OleTvisualBell	(OLconst char *)"visualbell_lab"
#define OleTlogging	(OLconst char *)"logging_lab"
#define OleTjumpScroll	(OLconst char *)"jumpscroll_lab"
#define OleTreverseVideo (OLconst char *)"reversevideo_lab"
#define OleTautoWrap	(OLconst char *)"autowraparound_lab"
#define OleTreverseWrap	(OLconst char *)"reversewarparound_lab"
#define OleTautoLf	(OLconst char *)"autolinefeed_lab"
#define OleTappCursor	(OLconst char *)"applicationcursor_lab"
#define OleTappPad	(OLconst char *)"applicationpad_lab"
#define OleTscrollbar	(OLconst char *)"scrollbar_lab"
#define OleTmarginBell	(OLconst char *)"marginbell_lab"
#define OleTsecureKbd	(OLconst char *)"securekeyboard_lab"
#define OleTcursesResize (OLconst char *)"cursesresize_lab"
#ifdef XTERM_COMPAT
#define OleTautoRepeat 	(OLconst char *)"autorepeat_lab"
#define OleTscrollonKey	(OLconst char *)"scrollonkey_lab"
#define OleTscrollonInput	(OLconst char *)"scrolloninput_lab"
#endif

#define OleNexcl	(OLconst char *)"excl"
#define OleTlargeChar	(OLconst char *)"large"
#define OleTmediumChar	(OLconst char *)"medium"
#define OleTsmallChar	(OLconst char *)"small"
#define OleTtinyChar	(OLconst char *)"tiny"

/* misc.c */
#define OleTtek (OLconst char *)"(Tek)"
#define OleTbadExecl (OLconst char *)"badexecl"
#define OleNaccess (OLconst char *)"access"
#define OleTloginFile (OLconst char *)"loginfile"
#define OleNprintf (OLconst char *)"printf"
#define OleTerrmsg1 (OLconst char *)"errmsg1"
#define OleTerrmsg2 (OLconst char *)"errmsg2"
#define OleTerrmsg3 (OLconst char *)"errmsg3"

/* openpty.c */
#define OleTopenpty1 (OLconst char *)"openpty1"
#define OleTopenpty2 (OLconst char *)"openpty2"
#define OleTptem (OLconst char *)"ptem"
#define OleTconsem (OLconst char *)"consem"
#define OleTldterm (OLconst char *)"ldterm"
#define OleTttcompat (OLconst char *)"ttcompat"

#define OleTtcseta (OLconst char *)"tcseta"

#define OleTbadConsole2 (OLconst char *)"badconsole2"
#define OleTwinSz2 (OLconst char *)"winsz2"

#define OleNputenv (OLconst char *)"putenv"
#define OleTbadPutenv (OLconst char *)"badputenv"

/* resize.c */
#define OleNtimeout (OLconst char *)"timeout"
#define OleTnoTime (OLconst char *)"notime"

#define OleNsetsize (OLconst char *)"setsize"
#define OleTbadSetsize (OLconst char *)"badsetsize"

#define OleNfopen (OLconst char *)"fopen"
#define OleTbadFopentty (OLconst char *)"badfopentty"

#define OleNtgetent (OLconst char *)"tgetent"
#define OleTbadTgetent (OLconst char *)"badtgetent"

#define OleNreadstring (OLconst char *)"readstring"
#define OleTnoRowscols (OLconst char *)"norowscols"
#define OleTwinSize (OLconst char *)"winsz"

#define OleTco2 (OLconst char *)"co2"
#define OleTli2 (OLconst char *)"li2"

#define OleNgetc (OLconst char *)"getc"
#define OleTunknownChar (OLconst char *)"unknownchar"

#define OleTmsg30 (OLconst char *)"msg30"
#define OleTmsg31 (OLconst char *)"msg31"

/* VTinit.c */
#define OleNolopenIM (OLconst char *)"olopenim"
#define OleTbadOlopenIM (OLconst char *)"badolopenim"

#define OleNolcreateIc (OLconst char *)"olcreateic"
#define	OleTbadOlcreateIc (OLconst char *)"badolcreateic"

#define OleNolsetIcValues (OLconst char *)"olseticvalues"
#define OleTbadOlsetIcValues (OLconst char *)"badolseticvalues"

#define OleNolsetIcValues (OLconst char *)"olseticvalues"
#define OleTbadOlsetIcValuesStatus (OLconst char *)"badolseticvaluesstatus"

/* from screen.c */
#define OleNpixel (OLconst char *)"pixel"
#define OleTbadPixel (OLconst char *)"badpixel"

#define OleNolGetnextstrsegment (OLconst char *)"olgetnextstrsegment"
#define OleTbadOlgetnextstrsegment (OLconst char *)"badolgetnextstrsegment"

#define OleNpanic	(OLconst char *)"panic"
#define OleTpanic_msg1	(OLconst char *)"msg1"
#define OleTpanic_msg2	(OLconst char *)"msg2"
#define OleTpanic_msg3	(OLconst char *)"msg3"
#define OleTpanic_msg4	(OLconst char *)"msg4"
#define OleTpanic_msg5	(OLconst char *)"msg5"
/* #define OleTpanic_msg6	"msg6" 	/* no longer used: free	*/
#define OleTpanic_msg7	(OLconst char *)"msg7"
#define OleTpanic_msg8	(OLconst char *)"msg8"
#define OleTpanic_msg9	(OLconst char *)"msg9"
#define OleTpanic_msg10	(OLconst char *)"msg10"
#define OleTpanic_msg11	(OLconst char *)"msg11"
#define OleTpanic_msg12	(OLconst char *)"msg12"
#define OleTpanic_msg13	(OLconst char *)"msg13"
#endif
