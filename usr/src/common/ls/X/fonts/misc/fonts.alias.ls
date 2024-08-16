PKGINST=${PKGINST:-ls}
sed -e "/# PKG=$PKGINST$/d" /usr/X/lib/fonts/misc/fonts.alias 2>/dev/null

if [ "$1" = install ]
then
	# add the following aliases to the file
	cat - <<!
r14	-misc-fixed-medium-r-normal--14-*-*-*-*-*-jisx0201.1976-* # PKG=$PKGINST
rk14	-misc-fixed-medium-r-normal--14-*-*-*-*-*-jisx0201.1976-* # PKG=$PKGINST
k14	-misc-fixed-medium-r-normal--14-130-75-75-c-140-jisx0208.1983-0 # PKG=$PKGINST
!
fi

exit 0
