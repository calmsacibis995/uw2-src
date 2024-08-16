#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:gui.d/i386/makefile	1.19"

include $(CMDRULES)

BASENAME = debug.$(UI).ui

TARGET = ../../$(BASENAME)

include ../../util/common/std.make

XINC = $(UIBASE)/include
XDEFS = -DUSL=1 -DSVR4=1
DFLAGS = $(XDEFS) -I../../lib$(UI)/common -I$(XINC)

# following does not use UIBASE because it's used by LD_RUN_PATH, 
# and as such needs to be the path in a native environment
XLIBPATH = /usr/X/lib

CCSOURCES = \
	$(COMMON)/Base_win.C \
	$(COMMON)/Command.C \
	$(COMMON)/Ctrl_dlg.C \
	$(COMMON)/Dialogs.C \
	$(COMMON)/Dis.C \
	$(COMMON)/Dispatcher.C \
	$(COMMON)/Event_dlg.C \
	$(COMMON)/Eventlist.C \
	$(COMMON)/Events.C \
	$(COMMON)/Expand_dlg.C \
	$(COMMON)/FileInfo.C \
	$(COMMON)/Help.C \
	$(COMMON)/Label.C \
	$(COMMON)/Mem_dlg.C \
	$(COMMON)/Notifier.C \
	$(COMMON)/Panes.C \
	$(COMMON)/Proclist.C \
	$(COMMON)/Ps_pane.C \
	$(COMMON)/Queries.C \
	$(COMMON)/Regs_pane.C \
	$(COMMON)/Sch_dlg.C \
	$(COMMON)/Source.C \
	$(COMMON)/Stack_pane.C \
	$(COMMON)/Status.C \
	$(COMMON)/Syms_pane.C \
	$(COMMON)/Transcript.C \
	$(COMMON)/Windows.C \
	$(COMMON)/config.C \
	$(COMMON)/main.C \
	$(COMMON)/userConfig.C \
	$(COMMON)/util.C \
	CoreInfo.C \
	ps.C \
	GLabel.C

CSOURCES = GMtable.c

include DEPEND

$(TARGET):	$(PRODLIB)/libint.a
$(TARGET):	$(PRODLIB)/libdbgen.a
$(TARGET):	../../lib/lib$(UI).a

include ../../lib$(UI)/$(CPU)/ui.libs

all:	$(TARGET)

$(TARGET):	$(OBJECTS)
	rm -f $(TARGET)
	LD_RUN_PATH=$(XLIBPATH) $(CPLUS) -o $(TARGET) $(LINK_MODE) $(OBJECTS) $(LIBRARIES) $(LDLIBS)

install:	$(CCSLIB)/$(BASENAME) $(UI)install

$(CCSLIB)/$(BASENAME):	$(TARGET)
	$(STRIP) $(TARGET)
	cp $(TARGET) $(CCSLIB)/$(BASENAME)

local_depend:
	cp BASEDEPEND /tmp
	sed -e 's/libol/lib$$(UI)/g' </tmp/BASEDEPEND | \
	sh ../../util/common/substdir $(XINC) '$$(XINC)' | \
	sh ../../util/common/substdir /usr/include '$$(INC)' >BASEDEPEND
	rm  -f /tmp/BASEDEPEND

