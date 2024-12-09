PKGS=xft x11 xfixes
CFLAGS=`pkg-config --cflags ${PKGS}`
LIBS=`pkg-config --libs ${PKGS}`

activate-linux: main.c
	cc ${CFLAGS} -o activate-linux main.c ${LIBS}
