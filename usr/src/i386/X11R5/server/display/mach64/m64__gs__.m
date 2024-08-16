#ident	"@(#)mach64:mach64/m64__gs__.m	1.1"

/***						-*-Mode: C; -*-
 ***	NAME
 ***
 ***		m64__gs__.m
 ***
 ***	SYNOPSIS
 ***
 ***		$(munch) -c -i '^m64_.*__gs_change__$$'\
 ***			-f m64__gs_change__ -p m64__gs__.m $(non_munch_o_files)
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

%INCLUDEFILELIST% <sidep.h>

%FUNCTIONPROTOTYPE% void

%FUNCTIONARGUMENTS%
