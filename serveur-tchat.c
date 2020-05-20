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
#define LG_MAX_MESSAGE 256
#define LG_MAX_LOGIN 50
#define NB_MAX_USERS 10

/* Déclaration des Structures */
// Structure Informations Utilisateur
struct user {
	int socket;
	char login[LG_MAX_LOGIN];
};

/* Déclaration des Fonctions */
int main();
int gestionCommandeUser();
void afficherMessageATous();

/* Fonction principale */
int main() {
	
	int socketEcoute;
	struct sockaddr_in pointDeRencontreLocal;
	socklen_t longueurAdresse;
	int socketDialogue;
	struct sockaddr_in pointDeRencontreDistant;
	char messageEnvoi[LG_MAX_MESSAGE]; // Message de la couche Application
	char messageRecu[LG_MAX_MESSAGE]; // Message de la couche Application
	int ecrits, lus; // Nb d'octets écrits et lus
	int retour;
	struct user users[NB_MAX_USERS];
	struct pollfd pollfds[NB_MAX_USERS + 1];

	memset(users, '\0', NB_MAX_USERS*sizeof(struct user));

	// Crée un socket de communication
	// 0 indique que l'on utilisera le protocole par défaut associé à SOCK_STREAM soit TCP
	socketEcoute = socket(PF_INET, SOCK_STREAM, 0);

	// Teste la valeur renvoyée par l'appel système socket()
	// Si échec...
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

	printf("Socket placée en écoute passive...\n");

	// Boucle d'attente de connexion : en théorie, un serveur attend indéfiniment
	while(1) {

		int nevents, i, j;
		int nfds = 0;

		// Liste des sockets à écouter
		pollfds[nfds].fd = socketEcoute;
		pollfds[nfds].events = POLLIN;
		pollfds[nfds].revents = 0;

		nfds++;

		// Remplissage du tableau pollfds avec les infos des utilisateurs actuellement connectés
		for (i = 0; i < NB_MAX_USERS; i++) {
			if (users[i].socket != 0) {
				pollfds[nfds].fd = users[i].socket;
				pollfds[nfds].events = POLLIN;
				pollfds[nfds].revents = 0;

				nfds++;
			}
		}

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
			for (i = 0; i < nfds; i++) {
				if (pollfds[i].revents != 0) {
					// S'il s'agit d'un évènement de la socket d'écoute (= nouvel utilisateur)
					if (pollfds[i].fd == socketEcoute) {
						socketDialogue = accept(socketEcoute, (struct sockaddr *)&pointDeRencontreDistant, &longueurAdresse);

						if (socketDialogue < 0) {
							perror("accept");
							exit(-4);
						}

						// Ajout de l'utilisateur
						for (j = 0; j < NB_MAX_USERS; j++) {
							// S'il y a une place de libre dans le tableau des utilisateurs connectés, on ajoute le nouvel utilisateur au tableau
							if (users[j].socket == 0) {
								users[j].socket = socketDialogue;
								snprintf(users[j].login, LG_MAX_LOGIN, "anonymous%d", socketDialogue);
								printf("[INFO] Ajout de l'utilisateur %s en position %d\n", users[j].login, j);

								char message[LG_MAX_MESSAGE];

								// Initialiser la variable message
								memset(message, '\0', LG_MAX_MESSAGE*sizeof(char));

								strcpy(message, "vient de rejoindre le tchat !\n");
								afficherMessageATous(users, users[j].login, users[j].socket, message);

								break;
							}

							// Si aucune place n'a été trouvée
							if (j == NB_MAX_USERS) {
								printf("[INFO] Plus de place de disponible pour ce nouvel utilisateur\n");
								close(socketDialogue);
							}
						}
					}
					
					// Sinon, il s'agit d'un évènement d'un utilisateur (message, commande, etc)
					else {
						// On cherche quel utilisateur a fait la demande grâce à sa socket
						for (j = 0; j < NB_MAX_USERS; j++) {
							if (users[j].socket == pollfds[i].fd) {
								break;
							}
						}

						// Si aucun utilisateur n'a été trouvé
						if (j == NB_MAX_USERS) {
							printf("[INFO] Utilisateur inconnu\n");
							break;
						}
						
						// On réceptionne les données du client
						lus = read(pollfds[i].fd, messageRecu, LG_MAX_MESSAGE*sizeof(char));

						switch (lus) {
							case -1 : /* Une erreur */
								perror("read");
								exit(-5);

							case 0 : /* La socket est fermée */
								printf("[INFO] L'utilisateur %s en position %d a quitté le tchat\n", users[j].login, j);

								char message[LG_MAX_MESSAGE];

								// Initialiser la variable message
								memset(message, '\0', LG_MAX_MESSAGE*sizeof(char));

								strcpy(message, "vient de quitter le tchat !\n");
								afficherMessageATous(users, users[j].login, users[j].socket, message);

								// Réinitialisation de l'utilisateur
								memset(&users[j], '\0', sizeof(struct user));

							default: /* Réception de n octets */
								printf("[INFO] Message reçu de %s : %s (%d octets)\n", users[j].login, messageRecu, lus);

								// Test si c'est une commande
								if (strncmp(messageRecu, "/", 1) == 0) {
									gestionCommandeUser(users, users[j].login, users[j].socket, messageRecu);
								}

								// Sinon, envoyer le message à tout le monde (sauf à celui qui envoie le message) en indiquant qui c'est qui envoie ce message
								else {
									afficherMessageATous(users, users[j].login, users[j].socket, messageRecu);
								}
								
								// Initialiser la variable message reçu
								memset(messageRecu, '\0', LG_MAX_MESSAGE*sizeof(char));
						}
					}
				}
			}
			
		} else {
			printf("poll() a renvoyé %d\n", nevents);
		}
	}

	// On ferme la ressource avant de quitter   
	close(socketEcoute);

	return 0;
}

