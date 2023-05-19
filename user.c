#include "notapp.h"

#define BUF_SIZE 8192

jmp_buf env;
//handler function
void u_handler(int sig){
    siglongjmp(env, 1);
}

//comparison function for two events
int e_compare(const void *v1, const void *v2){
	struct u_event *e1 = (struct u_event *)v1;
	struct u_event *e2 = (struct u_event *)v2;

	if(e1->sec == e2->sec){
		if(e2->usec > e1->usec){
			return 1;
		}else{
			return -1;
		}
	}else{
		if(e2->sec > e1->sec){
			return 1;
		}else{
			return -1;
		}
	}
}

int user(char *saddr, char *port) {

	 //set up signal handling
	struct sigaction sa;
	sa.sa_flags = 0; //no flag
	sigemptyset(&sa.sa_mask); // clear mask
	sa.sa_handler = u_handler;
	sigaction(SIGINT, &sa, NULL); //set handler
	sigaction(SIGTERM, &sa, NULL);

	int sport;
	if(port == NULL){
		sport = 0;
	}else{
		sport = atoi(port);
	}

	int sock = 0;
	struct sockaddr_in serv_addr;
	char *usr = "user";
	char buffer[BUF_SIZE] = {0};

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket failed");
		return -1;
	}

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(sport);

	// Convert IPv4 and IPv6 addresses from text to binary form
	if(inet_pton(AF_INET, saddr, &serv_addr.sin_addr)<=0) {
		printf("\nInvalid address/ Address not supported \n");
		return -1;
	}

	if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("connect");
		return -1;
	}
	
	//var for sending message to server
	unsigned int message = 1;

	//send handshake
	unsigned int handshake = 0;
	unsigned char hbuffer[4];
	serialize_ulong(hbuffer, handshake);
	send(sock, hbuffer, 4, 0);	
	
	int result;
    result = sigsetjmp(env, 1);
	if(result == 0){
		while(1){
			read(sock, buffer, BUF_SIZE);	
			
			unsigned int num_structs;
			unsigned char *ptr;
			ptr = deserialize_ulong(buffer, &num_structs);
			struct u_event e[num_structs];
			int i;
			//make array of structs
			for(i = 0; i < num_structs; i++){
				ptr = deserialize_event(ptr, &e[i]);
			}
			
			//sort structs
			qsort(e, num_structs, sizeof(struct u_event), e_compare);
			printf("\e[1;1H\e[2J");//refresh the terminal window from: https://stackoverflow.com/a/7660837
		
	
			printf("TIME             HOST                MONITORED                EVENT\n");
			for(i = 0; i < num_structs; i++){
				printf("%11u.%-6u %-15s %-15s %s\n", e[i].sec, e[i].usec, e[i].host, e[i].fod, e[i].event);
			}
			
			//send acknowledgement to server
        	serialize_ulong(buffer, message);
        	send(sock, buffer, 4, 0);
			
		}
	}else if(result == 1){
		//send termination message to server
		unsigned int goodbye = 0;
        serialize_ulong(buffer, goodbye);
        send(sock, buffer, 4, 0);
	}
	return 0;
}
