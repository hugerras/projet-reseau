// Programme Tchat Socket TCP (côté CLIENT)

/* Appel des Librairies */
#include <stdio.h> // Pour printf()
#include <stdlib.h> // Pour exit()
#include <string.h> // Pour memset()
#include <poll.h> // Pour poll()
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> // Pour struct sockaddr_in
#include <arpa/inet.h> // Pour htons et inet_aton

/* Déclaration des Définitions */
#define PORT IPPORT_USERRESERVED // = 5000
#define LG_MAX_MESSAGE 256
#define LG_MAX_LOGIN 50
#define NB_MAX_USERS 10

/* Fonction principale */
int main() {

	int descripteurSocket;
	struct sockaddr_in pointDeRencontreDistant;
	socklen_t longueurAdresse;
	char messageEnvoi[LG_MAX_MESSAGE]; // Message de la couche Application
	char messageRecu[LG_MAX_MESSAGE]; // Message de la couche Application
	int ecrits, lus; // Nb d’octets écrits et lus
	int retour;
	struct pollfd pollfds[2];

	// Crée un socket de communication
	// 0 indique que l’on utilisera le protocole par défaut associé à SOCK_STREAM soit TCP
	descripteurSocket = socket(PF_INET, SOCK_STREAM, 0);

	// Teste la valeur renvoyée par l’appel système socket()
	// Si échec
	if (descripteurSocket < 0) {
		perror("socket"); // Affiche le message d’erreur
		exit(-1); // On sort en indiquant un code erreur
	}

	printf("Socket créée avec succès ! (%d)\n", descripteurSocket);

	// Obtient la longueur en octets de la structure sockaddr_in
	longueurAdresse = sizeof(pointDeRencontreDistant);

	// Initialise à 0 la structure sockaddr_in
	memset(&pointDeRencontreDistant, 0x00, longueurAdresse);

	// Renseigne la structure sockaddr_in avec les informations du serveur distant
	pointDeRencontreDistant.sin_family = PF_INET;

	// On choisit le numéro de port d’écoute du serveur (= 5000)
	pointDeRencontreDistant.sin_port = htons(IPPORT_USERRESERVED);

	// On choisit l’adresse IPv4 du serveur
	inet_aton("10.0.2.15", &pointDeRencontreDistant.sin_addr); // A modifier selon ses besoins

	// Débute la connexion vers le processus serveur distant
	if ((connect(descripteurSocket, (struct sockaddr *)&pointDeRencontreDistant,longueurAdresse)) == -1) {
		perror("connect"); // Affiche le message d’erreur
		close(descripteurSocket); // On ferme la ressource avant de quitter
		exit(-2); // On sort en indiquant un code erreur
	}

	printf("Connexion au serveur réussie avec succès !\n");

	// Initialise à 0 les messages
	memset(messageEnvoi, 0x00, LG_MAX_MESSAGE*sizeof(char));
	memset(messageRecu, 0x00, LG_MAX_MESSAGE*sizeof(char));

	while (1) {
		int nevents, i, j;
		int nfds = 0;

		// Socket à écouter
		pollfds[nfds].fd = descripteurSocket;
		pollfds[nfds].events = POLLIN;
		pollfds[nfds].revents = 0;

		nfds++;

		// Message utilisateur à écouter
		pollfds[nfds].fd = stdin;
		pollfds[nfds].events = POLLIN;
		pollfds[nfds].revents = 0;

		/* Structure Pollfd pour information uniquement
		struct pollfd {
			int fd; // File Descriptor
			short events; // Requested Events
			short revents; // Returned Events
		}; */

		// Demander à poll s'il a vu des évènements
		nevents = poll(pollfds, nfds, -1);

		// Si poll a vu des évènements
		if (nevents > 0) {
			
			if (pollfds[0].revents != 0) {
				// Descripteur Socket
			}

			else if (pollfds[1].revents != 0) {
				// On réceptionne les données du client
				lus = read(pollfds[1].fd, messageEnvoi, LG_MAX_MESSAGE*sizeof(char));
				
				if (getchar() == '\n'){
					printf("\nMessage : %s\n", messageEnvoi);
				}
			}
			
		} else {
			printf("poll() a renvoyé %d\n", nevents);
		}
	
	}

	// Envoie un message au serveur
	sprintf(messageEnvoi, "Hello world !\n");
	ecrits = write(descripteurSocket, messageEnvoi, strlen(messageEnvoi)); // Message à TAILLE variable

	switch (ecrits) {
		case -1 : /* Une erreur */
			perror("write");
			close(descripteurSocket);
			exit(-3);
			
		case 0 : /* La socket est fermée */
			fprintf(stderr, "La socket a été fermée par le serveur !\n\n");
			close(descripteurSocket);
			return 0;

		default: /* Envoi de n octets */
			printf("Message %s envoyé avec succès (%d octets)\n\n", messageEnvoi, ecrits);
	}

	// Réception des données du serveur
	lus = read(descripteurSocket, messageRecu, LG_MAX_MESSAGE*sizeof(char)); // Attend un messagede TAILLE fixe

	switch (lus) {
		case -1 : /* Une erreur */
			perror("read");
			close(descripteurSocket);
			exit(-4);

		case 0 : /* La socket est fermée */
			fprintf(stderr, "La socket a été fermée par le serveur !\n\n");
			close(descripteurSocket);
			return 0;
			
		default : /* Réception de n octets */
			printf("Message reçu du serveur : %s (%d octets)\n\n", messageRecu, lus);
	}

	// On ferme la ressource avant de quitter
	close(descripteurSocket);

	return 0;
}

void viderBuffer() {
    int c = 0;

    while (c != '\n' && c != EOF) {
        c = getchar();
    }
}
