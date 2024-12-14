PKGS=xft x11 xfixes
CFLAGS=`pkg-config --cflags ${PKGS}`
LIBS=`pkg-config --libs ${PKGS}`

activate-linux: src/main.c src/flag.h
	cc ${CFLAGS} -o activate-linux src/main.c ${LIBS}
