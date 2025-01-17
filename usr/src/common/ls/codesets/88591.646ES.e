#	Copyright (c) 1989 AT&T
#	  All Rights Reserved  
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any   
#	actual or intended publication of such source code.
#

#ident	"@(#)langsup:common/ls/codesets/88591.646ES.e	1.1"

# 1989/01 AT&T-UEL

# From codeset: ISO 8859-1
# To codeset:	Spanish ascii variant - ISO 646 ES
# Based on:	88591.646.b
# Notes:	"extended"

map full (88591.646ES.e) {

	string('#' '_')

	string('@' '_')
	string('[' '(')
	string('\\' '_')
	string(']' ')')
	string('{' '(')
	string('|' '_')
	string('}' ')')

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

	string(\240 ' ')	# no-break space
	string(\241 '[')	# inverted exclamation mark
	string(\242 'c')	# cent sign
	string(\243 '#')	# pound sign -> hash
	string(\244 '_')	# currency sign
	string(\245 'Y')	# yen sign
	string(\246 '_')	# broken bar
	string(\247 '@')	# paragraph sign, section sign

	string(\250 '\"')	# diaereses
	string(\251 "(c)")	# copyright sign
	string(\252 '_')	# feminine ordinal indicator
	string(\253 '\"')	# left angle quotation mark
	string(\254 '_')	# not sign
	string(\255 '-')	# soft hyphen
	string(\256 "(R)")	# registered trademark sign
	string(\257 '_')	# macron

	string(\260 '{')	# ring above, degree sign
	string(\261 "+/-")	# plus-minus sign
	string(\262 '_')	# superscript 2
	string(\263 '_')	# superscript 3
	string(\264 '\'')	# acute accent
	string(\265 'u')	# micro sign
	string(\266 '_')	# pilcrow sign
	string(\267 '.')	# middle dot

	string(\270 ',')	# cedilla
	string(\271 '_')	# superscript 1
	string(\272 '_')	# masculine ordinal indicator
	string(\273 '\"')	# right angle quotation mark
	string(\274 "1/4")	# vulgar fraction one quarter
	string(\275 "1/2")	# vulgar fraction one half
	string(\276 "3/4")	# vulgar fraction three quarters
	string(\277 ']')	# inverted question mark

	string(\300 'A')
	string(\301 'A')
	string(\302 'A')
	string(\303 'A')
	string(\304 "AE")
	string(\305 'A')
	string(\306 "AE")
	string(\307 'C')

	string(\310 'E')
	string(\311 'E')
	string(\312 'E')
	string(\313 'E')
	string(\314 'I')
	string(\315 'I')
	string(\316 'I')
	string(\317 'I')

	string(\320 'D')
	string(\321 '\\')
	string(\322 'O')
	string(\323 'O')
	string(\324 'O')
	string(\325 'O')
	string(\326 "OE")
	string(\327 '*')	# multiplication sign

	string(\330 'O')
	string(\331 'U')
	string(\332 'U')
	string(\333 'U')
	string(\334 "UE")
	string(\335 'Y')
	string(\336 'P')
	string(\337 "ss")

	string(\340 'a')
	string(\341 'a')
	string(\342 'a')
	string(\343 'a')
	string(\344 "ae")
	string(\345 'a')
	string(\346 "ae")
	string(\347 '}')

	string(\350 'e')
	string(\351 'e')
	string(\352 'e')
	string(\353 'e')
	string(\354 'i')
	string(\355 'i')
	string(\356 'i')
	string(\357 'i')

	string(\360 'd')
	string(\361 '|')
	string(\362 'o')
	string(\363 'o')
	string(\364 'o')
	string(\365 'o')
	string(\366 "oe")
	string(\367 '/')	# division sign

	string(\370 'o')
	string(\371 'u')
	string(\372 'u')
	string(\373 'u')
	string(\374 "ue")
	string(\375 'y')
	string(\376 'p')
	string(\377 'y')
}

