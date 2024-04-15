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
#include <libgen.h>  //pt basename

#define PATH_MAX 4096

void parcurgere_director(char *nume_director, int snapshot, int nivel){
    DIR *dir;
    struct dirent *intrare;
    struct stat info;
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

    sprintf(mesaj, "%sDIRECTOR: %s\n", spatii, nume_director);
    write(snapshot, mesaj, strlen(mesaj));

    while(((intrare = readdir(dir)) != NULL)){
        if(strcmp (intrare->d_name, ".") ==0 || strcmp (intrare->d_name, "..")==0)
            continue;

        snprintf(cale, sizeof(cale), "%s/%s", nume_director, intrare->d_name);

        if((lstat(cale, &info)) < 0){
            //folosim lstat->daca avem leg simbolica, stat nu afiseaza atributele din legatura, le va afisa din fisier
            perror("Eroare la lstat\n");
            exit(1);
        }

        if(S_ISDIR(info.st_mode)){
            sprintf(mesaj, "    %s ---> DIRECTOR ---> Dimeniune %d bytes ---> Last access time %s", cale, intrare->d_reclen, ctime(&info.st_atime));
            write(snapshot, mesaj, strlen(mesaj));
            write(snapshot, "\n", strlen("\n"));
            parcurgere_director(cale, snapshot, nivel + 1);
        }else{
            if(S_ISLNK(info.st_mode)){
                sprintf(mesaj, "    %s ---> LINK SIMBOLIC ---> Dimeniune %d bytes ---> Last access time %s", cale, intrare->d_reclen, ctime(&info.st_atime));
                write(snapshot, mesaj, strlen(mesaj));
                n=readlink(cale, cale_link, sizeof(cale_link));  //citim continutul leg simbolice
                cale_link[n]='\0';
                sprintf(mesaj, "%s  %s -> %s\n", spatii, cale, cale_link);
                write(snapshot, mesaj, strlen(mesaj));
                write(snapshot, "\n", strlen("\n"));
            }else{
                sprintf(mesaj, "    %s ---> REGULAR FILE ---> Dimeniune %d bytes ---> Last access time %s", cale, intrare->d_reclen, ctime(&info.st_atime));
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

int main( int argc, char **argv )
{
    DIR *d;
    int ok;

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

    //aici ma gandeam ca trebuie sa verificam si daca directorul are numele SNAPSHOT-uri(in cazul meu), ca asa ii putem da oricare director si va functiona oricum
    if((ok == 0) && ((strcmp(argv[1], "-o")) == 0) && ((strcmp(argv[2], "SNAPSHOT-uri")) == 0)){
        if((d = opendir(argv[2])) == NULL){
            perror("Argumentul 2 din linia de comanda nu este director\n");
            exit(-1);
        } 

        for(int i = 3; i < argc; i++){
            if((d = opendir( argv[i] )) == NULL)
                continue;
            else{
                /*
                Modifica aceasta parte, ca eu in moment ce rulez aplicatie sa nu fie nevoie sa creez acest director manual
                */
                if((chdir("SNAPSHOT-uri")) == -1){ //Dar unde se afla acest director ? De regula variabele statice, se extrag intr-un fisier aparte, si din acesta se extrag date )
                    perror("Nu exista directorul SNAPSHOT-uri\n");
                    exit(-1);
                }
                int f; //Fara astfel de denumiri, faptul ca e fisier trebuie sa fie denumit explicit
                if((f = open(numeFisier(argv[i]), O_CREAT | O_RDWR, S_IWUSR | S_IRUSR | S_IXUSR)) == -1){
                    perror("eroare la deschideree\n");
                    exit(-1);
                }
                if((chdir("..")) == -1){ //Aici nu am inteles ce se intampla !
                    exit(-1);
                }
                parcurgere_director(argv[i], f, 0);

                close(f);
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
