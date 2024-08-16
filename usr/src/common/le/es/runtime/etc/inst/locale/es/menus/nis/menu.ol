.ul
Instalación y configuración del servicio de información de red (NIS)
.lr
F1=Ayuda
.top
`echo  "La siguiente configuración de NIS ya existe en esta máquina."`
`echo "\n \n \n"`
`[ "$TYPE" = "1" ] && echo "    Esta máquina es un servidor maestro NIS.\n \n"`
`[ "$TYPE" = "2" ] && echo "    Esta máquina es un servidor esclavo NIS.\n \n"`
`[ "$TYPE" = "3" ] && echo "    Esta máquina es un cliente NIS.\n \n"`
`echo "    Dominio NIS: $def_dom"` 
`echo "\n \n    Los servidores NIS para el dominio $def_dom son: \c" && cat /tmp/nis.overlay | xargs echo`
.button
Aplicar
Redefinir
.bottom
Pulse <Tab> para mover el cursor entre los campos. Al terminar, mueva el
cursor a "Aplicar" y pulse <Intro> para continuar.
.form
2 2//Yes::Sí//No::No//¿Usar configuración NIS actual?://USE_CURRENT//
//Teclee flecha derecha para escoger Sí y usar la config. actual NIS//
.help
Este computador ya se ha configurado con NIS.  Esta instalación puede 
continuar usando la configuración de NIS actual (teclee Sí), o 
también es posible crear una nueva configuración mediante el 
guión de instalación de NIS (Teclee No).

Nota: Este menú sólo muestra los tres primeros servidores NIS 
      configurados para este dominio.
.helpinst
Supr=Cancelar F1=Ayuda ESC=Salir de Ayuda 1=Av Pág 2=Re Pág
