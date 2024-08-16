#ident	"@(#)mach64:mach64/m64__vtout__.m	1.1"

/***
 ***	NAME
 ***
 ***		m64__vtout__.m : VT switch out munchable.
 ***
 ***	SYNOPSIS
 ***		$(munch) -c -i '^m64_.*__vt_switch_out__$$'\
 ***			-f m64__vt_switch_out__ -p m64__vtout__.m\
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
