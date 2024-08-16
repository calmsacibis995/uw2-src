#ident	"@(#)p9k:p9k/p9k__hwi__.m	1.1"
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

%INCLUDEFILELIST% <sidep.h>,"p9k_opt.h"

%FUNCTIONARGUMENTS% si_screen_p, options_p

%FUNCTIONPROTOTYPE% SIScreenRec *si_screen_p, struct p9000_options_structure *options_p
