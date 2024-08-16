/***										-*-Mode: C; -*-
 ***	NAME
 ***
 ***		l__pi__.h : initialization munchable for the 
 ***				LFB display library.
 ***
 ***	SYNOPSIS
 ***
 ***		$(munch) -c -i '^lfb_.*__pre_initialize__$$'\
 ***			-f lfb__pre_initialize__ -p l__pi__.m\
 ***			$(non_munch_o_files)
 ***
 ***	DESCRIPTION
 ***
 ***	This munchable causes the generation of the pre-chipset
 ***	initialization module synchronization code.
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

%INCLUDEFILELIST% <sidep.h>,"l_opt.h"

%FUNCTIONARGUMENTS% si_screen_p, options_p

%FUNCTIONPROTOTYPE% SIScreenRec *si_screen_p, struct lfb_options_structure *options_p
