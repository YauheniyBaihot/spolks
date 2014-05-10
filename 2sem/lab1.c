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


//////////////////////////////////////
//variables

int sent_count = 0;
int succsess_count =0;
double sum_time = 0;
double min_time = 0.0;
double max_time = 0.0;
pid_t pid;
int message_size;
int sock;
int times = 0;
struct sockaddr_in dest_addr;
struct sockaddr_in source_addr;
char send_buffer[60*1024];
//////////////////////////////////////

//////////////////////////////////////
//defaults
int default_ttl = 255;
int default_size = 56;
int default_count = 100;
int default_broadcast = 0;


int default_buffer_size = 60 * 1024;
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
				printf("Wrong parameter -c: %d , should be between 1 and 65555\n",temp);
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
				printf("Wrong parameter -c: %d , should be between 0 and 255\n",temp);
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
				printf("Wrong parameter -c: %d , should be between 1 and 65515\n",temp);
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
	times = info.count;  
	return info;
}

void parse_output(char *buffer, int length, struct timeval *time_recv)
{
	
	int ip_length;
	int icmp_length =0;
	struct ip *ip_packet;
	struct icmp *icmp_packet;
	struct timeval *time_send;
	double ping_time;
	
	ip_packet = (struct ip *) buffer;
	ip_length = ip_packet->ip_hl * 4;
	icmp_packet = (struct icmp *) (buffer + ip_length);
	icmp_length = length - ip_length;
	
	if(icmp_packet->icmp_type == ICMP_ECHOREPLY)
	{
		
		if(icmp_packet->icmp_id != (int)pid)
		{
			return;
		}
		if(icmp_length < 8)
		{
			printf("icmp_length %d < 8\n",icmp_length);
		}
		
		time_send = (struct timeval *) icmp_packet-> icmp_data;

		tv_sub(time_recv,time_send);
		
		ping_time = time_recv->tv_sec * 1000.0 + time_recv->tv_usec / 1000.0;
		
		succsess_count ++;
		
		sum_time += ping_time;
		if(min_time < 0.000001)
			min_time = ping_time;
		if(max_time == 0)
			max_time = ping_time;
		if(ping_time < min_time)
			min_time = ping_time;
		if(ping_time > max_time)
			max_time = ping_time;
		printf("%d bytes from %s: icmp_req= %u, ttl = %d, time=%.3f ms\n",icmp_length, inet_ntoa(source_addr.sin_addr),ntohs(icmp_packet->icmp_seq), ip_packet->ip_ttl,ping_time);	
	}
	
}

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

	sum = (sum >> 16) + (sum & 0xFFFF); 
	sum += (sum >> 16); 

	u_int16_t result = ~sum;
	return result;
}

void tv_sub(struct timeval *out, struct timeval *in)
{
    if ((out->tv_usec -= in->tv_usec) < 0) {
        out->tv_sec--;
        out->tv_usec += 1000000;
    }
    out->tv_sec -= in->tv_sec;
}

void ping(void)
{
	if(times == sent_count)
		return;
	struct icmp *icmp_packet = (struct icmp *) send_buffer;

	icmp_packet->icmp_type = ICMP_ECHO;
	icmp_packet->icmp_code = 0;
	icmp_packet->icmp_id = pid;
	icmp_packet->icmp_seq = htons(++sent_count);
	
	if (message_size >= sizeof(struct timeval))
		gettimeofday((struct timeval *) icmp_packet->icmp_data, NULL);

		// ICMP size is 8 bytes header plus size of data
	int icmp_length = 8 + message_size;

	// Calculate checksum
	icmp_packet->icmp_cksum = 0;
	icmp_packet->icmp_cksum = get_cksum((u_int16_t *) icmp_packet, icmp_length);
	
    int bytes_send = sendto(sock, send_buffer, icmp_length, 0,(struct sockaddr *) &dest_addr, sizeof(dest_addr));
	if (bytes_send < 0)
	{
		perror("sendto() failed");
		exit(-1);
	}
}

void show_output_info(int signum)
{
    if (signum == SIGALRM) {
        ping();

        return;

    } else if (signum == SIGINT) {
        printf("\n--- %s ping summary---\n",
               inet_ntoa(dest_addr.sin_addr));
        printf("%d packets transmitted, ", sent_count);
        printf("%d packets received, ", succsess_count);
        if (sent_count) {
            if (succsess_count > sent_count)
                printf("-- somebody's printing up packets!");
            else
                printf("%d%% packet loss",
                       (int) (((sent_count - succsess_count) * 100) / sent_count));
        }
        printf("\n");
        if (succsess_count && message_size >= sizeof(struct timeval))
            printf("round-trip min/avg/max = %.3f/%.3f/%.3f ms\n",
                   min_time, sum_time / succsess_count, max_time);
        fflush(stdout);
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
	
	struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = &show_output_info;
    sigaction(SIGALRM, &action, NULL);
    sigaction(SIGINT, &action, NULL);

	struct itimerval timer;
	
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 1;
	timer.it_interval.tv_sec = 1;
	timer.it_interval.tv_usec = 0;

	setitimer(ITIMER_REAL, &timer, NULL);

	bzero(&dest_addr,sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_addr = *((struct in_addr *) info.addr);;
	
	int recv_size,i;
	char recv_buffer[default_buffer_size];
	
	
	socklen_t fromlen = sizeof(source_addr);
	bzero(recv_buffer,sizeof(recv_buffer));
	while(1)
	{
		recv_size = recvfrom(sock, recv_buffer,sizeof(recv_buffer),0,(struct sockaddr *) &source_addr,&fromlen);
		if(recv_size <0)
		{
			if (errno == EINTR)
            	continue;
			printf("Recv failed %d\n",i);
			continue;
		}
		struct timeval time_value; 
		gettimeofday(&time_value,NULL);
		parse_output(recv_buffer,recv_size,&time_value);
		if(times == sent_count)
		{
			show_output_info(SIGINT);
			return 0;

		}	
	}
	
	
	return 0;
}





