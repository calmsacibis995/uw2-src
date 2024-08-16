#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

echo " "
echo "NOTE: regression test op.read may fail due to an NFS bug in HP/UX."
echo "If so, don't worry about it."
case `(uname -r) 2>/dev/null` in
*3.1*) d_syscall=$undef ;;
*2.1*) libswanted=`echo $libswanted | sed 's/ malloc / /'` ;;
esac
d_index=define
