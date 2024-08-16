#pragma ident	"@(#)s364:s364/g__init__.m	1.2"
/***							
 ***	NAME
 ***
 ***		g__init__.m
 ***
 ***	SYNOPSIS
 ***		$(munch) -c -i '^generic_.*__initialize__$$'\
 ***	 		-f generic__initialize__ -p g__init__.m\
 ***			 $(non_munch_o_files) > $(@)
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

%INCLUDEFILELIST% <sidep.h>

%FUNCTIONPROTOTYPE% SIScreenRec *si_screen_p

%FUNCTIONARGUMENTS% si_screen_p

