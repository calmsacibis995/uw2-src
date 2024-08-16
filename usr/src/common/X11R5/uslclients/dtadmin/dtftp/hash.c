/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:hash.c	1.1.2.1"
#endif

#include "ftp.h"
#include "SlideGizmo.h"

/*
 * The hash command has to be turned off and on, after every copy, because if
 * hash is on during a long listing - you will get # printed in the
 * middle of the listing.
 */
static void
_HashCmd (CmdPtr cp)
{
	Output (cp->cr, "hash\n");
}

extern StateTable HashTable[];

void
HashCmd (CnxtRec *cr, int grp)
{
	QueueCmd ("hash", cr, _HashCmd, HashTable, grp, 0, 0, 0, 0, Medium);
}

void
Hash (CnxtRec *cr)
{
	SliderGizmo *	g;

	cr->hash += strlen (cr->buffer);
	if (strchr (cr->buffer, '\n') != NULL) {
		cr->hash -= 1;	/* Don't count the final CR */
	}
	g = (SliderGizmo *) QueryGizmo (
		ModalGizmoClass, cr->sliderPopup, GetGizmoGizmo, "slider"
	);
	if (cr->hash <= cr->numHash) {
		SetSliderValue (g, (cr->hash*100)/cr->numHash);
	}
	else {
		SetSliderValue (g, 100);
	}
}
