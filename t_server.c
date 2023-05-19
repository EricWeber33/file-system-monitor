#include "notapp.h"

#define BUF_SIZE 1024

struct clientInfo{
	int socket;
	char *ip;
};
//struct to pass multiple args to pthtrea
//shared data
FILE *fptr; //file pointer for potential log file

int observerCount = 0;
int totalUsers =0;
int userCount = 0;
int usersWaiting = 0;
int ready = 0;

//an array that supports up to 512 observers being connected to the server at a time
struct obs_event obs_event_arr[512];
pthread_mutex_t lock;	//lock for the shared data
pthread_cond_t cond;	//conditional statement;

//thread for sending signals

void *broadcastThread(void * arg){
	double interval = *((double*)arg);
	while(1){
		sleep(interval);
		pthread_cond_broadcast(&cond);
	}
}

void *clientThread(void *arg){
	struct clientInfo ci = *((struct clientInfo*)arg);
	int socket = ci.socket;
	char* cip = ci.ip;
	
	//int socket = *((int*)arg);
	char buffer[BUF_SIZE] = {0};
	
	unsigned char * bptr;

	read(socket, buffer, BUF_SIZE);
	//determine handshake value
	unsigned int inc_buf_size;

	bptr = deserialize_ulong(buffer, &inc_buf_size);
	//determine the type of client
	//refer to protocal in readme
	if(inc_buf_size>0){
		//handle observer client
		//parse rest of observer handshake
		int id;
		unsigned int obs_saddr_len;
		bptr = deserialize_ulong(bptr, &obs_saddr_len);
		char saddr[obs_saddr_len+1];
		char fod[inc_buf_size+1];
		bptr = deserialize_chr_stream(bptr, fod, inc_buf_size);
		bptr = deserialize_chr_stream(bptr, saddr, obs_saddr_len);

		//create struct for this observer
		pthread_mutex_lock(&lock);
		//get id
		id = observerCount;
		observerCount++;
		obs_event_arr[id].connected = 0;
		strncpy(obs_event_arr[id].fod, fod, 256);
		strncpy(obs_event_arr[id].host, cip, 64);

		if(fptr!=NULL){
			fprintf(fptr, "obs: %d %s, %s added\n", id, obs_event_arr[id].fod, obs_event_arr[id].host);
			fflush(fptr);
		}

		pthread_mutex_unlock(&lock);

		while(1){
			//read incoming mesasges from the observer
			int bytes_read = read(socket, buffer, BUF_SIZE), bytes_processed = 0;
			//deserialize
			while(bytes_processed < bytes_read){
				bptr = deserialize_ulong(buffer + bytes_processed, &inc_buf_size);
				//if goodbye message sent end thread
				if(inc_buf_size == 0){
					pthread_mutex_lock(&lock);
					obs_event_arr[id].connected = 0; //set connected val of array to be zero
					//update log if specified
					if(fptr!=NULL){
						fprintf(fptr, "observer %d disconnected\n", id);
						fflush(fptr);
					}	
					pthread_mutex_unlock(&lock);
					pthread_exit(NULL);
				}	
				int e_str_len = inc_buf_size - sizeof(unsigned int)*3;
				unsigned int sec, usec;
				char e_str[e_str_len+1];
				//deserialize buffer
				bptr = deserialize_ulong(bptr, &sec);
				bptr = deserialize_ulong(bptr, &usec);
				bptr = deserialize_chr_stream(bptr, e_str, e_str_len);
                
                //use server time instead
                struct timeval current_time;
                gettimeofday(&current_time, NULL);
                sec = current_time.tv_sec;
                usec = current_time.tv_usec;
				//acquire lock to update structure
				pthread_mutex_lock(&lock);
				//update structure values
				obs_event_arr[id].connected = 1;
				obs_event_arr[id].sec = sec;
				obs_event_arr[id].usec = usec;
				strncpy(obs_event_arr[id].event, e_str, 256);

				//update log if specified
				if(fptr!=NULL){
					fprintf(fptr, "event recieved from observer %d:\n", id);
					fprintf(fptr, "%u.%u	%s	%s  %s\n", obs_event_arr[id].sec,  obs_event_arr[id].usec, obs_event_arr[id].host, obs_event_arr[id].fod, obs_event_arr[id].event);
					fflush(fptr);
				}	
				pthread_mutex_unlock(&lock);
				bytes_processed += inc_buf_size;
			}
		}
		
	}else if(inc_buf_size==0){

		pthread_mutex_lock(&lock);
		int uid = totalUsers;
		totalUsers++;
		userCount++;
		if(fptr!=NULL){
			fprintf(fptr, "user %d added\n", uid);
			fflush(fptr);
		}
		pthread_mutex_unlock(&lock);

		//handle user client
		while(1){
			

			pthread_mutex_lock(&lock);
			pthread_cond_wait(&cond, &lock);
			
			int i;
			//count number of events
			unsigned int num_events = 0;
			for(i = 0; i < observerCount; i++){
				if(obs_event_arr[i].connected == 1){
					num_events++;
				}
			}

			unsigned char sbuffer[num_events*sizeof(struct obs_event)], *sptr;

			//serialize number of events
			sptr = serialize_ulong(sbuffer, num_events);
			for(i = 0; i < observerCount; i++){
				if(obs_event_arr[i].connected == 1){
					//serialize the event
					sptr = serialize_event(sptr, obs_event_arr[i]);
				}
			}
			send(socket, sbuffer, (long)sptr-(long)sbuffer, 0);

			if(fptr!=NULL){
				fprintf(fptr, "sent data to user %d \n", uid);
				fflush(fptr);
			}
			usersWaiting++;
			pthread_mutex_unlock(&lock);

			read(socket, buffer, BUF_SIZE);
			unsigned int u_msg;
			deserialize_ulong(buffer, &u_msg);
			if(u_msg == 0){
				pthread_mutex_lock(&lock);
				userCount--;
				usersWaiting--;
				//update log if specified
				if(fptr!=NULL){
					fprintf(fptr, "user %d disconnected\n", uid);
					fflush(fptr);
				}	
				pthread_mutex_unlock(&lock);
				pthread_exit(NULL);
			}
		}
	}
}

