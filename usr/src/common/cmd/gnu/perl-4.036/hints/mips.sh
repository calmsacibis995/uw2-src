#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

cmd_cflags='optimize="-g"'
perl_cflags='optimize="-g"'
tcmd_cflags='optimize="-g"'
tperl_cflags='optimize="-g"'
d_volatile=undef
d_castneg=undef
cc=cc
libpth="/usr/lib/cmplrs/cc $libpth"
groupstype=int
nm_opts='-B'
case $PATH in
*bsd*:/bin:*) cat <<END
NOTE:  Some people have reported having much better luck with Mips CC than
with the BSD cc.  Put /bin first in your PATH if you have difficulties.
END
;;
esac
