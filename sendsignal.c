#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <winsock2.h>
#include "stdafx.h"
#include <sys/stat.h>
#include <windows.h>
#pragma comment(lib,"ws2_32.lib") //-lws2_32


#define MSG_SIZE 68			// message size

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int  portno, n;
    char buffer[MSG_SIZE];
    char buffer2[MSG_SIZE];
    char buffer3[MSG_SIZE];
    char ch;
    int t; //waste time
    FILE *fd;
    int row=0;
    int column=0;
    //struct stat buf;



    if (argc < 2)	// not enough arguments
    {
       fprintf(stderr,"usage %s hostname\n", argv[0]);
       exit(0);
    }


		WORD sockVersion=MAKEWORD(2,2);
		WSADATA data;
		
		if(WSAStartup(sockVersion,&data)!=0)
		{
			return 0;		
		}
		
		SOCKET sockfd=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
		if (sockfd==INVALID_SOCKET)
		{
				printf("invalid socket!");
				return 0;
		}
		struct sockaddr_in serAddr;
		serAddr.sin_family=AF_INET;
		portno = 2050;		// port # 2050
		serAddr.sin_port=htons(portno);
		serAddr.sin_addr.S_un.S_addr=inet_addr(argv[1]);
			
    
    //open file
    while((fd=fopen("EEG.txt","r"))==NULL)
    { t++;}

     // establish connection to the server  
    if(connect(sockfd,(struct sockaddr *)&serAddr,sizeof(serAddr))==SOCKET_ERROR)
		{
			printf("connect error!");
			closesocket(sockfd);
			return 0;
		}
			printf("connect");

		
    while(1)
    {
			if(row==0 && fgets(buffer,MSG_SIZE,fd)!=NULL) //the first row don't need to send
			{ 
				printf("aa:%s",buffer);
				//stat("EEG.txt",&buf);
				row++;
			}
    	if(row !=0 )
    	{       
    		if(fgets(buffer,MSG_SIZE,fd)!=NULL)
    		{
    			//stat("EEG.txt",&buf); 
    			printf("new:%d,%s",strlen(buffer),buffer);
 					// send message                             
        	n=send(sockfd,buffer,MSG_SIZE,0);                                    
			  	recv(sockfd,buffer2,MSG_SIZE,0); 
			  	row++;
			  }
			  else //if EOF wait then check again
			  {
			  	Sleep(500);	
			  	if(fgets(buffer,MSG_SIZE,fd)==NULL)
			  	{	break;} //end send
			  	n=send(sockfd,buffer,MSG_SIZE,0);                                    
			  	recv(sockfd,buffer2,MSG_SIZE,0); 
			  	row++;
			  }
			}          
    }
    closesocket(sockfd);			// close socket
    fclose(fd);
		WSACleanup();
    return 0;
}
