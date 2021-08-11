ser:ser.o 
	gcc ser.o -lsqlite3 -pthread -o ser
ser.o:ser.c
	gcc -c ser.c -o ser.o

cli:cli.o 
	gcc cli.o -o cli
cli.o:cli.c
	gcc -c cli.c -o cli.o


.PHONY:clean
clean:
	rm *.o ser  personnel.db
