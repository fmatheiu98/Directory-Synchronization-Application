#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <netdb.h>
#include <dirent.h>
#include <wait.h>
#include <time.h>
#define SERVER_PORT 56793
#define MAXBUF 4096
#define COMANDA 200
char delimitator[]={"\n"};
int sockfd,connfd;


void trimitere_X(char *name,int tip)
{
    char buf[MAXBUF];
    bzero(buf,MAXBUF);
    switch(tip)
    {
        case 0: snprintf(buf, COMANDA, "director %s%s",name, delimitator);
            break;
        case 1: snprintf(buf, COMANDA, "filename %s%s",name, delimitator);
            break;
        case 2: snprintf(buf, COMANDA, "date_fisier %s%s",name, delimitator);
            break;
        case 3: snprintf(buf, COMANDA, "inexistent_directory %s%s",name,delimitator);
            break;
    }
    if(send(connfd, buf, COMANDA,0)<0)
        perror("Server: Eroare la send(1)"); 
}

void parcurgere_dir(char *dirname)
{
    DIR *dir;
    if((dir = opendir(dirname))==NULL)
    {
        perror("Server: Eroare deschidere director / director inexistent.\n");
        trimitere_X(dirname,3);
        exit(1);
    }
    
    struct dirent *d;
    while((d = readdir(dir))!=NULL)
    {     
        char c[200];
        bzero(c,200);
        strcpy(c,dirname);
        strcat(c,"/");
        strcat(c,d->d_name);
        struct stat st;
        if(lstat(c,&st)<0)
        {
            perror("Server: Eroare lstat\n");
            exit(10);
        }
        //daca intrarea e fisier obisnuit, se transmite catre client
        if(S_ISREG(st.st_mode))
        {
            int fd,nread;
            if (-1 == (fd = open(c, O_RDONLY))) {
                perror("Server: Eroare deschidere fisier.\n");
                exit(1);
            }
            trimitere_X(c,1);
            trimitere_X(c,2);
            
            char continut[MAXBUF];
            bzero(continut,MAXBUF);

            while (0 < (nread = read(fd, continut, MAXBUF)))
            {
                continut[strlen(continut)-1]='\0';
                if(send(connfd,continut,MAXBUF,0)<0)
                    perror("Server: Eroare la send\n");
            }
            
            if (nread < 0) {
                perror("Server: Eroare citire din fisier\n");
                exit(1);
            }

            close(fd);
        }
        else //daca intrarea e director, se apeleaza recursiv parcurgerea
        if(S_ISDIR(st.st_mode) && strcmp(d->d_name,".")!=0 && strcmp(d->d_name,"..")!=0)
        {
            
            trimitere_X(c,0);
            parcurgere_dir(c);
        }
    }
    
    closedir(dir);
}

int invalid_d=0;
void sincronizare(int connfd,char *sendline)
{
    int n;
    n = strcspn(sendline," ");
    
    if(strncmp(sendline,"trimite_director",n) == 0)
    {
        strcpy(sendline, sendline+n+1);
        sendline[strlen(sendline)]='\0';
        DIR *dir_test;
        if((dir_test = opendir(sendline))==NULL)
        {
            perror("Server: Eroare deschidere director / director inexistent.\n");
            trimitere_X(sendline,3);
            invalid_d=1;
        }
        else
        {
            trimitere_X(sendline,0);
            parcurgere_dir(sendline);
        }
    }
    else
    if(strncmp(sendline,"quit_client",n) == 0)
    {
        close(connfd);
        exit(0);
    }
}

int main() 
{
    struct sockaddr_in local_a,remote_a;
    socklen_t len;
    pid_t pid;
    //se creeaza un descriptor sockfd pt. domeniul IPv4, transport TCP(SOCK_STREAM)
    if (-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0))) {
        perror("Server: Eroare creare socket\n");
        exit(1);
    }
    
    //setari pentru adresa si port
    local_a.sin_family = AF_INET; 
    local_a.sin_addr.s_addr = INADDR_ANY; 
    local_a.sin_port = htons( SERVER_PORT );
    
    //leaga socket-ul de IP-ul si portul dat.
    if (-1 == bind(sockfd, (struct sockaddr *) &local_a,sizeof(local_a))) {
        perror("Server: Eroare la bind\n");
        exit(1);
    }
    
    //listen - asteapta conexiuni de la clienti(maxim 5 in asteptare)
    if (-1 == listen(sockfd, 5)) {
        perror("Server: Eroare la listen");
        exit(1);
    }
    len = sizeof(remote_a);

    for(;;) 
    {
        //accepta o conexiune cu un client, creeaza un nou socket si returneza un nou descriptor ce se refera
        //la acea conexiune
        connfd = accept(sockfd,(struct sockaddr*) & remote_a, &len);
        if (connfd < 0)
        {
            perror("Server: Eroare la accept()");
            exit(1);
        }
        printf("Conexiunea cu clientul a reusit. Se asteapta un request...\n\n");
        pid = fork();//creeaza un proces fiu pentru fiecare client care porneste

        switch (pid) {
        case -1:
            perror("Server: Eroare la fork()");
            exit(1);
        case 0: //proces fiu pentru fiecare client
            close(sockfd);
            char sendline[255];
            if(recv(connfd, sendline, COMANDA, 0)<0)
                {
                    perror("Eroare recv");
                    exit(1);
                }
            sincronizare(connfd,sendline);
            if(invalid_d==0)
                printf("Director %s trimis cu succes la client.\n\n",sendline);
            else
            {
                printf("Director %s inexistent pe server.\n\n",sendline);
            }
            
            close(connfd);//incheierea conexiunii
            exit(0);

        default: //proces parinte
            close(connfd);
        }
    }
    exit(0);
}
