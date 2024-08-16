/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwmisc:netware/include/Login.h	1.2"

/////////////////////////////////////////////////////////
// Login.h:
/////////////////////////////////////////////////////////
#ifndef LOGIN_H
#define LOGIN_H
#include <Xm/Xm.h>


class Login {
    
private:
    
	Widget		_w, _form, _actionbuttons[3], _loginform, 
			_pwdform, _login_label, _login_text, _pwd_label, 
			_pwd_text, _pane, _control;

	static void	okCB (Widget, XtPointer, XtPointer);
	static void	helpCB (Widget, XtPointer, XtPointer);
	static void	cancelCB (Widget, XtPointer, XtPointer);
	static void	destroyCB (Widget, XtPointer, XtPointer);

protected:

	virtual void	ok (Widget) = 0;
	virtual void	help (Widget) = 0;
	virtual void	cancel (Widget);
	virtual void	destroy ();

public:
    
    	Login ();
    	virtual ~Login ();
	Widget	loginText () const { return _login_text; }
	Widget	pwdText () const { return _pwd_text; }
    	void postDialog ( Widget, char * , char *, char * = NULL);
};

#endif

