/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ucb:common/ucbcmd/tbl/tf.c	1.2"
#ident	"$Header: $"
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved. The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
     
/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */
  

 /* tf.c: save and restore fill mode around table */
# include "t..c"
savefill()
{
/* remembers various things: fill mode, vs, ps in mac 35 (SF) */
fprintf(tabout, ".de %d\n",SF);
fprintf(tabout, ".ps \\n(.s\n");
fprintf(tabout, ".vs \\n(.vu\n");
fprintf(tabout, ".in \\n(.iu\n");
fprintf(tabout, ".if \\n(.u .fi\n");
fprintf(tabout, ".if \\n(.j .ad\n");
fprintf(tabout, ".if \\n(.j=0 .na\n");
fprintf(tabout, "..\n");
fprintf(tabout, ".nf\n");
/* set obx offset if useful */
fprintf(tabout, ".nr #~ 0\n");
fprintf(tabout, ".if n .nr #~ 0.6n\n");
}
rstofill()
{
fprintf(tabout, ".%d\n",SF);
}
endoff()
{
int i;
	for(i=0; i<MAXHEAD; i++)
		if (linestop[i])
			fprintf(tabout, ".nr #%c 0\n", 'a'+i);
	for(i=0; i<texct; i++)
		fprintf(tabout, ".rm %c+\n",texstr[i]);
fprintf(tabout, "%s\n", last);
}
ifdivert()
{
fprintf(tabout, ".ds #d .d\n");
fprintf(tabout, ".if \\(ts\\n(.z\\(ts\\(ts .ds #d nl\n");
}
saveline()
{
fprintf(tabout, ".if \\n+(b.=1 .nr d. \\n(.c-\\n(c.-1\n");
linstart=iline;
}
restline()
{
fprintf(tabout,".if \\n-(b.=0 .nr c. \\n(.c-\\n(d.-%d\n", iline-linstart);
linstart = 0;
}
cleanfc()
{
fprintf(tabout, ".fc\n");
}
