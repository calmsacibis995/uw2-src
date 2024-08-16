/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/////////////////////////////////////////////////////////
// Prompt.h:
/////////////////////////////////////////////////////////
#ifndef PROMPT_H
#define PROMPT_H
#include "PDialog.h"
#include <Xm/Xm.h> 

class MenuBar;

class Prompt : public PDialog{
    
private:

	MenuBar	*_menu;

protected:

	virtual void 	ok (XmSelectionBoxCallbackStruct *);
	virtual void 	cancel ();
	virtual void 	help (Widget);

public:
    
    	Prompt (char *);
	void	postPrompt (Widget, char *, char *, char *, MenuBar *);
};

extern Prompt *thePromptMgr;

#endif
