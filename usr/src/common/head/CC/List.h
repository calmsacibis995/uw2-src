/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident       "@(#)sc:List/incl/List.h	3.11" */
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

#ifndef LISTH
#define LISTH

#include <stddef.h>
#include <new.h>
#include <Pool.h>
#include <Listio.h>

class lnk_ATTLC;
class Lizt_ATTLC;

template <class T> class lnnk_ATTLC;

typedef	void*	voidP;

// implements two-way pointers 

struct lnk_ATTLC {
	lnk_ATTLC* nxt;
	lnk_ATTLC* prv;

	lnk_ATTLC() {}
	virtual ~lnk_ATTLC() {}

	void	init(lnk_ATTLC* p, lnk_ATTLC* s) { prv = p; nxt = s; }
	virtual lnk_ATTLC* copy();
	virtual int operator==(lnk_ATTLC&);
};


// base class for all [Const_]Listiter(T)'s 

class	Liztiter_ATTLC {

	friend class Lizt_ATTLC;

	Liztiter_ATTLC(Lizt_ATTLC* lp) : theLizt(lp), cache(0), nextIt(0) {}

    protected:
	Lizt_ATTLC*	theLizt;	// associated List
	Liztiter_ATTLC* nextIt;		// next on chain
	lnk_ATTLC* 	cache;		// a recently retrieved link
	int		cacheNo;	// its index or garbage if cache == 0
	int		index;		// current position
	lnk_ATTLC*	pred;		// current position

	lnk_ATTLC*	getAt(int);
	lnk_ATTLC*	next();
	lnk_ATTLC*	prev();
	lnk_ATTLC*	peek_next() const { 
				return at_end() ? (lnk_ATTLC*)0 : pred->nxt;
			}
	lnk_ATTLC* 	peek_prev() const { 
				return at_head() ? (lnk_ATTLC*)0 : pred;
			}
	void		insert_prev(lnk_ATTLC*);
	void		insert_next(lnk_ATTLC*);
	lnk_ATTLC* 	remove_prev();
	lnk_ATTLC* 	remove_next();
	void 		reset0();

    public:
	
	Liztiter_ATTLC(Lizt_ATTLC&);
	Liztiter_ATTLC(const Liztiter_ATTLC&);
	~Liztiter_ATTLC();

	Liztiter_ATTLC&	operator=(const Liztiter_ATTLC&);

	int	operator==(const Liztiter_ATTLC& l) const {	
			return theLizt == l.theLizt && index == l.index;
		}
	int 	operator!=(const Liztiter_ATTLC& l) const {
			return !(*this == l);
		}
	int 	position() const {
			return index;
		}
	void 	reset(unsigned = 0);
	void 	end_reset(unsigned = 0);
	int 	at_head() const { return index == 0; }
	int 	at_end() const;
};

// base class for all List<T>'s 

class	Lizt_ATTLC {
	friend class Liztiter_ATTLC;

    protected:
	lnk_ATTLC*	t;		// tail
	int  		sz;		// number of elements
	Liztiter_ATTLC*	iter_head;

	Lizt_ATTLC();
	Lizt_ATTLC(const Lizt_ATTLC& x) : iter_head(0) { init_all(x); }
	Lizt_ATTLC(const Lizt_ATTLC&, const Lizt_ATTLC&);
	~Lizt_ATTLC();

	void 	delete_all();			// used by dtor and operator=()
	void	init_all(const Lizt_ATTLC&);	// used by ctor and operator=()
	void	init_all_to_empty();   		// used by make_empty()
	void	add_a_link(lnk_ATTLC*);		// used by put() and unget()

	lnk_ATTLC*  tail() const { return t; }
	lnk_ATTLC*  head() const { return t ? t->nxt : 0; }
	operator    const void*() const { return sz ? this : 0; }

	Lizt_ATTLC& unget(lnk_ATTLC*);
	Lizt_ATTLC& unget(const Lizt_ATTLC&);
	Lizt_ATTLC& put(lnk_ATTLC*);
	Lizt_ATTLC& put(const Lizt_ATTLC&);
	lnk_ATTLC*  get();
	lnk_ATTLC*  unput();
	lnk_ATTLC*  getAt(int);

