/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwmisc:netware/include/i18n.h	1.1"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/include/i18n.h,v 1.1 1994/01/24 19:38:20 renu Exp $"

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL							*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
///////////////////////////////////////////////////////////////
// I18nComponent.h: 
///////////////////////////////////////////////////////////////
#ifndef I18N_H
#define I18N_H

#include <Xm/Xm.h>

class I18n {
    
  private:  

  public:
	static char *GetStr (char *); 
};

#endif
