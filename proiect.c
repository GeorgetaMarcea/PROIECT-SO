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

int verificare_drepturi(const char *filename, struct stat st) {
    // Verificăm drepturile pentru proprietar
    if ((st.st_mode & S_IRUSR) || (st.st_mode & S_IWUSR) || (st.st_mode & S_IXUSR)) {
        // Verificăm drepturile pentru grup
        if ((st.st_mode & S_IRGRP) || (st.st_mode & S_IWGRP) || (st.st_mode & S_IXGRP)) {
            // Verificăm drepturile pentru alți utilizatori
            if ((st.st_mode & S_IROTH) || (st.st_mode & S_IWOTH) || (st.st_mode & S_IXOTH)) {
                return 0;  // Toate drepturile sunt prezente
            }
        }
    }
    return 1;  // Nu toate drepturile sunt prezente
}

void creare_proces_script(char *nume_fisier, struct stat st){
    int drepturi;
    if((drepturi = verificare_drepturi(nume_fisier, st)) == 1){
        pid_t pid = fork();
        int pfd[2];
        if(pipe(pfd)<0){
            perror("'Eroare la crearea pipe-ului\n");
            exit(-1);
        }
        
        if(pid < 0){ 
            perror("Eroare la fork (script)\n");
            exit(-1);
        }

        if(pid == 0){ //procesul fiu
            close(pfd[0]); //am inchis capatul de citire, ramane cel de scriere
            dup2(pfd[1], STDOUT_FILENO); //STDOUT_FILENO este folosit pentru a direcționa ieșirea standard a unui program către un fișier, alt program sau un dispozitiv specificat.
            close(pfd[1]);
            execl("/bin/bash", "sh", "verify_for_malicious.sh", nume_fisier, "izolated_space_dir", NULL);
            perror("Eroareee la rularea scriptului de analiză sintactică\n");
        }
        //proces parinte
        int status;
        close(pfd[1]); //inchidem capatul de scriere, lasam pentru citire
        char buff[PATH_MAX];
        char path[100000];
        memset(buff, 0, sizeof(buff)); //setam toti octetii din buff la 0
        read(pfd[0], buff, sizeof(buff)); //citim rez din pipe-ul de la fiu
        if(strcmp(buff, "SAFE") !=0){ //fisierul este periculos si in mutam
            sprintf(path, "izolated_space_dir/%s", basename(nume_fisier)); //am folosit basename ca sa am doar numele fisierului, daca foloseam nume_fisier aveam de ecd xemplu /dir/b.c
            rename(nume_fisier, path);
        }

        wait(&status);
        if(WIFEXITED(status)){
                printf("Analiza sintactică a fost efectuată cu succes.\n");          
                printf("\n");
        }
        close(pfd[0]); 
    }    
}

void parcurgere_director(char *nume_director, int snapshot, int nivel){
    DIR *dir;
    struct dirent *intrare;
    struct stat st;
    char cale[PATH_MAX];   
    char cale_link[PATH_MAX + 1];  //calea-calea catre fis curent
    char spatii[PATH_MAX];
    int n;
    char mesaj[100000];

    memset(spatii, ' ', 2 * nivel);  //pune spatii in primele 2*nivel caractere in sirul: spatii
    spatii[2 * nivel] = '\0';
    
    if(!(dir = opendir(nume_director))){
        perror("Eroare la deschiderea directorului din functia 'parcurgere_director");
        exit(-1);
    }

    snprintf(mesaj, sizeof(mesaj), "%sDIRECTOR: %s\n", spatii, nume_director);
    write(snapshot, mesaj, strlen(mesaj));

    if(stat(nume_director, &st) < 0){  // nume_director-cale absoluta catre programul executabil al fisierului, iar st-parametru de iesire, reprezinta un pointer spre o zona de memorie ce contine o variabila de tip struct stat
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
                creare_proces_script(cale_link, st);
            }else{
                sprintf(mesaj, "    %s ---> REGULAR FILE ---> I-node %ld ---> Dimeniune %ld bytes ---> Last access time %s", cale, st.st_ino, st.st_size, ctime(&st.st_atime));
                write(snapshot, mesaj, strlen(mesaj));
                write(snapshot, "\n", strlen("\n"));
                creare_proces_script(cale, st);
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
            exit(-1);
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

    lseek(nou, 0, SEEK_SET);
    int bytes_nou = read(nou, buff_nou, sizeof(buff_nou));
    if(bytes_nou == -1){
        perror("Eroare la citirea snapshot-ului nou\n");
        exit(-1);
    }

    if((bytes_vechi != bytes_nou) || (memcmp(buff_vechi, buff_nou, bytes_vechi) !=0)){
        perror("Snapshot actualizat\n");
        lseek(vechi, 0 ,SEEK_SET);
        if(write(vechi, buff_nou, bytes_nou) == -1){
            perror("Eroare la actualizare snapshot-ului\n");
            exit(-1);
        }
    }else{
        perror("Snapshot identic\n");
    }
}

int snapshot_dir; // mi am declarat acest snaphot_dir global, deoarece avem nevoie de el atat in functia de mai jos, cat si in main 

//aici mi am facut o functie care ma ajuta sa imi creez snapshot-urile pentru fiecare argument din linia de comanda in directorul de output (argv[2])
//initial am avut acest cod in main, dar am zis ca mai bine fac o functie ca sa fie mai usor de inteles main-ul

void creare_snapshot_uri_director_output(char *cale_fisier_output, char *director_linie_comanda){

                if((chdir(cale_fisier_output)) == -1){
    //!!!   aici mi am schimbat calea in directorul dat ca parametru
    //      de exemplu: eu am fost in PROIECT-SO si acum sunt in nume_output (SNAPSHOT-uri in cazul meu) 
                    perror("Nu exista directorul\n");
                    exit(-1);
                }
                if((snapshot_dir = open(numeFisier(director_linie_comanda), O_CREAT | O_RDWR, S_IWUSR | S_IRUSR | S_IXUSR)) == -1){
                    perror("Eroare la deschideree\n");
                    exit(-1);
                }

                if((chdir("..")) == -1){
    //!!!   aici revin in directorul meu PROIECT-SO
                    exit(-1);
                }
}

int main( int argc, char **argv ){
    
    DIR *d;
    int ok;  //ok=1 -> arg egale, ok=0 -> arg diferite 
    char *nume_output;
    nume_output = argv[2];
    int count=1;

    if((argc < 5) || (argc > 10)){
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
    creare_director("izolated_space_dir");

    if((ok == 0) && ((strcmp(argv[1], "-o")) == 0) && ((strcmp(argv[3],"-s")) ==0)){
        for(int i = 5; i < argc; i++){
            if((d = opendir( argv[i] )) == NULL)
                continue;
            else{

                creare_snapshot_uri_director_output(nume_output,argv[i]); //am creat snapshot-urile pentru fiecare director din linia de comanda
                
                pid_t pid;
                pid=fork();

                if(pid < 0){
                    perror("Eroare la fork\n");
                    exit(-1);
                }else{
                    if(pid == 0){
                        parcurgere_director(argv[i], snapshot_dir, 0);

                        int snapshot_nou;
                        if((snapshot_nou = open("/tmp/new_snapshot.txt", O_CREAT | O_RDWR, S_IWUSR | S_IRUSR)) == -1 ){
                            perror("Eroare la deschiderea snapshot-ului nou\n");
                            exit(-1);
                        }

                        comparare_actualizare(snapshot_dir, snapshot_nou);
                        close(snapshot_nou);

                        printf("\n");
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

                close(snapshot_dir);

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