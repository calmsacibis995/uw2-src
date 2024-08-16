#ident	"@(#)debugger:util/common/std.make	1.3"

include ../../util/$(CPU)/$(MACHDEFS)
include ../../util/common/defs.make
include	../../util/common/CC.rules

SOURCES = $(CSOURCES) $(CCSOURCES)

clean:
	-rm -f *.o y.* lex.yy.c

clobber:	clean
	rm -f $(TARGET)

basedepend:
	rm -f BASEDEPEND OBJECT.list
	@if [ "$(CCSOURCES)" ] ;\
		then echo "sh	../../util/common/depend $(CPLUS_CMD_FLAGS) $(CCSOURCES) >> BASEDEPEND" ; \
		CC=$(CC) sh ../../util/common/depend $(CPLUS_CMD_FLAGS) $(CCSOURCES) >> BASEDEPEND ; \
	fi
	@if [ "$(CSOURCES)" ] ;\
		then echo "sh	../../util/common/depend $(CC_CMD_FLAGS) $(CSOURCES) >> BASEDEPEND" ; \
		CC=$(CC) sh ../../util/common/depend $(CC_CMD_FLAGS) $(CSOURCES) >> BASEDEPEND ; \
	fi
	chmod 666 BASEDEPEND

depend:	local_depend
	rm -f DEPEND
	cat BASEDEPEND | \
		sh ../../util/common/substdir $(PRODINC) '$$(PRODINC)' | \
		sh ../../util/common/substdir $(SGSBASE) '$$(SGSBASE)' | \
		sh ../../util/common/substdir $(INCC) '$$(INCC)' | \
		sh ../../util/common/substdir $(INC) '$$(INC)' > DEPEND
	sh ../../util/common/mkdefine OBJECTS < OBJECT.list >> DEPEND
	chmod 444 DEPEND
	rm -f BASEDEPEND OBJECT.list

local_depend:	basedepend

rebuild:	clobber depend all