    public:
	Lizt_ATTLC& operator=(const Lizt_ATTLC&);

	int	operator==(const Lizt_ATTLC&) const;
	int	operator!=(const Lizt_ATTLC& x) const { return !(*this == x); }
	void    make_empty();
	int 	length() const { return sz; }

	void 	reset_all_iters();
};

template <class T> class List;
template <class T> class Listiter;
template <class T> class Const_listiter;

template <class T>
class	lnnk_ATTLC : public lnk_ATTLC {

	friend	class List<T>;
	friend	class Const_listiter<T>;
	friend	class Listiter<T>;
	friend	void voidP_List_sort_internal(List<voidP>&,
				int (*)(const voidP &, const voidP &), size_t);

	static	Pool* pool;
	T	val;

	lnnk_ATTLC(T& pp) : val(pp) {}
	~lnnk_ATTLC();

	lnk_ATTLC* copy();
	int	operator==(lnk_ATTLC&);

    public:
	void*	operator new(size_t);
	void	operator delete(void* l);
	static	void init_pool() {	// should be called by List constructors
			if (pool == 0)
			    pool = new Pool(sizeof(lnnk_ATTLC<T>));
		}

	static	lnk_ATTLC* getnewlnnk_ATTLC(const T&);
	static	void deletelnnk_ATTLC(T&, lnnk_ATTLC<T>*);
};

template <class T>
class	List : public Lizt_ATTLC {

	friend	void voidP_List_sort_internal(List<voidP>&,
					int(*)(const voidP &, const voidP &), size_t);

	List(const List<T>& a0, const List<T>& a1);
	List(const List<T>&, const T&);

    protected:
	T*	getAt(int);

    public:
	List();
	List(const List<T>&);
	List(const T&);
	List(const T&, const T&);
	List(const T&, const T&, const T&);
	List(const T&, const T&, const T&, const T&);
	~List();

	operator const void*() const { return Lizt_ATTLC::operator const void*(); }

	int	 operator==(const List<T>& l) const {
			return (Lizt_ATTLC&)*this == (Lizt_ATTLC&)l;
		 }
	int	 operator!=(const List<T>& l) const {
			return (Lizt_ATTLC&)*this != (Lizt_ATTLC&)l;
		 }

	List<T>  operator+(const List<T>& ll) const
		{ return List<T>(*this, ll); }
	List<T>  operator+(const T& t) const
		{ return List<T>(*this, t); }

	List<T>& operator=(const List<T>& a) {
			return (List<T>&)(*(Lizt_ATTLC*)this = *(Lizt_ATTLC*)&a);
		 }
	List<T>& put(const T& x);
	List<T>& put(const List<T>& ll) {
			return (List<T>&) Lizt_ATTLC::put((Lizt_ATTLC&) ll);
		 }
	T	 unput();
	int	 unput(T&);
	T	 get();
	int	 get(T&);
	List<T>& unget(const T& x);
	List<T>& unget(const List<T>& ll) { 
			return (List<T>&)Lizt_ATTLC::unget((Lizt_ATTLC&) ll);
		 }

	T*	 head() const;
	T*	 tail() const;

	T&	 operator[](unsigned);
	const T& operator[](unsigned) const;
#ifdef INT_INDEXING
	T&	 operator[](int);
	const T& operator[](int) const;
#endif
	void     sort(int (*)(const T&, const T&));
};

template <class T>
class	Const_listiter : public Liztiter_ATTLC {

    protected:
	T*	 getAt(int i);

    public:
	Const_listiter(const List<T>&);
	Const_listiter(const Const_listiter<T>&);
	~Const_listiter();

	Const_listiter<T>&	operator=(const Const_listiter<T>& l) {
			return (Const_listiter<T>&) ((Liztiter_ATTLC&)*this =
						    (const Liztiter_ATTLC&)l);
		}

	int	operator==(const Const_listiter<T>& l) const {
			return (const Liztiter_ATTLC&)*this == (const Liztiter_ATTLC&)l;
		}
	int	operator!=(const Const_listiter<T>& l) const {
			return (const Liztiter_ATTLC&)*this != (const Liztiter_ATTLC&)l;
		}

