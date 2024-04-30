#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <limits.h>
#include <time.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <libgen.h>  //pt basename

#define PATH_MAX 4096

void parcurgere_director(char *nume_director, int snapshot, int nivel){
    DIR *dir;
    struct dirent *intrare;
    struct stat st;
    char cale[PATH_MAX];   
    char cale_link[PATH_MAX + 1];  //calea-calea catre fis curent
    char spatii[PATH_MAX];
    int n;
    char mesaj[100000];

    memset(spatii, ' ', 2 * nivel);
    spatii[2 * nivel] = '\0';
    
    if(!(dir = opendir(nume_director))){
        perror("Eroare la deschiderea directorului din functia 'parcurgere_director");
        exit(-1);
    }

    snprintf(mesaj, sizeof(mesaj), "%sDIRECTOR: %s\n", spatii, nume_director);
    write(snapshot, mesaj, strlen(mesaj));

    if(stat(nume_director, &st) < 0){
        perror("Eroare la stat\n");
        exit(-1);
    }

        //afisam data ultimei modificari a directorului
    sprintf(mesaj, "        ---> Last modification time: %s\n", ctime(&st.st_mtime));
    write(snapshot, mesaj, strlen(mesaj));

    while(((intrare = readdir(dir)) != NULL)){
        if(strcmp (intrare->d_name, ".") ==0 || strcmp (intrare->d_name, "..")==0)
            continue;

        snprintf(cale, sizeof(cale), "%s/%s", nume_director, intrare->d_name);

        if((lstat(cale, &st)) < 0){
            //folosim lstat->daca avem leg simbolica, stat nu afiseaza atributele din legatura, le va afisa din fisier
            perror("Eroare la lstat\n");
            exit(1);
        }

        if(S_ISDIR(st.st_mode)){
            parcurgere_director(cale, snapshot, nivel + 1);
        }else{
            if(S_ISLNK(st.st_mode)){
                sprintf(mesaj, "    %s ---> LINK SIMBOLIC ---> I-node %ld ---> Dimeniune %ld bytes ---> Last access time %s", cale, st.st_ino, st.st_size, ctime(&st.st_atime));
                write(snapshot, mesaj, strlen(mesaj));
                n=readlink(cale, cale_link, sizeof(cale_link));  //citim continutul leg simbolice
                cale_link[n]='\0';
                sprintf(mesaj, "%s  %s -> %s\n", spatii, cale, cale_link);
                write(snapshot, mesaj, strlen(mesaj));
                write(snapshot, "\n", strlen("\n"));
            }else{
                sprintf(mesaj, "    %s ---> REGULAR FILE ---> I-node %ld ---> Dimeniune %ld bytes ---> Last access time %s", cale, st.st_ino, st.st_size, ctime(&st.st_atime));
                write(snapshot, mesaj, strlen(mesaj));
                write(snapshot, "\n", strlen("\n"));
            }
        }
    }
    closedir(dir);
}

//Functia aceasta ne denumeste fiecare fisier sub forma: "snapshot_[numele_directorului].txt"
char* numeFisier(char *director){
    char *nume = basename(director); //functia basename ne da numele de baza al directorului

    static char fisier[100];  //avem nevoie de static pentru ca variabila noastra sa ramana in memorie 
    sprintf(fisier, "snapshot_%s.txt", nume);

    return fisier;
}

//functie pentru crearea unui director
void creare_director(const char *nume){
    int output;
    DIR *d;
    if((d = opendir(nume)) != 0){    //verificam daca exita directorul nostru, iar in caz ca nu exista creem un director cu numele argumentului 2
        perror("Directorul exista\n");
    }
    else{
        output = mkdir(nume, 0777);   //0777 este un cod de permisiuni pentru noul director

        if(output != 0){    //verificam daca s a creat cu succes
            perror("Eroare la crearea directorului\n");
        }
    }

}

