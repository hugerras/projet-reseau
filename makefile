serveur : serveur-tchat.o
	gcc -o serveur serveur-tchat.o
	rm *.o

serveur-tchat.o : serveur-tchat.c
	gcc -g -Wall -c serveur-tchat.c
