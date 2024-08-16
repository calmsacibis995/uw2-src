#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)debugger:libexecon/i386/makefile	1.4"

include $(CMDRULES)

TARGET = $(PRODLIB)/libexecon.a

include ../../util/common/std.make

CCSOURCES = \
	Proccore.C \
	Frame.C \
	Reg1.C \
	RegAccess.C \
	Seg_mach.C \
	ProcFollow.C \
	HW_Watch.C \
	newproc.C \
	oldproc.C \
	$(COMMON)/Breaklist.C \
	$(COMMON)/EventTable.C \
	$(COMMON)/Event.C \
	$(COMMON)/Ev_Notify.C \
	$(COMMON)/PrObj.chk.C \
	$(COMMON)/PrObj.ctl.C \
	$(COMMON)/PrObj.event.C \
	$(COMMON)/PrObj.io.C \
	$(COMMON)/PrObj.new.C \
	$(COMMON)/PrObj.resp.C \
	$(COMMON)/PrObj.sym.C \
	$(COMMON)/Proc.ctl.C \
	$(COMMON)/Proc.event.C \
	$(COMMON)/Proc.io.C \
	$(COMMON)/Proc.new.C \
	$(COMMON)/Proc.thr.C \
	$(COMMON)/Procctl.C \
	$(COMMON)/Proglist.C \
	$(COMMON)/Program.C \
	$(COMMON)/PtyList.C \
	$(COMMON)/Reg.C \
	$(COMMON)/Seglist.C \
	$(COMMON)/Segment.C \
	$(COMMON)/Siglist.C \
	$(COMMON)/StopEvent.C \
	$(COMMON)/TSClist.C \
	$(COMMON)/Thr.ctl.C \
	$(COMMON)/Thr.event.C \
	$(COMMON)/Thr.io.C \
	$(COMMON)/Thr.new.C

CSOURCES=\
	Coffcore.c

include DEPEND

include ../../util/common/lib.make
