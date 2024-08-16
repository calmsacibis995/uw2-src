#ident	"@(#)p9k:p9k/p9k__vtout__.m	1.1"
/***							-*-Mode: C; -*-
 ***	NAME
 ***
 ***		debug.m : munchable for generation of the debug files.
 ***
 ***	SYNOPSIS
 ***		
 ***		$(munch) -d -i '^.*_debug$' -f mach_debug_control \
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

PUBLIC

/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

/*
 *	Forward declarations.
 */


/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))

#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

/***
 ***	Includes.
 ***/

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

%INCLUDEFILELIST% <sidep.h>

%FUNCTIONPROTOTYPE% void

%FUNCTIONARGUMENTS%