int server(double interval, char* port, char* logfile){
	//determine port
	int sport;
	if(port == NULL){
		sport = 0;
	}else{
		sport = atoi(port);
	}

	int server_fd, newSocket;
	struct sockaddr_in address;
	int opt = 1;
	int addrlen = sizeof(address);
	
	//set up space for 1024 thread ids
	pthread_t thread_id[1024];
	int i = 0;

	if((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
		perror("socket failed");
		exit(EXIT_FAILURE);
	}
	
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(sport);
	bind(server_fd, (struct sockaddr*)&address, addrlen);		
	

	if(listen(server_fd, 100) < 0){
		perror("listen");
		exit(EXIT_FAILURE);
	}

	if(getsockname(server_fd, (struct sockaddr*)&address, &addrlen) == -1){
		perror("getsockname error");
	}else if(sport == 0){
		printf("PORT NUM: %d\n", ntohs(address.sin_port));
	}

	
	//daemonize process
	pid_t pid;
	pid = fork();
	if(pid < 0){
		exit(EXIT_FAILURE);
	}
	if(pid > 0){
		exit(EXIT_SUCCESS);
	}
	if(setsid() < 0){
		exit(EXIT_FAILURE);
	}
	pid = fork();
	if(pid < 0){
		exit(EXIT_FAILURE);
	}
	if(pid > 0){
		exit(EXIT_SUCCESS);
	}

	//ignore SIGPIPE signals
	signal(SIGPIPE, SIG_IGN);

	//set up optional log file
	fptr = fopen(logfile, "w");
	//set up mutex lock
	if(pthread_mutex_init(&lock, NULL) != 0){
		printf("mutex init failure");
		return -1;
	}

	//set up broadcast threadT
	pthread_create(&thread_id[i++], NULL, broadcastThread, &interval);

	//wait for new connections
	while(1){
		if((newSocket = accept(server_fd, (struct sockaddr *)&address, 
					(socklen_t*)&addrlen))<0){
			perror("accept");
			exit(EXIT_FAILURE);		
		}
		struct clientInfo newClient;
		newClient.socket = newSocket;
		newClient.ip = inet_ntoa(address.sin_addr);
		
		//create thread for new client
		if( pthread_create(&thread_id[i++], NULL, clientThread, &newClient) != 0){
			printf("FAILED TO MAKE THREAD");
		}
	}

	if(fptr != NULL){
		fclose(fptr);
	}
	return 0;
}