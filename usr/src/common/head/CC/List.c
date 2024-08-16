/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident       "@(#)sc:List/incl/List.c	3.9" */
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

template <class T>
Pool* lnnk_ATTLC<T>::pool = 0;

template <class T>
lnnk_ATTLC<T>::~lnnk_ATTLC()
{
}

template <class T>
lnk_ATTLC*
lnnk_ATTLC<T>::copy()
{
	return new lnnk_ATTLC<T>((T&)val);
}

template <class T>
int
lnnk_ATTLC<T>::operator==(lnk_ATTLC& x)
{
	return val == ((lnnk_ATTLC<T>*)&x)->val;
}

template <class T>
void* 
lnnk_ATTLC<T>::operator new(size_t)
{
	return pool->alloc();
}

template <class T>
void 
lnnk_ATTLC<T>::operator delete(void* l)
{
	pool->free(l);
}

template <class T>
lnk_ATTLC*
lnnk_ATTLC<T>::getnewlnnk_ATTLC(const T& x)
{
	return (new lnnk_ATTLC<T>((T&)x));
}

template <class T>
void
lnnk_ATTLC<T>::deletelnnk_ATTLC(T& t, lnnk_ATTLC<T>* ptr)
{
	t = ptr->val;
	delete ptr;
}
template <class T>
List<T>::List(const List<T>& a0, const List<T>& a1)
        : Lizt_ATTLC((Lizt_ATTLC&)a0, (Lizt_ATTLC&)a1)
{
}

template <class T>
List<T>::List(const List<T>& a0, const T& t) : Lizt_ATTLC((Lizt_ATTLC&)a0)
{
	put(t);
}

template <class T>
List<T>::List()
{
	lnnk_ATTLC<T>::init_pool(); 
}

template <class T>
List<T>::List(const T& t)
{
	lnnk_ATTLC<T>::init_pool();
	put(t);
}

template <class T>
List<T>::List(const T& t, const T& u)
{
	lnnk_ATTLC<T>::init_pool();
	put(t);
	put(u);
}

template <class T>
List<T>::List(const T& t, const T& u, const T& v)
{
	lnnk_ATTLC<T>::init_pool();
	put(t);
	put(u);
	put(v);
}

template <class T>
List<T>::List(const T& t, const T& u, const T& v, const T& w)
{
	lnnk_ATTLC<T>::init_pool();
	put(t);
	put(u);
	put(v);
	put(w);
}

template <class T>
List<T>::List(const List<T>& a) : Lizt_ATTLC((const Lizt_ATTLC&)a)
{
	lnnk_ATTLC<T>::init_pool(); 
}

template <class T>
List<T>::~List()
{
}

template <class T>
T 
List<T>::unput()
{ 
	lnnk_ATTLC<T>* ll = (lnnk_ATTLC<T>*)Lizt_ATTLC::unput();
	T ans = ll->val; 
	delete ll; 
	return ans; 
}

template <class T>
T 
List<T>::get()
{ 
	lnnk_ATTLC<T>* ll = (lnnk_ATTLC<T>*)Lizt_ATTLC::get();
	// if (ll == 0)
	//	return 0;
	T ans = ll->val; 
	delete ll; 
	return ans; 
}

template <class T>
T*
List<T>::getAt(int i)
{ 
	lnnk_ATTLC<T>* ll = (lnnk_ATTLC<T>*)Lizt_ATTLC::getAt(i);
	if ( ll )
		return &(ll->val);
	else
		return (T*)0;
}

template <class T>
T&
List<T>::operator[](unsigned ii)
{
	return (T&)*(getAt(ii));
}

template <class T>
const T&
List<T>::operator[](unsigned ii) const
{
	return (const T&)*(((List<T>*)this)->getAt(ii));
}

#ifdef INT_INDEXING

template <class T>
T&
List<T>::operator[](int ii)
{
	return (T&)*(getAt(ii));
}

template <class T>
const T&
List<T>::operator[](int ii) const
{
	return (const T&)*(((List<T>*)this)->getAt(ii));
}
#endif

