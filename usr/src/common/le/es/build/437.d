$ SCSIDM @(#)messages	1.4    LCC     ;  Modified: 18:56:55 2/14/92
$domain LCC.MERGE.DOS.MERGE
$quote "
$ The following messages are from merge.c
$ 
MERGE2 "%1: %2\n"
MERGE3 "%1: %2\n"
MERGE4 "p->nombre: %1\n"
MERGE5 "p->direc: %1\n"
MERGE6 "p->boffset: %1\n"
MERGE7 "p->bsize: %1\n"
MERGE8 "p->verdadero: %1\n"
MERGE9 "p->tipo: %1\n"
MERGE10 "MERGE: %1"
MERGE11 "MERGE: sintaxis:\n"
MERGE12 "    merge set <nombre-testigo> <valor>\n"
MERGE13 "    merge display [<nombre_testigo>]\n"
MERGE14 "    merge return <nombre-testigo>\n"
MERGE15 "p->comentario: %1\n"
MERGE16	"\nlos nombres de testigo(s) v�lidos son:\n"
$ These are error messages from merge.c
BAD_NAME "no se puede encontrar el nombre del testigo.\n"
TOOBIG "el valor es demasiado grande.\n"
BAD_VALUE "el valor debe ser num�rico.\n"
BAD_FILE "No se puede abrir el archivo.\n"
USE_ONOFF "debe usarse on|off.\n"
USE_LOCREM "debe usarse remote|local\n"
BAD_SET "valores err�neos en archivo merge.set\n"
SET_MSG "set"
ON_MSG "Activo"
OFF_MSG "Inactivo"
REMOTE_MSG "remoto"
LOCAL_MSG "local"
DISPLAY_MSG "display"
RETURN_MSG "return"
$	SCCSID(@(#)messages	9.2	LCC)	; Modified: 15:55:17 3/28/94
$domain LCC.PCI.DOS.ON

$quote "
$ The following messages are from on.c
$ NOTE: on.c is also the source for jobs and kill

$ this is the legend bar for the jobs command
ON1 "TRAB  HOST        ESTADO  SALIR ESTADO   COMANDO\n"

$ this is the display format for the jobs command
$ if you change the order here be sure to change the
$ legend bar, ON1.
$ The values here are:
$	%1 job number
$	%2 host drive
$	%3 host name
$	%4 job state (running, sleeping, killed, etc.)
$	%5 exit status as text
$	%6 command
$ NOTE: it is unlikely that this format will need to be changed
ON2 "[%1]  %2 %3  %4 %5  %6\n"

$ %1 is the name of the current command running
ON3 "%1\n"

$ this is for detached processes. %1 is the job index.
ON4 "[%1] desasociado\n"

$ this message is used if a job cannot be run, %1 is the program
$ name (on, jobs, kill) and %2 is the command give.
ON5 "%1: %2: acceso denegado o archivo no hallado\n"

$ this is the first part of an error message, %1 is the program name
ON6 "%1: "

$ %1 is the program name
ON7 "%1: no hay procesos remotos en la tabla de tareas."


ON8 "Se�ales permitidas:\n\t"

$ this is the format for the output of each allowable signal,
$ %1 is the signal name
ON9 "%1, "

ON12 "%1: Imposible realizar la operaci�n "kill" %2\n"

$ these two messages define the user's options
$ abort check defines which characters to match
$ for each of the three options both in upper
$ and lower case
ON13 "a - abortar, c - continuar, d - desasociar: "
$ NOTE: ABORT_CHECK must have both upper and lower for each option,
$ immediately following each other or the program will behave
$ unexpectedly
ABORT_CHECK	 "AaCcDd"

ON16 "BRIDGE.DRV no est� instalado o es una versi�n incompatible"
ON18 "versi�n BRIDGE.DRV incompatible"

$ %1 is either the drive letter, the hostname or a '-'
ON19 "<%1> no es un host conectado"

$ drive %1 is not a virtual drive 
ON20 "<%1>: no es una unidad virtual"

$ program %1 received a bad numeric argument %2
ON21 "%1: argumento num�rico err�neo (%2)\n"

$ %1 is the name of the jobs program
ON22 "Uso: %1 [%%job o -]\n"

$ %1 is the name of the jobs program
ON23 "%1: tarea no encontrada"

ON24 "o un argumento num�rico.\n"

$ %1 is the name of the kill program
ON25 "%1: Debe especificar el identificador de tarea o proceso.\n"

$ %1 is the name of the jobs command, %2 is the job number or id given
ON26 "%1: tarea %2 no encontrada\n"

$ %1 is the program name, %2 is an error output, one of the ERROR
ON27 "%1\n"

$ %1 is the number of the unknown error
ON28 "Error unexec desconocido (%1)\n"

$ %1 is the name of the on program
ON29 "Uso: %1 [-se�al] %jobid\n"

$ %1 is the name of the program
ON30 "%1: tabla de tareas llena\n"

ON31 "Versi�n de DOS incompatible."

$ %1 is the name of the program
ON32 "Uso: %1 <hostcommand>\n"
ON33 "Uso: %1 <host> <comando>\n"

$ %1 is the name of the program %2 is the lcs_errno encountered in the
$ failed translation
ON34 "%1: Error de traducci�n %2.\n"

ON35 "archivo LMF \"ABORT_CHECK\" mal especificado\n"
ON36 "NO se ha podido crear archivo temporal %1\n"

$ shared usage message
ONUSAGE "       %1 /h o /? imprime este mensaje\n"

$ internal variables that need to be patched for on.c
$ NOTE: these names were placed here by hand!

$ these are the possible named signals
SIGNAME0	"Matar"
SIGNAME1	"t�rmino"
SIGNAME2	"usr1"
SIGNAME3	"usr2"

$ these are the possible jobstates
JOBSTATES0	"Desconocido"
JOBSTATES1	"Ejecutando"
JOBSTATES2	"Realizado"
	
$ these are the possible exit statuses
EXTYPES0    	"exit"
EXTYPES1	"se�al"
EXTYPES2	"coredump"
EXTYPES3	"err3"
EXTYPES4	"desconocido"

$ these are the failure error messages
ERRVEC0		"Error en servicio de red."
ERRVEC1		"No est� conectado a un host."
ERRVEC2		"Ha fallado exec con el host."
ERRVEC3		"Formato no v�lido."
ERRVEC4		"Error de asignaci�n de memoria de DOS."
$ SCCSID(@(#)messages	7.7 changed 1/28/92 19:37:36)
$domain LCC.PCI.DOS.LOGOUT
$quote "

$ The following messages are from logout.c
LOGOUT1 "Ha salido de todas las unidades virtuales.\n"

$ This is the virtual drive identifier of the host
LOGOUT2 "%1: "

$ This is the name of the host
LOGOUT3 " (%1)"

$ This indicates that the host to be logged out was not logged in
LOGOUT4 " no se ha entrado en el sistema\n"

$ The host has been logged out
LOGOUT5 " se ha salido del sistema\n"

$ This is the usage message for the logout program
LOGOUT6 "\
Uso:\n\
      %1            - todos los hosts\n\
  o  %2 <nombre-host> - host nombrado\n\
  o  %3 <unidad>:   - host conectado a la unidad\n\
  o  %4 /h o /?   - imprimir este mensaje\n\
"

$ The logout program has run out of memory
LOGOUT7 "memoria insuficiente"

$domain LCC.PCI.DOS.PCICONF
$ The following messages are from pciconf.c

$ This message is used to print an error message from pciconf
PCICONF1 "PCICONF: %1"

$ This is the usage message 
PCICONF2 "PCICONF: sintaxis:\n\
    pciconf set nombre_testigo [on|off][remote|local]\n\
    pciconf display [nombre_testigo]\n\
    pciconf return nombre_testigo\n\
    pciconf /h o /?  imprime este mensaje\n"

$ These messages are the commands that may be given as arguments to pciconf
PCICONF3 "set"
PCICONF4 "display"
PCICONF5 "return"

$ These messages are the names of values the options can have
VAL_ON     "Activo"
VAL_OFF    "Inactivo"
VAL_LOCAL  "local"
VAL_REMOTE "remoto"

$ These messages are the token-names
$ (they will be truncated to 14 characters)
O_NETBIOS "netbios"
O_NETDR   "vdrive"
O_5F02    "5f02"
O_LPTBIN  "lpt_binary"

$ This messages precedes the list of possible token names 
PCICONF6 "\nLos nombres de testigo posibles son:\n"

$ This message formats the possible tokens
PCICONF7 "\t%1\n"

$ This tells the user that he has selected an invalid option (on/off are valid)
PCICONF8 "debe usarse on|off.\n"

$ This tells the user that he has selected an invalid option 
$ (remote/local are valid)
PCICONF9 "debe usarse remote|local\n"
PCICONF10 "la memoria o el archivo del programa est� deteriorado\n"
PCICONF12 "nombre de testigo desconocido.\n"

$ these are debug messages only
PCICONF15 "depuraci�n activado\n"
PCICONF16 "CONJUNTO\n"
PCICONF17 "DISPLAY <NOMBRE>\n"
PCICONF18 "RETURN\n"
PCICONF19 "Error: acci�n no v�lida: %1\n"

$ These are used to list the values of local/remote drive and nbs,nd on/off 
PCICONF22 "%1: %2\n"
PCICONF23 "Activo"
PCICONF24 "Inactivo"
PCICONF25 "remoto"
PCICONF26 "local"

$domain LCC.PCI.DOS.PCIINIT
$ The following messages are from pciinit.c
PCIINIT1 "no se puede encontrar el dispositivo BRIDGE correcto\n"

$ The following shows an initialization error and the associated errno
$ PCIINIT2 is a discontinued message
PCIINIT2 "error en inicializaci�n $ %1\n"

PCIINIT_BAD_DOS     "error: no inicializando, DOS irreconocible\n"
PCIINIT_BAD_BRIDGE  "error: no inicializando, BRIDGE.DRV en mal estado\n"
PCIINIT_DRIVER_VER  "advertencia: versi�n incorrecta del controlador de red\n"
PCIINIT_DRIVER_INIT "advertencia: Fall� la inicializaci�n del controlador de red\n"
PCIINIT_NO_NET \\x13
      "error: no inicializando, no hay interfaz de red o puertos RS232\n"

PCIINIT_FOR_NET   "para su uso en la interfaz de red\n"
PCIINIT_FOR_RS232 "para su uso en puertos RS232\n"
PCIINIT_FOR_BOTH  "para su uso en la interfaz de red y puertos RS232\n"

PCIINIT_DRIVES    "Las unidades de PC-Interface son del %1 a %2\n"
PCIINIT_NETWARE_DRIVES "La primera unidad NetWare es %1\n"

PCIINIT3 "advertencia: no se puede definir el nombre de la m�quina\n"

$ An improperly formed (one without a leading '-' or '/' ) was given.
PCIINIT4 "argumento err�neo <%1>\n"

$ An invalid option was given
PCIINIT5 "conmutador desconocido <%1>\n"

$ The broadcast address given is not in the proper format 
PCIINIT6 "direcci�n de transmisi�n no v�lida <%1>\n"

$ there was no 'localhost' internet address found for the PC
PCIINIT7 "'%1' entrada no encontrada\n"

$ the localhost address is not in the proper format or invalid
PCIINIT8 "direcci�n local no v�lida <%1>\n"

$  The address does not conform to the internet Class A, B or C values
PCIINIT9 "Clase de direcci�n no v�lida\"%1\"\n"

$ Both the EXCELAN and the HOSTS environment variables must agree
PCIINIT10 "\"HOSTS\" y \"EXCELAN\\tcp\" deben coincidir\n"

PCIINIT11 "no se puede abrir %1\n"
PCIINIT12 "la versi�n del Bridge debe ser superior a 2.7.2\n"

$ The next two messages refer to the HOSTS file
PCIINIT13 "no se puede abrir archivo %1.\n"
PCIINIT14 "%1 est� da�ado o no es v�lido.\n"


PCIINIT15 "unidad remota: %1\n"
PCIINIT16 "Versi�n %1.%2.%3 (N�m. de serie %4.%5) Inicializada\n"

$ this prints the ip address
PCIINIT17 "%1"
PCIINIT18 "Direcci�n IP = "
PCIINIT19 "\nDirecci�n de transmisi�n = "

$ This is the program name
PCIINIT21 "pciinit: "

$ This is the product name
PCIINIT24 "PC-Interface "

$ This is printed if pciinit has already been run
PCIINIT25 "ya inicializada"

$ This warns the user to run PCIINIT without the /e option to initialize the
$ software.
PCIINIT26 "Advertencia: PC-I no ha sido inicializado.\n\
Ejecutar PCIINIT sin la opci�n /e para inicializar PC-I.\n"

$ The subnet mask is not in the proper format 
PCIINIT27 "m�scara subnet no v�lida<%1>\n"

$ The subnet mask is not in the proper format 
PCIINIT28 "m�scara subnet no v�lida<%1.%2.%3.%4>\n"

$ Netware is already installed
PCIINIT30 "no se puede inicializar despu�s de que Netware haya sido inicializado\n"

$ usage message for pciinit
PCIINIT_U "\
uso: pciinit [/i<host>] [/b<host>] [/s<m�scara>] [<directorio_serie>]\n\
       donde <host>    es un nombre o una direcci�n IP\n\
       donde <m�scara> es un nombre o una m�scara de subred\n\
       pciinit /e         imprime el par�metro actual de direcci�n de red\n\
       pciinit /h o /?    imprime este mensaje\n"

$ for bridge versions prior to release 3.2
PCIINIT_OLD_BRIDGE "advertencia: versi�n antigua de bridge.drv\n"

$ This is an error found in the bridge
ERROR_MSG "Error - "

$ This is used to indicate that a particular host did not respond
GEN_ERROR"no hay respuesta. �Reintentar? (S o N) : "


DISC_MSG "La sesi�n de PC-Interface ha terminado. Entre en el sistema para reintentar."
ERR_MSG	"ERROR DE PARAMETRO\r\n$"
DRV_MSG	 " unidades virtuales y $"
JOBS_MSG " entradas de tablas de tareas\r\n\n$"

$ The copy protection violation message
VIOLATION "Violaci�n de protecci�n de copias de PC Interface - SISTEMA DESACTIVADO"


$domain LCC.PCI.DOS.PRINTER
$ The following messages are from printer.c.
$
$ These tokens are the words that the user types to select
$ remote/local printer operation.
PCI_REMOTE_TOKEN "remoto"
PCI_LOCAL_TOKEN "local"
MRG_REMOTE_TOKEN "unix"
MRG_LOCAL_TOKEN "DOS"
AIX_REMOTE_TOKEN "aix"

$ the DOS call to set the printer state failed
PRINTER1 "printer: no se pudo definir el estado.\n"

$ the printer name given is not in the range allowed or is non-numeric
PRINTER2 "%1: Nombre de flujo de impresi�n err�nea.\n"

$ This message says that you can't reset the printer program
$ when you are just setting it
PRINTER3 "%1 no es v�lido al definir programa de impresora\n"


PRINTER4 "Las opciones /P y /D entran en conflicto\n"

$ The following 5 thru 8 are the usage message for the multistream
$ version of PC-Interface, and the Merge version.
$ The PCI version uses PRINTER7 instead of PRINTER7_M.
$ The Merge version uses PRINTER7_M instead of PRINTER7.
PRINTER5 "uso: \nimpresora\n"
$ %1 is the 'local' token, which is "local" in PCI and "dos" in Merge.
PRINTER6 "printer [LPTn] %1\n"
PRINTER7 "printer [LPTn] {host|unidad|-} [programa_impresi�n|/R] [/X[0|1]]\n        [/T[tiempo_l�mite]]\n"
$ %1 is the 'remote' token, for Merge is "unix" (or "aix", on an AIX machine).
PRINTER7_M "printer [LPTn] %1 [programa_impresi�n|/R] [/X[0|1]] [/T[tiempo_l�mite]]\n"
PRINTER8 "printer [LPTn] [/P|/D] [/X[0|1]] [/T[tiempo_l�mite]]\n"
PRINTER_HELP "printer /h o /? imprime este mensaje\n"

$ The following 9 and 10 are the usage message for the non-multistream
$ version of PC-Interface
PRINTER9 "uso: printer {%1}\n"
PRINTER10 "               {%1} [/Tn]\n"

PRINTER11 "Imposible definir los par�metros de la impresora (LPT%1)\n"
PRINTER12 "Imposible definir LPT%1 como remota. Probablemente no se ha entrado en el sistema.\n"
PRINTER13 "S�lo LPT1 est� soportada con su versi�n del puente.\n"
PRINTER14 "printer: no se pudo definir el estado.\n"
PRINTER15 "printer: no se pudo obtener el estado.\n"
PRINTER16 "La opci�n %1 no est� soportada con su versi�n del puente\n"
PRINTER22 "LPT%1: "


$ This message prints out which drive is associated with which host
PRINTER23 " (%1: %2)"

$ This message prints out the drive of the host for systems that support
$ only one host
PRINTER24 " (%1:)"

$ the following messages, PRINTER25 through PRINTER33 are used
$ to create an informational display for each printer
$ the format of this message is
$ 	LPT#: [local|remote] , print on exit [on|off], 
$	[no timeout | timeout = # ], [default printer | <printprog>]
PRINTER25 ", imprimir al salir "
PRINTER26 "Activo"
PRINTER27 "Inactivo"
PRINTER28 ", tiempo l�mite = %1"
PRINTER29 ", sin tiempo l�mite"
PRINTER30 ",\n      "
PRINTER31 ", "
PRINTER32 "\"%1\"\n"
PRINTER33 ", impresora por defecto\n"

$ this says whether the printer's state is local or remote
PRINTER34 "Estado actual de la impresora: %1"

PRINTER35 " con tiempo l�mite de %1 segundos."
PRINTER36 " sin tiempo l�mite."

$ This message indicates that a particular drive is not connected
PRINTER37 "%1: est� sin conectar"

$ This message is self-explanatory.
PRINTER38 "Memoria insuficiente.\n"

$domain LCC.PCI.DOS.UDIR
$ The following messages are from udir.c

$ %1 is drive number
UDIR1 " Volumen en unidad %1 "
UDIR2 "no tiene etiqueta\n"

$ %1 is the drive label 
UDIR3 "es %1\n"

$ %1 is the path name
UDIR4 " Directorio de %1\n"

$ This is the format of the udir output line
$ %1 is the UNIX side name, %2 is the mapped DOS file name,
$ %3 is the owner's name, %4 is the file attributes
UDIR5 "%1 %2%3%4"

$ This is the message that the file is a directory
UDIR7 "<DIR>    "

$ This completes the output line with date (%1) followed by time (%2)
UDIR9 "%1%2\n"

UDIR14 "\narchivo no encontrado\n"

$ %1 is the number of files found, %2 is for spacing, %3 is the number
$ of bytes found
UDIR15 "\t%1 archivo(s)%2%3 octetos libres\n"

$ the drive given is invalid
UDIR17 "Especificaci�n de unidad no v�lida\n"

$ udir usage message
UDIR18 "\
uso: udir [/a] [letra_unidad:][v�a_acceso][directorio|archivo]\n\
     udir /h o /?    - imprime este mensaje\n"


$ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
$ ! 	NOTE - convert.c and getopt.c messages are		!
$ !	 stored on the UNIX side in util			!
$ !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

$domain LCC.PCI.DOS.DOSWHAT

DOSWHAT2 "Uso: %1 [/f] [/w] archivo [archivo] ...\n\
     %1 /h o /? imprime este mensaje\n"

$ %1 is the file name
DOSWHAT3 "%1: No se puede encontrar %2\n"
DOSWHAT5 "%1:\n\tDirectorio\n"
DOSWHAT7 "%1:\n\tDispositivo\n"
DOSWHAT8 "%1: No se puede abrir %2\n"
DOSWHAT9 "%1: No se puede fstat %2\n"
DOSWHAT11 "%1:\n\tDispositivo\n"

DOSWHAT14 "Tipo de archivo sin determinar: intente la instrucci�n de archivos de UNIX.\n"

DOSTYPE0 " Archivo binario\n"
DOSTYPE1 " archivo de texto ascii de DOS\n"
DOSTYPE2 " archivo de texto ascii de UNIX\n"
DOSTYPE3 " Directorio\n"
DOSTYPE4 " Dispositivo\n"

$domain LCC.PCI.DOS.PCIDEBUG
$quote "
$ The following messages are from pcidebug.c
$ %1 is the program name - pcidebug
PCIDEBUG1 "%1: Uso: %1 <host|unidad> <[=+-~]chanList|on|off|close> [...]\n\t\t\t\t\tchanList = chanNum1[,chanNum2[...]]\n\
\t\t %1 /h o /?  imprime este mensaje\n"

$ %1 is the program name, %2 is the argument
PCIDEBUG2 "%1: Argumento no v�lido: \"%2\"\n"

$ %1 is the program name, %s is the bit value
PCIDEBUG3 "%1: Bit %2 est� fuera de rango\n"

$ %1 is the program name, %2 is the ioctl value
PCIDEBUG4 "%1: Error en ioctl %2\n"

$ the next three are tokens 
$ if they are changed, change them in PCIDEBUG1 to agree
PCIDEBUG10 "Inactivo"
PCIDEBUG11 "Activo"
PCIDEBUG12 "cerrar"


$ The following messages are from vdrive.c
$domain	LCC.PCI.DOS.NLSVD
$quote "

USEAGENODRIVE "No se ha entrado actualmente en ninguna unidad virtual PCI.\n"
NODRIVE "No se ha entrado actualmente en ninguna unidad virtual PCI.\n"
BADDRIVE "Especificador de unidad no v�lida - %1\n"
USEAGEBADDRIVE "Especificador de unidad no v�lida - %1\n"
USEAGENOTADRIVE "La unidad %1 no es una unidad virtual.\n"
FATAL "Error fatal al obtener el nombre del host.\n"
CONNECT "%1: est� conectado a %2.\n"

$ SCCSID("@(#)messages	6.1	LCC")	/* Modified: 10/15/90 15:48:13 */
$domain LCC.PCI.DOS.CONVERT
$quote "
$ NOTE: '\n' indicates that a new line will be printed
$ The following messages are from convert.c
CONVERT1 "May�sculas y Min�sculas AMBAS especificadas\n"
CONVERT1A "Las opciones mencionadas son incompatibles\n"
CONVERT2 "unix2dos y dos2unix AMBOS especificados\n"

$ %1 is the file name on which a read error occured
CONVERT3 "Se ha producido un error al leer %1\n"

$ %1 is the fiel name which cannot be opened
CONVERT4 "No se puede abrir %1\n"

$ input file %1 and output file %2 are identical
CONVERT5 "archivos %1 y %2 son id�nticos\n"

$ an error was encountered writing file %1
CONVERT6 "Se ha producido un error al escribir %1\n"

CONVERT7 "No se ha proporcionado tabla de traducci�n de salida\n"

CONVERT8 "Superior/Inferior especificado sin traducci�n\n"

$ translation table %1 cannot be opened
CONVERT10 "No se puede abrir tabla de traducci�n %1\n"

CONVERT15 "�Tabla de traducci�n no v�lida!\n"
CONVERT17 "No se ha proporcionado tabla de traducci�n de entrada\n"
CONVERT21 "�Fallo de la escritura en salida!\n"
CONVERT30 "�Imposible asignar espacio para el buffer de traducci�n!\n"
CONVERT31 "�Tablas de traducci�n no definidas!\n"

$ character %1 was untranslatable with the options used
CONVERT32 "\nCar�cter intraducible en la l�nea # %1\n"

$ unknown error %1 occurred
CONVERT42 "Error de traducci�n desconocido: %1\n"

CONVERT45 "�Tabla(s) de traducci�n no encontrada(s)!\n"
CONVERT46 "�Tabla(s) de traducci�n no v�lida(s)!\n"
CONVERT60 "Variable de entorno COUNTRY no definida, utilizando 1\n"

$ code page %1 will be used as no other was specified
CONVERT61 "Variable de entorno CODEPAGE no definida, utilizando %1\n"

CONVERT77 "No se han proporcionado tablas de traducci�n de entrada y salida\n"
CONVERT80 "�Advertencia! Car�cter de 8 bits\n"
CONVERT86 "�No se puede encontrar memoria para procesar CONVOPTs!\n"
CONVERT90 "�No se pudo asignar m�s memoria!\n"
CONVERT_B1 "No se puede realizar el mejor car�cter �nico con juego de caracteres por defecto\n"
CONVERT_S1 "\nInformaci�n de traducci�n:\n"

$ %1 is the number of glyphs
CONVERT_S2 "Glifos traducidos exactamente:\t\t%1\n"
CONVERT_S3 "Glifos traducidos a m�ltiples octetos:\t%1\n"
CONVERT_S4 "Graf�as traducidos a valores de usuario por defecto:\t%1\n"
CONVERT_S5 "Glifos traducidos al mejor glifo �nico:\t%1\n"
CONVERT_S6 "\nN�mero total de glifos procesados:\t%1\n"

$ %1 is the number of bytes, %2 is the number of lines in the text file
CONVERT_S7 "\n%1 octetos procesados en %2 l�neas\n"

$ %1 is the name of the program
CONVERT_M1_D "uso: %1   [/opciones] ... [entrada [salida]]\n"
CONVERT_M3_D "Las opciones incluyen: /u      Poner archivo en may�sculas.\n\
\                      /l      Poner archivo en min�sculas.\n\
\                      /f      Forzar (conmutador old).\n\
\                      /b      Binario (conmutador old).\n"
CONVERT_M4_D "\                /7      Emitir advertencia si alg�n car�cter usa el 8� bit.\n\
\                /x      Directo.  No traducir.\n\n\
\                /i tbl  Traducir la entrada usando la tabla tbl.\n\
\                /o tbl  Traducir la salida usando la tabla tbl.\n\
\n"
CONVERT_M5_D "\                /c c    Usar c como car�cter_de_usuario para fallo de\n\
\                        traducci�n..\n\
\                /m      Permitir traducciones de m�ltiples caracteres.\n\
\                /a      Abortar si falla la traducci�n.\n\
\                /s      Usar la traducci�n del mejor car�cter �nico.\n\
\                /z      Manejar correctamente C-Z (dos2unix/unix2dos).\n\
\n"
CONVERT_M6_D "\                /d      Llevar a cabo retorno de carro como salto de l�nea en la traducci�n.\n\
\                /p      Llevar a cabo saltos de l�nea para realizar retornos de carro.\n\
\n"
CONVERT_M7_D "\                /q      No mostrar mensajes de advertencia.\n\
\                /v      Mostrar mensajes de advertencia y estad�sticas de traducci�n.\n\
\                /h o /? Imprimir este mensaje.\n\
"


CONVERT_M1_U "uso: %1   [-opciones] ... [entrada [salida]]\n"
CONVERT_M3_U "Las opciones incluyen: -u      Poner archivo en may�sculas.\n\
\                      -l      Poner archivo en min�sculas.\n\
\                      -f      Forzar (conmutador old).\n\
\                      -b      Binario (conmutador old).\n"
CONVERT_M4_U "\                -7      Emitir advertencia si alg�n car�cter usa el 8� bit.\n\
\                -x      Directo.  No traducir.\n\n\
\                -i tbl  Traducir la entrada usando la tabla tbl.\n\
\                -o tbl  Traducir la salida usando la tabla tbl.\n\
\n"
CONVERT_M5_U "\                -c c    Usar c como car�cter_de_usuario para fallo de\n\
\                        traducci�n..\n\
\                -m      Permitir traducciones de m�ltiples caracteres.\n\
\                -a      Abortar si falla la traducci�n.\n\
\                -s      Usar la traducci�n del mejor car�cter �nico.\n\
\                -z      Manejar correctamente C-Z (dos2unix/unix2dos).\n\
\n"
CONVERT_M6_U "\                -d      Llevar a cabo retorno de carro como salto de l�nea en la traducci�n.\n\
\                -p      Llevar a cabo saltos de l�nea para realizar retornos de carro.\n\
\n"
CONVERT_M7_U "\                -q      No mostrar mensajes de advertencia.\n\
\                -v      Mostrar mensajes de advertencia y estad�sticas de traducci�n.\n\
\                -h o -? Imprimir este mensaje.\n\
"

$ %1 is the filename, %2 is the options
CONVERT_HELP00_D "uso: %1 [/%2] [archivo_entrada [archivo_salida]]\n"
CONVERT_HELP00_U "uso: %1 [-%2] [archivo_entrada [archivo_salida]]\n"
CONVERT_HELP01_D "     %1 -h o -? para ayuda m�s detallada\n"
CONVERT_HELP01_U "     %1 -h o -? para ayuda m�s detallada\n"
CONVERT_HELP1_D "uso: %1 /%2 [archivo_entrada [archivo_salida]]\n"
CONVERT_HELP1_U "uso: %1 -%2 [archivo_entrada [archivo_salida]]\n"

$ The following messages are from getopt.c
$ Do NOT change of the order of %1 and %2!
GETOPT1 "Opci�n desconocida %1%2\n"
GETOPT2 "Falta argumento para la opci�n %1%2\n"
$ SCCSID("@(#)messages	5.1	LCC")	/* Modified: 6/3/91 13:46:54 */
$quote "
$domain LCC.PCI.DOS.HOSTDRV
$ The following messages are from hostdrv.c
$ %1 is the drive letter given
HOSTDRV1 "%1: no es una unidad virtual"
HOSTDRV2 "%1 no es un host conectado"

$domain LCC.PCI.DOS.HOSTOPTN
$ The following messages are from hostoptn.c
HOSTOPTN1 "opci�n(es) de selecci�n del host no v�lida(s)"
