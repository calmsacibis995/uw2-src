#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#******************************************************
# *** IMPORTANT ***
# This file is generated using a build tool... DO NOT
# MODIFY THIS FILE!!!  If this file gets out of sync
# then use the build tools to update it.  THIS FILE
# SHOULD BE UPDATED BY BUILD TOOLS ONLY!
#******************************************************

#----------------------------------------------------
# Make sure BUILD_TOOLS has been set, otherwise we
# won't know how to include the master makefile
#----------------------------------------------------
%if !%defined(BUILD_TOOLS)
all : ERR_NO_BUILD_TOOLS
%else

#----------------------------------------------------
# If the build target hasn't been specified then set
# it to the current directory.
#----------------------------------------------------
%if !%defined(TARG)
TARG = PROJ!IMPORTS!I_NVT!WIN
%endif

#----------------------------------------------------
# Now include the makefile in the BUILD_TOOLS directory.
# This will include all of the magical files that makes
# the build process work.
#----------------------------------------------------
.include $(BUILD_TOOLS)\template\navigate.tpl
%endif

#----------------------------------------------------
# BUILD_TOOLS hasn't been set, tell the user about
# the problem.
#----------------------------------------------------
ERR_NO_BUILD_TOOLS :
   @echo *** ERROR ***
   @echo Can't find build tools.  To correct this, you
   @echo must set the BUILD_TOOLS environment variable
   @echo to point to the directory that contains the
   @echo build tools.
