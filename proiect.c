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

void parcurgere_director(char *nume_director, int snapchot)
{
    struct dirent *intrare;
    struct stat info;
    char cale[PATH_MAX];   
    char cale_link[PATH_MAX + 1];  //calea-calea catre fis curent
    int n;
    DIR *dir;
    
    if(!(dir = opendir(nume_director))) //verificam daca putem deschide directorul 
    {
        perror("eroare la opendir");
        exit(-1);
    }

    char mesaj[100000];
    sprintf(mesaj, "\nDIRECTOR: %s\n",  nume_director);
    write(snapchot, mesaj, strlen(mesaj));
    
    if(((intrare = readdir(dir)) != NULL)) //daca nu e e gol afisam info despre el
    {
        sprintf(mesaj, "%s   Dimeniune %d bytes, Last access time %s", cale, intrare->d_reclen, ctime(&info.st_atime)); //afisam despre dir in care suntem
        write(snapchot, mesaj, strlen(mesaj));
    }
    else
    {
        perror("eroare la readdir\n");
        exit(-1);
    }

    while(((intrare = readdir( dir)) != NULL)) //parcurgem directorul
    {
        if(strcmp (intrare->d_name, ".") ==0 || strcmp (intrare->d_name, "..")==0)
            continue;

        write(snapchot,"    ->",strlen("    ->"));
        snprintf(cale, sizeof(cale), "%s/%s", nume_director, intrare->d_name);

        if((lstat(cale, &info)) < 0)   // stat->citeste atributele unui fisier
        {                               //daca e <0  inseamna nu avem atribute
        
            //folosim lstat->daca avem leg simbolica, stat nu afiseaza atributele din legatura, le va afisa din fisier
            perror("eroare la lstat");
            exit(1);
        }

        if(S_ISDIR(info.st_mode))  //daca e director
        {
            sprintf(mesaj, "%s->DIRECTOR, Dimeniune %d bytes, Last access time %s", cale, intrare->d_reclen, ctime(&info.st_atime));
            write(snapchot, mesaj, strlen(mesaj));
            parcurgere_director(cale, snapchot);
        }
        else
        {
            if(S_ISLNK(info.st_mode))  //daca e leg simbolica
            {
                sprintf(mesaj, "%s->LINK SIMBOLIC, Dimeniune %d bytes, Last access time %s", cale, intrare->d_reclen, ctime(&info.st_atime));
                write(snapchot,mesaj,strlen(mesaj));
                n=readlink(cale, cale_link, sizeof(cale_link));  //citim continutul leg simbolice
                cale_link[n]='\0';
                sprintf(mesaj, "%s -> %s\n", cale, cale_link);
                write(snapchot, mesaj, strlen(mesaj));
            }
            else
            {
                sprintf(mesaj, "%s->REGULAR FILE, Dimeniune %d bytes, Last access time %s", cale, intrare->d_reclen, ctime(&info.st_atime));
                write(snapchot, mesaj, strlen(mesaj));

                /*if(info.st_mode & S_IXUSR || info.st_mode & S_IXGRP || info.st_mode & S_IXOTH)  //verificam daca avem drepturi de executie, dar nu cred ca ne e de folos
                    write(snapchot,"*",strlen("*"));*/
                write(snapchot,"\n",strlen("\n"));

            }
     
        }
    }

       closedir(dir);
}

        //S_IXUSR - execute/search permission, owner
        //S_IXGRP - execute/search permission, group
        //S_IXOTH - execute/search permission, others


//Functia aceasta ne denumeste fiecare fisier sub forma: "snapchot_[numele_directorului].txt"
char* numeFisier(char *director)
{
    char *nume = basename(director); //functia basename ne da numele de baza al directorului

    static char fisier[100];  //avem nevoie de static pentru ca variabila noastra sa ramana in memorie 
    sprintf(fisier, "snapchot_%s.txt", nume);

    return fisier;
}

int main( int argc, char **argv )
{
    //int snapc;
    DIR *d;
    int ok;

    if( argc < 2 )    //verificam daca avem cel putin un director ca si argument
    {
        perror("Numar invalid de argumente\n");
        exit(-1);
    }

    /*if(( d = opendir( argv[1] )) == NULL )   //verificam daca argv[1] e director
    {
        perror("Argumentul nu este director\n");
        exit(-1);
    }*/

    //AICI DESCHID UN FISIER DENUMIT snapchot.txt, dar am incercat mai sus sa fac o functie care imi deschide 
    //un fisier pentru fiecare director 

    /*if(( snapc = open("snapchot.txt", O_WRONLY )) == -1) //deschidem un fisier pentru a salva in el metadatele
    {
        perror("Eroare la deschiderea snapchot-ului\n");
        exit(-1);
    }*/

    if(argc > 10)
    {
        perror("argumente multe\n");
        exit(-1);
    }
    else
    {
        ok = 0;
        for(int i = 3; i < argc-1; i++)  //verificam daca sunt distincte
        {
            for(int j = i+1; j < argc; j++)
            {
                if((strcmp(argv[i], argv[j])) == 0)  //daca avem arg egale ok va deveni 1
                {
                    ok = 1;
                    perror("argumente egale\n");
                    exit(-1);
                }
            }

        }
    }

    //aici nu am inteles daca trebuie sa facem un snapchot pentru fiecare director, sau intr-unul singur sa afisam tot
    //AICI E CU UN SINGUR SNAPCHOT PENTRU TOATE 

    /*if(ok == 0)  //daca sunt arg distincte, atunci nu se intra in if-ul de mai sus si ok nu devine 1
    {
        for(int i = 1; i < argc; i++)
        {
            if((d = opendir( argv[i] )) == NULL) //verificam daca argumentele din linia de comanda sunt directoare
            {
                perror("arg nu e director\n");
            }
            else
            {
                parcurgere_director(argv[i], snapc);
            }
        }
    }
    else
    {
        perror("Exista argumente egale\n");
    }

    close(snapc); */

    DIR *output;

        //aici ma gandeam ca trebuie sa verificam si daca directorul are numele SNAPCHOT-uri(in cazul meu), ca asa ii putem da oricare director si atunci va functiona
     if((ok == 0) && ((strcmp(argv[1], "-o")) == 0) && ((strcmp(argv[2], "SNAPCHOT-uri")) == 0))  //daca sunt arg distincte, atunci nu se intra in if-ul de mai sus si ok nu devine 1
    {                                           //verificam si daca argv[1] e -o
        if((output = opendir(argv[2])) == NULL) //verificam daca argv[2] e un director
        {
            perror("eroare\n");
            exit(-1);
        } 
        for(int i = 3; i < argc; i++)
        {
            if((d = opendir( argv[i] )) == NULL) //verificam daca argumentele din linia de comanda sunt directoare
            {
                perror("arg nu e director\n");
            }
            else
            {
      //!!!!!          aici trebuie sa imi deschid snapchot-urile directoarelor in SNAPCHOT-uri

                int f;
                if((f = open(numeFisier(argv[i]), O_CREAT | O_RDWR, S_IWUSR | S_IRUSR | S_IXUSR)) == -1)  //am folosit O_CREAT pentru a-l crea si O_RDWR pentru a putea scrie/citi in/din el, iar S_IWUSR si celelate trebuie folosite, deoarece cu O_CREAT trebuie sa adaugam si param mode in functia open
                {
                    perror("eroare la deschideree\n");
                    exit(-1);
                }

                parcurgere_director(argv[i],f);
                close(f);
            }
        }
    }
    else
    {
        perror("Exista argumente egale sau nu gasim -o sau nu avem dir SNAPCHOT-uri\n");
    }

    return 0;
}
    