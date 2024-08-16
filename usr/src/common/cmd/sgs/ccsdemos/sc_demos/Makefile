#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)ccsdemos:sc_demos/Makefile	1.1"

DEMOS=calendar_sc Args_demo Array_set_demo Graph_demo List_demo Map_demo

all: $(DEMOS)

calendar_sc: calendar_sc.C calendar_sc.h appoint.h
	CC -Tauto -o calendar_sc calendar_sc.C -l++
	@echo Executing calendar_sc
	./calendar_sc -n < appts

Args_demo: Args_demo.C
	CC -o Args_demo Args_demo.C -l++
	@echo Executing Args_demo
	./Args_demo -o blech -O -o -O -DFOO=BAR,BAZ foo.c -dryrun

Array_set_demo: Array_set.C Array_set_demo.C Array_set.h
	CC -Tauto -o Array_set_demo Array_set.C Array_set_demo.C -l++
	@echo Executing Array_set_demo
	./Array_set_demo

Graph_demo: Graph_demo.C
	CC -Tauto -w -o Graph_demo Graph_demo.C -l++
	@echo Executing Graph_demo
	./Graph_demo

List_demo: List_demo.C List_demo.h
	CC -Tauto -o List_demo List_demo.C -l++
	@echo Executing List_demo
	./List_demo

Map_demo: Map_demo.C
	CC -Tauto -o Map_demo Map_demo.C -l++
	@echo Executing Map_demo
	./Map_demo < Map_demo.C

clean:
	rm -f *.o *.ii

clobber: clean
	rm -f $(DEMOS)
