#ident	"@(#)de_le:common/le/de/build/ebt.mk	1.8"
#	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


include $(CMDRULES)

LOCALE	= de

EBTSRC = ../runtime/usr/doc/data/config/$(LOCALE)

EBTFILES = dhelp/ftwin.rc dtext/awin1.rc dtext/awin2.rc dtext/awin3.rc \
	dtext/awin4.rc dtext/awin5.rc dtext/bwin.rc dtext/bwin1.rc \
	dtext/bwin2.rc dtext/bwin3.rc dtext/bwin4.rc dtext/bwin5.rc \
	dtext/bwin6.rc dtext/edit.rc dtext/ftwin.rc dtext/ftwin1.rc \
	dtext/hwin.rc dtext/hwin1.rc dtext/lwin1.rc dtext/lwin2.rc \
	dtext/rwin.rc errors.txt msgs.txt props.txt query.txt \
	values.txt README

EBTHSRC = ../runtime/usr/doc/data/help/$(LOCALE)

EBTHFILES = booklist.txt books/dhelp/ebt/dhelp.dat books/dhelp/ebt/dhelp.edr \
	books/dhelp/ebt/dhelp.tag books/dhelp/ebt/figures.tdr \
	books/dhelp/ebt/tables.tdr books/dhelp/ebt/toc.tdr \
	books/dhelp/index/index.dat books/dhelp/index/index.log \
	books/dhelp/index/vocab.dat books/dhelp/styles books/dtext/dtext.qrs \
	books/dtext/ebt/dtext.dat books/dtext/ebt/dtext.edr \
	books/dtext/ebt/dtext.tag books/dtext/ebt/figures.tdr \
	books/dtext/ebt/tables.tdr books/dtext/ebt/toc.tdr \
	books/dtext/figures/advanced.tif books/dtext/figures/aftercol.tif \
	books/dtext/figures/afterexp.tif books/dtext/figures/alttocvu.tif \
	books/dtext/figures/annotmgr.tif books/dtext/figures/basic.tif \
	books/dtext/figures/be4col.tif books/dtext/figures/be4exp.tif \
	books/dtext/figures/bwin.tif books/dtext/figures/filter.tif \
	books/dtext/figures/fulltext.tif books/dtext/figures/graphwin.tif \
	books/dtext/figures/helplist.tif books/dtext/figures/hottext.tif \
	books/dtext/figures/j_entry1.tif books/dtext/figures/j_entry2.tif \
	books/dtext/figures/j_entry3.tif books/dtext/figures/j_entry4.tif \
	books/dtext/figures/journal2.tif books/dtext/figures/journal.tif \
	books/dtext/figures/jrnlentr.tif books/dtext/figures/jsave.tif \
	books/dtext/figures/libwin.tif books/dtext/figures/linkicon.tif \
	books/dtext/figures/linkmark.tif books/dtext/figures/lns_hit.tif \
	books/dtext/figures/srcharea.tif books/dtext/figures/srchdiag.tif \
	books/dtext/figures/srchpanl.tif books/dtext/figures/srchpnl2.tif \
	books/dtext/figures/toc2.tif books/dtext/figures/toc.tif \
	books/dtext/index/index.dat books/dtext/index/index.log \
	books/dtext/index/vocab.dat books/dtext/styles \
	books/readme/ebt/figures.tdr books/readme/ebt/readme.dat \
	books/readme/ebt/readme.edr books/readme/ebt/readme.tag \
	books/readme/ebt/tables.tdr books/readme/ebt/toc.tdr \
	books/readme/index/index.dat books/readme/index/index.log \
	books/readme/index/vocab.dat books/readme/readme.qrs \
	books/readme/styles ents/map.txt styles/aspects/figtext.w \
	styles/aspects/figures.tv styles/aspects/foot.w \
	styles/aspects/fulltext.pv styles/aspects/fulltext.v \
	styles/aspects/icononly.v styles/aspects/showbrks.pv \
	styles/aspects/tables.tv styles/aspects/table.w \
	styles/aspects/textonly.pv styles/aspects/titles_1.pv \
	styles/aspects/titles_2.pv styles/aspects/toc.tv \
	styles/caspects/figtext.w styles/caspects/figures.tv \
	styles/caspects/foot.w styles/caspects/fulltext.pv \
	styles/caspects/fulltext.v styles/caspects/icononly.v \
	styles/caspects/showbrks.pv styles/caspects/tables.tv \
	styles/caspects/table.w styles/caspects/textonly.pv \
	styles/caspects/titles_1.pv styles/caspects/titles_2.pv \
	styles/caspects/toc.tv styles/cebtstyles/arbor.rev \
	styles/cebtstyles/big.v styles/cebtstyles/figures.tv \
	styles/cebtstyles/fulltext.pv styles/cebtstyles/fulltext.v \
	styles/cebtstyles/help.rev styles/cebtstyles/inline.v \
	styles/cebtstyles/popup.rev styles/cebtstyles/tables.tv \
	styles/cebtstyles/texeqn.pri styles/cebtstyles/texeqn.rev \
	styles/cebtstyles/toc.tv styles/ebtstyles/arbor.rev \
	styles/ebtstyles/big.v styles/ebtstyles/figures.tv \
	styles/ebtstyles/fulltext.pv styles/ebtstyles/fulltext.v \
	styles/ebtstyles/help.rev styles/ebtstyles/inline.v \
	styles/ebtstyles/popup.rev styles/ebtstyles/tables.tv \
	styles/ebtstyles/texeqn.pri styles/ebtstyles/texeqn.rev \
	styles/ebtstyles/toc.tv styles/modules/base.mod \
	styles/modules/figtext.rt styles/modules/figtoc.rt \
	styles/modules/foot.rt styles/modules/icononly.dff \
	styles/modules/idx.mod styles/modules/links.mod \
	styles/modules/localize.ent styles/modules/nums.ent \
	styles/modules/online.dff styles/modules/onlinfig.mod \
	styles/modules/printfig.mod styles/modules/print.rt \
	styles/modules/showbrks.dff styles/modules/table.rt \
	styles/modules/tbltoc.rt styles/modules/textonly.dff \
	styles/modules/text.rt styles/modules/titles_2.dff \
	styles/modules/titles.rt styles/modules/tocchaps.mod \
	styles/modules/toc.rt styles/modules/txtchaps.mod \
	styles/modules/withfigs.dff styles/params/unix/colors.p \
	styles/params/unix/dings.p styles/params/unix/fulltext.p \
	styles/params/unix/print.p styles/params/unix/toc.p \
	ebt_ents/ati-eqn1.gml ebt_ents/ati-graph.gml ebt_ents/ati-math.elm \
	ebt_ents/ati-num.gml ebt_ents/ati-tbl.elm ebt_ents/iso-amsa.gml \
	ebt_ents/iso-amsb.gml ebt_ents/iso-amsc.gml ebt_ents/iso-amsn.gml \
	ebt_ents/iso-amso.gml ebt_ents/iso-amsr.gml ebt_ents/iso-box.gml \
	ebt_ents/iso-cyr1.gml ebt_ents/iso-cyr2.gml ebt_ents/iso-dia.gml \
	ebt_ents/iso-grk1.gml ebt_ents/iso-grk2.gml ebt_ents/iso-grk3.gml \
	ebt_ents/iso-grk4.gml ebt_ents/iso-lat1.gml ebt_ents/iso-lat2.gml \
	ebt_ents/iso-num.gml ebt_ents/iso-pub.gml ebt_ents/iso-tech.gml \
	ebt_ents/localize.ent libidx/books.lst libidx/elems/index.log \
	libidx/freqs/index.dat libidx/freqs/index.log libidx/freqs/vocab.dat

