#ident	"@(#)mach64:mach64/m64__init__.m	1.1"

/***							-*-Mode: C; -*-
 ***	NAME
 ***
 ***		m64__init__.h : initialization munchable for the 
 ***				S3 display library.
 ***
 ***	SYNOPSIS
 ***
 ***		$(munch) -c -i '^m64_.*__initialize__$$'\
 ***			-f m64__initialize__ -p m64__init__.m\
 ***			$(non_munch_o_files)
 ***
 ***	DESCRIPTION
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***	SEE ALSO
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	HISTORY
 ***
 ***
 ***/

%INCLUDEFILELIST% <sidep.h>,"m64_opt.h"

%FUNCTIONARGUMENTS% si_screen_p, options_p

%FUNCTIONPROTOTYPE% SIScreenRec *si_screen_p, struct m64_options_structure *options_p
