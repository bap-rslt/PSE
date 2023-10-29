#include "pse.h"

#define CMD   "client"

void displayMenu(void);
void receiveFileList(int serverSocket);
void receiveFile(int serverSocket, const char *filename);
void sendFileToServer(int serverSocket, const char *filename);

int main(int argc, char *argv[]) {
  int sock, ret;
  struct sockaddr_in *adrServ;
  int fin = FAUX;
  char ligne[LIGNE_MAX];
  int lgEcr;

  if (argc != 3)
    erreur("usage: %s machine port\n", argv[0]);

  printf("%s: creating a socket\n", CMD);
  sock = socket (AF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    erreur_IO("socket");

  printf("%s: DNS resolving for %s, port %s\n", CMD, argv[1], argv[2]);
  adrServ = resolv(argv[1], argv[2]);
  if (adrServ == NULL)
    erreur("adresse %s port %s inconnus\n", argv[1], argv[2]);

  printf("%s: adr %s, port %hu\n", CMD,
	        stringIP(ntohl(adrServ->sin_addr.s_addr)),
	        ntohs(adrServ->sin_port));

  printf("%s: connecting the socket\n", CMD);
  ret = connect(sock, (struct sockaddr *)adrServ, sizeof(struct sockaddr_in));
  if (ret < 0)
    erreur_IO("connect");

  while (!fin) {
    displayMenu();
    printf("ligne> ");
    if (fgets(ligne, LIGNE_MAX, stdin) == NULL)
      erreur("saisie fin de fichier\n");

    lgEcr = ecrireLigne(sock, ligne);
    if (lgEcr == -1)
      erreur_IO("ecrire ligne");

    if (strcmp(ligne, "fin\n") == 0)
      fin = VRAI;
    
    if (strncmp(ligne, "liste", 5) == 0)
      receiveFileList(sock);
    else if (strncmp(ligne, "download ", 9) == 0) {
      // Extraire le nom de fichier à partir de la commande
      char filename[LIGNE_MAX];
      strncpy(filename, ligne + 9, LIGNE_MAX - 9);
      filename[LIGNE_MAX - 9] = '\0';

      // Supprimer le caractère de saut de ligne à la fin de la chaîne
      char *newline = strchr(filename, '\n');
      if (newline != NULL)
        *newline = '\0';
      
      printf("%s: receiving file '%s'\n", CMD, filename);
      receiveFile(sock, filename);
    }
    else if (strncmp(ligne, "upload ", 7) == 0) {
      // Extraire le nom de fichier à partir de la commande
      char filename[LIGNE_MAX];
      strncpy(filename, ligne + 7, LIGNE_MAX - 7);
      filename[LIGNE_MAX - 7] = '\0';

      // Supprimer le caractère de saut de ligne à la fin de la chaîne
      char *newline = strchr(filename, '\n');
      if (newline != NULL)
        *newline = '\0';

      printf("%s: uploading file '%s'\n", CMD, filename);
      sendFileToServer(sock, filename);
    }
  }

  if (close(sock) == -1)
    erreur_IO("close socket");

  exit(EXIT_SUCCESS);
}

void displayMenu() {
  printf("\nActions disponibles:\n");
  printf("Afficher la liste des fichiers disponibles > liste\n");
  printf("Télécharger un fichier > download \"fichier\"\n");
  printf("Téléverser un fichier > upload \"fichier\"\n");
  printf("Quitter > fin\n");
}

void receiveFileList(int serverSocket) {
  char fileName[256];
  int bytesRead;
  int fin = FAUX;

  while (!fin) {
    bytesRead = recv(serverSocket, fileName, sizeof(fileName), 0);
    if (strncmp(fileName, "end_of_list", 11) == 0)
      fin = VRAI;
    else if (strncmp(fileName, ".", 1) == 0);
    else if (strncmp(fileName, "..", 2) == 0);
    else if (bytesRead > 0)
      printf("%s\n", fileName); // Affiche le nom du fichier reçu
  }
}

void receiveFile(int serverSocket, const char *filename) {
  char filepath[256];
  snprintf(filepath, sizeof(filepath), "client_files/%s", filename);
  
  FILE *file = fopen(filepath, "wb");
  if (file == NULL) {
    perror("Erreur lors de l'ouverture du fichier");
    return;
  }
  
  char buffer[1024];
  int bytesReceived;
  int fin = FAUX;
  
  while (!fin) {
    bytesReceived = recv(serverSocket, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "file_not_found", 14) == 0){
      printf("%s: file not available on the server\n", CMD);
      fin = VRAI;
    }
    else if (strncmp(buffer, "end_of_file", 11) == 0)
      fin = VRAI;
    else if (bytesReceived > 0){
      if (fwrite(buffer, 1, bytesReceived, file) != (size_t)bytesReceived) {
      perror("Erreur lors de l'écriture dans le fichier");
      fclose(file);
      return;
      }
    }
  }
  fclose(file);
}

void sendFileToServer(int serverSocket, const char *filename) {
  char filepath[256];
  snprintf(filepath, sizeof(filepath), "client_files/%s", filename);

  FILE *file = fopen(filepath, "rb");
  if (file == NULL) {
    perror("Erreur lors de l'ouverture du fichier");
    return;
  }

  char buffer[1024];
  size_t bytesRead;

  while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) == 1024) {
    if (send(serverSocket, buffer, bytesRead, 0) != bytesRead) {
      perror("Erreur lors de l'envoi du fichier");
      fclose(file);
      return;
    }
  }

  send(serverSocket, buffer, bytesRead, 0);
  sleep(2);
  send(serverSocket, "end_of_file", sizeof("end_of_file"), 0); //signal plus de fichier

  fclose(file);
  printf("%s: upload completed\n", CMD);
}