/* Fonction traitant la commande de l'utilisateur */
int gestionCommandeUser(struct user users[], char nom_user_qui_demande[], int socket_user_qui_demande, char messageRecu[]) {
	
	char messageAEnvoyer[LG_MAX_MESSAGE + 50];
	int ecrits;

	// Initialiser la variable message à envoyer
	memset(messageAEnvoyer, '\0', (LG_MAX_MESSAGE + 50)*sizeof(char));

	// Test si c'est la commande de message privé
	if (strncmp(messageRecu, "/msg ", strlen("/msg ")) == 0) {
		
		printf("[DEBUG] Commande de message privé détectée !\n");

		int i;
		char syntaxe_msg[strlen("/msg ") + LG_MAX_LOGIN];

		// Initialiser la variable syntaxe commande msg
		memset(syntaxe_msg, '\0', (strlen("/msg ") + LG_MAX_LOGIN)*sizeof(char));

		// Chercher quel utilisateur est ciblé
		for (i = 0; i < NB_MAX_USERS; i++) {
			
			strcpy(syntaxe_msg, "/msg ");
			strcat(syntaxe_msg, users[i].login);
			strcat(syntaxe_msg, " ");

			// Si on a trouvé l'utilisateur ciblé, on lui envoie le message
			if (strncmp(messageRecu, syntaxe_msg, strlen(syntaxe_msg)) == 0) {
				
				// Création du message à envoyer
				sprintf(messageAEnvoyer, "Message privé de %s : ", nom_user_qui_demande);
				strcat(messageAEnvoyer, messageRecu + strlen(syntaxe_msg));
				strcat(messageAEnvoyer, "\n");

				// Envoi du message à l'utilisateur ciblé
				ecrits = write(users[i].socket, messageAEnvoyer, strlen(messageAEnvoyer));
				printf("[INFO] Message privé %s envoyé à %s (%d octets)\n", messageAEnvoyer, users[i].login, ecrits);

				return 0;
			}
		}

		// Si l'utilisateur n'a pas été trouvé
		if (i == NB_MAX_USERS) {
			
			// Création du message à envoyer
			strcat(messageAEnvoyer, "Utilisateur inconnu !\n");

			// Envoi du message à l'utilisateur ciblé
			ecrits = write(users[i].socket, messageAEnvoyer, strlen(messageAEnvoyer));
			printf("[DEBUG] Utilisateur inconnu !\n");
		}
		
	}

	// Test si c'est la commande de liste
	else if (strncmp(messageRecu, "/liste", strlen("/liste")) == 0) {
		
		printf("[DEBUG] Commande de liste détectée !\n");

		// Création du message à envoyer
		strcpy(messageAEnvoyer, "Liste des utilisateurs : ");

		for (int i = 0; i < NB_MAX_USERS; i++) {
			if (users[i].socket != 0) {
				strcat(messageAEnvoyer, users[i].login);
				strcat(messageAEnvoyer, " ");
			}
		}

		strcat(messageAEnvoyer, "\n");

		// Envoi du message à l'utilisateur
		ecrits = write(socket_user_qui_demande, messageAEnvoyer, strlen(messageAEnvoyer));
		printf("[INFO] Message %s envoyé à %s (%d octets)\n", messageAEnvoyer, nom_user_qui_demande, ecrits);
	}

	// Test si c'est la commande de changement de pseudo
	else if (strncmp(messageRecu, "/nick ", strlen("/nick ")) == 0) {

		// Réinitialiser le nom du client
		memset(nom_user_qui_demande, '\0', sizeof(struct user));
		strcpy(nom_user_qui_demande, messageRecu + strlen("/nick "));

	}

	// Test si c'est la commande de demande de version
	else if (strncmp(messageRecu, "/ver", strlen("/ver")) == 0) {

		printf("[DEBUG] Commande de version détectée !\n");

		// Création du message à envoyer
		strcpy(messageAEnvoyer, "Version du serveur : 1.0\n");

		// Envoi du message à l'utilisateur
		ecrits = write(socket_user_qui_demande, messageAEnvoyer, strlen(messageAEnvoyer));
		printf("[INFO] Message %s envoyé à %s (%d octets)\n", messageAEnvoyer, nom_user_qui_demande, ecrits);
	}

	// Test si c'est la commande du respect
	else if (strncmp(messageRecu, "/f", strlen("/f")) == 0) {

		printf("[DEBUG] Commande du respect détectée !\n");

		// Création du message à envoyer
		strcpy(messageAEnvoyer, "[SERVEUR] You pay respect !\n");

		// Envoi du message à l'utilisateur
		ecrits = write(socket_user_qui_demande, messageAEnvoyer, strlen(messageAEnvoyer));	
		printf("[INFO] Message %s envoyé à %s (%d octets)\n", messageAEnvoyer, nom_user_qui_demande, ecrits);
	}

	// Test si c'est la commande du doute
	else if (strncmp(messageRecu, "/x", strlen("/x")) == 0) {

		printf("[DEBUG] Commande du doute détectée !\n");

		// Création du message à envoyer
		strcpy(messageAEnvoyer, "[SERVEUR] DOUBT\n");

		// Envoi du message à l'utilisateur
		ecrits = write(socket_user_qui_demande, messageAEnvoyer, strlen(messageAEnvoyer));
		printf("[INFO] Message %s envoyé à %s (%d octets)\n", messageAEnvoyer, nom_user_qui_demande, ecrits);
	}
	
	// Si la commande ne correspond à aucune connue
	else {

		printf("[DEBUG] Commande inconnue !\n");

		// Création du message à envoyer
		strcpy(messageAEnvoyer, "[SERVEUR] Commande inconnue !\n");

		// Envoi du message à l'utilisateur
		ecrits = write(socket_user_qui_demande, messageAEnvoyer, strlen(messageAEnvoyer));
		printf("[INFO] Message %s envoyé à %s (%d octets)\n", messageAEnvoyer, nom_user_qui_demande, ecrits);
	}

}

