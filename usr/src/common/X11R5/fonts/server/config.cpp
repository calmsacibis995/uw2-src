#ident	"@(#)r5fonts:server/config.cpp	1.4"
#ifdef sun
#undef sun
#endif

XCOMM font server configuration file
XCOMM $XConsortium: config.cpp,v 1.7 91/08/22 11:39:59 rws Exp $

clone-self = on
use-syslog = off
catalogue = FSDEFAULTFONTPATH
error-file = FSERRORS
XCOMM in decipoints
default-point-size = 120
default-resolutions = 75,75,100,100
