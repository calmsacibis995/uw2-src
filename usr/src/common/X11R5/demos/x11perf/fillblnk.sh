#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)r5x11perf:fillblnk.sh	1.1"
#!/bin/sh
echo '#####EOF#####' | cat $2 - $1 |
awk -F: '\
$1 == "#####EOF#####"	{ filling = 1; currentItem = 1; lastItem = NR; next; }
filling != "1"	{ itemOrder[" " $1] = NR; name[NR] = $1; }
filling == "1"	{ rate[itemOrder[$2]] = $1; }
END	{
	for (i = 1; i < lastItem; i++) {
		if (rate[i] != "") {
			printf ("%s: %s\n", rate[i], name[i]);
		} else {
			printf (" 0 trep @ 0.0 msec (0.0/sec): %s\n", name[i]);
		}
	}
	}'