template <class T>
T*
List<T>::head() const
{
	lnnk_ATTLC<T>* ll = (lnnk_ATTLC<T>*)Lizt_ATTLC::head();
	if (ll == 0)
		return 0;
	return &(ll->val);
}

template <class T>
T*
List<T>::tail() const
{
	lnnk_ATTLC<T>* ll = (lnnk_ATTLC<T>*)Lizt_ATTLC::tail();
	if (ll == 0)
		return 0;
	return &(ll->val);
}

template <class T>
void
List<T>::sort(int (*cmp)(const T&,const T&))
{
	if ( length() < 2 )
		return;

	voidP_List_sort_internal(*(List<voidP>*)this, (int (*)(const voidP &, const voidP &))cmp, /*stddef.h*/ offsetof(lnnk_ATTLC<T>,val));
	reset_all_iters();
}

template <class T>
int
List<T>::unput(T& t)
{
	lnnk_ATTLC<T>* ll = (lnnk_ATTLC<T>*)Lizt_ATTLC::unput();
	if ( ll )
	{ 
		lnnk_ATTLC<T>::deletelnnk_ATTLC(t, ll);
		return 1;
	}
	else
		return 0;
}

template <class T>
int
List<T>::get(T& t)
{ 
	lnnk_ATTLC<T>* ll = (lnnk_ATTLC<T>*)Lizt_ATTLC::get();
	if ( ll )
	{ 
		lnnk_ATTLC<T>::deletelnnk_ATTLC(t, ll);
		return 1;
	}
	else
		return 0;
}

template <class T>
List<T>&
List<T>::put(const T& x)
{
	return (List<T>&) Lizt_ATTLC::put(lnnk_ATTLC<T>::getnewlnnk_ATTLC(x));
}

template <class T>
List<T>&
List<T>::unget(const T& x)
{ 
	return (List<T>&)Lizt_ATTLC::unget(lnnk_ATTLC<T>::getnewlnnk_ATTLC(x));
}

template <class T>
Const_listiter<T>::Const_listiter(const List<T>& a) : Liztiter_ATTLC((Lizt_ATTLC&)a)
{
}

template <class T>
Const_listiter<T>::Const_listiter(const Const_listiter<T>& a) : Liztiter_ATTLC((const Liztiter_ATTLC&)a)
{
}

template <class T>
Const_listiter<T>::~Const_listiter()
{
}

template <class T>
int
Const_listiter<T>::next(T& t)
{
	if ( at_end() )
		return 0;
	else
		return (t = ((lnnk_ATTLC<T>*)Liztiter_ATTLC::next())->val, 1);
}

template <class T>
int
Const_listiter<T>::next(T*& t)
{
	if ( at_end() )
		return 0;
	else
		return (t = &((lnnk_ATTLC<T>*)Liztiter_ATTLC::next())->val, 1);
}

template <class T>
T*
Const_listiter<T>::next()
{
	if ( at_end() )
		return 0;
	lnnk_ATTLC<T>* ll = (lnnk_ATTLC<T>*)Liztiter_ATTLC::next();
	return &(ll->val);
}

template <class T>
int
Const_listiter<T>::prev(T& t)
{
	if ( at_head() )
		return 0;
	else
		return (t=((lnnk_ATTLC<T>*)Liztiter_ATTLC::prev())->val, 1);
}

template <class T>
int
Const_listiter<T>::prev(T*& t)
{
	if ( at_head() )
		return 0;
	else
		return (t= &((lnnk_ATTLC<T>*)Liztiter_ATTLC::prev())->val, 1);
}

template <class T>
T*
Const_listiter<T>::prev()
{
	if ( at_head() )
		return 0;
	lnnk_ATTLC<T>* ll = (lnnk_ATTLC<T>*)Liztiter_ATTLC::prev();
	return &(ll->val);
}

