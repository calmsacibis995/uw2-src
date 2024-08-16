#	Copyright (c) 1989 AT&T
#	  All Rights Reserved  
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any   
#	actual or intended publication of such source code.
#

#ident	"@(#)langsup:common/ls/codesets/88591.asc.e	1.1"

# 1989/01 AT&T-UEL
# From codeset:	ISO 8859-1
# To codeset:	8 bit ASCII
# Based on:
# Notes:	Basicly uses the default character '_'
#		for all characters with the top bit set.

map full (88591.asc.e) {

	string(\200 '_')
	string(\201 '_')
	string(\202 '_')
	string(\203 '_')
	string(\204 '_')
	string(\205 '_')
	string(\206 '_')
	string(\207 '_')

	string(\210 '_')
	string(\211 '_')
	string(\212 '_')
	string(\213 '_')
	string(\214 '_')
	string(\215 '_')
	string(\216 '_')
	string(\217 '_')

	string(\220 '_')
	string(\221 '_')
	string(\222 '_')
	string(\223 '_')
	string(\224 '_')
	string(\225 '_')
	string(\226 '_')
	string(\227 '_')

	string(\230 '_')
	string(\231 '_')
	string(\232 '_')
	string(\233 '_')
	string(\234 '_')
	string(\235 '_')
	string(\236 '_')
	string(\237 '_')

	string(\240 ' ')
	string(\241 '\255')
	string(\242 '\233')
	string(\243 '\234')
	string(\244 '_')
	string(\245 '\235')
	string(\246 '|')
	string(\247 '_')

	string(\250 '\"')
	string(\251 "(c)")
	string(\252 '\246')
	string(\253 '\256')
	string(\254 '\252')
	string(\255 '-')
	string(\256 "(R)")
	string(\257 '_')

	string(\260 '\370')
	string(\261 '\361')
	string(\262 '\375')
	string(\263 '_')
	string(\264 '\'')
	string(\265 '\346')
	string(\266 '_')
	string(\267 '\372')

	string(\270 ',')
	string(\271 '_')
	string(\272 '\247')
	string(\273 '\257')
	string(\274 '\254')
	string(\275 '\253')
	string(\276 "3/4")
	string(\277 '\250')

	string(\300 'A')
	string(\301 'A')
	string(\302 'A')
	string(\303 'A')
	string(\304 '\216')
	string(\305 '\217')
	string(\306 '\222')
	string(\307 '\200')

	string(\310 'E')
	string(\311 '\220')
	string(\312 'E')
	string(\313 'E')
	string(\314 'I')
	string(\315 'I')
	string(\316 'I')
	string(\317 'I')

	string(\320 'D')
	string(\321 '\245')
	string(\322 'O')
	string(\323 'O')
	string(\324 'O')
	string(\325 'O')
	string(\326 '\231')
	string(\327 '*')

	string(\330 '\355')
	string(\331 'U')
	string(\332 'U')
	string(\333 'U')
	string(\334 '\232')
	string(\335 'Y')
	string(\336 'P')
	string(\337 '\341')

	string(\340 '\205')
	string(\341 '\240')
	string(\342 '\203')
	string(\343 'a')
	string(\344 '\204')
	string(\345 '\206')
	string(\346 '\221')
	string(\347 '\207')

	string(\350 '\212')
	string(\351 '\202')
	string(\352 '\210')
	string(\353 '\211')
	string(\354 '\215')
	string(\355 '\241')
	string(\356 '\214')
	string(\357 '\213')

	string(\360 'd')
	string(\361 '\244')
	string(\362 '\225')
	string(\363 '\242')
	string(\364 '\223')
	string(\365 'o')
	string(\366 '\224')
	string(\367 '\366')

	string(\370 '\355')
	string(\371 '\227')
	string(\372 '\243')
	string(\373 '\226')
	string(\374 '\201')
	string(\375 'y')
	string(\376 'p')
	string(\377 '\230')
}

