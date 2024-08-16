/***										-*-Mode: C; -*-
 ***	NAME
 ***
 ***		lfb__gs__.m
 ***
 ***	SYNOPSIS
 ***
 ***		$(munch) -c -i '^lfb_.*__gs_change__$$'\
 ***			-f lfb__gs_change__ -p lfb__gs__.m $(non_munch_o_files)
 ***
 ***	DESCRIPTION
 ***
 ***	Munch file to generate graphics state change synchronization
 ***	code. 
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

%FUNCTIONPROTOTYPE% void

%FUNCTIONARGUMENTS%
