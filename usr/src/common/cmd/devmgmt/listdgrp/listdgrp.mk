#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


#ident	"@(#)devmgmt:common/cmd/devmgmt/listdgrp/listdgrp.mk	1.9.5.3"
#ident "$Header: listdgrp.mk 1.2 91/04/05 $"

include $(CMDRULES)

HDRS=$(INC)/stdio.h $(INC)/string.h $(INC)/devmgmt.h
FILE=listdgrp
PROTO=../Prototype
OBJECTS=listdgrp
SRC=main.c
OBJ=$(SRC:.c=.o)
LOCALINC=-I.
LDLIBS=-ladm
LINTFLAGS=$(DEFLIST)

all		: $(FILE) 

install: all
	@eval `sed -e '/^![^=]*=/!d' -e 's/^!//' $(PROTO)` ;\
	mkpath() { \
		while true ;\
		do \
			tmpdir=$$1 ;\
			[ -d $$tmpdir ] && break ;\
			while [ ! -d $$tmpdir ] ;\
			do \
				lastdir=$$tmpdir ;\
				tmpdir=`dirname $$tmpdir` ;\
			done ;\
			mkdir $$lastdir ;\
		done ;\
	} ;\
	for object in $(OBJECTS) ;\
	do \
		if entry=`grep "[ 	/]$$object[= 	]" $(PROTO)` ;\
		then \
			set -- $$entry ;\
			path=`eval echo $$3` ;\
			expr $$path : '[^/]' >/dev/null && \
				path=$(BASEDIR)/$$path ;\
			dir=$(ROOT)/$(MACH)`dirname $$path` ;\
			[ ! -d $$dir ] && mkpath $$dir ;\
			$(INS) -f $$dir -m $$4 -u $$5 -g $$6 $$object ;\
		else \
			echo "unable to install $$object" ;\
		fi ;\
	done

clobber		: clean
		rm -f $(FILE)

clean		:
		rm -f $(OBJ)

lintit		:
		for i in $(SRC); \
		do \
		    $(LINT) $(LINTFLAGS) $$i; \
		done

$(FILE)		: $(OBJ)
		$(CC) $(OBJ) -o $@ $(LDFLAGS) $(LDLIBS) $(SHLIBS)

$(OBJ)		: $(HDRS)
