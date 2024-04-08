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

DIR *dir;
int snapchot;

void parcurgere_director( char *nume_director, int snapchot )
{
    struct dirent *intrare;
    struct stat info;
    char cale[PATH_MAX];    //CE DIMENSIUNE SA DAU LA PATH? MERGE SI ASA
    char cale_link[PATH_MAX +1];
    int n;
    
    if(!(dir = opendir(nume_director))) //verificam daca putem deschide directorul 
    {
        perror("eroare la opendir");
        exit(-1);
    }

    char mesaj[100000];
    sprintf(mesaj,"\nDIRECTOR: %s\n",nume_director);
    write(snapchot,mesaj,strlen(mesaj));
    
    if(((intrare = readdir(dir)) != NULL)) //daca e gol
    {
        sprintf(mesaj,"%s   Dimeniune %d bytes, Last access time %s", cale, intrare->d_reclen, ctime(&info.st_atime)); //afisam despre dir in care suntem
        write(snapchot,mesaj,strlen(mesaj));
    }
    else
    {
        perror("eroare la readdir\n");
        exit(-1);
    }

    while(((intrare = readdir( dir)) != NULL)) //daca directorul nu e NULL
    {
        if(strcmp (intrare->d_name, ".") ==0 || strcmp (intrare->d_name, "..")==0)
            continue;
        write(snapchot,"    ->",strlen("    ->"));
        snprintf(cale, sizeof(cale), "%s/%s", nume_director, intrare->d_name);

        if((lstat(cale, &info))<0)   // stat->citeste atributele unui fisier
        {
            //folosim lstat->daca avem leg simbolica, stat nu afiseaza atributele din legatura, le va afisa din fisier
            sprintf(mesaj, "%s: ", cale);
            write(snapchot, mesaj, strlen(mesaj));
            fflush(stdout);                        //iar lstat afiseaza atributele din legatura 
            perror("eroare la lstat");
            exit(1);
        }

        if(S_ISDIR(info.st_mode))
        {
            sprintf(mesaj, "%s->DIRECTOR, Dimeniune %d bytes, Last access time %s", cale, intrare->d_reclen, ctime(&info.st_atime));
            write(snapchot, mesaj,strlen(mesaj));
            parcurgere_director(cale, snapchot);
        }
        else
        {
            if(S_ISLNK(info.st_mode))
            {
                sprintf(mesaj, "%s->LINK SIMBOLIC, Dimeniune %d bytes, Last access time %s", cale, intrare->d_reclen, ctime(&info.st_atime));
                write(snapchot,mesaj,strlen(mesaj));
                n=readlink(cale, cale_link, sizeof(cale_link));
                cale_link[n]='\0';
                sprintf(mesaj, "%s -> %s\n", cale, cale_link);
                write(snapchot,mesaj,strlen(mesaj));
            }
            else
            {
                sprintf(mesaj, "%s->REGULAR FILE, Dimeniune %d bytes, Last access time %s", cale, intrare->d_reclen, ctime(&info.st_atime));
                write(snapchot,mesaj,strlen(mesaj));

                if(info.st_mode & S_IXUSR || info.st_mode & S_IXGRP || info.st_mode & S_IXOTH)
                    write(snapchot,"*",strlen("*"));
                write(snapchot,"\n",strlen("\n"));

            }
     
        }
    }

       closedir(dir);
}

        //S_IXUSR - execute/search permission, owner
        //S_IXGRP - execute/search permission, group
        //S_IXOTH - execute/search permission, others

int main( int argc, char **argv )
{
    if( argc < 2 )    //verificam daca avem cel putin un director ca si argument
    {
        perror("Numar invalid de argumente\n");
        exit(-1);
    }

    if(( dir = opendir( argv[1] )) == NULL )   //verificam daca argv[1] e director
    {
        perror("Argumentul nu este director\n");
        exit(-1);
    }

    if(( snapchot = open("snapchot.txt", O_WRONLY )) == -1) //deschidem un fisier pentru a salva in el metadatele
    {
        perror("Eroare la deschiderea snapchot-ului\n");
        exit(-1);
    }

    parcurgere_director(argv[1],snapchot);
    close(snapchot);

    return 0;
}
    