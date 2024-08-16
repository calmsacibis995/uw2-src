#ident	"@(#)S3:S3/s3__init__.m	1.1"

/***							-*-Mode: C; -*-
 ***	NAME
 ***
 ***		s3__init__.h : initialization munchable for the 
 ***				S3 display library.
 ***
 ***	SYNOPSIS
 ***
 ***		$(munch) -c -i '^s3_.*__initialize__$$'\
 ***			-f s3__initialize__ -p s3__init__.m\
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

%INCLUDEFILELIST% <sidep.h>,"s3_options.h"

%FUNCTIONARGUMENTS% si_screen_p, options_p

%FUNCTIONPROTOTYPE% SIScreenRec *si_screen_p, struct s3_options_structure *options_p
