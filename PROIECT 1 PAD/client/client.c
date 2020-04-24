#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <dirent.h>

#define SERVER_PORT 56793
#define MAXBUF 4096
#define COMANDA 200

void actiuni_client(int sockfd,char *sendline)
{
    if(send(sockfd, sendline, COMANDA, 0) <= 0)
    {
        perror("Client: Eroare la send(1)."); 
    }
    int n;
    int fd;
    char buf[MAXBUF];
    printf("Mesaje primite de la server:\n");
    
    while ( (n = recv(sockfd, buf, COMANDA, 0)) > 0)
    {       
        int y = strcspn(buf," \n");//toata comanda
        char nume_x[MAXBUF];
        strcpy(nume_x,buf+y+1);
        nume_x[strlen(nume_x)-1]='\0';//nume_x = nume fisier/director
        
        if(strncmp(buf,"director",y)==0)
        {
            mkdir(nume_x,0744);
        }
        
        if(strncmp(buf,"filename",y)==0)
        {   
            //creare fisier nou cu numele primit
            fd = open(nume_x, O_WRONLY | O_CREAT | O_TRUNC, 0744);
            if (fd == -1) {
                perror("Client: Eroare deschidere fisier creat.\n");
                exit(1);
            }
        }
        
        if(strncmp(buf,"date_fisier",y)==0)
        {
            char text_primit[MAXBUF];
            if(recv(sockfd, text_primit, MAXBUF, 0)<0) //primire continut fisier de la server
                perror("Client: Eroare la recv(1)."); 
            if(write(fd,text_primit,strlen(text_primit))<0)
                perror("Client: Eroare la write(1).");  
        }

        if(strncmp(buf,"inexistent_directory",20)==0)
        {
            char text_primit[MAXBUF];
            if(recv(sockfd, text_primit, MAXBUF, 0)<0)
                perror("Client: Eroare la recv(2)."); 
            printf("Director inexistent pe server.\n");
            close(sockfd);
            exit(11);
        }
        printf("%s\n",buf);
        bzero(buf,MAXBUF);
        
    }
    if(n<0)
        perror("Eroare recv while.\n");
}

int main(int argc, char* argv[]) {
  
    int sockfd;
    struct sockaddr_in servaddr;
    if (argc != 2) {
        printf("Client -> Mod de apelare: %s adresa\n", argv[0]);
        exit(1);
    }
    
    //se creeaza un descriptor sockfd pt. domeniul IPv4, transport TCP(SOCK_STREAM)
    if (-1 == (sockfd = socket(AF_INET, SOCK_STREAM, 0))) {
        perror("Client: Eroare la socket\n");
        exit(1);
    }

    //setari pentru adresa si port
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr= inet_addr(argv[1]);
    servaddr.sin_port =  htons(SERVER_PORT);
    
    //realizarea conexiunii cu serverul
    if (-1 == connect(sockfd, (struct sockaddr*) & servaddr,sizeof(servaddr))) {
        perror("Client: Conectarea la server a esuat\n");
        exit(1);
    }
    printf("Conexiunea cu serverul s-a efectuat.\n\n");
    printf("Comenzi valide pentru server:\n");
    printf("trimite_director <nume_director de pe server>\n");
    for(;;)
    {
        char sendline[200];
        printf("Dati comanda pentru server: ");
        fgets(sendline,COMANDA,stdin);
        sendline[strlen(sendline)-1]='\0';
        if(strncmp(sendline,"trimite_director",16)==0)//validare comanda pentru server
        {
            actiuni_client(sockfd,sendline);
            printf("Director primit cu succes.\n");
            close(sockfd);//incheierea conexiunii
            exit(0);
        }

    }
}
