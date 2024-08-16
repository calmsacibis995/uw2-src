#ident	"@(#)mach64:mach64/m64__vtin__.m	1.1"

/***							-*-Mode: C; -*-
 ***	NAME
 ***
 ***		m64__vtin__.m : Vt switch in munchable.
 ***
 ***	SYNOPSIS
 ***
 ***		$(munch) -c -i '^m64_.*__vt_switch_in__$$'\
 ***			-f m64__vt_switch_in__	-p m64__vtin__.m\
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

%INCLUDEFILELIST% <sidep.h>

%FUNCTIONPROTOTYPE% void

%FUNCTIONARGUMENTS%
