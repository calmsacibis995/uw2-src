#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

set `echo $libswanted | sed -e 's/ x / /' -e 's/ PW / /' -e 's/ malloc / /'`
libswanted="inet malloc $*"
doio_cflags='ccflags="$ccflags -DENOTSOCK=103"'
tdoio_cflags='ccflags="$ccflags -DENOTSOCK=103"'
echo "<net/errno.h> defines error numbers for network calls, but"
echo "the definitions for ENAMETOOLONG and ENOTEMPTY conflict with"
echo "those in <sys/errno.h>.  Instead just define ENOTSOCK here."
