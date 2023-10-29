#include "pse.h"

#define CMD      "serveur"

void *sessionClient(void *arg);
void sendFileList(int clientSocket);
void sendFile(int clientSocket, const char *filename);
void receiveFileFromClient(int clientSocket, const char *filename);

int main(int argc, char *argv[]) {
  short port;
  int ecoute, canal, ret;
  struct sockaddr_in adrEcoute, adrClient;
  unsigned int lgAdrClient;
  DataThread *dataThread;

  initDataThread();

  if (argc != 2)
    erreur("usage: %s port\n", argv[0]);

  port = (short)atoi(argv[1]);

  printf("%s: creating a socket\n", CMD);
  ecoute = socket (AF_INET, SOCK_STREAM, 0);
  if (ecoute < 0)
    erreur_IO("socket");
  
  adrEcoute.sin_family = AF_INET;
  adrEcoute.sin_addr.s_addr = INADDR_ANY;
  adrEcoute.sin_port = htons(port);
  printf("%s: binding to INADDR_ANY address on port %d\n", CMD, port);
  ret = bind (ecoute,  (struct sockaddr *)&adrEcoute, sizeof(adrEcoute));
  if (ret < 0)
    erreur_IO("bind");
  
  printf("%s: listening to socket\n", CMD);
  ret = listen (ecoute, 5);
  if (ret < 0)
    erreur_IO("listen");
  
  while (VRAI) {
    printf("%s: accepting a connection\n", CMD);
    canal = accept(ecoute, (struct sockaddr *)&adrClient, &lgAdrClient);
    if (canal < 0)
      erreur_IO("accept");

    printf("%s: adr %s, port %hu\n", CMD,
	      stringIP(ntohl(adrClient.sin_addr.s_addr)), ntohs(adrClient.sin_port));

    dataThread = ajouterDataThread();
    if (dataThread == NULL)
      erreur_IO("ajouter data thread");

    dataThread->spec.canal = canal;
    ret = pthread_create(&dataThread->spec.id, NULL, sessionClient,
                          &dataThread->spec);
    if (ret != 0)
      erreur_IO("creation thread");

    joinDataThread();
  }

  if (close(ecoute) == -1)
    erreur_IO("fermeture ecoute");

  exit(EXIT_SUCCESS);
}

void *sessionClient(void *arg) {
  DataSpec *dataTh = (DataSpec *)arg;
  int canal;
  int fin = FAUX;
  char ligne[LIGNE_MAX];
  int lgLue;

  canal = dataTh->canal;

  while (!fin) {
    lgLue = lireLigne(canal, ligne);
    if (lgLue < 0)
      erreur_IO("lireLigne");
    else if (lgLue == 0)
      erreur("interruption client\n");

    if (strcmp(ligne, "fin") == 0) {
      printf("%s: fin client\n", CMD);
      fin = VRAI;
    }
    else if (strcmp(ligne, "liste") == 0){
      printf("%s: sending list files\n", CMD);
      sendFileList(canal);
    }
    else if (strncmp(ligne, "download ", 9) == 0) {
      // Extraire le nom de fichier à partir de la commande
      char filename[LIGNE_MAX];
      strncpy(filename, ligne + 9, LIGNE_MAX - 9);
      filename[LIGNE_MAX - 9] = '\0';
      
      printf("%s: sending file '%s'\n", CMD, filename);
      sendFile(canal, filename);
    }
    else if (strncmp(ligne, "upload ", 7) == 0) {
      // Extraire le nom de fichier à partir de la commande
      char filename[LIGNE_MAX];
      strncpy(filename, ligne + 7, LIGNE_MAX - 7);
      filename[LIGNE_MAX - 7] = '\0';

      printf("%s: receiving file '%s'\n", CMD, filename);
      receiveFileFromClient(canal, filename);
}
    else
        printf("%s : Commande non valide", CMD);
  } // fin while

  if (close(canal) == -1)
    erreur_IO("fermeture canal");

  dataTh->libre = VRAI;

  pthread_exit(NULL);
}

void sendFileList(int clientSocket) {
  DIR *dir;
  struct dirent *entry;

  dir = opendir("server_files/"); 

  if (dir == NULL) {
    perror("Erreur lors de l'ouverture du dossier");
    return;
  }

  while ((entry = readdir(dir)) != NULL) {
    send(clientSocket, entry->d_name, sizeof(entry->d_name), 0);
  }
  
  send(clientSocket, "end_of_list", sizeof("end_of_list"), 0); //signal plus de fichier
  closedir(dir);
}

void sendFile(int clientSocket, const char *filename) {
  char filepath[256];
  snprintf(filepath, sizeof(filepath), "server_files/%s", filename);
  
  FILE *file = fopen(filepath, "rb");
  if (file == NULL) {
    perror("Erreur lors de l'ouverture du fichier");
    send(clientSocket, "file_not_found", sizeof("file_not_found"), 0);
    return;
  }
  
  char buffer[1024];
  size_t bytesRead;
  
  while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) == 1024) {
    if (send(clientSocket, buffer, bytesRead, 0) != bytesRead) {
      perror("Erreur lors de l'envoi du fichier");
      fclose(file);
      return;
    }
  }

  send(clientSocket, buffer, bytesRead, 0);
  sleep(2);
  send(clientSocket, "end_of_file", sizeof("end_of_file"), 0); //signal plus de fichier

  fclose(file);
}

void receiveFileFromClient(int clientSocket, const char *filename) {
  char filepath[256];
  snprintf(filepath, sizeof(filepath), "server_files/%s", filename);

  FILE *file = fopen(filepath, "wb");
  if (file == NULL) {
    perror("Erreur lors de la création du fichier sur le serveur");
    return;
  }

  char buffer[1024];
  ssize_t bytesRead;
  int fin = FAUX;

  while(!fin){
    bytesRead = recv(clientSocket, buffer, sizeof(buffer), 0);
    if (strncmp(buffer, "end_of_file", 11) == 0)
      fin = VRAI;
    else if (bytesRead > 0){
      if (fwrite(buffer, 1, bytesRead, file) != (size_t)bytesRead) {
      perror("Erreur lors de l'écriture dans le fichier");
      fclose(file);
      return;
      }
    }
  }
  fclose(file);
  printf("%s: upload completed\n", CMD);
}
