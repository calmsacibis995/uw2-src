#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)proto-cmd:i386at/cmd/proto-cmd/proto-cmd.mk	1.3.4.7"
# Makefile for Packaging and Installation Commands

include $(CMDRULES)

LOCALDEF = -DAT386

MSGS = bootcntl.str

ALL = \
	adminobj \
	get_tz_offset \
	install_more \
	loadhba \
	odm \
	pdiconfig \
	rebuild \
	instlist \
	machine_type \
	machine_type.dy \
	links \
	setmods \
	setmods.elf \
	mkflist \
	contents \
	x286 \
	bootcntl \
	check_uart \
	choose_lang

DIRS = \
	$(USRBIN) \
	$(SBIN) \
	$(VAR)/sadm/pkg/dfm \
	$(VAR)/tmp \
	$(ETC)/inst/scripts \
	$(USRLIB)/locale/C/MSGFILES

all: $(ALL) pkginfo default

install: all $(DIRS) $(MSGS)
	@$(INS) -f $(ETC)/inst/scripts adminobj
	@$(INS) -f $(ETC)/inst/scripts get_tz_offset
	@$(INS) -f $(ETC)/inst/scripts install_more
	@$(INS) -f $(ETC)/inst/scripts loadhba
	@$(INS) -f $(ETC)/inst/scripts odm
	@$(INS) -f $(ETC)/inst/scripts pdiconfig
	@$(INS) -f $(ETC)/inst/scripts rebuild
	@$(INS) -f $(SBIN) instlist
	@$(INS) -f $(USRBIN) machine_type
	@$(INS) -f $(USRBIN) machine_type.dy
	@$(INS) -f $(USRBIN) bootcntl
	@$(INS) -f $(USRBIN) x286
	@$(INS) -f $(USRBIN) choose_lang
	@$(INS) -f $(USRLIB)/locale/C/MSGFILES bootcntl.str
	@$(INS) -f $(ETC) links
	@$(INS) -f $(ETC) setmods
	@$(INS) -f $(ETC) setmods.elf
	@$(INS) -f $(ETC) mkflist
	@$(INS) -f $(ETC) contents
	@$(INS) -f $(ETC) check_uart
	@$(INS) -f $(VAR)/sadm/pkg/dfm pkginfo
	@$(INS) -f $(VAR) default

$(DIRS):
	-mkdir -p $@

setmods contents mkflist: $$@.c
	$(HCC) $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS)

check_uart machine_type: $$@.c
	$(CC) $(CFLAGS) -I$(INC) -o $@ $@.c $(NOSHLIBS) $(LDFLAGS)

machine_type.dy: $(@:.dy=.c)
	$(CC) $(CFLAGS) -I$(INC) -o $@ $(@:.dy=.c) $(LDFLAGS)

setmods.elf: $(@:.elf=.c)
	$(CC) $(CFLAGS) -I$(INC) -o $@ $(@:.elf=.c) $(LDFLAGS)

links x286 bootcntl instlist: $$@.c
	$(CC) $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS)

choose_lang: $$@.c
	$(CC) $(CFLAGS) -I$(INC) -o $@ $@.c $(LDFLAGS) -lcurses


adminobj get_tz_offset install_more loadhba pdiconfig rebuild: $$@.sh

clean:
	$(RM) -f *.o

clobber: clean
	$(RM) -f $(ALL)
