$ SCCSID(@(#)dos.msg	1.5 LCC) Merge 3.2.0 Branch Modified 18:17:50 11/9/94
$ SCCSID(@(#)dos.msg	1.74 LCC) Modified 21:46:23 11/2/94
$ English message file for Merge UNIX utility programs:
$	dosexec (dos)
$	dosopt
$	xdosopt
$	crt
$	tkbd
$	xcrt
$	display
$	romsnap
$	newunix
$	pssnap
$	swkey
$	x_msg
$	UI library
$	xmrgconfig
$	mrgconfig
$	mrgadmin
$	admconvert
$
$quote "
$domain LCC.PCI.UNIX.JANUS

UNKNOWN "Mensaje desconocido:\n"
ERROR	"ERROR:%1 %2 %3 %4 %5 %6\n"
WARNING	"ADVERTENCIA:\n"
$	-	dosexec
AOPT_BAD "AOPT_BAD: %1: ERROR: Uso de la opci�n 'a' inadecuada '%2=%3'\n"
$	-	dosexec
$	-	dosopt
AOPT_TOOMANY "AOPT_TOOMANY: %1: ERROR: Demasiados dispositivos asignados.\n\
No se puede continuar.\n"
$	-	crt
$	-	tkbd
BAD_ARGS "BAD_ARGS: %1: ERROR: Se pasaron par�metros inadecuados a este programa.\n"
$	-	dosexec
BAD_IMG "BAD_IMG: %1: ERROR: El archivo de imagen '%2' no es del tipo esperado.\n"
$	-	crt
$	-	tkbd
BAD_LCS_SET_TBL "BAD_LCS_SET_TBL: %1: ERROR: Tabla de conjunto de c�digo err�nea.\n\
\   No se puede utilizar tabla %2 o %3. Error=%4."
$	-	dosopt
BAD_OPT_USAGE "BAD_OPT_USAGE: %1: ERROR: La opci�n '%2' no se usa correctamente.\n"
$	-	dosexec
BAD_SWD "BAD_SWD: %1: ERROR: Fallo de comunicaci�n UNIX-DOS.\n"
$	-	dosexec
BOPT_ERR "BOPT_ERR: %1: ERROR: 'dos +b' no est� permitido.\n"
$	-	dosexec
CD_X "CD_X: %1: ERROR: Fallo en 'cd' inicial en unidad %2.\n\
Directorio que no pudo cambiar a: %3%4\n"
$	-aix	pssnap
$	-	romsnap
CHOWN_ERR "CHOWN_ERR: %1: ERROR:  No se puede hacer chown del archivo '%2', error= %3.\n"
$	-	dosexec
CL_TOOLONG "CL_TOOLONG: %1: ERROR: L�nea de comando demasiado larga.\n"
$	-	dosexec
CODEPAGE_NS "CODEPAGE_NS: %1: ERROR: Traducci�n de DOS CODEPAGE '%2' no soportada.\n\
\    (CODEPAGE archivo '%3/%4' no encontrado.  err=%5)\n"
$	-	dosexec
CODESET_NS "CODESET_NS: %1: ERROR: Traducci�n de definici�n de c�digo UNIX '%2' no soportada.\n\
\    (archivo de conjunto de c�digo '%3/%4' no encontrado.  err=%5)\n"
$	-	dosexec
CODESET_USE "\n\
Uso de definicion de codigo '%2' en su lugar.\n"
$	-	dosexec
CONFIG_NF "CONFIG_NF: %1: ERROR: No se puede usar el archivo config.sys %2.\n\
No se pudo abrir para lectura.\n"
$	-	newunix
CONS_ONLY "CONS_ONLY: %1: ERROR: S�lo puede ejecutarse en la consola.\n"
$	-	romsnap
$	-	dosopt
CREATE_ERR "CREATE_ERR: %1: ERROR: Imposible crear archivo '%2', error = %3.\n"
$	-	dosopt
CRT_DISPLAY "CRT_DISPLAY: %1: ERROR: No se puede usar tipo de visualizaci�n '%2' con 'crt'.\n"
$	-	dosexec
CWD_TOOLONG "CWD_TOOLONG: %1: ERROR: El directorio de trabajo actual es demasiado largo %2.\
\nMax=%3.\n"
$	-	dosexec
DA_FAIL "DA_FAIL: No se puede a�adir directamente '%2'.  Error en dispositivo '%3'.\n"
$	-	dosexec
DEV_NA "DEV_NA: %1: ERROR: Dispositivo requerido %2 no disponible.\n"
$	-	dosexec
DEV_TOOMANY "DEV_TOMANY: %1: ERROR: Demasiados dispositivos agregados.\n"
$	-	dosopt
DFLTS "* opciones por defecto de %2.\n"
$	-	dosopt
NO_OPTIONS "Ninguna opci�n en %1.\n"
$	-	dosexec
DINFO_BYTES "Bytes"
DINFO_CGA "CGA"
DINFO_COM1 "COM1"
DINFO_COM2 "COM2"
DINFO_DIRECT "directo"
DINFO_DOS_DIR "DOS (directo)"
DINFO_DOSPART "Particiones de DOS:"
DINFO_EGA "EGA"
DINFO_RAM "RAM"
DINFO_EMS "EMS"
DINFO_FDRIVES "Unidades de disquete:"
DINFO_HERC "HERC"
DINFO_LPT "LPT"
DINFO_MONO "MONO"
DINFO_MOUSE "Rat�n"
DINFO_NONE "ninguno"
DINFO_OTHER "OTRAS"
DINFO_PMSG1 "Informaci�n de dispositivo DOS"
DINFO_PMSG2 "Pulse <espacio> para continuar"
DINFO_TTY "TTY"
DINFO_UD_SHARED "UNIX/DOS compartidos:"
DINFO_UNIX_DEF "UNIX por defecto"
DINFO_UNKNOWN "desconocido"
DINFO_USIZE "tama�o desconocido"
DINFO_VGA "VGA"
DINFO_VIDEO "V�deo"
DINFO_2BUTTON "Rat�n de 2 botones"
$	-	display
DISPLAY_USAGE	"Uso:  display [nombre_dispositivo]\n\
\    Si no se especifica nombre_dispositivo, la opci�n por defecto es est�ndar.\n"
$	-	dosexec
DMA_NA "DMA_NA: %1: ERROR: No se puede asignar canal dma %2\n"
$	-	dosexec
DOSDEV_ERR "DOSDEV_ERR: %1: ERROR: La l�nea %2 del archivo de definici�n de dispositivo\n\
'%3'\n\
est� mal:\n\
%4\n"
$	-	dosexec
DOSPART_TWOSAME "\
%1: Advertencia: Est� tratando de interconectar la misma partici�n de\n\
DOS como dos letras de unidad, %2 y %3, con acceso DE ESCRITURA.\n\
Esto no est� permitido.  Se continuar� con el acceso DE SOLO LECTURA\n\
para esas unidades.\n"
$	-	dosexec
DPOPT_ERR "DPOPT_ERR: %1: ERROR: No se puede ejecutar autom�ticamente autoexec.bat\
\  en unidad de disco %2.\n\
(Opciones +p y +d%2 no est�n permitidas al mismo tiempo)\n"
$	-	dosexec
DRIVE_LETTER "DRIVE_LETTER: %1: ERROR: Letra de unidad no v�lida %2\n"
$	-	dosexec
EM_FAIL "EM_FAIL: %1: ERROR: Fallo al inicializar el emulador.\n"
$	-	dosexec
EM_VER "EM_VER: %1: ERROR: Instalada una versi�n incorrecta del emulador.\n"
$	-	dosexec
ENV_NOTSET "ENV_NOTSET: %1: ERROR: Se necesita variable de entorno '%2'\
\  no est� configurada.\n"
$	-	dosexec
ENV_TOOMUCH "ENV_TOOMUCH: %1: ERROR: Entorno demasiado largo %2. m�x.=%3.\n"
$	-	crt
ERR_SEMAPHORE "ERR_SEMAPHORE: %1: ERROR: Accediendo a sem�foro.\n"
$	-	dosexec
EXEC_CMDCOM "EXEC_CMDCOM: %1: ERROR: Fallo en ejecuci�n de command.com.\n"
$	-	dosexec
EXEC_ERR "EXEC_ERR: %1: ERROR: %2 no ha sido instalado correctamente.\n"
$	-	dosexec
EXEC_FAILED "EXEC_FAILED:  %1:  ERROR:  Fallo en la ejecuci�n de %2, error = %3.\n"
$	-	dosexec
EXIT_SIGNAL "EXIT_SIGNAL: %1: ERROR: Se�al %2 captada.\n"
$	-	dosexec
FATAL "FATAL: Error fatal en %1:\n%2\n"
$	-	dosopt
FILE_OPTS_ERR "FILE_OPTS_ERR: %1: ERROR: Errores en las operaciones de archivos.\n"
$	-	dosopt
FN "\nArchivo: %2\n"
$	-	dosopt
FN_TOOLONG "FN_TOOLONG: %1: ERROR: Nombre de archivo demasiado largo: %2. m�x.=%3.\n"
$	-	dosexec
FORK_FAIL "FORK_FAIL: %1: ERROR: Fallo de horquilla interna.  Error=%2\n\
(Demasiados procesos en ejecuci�n o memoria insuficiente.)\n"
$
FORK_EXEC_FAIL "FORK_EXEC_FAIL: %1: ERROR: Fallo de horquilla interna. Error=%2\n\
(Demasiados procesos en ejecuci�n o memoria insuficiente.)\n\
No se puede ejecutar %3\n"
$	-	dosexec
IA_FAIL "IA_FAIL: '%2' no est� disponible.\n\
Dispositivo '%3' est� en uso o tiene autorizaci�n incorrecta.\n"
$	-	dosexec
$	-	dosopt
ILL_DRV "ILL_DRV: %1: ERROR: Unidad de disco '%2' no v�lida.\n"
$	-	dosexec
$	-	dosopt
ILL_MX "ILL_MX: %1: ERROR: Selector de memoria no v�lido '%2'.\n"
$	-	dosexec
$	-	dosopt
ILL_OPT "ILL_OPT: %1: ERROR: Opci�n '%2' no v�lida.\nUse +h para obtener lista de opciones.\n"
$	-	dosexec
IMPROPER_INSTL "IMPROPER_INSTL: %1: ERROR: Instalaci�n incorrecta.\n\
\  Los permisos o privilegios no son correctos.\n"
$	-	dosexec
INITDEV_FAIL "INITDEV_FAIL: %1: ERROR: No se pudo inicializar '%2=%3'.\n"
$	-	dosexec
INIT_FAIL "INIT_FAIL: %1: ERROR: No se pudo inicializar el dispositivo '%2'.\n"
$	-	dosexec
INTERNAL_ERR "INTERNAL_ERR: %1: ERROR: Error interno. Fallo en %2. %3 %4 %5 %6\n"
$	-	dosexec
MEMORY_ERR "MEMORY_ERR: %1: ERROR: Memoria insuficiente.\n"
$	-	dosexec
INVALID_DEV "INVALID_DEV: %1: ERROR: %2 no es un dispositivo v�lido. (%3)\n"
$	-	dosexec
INVALID_TMM "INVALID_TMM: %1: ERROR: %2 no es un dispositivo v�lido.\n\
(Alcance de E/S en mapa de memoria %3-%4 no v�lido)\n"
$	-	dosexec
INVALID_MEMMAP "INVALID_MEMMAP: %1: ERROR: Enlace directo no v�lido\n\
rango de E/S en la asignaci�n de memoria %2-%3\n"
$	-	dosexec
BAD_PRINTER_CMD "BAD_PRINTER_CMD: Comando de impresora no v�lido para %s.\n"
$	-	crt
$	-	xcrt
IOCTL_FAIL "IOCTL_FAIL: %1: ERROR: Ioctl %2 en dispositivo %3 ha fallado. Error=%4.\n"
$	-	dosexec
KILL_FAIL "KILL_FAIL: %1: ADVERTENCIA: problemas que detienen proceso de pantalla.\n"
$	-	dosexec
LOADDEV "LOADDEV: %1: ERROR: Error en loaddev.\n"
$	-	dosexec
LOAD_FAIL "LOAD_FAIL: %1: ERROR: Fallo en carga de archivo de imagen '%2'.\nError = %3.\n"
$	-	dosexec
MAX_USER "No se puede arrancar otra sesi�n DOS/Windows porque actualmente est�n \n\
usando el sistema el n�mero m�ximo de usuarios (%2) de DOS/Windows.\n"
$	-	dosexec
MEM_NA "MEM_NA: %1: ERROR: No se puede asignar memoria %2\n"
$	-	dosexec
$	-	dosopt
MEM_OOR "MEM_OOR: %1: ERROR: La memoria requerida est� fuera de rango. M�x=%2 M�n=%3.\n"
$	-	dosexec
MERGE_NA "MERGE_NA: %1: ERROR: La fusi�n (Merge) no est� configurada en su sistema.\n\
\  DOS no se puede ejecutar.\n"
$	-	dosexec tkbd ...
MERGE_DEV_NA "MERGE_DEV_NA: %1: ERROR: Imposible acceder a las llamadas al sistema de Merge.\n"
$	-	newunix
NEWUNIX_USAGE "\nUso:  %2 [-n n�mero_vt] [-p] [[-e] comando]\n"
$	-	dosexec
NO_DOS "NO_DOS: %1: ERROR: DOS no est� instalado.\n\
\  Hay que ejecutar 'dosinstall' para instalar DOS antes de usar Merge.\n"
$	-	dosexec
NO_PATH "NO_PATH: %1: ERROR: La v�a de acceso UNIX no est� definida.\n"
$	-	dosexec
NO_LICENCE "NO_LICENCE: %1: No hay licencia disponible para ejecutar ahora DOS/Windows.\n"
$	-	dosexec
NO_USERS "NO_USERS: %1: Se cerraron los servicios de DOS/Windows Merge.\n\
En este momento no se permite el uso de DOS/Windows.\n"
$	-	dosexec
$	-	crt
$	-	tkbd
$	-	dosexec
NOT_DEV "NOT_DEV: %1: ERROR: %2 (%3) no es un dispositivo.\n"
$	-	dosexec
$	-	dosopt
NOT_DOS_PROG "NOT_DOS_PROG: %1: ERROR: '%2' no es un programa DOS.\n"
$	-	dosopt
NOT_RW_ERR "NOT_RW_ERR: %1: ERROR: Imposible abrir archivo '%2'.\n\
\  Archivo no existe o no admite lectura/escritura por el usuario.\n"
$	-	dosopt
NOT_RW_WARN "NOT_RW_WARN: %1: ADVERTENCIA Imposible abrir archivo '%2'.\n\
\  Archivo no existe o no admite lectura/escritura por el usuario.\n"
$	-	dosexec
$	-	dosopt
$	-	display
$	-aix	pssnap
$	-	romsnap
NOT_R_ERR "NOT_R_ERR: %1: ERROR: No se puede abrir archivo '%2'.\n\
\  Archivo no existe o no admite lectura por el usuario.\n"
$	-	dosopt
NOT_R_WARN "NOT_R_WARN: %1: ADVERTENCIA Imposible abrir archivo '%2'.\n\
\  Archivo no existe o no admite lectura por el usuario.\n"
$	-	dosopt
NOT_W_ERR "NOT_W_ERR: %1: ERROR: Imposible abrir archivo '%2'.\n\
\  Archivo no existe o no admite escritura por el usuario.\n"
$	-	dosexec
NO_BG_CMD "NO_BG_CMD: %1: ERROR: No se puede ejecutar programa DOS orientado a visualizaci�n\n\
\         en segundo plano.  Use 'dos ...' en lugar de 'dos ... &'\n"
$	-	dosexec
NO_BG_DOS "NO_BG_DOS: %1: ERROR: No se puede redirigir o ejecutar\n\
\  una sesi�n DOS en segundo plano.\n"
$	-	dosopt
NO_FILENAME "NO_FILENAME: %1: ERROR: Debe tener nombre de archivo.\n"
$	-	crt
NO_GRAPHICS "\
\r\n
\          +============================+ \r\n\
\          |                            | \r\n\
\          |          NO HAY            | \r\n\
\          |         GRAFICOS           | \r\n\
\          |            EN              | \r\n\
\          |       TERMINAL ASCII       | \r\n\
\          |                            | \r\n\
\          |    Salga de modo grafico   | \r\n\
\          |                            | \r\n\
\          +============================+ \r\n\
\r\n\
\n"
$	-	dosexec
NO_INIT_DATA "NO_INIT_DATA: %1: ERROR: No se puede obtener datos de inicializaci�n.\n"
$	-	dosexec
NO_MORE "NO_MORE: %1: ERROR: No queda m�s memoria. Error=%2\n"
$	-	dosexec
$	-	newunix
NO_RESOURCES "NO_RESOURCES: %1: ERROR: No hay m�s recursos disponibles.\n"
$	-	newunix
NO_VT "NO_VT: %1: ERROR: vt no disponible.\n"
$	-	dosexec
NO_VMPROC "NO_VMPROC: %1: ERROR: No hay proceso vm.\n"
$	-	crt
NO_WIDE "\
\r\n\
\          +===============================+ \r\n\
\          |                               | \r\n\
\          |       SOLO 80 COLUMNAS        | \r\n\
\          |              EN               | \r\n\
\          |        TERMINAL ASCII         | \r\n\
\          |                               | \r\n\
\          | Salga de modo de 132 columnas | \r\n\
\          |                               | \r\n\
\          +===============================+ \r\n\
\r\n\
\n"
$	-	dosexec
ON_ERR "ON_ERR: %1: ERROR: No se puede ejecutar DOS orientado a visualizaci�n con ON.\n"
$	-	dosexec
PATH_TOOMUCH "PATH_TOOMUCH: %1: ERROR: PATH de entorno demasiado largo %2. m�x.=%3.\n"
$	-	dosexec
PROC_EXIT "PROC_EXIT: %1: ERROR: Salida inesperada del proceso necesario %2.\n\
 Saliendo.\n"
$	-	dosexec
$	-	dosopt
$	-aix	pssnap
$	-	romsnap
$	-	tkbd
READ_ERR "READ_ERR: %1: ERROR: Imposible leer archivo '%2'.\n"
$	-	dosexec
PORT_NA "PORT_NA: %1: ERROR: No se puede(n) asignar puerto(s) %2\n"
$	- dosexec (SVR4 only)
REBOOT_FOR_PARTITIONS "\
%1: Advertencia:\n\
Las particiones DOS no estar�n accesibles hasta que se vuelva a arrancar el sistema.\n"
$	-	dosopt
RMV_OPTS_ERR1 "RMV_OPTS_ERR1: %1: ERROR: No se puede suprimir la opci�n %2.\n\
\  La opci�n no est� instalada."
$	-	dosopt
RMV_OPTS_ERR2 "RMV_OPTS_ERR2: %1: ERROR: No se puede suprimir opci�n %2.\n\
\  Opci�n no autorizada.\n"
$	-	romsnap
ROMSNAP_BAD_ADDR "ROMSNAP_BAD_ADDR: %1: ERROR:  Direcci�n inicial de rom err�nea %2.\n"
$	-	romsnap
ROMSNAP_USAGE "\nUso:  romsnap  direcci�n_inicial_rom  archivo_imagen_rom [-k].\n"
$	-	dosopt
$	-aix	pssnap
$	-	romsnap
SEEK_ERR "SEEK_ERR: %1: ERROR: Problema al utilizar el archivo %2. (fallo de b�squeda).\n"
$	-	dosexec
SEND_SIG_ERR "SEND_SIG_ERR: %1  No se puede enviar se�al %2 a %3, error = %4.\n"
$	-	dosexec
SERIOUS	"SERIOUS: Error grave en %1:\n%2\n"
$	-	dosexec
SHAREINIT "SHAREINIT: %1: ERROR: No se puede inicializar segmento de memoria compartida.\n\
Error = %2 (%3)\n"
$	-	dosexec
SHELL_NF "SHELL_NF: %1: ERROR: No se puede utilizar SHELL=%2. archivo no encontrado.\n"
$	-	crt
SHMA_FAILED "SHMA_FAILED: %1: ERROR: No se pudo usar la memoria compartida. Error=%2\n"
$	-	dosexec
SHMC_FAILED "SHMC_FAILED: %1: ERROR: No se pudo crear memoria compartida.\n\
Error=%2 a=%3 s=%4\n"
$	-aix	dosexec
SITE_NI "SITE_NI: %1: ERROR: Merge no est� instalado en emplazamiento %2.\n"
$	-	dosexec
$	-	dosopt
SOPT_USAGE "SOPT_USAGE: %1: ERROR: n�mero del tiempo l�mite +s fuera de rango.\n\
Para especificar que no hay tiempo l�mite, use 0.\n\
Para especificar uno, use %2 a %3.\n"
$	-	dosexec
SWITCHDOS_FAIL "SWITCHDOS_FAIL: %1: ERROR: Fallo de comunicaci�n UNIX-DOS.\n\
Error=%2.\n"
$	-	dosexec
$	-	crt
SWKEY_FAIL "SWKEY_FAIL: ERROR: No se puede ejecutar switchkey desde un terminal.\n"
$	-	swkey
SWKEY_CUR "\nSecuencia actual de pantalla de conmutaci�n de Merge 386 y Xsight: %2%3%4F<n>.\n"
$	-	swkey
SWKEY_USAGE "\n%1: ERROR: Opcion err�nea\n\
Uso: switchkey [-acs]\n"
$	-	swkey
SWKEY_NEW "\nSecuencia nueva de pantalla de conmutaci�n de Merge 386 y Xsight: %2%3%4F<n>.\n"
$	-	swkey
TERM_CAP "TERM_CAP: %1: ERROR: Su terminal no tiene las capacidades\n\
\   necesarias para soportar DOS remoto orientado a visualizaci�n.\n"
$	-	dosexec
TERM_ERR "TERM_ERR: %1: ERROR: Sin informaci�n de terminal para TERM '%2'.\n"
$	-	dosexec
TOKENPAIR_NF "TOKENPAIR_NF: %1: ERROR: No se pudo encontrar\n\
tipos de testigo adecuados para\n'%2=%3'.\n"
$	-	dosexec
TOKEN_BAD "TOKEN_BAD: %1: ERROR: Testigo err�neo %2 (%3).\n"
$	-	dosexec
TOKEN_NF "TOKEN_NF: %1: ERROR: No se pudo encontrar testigo '%2' del tipo necesario.\n"
$	-	dosexec
BAD_DRIVE "BAD_DRIVE: %1: ERROR: letra de unidad err�nea %2.\n"
$	-	dosexec
LPT_BAD "LPT_BAD: %1: ERROR: LPT2 y LPT3 no pueden definirse como un tipo de testigo de conexi�n directa.\n"
$	-	dosopt
UI_PROB "UI_PROB: %1: ERROR: No se puede desinstalar el archivo '%2'.\n\
\     El archivo no est� instalado en este momento.\n"
$	-	dosexec
UMB_CONFLICT "UMB_CONFLICT: %1: ERROR: Conflicto al asignar memoria superior\n\
\     bloques (UMBs) %2-%3\n"
$	-	dosexec
UPOPT_ERR "UPOPT_ERR: %1: ERROR: Opciones +p y +u no autorizadas al\
\  mismo tiempo.\n"
$	-	dosexec
VECT_NA "VECT_NA: %1: ERROR: No se puede asignar vector de interrupci�n %2\n"
$	-	crt
VMATTACH_FAILED "VMATTACH_FAILED: %1: ERROR: No se pudo adjuntar memoria vm86.\n\
Error=%2\n"
$	-	dosexec
VMSVR_FAIL "VMSVR_FAIL: %1: Fallo en la inicializaci�n del servidor VM. Error=%2.\n"
$	-	crt
$	-	dosexec
VM_DIED "VM_DIED: %1: ERROR: El proceso VM86 ha expirado.\n"
$	-	dosexec		janus_main.c
WAIT_RET_ERR "WAIT_RET_ERR: %1:  ERROR:  la llamada del sistema de espera ha regresado prematuramente\n\
Error= %2.\n"
$	-	dosopt
$	-aix	pssnap
$	-	romsnap
WRITE_ERR "WRITE_ERR: %1: ERROR: Imposible escribir archivo '%2'.\n"
$	-	dosopt
ZOPT_ERR "ZOPT_ERR: %1: ERROR: No se puede usar la opci�n 'z' o 'Z' en un\n\
archivo de opciones por defecto %2.\n"
$	-	dosopt
ZOPT_EXTRA "ZOPT_EXTRA: %1: ERROR: S�lo puede especificarse una opci�n despu�s de 'z'.\n"
$	-	dosopt
ZOPT_MISSING "ZOPT_MISSING: %1: ERROR: Falta opci�n despu�s de 'z'.\n"
$	-	dosopt
ZOPT_TOOMANY "ZOPT_TOOMANY: %1: ERROR: Demasiados elementos +z especificados. No se puede continuar.\n"
$	-	dosexec
HELP_EXEC "\n\
\   Uso: %1 [indicadores] nombre_archivo args...\n\
\   Uso: %1 [indicadores]\n\
\   La primera forma ejecuta nombres de archivo de DOS, y la segunda\n\
\   'command.com' de DOS.  Los indicadores no tienen en cuenta las opciones\n\
\   por defecto o instaladas y proporcionan directrices para anular las\n\
\   operaciones normales de dosexec.\n\
\   ('+' indica selecci�n, '-' indica cancelaci�n de la selecci�n de la opci�n.)\n\
\     +- a[x] Asignar dispositivo 'x'.\n\
\     +- b    Programa coordinado. (redirecciona stdio de/a UNIX)\n\
\     +  c    Comando: El nombre_de_archivo del comando DOS se pasa a DOS sin\n\
\             cambios.\n\
\     +  dX   Definir unidad inicial actual. X= de la a hasta la z.\n\
\     +- e[f] Usar el archivo de configuraci�n de dispositivos 'f'.\n\
\     +  h    Muestra informaci�n de ayuda (esta pantalla). DOS no se ejecuta.\n\
\     +- l[f] Cargar archivo de imagen de DOS 'f' ('f' tambi�n puede ser un\n\
\             nombre de directorio).\n\
\     +  mn   Tama�o de la memoria. N�mero 'n' (decimal) en Megabytes.\n\
\     +- p[f] Hacer que command.com sea 'Permanente'. 'f' es el autoexec.bat que\n\
\             se ejecutar�.\n\
\     +- s[n] Enviar la salida de impresi�n DOS a la cola de espera de la\n\
\             impresora UNIX. El tiempo l�mite est� en 'n' segundos.\n\
\     +- t    Traducir los argumentos de la l�nea de comandos de DOS.\n"

$ 	-	dosopt
HELP_INST "\n\
Uso: %1 [indicadores] nombres_archivo\n\
\   Opciones de instalaci�n para nombre de archivo. Los indicadores de\n\
\   install van precedidos de '+' o '-'.  Los '+' o '-' indican la seleccion\n\
\   o la anulacion de dicha opci�n.  Cuando no se especifican opciones ni\n\
\   directrices, se imprimen las opciones actuales.\n\
\     +- a[x] Asignar 'x' al dispositivo.\n\
\     +- b    Programa coordinado. (redireccionar  E/S est�ndar de/a UNIX)\n\
\     +- dX   Definir unidad actual inicial. X= a thru z.\n\
\     +- e[f] Usar archivo de configuraci�n de dispositivo 'f'.\n\
\     +  h    Mostrar informaci�n de ayuda (esta pantalla).\n\
\     +- l[f] Cargar imagen del archivo DOS 'f' ('f' tambi�n puede ser un nombre\n\
\             de directorio).\n\
\     +  mn   Tama�o de la memoria.  N�mero 'n' (decimal) en Megabytes.\n\
\     +- p[f] Hacer que command.com sea 'Permanente'. 'f' es el autoexec.bat que\n\
\             se va a ejecutar.\n\
\     +- s[n] Enviar la salida de la impresora DOS a la cola de espera de la\n\
\             impresora UNIX. El tiempo l�mite en 'n' segundos.\n\
\     +- t    Traducir los argumentos de la l�nea de comandos de  DOS.\n\
\     +- v    Detalle.  Mostrar mensaje de acuse de recibo.\n\
\     +  y    Hace que los archivos .COM puedan ejecutarse desde UNIX.  No es\n\
\             necesario cuando hay otras opciones UNIX definidas.\n\
\     +  zX   Suprimir o redefinir la opci�n X.\n\
\     +  Z    Suprimir todos los datos de instalaci�n del archivo.\n"
$
$ Note: The messages for the ifor_pm_* calls are for SCO only dosexec.
$
$ Error messages for error returns from 'ifor_pm_init_sco()'
$
$ --- For return value:  IFOR_PM_REINIT
PM_INIT_REINIT "\
%1: ADVERTENCIA- Se ha llevado a cabo un intento de reinicializar el gestor de criterios. (IFOR_PM_REINIT)\n"
$
$ --- For return value:  IFOR_PM_BAD_PARAM
PM_INIT_BAD_PARAM "\
%1: ERROR-  Par�metro incorrecto para ifor_pm_init_sco.  (IFOR_PM_BAD_PARAM)\n"
$
$ --- For return value:  IFOR_PM_FATAL
PM_INIT_FATAL "\
%1: ERROR-  No se pudo inicializar el gestor de criterios. (IFOR_PM_FATAL)\n"
$
$ Error messages for error returns from 'ifor_pm_request_sco()'
$
$ --- For return value:  IFOR_PM_BAD_PARAM
PM_REQ_BAD_PARAM "\
%1: ERROR-  Par�metro incorrecto para ifor_pm_request_sco.  (IFOR_PM_BAD_PARAM)\n"
$
$ --- For return value:  IFOR_PM_NO_INIT
PM_REQ_NO_INIT "\
%1: ERROR-  Se pidi� la licencia antes de inicializar el gestor de criterios. \n\
(IFOR_PM_NO_INIT)\n"
$
$ --- For return value:  IFOR_PM_FATAL
PM_REQ_FATAL "\
%1: ERROR-  No se pudo pedir la licencia del gestor de criterios. (IFOR_PM_FATAL)\n"
$
$ Error messages for error returns from 'ifor_pm_release()'
$
$ --- For return value:  IFOR_PM_BAD_PARAM
PM_RELEASE_BAD_PARAM "\
%1: ADVERTENCIA- Par�metro incorrecto para ifor_pm_release.  (IFOR_PM_BAD_PARAM)\n"
$
$ --- For return value:  IFOR_PM_FATAL
PM_RELEASE_FATAL "\
%1: ADVERTENCIA- No se pudo entregar la licencia. (IFOR_PM_FATAL)\n"
$
$	- 	xdosopt
XDOSOPT_TITLE "Opciones de DOS"
XDOSOPT_START_BUTTON "INICIO"
XDOSOPT_SAVE_BUTTON "Aplicar"
XDOSOPT_DEFAULT_BUTTON "Valores por defecto"
XDOSOPT_HELP_BUTTON "Ayuda"
XDOSOPT_CANCEL_BUTTON "Cancelar"
XDOSOPT_VIDEO_TITLE "V�deo"
XDOSOPT_VGA_LABEL "VGA"
XDOSOPT_CGA_LABEL "CGA"
XDOSOPT_MDA_LABEL "MDA"
XDOSOPT_HERC_LABEL "Hercules"
XDOSOPT_COM_TITLE "Puertos COM"
XDOSOPT_COM1_LABEL "COM1"
XDOSOPT_COM2_LABEL "COM2"
XDOSOPT_EMS_TITLE "EMS"
XDOSOPT_DRIVES_TITLE "Unidades"
XDOSOPT_DEFAULT_DRIVE "Ninguno"
XDOSOPT_LPT_TITLE "Puertos LPT"
XDOSOPT_LPT_NAME "LPT1"
XDOSOPT_SPOOL_LP_NAME   "UNIX (en cola de espera)"
XDOSOPT_DIRECT_LP0_NAME "DOS (Directo):lp0"
XDOSOPT_DIRECT_LP1_NAME "DOS (Directo):lp1"
XDOSOPT_DIRECT_LP2_NAME "DOS (Directo):lp2"
XDOSOPT_STATUS_DONE_MSG "Realizado"
XDOSOPT_QUIT_MSG "No ha aplicado sus cambios.\n�Est� seguro de que quiere abandonar?"
XDOSOPT_YES_BUTTON "S�"
XDOSOPT_NO_BUTTON "No"
XDOSOPT_OK_BUTTON "OK"
XDOSOPT_SAVE_ERR_MSG "No tiene permiso para\nguardar los cambios de este archivo."
XDOSOPT_MEM_TITLE "Memoria"
XDOSOPT_XMEM_NAME "Est�ndar"
XDOSOPT_EMEM_NAME "EMS"
XDOSOPT_CONFIG_READ_MSG "Imposible leer la configuraci�n de DOS o Windows."
XDOSOPT_MEMORY_MSG	"Memoria insuficiente."
XDOSOPT_INTERNAL_ERROR	"Error interno."
$	- 	x_msg
XMSG_DOSERRTITLE "Error de DOS"
XMSG_OKTITLE "OK"
$	- 	xcrt
XCRT_DOS_TITLE	"DOS"
XCRT_WINDOWS_TITLE "Windows"
XCRT_FILE	"Archivo"
XCRT_FILE_M	"A"
XCRT_ZOOM	"Zoom"
XCRT_ZOOM_M	"Z"
XCRT_REFRESH	"Renovar"
XCRT_REFRESH_M	"R"
XCRT_EXIT	"Salir"
XCRT_EXIT_M	"S"
XCRT_OPTIONS	"Opciones"
XCRT_OPTIONS_M	"O"
XCRT_FOCUS	"Zona activa del rat�n al DOS"
XCRT_FOCUS_M	"Z"
XCRT_FONTS	"Fuentes de DOS"
XCRT_FONTS_M	"F"
XCRT_KEYS	"Teclas aceleradoras"
XCRT_KEYS_M	"T"
XCRT_TUNE	"Ajustar..."
XCRT_TUNE_M	"A"
XCRT_AUTO	"Autom�tico"
XCRT_AUTO_M	"t"
XCRT_SMALL	"Peque�o"
XCRT_SMALL_M	"P"
XCRT_MEDIUM	"Medio"
XCRT_MEDIUM_M	"M"
XCRT_TO_DOS	"A DOS/Windows"
XCRT_TO_DOS_M	"W"
XCRT_TO_X	"Al escritorio X"
XCRT_TO_X_M	"X"
XCRT_OK		"OK"
XCRT_CANCEL	"Cancelar"
XCRT_HELP	"Ayuda"
XCRT_HELP_M	"y"
XCRT_ON_WINDOW	"En Ventana"
XCRT_ON_WINDOW_M "V"
XCRT_ON_KEYS	"En Claves"
XCRT_ON_KEYS_M	"C"
XCRT_INDEX	"Indice"
XCRT_INDEX_M	"I"
XCRT_ON_VERSION "En Versi�n"
XCRT_ON_VERSION_M "r"
XCRT_TUNE_TITLE		"Opciones"
XCRT_TUNE_COLORMAP	"Mapa de colores"
XCRT_TUNE_AUTOZOOM	"Zoom autom�tico"
XCRT_TUNE_DEFAULTS	"Valores de f�brica por defecto"
XCRT_CLIPBOARD	"Actualizando portapapeles X ..."
XCRT_CLIPBOARD_DONE	"Actualizando portapapeles X ... finalizado."
XCRT_VERSION		"Versi�n"
XCRT_VERSION_TEXT	"DOS Merge\n%1\nCopyright %2\nLocus Computing Corporation"
XCRT_QUIT_MSG "�Abandonar DOS?"
XCRT_NOHELP_MSG "No hay ayuda disponible."
XCRT_YES_BUTTON_TEXT "S�"
XCRT_NO_BUTTON_TEXT "No"
XCRT_OK_BUTTON_TEXT "OK"
XCRT_NODEV_ERR "Error: Dispositivo %1 no disponible."
XCRT_INTR_ERR "Error: Error MERGE interno."
XCRT_VT_ERR "Error: No se pudo obtener una nueva pantalla."
XCRT_SERVER_ERR "Error: El servidor no liberar� la visualizaci�n"
XCRT_NOMSE_ERR "Sesi�n DOS no configurada para rat�n."
XCRT_TMPL1 "%1 aparece aumentado en la pantalla vt%2."
XCRT_TMPL2 "Pulse %1 para volver al men� de DOS."
XCRT_MSG1 "Los gr�ficos EGA/VGA no est�n disponibles en una ventana."
XCRT_MSG2 "Abandonar la ventana DOS y reiniciar DOS."
XCRT_MSG3 "Seleccione Zoom en el men� Ventana para ejecutar los gr�ficos"
XCRT_MSG4 "El zoom no puede continuar."
XCRT_MSG5 "La zona no es funcional."
XCRT_VERSION_ERROR "Su controlador de visualizaci�n DOS Merge Windows/X\n\
precisa ser actualizado. \n\
V�anse las notas, al administrador o haga lo\n\
siguiente:\n\
1) arranque DOS\n\
2) teclee\n\
   WINXCOPY <unidad>:<directorio windows\\system>\n\
   ejemplo: WINXCOPY D:\\WINDOWS\\SYSTEM"
XCRT_PICTURE_ERROR "Imposible mostrar imagen.  Trate de \nusar una resoluci�n de pantalla de \n800 x 600 o superior, y 256 colores \no m�s."
$	- 	xcrt - Old GUI Messages
XCRT_MENU_TITLE "Men� de DOS"
XCRT_UNFOCUS_LABEL "Desactivar zona"
XCRT_X_COLORS_LABEL "Colores X"
XCRT_DOS_COLORS_LABEL "Colores de DOS"
XCRT_FREEZE_LABEL "Autocongelaci�n activada"
XCRT_UNFREEZE_LABEL "Autocongelaci�n desactivada"
XCRT_UNZOOM_LABEL "Desactivar zoom"
XCRT_NORMAL_KEYS_LABEL "Teclas de DOS"
XCRT_WINDOW_KEYS_LABEL "Teclas de escritorio"
XCRT_OLD_MSG3 "Haga click sobre Zoom en el men� de DOS para ejecutar gr�ficos."
$	- 	xmrgconfig
GUI_OK			"OK"
GUI_CANCEL		"Cancelar"
GUI_HELP		"Ayuda"
GUI_YES			"S�"
GUI_NO			"No"
GUI_DELETE		"Suprimir"
GUI_MODIFY		"Modificar"
GUI_TITLE 		"DOS Merge"
GUI_AUTOMATIC_LABEL	"Autom�tico"
GUI_CONFIG_TITLE	"Configuraci�n de DOS y Windows"
GUI_NONE_LABEL		"Ninguno"
GUI_HOME_LABEL		"Origen"
GUI_DEVICES_BUTTON_LABEL "Dispositivos"
GUI_OPTIONS_BUTTON_LABEL "Opciones"
GUI_SAVE_LABEL		"Guardar"
GUI_SAVE_AS_LABEL	"Guardar como..."
GUI_START_LABEL		"INICIO"
GUI_DRIVES_LABEL	"Unidades"
GUI_CONFIGURE_LABEL	"Configurar"
GUI_DOS_DRIVE_LABEL	"Unidad de DOS..."
GUI_UNIX_FILESYS_LABEL	"Sistema de archivos UNIX..."
GUI_COM_PORTS_LABEL	"Puertos COM"
GUI_COM1_LABEL		"COM1"
GUI_COM2_LABEL		"COM2"
GUI_LPT_PORTS_LABEL	"Puertos LPT"
GUI_LPT1_LABEL		"LPT1"
GUI_LPT2_LABEL		"LPT2"
GUI_LPT3_LABEL		"LPT3"
GUI_TIMEOUT_LABEL	"Tiempo l�mite"
GUI_DEVICES_LABEL	"Dispositivos"
GUI_DEVICES_STATUS_MSG	"Existe un conflicto entre\n%1 y\n%2."
GUI_MULTI_CONFLICT_MSG  "M�ltiples conflictos de dispositivos\ndetectados."
GUI_MEMORY_LABEL	"Memoria"
GUI_STANDARD_LABEL	"Est�ndar"
GUI_EMS_LABEL		"EMS"
GUI_AUTOEXEC_LABEL	"AUTOEXEC.BAT"
GUI_RUN_SYS_LABEL	"Ejecutar para todo el sistema"
GUI_PERSONAL_LABEL	"Ejecutar personal"
GUI_OTHER_LABEL		"Otros"
GUI_BROWSE_LABEL	"Observar..."
GUI_EDIT_LABEL		"Editar archivos"
GUI_AUTOEXEC_MNEMONIC	"A"
GUI_SYS_AUTO_LABEL	"AUTOEXEC.BAT para todo el sistema..."
GUI_PERSONAL_AUTO_LABEL "AUTOEXEC.BAT personal..."
GUI_OTHER_AUTO_LABEL	"Otro AUTOEXEC.BAT..."
GUI_CONFIG_LABEL	"CONFIG.SYS"
GUI_CONFIG_MNEMONIC	"C"
GUI_SYS_CONFIG_LABEL	"CONFIG.SYS para todo el sistema..."
GUI_PERSONAL_CONFIG_LABEL "CONFIG.SYS personal..."
GUI_OTHER_CONFIG_LABEL	"Otro CONFIG.SYS..."
GUI_WINDOWS_SIZE_LABEL	"Tama�o de Windows"
GUI_CUSTOM_LABEL	"Personalizar:"
GUI_RESIZE_LABEL	"Ajuste de tama�o manual..."
GUI_WIDTH_LABEL		"Width:"
GUI_HEIGHT_LABEL	"Height:"
GUI_DOS_SIZE_LABEL	"Tama�o de DOS"
GUI_DOS_FONT_LABEL	"Fuente de DOS"
GUI_SCALE_DOS_LABEL	"Escala de gr�ficos de DOS"
GUI_DISPLAY_TYPE_LABEL	"Tipo de visualizaci�n"
GUI_SMALL_LABEL		"Peque�o"
GUI_MEDIUM_LABEL	"Medio"
GUI_LARGE_LABEL		"Grande"
GUI_X1_LABEL		"x1"
GUI_X2_LABEL		"x2"
GUI_MDA_LABEL		"MDA"
GUI_HERCULES_LABEL	"Hercules"
GUI_CGA_LABEL		"CGA"
GUI_VGA_LABEL           "VGA"
GUI_OPTIONS_LABEL	"Opciones"
GUI_AUTOZOOM_LABEL	"Zoom autom�tico"
GUI_COLORMAP_LABEL	"Mapa de colores"
GUI_ACCEL_KEYS_LABEL	"Teclas aceleradoras para X"
GUI_FACTORY_LABEL	"Valores de f�brica por defecto"
GUI_COMMAND_LABEL	"Comando"
GUI_DOS_DRIVE_TITLE	"Unidad de DOS"
GUI_READ_ONLY_LABEL	"S�lo lectura"
GUI_EXCLUSIVE_LABEL	"Exclusivo"
GUI_FILES_LABEL	 	"Archivos"
GUI_FILTER_LABEL	"Filtro"
GUI_DIRECTORIES_LABEL	"Directorios"
GUI_SELECTION_LABEL	"Selecci�n"
GUI_RESIZE_TITLE	"Tama�o de ventana de Windows"
GUI_FILE_LABEL		"Archivo"
GUI_RESIZE_MESSAGE	"Use el rat�n o el teclado para ajustar el tama�o de\nesta ventana para su tama�o deseado de Microsoft Windows."
GUI_STATUS_MSG		"L�nea de estado"
GUI_CONFIRM_MSG		"Ha realizado cambios de configuraci�n.\n�Est� seguro de que desea salir?"
GUI_FILE_DOESNT_EXIST_MSG "El archivo no existe."
GUI_DIR_DOESNT_EXIST	"El directorio no existe."
GUI_FILE_IS_DIR_MSG	"El archivo seleccionado es un directorio, y no puede abrirse para su edici�n."
GUI_PERMISSION_DENIED	"Permiso denegado."
GUI_NOT_DIR		"La selecci�n especificada no es un directorio."
GUI_EDIT_ERROR		"Error al editar archivo"
GUI_OPEN_CONFIG_ERROR	"Error al abrir archivo de configuraci�n."
GUI_CONFIG_DOESNT_EXIST "La configuraci�n \"%1\" no existe."
GUI_INTERNAL_ERROR	"Error interno"
GUI_MEMORY_ERROR	"Memoria insuficiente."
GUI_WRITE_CONFIG_ERROR	"No se puede guardar el archivo de configuraci�n:\n%1"
GUI_DELETE_CONFIG_ERROR	"No se puede borrar el archivo de configuraci�n."
GUI_ALREADY_EXISTS	"La configuraci�n \"%1\" ya existe."
GUI_CONFLICT_MSG	"Hay conflictos de hardware entre los siguientes dispositivos:\n%1.\nPara resolver los conflictos, cancele la selecci�n de \nuno o m�s de estos elementos en el �rea \"Dispositivos\". \nSi desea usar estos dispositivos juntos, debe reconfigurar uno \no m�s de ellos para evitar un conflicto de hardware."
GUI_DRIVE_WARNING	"Advertencia: \"%1\" no existe."
GUI_CANT_START		"No se puede iniciar la sesi�n de DOS o Windows."
GUI_NO_DRIVES_ERROR	"No hay disponibles unidades de DOS."
GUI_CANT_READ_FILE	"No se puede leer el archivo \"%1\"."
GUI_CANT_CREATE_FILE	"No se puede crear el archivo \"%1\"."
GUI_VIEW_FILE_MSG	"El archivo \"%1\" no dispone del permiso de escritura. �Desea visualizarlo?"
GUI_CREATE_FILE_MSG	"El archivo \"%1\" no existe. �Desea crearlo?"
$	-	mrgadmin messages
ADM_USAGE_CMD "Uso: mrgadmin class add testigo:archivo:nombre completo\n     mrgadmin class delete testigo\n     mrgadmin class list [testigo]\n     mrgadmin class update testigo:archivo:nombre completo\n     mrgadmin class printdef testigo\n"
ADM_CLASS_MSG "Error: las clases permitidas son:\n"
ADM_NO_CLASS_MSG "Imposible obtener informaci�n sobre clase \"%1\".\n"
ADM_NO_CLASS_ENTRIES_MSG "No se han hallado entradas para la clase \"%1\".\n"
ADM_BAD_TOKEN_MSG "No se puede obtener informaci�n sobre el testigo \"%1\".\n"
ADM_PERMISSION_MSG "Tiene que ser el usuario ra�z para modificar la administraci�n de Merge.\n"
ADM_CANT_DELETE_TOKEN_MSG "No se puede  borrar el testigo \"%1\"\n"
ADM_BAD_TOKEN_DEF_MSG "La definici�n del testigo debe tener el formato \"testigo:archivo:nombre\".\n"
ADM_CANT_READ_MSG	"No se puede leer \"%1\": %2.\n"
$	-	mrgadmin commands
ADM_ADD_STR		"add"
ADM_DELETE_STR		"delete"
ADM_LIST_STR		"list"
ADM_UPDATE_STR		"update"
ADM_PRINTDEF_STR	"printdef"
$	-	admin library messages
ADM_NO_ERROR "Error interno - no hay error."
ADM_INTERNAL_ERROR "Error interno en librer�a de administraci�n."
ADM_PARSE_VARIABLE "Nombre de variable no reconocida."
ADM_PARSE_NO_NUMBER "Debe especificarse un n�mero."
ADM_PARSE_NUMBER "Valor num�rico no v�lido."
ADM_PARSE_ILLEGAL_VARIABLE "Variable no permitida para el tipo de conexi�n especificado."
ADM_PARSE_MAX_NUMBERS "Se ha excedido el n�mero m�ximo permitido de elementos."
ADM_MEMORY "Memoria insuficiente."
ADM_PARSE_MAX_NUMBER "Se ha excedido el valor num�rico m�ximo."
ADM_PARSE_RANGE "Especificaci�n err�nea de rango."
ADM_BAD_ATTACH "Especificaci�n err�nea de tipo de conexi�n."
ADM_BAD_DEV_TYPE "Especificaci�n err�nea de tipo de dispositivo."
ADM_BAD_FAILURE_ACTION "Especificaci�n err�nea de acci�n de fallo."
ADM_BAD_USER_ACCESS "Especificaci�n err�nea de acceso de usuario."
ADM_BAD_VPI_PPI_OPTION "Opci�n VPI/PPI err�nea."
ADM_BAD_DRIVE_OPTION "Opci�n de unidad err�nea."
ADM_MISSING_VPI_DEV "Falta la especificaci�n requerida de dispositivo vpi."
ADM_MISSING_PPI_DEV "Falta la especificaci�n requerida de dispositivo ppi."
ADM_MISSING_DRIVE_NAME "Falta la especificaci�n requerida de nombre de unidad."
ADM_MISSING_PRINTER_CMD "Falta la especificaci�n requerida de comando de impresora."
ADM_IRQ_MISMATCH "No concordancia entre irq y physicalirq."
ADM_DMA_MISMATCH "No concordancia entre dma y physicaldma."
ADM_IOPORTS_MISMATCH "No concordancia entre ioports y physicalioports."
ADM_MEM_MISMATCH "No concordancia entre memmappedio y physicalmemmappedio."
ADM_SYSERR "Error de sistema."
ADM_CLASS_NOT_FOUND "Clase no encontrada"
ADM_TOKEN_NOT_FOUND "Testigo no encontrado."
ADM_LOCK "No se puede bloquear el archivo de testigos."
ADM_TOKEN_EXISTS "Ya existe el testigo."
ADM_BAD_TOKEN "Los nombres de testigos se limitan a los caracteres A-Z, a-z, 0-9 y \"-\"."
ADM_NO_ATTACH "Falta la especificaci�n requerida de tipo de conexi�n."
ADM_PARSE_OPTION "Valores de opciones no reconocidos."
ADM_BAD_UMB "Especificaci�n UMB no v�lida.  Debe estar en el rango 0xA0000-0xFFFF."
$       -       admconvert mensajes
ADMCVT_CANT_OPEN "No se puede abrir \"%1\".\n"
ADMCVT_CANT_CREATE "No se puede crear \"%1\".\n"
ADMCVT_BAD_FORMAT "Error de formato en /etc/dosdev.\n"
ADMCVT_MEMORY "Memoria insuficiente.\n"
ADMCVT_CANT_ADD "No se puede a�adir testigo \"%1\": %2\n"
ADMCVT_LP_CONVERT "No se puede convertir /usr/lib/merge/lp.\n"
ADMCVT_DOS_DRIVE_NAME "DOS original %1:"
ADMCVT_COM1_NAME "DOS Direct IRQ 4"
ADMCVT_COM2_NAME "DOS Direct IRQ 3"
ADMCVT_PORT1_NAME "DOS Direct Port 1"
ADMCVT_PORT2_NAME "DOS Direct Port 2"
$	-	Configuration library errors
CFG_NO_ERROR		"Error interno - no hay error."
CFG_PARSE_INTERNAL_ERROR "Error interno en an�lisis."
CFG_PARSE_VARIABLE	"Nombre de variable no reconocida."
CFG_PARSE_ACCEL		"Valor no reconocido de teclas aceleradoras.  Use \"dos\" o \"x\"."
CFG_PARSE_BOOLEAN	"Valor booleano no reconocido.  Use \"true\" o \"false\"."
CFG_PARSE_DISPLAY	"Valor no reconocido de tipo de visualizaci�n.  Use \"auto\", \"vga\", \"cga\", \"mono\" o \"hercules\"."
CFG_PARSE_DRIVE_DEF	"Definici�n no reconocida de unidad.  Use un testigo de unidad o \"none\"."
CFG_PARSE_DRIVE_OPTION	"Opci�n no reconocida de unidad.  Use \"readonly\" o \"exclusive\"."
CFG_PARSE_DRIVE_LETTER	"Letra de unidad no v�lida.  S�lo se permite de la C a la J."
CFG_PARSE_NUMBER	"Valor num�rico no v�lido."
CFG_PARSE_SCALE_VALUE	"Valor scaledosgraphics no reconocido.  Use \"auto\", \"1\"," � \"2\"."
CFG_PARSE_NO_NUMBER	"Debe especificarse un n�mero."
CFG_PARSE_OPTION	"Valores de opciones no reconocidos."
CFG_PARSE_FONT		"Valor no reconodido de fuente.  Use \"auto\", \"small\" o \"medium\"."
CFG_UNZOOM_KEY		"Debe especificarse un nombre de clave X."
OPTCVT_BAD_USER		"No se puede obtener informaci�n del usuario actual."
OPTCVT_BAD_USER2	"No se puede obtener informaci�n para el usuario %1.\n"
OPTCVT_NOT_DIR		"Ya existe el directorio de configuraci�n."
OPTCVT_MKDIR_FAILED	"No se puede crear el directorio de configuraci�n."
OPTCVT_WRITE_FAILED	"No se pudieron escribir los archivos de configuraci�n."
OPTCVT_UNLINK		"Optconvert no puede suprimir los archivos antiguos de configuraci�n."
OPTCVT_ADM_ERROR	"Error de administraci�n durante la conversi�n."
OPTCVT_BAD_OPTIONS	"Uso: optconvert [-c usuario]\n"
OPTCVT_NO_CONVERT	"Ya existe la configuraci�n para el usuario %1.\n"
OPTCVT_BAD_LOG		"No se puede escribir en registro %1.\n"
OPTCVT_BAD_CONVERT	"No se puede convertir +a%1.\n"
CFG_BAD_DIR		"No se puede abrir el directorio de configuraci�n."
CFG_MEMORY		"Memoria insuficiente."
CFG_SYSERR		"Error de sistema."
CFG_BAD_NAME		"Nombre de configuraci�n no v�lido."
CFG_NOT_DEL	"No se puede borrar las configuraciones \"dos\" o \"win\"."
CFG_HEADER1	"Este archivo ha sido generado por las utilidades de configuraci�n de Merge."
CFG_HEADER2	"Use las utilidades de configuraci�n para realizar cambios en este archivo."
$	-	mrgconfig
CFG_BAD_CMD	"Error interno - comando no reconocido.\n"
CFG_YES_RESP	"y"
CFG_NO_RESP	"n"
CFG_CONFIRM	"�Est� seguro de borrar \"%1\"?"
CFG_RESPONSE	"Responda \"%1\" o \"%2\"\n"
CFG_CANT_DELETE "No se puede suprimir "%1".\n"
CFG_DELETED	"Configuraci�n \"%1\" borrada.\n"
CFG_NOT_DELETED	"Configuraci�n \"%1\" no borrada.\n"
CFG_ADD_USAGE_CMD "Uso: addmrgconfig config_antigua [copyto] config_nueva.\n"
CFG_DEL_USAGE_CMD "Uso: delmrgconfig config.\n"
CFG_LIST_USAGE_CMD "Uso: listmrgconfig\n"
CFG_DEL_OPT_ERROR "S�lo puede borrar opciones customdev.\n"
CFG_SET_OPT_ERROR "S�lo puede especificar una opci�n.\n"
CFG_USAGE_CMD	"Uso: mrgconfig config list opci�n[,opci�n...].\n     mrgconfig config set opci�n\n     mrgconfig config delete opci�n\n"
CFG_COPYTO_STR	"copyto"
CFG_LIST_STR	"list"
CFG_SET_STR	"set"
CFG_DELETE_STR	"delete"
CFG_ALREADY_EXISTS "La configuraci�n \"%1\" ya existe.\n"
CFG_BAD_OPTION	"Opci�n \"%1\"no reconocida.\n"
CFG_WRITE_ERROR	"Error interno - fallo de escritura.\n"
CFG_DEL_VALUE_ERROR "Debe especificar un valor customdev.\n"
CFG_DEL_NOT_FOUND "El dispositivo personalizado \"%1\" no existe en la configuraci�n \"%2\".\n"
CFG_READ_ERROR  "Error interno - no se puede leer la configuraci�n.\n"
CFG_LIST_ERROR	"Imposible obtener una lista de configuraciones.\n"
CFG_NOLIST_MSG	"No existe ninguna configuraci�n que incluir en lista.\n"
$ SCCSID(@(#)install.msg	1.23 LCC) Modified 16:45:28 9/7/94
$ Messages for the install and remove scripts.
$domain LCC.MERGE.UNIX.INSTALL
$quote "

GEN_ERR_1 "No se puede completar la instalaci�n debido a un error.\n"
GEN_ERR_2 "Inspeccione %1 para salida de errores.\n"
GEN_ERR_3 "Limpiando y saliendo...\n"
GEN_ERR_4 "Error en la supresi�n.\n"
GEN_ERR_5 "Continuando ....\n"
GEN_ERR_6 "No se est� reconstruyendo el kernel.\n"

BUILD_FAIL "Fallo en construcci�n del kernel.\n"
LU_FAIL   "Fallo en el enlace_unix\n"
IDB_FAIL  "Fall� el idbuild\n"

REPL_ERR  "No se puede reemplazar el %1 actualmente instalado.\n\
Borre %2 antes de volver a instalar.\n"

I_WARN_1   "El kernel en ejecuci�n tiene el %1 ya instalado.\n"
I_ERR_1   "%1 no se puede instalar en este sistema.\n"
I_ERR_2   "El Paquete Unix Base debe contener %1\n\
soporte o una actualizacion del Paquete Unix Base\n\
y debe instalarse primero.\n"

KERNEL_HAS_MERGE "\n\
El eco del kernel de UNIX actualmente en ejecucion tiene %1 instalado.\n\
%2 debe eliminarse completamente antes de reinstalar, o la \n\
instalaci�n podria fallar y el kernel no enlazarse correctamente.\n"

NO_STREAMS "\n\
Controlador de flujo sin instalar. El controlador de flujo se debe instalar\n\
antes de instalar %1.\n"

CANT_INST  "No se puede instalar %1.\n"
CANNOT_REMOVE "No se puede eliminar %1.\n"
NOSPACE    "\       No hay espacio suficiente para reconstruir el kernel.\n"

INSTALL_1 "Instalando %1 archivos. Espere...\n"
INSTALL_2 "Configurando %1 archivos. Espere...\n"

REMOVE_3 "Suprimiendo %1 archivos. Espere...\n"
REMOVE_4 "Los archivos de distribuci�n %1 han sido desinstalados.\n"

IDIN_FAIL "fall� idinstall %1.\n" \
ALREADY_INSTALLED "Advertencia: Controlador %1 ya instalado.\n"
IDCHK_FAIL  "idcheck -p de %1 devolvi� %2\n"

LINK_FAIL_1 "Fall� enlace de un archivo.\n"
LINK_FAIL_2 "Fall� el enlace: %1 %2\n"
CPIO_CP_FAIL "Fall� la copia de archivos al usar cpio.\n"
IDTUNE_FAIL "Fall� idtune\n"

$ The following five are used in install and remove when the user is
$ prompted if a re-link of the kernel is wanted.  YESES and NOES are
$ each a list of the acceptable single character responses for yes and no.
YESES "sS"
NOES "nN"
LINK_NOW_Q "�Desea construir ahora un nuevo kernel? (s/n) "
Y_OR_NO "Conteste s o n: "
REBUILD_WARN "Debe reconstruir el kernel para tener la funci�n %1 correctamente.\n"
REBUILD_NEED "Debe reconstruir el kernel para tener la funci�n de sistema correctamente.\n"

$ This section (to the line with END-PKGADD) is PKGADD/SVR4 specific.

INST_LOG_HDR "\
# Este es un archivo de registro creado con el proceso de instalaci�n %1.\n\
# Si aparece un error durante la parte %2 controlada de la instalaci�n,\n\
# aparecer� aqu�. Este proceso se crea durante la instalaci�n y el usuario\n\
# podr�a suprimirlo. Este archivo puede contener una salida sin errores\n\
# de otros programas (est�ndares) de UNIX.\n"

REM_LOG_HDR "\
# Esto es un archivo de registros creado con el proceso de supresi�n %1.\n\
# Si aparece un error durante la parte %2 controlada de la supresi�n, aparecer�\n\
# aqu�. Este archivo se ha creado durante la supresi�n y el usuario podr�a\n\
# suprimirlo. Este archivo puede contener una salida sin errores\n\
# de otros programas (est�ndares) de UNIX.\n"

INST_NO_VERS "\
ERROR: No se puede encontrar archivo de versi�n\n\
       No se puede instalar Merge\n"

PO_ERR_MSG1 "Imposible crear una entrada de impresora (%1).\n"

INST_USER_STARTUP "Actualizando los archivos del usuario de arranque de UNIX ...\n"
INST_PRINTER "Instalando la interfaz de impresora de DOS ...\n"
INST_CONFIG_SYS "Actualizando config.sys global ...\n"
INST_AUTOEXEC "Actualizando autoexec.bat global ...\n"
INST_DRV_MSG "Instalando %1 controladores ...\n"
INST_MOD_MSG "\tm�dulo: %1\n"
INST_MERGE "Conectando el controlador MERGE ...\n"
INST_DOSX "Conectando el controlador DOSX ...\n"
INST_SEG_WRAP "Conectando el controlador SEG WRAP ...\n"
INST_DONE "La instalaci�n de %1 ha finalizado.\n"
INST_CANCEL "Instalaci�n anulada por el usuario.\n"
INST_DRV_SH "Error al instalar los controladores Merge.\n"
INST_MRG_SH "Problemas al instalar los archivos Merge.\n"
INST_X_SH "Problemas al instalar los archivos Merge X.\n"

REM_USER_STARTUP "Actualizando los archivos del usuario de arranque de UNIX ...\n"
REM_DOS_FILES "Suprimiendo los archivos de DOS (desde dosinstall) ...\n"
REM_PRINTER "Suprimiendo la interfaz de impresora de DOS ...\n"
REM_PRINTER_NOT "Conservando la interfaz de impresora de DOS.\n"
MISSING_PROG "Falta el programa %1.\n"
REM_CONFIG_SYS "Actualizando config.sys global ...\n"
REM_AUTOEXEC "Actualizando autoexec.bat global ...\n"
REM_DRIVERS "Suprimiendo %1 controladores ...\n"
REM_MERGE "Desconectando el controlador MERGE ...\n"
REM_DOSX "Desconectando el controlador DOSX ...\n"
REM_SEG_WRAP "Desconectando el controlador SEG WRAP ...\n"
REM_DONE "La supresi�n de %1 ha finalizado.\n"
REM_CANCEL "Supresi�n anulada por el usuario.\n"

REM_PROB_MODLIST "Imposible encontrar %1/modlist, no se ha borrado ning�n m�dulo.\n"
REM_PROB_MODULE "Imposible eliminar %1.\n"

# request script
REQ_PREV_INSTALL "\n\
$MERGENAME ya est� instalado.\n\
Debe borrarse antes de que pueda reinstalarlse.\n\n"

$ END-PKGADD
$ SCCSID(@(#)scripts.msg	1.2 LCC) Merge 3.2.0 Branch Modified 12:38:24 10/14/94
$ SCCSID(@(#)scripts.msg	1.40 LCC) Modified 23:55:19 10/10/94
$ English message file for the Merge scripts.
$quote "

$domain LCC.MERGE.UNIX.SET_NLS
$ Messages for the "set_nls" script.
WARN_PREFIX "ADVERTENCIA: "
ERR_PREFIX "ERROR: "

$domain LCC.MERGE.UNIX.MKVFLP
$ Messages for the "mkvfloppy" script.
USAGE "USO: %1 v�a_de_acceso-nombre_de_archivo [-s]\n"

$domain LCC.MERGE.UNIX.DOSBOOT
$ Messages for the "dosboot" script.
ERR1 "ERROR: No se puede usar la opci�n \"+1\" con %1.\n"

$domain LCC.MERGE.UNIX.INITDPART
$ Messages for the "initdospart" script.

INSERT "Inserte un disquete de alta densidad en la unidad A: y pulse <INTRO>: "
FORMAT "Dar formato a la partici�n de disco de DOS.\n"
ERR_1  "ERROR: No hay partici�n de disco de DOS.\n"
ERR_2  "ERROR: Debe estar en el ra�z para dar formato a la partici�n de DOS.\n"
ERR_3  "ERROR:  No se puede dar formato al disquete.\n\
\n\
Aseg�rese de que no est� protegido contra escritura.\n"

ERR_4  "ERROR:  El disquete parece estar defectuoso.\n"

TEXT_1 "Debe dar formato a la partici�n de DOS antes de utilizarla.\n\
\n\
No es necesario darle formato \n\
si ya lo tiene, a menos que la partici�n de disco de DOS\n\
haya sido redise�ada o movida desde que se le dio formato.\n\
\n\
Advertencia:  Si da formato a la partici�n de DOS se borrar�n todos los datos de \n\
\              la partici�n de DOS.\n\
Advertencia:  Para dar formato a la partici�n de disco de DOS es necesario parar\n\
\              el sistema y reiniciarlo desde un disquete.\n\
\n\
Debe disponer de un disquete de alta densidad.  Aseg�rese de que no est�\n\
protegido contra escritura.  D� formato al disquete y convi�rtalo en\n\
un disco de arranque.\n"

$ NOTE: the CONTINUE, YESES, NOES and Y_OR_N string are closely related.
$ The CONTINUE and Y_OR_N are prompts for a yes or no answer.
$ They don't have newline chars, so the cursor will stay at the end
$ of the printed line.
$ The YESSES and NOES strings define the recognized single character
$ yes or no answers.
$ The YESES  string is a list of single characters that mean "yes".
$ The NOES  string is a list of single characters that mean "no".
CONTINUE "�Desea continuar (s/n)? "
Y_OR_N "Conteste s o n: "
YESES "sS"
NOES  "nN"

RETRY "Asegurese de que esta usando un disquete de alta densidad sin defectos.\n\
\n\
�Desea intentarlo de nuevo (s/n)? "

REBOOT "\n\
Para dar formato a la particion de DOS, es necesario arrancar\n\
el sistema desde el disquete que se encuentra en la unidad.\n\
\n\
NO lo saque\n\
\n\
Pulse <INTRO> para cerrar el sistema y que rearranque. "

EXIT   "No se da formato a la partici�n de DOS.\n"
EXIT_2 "Debe dar formato a la partici�n de DOS antes de utilizarla.\n"


$domain LCC.MERGE.UNIX.DOSINSTALL
$ Messages for the dosinstall and dosremove scripts.
$    (sourcefiles: unix/janus/dosinst.sh, unix/janus/dosremove.sh)
EXITING "Saliendo...\n"
ERR_0 "No se puede instalar DOS.\n"
ERR_1 "Debe ser el ra�z o su para instalar DOS.\n"
ERR_2 "DOS esta ya instalado. Debe suprimir el DOS actualmente instalado\n\
antes de poder iniciar la nueva instalaci�n.\n"
CANNOT_CONTINUE "No se puede continuar.\n"
$ The BAD_FLOP_NAME error can happen when the user used the "DRIVE_DEV"
$ variable to specify which floppy device to use, and the device does
$ not exist.  If the user did not set DRIVE_DEV, then the internal
$ logic that determines the device filename did not work.  To work around
$ this problem, the use should set DRIVE_DEV to the device name to use.
BAD_FLOP_NAME "Nombre '%1' del dispositivo de la unidad de disco no v�lido\n"

RM_MSG "Suprimiendo DOS\n"
ERR_RM "Ha habido problemas suprimiendo DOS.\n\
La instalaci�n de DOS podria fallar.\n"
RE_INST "Reinstalando DOS\n"
INSTALLING_DOS "Instalando DOS\n"
INSTALLING_DOSVER "Instalando %1\n"
DOS_INST_DONE "Instalaci�n de DOS finalizada.\n"

DSK_DONE "Ahora puede retirar el disco.\n"

DOSINST_MKIMG_FAILED "Fall� la creaci�n de archivos de imagenes de DOS.\n\
\     La instalaci�n de DOS fall� parcialmente.\n"

DOSINST_MKIMG_MONO_FAILED "No se puede crear archivos imagen de DOS mono.\n\
\     Abortando la instalaci�n de DOS.\n"

NO_SPACE    "     No hay suficiente espacio.\n"
NO_SPACE_IN "     No hay epacio suficiente en %1.\n"
NEED_SPACE  "     Necesita: %1 Bloques, Tiene:%2.\n"

DOSINST_CANT "No se puede instalar %1\n"

DOSINST_DOSBRAND_MENU1 "Elija el DOS que vaya a instalar.\n"

DOSINST_DOSBRAND_MENU2 "\
\      0: No es ninguno de los que aparecen anteriormente.  Aborte la instalaci�n de DOS.\n\
\n\
Teclee el n�mero de elemento y pulse Intro: "

DOSINST_01 "Pulse 0 � 1\n"
DOSINST_012 "Pulse 0, 1 � 2\n"
DOSINST_0123 "Pulse 0, 1, 2 � 3\n"
DOSINST_01234 "Pulse 0, 1, 2, 3 � 4\n"
DOSINST_012345 "Pulse 0, 1, 2, 3, 4 � 5\n"
DOSINST_0123456 "Pulse 0, 1, 2, 3, 4, 5 � 6\n"
DOSINST_01234567 "Pulse 0, 1, 2, 3, 4, 5, 6 � 7\n"

DOSINST_SCO_DISKSIZE "\
Pulse el tama�o de los discos de DOS que tenga.\n\
\n\
\      1: 3,5\" discos de baja densidad.\n\
\      2: 3,5\" discos de alta densidad.\n\
\      3: 3,5\" discos de densidad desconocida.\n\
\      4: 5,25\" discos de baja densidad.\n\
\      5: 5,25\" discos de alta densidad.\n\
\      6: 5,25\" discos de densidad desconocida.\n\
\      0: Ninguno de los que aparecen anteriormente.  Abortar instalaci�n de DOS.\n\
\n\
Teclee el n�mero de elemento y pulse Intro: "

DOSINST_FLOP_P "\
�Desde qu� tipo de unidad de disco est� haciendo la instalaci�n?\n\
\n\
\      1: La primera unidad. (unidad '%1')\n\
\      2: La segunda unidad. (unidad '%2')\n\
\      0: Interrumpir. Abortar instalaci�n de DOS.\n\
\n\
Escriba el n�mero de opci�n y pulse Intro: "

DOSINST_MSG1 "Instalando %1 desde los discos %2.\n"

DOSINST_FROM "Instalando desde la unidad %1.\n"

WRONG_DISK_1 "\
\      El disco no es el correcto.\n\
\      Compruebe que tiene el disco correcto en la unidad correcta.\n"

WRONG_DISK_N "\
\      Este disco ya se ha le�do como disco #%1.\n\
\      Compruebe que tiene el disco correcto en la unidad correcta.\n"

DOSINST_DW_P "\
�Qu� desea hacer ahora?:\n\
\      0: Abortar la instalaci�n de DOS.\n\
\      1: Reintentar.\n\
Escriba 0 � 1 y pulse Intro: "

DOSINST_DW_R "Pulse 0 � 1\n"

DOSINST_NOTINST "No se est� instalando DOS.\n"
DOSINST_AUTO_NA "\
Imposible instalar DOS autom�ticamente.\n\
Para instalar DOS desde sus disquetes de DOS\n\
ejecute el programa 'dosinstall'.\n"

CHECK_DISK  "Verificando el disco."
INSERT_S "Inserte el disco N�%1 de DOS en la unidad y pulse INTRO. "
INSERT_D "Inserte el disco N�%1 de DOS en la unidad %2 y pulse INTRO. "
INSERT_NEXT "Reemplace el disco n�%1 por el n�%2 y pulse INTRO. "
INST_CANCEL "Instalaci�n anulada por el usuario.\n"
INST_SYS "Instalando los archivos de sistema de DOS.\n"
READING_MESSAGE "Leyendo el disquete."
CANNOT_CREATE "Error: No se pudo crear %1\n"
NO_FILE "Error: Falta %1\n"
MISSING_DIR "Error: Falta directorio %1\n"
CREATING_DIR "Advertencia: Falta el directorio %1\n\
Se est� creando.\n"

$ Note: Q_REMOVE, Q_CONTINUE, and Q_ANYWAY don't end in a newline.
$ Also, the answer to them must be either the string in ANS_Y or ANS_N or ANS_Q.
Q_REMOVE "�Desea suprimir ahora el DOS actualmente instalado (s/n/a) ? "
Q_CONTINUE "�Desea seguir con la instalaci�n de DOS (s/n/a) ? "
Q_ANYWAY "�Desea de todas formas intentar suprimir DOS (s/n/a) ? "
Q_PROMPT "Conteste s, n o a\n"
ANS_Y "y"
ANS_N "n"
$ Note: ANS_Q is what users can type at all prompts to "quit" or abort
$ the installation, (also entering 0 does the same thing as ANS_Q).
ANS_Q "q"
DOSINST_0Q_MSG "\
Si escribe 0 o a en cualquier indicador, se abortar� la instalaci�n de DOS.\n"
CANCEL_MSG "Cancelando la reinstalaci�n de DOS.\n"
IMPROPER "DOS no ha sido instalado correctamente.\n"
RM_ABORT "Abortando la supresi�n de DOS. No se ha suprimido ning�n archivo.\n"
RM_PROB "Error: Problemas en la supresi�n de DOS.\n"
RM_PROB_1 "Falta lista de archivos. No se puede desinstalar DOS completamente.\n"
RM_ALMOST "Intento de supresi�n de DOS realizado.\n"
RM_DONE "Supresi�n de DOS realizada.\n"
BAD_DISK "\
\      Problemas en la lectura del disco.\n\
\      Compruebe que el disco se haya insertado en la unidad correcta.\n"
CLEAN_UP "\
Para completar la instalaci�n de DOS, deje mas espacio libre en disco y arranque de nuevo.\n"
VDISK_ERR "Error con el disco virtual. No se puede instalar DOS.\n"
MISSING_FILE "Error: No encontr� %1\n"
INTERNAL_ERR "Error interno de dosinstall %1\n"
EXPANDING "Ampliando archivos de DOS.\n"
CONFIGURING "Configurando archivos de DOS.\n"
DRIVEA_NAME "A"
DRIVEB_NAME "B"
DOSINST_NDISKS_Q "N�mero de disquetes %1 de su conjunto: "
DOSINST_0_9 "Escriba un n�mero de 0 a 9\n"
DOSINST_0_ABORT "Si escribe 0 se abortar� la instalaci�n de DOS.\n"
FROM_BUILTIN "(desde archivos internos)"
FROM_FLOP "(desde disquetes)"
MIN_SYSTEM "sistema m�nimo"
DOSINST_PLS_ENTER "Pulse %1\n"
MISSING_SET "Faltan los archivos 'conjunto'\n"
CREATE_BOOT_FAIL "\
Advertencia: Fall� la creaci�n del nuevo archivo boot.dsk.\n\
\             Utilizando el archivo boot.dsk existente.\n"
BOOT_TOOSMALL "\
Advertencia: Imposible crear un disco virtual lo suficientemente grande\n\
\             para inicializar todas las funcionalidades de DOS NLS, por lo que se est�n desactivando\n\
\             las funcionalidades autom�ticas de DOS NLS.\n"

$domain LCC.MERGE.UNIX.CHECKMERGE
$ Messages for the "checkmerge" script.
USAGE "USO: %1\n"
MSG_1 "Comparando archivo de sumas de comprobaci�n para\n"
MSG_2 "con valores retenidos en %1...\n"
MSG_3 "se ha modificado desde la �ltima instalaci�n de Merge (Fusi�n).\n"
MSG_4 "Imposible recopilar la suma de comprobaci�n en %1.\n\
Continuando...\n"
MSG_5 "Verificaciones de archivo realizadas.\n"

$domain LCC.MERGE.UNIX.LPINSTALL
$ Messages for the lpinstall script.
MAIN_MENU "\n\
\n\
\n\
\         Programa de Instalaci�n de Impresora Merge\n\
\         ------------------------------------------\n\
\n\
\         1)  Instalar impresora\n\
\         2)  Suprimir impresora\n\
\         3)  Lista de impresoras\n\
\n\
\         %1) Abandonar\n\
\n\
\         ------------------------------------------\n\
\         Pulsar opci�n: "
$ Note: QUIT_CHAR must be a single character that mean quit.  It is used
$ as the %1 in the MAIN_MENU message and is compared with what the user
$ typed as the response to the "enter option" question.
QUIT_CHAR "q"
INSTALL "\n\
\         INSTALAR\n\
\         ---------------------\n"
PRINTER "       Nombre de impresora: [%1]: "
MODEL   "        Modelo de impresora [%1]: "
DEVICE  "        Dispositivo [%1]: "
$ Note: YES_CHAR must be a single character that means yes.  It is used
$ as the %1 in the OK_INST and  %2 in OK_REMOVE messages and is compared
$ with what the user typed as the response to those messages.
YES_CHAR "y"
OK_INST  "        �Preparado para instalar? [%1]: "
OK_REMOVE "        �Preparado para suprimir %1? [%2]: "
CONTINUE "        Pulse [INTRO] para continuar"
REMOVE "\n\
\         SUPRIMIR\n\
\         ---------------------\n\
\         Nombre de impresora: "
LIST "\n\
N�mero de impresoras instaladas: %1\n\
--------------------------------------------\n"
MISSING_PROG	"Falta el programa %1.\n"
NO_LP		"El servicio de impresi�n LP podr�a no estar instalado.\n"
CANNOT_CONTINUE	"No se puede continuar.\n"

$domain LCC.MERGE.UNIX.MKIMG
$ Messages for the "mkimg" script.

CANNOT_CONTINUE	"No se puede continuar.\n"
CANNOT_COPY	"Advertencia: No se puede copiar el archivo %1.\n"
CANNOT_MAKE	"No se puede crear la imagen %1.\n"
CANNOT_MKDIR	"No se puede crear el directorio: %1\n"
CANNOT_READ	"No se puede leer el archivo %1\n"
CANNOT_WRITE	"No se pudo escribir %1\n"
CONFIG_MSG	"Configurando archivos de imagen de DOS.\n"
COPY_PROB	"Problemas al copiar archivos en disco virtual %1\n"
CREATE_NOROOM	"No hay espacio suficiente para crear el archivo %1\n"
DOS_NI		"No se ha instalado DOS\n"
FORMAT_PROB	"Problemas al dar formato al disco virtual %1\n"
IMG_MADE	"Imagen %1 creada.\n"
INCOMPATABLE	"(�Tarjeta %1 incompatible?)\n"
MAKING		"Creando imagen %1.\n"
MISSING		"Falta el archivo %1\n"
MUST_SU		"Debe ser ra�z o su para crear imagen %1.\n"
NATIVE_MUST_SU	"Debe ser ra�z o su para crear imagen nativa %1.\n"
NATIVE_TOKEN	"nativo"
NOT_MADE	"No se ha creado la imagen %1.\n"
NO_DIRECTORY	"El directorio %1 no existe.\n"
NO_IMG_MADE	"No se cre� ning�n archivo de im�genes.\n"
NO_PERMIS_TMP	"No tiene permiso para crear el archivo temporal %1\n"
NO_PERMIS_WRITE	"No tiene permiso para escribir %1\n"
NO_ROOM		"(�No hay m�s espacio?)\n"
NO_ROOM_TMP	"No hay espacio suficiente para crear archivo temporal %1\n"
OPT_NA		"Opci�n %1 no permitida.\n"
REMOVE_FAIL	"No se pudo suprimir el archivo %1\n"
SOME_NOT_MADE	"No se han creado algunos archivos de im�genes.\n"
USAGE_LINE	"USO: %1 [cga] [mda] [ d directorio ] [devicelow] [+aXXX]\n"
USING_INSTEAD	"Utilizando %1 en su lugar.\n"
USING_ONCARD	"Usando %1 ROM en la tarjeta de visualizaci�n.\n"
WRONG_DISP	"No se puede crear imagen %1 sobre la visualizaci�n del tipo %2.\n"

$domain LCC.MERGE.UNIX.S55MERGE
$ Messages for "S55merge" script.
NOT_DOSINST	"Advertencia: No se est� instalando DOS autom�ticamente.\n"
NOT_MKIMG	"Advertencia: No se est�n creando archivos de im�genes autom�ticamente.\n"
DOING_MKIMG	"Creando im�genes de DOS autom�ticamente en el fondo.\n"

$domain LCC.MERGE.UNIX.PART_SET
$ Messages for the "part_set" script.
BAD_DOSDEV	"archivo '%1' da�ado.\n"
MISSING_LINE	"Falta l�nea '%1'.\n"

$domain LCC.MERGE.UNIX.MERGEFONTMAKE
$ Messages for the "mergefontmake" script (part of X-Clients package).
MFM_USAGE	"\
USO: mergefontmake  0|1|2|3|4  [s|ML]\n\
Recompilar las fuentes de Merge (Fusi�n).\n\
\n\
Se pueden compilar de cuatro maneras diferentes, la fuente s�lo funciona\n\
cuando se ha compilado del modo esperado por el X-Server.  El primer par�metro\n\
especifica el tipo de compilaci�n de la fuente: 1, 2, 3, � 4.\n\
El par�metro '0' produce que se utilicen los valores por defecto, cualesquiera\n\
que sean.\n\
\n\
'mergefontmake' intenta averiguar qu� versi�n de compilador est� utilizando,\n\
porque las versiones X usan opciones diferentes para la compilaci�n de fuentes.\n\
(Alguna versi�n usa la opci�n '-s' en lugar de '-M' y '-L'). Para forzar el tipo\n\
de opciones que se utilizan, especifique 's' o 'ML' como segundo par�metro.\n"
MFM_MISSING	"Error: Falta %1\n"
MFM_BAD_SECONDP	"Error: El segundo par�metro debe ser 's' o 'ML' o uno sin especificar.\n"
MFM_NO_XBIN	"Error: No se puede encontrar el directorio X bin.\n"
MFM_NO_FONTDIR	"Error: No se puede determinar el directorio de fuentes.\n"
MFM_I_NOPERMIS	"Error: No tiene permiso para instalar fonts en %1\n"
MFM_01234	"Error: Debe usar 0, 1, 2, 3 � 4.\n"
MFM_U_NOPERMIS	"Error: No tiene permiso para actualizar %1\n"
MFM_FC_ERR	"Error: %1 devolvi� %2\n"
MFM_COMP_ERR	"Error al compilar %1\n"

$domain LCC.MERGE.UNIX.WINSETUP
$ Messages for the "winsetup" script.  This scripts runs when the
$ "Win_Setup" icon is used, or the user types "winsetup".
$ There are a maximum of 24 lines of message that can be printed out,
$ altough all don't have to be used.
$ Also, a single line message can be put into "SPCL_MSG", which is printed
$ out before the batch scripts are run.
$ All these messages are printed on the DOS screen, so these messages
$ must be in the appropriate DOS code page.
LINE01 "�����������������������������������������������������������������������������Ŀ"
LINE02 "�         Usar esta sesi�n de DOS para instalar y reconfigurar Windows 3.1    �"
LINE03 "�����������������������������������������������������������������������������Ĵ"
LINE04 "�                                                                             �"
LINE05 "� - Para instalar una copia personal de Windows, inserte el Disco 1           �"
LINE06 "�   de Windows y teclee lo siguiente si est� instalando desde la unidad       �"
LINE07 "�                                                                             �"
LINE08 "�                a:\\instalar                                                  �"
LINE09 "�                                                                             �"
LINE10 "�   (o b:\\instalar si est� instalando desde la unidad B:)                     �"
LINE11 "�                                                                             �"
LINE12 "�   Consulte su manual para una descripci�n paso por paso del                 �"
LINE13 "�   procedimiento de instalaci�n. Tambi�n se describen otros tipos de         �"
LINE14 "�                                                                             �"
LINE15 "� - Para reconfigurar Windows, cambie primero la unidad y el directorio al    �"
LINE16 "�   lugar de instalaci�n de Windows, y escriba luego:                         �"
LINE17 "�                                                                             �"
LINE18 "�                instalar                                                     �"
LINE19 "�                                                                             �"
LINE20 "�������������������������������������������������������������������������������"
$quote "
$domain LCC.PCI.UNIX.LSET_ESTR
$ The following messages are from lset_estr.c
LSET_ESTR0 "no es un error"
LSET_ESTR1 "error de sistema"
LSET_ESTR2 "error de par�metro"
LSET_ESTR3 "la tabla est� bloqueada/desbloqueada"
LSET_ESTR4 "no se puede bloquear/desbloquear la tabla"
LSET_ESTR5 "no se puede obtener el identificador IPC"
LSET_ESTR6 "no se puede crear/acceder a la configuraci�n de bloqueo"
LSET_ESTR7 "no se puede anexar segmento de memoria compartida"
LSET_ESTR8 "no hay espacio de memoria/tabla"
LSET_ESTR9 "la ranura especificada est� en uso"
LSET_ESTR10 "la ranura especificada no est� en uso"
LSET_ESTR11 "configuraci�n de bloqueo ya existe"
LSET_ESTR12 "permiso denegado"
LSET_ESTR13 "error LSERR desconocido (%d)"

$ @(#)messages	4.5 9/6/94 14:20:37

$quote "
$domain LCC.PCI.UNIX.RLOCKSHM

$ The following messages are from rlockshm.c
RLOCKSHM6 "Config s�lo es v�lido con memoria compartida de nuevo estilo.\n"
RLOCKSHM7 "Imposible %1 memoria compartida com�n, "
RLOCKSHM8 "[error de sistema: %1].\n"
RLOCKSHM9 "%1.\n"
RLOCKSHM10 "\nInformaci�n de configuraci�n por defecto:\n\n"
RLOCKSHM11 "\tdirecci�n de anexo de segmento\t\t(programa seleccionado)\n"
RLOCKSHM12 "\tdirecci�n de anexo de segmento\t\t%1\n"
RLOCKSHM14 "\tclave de memoria compartida\t\t%1\n"
RLOCKSHM15 "\tclave lockset\t\t\t%1\n"
RLOCKSHM17 "\tentradas de tabla de archivos abiertos\t\t%1\n"
RLOCKSHM18 "\tentradas de tabla de encabezados de archivos\t%1\n"
RLOCKSHM19 "\tentradas de tabla de archivos actualizados\t%1\n"
RLOCKSHM20 "\tentradas de tabla de bloqueo de registros\t%1\n"
RLOCKSHM22 "\tbloqueos de registro individuales\t\t%1\n"
RLOCKSHM24 "Estos datos pueden cambiarse por estos nombres"
RLOCKSHM25 " de par�metros configurables:\n"
RLOCKSHM27 "\t%1 - %2\n"
RLOCKSHM28 "\nInformaciones varias sobre memoria existente:\n\n"
RLOCKSHM29 "\tabrir entradas de tabla de archivos\t\t%1 @ %2 bytes\n"
RLOCKSHM30 "\tentradas de tabla de encabezados de archivos\t%1 @ %2 bytes\n"
RLOCKSHM31 "\tentradas de tabla de archivos actualizados\t%1 @ %2 bytes\n"
RLOCKSHM32 "\tregistrar entradas de tabla de bloqueo\t%1 @ %2 bytes\n"
RLOCKSHM34 "\tabrir base de segmentos\t\t%1\n"
RLOCKSHM35 "\tbase de segmentos de archivos\t\t%1\n"
RLOCKSHM36 "\tbloquear base de segmentos\t\t%1\n"
RLOCKSHM38 "\tabrir tabla de archivos\t\t\t%1\n"
RLOCKSHM39 "\ttabla de encabezados de archivos\t\t%1\n"
RLOCKSHM40 "\ttabla de archivos actualizada\t\t%1\n"
RLOCKSHM41 "\tregistrar tabla de bloqueo\t\t%1\n"
RLOCKSHM43 "\tabrir lista libre de tablas\t\t%1\n"
RLOCKSHM44 "\tlista libre de tablas de archivos\t\t%1\n"
RLOCKSHM45 "\tlista libre de tablas de bloqueo\t\t%1\n"
RLOCKSHM46 "\tabrir entradas de tabla de archivos\t\t%1 @ %2 bytes\n"
RLOCKSHM47 "\tentradas de tabla de encabezados de archivos\t%1 @ %2 bytes\n"
RLOCKSHM48 "\tentradas de tabla de archivos actualizados\t%1 @ %2 bytes\n"
RLOCKSHM49 "\tregistrar entradas de tabla de bloqueo\t%1 @ %2 bytes\n"
RLOCKSHM50 "\ttama�o total de segmento\t\t%1 bytes\n"
RLOCKSHM52 "\tbloqueos de registro individuales\t\t%1\n"
RLOCKSHM54 "\tbase de anexo\t\t\t%1"
RLOCKSHM55 " (programa seleccionado)"
RLOCKSHM57 "\tabrir tabla de archivos\t\t\t%1\n"
RLOCKSHM58 "\ttabla de encabezados de archivos\t\t%1\n"
RLOCKSHM59 "\ttabla de archivos actualizada\t\t%1\n"
RLOCKSHM60 "\tregistrar tabla de bloqueo\t\t%1\n"
RLOCKSHM62 "\tabrir �ndice de listas libres de tablas\t%1\n"
RLOCKSHM63 "\t�ndice de listas libres de tablas\t%1\n"
RLOCKSHM64 "\tbloquear �ndice de listas libres de tablas\t%1\n"
RLOCKSHM65 "\tfactor de restricci�n de alineamiento\t%1\n"
RLOCKSHM66 "\nEntradas de tabla de encabezado de archivo:\n"
RLOCKSHM100 "entrada"
RLOCKSHM101 "abrir-lista"
RLOCKSHM102 "bloquear-lista"
RLOCKSHM103 "actualizar-enlace"
RLOCKSHM104 "Identificador-�nico"
RLOCKSHM105 "abrir-�ndice"
RLOCKSHM106 "bloquear-�ndice"
RLOCKSHM70 "\nEntradas actualizadas de la tabla de encabezados de archivos:\n"
RLOCKSHM200 "entrada"
RLOCKSHM201 "encabezado-archivo"
RLOCKSHM75 "\nEntradas de tabla de bloqueo de registro:\n"
RLOCKSHM111 "bloqueo-siguiente"
RLOCKSHM112 "bloqueo-bajo"
RLOCKSHM113 "bloqueo-alto"
RLOCKSHM114 "Identificador-sesi�n"
RLOCKSHM115 "DOS-PID"
RLOCKSHM116 "�ndice-siguiente"
RLOCKSHM80 "\nAbrir entradas de tabla de archivos:\n"
RLOCKSHM120 "abrir-siguiente"
RLOCKSHM121 "encabezado"
RLOCKSHM122 "acc"
RLOCKSHM123 "denegar"
RLOCKSHM130 "fhdr-�ndice"
RLOCKSHM131 "archivo-desc"
RLOCKSHM_OLDST	"ADVERTENCIA: Utilizando memoria de estilo antiguo -- ahora obsoleta\n"
RLOCKSHM_USAGE	"\nuso: %1 [-cdhmrAFHLOV] [nombre=datos] ...\n"
RLOCKSHM_DETAIL "\
\  -c  Crea el segmento de memoria compartida.\n\
\  -d  Muestra (s�lo) la configuraci�n por defecto.\n\
\  -h  Muestra (s�lo) esta informaci�n.\n\
\  -m  Muestra la informaci�n de los distintos  segmentos existentes.\n\
\  -r  Suprime el segmento de memoria compartida.\n\
\  -A  Muestra todas las entradas (incluso las que no se usan).\n\
\  -F  Muestra la tabla de encabezado del archivo.\n\
\  -H  Muestra la tabla hashed de encabezado del archivo.\n\
\  -L  Muestra la tabla de bloqueo del registro.\n\
\  -O  Muestra la tabla del archivo abierto.\n\
\  -V  Imprime la versi�n/copyright (s�lo).\n\
\  nombre=datos -Cero o m�s definiciones de configuraci�n.\n\n"

$ The following messages are from set_cfg.c
$quote "
SET_CFG1 "Cadena de configuraci�n incorrecta: '%1'\n"
SET_CFG2 "No se proporciona ning�n nombre para '%1'.\n"
SET_CFG3 "El nombre en '%1' es demasiado largo.\n"
SET_CFG4 "No se suministra ning�n dato para '%1'.\n"
SET_CFG5 "Nombre desconocido: '%1'\n"
SET_CFG6 "Los datos para '%1' deben ser positivos.\n"
SET_CFG7 "Los datos para '%1' son incorrectos.\n"
SET_CFG8 "Los datos para '%1' deben ser menores que %2.\n"

$ The following are handcrafted from set_cfg.c
SET_CFG100 "base"
SET_CFG101 "direcci�n de uni�n de segmentos (0 = programa seleccionado)"
SET_CFG102 "claves"
SET_CFG103 "configure la memoria compartida y las claves lockset LSW"
SET_CFG104 "abrir-tabla"
SET_CFG105 "n�mero m�ximo de entradas abiertas de tabla de archivos"
SET_CFG106 "tabla-archivos"
SET_CFG107 "n�mero m�ximo de entradas de tabla de encabezado de archivos"
SET_CFG108 "tabla-hash"
SET_CFG109 "n�mero m�ximo de entradas actualizadas de tabla de archivo"
SET_CFG110 "tabla-bloqueada"
SET_CFG111 "n�mero m�ximo de entradas de tabla de bloqueo de registros"
SET_CFG112 "bloqueos-entrados"
SET_CFG113 "n�mero m�ximo de bloqueos de registro individuales"
$ SCCSID(@(#)messages	7.11	LCC)	/* Modified: 10:33:25 10/20/93 */

$quote "
$domain LCC.PCI.UNIX

$ In general, the names below start with some indication of the file in
$ which the string is actually used.  Usually, this is the base name of
$ the file, although it may be some "shorter" version of the name.

LICM_VIOLATION	"Infracci�n de licencia, %1 (finalizando) vs. %2\n"
LICM_TERMINATE	"Infracci�n de licencia -- finalizando servidor\n"
LICM_BAD_KEY	"Licencia incorrecta (clave de licencia err�nea)\n"
LICM_ALTERED	"Licencia incorrecta (texto alterado)\n"
LICM_NO_ID	"Licencia incorrecta (no hay ID de licencia)\n"
LICM_NO_MEMORY	"Licencia no utilizable (sin memoria)\n"
LICM_EXPIRED	"Licencia no utilizable (caducada)\n"
LICM_NO_SERIAL	"No se ha especificado n�mero de serie del cliente\n"
LICM_BAD_SERIAL	"El n�mero de serie del cliente especificado no es v�lido\n"

LICU_DUPLICATE	"N�mero de serie del cliente duplicado, conexi�n no permitida."
LICU_INVALID	"El n�mero de serie del cliente no es v�lido, conexi�n no permitida."
LICU_RESOURCE	"El recurso host no est� disponible, consulte al administrador del sistema."

$ %1 is the maximum number of clients
LICU_LIMIT	"Este servidor est� al l�mite de la licencia de su cliente (%1), no puede conectarse."

$ %1 is the log file name, %2 is the error string, %3 and %4 (where used) are
$ the user and group IDs, respectively
LOG_OPEN	"Imposible abrir archivo de registro '%1', %2\n"
LOG_CHMODE	"Imposible cambiar modo de '%1', %2\n"
LOG_CHOWN	"Imposible cambiar propietario de '%1' a %3/%4, %2\n"
LOG_REOPEN	"Imposible reabrir '%1' despu�s de truncado, %2\n"

$ %1 is a host file descriptor
LOG_OPEND	"Imposible reabrir registro desde descriptor %1\n"

$ %1 is the program name, %2 is the process id
LOG_SERIOUS	"Error grave en %1 (PID %2), se intentar� continuar\n"
LOG_FATAL	"Error fatal en %1 (PID %2), imposible continuar\n"

$ %1 is a number of bytes
MEM_NONE	"Imposible asignar %1 bytes de memoria\n"
MEM_RESIZE	"Imposible cambiar el tama�o de la memoria a %1 bytes\n"

$ %1 is the host name
NETAD_NOHOST	"Imposible encontrar datos sobre el host '%1'\n"

$ %1 is the error string
NETIF_DEVACC	"Imposible acceder al dispositivo de red, %1\n"
NETIF_CONFIG	"Imposible determinar la configuraci�n de la red, %1\n"

NETIO_DESC_ARR	"Imposible asignar la tabla del descriptor de la red\n"
NETIO_RETRY	"Imposible recibir paquetes desde la red\n"
NETIO_CORRUPT	"La tabla de red interna ha quedado da�ada\n"
NETIO_MUXERR	"Error de multiplexi�n de red\n"

$ %1 is the size of a network packet
NETIO_PACKET	"El tama�o m�ximo del paquete de red (%1) es demasiado peque�o\n"

$ %1 is a network address
NETIO_IFERR	"Error irrecuperable en la interfaz %1\n"

DOSSVR_NO_CSVR	"Imposible encontrar la direcci�n del servidor de conexiones\n"
DOSSVR_SETIF	"Imposible abrir interfaz de red local\n"
DOSSVR_CURDIR	"Imposible determinar el directorio actual de trabajo\n"

$ %1 is the error string
DOSSVR_R_CPIPE	"Imposible leer el conducto de configuraci�n, %1\n"
DOSSVR_G_NETA	"Imposible obtener atributos de red, %1\n"
DOSSVR_S_NETA	"Imposible establecer atributos de red, %1\n"
DOSSVR_G_TERMA	"Imposible obtener atributos de l�nea de terminal, %1\n"
DOSSVR_S_TERMA	"Imposible establecer atributos de l�nea de terminal, %1\n"
DOSSVR_C_CPIPE	"Imposible crear conducto de configuraci�n, %1\n"
DOSSVR_NOFORK	"Imposible crear un nuevo proceso, %1\n"

$ %1 is an RLOCK package error, %1 is a system error
DOSSVR_RLINIT	"Imposible inicializar datos de bloqueo de registro, %1, %2\n"

$ %1 is a program name, %2 is an error string (if used)
DOSSVR_NOEXEC	"Imposible arrancar '%1', %2\n"
DOSSVR_NOSHELL	"Imposible arrancar shell de usuario '%1'\n"
DOSSVR_ACC_SVR	"Imposible acceder a servidor de DOS '%1'\n"
DOSSVR_RUN_SVR	"Imposible ejecutar servidor de DOS '%1', %2\n"

$ %1 is an luid, %2 is the error string
DOSSVR_LUID	"Imposible establecer el luid como %1, %2\n"

$ %1 is the written count, %2 is the expected count, %3 is the error string
DOSSVR_W_CPIPE	"Imposible escribir en conducto de configuraci�n (%1 bytes de %2), %3\n"

CONSVR_NOMEM	"Imposible asignar memoria para la cadena de funcionalidades\n"
CONSVR_NO_NET	"Imposible abrir la(s) interfaz(interfaces) de red\n"

$ %1 is the luid that started the consvr process
CONSVR_LUID	"El luid ya est� configurado como %1\n"

$ %1 is file or program name, %2 is error string (where used)
CONSVR_RUN_SVR	"Imposible ejecutar servidor de DOS '%1', %2\n"
CONSVR_NO_FF	"Imposible abrir el archivo de funcionalidades '%1'\n"
CONSVR_ERR_FF	"Error en el archivo de funcionalidades '%1'\n"

$ %1, %2, %3 and %4 are the major, minor, sub-minor and special version ID
$ values, respectively
CONSVR_BANNER		"Interfaz de PC para DOS, versi�n %1.%2.%3 %4\n"
CONSVR_BANNER_AADU	"Servidor de DOS para AIX, versi�n %1.%2\n"

$ %1 is error string
IPC_NO_MSGQ	"Imposible crear una cola de mensajes, %1\n"
IPC_NO_SEM	"Imposible crear un sem�foro, %1\n"

MAPSVR_NO_NET	"Imposible abrir la(s) interfaz(interfaces) de red\n"

DOSOUT_SEGMENT	"Imposible acceder a segmento de memoria compartida RD\n"
DOSOUT_REXMIT	"Demasiadas retransmisiones\n"

$ %1 is an error string
DOSOUT_NO_SHM	"No hay memoria compartida, %1\n"
DOSOUT_S_NETA	"Imposible establecer atributos de red, %1\n"
DOSOUT_PIPE_ACK	"Error de conducto E/S (espera ACK), %1\n"
DOSOUT_PIPE_CNT	"Error de conducto E/S (control ACK), %1\n"
DOSOUT_ERR6	"Error 6 de lectura PTY: %1\n"
DOSOUT_ERR7	"Error 7 de lectura PTY: %1\n"
DOSOUT_ERR8	"Error 8 de lectura PTY: %1\n"
DOSOUT_ERR13	"Error 13 de lectura PTY: %1\n"
DOSOUT_ERR14	"Error 14 de lectura PTY: %1\n"
DOSOUT_ERR19	"Error 19 de lectura PTY: %1\n"
DOSOUT_ERR9	"Error 9 de escritura TTY: %1\n"
DOSOUT_ERR10	"Error 10 de escritura TTY: %1\n"
DOSOUT_ERR15	"Error 15 de escritura TTY: %1\n"
DOSOUT_ERR16	"Error 16 de escritura TTY: %1\n"
DOSOUT_ERR17	"Error 17 de escritura TTY: %1\n"

$ %1 is an error string
SEMFUNC_NOSEM	"Imposible crear un sem�foro RD, %1\n"
SEMFUNC_NOSHM	"Imposible crear un segmento de memoria compartida RD, %1\n"

$ %1 is the number of objects in the caches, %2 is the size of each object
VFILE_NOMEM	"Imposible asignar memoria para %1 objetos, %2 bytes cada uno\n"

$ %1 is the lcs error number
NLSTAB_HOST	"Imposible acceder a tabla LCS host, error %1\n"
NLSTAB_CLIENT	"Imposible acceder a tabla LCS cliente, error %1\n"
NLSTAB_SET	"Imposible establecer las tablas LCS, error %1\n"

DEBUG_CHILD	"hijo"
DEBUG_OFF	"Inactivo"
DEBUG_ON	"Activo"

$ %1 is the program name
DEBUG_USAGE	"Uso: %1 <PID> [[op]canales] [hijo] [on] [off]\n"

$ %1 is the program name, %2 is the faulty command line argument
DEBUG_ARG	"%1: Argumento no v�lido: '%2'\n"

$ %1 is the program name, %2 is the channel file, %3 is an error string
DEBUG_NOFILE	"%1: Imposible crear archivo de canal '%2', %3\n"

$ %1 is the program name, %2 is a PID, %3 is an error string
DEBUG_INVAL	"%1: El proceso %2 no puede se�alizarse, %3\n"
DEBUG_NO_SIG	"%1: Imposible se�alizar proceso %2, %3\n"

$ %1 is the program name, %2 is a channel number specified in the argument list
DEBUG_BADBIT	"%1: El canal #%2 est� fuera de rango\n"
$ SCCSID("@(#)messages	7.2	LCC")	/* Modified: 1/19/93 20:08:07 */
$domain LCC.PCI.DOS.CONVERT
$quote "
$ NOTE: '\n' indicates that a new line will be printed
$ The following messages are from convert.c
CONVERT1 "May�sculas y Min�sculas AMBAS especificadas\n"
CONVERT1A "Las opciones mencionadas son incompatibles\n"
CONVERT2 "Transacciones especificadas en conflicto\n"

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
CONVERT_M5_D "\                /c c    Usa c como el user_char de fallo de traducci�n.\n\
\                /m      Permite la traducci�n de caracteres m�ltiples.\n\
\                /a      Aborta al producirse un fallo de traducci�n.\n\
\                /s      Usa la mejor traducci�n de car�cter �nico.\n\  
\                /z      Hace uso apropiado de CTL-Z.\n\
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
CONVERT_M5_U "\                -c c    Usa c como el user_char de fallo de traducci�n.\n\
\                -m      Permite la traducci�n de caracteres m�ltiples.\n\
\                -a      Aborta al producirse un fallo de traducci�n.\n\
\                -s      Usa la mejor traducci�n de car�cter �nico.\n\
\                -z      Hace uso apropiado de CTL-Z.\n\
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
