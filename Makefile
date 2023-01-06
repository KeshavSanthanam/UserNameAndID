CC=/usr/bin/gcc
SRC=`pwd`
LIB=-pthread
CC_OPTS=-Wall -Werror -std=gnu99

all: dbclient dbserver

dbclient: dbclient.c
	echo "compiling dbclient ..."
	${CC} ${SRC}/dbclient.c -o dbclient ${CC_OPTS} ${LIB} 

dbserver: dbserver.c
	echo "compiling dbserver ..."
	${CC} ${SRC}/dbserver.c -o dbserver ${CC_OPTS} ${LIB} 

clean:
	rm ${SRC}/dbclient ${SRC}/dbserver
