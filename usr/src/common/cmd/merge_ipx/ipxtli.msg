DOS Client Message File: IPXODI.COM 0, Tran 0
 D�@      ,�(� ��(��(!       �,va�(? �      "   �   �   �     �   H  j  �  �    [  �  P  }  �  �  "  �    �  �  �  w  z  3	  �	  *
  `
  �
  [  �  >  �  N  �  <<< BAD MESSAGE >>> Message file %s is invalid.
Program load aborted.
 CoPyRiGhT=(C) Copyright 1990-94 Novell,Inc.  All Rights Reserved.
 IPX protocol has been successfully removed.
 Available command line options:
 /?     Display this help screen.
 /D     Eliminate Diagnostic Responder - Reduces size by 3K.
 /A     Eliminate Diagnostic Responder and SPX - Reduces size by 9K.

 /C=[path\]filename.ext
       Specify a configuration file to use.

 /U     Unload resident IPXTLI from memory.
 /F     Forcibly unload resident IPXTLI from memory, regardless of programs 
       loaded above it.  Using this option can cause a machine to crash if
       applications are still using IPX/SPX.
 The IPX protocol has already been loaded.
 The LSL is not loaded.  Please load the LSL then IPXTLI.
 The loaded LSL is too old.  LSL.COM must be v1.20 or higher.
 Cannot find a IPX protocol to unload.
 Cannot unload IPXTLI because another program has been loaded
above it.  Unload the other program or programs and try again.
 IPXTLI cannot be unloaded because the installed IPXTLI is a 
different version than this IPXTLI module.
 IPXTLI cannot be unloaded because interrupt 0x%X is owned by 
another program.  Unload the other program then unload IPXTLI.
 A network adapter driver must be loaded before IPXTLI.COM can
be loaded.
 IPX could not find a board to bind to.  Please specify a board
in the NET.CFG file by adding a 'BIND' entry to the 'PROTOCOL IPX' section
and specify the protocol ID IPX should use for that board by adding a
'PROTOCOL IPX id frametype' entry to the board's 'LINK DRIVER' section.
 IPX cannot bind to board '%u' because a IPX protocol ID for 
the '%s' frame type has not been registered with the LSL.

 Please register an ID by adding a 'PROTOCOL IPX id %s' entry
to the 'LINK DRIVER %s' NET.CFG section, replacing 'id' with the
appropriate protocol ID for IPX to use on this frame type.  For the new ID to
be recognized the %s LAN Driver must be reloaded.
 Maximum number of stacks already loaded.  Either unload an
existing stack or increase the maximum by adding a 'MAX STACKS' entry to
the 'LINK SUPPORT' section in the NET.CFG file.
 Attempt to register a IPX Protocol ID failed.  Increase the maximum number
of stacks in the LSL.COM by adding a 'MAX STACKS' entry to the 'LINK SUPPORT'
section in the NET.CFG file.
 A IPX protocol stack is already registered with the LSL.
 Invalid command line option was specified: '%s'.

 Missing or invalid value for '%s'
parameter was specified on line %u of the configuration file.
Entry was ignored.

 Missing or invalid ON/OFF value for '%s'
parameter was specified on line %u of the configuration file.
Entry was ignored.

 Missing or invalid IPATCH values were specified on line %u of
the configuration file.  Entry was ignored.

 Specified board in 'BIND' statement on line %u of the
configuration file does not exist.  Entry was ignored.

 Invalid 'BIND' statement on line %u of the configuration file.
Requires a board number (Example, BIND 2).  Entry was ignored.

 Duplicate 'BIND' statement on line %u of the configuration 
file.  IPXTLI cannot be bound to more than one board.  Entry was ignored.

 An invalid entry was found on line %u of the configuration
file.  Entry '%s' ignored.

 