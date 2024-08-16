#	Copyright (c) 1989 AT&T
#	  All Rights Reserved  
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any   
#	actual or intended publication of such source code.
#

#ident	"@(#)langsup:common/ls/codesets/6937.88591.d	1.1"

# 1989/01 AT&T-UEL
# From codeset:	ISO 6937 (Teletext)
# To codeset:	ISO 8859-1
# Based on:
# Notes:

map full (6937.88591.d){
	error('_')

	define("'" \302)	# acute
	define('`' \301)	# grave
	define('^' \303)	# circumflex
	define(':' \310)	# umlaut
	define('~' \304)	# tilde
	define('v' \306)	# breve
	define('o' \312)	# ring
	define('-' \305)	# macron
	define(ogonek \316)	# ogonek
	define("''" \315)	# double acute
	define(',' \313)	# cedilla
	define('.' \307)
	define(caron \317)
	"'"(a \341)
	"'"(A \301)
	`(a \340)
	`(A \300)
	^(a \342)
	^(A \302)
	:(a \344)
	:(A \304)
	~(a \343)
	~(A \303)
	v(a a)
	v(A A)
	o(a \345)
	o(A \305)
	-(a a)
	-(A A)
	ogonek(a a)
	ogonek(A A)
	string(\361 \346)
	string(\341 \306)
	"'"(c c)
	"'"(C C)
	^(c c)
	^(C C)
	caron(c c)
	caron(C C)
	.(c c)
	.(C C)
	,(c \347)
	,(C \307)
	caron(d d)
	caron(D D)
	string(\362 d)
	string(\342 \320)
	string(\363 \360)
	"'"(e \351)
	"'"(E \311)
	`(e \350)
	`(E \310)
	^(e \352)
	^(E \312)
	:(e \353)
	:(E \313)
	caron(e e)
	caron(E E)
	.(e e)
	.(E E)
	-(e e)
	-(E E)
	ogonek(e e)
	ogonek(E E)
	"'"(g g)
	^(g g)
	^(G G)
	v(g g)
	v(G G)
	.(g g)
	.(G G)
	,(G G)
	^(h h)
	^(H H)
	string(\364 h)
	string(\344 H)
	"'"(i \355)
	"'"(I \315)
	`(i \354)
	`(I \314)
	^(i \356)
	^(I \316)
	:(i \357)
	:(I \317)
	~(i i)
	~(I I)
	.(I I)
	-(i i)
	-(I I)
	ogonek(i i)
	ogonek(I I)
	string(\366 "ij")
	string(\346 "IJ")
	string(\365 i)
	^(j j)
	^(J J)
	,(k k)
	,(K K)
	string(\360 k)
	"'"(l l)
	"'"(L L)
	caron(l l)
	caron(L L)
	,(l l)
	,(L L)
	string(\370 l)
	string(\350 L)
	string(\367 l)
	string(\347 L)
	"'"(n n)
	"'"(N N)
	~(n \361)
	~(N \321)
	caron(n n)
	caron(N N)
	,(n n)
	,(N N)
	string(\376 _)
	string(\356 _)
	string(\357 "'n")
	"'"(o \363)
	"'"(O \323)
	`(o \362)
	`(O \322)
	^(o \364)
	^(O \324)
	:(o \366)
	:(O \326)
	~(o \365)
	~(O \325)
	"''"(o o)
	"''"(O O)
	-(o o)
	-(O O)
	string(\372 "oe")
	string(\352 "OE")
	string(\371 \370)
	string(\351 \330)
	"'"(r r)
	"'"(R R)
	caron(r r)
	caron(R R)
	,(r r)
	,(R R)
	"'"(s s)
	"'"(S S)
	^(s s)
	^(S S)
	caron(s s)
	caron(S S)
	,(s s)
	,(S S)
	string(\373 \337)
	caron(t t)
	caron(T T)
	,(t t)
	,(T T)
	string(\375 t)
	string(\355 T)
	string(\374 \376)
	string(\354 \336)
	"'"(u \372)
	"'"(U \332)
	`(u \371)
	`(U \331)
	^(u \373)
	^(U \333)
	:(u \374)
	:(U \334)
	~(u u)
	~(U U)
	v(u u)
	v(U U)
	"''"(u u)
	"''"(U U)
	o(u u)
	o(U U)
	-(u u)
	-(U U)
	ogonek(u u)
	ogonek(U U)
	^(w w)
	^(W W)
	"'"(y \375)
	"'"(Y \335)
	^(y y)
	^(Y Y)
	:(y \377)
	:(Y Y)
	"'"(z z)
	"'"(Z Z)
	caron(z z)
	caron(Z Z)
	.(z z)
	.(Z Z)
	string(\044 \244)
	string(\244 '$')
	string(\246 \043)
	string(\250 \044)
	string(\251 \050)
	string(\252 '"')
	string(\254 '_')
	string(\255 '_')
	string(\256 '_')
	string(\257 '_')
	string(\264 \327)
	string(\270 \367)
	string(\271 "'")
	string(\272 '"')
	string(\300,'_')
	`(\040 '`')
	"'"(\040 \264)
	~(\040 '~')
	-(\040 \257)
	:(\040 \250)
	string("\311\040" '_')
	o(\040 \260)
	,(\040 \270)
	string("\314\040" '_')
	string(\320 '-')
	string(\321 \271)
	string(\322 \256)
	string(\323 \251)
	string(\324 "(TM)")
	string(\325 '_')
	string(\326 '_')
	string(\327 '_')
	string(\330 '_')
	string(\331 '_')
	string(\332 '_')
	string(\333 '_')
	string(\334 "1/8")
	string(\335 "3/8")
	string(\336 "5/8")
	string(\337 "7/8")
	string(\340 '_')
	string(\343 \252)
	string(\345 '_')
	string(\353 \272)
	string(\377 '_')
	
	
}