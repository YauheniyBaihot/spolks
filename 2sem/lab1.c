#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <float.h>

#define default_buffer_size = 60 * 1024;
//////////////////////////////////////
//variables


int succsess_count =0;
double sum_time = 0;
double min_time = 0.0;
double max_time = 0.0;
pid_t pid;
int message_size;
int sock;
struct sockaddr_in dest_addr;
struct sockaddr_in source_addr;
char send_buffer[default_buffer_size];
//////////////////////////////////////

//////////////////////////////////////
//defaults
int default_ttl = 255;
int default_size = 56;
int default_count = 100;
int default_broadcast = 0;



int on = 1;
/////////////////////////////////////


typedef struct input_info input_info;

struct input_info {
	int ttl;
	int size;
	int count;
	int broadcast;
	struct in_addr *addr;
};


struct input_info parse_input(int argc,char **argv)
{

	struct input_info info;
	info.ttl = default_ttl;
	info.size = default_size;
	info.count = default_count;
	info.broadcast = default_broadcast;
	if(argc%2 !=0 || argc < 2)
	{
		printf("Wrong input parameters\n");
		exit(-1);
	}
	
	struct hostent *host = gethostbyname(argv[1]);
	if(host == NULL)
	{
		printf("Wrong parameter address: %s\n",argv[1]);
		exit(-1);
	}
	info.addr =  (struct in_addr *) host->h_addr;
	int i,temp;
	for( i=2;i<argc;i=i+2)
	{
		if(strcmp(argv[i],"-c") ==0)
		{
			temp = atoi(argv[i+1]);
			if(temp <1 || temp > 30000)
			{
				printf("Wrong parameter -c: %d , should be between 1 and 65555\n");
				exit(-1);
			}
			info.count = temp;
			continue;
		}
		if(strcmp(argv[i],"-t") == 0)
		{
			temp = atoi(argv[i+1]);
			if(temp <0 || temp > 255)
			{
				printf("Wrong parameter -c: %d , should be between 0 and 255\n");
				exit(-1);
			}
			info.ttl = temp;
			continue;
		}
		if(strcmp(argv[i],"-s") == 0)
		{
			temp = atoi(argv[i+1]);
			if(temp <1 || temp > 65515)
			{
				printf("Wrong parameter -c: %d , should be between 1 and 65515\n");
				exit(-1);
			}
			info.size = temp;
			continue;
		}
		if(strcmp(argv[i],"-b") ==0)
		{
			if(argv[i+1][0] == 'y')
			{
				info.broadcast = 1;
			}
			continue;
		}
		printf("Wrong input parameter: %s\n",argv[i]);
		exit(-1);
		
		
		
	}  
	return info;
}

void parse_output(char *buffer, int length, struct timeval *time_recv)
{
	int ip_length;
	int icmp_length;
	struct ip *ip_packet;
	struct icmp *icmp_packet;
	struct timeval *time_send;
	double ping_time;
	
	ip_packet = (struct ip *) buffer;
	ip_length = ip_packet->ip_hl << 2;
	icmp_packet = (struct icmp *) (buffer + ip_length);
	icmp_length = length - ip_length;
	if(icmp_length < 8)
	{
		printf("icmp_length %d < 8",icmp_length);
	}
	if(icmp_packet->icmp_type == ICMP_ECHOREPLY)
	{
		if(icmp_packet->icmp_id != pid)
			return;
		time_send = (struct timeval *) icmp_packet-> icmp_data;
		tv_sub(time_recv,time_send);
		
		ping_time = time_recv->tv_sec * 1000.0 + time_recv->tv_usec / 1000.0;
		
		succsess_count ++;
		
		sum_time += ping_time;
		if(ping_time < min_time)
			min_time = ping_time;
		if(ping_time > max_time)
			max_time = ping_time;
		printf("%d bytes from %s: icmp_seq= %u, ttl = %d, time=%.3f ms\n",icmp_length, inet_ntoa(source_addr.sin_addr),icmp_packet->icmp_seq, ip_packet->ip_ttl,ping_time);
	}
	
}
/*
u_int16_t get_cksum(u_int16_t * addr, int length)
{
	u_int32_t sum = 0;

	while (length > 1) 
	{
		sum += *addr++;
		length -= 2;
	}

	if (length == 1)
		sum += *(u_int8_t *) addr;

	sum = (sum » 16) + (sum & 0xFFFF); 
	sum += (sum » 16); 

	u_int16_t result = ~sum;
	return result;
}*/

void ping(void)
{
	struct icmp *icmp_packet = (struct icmp *) send_buffer;

	icmp_packet->icmp_type = ICMP_ECHO;
	icmp_packet->icmp_code = 0;
	icmp_packet->icmp_id = pid;
	icmp_packet->icmp_seq = nsent++;
	
	if (message_size >= sizeof(struct timeval))
		gettimeofday((struct timeval *) icmp_packet->icmp_data, NULL);

		// ICMP size is 8 bytes header plus size of data
	int icmp_length = 8 + message_size;

	// Calculate checksum
	icmp_packet->icmp_cksum = 0;
	icmp_packet->icmp_cksum = in_cksum((u_int16_t *) icmp_packet, icmp_length);

	if (sendto(sock, send_buffer, icmp_length, 0,(struct sockaddr *) &dest_addr, sizeof(dest_addr)) < 0)
	{
		perror("sendto() failed");
		exit(-1);
		
	}
}

int main(int argc, char *argv[])
{

	struct input_info info = parse_input(argc,argv);
	message_size = info.size;
	printf("Starting ping %s, with %d ttl and %d data, %d times \n",inet_ntoa(* info.addr),info.ttl,info.size,info.count);
	pid = getpid();
	
	sock = socket(PF_INET, SOCK_RAW, IPPROTO_ICMP);
	if(sock <0)
	{
		printf("Can`t create socket\n");
		exit(-1);
	}
	setuid(getuid());
	if(info.broadcast == 1)
		setsockopt(sock,SOL_SOCKET, SO_BROADCAST,&on,sizeof(on));
	setsockopt(sock,SOL_SOCKET,SO_RCVBUF,&default_buffer_size,sizeof(default_buffer_size));
	
	struct itimerval timer;
	
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec =1;
	timer.it_interval.tv_sec =1;
	timer.it_interval.tv_usec =0;
	
	settimer(ITIMER_REAL, &timer, NULL);
	
	bzero(&dest_addr,sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr = *	info.addr;
	
	
	int recv_size,i;
	char recv_buffer[default_buffer_size];
	
	
	for(i=0;i<info.count;i++)
	{
		recv_size = recvfrom(sock, recv_buffer,sizeof(recv_buffer),0,(struct sockaddr *)&source_addr,sizeof(source_addr));
		if(recv_size <0)
			printf("Recv failed");
		
		struct timeval time_value; 
		gettimeofday(&time_value,NULL);
		
		parse_output(recv_buffer,recv_size,&time_value);
	}
	
	
	return 0;
}





