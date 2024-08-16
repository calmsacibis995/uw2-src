#ident	"@(#)S3:S3/s3__vtout__.m	1.1"

/***						-*-Mode: C; -*-
 ***	NAME
 ***
 ***		s3__vtout__.m : VT switch out munchable.
 ***
 ***	SYNOPSIS
 ***		$(munch) -c -i '^s3_.*__vt_switch_out__$$'\
 ***			-f s3__vt_switch_out__ -p s3__vtout__.m\
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
 ***/

%INCLUDEFILELIST% <sidep.h>

%FUNCTIONPROTOTYPE% void

%FUNCTIONARGUMENTS%
