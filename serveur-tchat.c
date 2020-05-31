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
int gestionCommande();
void afficherMessageATous();
void nouvelleConnexion();

/* Fonction principale */
int main() {
	
	int socketEcoute;
	struct sockaddr_in pointDeRencontreLocal;
	socklen_t longueurAdresse;
	int socketDialogue;
	struct sockaddr_in pointDeRencontreDistant;
	char messageRecu[LG_MAX_MESSAGE]; // Message de la couche Application
	int ecrits, lus; // Nb d'octets écrits et lus
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

								nouvelleConnexion(users[j].login, users[j].socket);

								char message[LG_MAX_MESSAGE];

								// Initialiser la variable message
								memset(message, '\0', LG_MAX_MESSAGE*sizeof(char));

								sprintf(message, "%s vient de rejoindre le tchat !\n", users[j].login);
								afficherMessageATous(users, -1, message);

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

								sprintf(message, "%s vient de quitter le tchat !\n", users[j].login);
								afficherMessageATous(users, -1, message);

								// Réinitialisation de l'utilisateur
								memset(&users[j], '\0', sizeof(struct user));

							default: /* Réception de n octets */
								printf("[INFO] Message reçu de %s : %s (%d octets)\n", users[j].login, messageRecu, lus);

								gestionCommande(users, j, messageRecu);
								
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
int gestionCommande(struct user users[], int id_user_qui_demande, char messageRecu[]) {
	
	char messageAEnvoyer[LG_MAX_MESSAGE + 100];
	int ecrits;

	// Initialiser la variable message à envoyer
	memset(messageAEnvoyer, '\0', (LG_MAX_MESSAGE + 100)*sizeof(char));

	// Test si c'est la commande de message privé
	if (strncmp(messageRecu, "<message> ", strlen("<message> ")) == 0) {
		
		printf("[DEBUG] Commande de message privé détectée !\n");

		// Si c'est un message global
		if (strncmp(messageRecu, "<message> * ", strlen("<message> * ")) == 0) {
			afficherMessageATous(users, id_user_qui_demande, messageRecu + strlen("<message> * "));
		}

		// Sinon c'est un message privé
		else {

			int i;
			char syntaxe_msg[strlen("<message> ") + LG_MAX_LOGIN + 1];
		
			// Initialiser la variable syntaxe commande msg
			memset(syntaxe_msg, '\0', (strlen("<message> ") + LG_MAX_LOGIN + 1)*sizeof(char));

			// Chercher quel utilisateur est ciblé
			for (i = 0; i < NB_MAX_USERS; i++) {
				
				strcpy(syntaxe_msg, "<message> ");
				strcat(syntaxe_msg, users[i].login);
				strcat(syntaxe_msg, " ");

				// Si on a trouvé l'utilisateur ciblé, on lui envoie le message
				if (strncmp(messageRecu, syntaxe_msg, strlen(syntaxe_msg)) == 0) {
					
					// Création du message à envoyer
					sprintf(messageAEnvoyer, "<message> %s %s ", users[id_user_qui_demande].login, users[i].login);
					strcat(messageAEnvoyer, messageRecu + strlen(syntaxe_msg));

					// Envoi du message à l'utilisateur ciblé
					ecrits = write(users[i].socket, messageAEnvoyer, strlen(messageAEnvoyer));
					printf("[INFO] Message privé %s envoyé à %s (%d octets)\n", messageAEnvoyer, users[i].login, ecrits);

					break;
				}
			}

			// Si l'utilisateur n'a pas été trouvé
			if (i == NB_MAX_USERS) {
				
				// Création du message à envoyer
				strcpy(messageAEnvoyer, "<error> Utilisateur inconnu !\n");
				printf("[DEBUG] Utilisateur inconnu !\n");

				// Envoi du message à l'utilisateur ciblé
				ecrits = write(users[id_user_qui_demande].socket, messageAEnvoyer, strlen(messageAEnvoyer));
				printf("[INFO] Message d'erreur %s envoyé à %s (%d octets)\n", messageAEnvoyer, users[id_user_qui_demande].login, ecrits);
			}
		}

	}

	// Test si c'est la commande de liste
	else if (strncmp(messageRecu, "<list>", strlen("<list>")) == 0) {
		
		printf("[DEBUG] Commande de liste détectée !\n");

		// Création du message à envoyer
		strcpy(messageAEnvoyer, "<list> ");

		for (int i = 0; i < NB_MAX_USERS; i++) {
			if (users[i].socket != 0) {
				strcat(messageAEnvoyer, users[i].login);
				strcat(messageAEnvoyer, "|");
			}
		}

		strcat(messageAEnvoyer, "\n");

		// Envoi du message à l'utilisateur
		ecrits = write(users[id_user_qui_demande].socket, messageAEnvoyer, strlen(messageAEnvoyer));
		printf("[INFO] Message %s envoyé à %s (%d octets)\n", messageAEnvoyer, users[id_user_qui_demande].login, ecrits);
	}

	// Test si c'est la commande de changement de pseudo
	else if (strncmp(messageRecu, "<login> ", strlen("<login> ")) == 0) {

		printf("[DEBUG] Commande de changement de login détectée !\n");

		char syntaxe_msg[strlen("<login> ") + LG_MAX_LOGIN];
	
		// Initialiser la variable syntaxe commande msg
		memset(syntaxe_msg, '\0', (strlen("<login> ") + LG_MAX_LOGIN)*sizeof(char));

		// On s'assure que quelqu'un ne possède pas déjà ce nom
		for (int i = 0; i < NB_MAX_USERS; i++) {

			if (users[i].socket != 0) {
				
				strcpy(syntaxe_msg, "<login> ");
				strcat(syntaxe_msg, users[i].login);
				
				// Si le client a entré un login déjà utilisé par un autre utilisateur
				if (strncmp(messageRecu, syntaxe_msg, strlen(syntaxe_msg)) == 0) {
					
					printf("[DEBUG] Un nom identique a été trouvé !\n");

					// Initialiser la variable syntaxe commande msg
					memset(syntaxe_msg, '\0', (strlen("<login> ") + LG_MAX_LOGIN)*sizeof(char));

					strcpy(syntaxe_msg, "<login> ");
					strcat(syntaxe_msg, users[id_user_qui_demande].login);

					// Si le client essaye de mettre son même login
					if (strncmp(messageRecu, syntaxe_msg, strlen(syntaxe_msg)) == 0) {
						// Création du message à envoyer
						strcpy(messageAEnvoyer, "<error> Vous portez déjà ce nom !\n");
					}

					else {
						// Création du message à envoyer
						strcpy(messageAEnvoyer, "<error> Une personne utilise déjà ce nom !\n");
					}

					// Envoi du message à l'utilisateur ciblé
					ecrits = write(users[id_user_qui_demande].socket, messageAEnvoyer, strlen(messageAEnvoyer));
					printf("[INFO] Message privé %s envoyé à %s (%d octets)\n", messageAEnvoyer, users[i].login, ecrits);

					return 0;
				}

				// Si le client a entré le login réservé au serveur
				else if (strncmp(messageRecu, "<login> srv", strlen("<login> srv")) == 0) {
					
					printf("[DEBUG] Le nom réservé au serveur a été trouvé !\n");

					// Création du message à envoyer
					strcpy(messageAEnvoyer, "<error> Ce nom est réservé au serveur !\n");

					// Envoi du message à l'utilisateur ciblé
					ecrits = write(users[id_user_qui_demande].socket, messageAEnvoyer, strlen(messageAEnvoyer));
					printf("[INFO] Message privé %s envoyé à %s (%d octets)\n", messageAEnvoyer, users[i].login, ecrits);

					return 0;
				}

			}

		}

		// Stocker l'ancien login du client
		char ancien_login[LG_MAX_LOGIN];
		strcpy(ancien_login, users[id_user_qui_demande].login);

		// Réinitialiser le nom du client
		memset(users[id_user_qui_demande].login, '\0', LG_MAX_LOGIN*sizeof(char));

		// Enregistrer le nouveau nom du client
		strcpy(users[id_user_qui_demande].login, messageRecu + strlen("<login> "));

		// On supprime le retour à la ligne
		users[id_user_qui_demande].login[strlen(users[id_user_qui_demande].login) - 1] = '\0';

		// Création du message à envoyer
		sprintf(messageAEnvoyer, "%s s'appelle désormais %s\n", ancien_login, users[id_user_qui_demande].login);

		// Informer tout le monde
		afficherMessageATous(users, -1, messageAEnvoyer);
	}
	
	// Test si c'est la commande d'aide
	else if (strncmp(messageRecu, "<help>", strlen("<help>")) == 0) {
		
		printf("[DEBUG] Commande d'aide détectée !\n");

		// Création du message à envoyer
		sprintf(messageAEnvoyer, "<message> srv %s Listes des commandes :\n", users[id_user_qui_demande].login);
		strcat(messageAEnvoyer, "• <message> [destinataire] [message] : Envoyer un message (* en destinataire pour envoyer à tout le monde)\n");
		strcat(messageAEnvoyer, "• <list> : Afficher la liste des utilisateurs connectés\n");
		strcat(messageAEnvoyer, "• <login> [pseudo] : Modifier son pseudo\n");
		strcat(messageAEnvoyer, "• <help> : Afficher la liste des commandes\n");

		// Envoi du message à l'utilisateur
		ecrits = write(users[id_user_qui_demande].socket, messageAEnvoyer, strlen(messageAEnvoyer));
		printf("[INFO] Message %s envoyé à %s (%d octets)\n", messageAEnvoyer, users[id_user_qui_demande].login, ecrits);
	}

	// Si la commande ne correspond à aucune connue
	else {

		printf("[DEBUG] Commande inconnue !\n");

		// Création du message à envoyer
		strcpy(messageAEnvoyer, "<error> Commande inconnue !\n");

		// Envoi du message à l'utilisateur
		ecrits = write(users[id_user_qui_demande].socket, messageAEnvoyer, strlen(messageAEnvoyer));
		printf("[INFO] Message %s envoyé à %s (%d octets)\n", messageAEnvoyer, users[id_user_qui_demande].login, ecrits);
	}

	return 0;
}

/* Fonction permettant d'afficher à tous (sauf à l'envoyeur) un message envoyé par un utilisateur */
void afficherMessageATous(struct user users[], int id_user_qui_demande, char message[]) {

	char messageAEnvoyer[LG_MAX_MESSAGE + 100];

	// Parcourir la liste des utilisateurs
	for (int i = 0; i < NB_MAX_USERS; i++) {
		
		// Si c'est un message du serveur
		if (id_user_qui_demande == -1 && users[i].socket != 0) {
			
			// Initialiser la variable message à envoyer
			memset(messageAEnvoyer, '\0', (LG_MAX_MESSAGE + 100)*sizeof(char));
			
			// Création du message à envoyer
			strcpy(messageAEnvoyer, "<message> srv * ");
			strcat(messageAEnvoyer, message);

			// Envoi du message
			int ecrits = write(users[i].socket, messageAEnvoyer, strlen(messageAEnvoyer));
			printf("[INFO] Message %s envoyé à %s (%d octets)\n", messageAEnvoyer, users[i].login, ecrits);

		}
		
		// Si le socket est non nul (= utilisateur qui existe et est connecté) et qu'il ne s'agit pas de l'utilisateur qui envoie le message
		else if (users[i].socket != 0 && users[i].socket != users[id_user_qui_demande].socket) {

			// Initialiser la variable message à envoyer
			memset(messageAEnvoyer, '\0', (LG_MAX_MESSAGE + 100)*sizeof(char));
			
			// Création du message à envoyer
			sprintf(messageAEnvoyer, "<message> %s * ", users[id_user_qui_demande].login);
			strcat(messageAEnvoyer, message);

			// Envoi du message
			int ecrits = write(users[i].socket, messageAEnvoyer, strlen(messageAEnvoyer));
			printf("[INFO] Message %s envoyé à %s (%d octets)\n", messageAEnvoyer, users[i].login, ecrits);
		}

	}

}

/* Fonction gérant les infos à envoyer au nouveau client */
void nouvelleConnexion(char nom_nouveau_user[], int socket_nouveau_user) {

	char messageAEnvoyer[LG_MAX_MESSAGE + 100];
	int ecrits;

	// Initialiser la variable message à envoyer
	memset(messageAEnvoyer, '\0', (LG_MAX_MESSAGE + 100)*sizeof(char));

	// Création du message à envoyer
	strcpy(messageAEnvoyer, "<version> 1.0\n");

	// Envoi du message
	ecrits = write(socket_nouveau_user, messageAEnvoyer, strlen(messageAEnvoyer));
	printf("[INFO] Message %s envoyé à %s (%d octets)\n", messageAEnvoyer, nom_nouveau_user, ecrits);

	// Initialiser la variable message à envoyer
	memset(messageAEnvoyer, '\0', (LG_MAX_MESSAGE + 100)*sizeof(char));

	// Création du message à envoyer
	sprintf(messageAEnvoyer, "<greetings> Bienvenue sur le serveur %s ! Tapez <help> pour afficher la liste des commandes !\n", nom_nouveau_user);

	// Envoi du message
	ecrits = write(socket_nouveau_user, messageAEnvoyer, strlen(messageAEnvoyer));
	printf("[INFO] Message %s envoyé à %s (%d octets)\n", messageAEnvoyer, nom_nouveau_user, ecrits);

}