EBTMSG = dhelp.msg

EBTDIR = $(ROOT)/$(MACH)/usr/doc/data/config/$(LOCALE)

EBTHDIR = $(ROOT)/$(MACH)/usr/doc/data/help/$(LOCALE)

all: $(EBTMSG)

$(EBTMSG):
	gencat $@ $(EBTSRC)/dhelp/$@ 2>/dev/null


install: all
	[ -d $(EBTDIR)/dhelp ] || mkdir -p $(EBTDIR)/dhelp
	[ -d $(EBTDIR)/dtext ] || mkdir -p $(EBTDIR)/dtext
	[ -d $(EBTHDIR)/books/dhelp/ebt ] || mkdir -p $(EBTHDIR)/books/dhelp/ebt
	[ -d $(EBTHDIR)/books/dhelp/index ] || \
		mkdir -p $(EBTHDIR)/books/dhelp/index
	[ -d $(EBTHDIR)/books/dtext/ebt ] || mkdir -p $(EBTHDIR)/books/dtext/ebt
	[ -d $(EBTHDIR)/books/dtext/figures ] || \
		mkdir -p $(EBTHDIR)/books/dtext/figures
	[ -d $(EBTHDIR)/books/dtext/index ] || \
		mkdir -p $(EBTHDIR)/books/dtext/index
	[ -d $(EBTHDIR)/books/readme/ebt ] || \
		mkdir -p $(EBTHDIR)/books/readme/ebt
	[ -d $(EBTHDIR)/books/readme/index ] || \
		mkdir -p $(EBTHDIR)/books/readme/index
	[ -d $(EBTHDIR)/ents ] || mkdir -p $(EBTHDIR)/ents
	[ -d $(EBTHDIR)/styles/aspects ] || mkdir -p $(EBTHDIR)/styles/aspects
	[ -d $(EBTHDIR)/styles/caspects ] || mkdir -p $(EBTHDIR)/styles/caspects
	[ -d $(EBTHDIR)/styles/cebtstyles ] || \
		mkdir -p $(EBTHDIR)/styles/cebtstyles
	[ -d $(EBTHDIR)/styles/ebtstyles ] || \
		mkdir -p $(EBTHDIR)/styles/ebtstyles
	[ -d $(EBTHDIR)/styles/modules ] || mkdir -p $(EBTHDIR)/styles/modules
	[ -d $(EBTHDIR)/styles/params/unix ] || \
		mkdir -p $(EBTHDIR)/styles/params/unix
	[ -d $(EBTHDIR)/ebt_ents ] || mkdir -p $(EBTHDIR)/ebt_ents
	[ -d $(EBTHDIR)/libidx ] || mkdir -p $(EBTHDIR)/libidx
	[ -d $(EBTHDIR)/libidx/elems ] || mkdir -p $(EBTHDIR)/libidx/elems
	[ -d $(EBTHDIR)/libidx/freqs ] || mkdir -p $(EBTHDIR)/libidx/freqs
	for i in $(EBTFILES) ;\
	do \
		j=`dirname $$i` ;\
		$(INS) -f $(EBTDIR)/$$j $(EBTSRC)/$$i ;\
	done
	for i in $(EBTHFILES) ;\
	do \
		j=`dirname $$i` ;\
		$(INS) -f $(EBTHDIR)/$$j $(EBTHSRC)/$$i ;\
	done
	$(INS) -f $(EBTDIR)/dhelp dhelp.msg
	$(INS) -f $(EBTDIR)/dhelp dhelp.msg.m

clean:
	rm -f dhelp.msg*

clobber: clean

