#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:libol/i386/makefile	1.12"

include $(CMDRULES)

TARGET = $(PRODLIB)/libol.a

include ../../util/common/std.make

XINC = $(UIBASE)/include
XDEFS = -DUSL=1 -DSVR4=1 -DI18N
DFLAGS = $(XDEFS) -I../../gui.d/common -I../../gui.d/$(CPU) -I../common \
	-I$(XINC)

CCSOURCES = \
	$(COMMON)/Alert_sh.C \
	$(COMMON)/Boxes.C \
	$(COMMON)/Button_bar.C \
	$(COMMON)/Caption.C \
	$(COMMON)/Dialog_sh.C \
	$(COMMON)/Menu.C \
	$(COMMON)/Radio.C \
	$(COMMON)/Resources.C \
	$(COMMON)/Sel_list.C \
	$(COMMON)/Slider.C \
	$(COMMON)/Stext.C \
	$(COMMON)/Table.C \
	$(COMMON)/Text_area.C \
	$(COMMON)/Text_disp.C \
	$(COMMON)/Text_line.C \
	$(COMMON)/Timer.C \
	$(COMMON)/Toggle.C \
	$(COMMON)/Window_sh.C \
	$(COMMON)/olutil.C

CSOURCES=\
	$(COMMON)/xpm.c \
	$(COMMON)/DbgTextEditM.c \
	$(COMMON)/DbgTextEditO.c \
	$(COMMON)/DbgTextEdit.c

include DEPEND

include ../../util/common/lib.make

local_depend:
	cp BASEDEPEND /tmp
	sh ../../util/common/substdir $(XINC) '$$(XINC)' </tmp/BASEDEPEND | \
	sh ../../util/common/substdir /usr/include '$$(INC)' >BASEDEPEND
	rm  -f /tmp/BASEDEPEND
