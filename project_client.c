#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include <rtai_lxrt.h>
#include <pthread.h>

#define MSG_SIZE 68			// message size
int sockfd;


void error(const char *msg)
{
    perror(msg);
    exit(0);
}

void readfromnetwork(int socksendto)
{
	//server to listen
	int sockfd, newsockfd, portno;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
    int flag=1;
	int n;
	char buffer[MSG_SIZE]; //for real signal
	char buffer2[MSG_SIZE]; //send to client
	char buffer3[MSG_SIZE]; //wait to server


	sockfd = socket(AF_INET, SOCK_STREAM, 0); // Creates socket. Connection based.
	if (sockfd < 0)
	    error("ERROR opening socket");

	// fill in fields
	bzero((char *) &serv_addr, sizeof(serv_addr));
	portno = 2050;	// get port number from input
	serv_addr.sin_family = AF_INET;		 // symbol constant for Internet domain
	serv_addr.sin_addr.s_addr = INADDR_ANY; // IP address of the machine on which
												 // the server is running
	serv_addr.sin_port = htons(portno);	 // port number

	// binds the socket to the address of the host and the port number
	if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
	    error("ERROR on binding");

	listen(sockfd, 5);			// listen for connections
	clilen = sizeof(cli_addr);	// size of structure
	// blocks until a client connects to the server
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    close(sockfd);

	memset(buffer2,0,MSG_SIZE);
	strcpy(buffer2,"go"); //go message tell client to send

	while(flag==1)
    {
    	n=read(newsockfd,buffer,MSG_SIZE); //read data
    	if (n<=0) //if no data end connection
    	{
    		flag=0;
    		close(newsockfd);
    	}
    	if (write(socksendto,buffer,MSG_SIZE) < 0) //write to server
		{
    		error("ERROR writing to socket");
		}

    	read(socksendto,buffer2,MSG_SIZE);//block until server reply
    	write(newsockfd,buffer3,MSG_SIZE); //tell client go

    }
  fclose(fd);
	exit(0);
}

void readfromtxt(FILE *fd,int sockfd,int* stopflag)
{
	char buffer[MSG_SIZE];//send
	char buffer2[MSG_SIZE]; //receive
	//first line is parameter
	fgets(buffer,MSG_SIZE,fd);
	printf("%s\n",buffer);

	bzero(buffer,MSG_SIZE);
	while(*stopflag==0 && fgets(buffer,MSG_SIZE,fd)!=NULL) //read every line
	{
		// send message
		if (write(sockfd,buffer,MSG_SIZE) < 0)
			error("ERROR writing to socket");
		bzero(buffer,MSG_SIZE);

		read(sockfd,buffer2,MSG_SIZE);//block until server reply
	}
	fclose(fd);
	exit(0);
}
void *Thread1(void *ptr) //stop the connection is type stop
{
	int *stopflag=(int *)ptr;
	char buffer[MSG_SIZE];
	while(*stopflag==0)
	{
		fgets(buffer,MSG_SIZE-1,stdin); //check the user type
		if(strcmp(buffer,"stop\n")==0)
		{
			*stopflag=1;
		}
	}
	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int portno,n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char buffer[MSG_SIZE];

	/*get date*/
	time_t now;
	struct tm *tm_now;
	/*txt file*/
	FILE *fd;
	char txt_name[10];
	/*thread*/
	pthread_t thread1;
	int stopflag;

    if (argc < 5)	// not enough arguments
    {
       fprintf(stderr,"usage %s hostname port usrno signal_type\n", argv[0]);
       exit(0);
    }

    portno = atoi(argv[2]);		// port # was an input.
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // Creates socket. Connection based.
    if (sockfd < 0)
        error("ERROR opening socket");

    server = gethostbyname(argv[1]);  // converts hostname input (e.g. 10.3.52.255)
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    // fill in fields of serv_addr
    bzero((char *) &serv_addr, sizeof(serv_addr));	// sets all values to zero
    serv_addr.sin_family = AF_INET;		// symbol constant for Internet domain

    // copy to serv_addr.sin_addr.s_addr. Function memcpy could be used instead.
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    serv_addr.sin_port = htons(portno);		// fill sin_port field

    // establish connection to the server
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0)
        error("ERROR connecting");


    bzero(buffer,MSG_SIZE);
    strcpy(buffer,"write");
    write(sockfd,buffer,MSG_SIZE); //tell server it want write

  //first message date,usrno
  bzero(buffer,MSG_SIZE);
	time(&now);
	tm_now=localtime(&now);
	strftime(buffer,MSG_SIZE,"%m/%d/%Y",tm_now); //date
	strcat(buffer,",");
	strcat(buffer,argv[3]); //date+usrno
	n=write(sockfd,buffer,MSG_SIZE);


	//check user no
	bzero(buffer,MSG_SIZE);
	n=read(sockfd,buffer,MSG_SIZE);
    if(strcmp(buffer,"stop")==0)
    {
    	printf("user doesn't exist\n");
    	close(sockfd);
    	exit(0);
    }

	//thread: monitor stop
	stopflag=0;
	pthread_create(&thread1,NULL,Thread1,(void*)&stopflag);
	//open txt
	if (atoi(argv[4])==0) //0: real signa,. 1,2,3: txt name
	{
		readfromnetwork(sockfd); //read from real EEG
	}
	else
	{
		sprintf(txt_name,"eeg%s.txt",argv[4]);
		printf("%s\n",txt_name);
		if((fd=fopen(txt_name,"r"))==NULL)
		{
			printf("error");
			exit(1);
		 }
		readfromtxt(fd,sockfd,&stopflag); //real from Simulated EEG
	}

	pthread_join(thread1,NULL);
    close(sockfd);			// close socket
    return 0;
}

