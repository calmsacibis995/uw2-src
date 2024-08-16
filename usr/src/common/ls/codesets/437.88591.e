#	Copyright (c) 1989 AT&T
#	  All Rights Reserved  
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any   
#	actual or intended publication of such source code.
#

#ident	"@(#)langsup:common/ls/codesets/437.88591.e	1.1"

# 1989/01 AT&T-UEL
# From codeset:	PC437
# To codeset:	ISO 8859-1
# Derived from:	asc.88591.e
# Notes:	"best fit" is the same but franc sign = 'f', alpha ='a'

map full (437.88591.e) {

	string(\200 \307)	# -> C cedilla
	string(\201 \374)	# -> u umlaut
	string(\202 \351)	# -> e acute
	string(\203 \342)	# -> a circumflex
	string(\204 \344)	# -> a umlaut
	string(\205 \340)	# -> a grave
	string(\206 \345)	# -> a circle
	string(\207 \347)	# -> c cedilla

	string(\210 \352)	# -> e circumflex
	string(\211 \353)	# -> e umlaut
	string(\212 \350)	# -> e grave
	string(\213 \357)	# -> i umlaut
	string(\214 \356)	# -> i circumflex
	string(\215 \354)	# -> i grave
	string(\216 \304)	# -> A umlaut
	string(\217 \305)	# -> A circle

	string(\220 \311)	# -> E acute
	string(\221 \346)	# -> ae
	string(\222 \306)	# -> AE
	string(\223 \364)	# -> o circumflex
	string(\224 \366)	# -> o umlaut
	string(\225 \362)	# -> o grave
	string(\226 \373)	# -> u circumflex
	string(\227 \371)	# -> u grave

	string(\230 \377)	# -> y umlaut
	string(\231 \326)	# -> O umlaut
	string(\232 \334)	# -> U umlaut
	string(\233 \242)	# -> cent sign
	string(\234 \243)	# -> pound sign
	string(\235 \245)	# -> yen sign
	string(\236 "PT")	# -> peseta sign
	string(\237 'f')	# -> franc sign

	string(\240 \341)	# -> a acute
	string(\241 \355)	# -> i acute
	string(\242 \363)	# -> o acute
	string(\243 \372)	# -> u acute
	string(\244 \361)	# -> n tilde
	string(\245 \321)	# -> N tilde
	string(\246 \252)	# -> a bar
	string(\247 \272)	# -> o bar

	string(\250 \277)	# -> inverted ?
	string(\251 '_')
	string(\252 \254)
	string(\253 \275)	# -> one half
	string(\254 \274)	# -> one quarter
	string(\255 \241)	# -> inverted !
	string(\256 \253)	# -> left angle quotation mark
	string(\257 \273)	# -> right angle quotation mark

	string(\260 '_')
	string(\261 '_')
	string(\262 '_')
	string(\263 '_')
	string(\264 '_')
	string(\265 '_')
	string(\266 '_')
	string(\267 '_')

	string(\270 '_')
	string(\271 '_')
	string(\272 '_')
	string(\273 '_')
	string(\274 '_')
	string(\275 '_')
	string(\276 '_')
	string(\277 '_')

	string(\300 '_')
	string(\301 '_')
	string(\302 '_')
	string(\303 '_')
	string(\304 '_')
	string(\305 '_')
	string(\306 '_')
	string(\307 '_')

	string(\310 '_')
	string(\311 '_')
	string(\312 '_')
	string(\313 '_')
	string(\314 '_')
	string(\315 '_')
	string(\316 '_')
	string(\317 '_')

	string(\320 '_')
	string(\321 '_')
	string(\322 '_')
	string(\323 '_')
	string(\324 '_')
	string(\325 '_')
	string(\326 '_')
	string(\327 '_')

	string(\330 '_')
	string(\331 '_')
	string(\332 '_')
	string(\333 '_')
	string(\334 '_')
	string(\335 '_')
	string(\336 '_')
	string(\337 '_')

	string(\340 '_')	# best fit for alpha
	string(\341 \337)	# -> beta
	string(\342 '_')
	string(\343 '_')
	string(\344 '_')
	string(\345 '_')
	string(\346 \265)	# -> mu
	string(\347 '_')

	string(\350 '_')
	string(\351 '_')
	string(\352 '_')
	string(\353 '_')
	string(\354 '_')
	string(\355 \330)
	string(\356 '_')
	string(\357 '_')

	string(\360 '_')
	string(\361 \261)
	string(\362 ">=")
	string(\363 "<=")
	string(\364 '_')
	string(\365 '_')
	string(\366 \367)	#  -> divides
	string(\367 '_')

	string(\370 \260)
	string(\371 \267)
	string(\372 \267)
	string(\373 '_')
	string(\374 '_')
	string(\375 \262)	# -> power 2
	string(\376 '_')
	string(\377 ' ')

}
