#include <string>
#include <iostream>
#include <fstream>
#include <map>
#include <utility>
#include <cstdlib>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
	 
using namespace std;

bool ContainsFile(string key);
void Init();
int SendAllData(int sock,void *buf,int len,int flags);

char * configFileName = "/home/yellow/Рабочий стол/config.txt"; 
map<string, string> fileNames;
int main(int argc, char *argv[])
{
	
	Init();
	char *address = argv[1];
	int port = atoi(argv[2]);
	cout << "Starting server on address - "<<address << ", port - " << port << "\n";
	
	int sock, listener;
	struct sockaddr_in addr;
	
	
	
	listener = socket(AF_INET, SOCK_STREAM, 0);
	if(listener < 0)
	{
		perror("socket");
		exit(1);
	}
	
	addr.sin_family = AF_INET;
    	addr.sin_port = htons(port);
	inet_aton(address, &addr.sin_addr);
	
	if(bind(listener, (struct sockaddr *)&addr, sizeof(addr))<0)
	{
		perror("bind");
		exit(1);
	}
	listen(listener, 1);
	
	while(1)
	{
		char buf[1024];
		int bytes_read;
		sock = accept(listener, NULL, NULL);
		if(sock < 0)
		{
			perror("accept");
			exit(1);
		}		
		bytes_read = recv(sock, buf, 1024, 0);
		if(bytes_read <= 0) 
		{
			close(sock);
		}
		else 
		{
			string key(buf,bytes_read);
			if(ContainsFile(key))
			{
				string name = fileNames[key];
				FILE *fileToSend = fopen(name.c_str(),"r");
				if(fileToSend == NULL)
				{
					cout<< "Cannot open file" << "\n";
					close(sock);
				}
				else
				{
					fseek(fileToSend, 0, SEEK_END);
					int sz = ftell(fileToSend);
					send(sock,(char *)&sz,sizeof(sz),0);
					int possition;
					recv(sock,&possition,sizeof(possition),0);
					fseek(fileToSend,possition, SEEK_SET);
					while(possition < sz)
					{
						if(possition%10000 == 0)
						{
							send(sock,"*",1,MSG_OOB);
						}
						char buffer[1024];
						int read = fread(buffer,sizeof(char),1024,fileToSend);
						int send = SendAllData(sock,buffer,read,0);
						if(send != read)
						{
							perror("sending");
							exit(1);
						}
						possition += read;
					}
					cout << "ready" << "\n";
					fclose(fileToSend);
					close(sock);
				}
			}
			else
			{
				int size = 0;
				send(sock,&size,sizeof(size),0);
				close(sock);
			}
			
		}    
		

	}
	
	
	
	

}

void Init()
{
	string lineName;
	string linePath;
	ifstream fileWithNames (configFileName);
	if (fileWithNames.is_open())
	{
		while ( getline (fileWithNames,lineName) )
		{	
			getline (fileWithNames,linePath);
			fileNames.insert(map<string,string>::value_type(lineName,linePath));
		}
		fileWithNames.close();
	}
	else 
	{
		cout << "Unable to open config file \n";
		exit(EXIT_FAILURE);
	}
}

bool ContainsFile(string key)
{
	
	//key =  key.substr(0,key.length() -1);
	map<string,string>::iterator it = fileNames.find(key);
	if(it == fileNames.end())
		return false;
	else
		return true;
}

int SendAllData(int sock,void *buf,int len,int flags)
{
	int totalSend = 0;
	int n;
	
	while(totalSend < len)
	{
		n = send(sock,buf+totalSend,len - totalSend,flags);
		if(n== -1)
			break;
		totalSend += n;
	}
	
	return (n==-1 ? -1 : totalSend);
}
