/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)ccsdemos:sc_demos/List_demo.h	1.1" */

#include <String.h>
#include <iostream.h>

class Student {
    String name;
    String grade;
public:
    Student(const String& n,const String& g) : name(n), grade(g) {}
    Student(const Student& s) : name(s.name), grade(s.grade) {}
    int operator==(const Student& s) { return name == s.name && grade == s.grade; }
    friend inline ostream& operator<<(ostream&,const Student&);
    friend inline int name_compare(const Student&,const Student&);
    friend inline int grade_compare(const Student&,const Student&);
}; 

inline ostream&
operator<<(ostream& os,const Student& s)
{
    os << s.name << ": " << s.grade;
    return os;
}

inline int
name_compare(const Student& s1,const Student& s2)
{
    if(s1.name + s1.grade < s2.name + s2.grade)
        return 1;
    else return 0;
}

inline int
grade_compare(const Student& s1,const Student& s2)
{
    if(s1.grade < s2.grade)
        return 1;
    else return 0;
}
