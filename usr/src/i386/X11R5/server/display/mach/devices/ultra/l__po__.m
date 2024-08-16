/***							-*-Mode: C; -*-
 ***	NAME
 ***
 ***		l__po__.m : initialization munchable for the 
 ***				LFB display library.
 ***
 ***	SYNOPSIS
 ***
 ***		$(munch) -c -i '^lfb_.*__post_initialize__$$'\
 ***			-f lfb__post_initialize__ -p l__po__.m\
 ***			$(non_munch_o_files)
 ***
 ***	DESCRIPTION
 ***
 ***	This munchable is used to generate the post-chipset
 ***	initialization code.
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

