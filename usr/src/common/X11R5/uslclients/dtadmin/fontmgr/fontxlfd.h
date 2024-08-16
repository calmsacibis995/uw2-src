/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)dtadmin:fontmgr/fontxlfd.h	1.1"

typedef	struct _motif_font_resources {
	char *plain_font;
	char *italic_font;
	char *bold_font;
	char *italic_bold_font;
	Boolean complete_family;
} motif_font_resources;
