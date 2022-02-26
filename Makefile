all: serveur1-ElYouDP serveur2-ElYouDP serveur3-ElYouDP

serveur1-ElYouDP: serveur1-ElYouDP.o
		gcc serveur1.ElYouDP.o -o serveur1-ElYouDP
serveur1-ElYouDP.o:	serveur1-ElYouDP.c
		gcc -c serveur1-ElYouDP.c -o serveur1.ElYouDP.o

serveur2-ElYouDP: serveur2-ElYouDP.o
		gcc serveur2.ElYouDP.o -o serveur2-ElYouDP
serveur2-ElYouDP.o:	serveur2-ElYouDP.c
		gcc -c serveur2-ElYouDP.c -o serveur2.ElYouDP.o


serveur3-ElYouDP: serveur3-ElYouDP.o
		gcc serveur3.ElYouDP.o -o serveur3-ElYouDP
serveur3-ElYouDP.o:	serveur3-ElYouDP.c
		gcc -c serveur3-ElYouDP.c -o serveur3.ElYouDP.o

clean:
	rm *.o serveur1-ElYouDP serveur2-ElYouDP serveur3-ElYouDP