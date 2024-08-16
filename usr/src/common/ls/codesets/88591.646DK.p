#	Copyright (c) 1989 AT&T
#	  All Rights Reserved  
#
#	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
#	The copyright notice above does not evidence any   
#	actual or intended publication of such source code.
#

#ident	"@(#)langsup:common/ls/codesets/88591.646DK.p	1.1"

# 1989/01 AT&T-UEL
# From codeset:	ISO 8859-1
# To codeset:	Danish 646 ascii variant - ISO 646 DK
# Based on:	88591.646.p
# Notes:	Overprinting

map full (88591.646DK.p) {

	string('[' '(')
	string(\134 '_')	#\134 = '\'
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
	string(\241 '!')	# inverted exclamation mark
	string(\242 "c\b~")	# cent sign
	string(\243 "L\b=")	# pound sign
	string(\244 '_')	# currency sign
	string(\245 "Y\b=")	# yen sign
	string(\246 '_')	# broken bar is substituted
	string(\247 '_')	# paragraph sign, section sign

	string(\250 '"')	# diaereses
	string(\251 "(c)")	# copyright sign
	string(\252 '_')	# feminine ordinal indicator
	string(\253 '"')	# left angle quotation mark
	string(\254 '_')	# not sign
	string(\255 '-')	# soft hyphen
	string(\256 "(R)")	# registered trademark sign
	string(\257 '_')	# macron

	string(\260 '_')	# ring above, degree sign
	string(\261 "+\b_")	# plus-minus sign
	string(\262 '_')	# superscript 2
	string(\263 '_')	# superscript 3
	string(\264 "'")	# acute accent
	string(\265 'u')	# micro sign
	string(\266 '_')	# pilcrow sign
	string(\267 '.')	# middle dot

	string(\270 ',')	# cedilla
	string(\271 '_')	# superscript 1
	string(\272 '_')	# masculine ordinal indicator
	string(\273 '"')	# right angle quotation mark
	string(\274 "1/4")	# vulgar fraction one quarter
	string(\275 "1/2")	# vulgar fraction one half
	string(\276 "3/4")	# vulgar fraction three quarters
	string(\277 '?')	# inverted question mark

	string(\300 "A\b`")
	string(\301 "A\b'")
	string(\302 "A\b^")
	string(\303 "A\b~")
	string(\304 "A\b\"")
	string(\305 ']')
	string(\306 '[')
	string(\307 "C\b,")

	string(\310 "E\b`")
	string(\311 "E\b'")
	string(\312 "E\b^")
	string(\313 "E\b\"")
	string(\314 "I\b`")
	string(\315 "I\b'")
	string(\316 "I\b^")
	string(\317 "I\b\"")

	string(\320 "D\b-")
	string(\321 "N\b~")
	string(\322 "O\b`")
	string(\323 "O\b'")
	string(\324 "O\b^")
	string(\325 "O\b~")
	string(\326 "O\b\"")
	string(\327 '*')	# multiplication sign

	string(\330 '\134')	#\134 = '\'
	string(\331 "U\b`")
	string(\332 "U\b'")
	string(\333 "U\b^")
	string(\334 "U\b\"")
	string(\335 "Y\b'")
	string(\336 'P')
	string(\337 "ss")

	string(\340 "a\b`")
	string(\341 "a\b'")
	string(\342 "a\b^")
	string(\343 "a\b~")
	string(\344 "a\b\"")
	string(\345 '}')
	string(\346 '{')
	string(\347 "c\b,")

	string(\350 "e\b`")
	string(\351 "e\b'")
	string(\352 "e\b^")
	string(\353 "e\b\"")
	string(\354 "i\b`")
	string(\355 "i\b'")
	string(\356 "i\b^")
	string(\357 "i\b\"")

	string(\360 "d\b-")
	string(\361 "n\b~")
	string(\362 "o\b`")
	string(\363 "o\b'")
	string(\364 "o\b^")
	string(\365 "o\b~")
	string(\366 "o\b\"")
	string(\367 "-\b:")	# division sign

	string(\370 '|')
	string(\371 "u\b`")
	string(\372 "u\b'")
	string(\373 "u\b^")
	string(\374 "u\b\"")
	string(\375 "y\b'")
	string(\376 'p')
	string(\377 "y\b\"")
}

