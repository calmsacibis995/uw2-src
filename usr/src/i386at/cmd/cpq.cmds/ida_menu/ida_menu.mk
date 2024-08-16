#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cpq.cmds:ida_menu/ida_menu.mk	1.7"

include $(CMDRULES)

MAKEFILE  = ida_menu.mk
CPQ_INS	  = $(USRBIN)/compaq/diags/ida
LDLIBS	  = -lc -lw
SCSI	  = ../cpqsmu

# CPQ_DEBUG - Enables the debugging output. (OFF)
# CPQ_PHASE4 - Enables the code which will be released for phase 4 of
#	the agreement with Compaq. (OFF)
# CPQ_SCSI - Enables the SCSI specific code. (OFF)
# CPQ_IDA - Enables the IDA specific code. (ON)
# UNIXWARE - It enables the Unixware changes in the
#	Compaq code. (ON)

LOCALDEF  = -DCPQ_IDA -DUNIXWARE
LOCALINC  = -I. -I$(SCSI)

HFILES	  =
CFILES	  = dis_beep.c flush_cache.c ida_info.c ida_init.c id_ctrl.c \
	    id_lstatus.c media_chg.c ret_cache_st.c ret_err_log.c ret_hold.c \
	    ret_mp_cont.c return_stat.c sense_config.c set_delay.c \
	    thresh_stat.c ida_disks.c reset.c set_d_hold.c set_mp_cont.c \
	    unixware.c
OFILES	  = dis_beep.o flush_cache.o ida_info.o ida_init.o id_ctrl.o \
	    id_lstatus.o media_chg.o ret_cache_st.o ret_err_log.o ret_hold.o \
	    ret_mp_cont.o return_stat.o sense_config.o set_delay.o \
	    thresh_stat.o ida_disks.o reset.o set_d_hold.o set_mp_cont.o \
	    unixware.o
TARGET	  = dis_beep flush_cache ida_info ida_init id_ctrl \
	    id_lstatus media_chg ret_cache_st ret_err_log ret_hold \
	    thresh_stat ret_mp_cont return_stat sense_config set_delay ida_disks
INTUSE    = reset set_d_hold set_mp_cont
PROBEFILE = dis_beep.c

all:
	@if [ -f $(PROBEFILE) ]; \
	then \
	    $(MAKE) -f $(MAKEFILE) unixware.c ;\
	    for i in $(TARGET) ;\
	    do \
		find $$i \( ! -type f -o -links +1 \) \
			-exec echo rm -f {} \; -exec rm -f {} \; 2> /dev/null ;\
		$(MAKE) -f $(MAKEFILE) $$i $(MAKEARGS) ;\
	    done \
	else \
	    for i in $(TARGET) ;\
	    do \
		if [ ! -r $$i ]; \
		then \
			echo "ERROR: $$i is missing" 1>&2 ;\
			false ;\
			break ;\
		fi \
	    done \
	fi

clean:
	rm -f $(OFILES)

clobber:
	@if [ -f $(PROBEFILE) ]; then \
		echo "rm -f $(OFILES)" ;\
		rm -f $(OFILES) ;\
		rm -f unixware.c ;\
	fi

install: all
	[ -d $(CPQ_INS) ] || mkdir -p $(CPQ_INS)
	$(INS) -f $(CPQ_INS) -m 0750 -u root -g bin ida_menu
	@for i in $(TARGET);\
	do \
		$(INS) -f $(CPQ_INS) -m 0750 -u root -g bin $$i ;\
	done

# Copy common source file from another directory
unixware.c: $(SCSI)/unixware.c
	rm -f unixware.c
	cp $(SCSI)/unixware.c .

$(INTUSE):$$@.o unixware.o
	$(CC) -o $@ $@.o unixware.o $(LDFLAGS) $(LDLIBS) $(LOCALINC)

$(TARGET):$$@.o unixware.o
	$(CC) -o $@ $@.o unixware.o $(LDFLAGS) $(LDLIBS) $(LOCALINC)

ida_disks.o:$(SCSI)/unixware.h
dis_beep.o:$(SCSI)/unixware.h
flush_cache.o:$(SCSI)/unixware.h
id_ctrl.o:$(SCSI)/unixware.h
id_lstatus.o:$(SCSI)/unixware.h
ida_disks.o:$(SCSI)/unixware.h
ida_info.o:$(SCSI)/unixware.h
ida_init.o:$(SCSI)/unixware.h
media_chg.o:$(SCSI)/unixware.h
ret_cache_st.o:$(SCSI)/unixware.h
ret_err_log.o:$(SCSI)/unixware.h
ret_hold.o:$(SCSI)/unixware.h
ret_mp_cont.o:$(SCSI)/unixware.h
return_stat.o:$(SCSI)/unixware.h
sense_config.o:$(SCSI)/unixware.h
set_delay.o:$(SCSI)/unixware.h
thresh_stat.o:$(SCSI)/unixware.h
reset.o:$(SCSI)/unixware.h
set_d_hold.o:$(SCSI)/unixware.h
set_mp_cont.o:$(SCSI)/unixware.h
