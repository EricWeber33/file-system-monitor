all: notapp
serial.o: serialfunctions.c
	gcc -c -std=gnu99 serialfunctions.c -o serial.o
server.o: server.c
	gcc -c -std=gnu99 server.c -o server.o
t_server.o: t_server.c
	gcc -c -std=gnu99 t_server.c -o t_server.o
observer.o: observer.c
	gcc -c -std=gnu99 observer.c -o observer.o
user.o: user.c
	gcc -c -std=gnu99 user.c -o user.o
notapp: notapp.c server.o observer.o user.o serial.o
	gcc -std=gnu99 notapp.c server.o observer.o user.o serial.o -o notapp -lpthread
notapp.time: notapp.c t_server.o observer.o user.o serial.o
	gcc -std=gnu99 notapp.c t_server.o observer.o user.o serial.o -o notapp.time -lpthread
clean:
	rm *.o notapp notapp.time
