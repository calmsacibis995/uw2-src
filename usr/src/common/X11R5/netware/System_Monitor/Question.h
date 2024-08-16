/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/////////////////////////////////////////////////////////
// Question.h:
/////////////////////////////////////////////////////////
#ifndef QUESTION_H
#define QUESTION_H
#include "QDialog.h"

class MenuBar;

class Question : public QDialog{
    
private:
	
	MenuBar	*_menu;
	int	_whichq;	
	Widget	_parent;
    
protected:

	virtual void 	ok ();
	virtual void 	cancel ();
	virtual void 	destroy ();

public:
    
    	Question (char *);
	void	postQuestion (Widget, char *, char *, MenuBar *, int);
};

extern Question *theQuestionMgr;

#endif
