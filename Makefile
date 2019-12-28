all:
	gcc	-Wall	-g	-o	rserver	src/server.c	src/diewithmessage.c	src/optparser.c	src/assignment3.c	-lpthread
	
