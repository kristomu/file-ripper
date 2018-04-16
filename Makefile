
CC = g++
OPT = -O9 --std=c++11 -Werror
FAXX = -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -D_LARGEFILE_SOURCE

#DBG = -ggdb -pg

ripper: main.cc
	${CC} ${CFLAGS} ${OPT} ${FAXX} main.cc -o ripper
