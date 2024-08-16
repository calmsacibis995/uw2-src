/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:G2++/incl/Vblock.h	3.2" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#ifndef VBLOCKH
#define VBLOCKH

#include <Block.h>

class Vb_ATTLC{
public:
    Vb_ATTLC(){};
    virtual ~Vb_ATTLC(){};
    virtual unsigned size() const =0;
    virtual unsigned size(unsigned)=0;
    virtual void* elem(int i)=0;
    virtual void* beginning()=0;
    virtual void* finish()=0;
};

template <class T>
class Vblock : public Vb_ATTLC {
public:
    unsigned size() const;
    unsigned size(unsigned k);

    Vblock(){}
    Vblock(unsigned k):B(k){}
    Vblock(const Vblock<T>& b):B(b.B){}
    ~Vblock(){}

    Vblock<T>& operator=(const Vblock<T>& b){
	B=b.B;
	return *this;
    }
    operator T*() { return (T*)B; }
    T* end(){ return B.end(); }
    T& operator[](int i) { return B.operator [](i);}
    const T& operator[](int i) const { return B.operator [](i); }
    int reserve(unsigned k) { return B.reserve(k); }
    void swap(Vblock<T>& b){B.swap(b.B);}
private:
    Block<T> B;

    void* elem(int i);

    void* beginning();

    void* finish();
};


#pragma can_instantiate Block<char>
#pragma can_instantiate Vblock<char>

#endif
