NAME			EUC

MB_CUR_MAX 		2

STATE_DEPEND_ENCODING	FALSE

#if EUC32
WC_ENCODING_MASK	0x30000000
WC_SHIFT_BITS		7
#else
WC_ENCODING_MASK	0x00008080
WC_SHIFT_BITS		8
#endif

CODESET0
    GL
    INITIAL_STATE_GL
    LENGTH		1
    WC_ENCODING		0x00000000
    ENCODING
	ISO8859-1	GL
    FONT
	ISO8859-1	GL

CODESET1
    GR
    INITIAL_STATE_GR
    LENGTH		2
#if EUC32
    WC_ENCODING		0x30000000
#else
    WC_ENCODING		0x00008080
#endif
    ENCODING
	KSC5601.1987-0	GL
	KSC5601.1987-0	GR
    FONT
	KSC5601.1987-0	GL