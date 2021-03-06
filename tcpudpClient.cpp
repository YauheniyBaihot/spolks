#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <arpa/inet.h>
#include <utility>

using namespace std;

void RunTcpClient(char * adress,int port, char *path);
void RunUdpClient(char * adress,int port, char *path);
char getHor(char *ms,int count);


int main (int argc, char *argv[])
{

	if(argc != 5)
	{
		perror("wrong arguments");
		exit(1);
	}
	else
	{	
		char *address = argv[2];
		int port = atoi(argv[3]);
		char *path = argv[4];
		if(!strcmp(argv[1],"tcp"))
			RunTcpClient(address,port,path);
		else
		{
			if(!strcmp(argv[1],"udp"))
				RunUdpClient(address,port,path);
			else
				return 0;
		}
	}
	return 0;
}



void RunTcpClient(char * address,int port, char *path)
{
	
	int sock;
	struct sockaddr_in addr;
	
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock <0)
	{
		perror("socket");
		exit(1);
	}
	
	addr.sin_family = AF_INET;
    	addr.sin_port = htons(port);
	inet_aton(address, &addr.sin_addr);
	
	if(connect(sock,(struct sockaddr *)&addr,sizeof(addr)) < 0)
	{
		perror("connect");
		exit(1);		
	}
	
	string fileName;
	cout << "Enter file you want to download"<< "\n";
	cin >> fileName;
	send(sock,fileName.c_str(), fileName.length(),0);
	int sizeOfFile;
	recv(sock,&sizeOfFile,sizeof(sizeOfFile),0);
	if(sizeOfFile == 0)
	{
		perror("No file on server");
		exit(1);
	}
	cout << "File size - " << sizeOfFile << "\n";
	
	string filePath(path);
	
	filePath += fileName;
	cout << filePath << "\n";
	FILE *fileToSave = fopen(filePath.c_str(),"a+");
	if(fileToSave == NULL)
	{
		perror("error while opening file");
		exit(1);
	}
	fseek(fileToSave,0,SEEK_END);
	int curPossition = ftell(fileToSave);
	if(curPossition != 0)	
	{
		cout << "Continue downloading?(y/n)" << "\n";
		char answer;
		cin >> answer;
		if(answer != 'y')
			curPossition = 0;
	}
	send(sock,&curPossition,sizeof(curPossition),0);
	char buffer[1024];
	int readbytes;
	while(curPossition < sizeOfFile)
	{
		if(sockatmark(sock) == 1) //receaving oob data
		{
			char oobBuf;
			int count = recv(sock,&oobBuf,1,MSG_OOB);
			if(count == -1)
			{
				perror("error oob");
				
			}
			cout << oobBuf;
		}
		readbytes = recv(sock,&buffer,sizeof(buffer),0);
		if(readbytes > 0)
		{
			fwrite(buffer,sizeof(char),readbytes,fileToSave);
		}
		if(readbytes <0)
		{
			fclose(fileToSave);
			perror("While recive");
			exit(1);
		}
		curPossition += readbytes;
	}
	fclose(fileToSave);
	cout<< "Ready" << "\n";
	close(sock);
}



void RunUdpClient(char * address,int port, char *path)
{
	int sock;
	struct sockaddr_in addr;
	
	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if(sock <0)
	{
		perror("socket");
		exit(1);
	}
	
	addr.sin_family = AF_INET;
    	addr.sin_port = htons(port);
	inet_aton(address, &addr.sin_addr);
	
	bind(sock,(struct sockaddr *)&addr,sizeof(addr));
	//connect(sock,(struct sockaddr *)&addr,sizeof(addr));	
	
	string fileName;
	cout << "Enter file you want to download udp"<< "\n";
	cin >> fileName;
	sendto(sock,fileName.c_str(), fileName.length(),0,(struct sockaddr *)&addr,sizeof(addr));
	int sizeOfFile;
	int g = recvfrom(sock,&sizeOfFile,sizeof(sizeOfFile),0,(struct sockaddr *)&addr,sizeof(addr));
	if(g < 0)
	{
		perror("no response from server");
		exit(1);
	}
	cout << g << "\n";
	if(sizeOfFile == 0)
	{
		cout <<"No file on server";
		return;
	}
	cout << "File size - " << sizeOfFile << "\n";
	
	string filePath(path);
	
	filePath += fileName;
	cout << filePath << "\n";
	FILE *fileToSave = fopen(filePath.c_str(),"a+");
	if(fileToSave == NULL)
	{
		perror("error while opening file");
		exit(1);
	}
	fseek(fileToSave,0,SEEK_END);
	int curPossition = ftell(fileToSave);
	if(curPossition != 0)	
	{
		cout << "Continue downloading?(y/n)" << "\n";
		char answer;
		cin >> answer;
		if(answer != 'y')
			curPossition = 0;
	}
	send(sock,&curPossition,sizeof(curPossition),0);
	char buffer[1024];
	int readbytes,horSum;
	while(curPossition < sizeOfFile)
	{
		readbytes = recv(sock,&buffer,sizeof(buffer),0);
		if(readbytes > 0)
		{
			horSum = getHor(buffer,readbytes - 1);
			if(horSum == buffer[readbytes])
			{
				fwrite(buffer,sizeof(char),readbytes,fileToSave);
			}
			send(sock,&horSum,sizeof(horSum),0);
		}
		if(readbytes <0)
		{
			fclose(fileToSave);
			perror("While recive");
			exit(1);
		}
		curPossition += readbytes;
	}
	fclose(fileToSave);
	cout<< "Ready" << "\n";
	close(sock);
}


char getHor(char *ms,int count)
{
	char result=0;
	for(int i=0;i<count;i++)
		result = result ^ ms[i];
	return result;
}

