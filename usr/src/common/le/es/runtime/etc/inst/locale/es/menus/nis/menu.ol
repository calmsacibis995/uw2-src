.ul
Instalaci�n y configuraci�n del servicio de informaci�n de red (NIS)
.lr
F1=Ayuda
.top
`echo  "La siguiente configuraci�n de NIS ya existe en esta m�quina."`
`echo "\n \n \n"`
`[ "$TYPE" = "1" ] && echo "    Esta m�quina es un servidor maestro NIS.\n \n"`
`[ "$TYPE" = "2" ] && echo "    Esta m�quina es un servidor esclavo NIS.\n \n"`
`[ "$TYPE" = "3" ] && echo "    Esta m�quina es un cliente NIS.\n \n"`
`echo "    Dominio NIS: $def_dom"` 
`echo "\n \n    Los servidores NIS para el dominio $def_dom son: \c" && cat /tmp/nis.overlay | xargs echo`
.button
Aplicar
Redefinir
.bottom
Pulse <Tab> para mover el cursor entre los campos. Al terminar, mueva el
cursor a "Aplicar" y pulse <Intro> para continuar.
.form
2 2//Yes::S�//No::No//�Usar configuraci�n NIS actual?://USE_CURRENT//
//Teclee flecha derecha para escoger S� y usar la config. actual NIS//
.help
Este computador ya se ha configurado con NIS.  Esta instalaci�n puede 
continuar usando la configuraci�n de NIS actual (teclee S�), o 
tambi�n es posible crear una nueva configuraci�n mediante el 
gui�n de instalaci�n de NIS (Teclee No).

Nota: Este men� s�lo muestra los tres primeros servidores NIS 
      configurados para este dominio.
.helpinst
Supr=Cancelar F1=Ayuda ESC=Salir de Ayuda 1=Av P�g 2=Re P�g
