/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*      Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.   */
/*      Copyright (c) 1988, 1989, 1990 AT&T     */
/*        All Rights Reserved   */

/*      THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF          */
/*      UNIX System Laboratories, Inc.                          */
/*      The copyright notice above does not evidence any        */
/*      actual or intended publication of such source code.     */
#ident	"@(#)langsup:common/ls/Xim/lslib/ol/messages.h	1.2"

#define MsgCatalogue       "elsim:" /* name of the catalogue */
#define MSGCATLEN           6       /* length og MsgCatalogue */
#define FS                 "\000"   /* Field Separator, for trickery later */

/* Input Method messages */
#define NoFontsInFontSet   "1" FS "No fonts in font set"
#define OutOfMemory        "2" FS "Out of memory"
#define CantEnquireStyles  "3" FS "Could not enquire IM supported styles"
#define NoClientOrFocusWin "4" FS "No client/focus window associated with input context"
#define NoClientWindow     "5" FS "Input context has no client window ID"
#define OlNresourceDB      "6" FS "OlNresourceDatabase not handled"
#define IllegalAttr	   "7" FS "Illegal attribute: "
