#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1993 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

d_voidsig='undef'
d_tosignal='int'
gidtype='int'
groupstype='int'
uidtype='int'
# Note that 'Configure' is run from 'UU', hence the strange 'ln'
# command.
for i in .. ../x2p
do
      rm -f $i/3b1cc
      ln ../hints/3b1cc $i
done
echo "\nIf you want to use the 3b1 shared libraries, complete this script then"
echo "read the header in 3b1cc.           [Type carriage return to continue]\c"
read vch