/* Fonction permettant d'afficher à tous (sauf à l'envoyeur) un message envoyé par un utilisateur */
void afficherMessageATous(struct user users[], char nom_user_qui_envoie[], int socket_user_qui_envoie, char message[]) {

	// Parcourir la liste des utilisateurs
	for (int i = 0; i < NB_MAX_USERS; i++) {
		
		// Si le socket est non nul (= utilisateur qui existe et est connecté) et qu'il ne s'agit pas de l'utilisateur qui envoie le message
		if (users[i].socket != 0 && users[i].socket != socket_user_qui_envoie) {
			
			char messageAEnvoyer[LG_MAX_MESSAGE + 10];

			// Initialiser la variable message à envoyer
			memset(messageAEnvoyer, '\0', (LG_MAX_MESSAGE + 10)*sizeof(char));
			
			// Création du message à envoyer
			sprintf(messageAEnvoyer, "%s : ", nom_user_qui_envoie);
			strcat(messageAEnvoyer, message);
			strcat(messageAEnvoyer, "\n");

			// Envoi du message
			int ecrits = write(users[i].socket, messageAEnvoyer, strlen(messageAEnvoyer));
			printf("[INFO] Message %s envoyé à %s (%d octets)\n", messageAEnvoyer, users[i].login, ecrits);
		}

	}

}