template <class T>
int
Const_listiter<T>::find_prev(const T& x)
{
	if ( at_head() || theLizt->length()==0 )
		return 0;

	lnnk_ATTLC<T>* iter = (lnnk_ATTLC<T>*) pred->nxt;
	register int i = index;
	do {
		iter = (lnnk_ATTLC<T>*) iter->prv;
		if (iter->val==x) {
			index = i;
			pred = iter;
			return 1;
		}
		i--;
	} while ( i > 0 );
	return 0;
}

template <class T>
int
Const_listiter<T>::find_next(const T& x)
{
	if ( at_end() || theLizt->length()==0 )
		return 0;

	lnnk_ATTLC<T>* iter = (lnnk_ATTLC<T>*) pred;
	register int i = index;
	do {
		iter = (lnnk_ATTLC<T>*) iter->nxt;
		if (iter->val==x) {
			index = i;
			pred = iter->prv;
			return 1;
		}
		i++;
	} while ( i < theLizt->length() );
	return 0;
}

template <class T>
T*
Const_listiter<T>::peek_prev() const
{
	if ( at_head() )
	 	return 0;
	return &(((lnnk_ATTLC<T>*)Liztiter_ATTLC::peek_prev())->val);
}

template <class T>
int
Const_listiter<T>::peek_prev(T& t) const
{
	if ( at_head() )
		return 0;
	else
		return (t = ((lnnk_ATTLC<T>*)Liztiter_ATTLC::peek_prev())->val, 1);
}

template <class T>
int
Const_listiter<T>::peek_prev(T*& t) const
{
	if ( at_head() )
		return 0;
	else
		return (t = &((lnnk_ATTLC<T>*)Liztiter_ATTLC::peek_prev())->val, 1);
}

template <class T>
T*
Const_listiter<T>::peek_next() const
{
	if ( at_end() )
	 	return 0;
	return &(((lnnk_ATTLC<T>*)Liztiter_ATTLC::peek_next())->val);
}

template <class T>
int
Const_listiter<T>::peek_next(T& t) const
{
	if ( at_end() )
		return 0;
	else
		return (t = ((lnnk_ATTLC<T>*)Liztiter_ATTLC::peek_next())->val, 1);
}

template <class T>
int
Const_listiter<T>::peek_next(T*& t) const
{
	if ( at_end() )
		return 0;
	else
		return (t = &((lnnk_ATTLC<T>*)Liztiter_ATTLC::peek_next())->val, 1);
}

template <class T>
T*
Const_listiter<T>::getAt(int i)
{
	lnnk_ATTLC<T>* ll = ((lnnk_ATTLC<T>*)Liztiter_ATTLC::getAt(i));
	if ( ll )
		return &(ll->val);
	else
		return (T*)0;
}

template <class T>
Listiter<T>::Listiter(List<T>& a) : Const_listiter<T>((const List<T>&)a)
{
}

template <class T>
Listiter<T>::Listiter(const Listiter<T>& a) : Const_listiter<T>((/*const*/ Const_listiter<T>)a)
                                                                 // causes compiler warning
{
}

template <class T>
Listiter<T>::~Listiter()
{
}

template <class T>
int
Listiter<T>::remove_prev()
{
	lnnk_ATTLC<T> *aLink = (lnnk_ATTLC<T> *)Liztiter_ATTLC::remove_prev();
	if ( aLink )
		return (delete aLink, 1);
	else
		return 0;
}

template <class T>
int
Listiter<T>::remove_prev(T &x)
{
	lnnk_ATTLC<T> *aLink = (lnnk_ATTLC<T> *)Liztiter_ATTLC::remove_prev();
	if ( aLink )
		return (x = aLink->val,delete aLink, 1);
	else
		return 0;
}

template <class T>
int
Listiter<T>::remove_next()
{
	lnnk_ATTLC<T> *aLink = (lnnk_ATTLC<T> *)Liztiter_ATTLC::remove_next();
	if ( aLink )
		return (delete aLink, 1);
	else
		return 0;
}

