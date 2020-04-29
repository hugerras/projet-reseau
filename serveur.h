typedef struct User {
	int socketClient;
	char login[50];
} user;

typedef struct Pollfd {
	int fd; // File Descriptor
	short events; // Requested Events
	short revents; // Returned Events
} pollfd;
