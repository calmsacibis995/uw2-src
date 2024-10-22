#! /usr/bin/csh -f
#ident	"@(#)ucb:common/ucbcmd/which/which.csh	1.4"
#ident	"$Header: $"
#		Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#		Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#		  All Rights Reserved

#		THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#		The copyright notice above does not evidence any
#		actual or intended publication of such source code.

#
#	Copyright (c) 1982, 1986, 1988
#	The Regents of the University of California
#	All Rights Reserved.
#	Portions of this document are derived from
#	software developed by the University of
#	California, Berkeley, and its contributors.
#
#
#
#       which : tells you which program you get
#
# Set prompt so .cshrc will think we're interactive and set aliases.
# Save and restore path to prevent .cshrc from messing it up.
set _which_saved_path_ = ( $path )
set prompt = ""
if ( -r ~/.cshrc && -f ~/.cshrc ) source ~/.cshrc
set path = ( $_which_saved_path_ )
unset prompt _which_saved_path_
set noglob
foreach arg ( $argv )
    set alius = `alias $arg`
    switch ( $#alius )
        case 0 :
            breaksw
        case 1 :
            set arg = $alius[1]
            breaksw
        default :
            echo ${arg}: "      " aliased to $alius
            continue
    endsw
    unset found
    if ( $arg:h != $arg:t ) then
        if ( -e $arg ) then
            echo $arg
        else
            echo $arg not found
        endif
        continue
    else
        foreach i ( $path )
            if ( -x $i/$arg && ! -d $i/$arg ) then
                echo $i/$arg
                set found
                break
            endif
        end
    endif
    if ( ! $?found ) then
        echo no $arg in $path
    endif
end

