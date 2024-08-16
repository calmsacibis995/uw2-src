/***							-*-Mode: C; -*-
 ***	NAME
 ***
 ***		m__init__.h : initialization munchable for the 
 ***				MACH display library.
 ***
 ***	SYNOPSIS
 ***
 ***		$(munch) -c -i '^mach_.*__initialize__$$'\
 ***			-f mach__initialize__ -p m__init__.m\
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
 ***/

%INCLUDEFILELIST% <sidep.h>,"m_opt.h"

%FUNCTIONARGUMENTS% si_screen_p, options_p

%FUNCTIONPROTOTYPE% SIScreenRec *si_screen_p, struct mach_options_structure *options_p
