#!/sbin/sh
#	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
#	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
#	  All Rights Reserved

#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
#	The copyright notice above does not evidence any
#	actual or intended publication of such source code.

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/nwdiscover/test.sh	1.1"

echo "\n\n\n\n******testing the usage line with invalid argument******\n"
nwdiscover -p

echo "\n\n\n\n******testing the usage line for invalid -r 0 ******\n"
nwdiscover -r 0

echo "\n\n\n\n******testing the usage line for valid -r 4 ******\n"
nwdiscover -r 4

echo "\n\n\n\n******testing the usage line for invalid -t 0 ******\n"
nwdiscover -t 0

echo "\n\n\n\n******testing the usage line for valid -t 10 ******\n"
nwdiscover -t 10

echo "\n\n\n\n******testing the usage line for invalid device path******\n"
nwdiscover -d tmp

echo "\n\n\n\n******testing the usage line for invalid frame exclusion******\n"
nwdiscover -e tmp

echo "\n\n\n\n******testing the usage line for invalid frame type******\n"
nwdiscover -f tmp

echo "\n\n\n\n******expecting the search to stop at the first found entry******\n"
nwdiscover

echo "\n\n\n\n******expecting the search to stop at the first found entry******\n"
nwdiscover -v

echo "\n\n\n\n******expecting to search for all devices and all frame types******\n"
nwdiscover -a

echo "\n\n\n\n******expecting to search for all devices and all frame types******\n"
nwdiscover -a -v

echo "\n\n\n\n******expecting to search for all devices and all frame types, with a 1 retry******\n"
nwdiscover -a -v -r 1

echo "\n\n\n\n******expecting to search all devices for the frame ethernet_ii******\n"
nwdiscover -a -f ethernet_ii

echo "\n\n\n\n******expecting to search all devices for the frame ethernet_ii******\n"
nwdiscover -f ethernet_ii -a

echo "\n\n\n\n******expecting to search all frame types with the device /dev/el16_0******\n"
nwdiscover -a -d /dev/el16_0

echo "\n\n\n\n******expecting to search all frame types with the device /dev/el16_0******\n"
nwdiscover -d /dev/el16_0 -a

echo "\n\n\n\n******expecting to only search for 802.2 frame type******\n"
nwdiscover -a -e ethernet_ii -e ethernet_snap -e ethernet_802.3 -v

echo "\n\n\n\n******expecting to only search for 802.3 frame type******\n"
nwdiscover -a -e ethernet_ii -e ethernet_snap -e ethernet_802.2 -v

echo "\n\n\n\n******expecting to only search for SNAP frame type******\n"
nwdiscover -a -e ethernet_ii -e ethernet_802.3 -e ethernet_802.2 -v

echo "\n\n\n\n******expecting to only search for Ethernet_II frame type******\n"
nwdiscover -a -e ethernet_snap -e ethernet_802.3 -e ethernet_802.2 -v

echo "\n\n\n\n******expecting that the default network will be created******\n"
nwdiscover -e  ethernet_802.2 -a -r 1 -e ethernet_ii -e ethernet_snap -e ethernet_802.3 -v



