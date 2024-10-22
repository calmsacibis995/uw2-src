#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ufs.cmds:common/cmd/fs.d/ufs/ufs.mk	1.3.5.4"
#ident "$Header: ufs.mk 1.2 91/04/11 $"
#  /usr/src/cmd/lib/fs/ufs is the directory of all ufs specific commands
#  whose executable reside in $(INSDIR1) and $(INSDIR2).

include $(CMDRULES)

#
#  This is to build all the ufs commands
#
.DEFAULT:
	@for i in *;\
	do\
	    if [ -d $$i -a -f $$i/$$i.mk ]; \
		then \
		cd  $$i;\
		$(MAKE) -f $$i.mk $(MAKEARGS) $@; \
		cd .. ; \
	    fi;\
	done