	int	find_next(const T&);
	int	find_prev(const T&);
	int	next(T& t);
	int	next(T*& t);
	T*	next();
	int	prev(T& t);
	int	prev(T*& t);
	T*	prev();

	int	step_next() {
			return Liztiter_ATTLC::next() != 0;
		}
	int	step_prev() {
			return Liztiter_ATTLC::prev() != 0;
		}
	int	peek_next(T&) const;
	int	peek_next(T*&) const;
	T*	peek_next() const;
	int	peek_prev(T&) const;
	int	peek_prev(T*&) const;
	T*	peek_prev() const;

	const List<T>* the_list() {
			return (const List<T>*)theLizt;
		}
};

template <class T>
class	Listiter : public Const_listiter<T> {

    public:
	Listiter(List<T>&);
	Listiter(const Listiter<T>&);
	~Listiter();

	Listiter<T>&	operator=(const Listiter<T>& l) {
				return (Listiter<T>&) ((Liztiter_ATTLC&)*this =
						      (Liztiter_ATTLC&)l);
			}

	int	operator==(const Listiter<T>& l) const {
			return (Liztiter_ATTLC&)*this == (Liztiter_ATTLC&)l;
		}
	int	operator!=(const Listiter<T>& l) const {
			return (Liztiter_ATTLC&)*this != (Liztiter_ATTLC&)l;
		}

	List<T>* the_list() { return (List<T>*)theLizt; }

	// the following operations change the container
	int	remove_prev();
	int	remove_next();
	int	remove_prev(T&);
	int	remove_next(T&);
	void 	insert_prev(const T& x);
	void 	insert_next(const T& x);
	int	replace_prev(const T&);
	int	replace_next(const T&);
};

template <class T> class List_of_p;
template <class T> class List_of_piter;

template <class T>
class List_of_p : public List<voidP> {

    public:
	List_of_p();
	List_of_p(const T*);
	List_of_p(const T*, const T*);
	List_of_p(const T*, const T*, const T*);
	List_of_p(const T*, const T*, const T*, const T*);
	List_of_p(const List_of_p<T>& ll);
	~List_of_p();

	T*&	  operator[](unsigned);
	const T*& operator[](unsigned) const;
#ifdef INT_INDEXING
	T*&	  operator[](int);
	const T*& operator[](int) const;
#endif

	operator  const void*() const {
			return List<voidP>::operator const void*();
		  }

	int	operator==(const List_of_p<T>& ll) const {
			return (const List<voidP>&)*this == (const List<voidP>&)ll;
		}
	int	operator!=(const List_of_p<T>& l) const {
			return !(*this == l);
		}

	List_of_p<T>&	operator=(const List_of_p<T>& ll) {
				return (List_of_p<T>&) ((List<voidP>&)*this =
					       (const List<voidP>&)ll);
			}
	List_of_p<T>	operator+(const List_of_p<T>&) const;
	List_of_p<T>	operator+(const T*) const;

	List_of_p<T>&	put(const T* t) {
				return (List_of_p<T>&) List<voidP>::put((voidP)t);
			}
	List_of_p<T>&	put(const List_of_p<T>& ll) {
				return (List_of_p<T>&) List<voidP>::put((const List<voidP>&)ll);
			}

	T*	unput() { return (T*)List<voidP>::unput(); }
	int 	unput(T*& t) { return List<voidP>::unput((voidP&)t); }

	T*	get() { return (T*)List<voidP>::get(); }
	int	get(T*& t) { return List<voidP>::get((voidP&)t); }

	List_of_p<T>&	unget(const T* x) {
				return (List_of_p<T>&) List<voidP>::unget((voidP)x);
			}
	List_of_p<T>&	unget(const List_of_p<T>& ll) {
				return (List_of_p<T>&) List<voidP>::unget((const List<voidP>&)ll);
			}

	T*	head() const {
		    voidP* ptr = List<voidP>::head();
		    return (ptr ? (T*)*ptr : 0);
		}
	T*	tail() const {
		    voidP* ptr = List<voidP>::tail();
		    return (ptr ? (T*)*ptr : 0);
		}

