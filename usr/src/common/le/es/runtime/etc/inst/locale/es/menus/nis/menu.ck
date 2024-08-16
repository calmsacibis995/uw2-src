.ul
Instalaci�n y configuraci�n del servicio de informaci�n de red (NIS)
.lr
F1=Ayuda
.top
`echo  "\n \nHa escogido la siguiente configuraci�n:"`
`echo  "\n \n  $host es el NIS $host_type"` 
`echo  "\n \n  dominio NIS: $def_dom"`
`[ "$slavep" = "F" ] && echo  "\n \n  Los servidores NIS del dominio $def_dom son: $SERV1"`
`[ "$slavep" = "F" ] && echo  "                                       $SERV2"`
`[ "$slavep" = "F" ] && echo  "                                       $SERV3"`
`[ "$slavep" = "T" ] && echo  "\n \n  El servidor maestro NIS del dominio $def_dom es: $master"`
`echo   "\n \nTeclee No para volver a configurar o para cancelar la configuraci�n de\nNIS."`
.button
Aplicar
Redefinir
.bottom
Pulse <Tab> para mover el cursor entre los campos. Al terminar, mueva el
cursor a "Aplicar" y pulse <Intro> para continuar.
.form
2 2//No::No//Yes::S�//�Aceptar configuraci�n NIS?://ACCEPT//
//Teclee la flecha derecha para escoger Si y aceptar la configuraci�n//
.help
Si esta configuraci�n de NIS no es aceptable, introduzca "No" y
dispondr� de la oportunidad de volver a configurar el NIS o de cancelar
la configuraci�n en el men� subsiguiente.  Independientemente de si
cancela la configuraci�n de NIS, NIS se instalar� igualmente.

Nota: Este men� s�lo muestra los tres primeros servidores NIS 
      configurados para este dominio.
.helpinst
Supr=Cancelar F1=Ayuda ESC=Salir de Ayuda 1=Av P�g 2=Re P�g
