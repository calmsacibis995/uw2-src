/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3__gs__.c	1.4"

#include <sidep.h>

extern void s3_state__gs_change__(void);
extern void s3_fill__gs_change__(void);
extern void s3_font__gs_change__(void);
extern void s3_line__gs_change__(void);
extern void s3_polypoint__gs_change__(void);
extern void s3_arc__gs_change__(void);
extern void s3_fillspans__gs_change__(void);
extern void s3_bitblt__gs_change__(void);

void
s3__gs_change__(void)
{
	s3_state__gs_change__();
	s3_fill__gs_change__();
	s3_font__gs_change__();
	s3_line__gs_change__();
	s3_polypoint__gs_change__();
	s3_arc__gs_change__();
	s3_fillspans__gs_change__();
	s3_bitblt__gs_change__();
}
