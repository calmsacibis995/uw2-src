#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

yacc='/usr/bin/yacc -Sm11000'
libswanted=`echo $libswanted | sed 's/ x / /'`
ccflags="$ccflags -U M_XENIX"
cppstdin='/lib/cpp -Di386 -DM_I386 -Dunix -DM_UNIX -DM_INTERNAT -DLAI_TCP'
cppminus=''
i_varargs=undef
d_rename='undef'
