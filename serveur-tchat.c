// Programme Tchat Socket TCP (côté SERVEUR)

/* Appel des Librairies */
#include <stdio.h> // Pour printf()
#include <stdlib.h> // Pour exit()
#include <unistd.h> // Pour close() et sleep()
#include <string.h> // Pour memset()
#include <poll.h> // Pour poll()
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // Pour struct sockaddr_in
#include <arpa/inet.h> // Pour htons et inet_aton

/* Déclaration des Définitions */
#define PORT IPPORT_USERRESERVED // = 5000
#define LG_MESSAGE 256
#define MAX_USERS 10

/* Fonction principale */
int main() {
	
	int socketEcoute;
	struct sockaddr_in pointDeRencontreLocal;
	socklen_t longueurAdresse;
	int socketDialogue;
	struct sockaddr_in pointDeRencontreDistant;
	char messageEnvoi[LG_MESSAGE]; // Message de la couche Application
	char messageRecu[LG_MESSAGE]; // Message de la couche Application
	int ecrits, lus; // Nb d'octets écrits et lus
	int retour;
	struct user users[MAX_USERS];
	struct pollfd pollfds[MAX_USERS + 1];

	memset(users, '\0', MAX_USERS*sizeof(struct user));

	// Crée un socket de communication
	// 0 indique que l'on utilisera le protocole par défaut associé à SOCK_STREAM soit TCP
	socketEcoute = socket(PF_INET, SOCK_STREAM, 0);

	// Teste la valeur renvoyée par l'appel système socket()
	// Si échec
	if (socketEcoute < 0) {
		perror("socket"); // Affiche le message d'erreur
		exit(-1); // On sort en indiquant un code erreur
	} 

	printf("Socket créée avec succès ! (%d)\n", socketEcoute);

	// On prépare l'adresse d'attachement locale
	longueurAdresse = sizeof(struct sockaddr_in);
	memset(&pointDeRencontreLocal, 0x00, longueurAdresse);
	pointDeRencontreLocal.sin_family = PF_INET;
	pointDeRencontreLocal.sin_addr.s_addr = htonl(INADDR_ANY); // Toutes les interfaces locales disponibles
	pointDeRencontreLocal.sin_port = htons(PORT); // = 5000

	// On demande l'attachement local de la socket
	if ((bind(socketEcoute, (struct sockaddr *)&pointDeRencontreLocal, longueurAdresse)) < 0) {
		perror("bind");
		exit(-2);
	}

	printf("Socket attachée avec succès !\n");

	// On fixe la taille de la file d'attente à 5 (pour les demande de connexion non encore traitées)
	if (listen(socketEcoute, 5) < 0) {
		perror("listen");
		exit(-3);
	}

	printf("Socket placée en écoute passive ...\n");

	// Boucle d'attente de connexion : en théorie, un serveur attend indéfiniment
	while(1) {

		int nevents, i, j;
		int nfds = 0;

		// Liste des sockets à écouter
		// socketEcoute + users[].socket => pollfds[]

		nevents = poll(pollfds, nfds, -1);

		if (nevents > 0) {
			// Parcours de pollfds[] à la recherche des revents != 0
			//
			// Si c'est la socket socketEcoute => accept() + création d'une nouvelle entrée dans la table users[]
			//
			// Sinon c'est une socket client => read() et gestion des erreurs pour le cas de la déconnexion
		} else {
			printf("poll() returned %d\n", nevents);
		}
	}

	// On ferme la ressource avant de quitter   
	close(socketEcoute);

	return 0;
}