template <class T>
int
Listiter<T>::remove_next(T &x)
{
	lnnk_ATTLC<T> *aLink = (lnnk_ATTLC<T> *)Liztiter_ATTLC::remove_next();
	if ( aLink )
		return (x = aLink->val, delete aLink, 1);
	else
		return 0;
}

template <class T>
int
Listiter<T>::replace_prev(const T& x)
{
	if ( at_head() )
		return 0;
	else
		return (((lnnk_ATTLC<T>*)Liztiter_ATTLC::peek_prev())->val=x,1);
}

template <class T>
int
Listiter<T>::replace_next(const T& x)
{
	if ( at_end() )
		return 0;
	else
		return (((lnnk_ATTLC<T>*)Liztiter_ATTLC::peek_next())->val=x,1);
}

template <class T>
void 
Listiter<T>::insert_prev(const T& x)
{
	Liztiter_ATTLC::insert_prev(lnnk_ATTLC<T>::getnewlnnk_ATTLC(x));
}

template <class T>
void 
Listiter<T>::insert_next(const T& x)
{
	Liztiter_ATTLC::insert_next(lnnk_ATTLC<T>::getnewlnnk_ATTLC(x));
}

template <class T>
List_of_piter<T>::List_of_piter(List_of_p<T>& l) : 
	Const_list_of_piter<T>((const List_of_p<T>&)l)
{
}

template <class T>
List_of_piter<T>::List_of_piter(const List_of_piter<T>& l) : 
	Const_list_of_piter<T>((Const_list_of_piter<T>)l)
{
}

template <class T>
List_of_piter<T>::~List_of_piter()
{
}

template <class T>
List_of_p<T>::List_of_p(const T* x) : List<voidP>((voidP) x)
{
}

template <class T>
List_of_p<T>::List_of_p(const T* x, const T* y) :
	List<voidP>((voidP) x, (voidP) y)
{
}

template <class T>
List_of_p<T>::List_of_p(const T* x, const T* y, const T* z) :
	List<voidP>((voidP)x, (voidP)y, (voidP)z)
{
}

template <class T>
List_of_p<T>::List_of_p(const T* x, const T* y, const T* z, const T* w) :
	List<voidP>((voidP) x, (voidP) y, (voidP) z, (voidP) w)
{
}

template <class T>
Const_list_of_piter<T>::Const_list_of_piter(const List_of_p<T>& l) :
	Listiter<voidP>((List<voidP>&) l)
{
}

template <class T>
Const_list_of_piter<T>::Const_list_of_piter(const Const_list_of_piter<T>& l) :
	Listiter<voidP>((Listiter<voidP>&) l)
{
}

template <class T>
Const_list_of_piter<T>::~Const_list_of_piter()
{
}

template <class T>
List_of_p<T>::List_of_p()
{
}

template <class T>
List_of_p<T>::List_of_p(const List_of_p<T>& ll)
	: List<voidP>((const List<voidP>&) ll)
{
}

template <class T>
List_of_p<T>::~List_of_p()
{
}

template <class T>
List_of_p<T> List_of_p<T>::operator+(const List_of_p<T>& ll) const
{
	return (List_of_p<T>&) ((List<voidP>&)*this + (List<voidP>&)ll);
}

template <class T>
List_of_p<T> List_of_p<T>::operator+(const T* t) const
{
	return (List_of_p<T>&) ((List<voidP>&)*this + (voidP)t); 
}

template <class T>
T*&
List_of_p<T>::operator[](unsigned ii)
{
	return (T*&)*getAt(ii);
}

template <class T>
const T*&
List_of_p<T>::operator[](unsigned ii) const
{
	return (const T*&)*((List_of_p<T>*)this)->getAt(ii);
}

#ifdef INT_INDEXING

template <class T>
T*&
List_of_p<T>::operator[](int ii)
{
	return (T*&)*getAt(ii);
}

template <class T>
const T*&
List_of_p<T>::operator[](int ii) const
{
	return (const T*&)*((List_of_p<T>*)this)->getAt(ii);
}
#endif