	void  	sort(int (*pf)(const T* &, const T* &)) {
			List<voidP>::sort((int (*)(const voidP &, const voidP &))pf);
		}
};

template <class T>
class Const_list_of_piter : public Listiter<voidP> {

    public:
	Const_list_of_piter(const List_of_p<T>&);
	Const_list_of_piter(const Const_list_of_piter<T>&);
	~Const_list_of_piter();


	int 	operator==(const Const_list_of_piter<T>& l) const {
			return (const Listiter<voidP>&)*this == ((const Listiter<voidP>&)l);
		}

	int 	operator!=(const Const_list_of_piter<T>& l) const {
			return !(*this == l);
		}

	Const_list_of_piter<T>& 	operator=(const Const_list_of_piter<T>& ll) {
			return (Const_list_of_piter<T>&)((Listiter<voidP>&)*this =
				(const Listiter<voidP>&)ll);
		}

	const List_of_p<T>* 	the_list() {
			return (const List_of_p<T>*)Listiter<voidP>::the_list();
		}

	int 	find_next(T*& t) { return Listiter<voidP>::find_next((voidP&)t); }
	int 	find_prev(T*& t) { return Listiter<voidP>::find_prev((voidP&)t); }

	T* 	next() {
		    voidP* ptr = Listiter<voidP>::next();
		    return (ptr ? (T*)*ptr : 0);
		}
	int 	next(T* &t) { return Listiter<voidP>::next((voidP&)t); }
	int 	next(T**& t) { return Listiter<voidP>::next((voidP*&)t); }

	T* 	prev() {
		    voidP* ptr = Listiter<voidP>::prev();
		    return (ptr ? (T*)*ptr : 0);
		}
	int 	prev(T*& t) { return Listiter<voidP>::prev((voidP&)t); }
	int 	prev(T**& t) { return Listiter<voidP>::prev((voidP*&)t); }

	T* 	peek_next() const {
		    voidP* ptr = Listiter<voidP>::peek_next();
		    return (ptr ? (T*)*ptr : 0);
		}
	int 	peek_next(T*& t) const { return Listiter<voidP>::peek_next((voidP&)t); }
	int 	peek_next(T**& t) const { return Listiter<voidP>::peek_next((voidP*&)t); }

	T* 	peek_prev() const {
		    voidP* ptr = Listiter<voidP>::peek_prev();
		    return (ptr ? (T*)*ptr : 0);
		}
	int 	peek_prev(T*& t) const { return Listiter<voidP>::peek_prev((voidP&)t); }
	int 	peek_prev(T**& t) const { return Listiter<voidP>::peek_prev((voidP*&)t); }

};


template <class T>
class List_of_piter : public Const_list_of_piter<T> {

    public:
	List_of_piter(List_of_p<T>&);
	List_of_piter(const List_of_piter<T>&);
	~List_of_piter();


	int	operator==(const List_of_piter<T>& l) const {
			return (const Const_list_of_piter<T>&)*this ==
				((const Const_list_of_piter<T>&)l);
		}

	int	operator!=(const List_of_piter<T>& l) const {
			return !(*this == l);
		}

	List_of_piter<T>&	operator=(const List_of_piter<T>& ll) {
			return (List_of_piter<T>&)((Listiter<voidP>&)*this =
				(const Listiter<voidP>&)ll);
		}

	List_of_p<T>*	the_list() {
			return (List_of_p<T>*)Listiter<voidP>::the_list();
		}

	// the following operations change the container

	int	remove_prev() { return Listiter<voidP>::remove_prev(); }
	int	remove_prev(T*& x) { return Listiter<voidP>::remove_prev((voidP&)x); }
	int	remove_next() { return Listiter<voidP>::remove_next(); }
	int	remove_next(T*& x) { return Listiter<voidP>::remove_next((voidP&)x); }

	void	insert_prev(T*& x) { Listiter<voidP>::insert_prev((voidP&)x); }
	void	insert_next(T*& x) { Listiter<voidP>::insert_next((voidP&)x); }

	int	replace_prev(T* x) { return Listiter<voidP>::replace_prev((voidP)x); }
	int	replace_next(T* x) { return Listiter<voidP>::replace_next((voidP)x); }
};

#endif
