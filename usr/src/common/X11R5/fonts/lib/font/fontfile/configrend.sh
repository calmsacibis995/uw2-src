#!/bin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.


# $Header: /home/x_cvs/mit/fonts/lib/font/fontfile/configrend.sh,v 1.1 1992/08/19 14:16:03 dawes Exp $
#
# This script generates rendererConf.c
#
# usage: configrend.sh driver1 driver2 ...
#

RENDCONF=./rendererConf.c

cat > $RENDCONF <<EOF
/*
 * This file is generated automatically -- DO NOT EDIT
 */

FontFileRegisterFontFileFunctions ()
{
    BitmapRegisterFontFileFunctions ();
EOF
for i in $*; do
  echo "    ${i}RegisterFontFileFunctions ();" >> $RENDCONF
done
echo '}' >> $RENDCONF
