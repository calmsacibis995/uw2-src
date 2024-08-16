#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

ccflags="$ccflags -DLANGUAGE_C"
tmp="`(uname -a) 2>/dev/null`"
case "$tmp" in
*3.[01]*RISC) d_waitpid=$undef;;
'') d_waitpid=$undef;;
esac
case "$tmp" in
*RISC)
    cmd_cflags='optimize="-g"'
    perl_cflags='optimize="-g"'
    tcmd_cflags='optimize="-g"'
    tperl_cflags='optimize="-g"'
    ;;
esac
