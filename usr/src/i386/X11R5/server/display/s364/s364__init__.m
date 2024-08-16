#pragma ident	"@(#)s364:s364/s364__init__.m	1.2"
/***							-*-Mode: C; -*-
 ***	NAME
 ***
 ***		s364__init__.h : initialization munchable for the 
 ***				S3 display library.
 ***
 ***	SYNOPSIS
 ***
 ***		$(munch) -c -i '^s364_.*__initialize__$$'\
 ***			-f s364__initialize__ -p s364__init__.m\
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

%INCLUDEFILELIST% <sidep.h>,"s364_opt.h"

%FUNCTIONARGUMENTS% si_screen_p, options_p

%FUNCTIONPROTOTYPE% SIScreenRec *si_screen_p, struct s364_options_structure *options_p