void comparare_actualizare(int vechi, int nou){
    char buff_vechi[10000];
    char buff_nou[10000];

    int bytes_vechi = read(vechi, buff_vechi, sizeof(buff_vechi));
    if(bytes_vechi == -1){
        perror("Eroare la citirea snapshot-ului vechi\n");
        exit(-1);
    }

    int bytes_nou = read(vechi, buff_nou, sizeof(buff_nou));
    if(bytes_nou == -1){
        perror("Eroare la citirea snapshot-ului nou\n");
        exit(-1);
    }

    if((bytes_vechi != bytes_nou) || (memcmp(buff_vechi, buff_nou, bytes_vechi) !=0)){
        lseek(vechi, 0 ,SEEK_SET);
        if(write(vechi, buff_nou, bytes_nou) == -1){
            perror("Eroare la actualizare snapshot-ului\n");
            exit(-1);
        }
    }
}

int main( int argc, char **argv ){
    
    DIR *d;
    int ok;  //ok=1 -> arg egale, ok=0 -> arg diferite 
    char *nume_output;
    nume_output = argv[2];
    int count=1;

    if((argc < 3) || (argc > 10)){
        perror("Numar de argumente invalid\n");
        exit(-1);
    }else{
        ok = 0;
        for(int i = 3; i < argc - 1; i++){
            for(int j = i+1; j < argc; j++){ 
                if((strcmp(argv[i], argv[j])) == 0){
                    ok = 1;
                    perror("Argumente egale in linia de comanda\n"); 
                    exit(-1);
                }
            }
        }
    }

    creare_director(nume_output); //creeam directorul cu SNAPSHOT-uri

    if((ok == 0) && ((strcmp(argv[1], "-o")) == 0)){
        for(int i = 3; i < argc; i++){
            if((d = opendir( argv[i] )) == NULL)
                continue;
            else{
                if((chdir(nume_output)) == -1){ //Dar unde se afla acest director ? De regula variabele statice, se extrag intr-un fisier aparte, si din acesta se extrag date )
    //!!!   aici mi am schimbat calea in directorul dat ca parametru
    //      de exemplu: eu am fost in PROIECT-SO si acum sunt in nume_output (SNAPSHOT-uri in cazul meu) 
                    perror("Nu exista directorul\n");
                    exit(-1);
                }

                int snapshot_director; 
                if((snapshot_director = open(numeFisier(argv[i]), O_CREAT | O_RDWR, S_IWUSR | S_IRUSR | S_IXUSR)) == -1){
                    perror("Eroare la deschideree\n");
                    exit(-1);
                }

                if((chdir("..")) == -1){ //Aici nu am inteles ce se intampla !cd
    //!!!   aici revin in directorul meu PROIECT-SO
                    exit(-1);
                }

                parcurgere_director(argv[i], snapshot_director, 0);

                int snapshot_nou;
                if((snapshot_nou = open("/tmp/new_snapshot.txt", O_CREAT | O_RDWR, S_IWUSR | S_IRUSR)) == -1 ){
                    perror("Eroare la deschiderea snapshot-ului nou\n");
                    exit(-1);
                }

                comparare_actualizare(snapshot_director, snapshot_nou);

                close(snapshot_director);
                close(snapshot_nou);

                pid_t pid;
                pid=fork();

                if(pid < 0){
                    perror("Eroare la fork\n");
                    exit(-1);
                }else{
                    if(pid == 0){
                        printf("Snapshot for Directory %s created successfully\n",argv[i]);
                        exit(0);
                    }
                    else{
                            int status;
                            wait(&status);
                            if(WIFEXITED(status)){
                                printf("Child process %d terminated with PID %d and exit code %d.\n", count, pid, WEXITSTATUS(status));
                            count++;
                            printf("\n");
                            }
                    }
                }
            }
        }
    }
    else
    {
        perror("Probleme la primele 3 argumente din linia de comanda\n");
        exit(-1);
    }

    return 0;
}