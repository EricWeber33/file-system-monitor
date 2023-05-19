#include "notapp.h"
#define BUF_SIZE 8192


struct event_content{
    unsigned int sec;
    unsigned int usec;
    char event_str[256];
};

jmp_buf env;
//handler function
void o_handler(int sig){
    siglongjmp(env, 1);
}

int observer(char *saddr, char *port, char *fileordir) {

    
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
    
    //set up signal handling
	struct sigaction sa;
	sa.sa_flags = 0; //no flag
	sigemptyset(&sa.sa_mask); // clear mask
	sa.sa_handler = o_handler;
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
    
    //set up handshake
    int hbuffer_len = 8 + strlen(saddr) + strlen(fileordir);
	unsigned char hbuffer[hbuffer_len], *bptr;
    bptr = serialize_ulong(hbuffer, strlen(fileordir));
    bptr = serialize_ulong(bptr, strlen(saddr));
    bptr = serialize_chr_stream(bptr, fileordir, strlen(fileordir));
    bptr = serialize_chr_stream(bptr, saddr, strlen(saddr));

    //send handshake
	send(sock, hbuffer, hbuffer_len, 0);	

	//set up inotify
	// create a new inotify file descriptor
    // it has an associated watch list
    // read from it to get events
    int inotify_fd = inotify_init();
    if(inotify_fd < 0)
        return 1; // can't create the inotify fd, return 1 to os and exit

    // add a new watch to inotify_fd, monitor the current folder for all events
    // returns a watch descriptor. 
    int watch_des = inotify_add_watch(inotify_fd, fileordir, IN_ALL_EVENTS);
    if(watch_des == -1)
        return 1; // can't create the watch descriptor, return 1 to os and exit

    // create a buffer for at most 100 events
    #define EVENT_STRUCT_SIZE sizeof(struct inotify_event) 
    #define BUFFER_SIZE (100 * (EVENT_STRUCT_SIZE + NAME_MAX + 1))
    char ebuffer[BUFFER_SIZE];

    int result;
    result = sigsetjmp(env, 1);
    if(result == 0){
        // start to monitor
        while(1 == 1) {
            // read 
            int bytesRead = read(inotify_fd, ebuffer, BUFFER_SIZE), bytesProcessed = 0;
            if(bytesRead < 0) // read error
                return 1;
        
            while(bytesProcessed < bytesRead) {
                struct inotify_event* event = (struct inotify_event*)(ebuffer + bytesProcessed);
                struct timeval current_time;

                struct event_content new_event;
                char *event_desc;

                gettimeofday(&current_time, NULL);
                new_event.sec = current_time.tv_sec;
                new_event.usec = current_time.tv_usec;

                //get the event
                if (event->mask & IN_CREATE){
                    strncpy(new_event.event_str, "\0", 256);
                    if (event->mask & IN_ISDIR){
                        sprintf(new_event.event_str,"%s IN_CREATE IN_ISDIR", event->name);
                    }else if(event->mask & IN_IGNORED){
                        sprintf(new_event.event_str,"%s IN_CREATE IN_IGNORED", event->name);
                    }else if(event->mask & IN_Q_OVERFLOW){
                        sprintf(new_event.event_str,"%s IN_CREATE IN_Q_OVERFLOW", event->name);
                    }else if(event-> mask & IN_UNMOUNT){
                        sprintf(new_event.event_str,"%s IN_CREATE IN_UNMOUNT", event->name);
                    }else{
                        sprintf(new_event.event_str,"%s IN_CREATE", event->name);
                    }
                }else if ((event->mask & IN_DELETE)){
                    if (event->mask & IN_ISDIR){
                        sprintf(new_event.event_str,"%s IN_DELETE IN_ISDIR", event->name);
                    }else if(event->mask & IN_IGNORED){
                        sprintf(new_event.event_str,"%s IN_DELETE IN_IGNORED", event->name);
                    }else if(event->mask & IN_Q_OVERFLOW){
                        sprintf(new_event.event_str,"%s IN_DELETE IN_Q_OVERFLOW", event->name);
                    }else if(event-> mask & IN_UNMOUNT){
                        sprintf(new_event.event_str,"%s IN_DELETE IN_UNMOUNT", event->name);
                    }else{
                        sprintf(new_event.event_str,"%s IN_DELETE", event->name);
                    }
                }else if ((event->mask & IN_ACCESS)){
                    if (event->mask & IN_ISDIR){
                        sprintf(new_event.event_str,"%s IN_ACCESS IN_ISDIR", event->name);
                    }else if(event->mask & IN_IGNORED){
                        sprintf(new_event.event_str,"%s IN_ACCESS IN_IGNORED", event->name);
                    }else if(event->mask & IN_Q_OVERFLOW){
                        sprintf(new_event.event_str,"%s IN_ACCESS IN_Q_OVERFLOW", event->name);
                    }else if(event-> mask & IN_UNMOUNT){
                        sprintf(new_event.event_str,"%s IN_ACCESS IN_UNMOUNT", event->name);
                    }else{
                        sprintf(new_event.event_str,"%s IN_ACCESS", event->name);
                    }
                }else if ((event->mask & IN_ATTRIB)){
                    if (event->mask & IN_ISDIR){
                        sprintf(new_event.event_str,"%s IN_ATTRIB IN_ISDIR", event->name);
                    }else if(event->mask & IN_IGNORED){
                        sprintf(new_event.event_str,"%s IN_ATTRIB IN_IGNORED", event->name);
                    }else if(event->mask & IN_Q_OVERFLOW){
                        sprintf(new_event.event_str,"%s IN_ATTRIB IN_Q_OVERFLOW", event->name);
                    }else if(event-> mask & IN_UNMOUNT){
                        sprintf(new_event.event_str,"%s IN_ATTRIB IN_UNMOUNT", event->name);
                    }else{
                        sprintf(new_event.event_str,"%s IN_ATTRIB", event->name);
                    }
                }else if ((event->mask & IN_CLOSE_WRITE)){
                    if (event->mask & IN_ISDIR){
                        sprintf(new_event.event_str,"%s IN_CLOSE_WRITE IN_ISDIR", event->name);
                    }else if(event->mask & IN_IGNORED){
                        sprintf(new_event.event_str,"%s IN_CLOSE_WRITE IN_IGNORED", event->name);
                    }else if(event->mask & IN_Q_OVERFLOW){
                        sprintf(new_event.event_str,"%s IN_CLOSE_WRITE IN_Q_OVERFLOW", event->name);
                    }else if(event-> mask & IN_UNMOUNT){
                        sprintf(new_event.event_str,"%s IN_CLOSE_WRITE IN_UNMOUNT", event->name);
                    }else{
                        sprintf(new_event.event_str,"%s IN_CLOSE_WRITE", event->name);
                    }
                }else if ((event->mask & IN_CLOSE_NOWRITE)){
                    if (event->mask & IN_ISDIR){
                        sprintf(new_event.event_str,"%s IN_CLOSE_NOWRITE IN_ISDIR", event->name);
                    }else if(event->mask & IN_IGNORED){
                        sprintf(new_event.event_str,"%s IN_CLOSE_NOWRITE IN_IGNORED", event->name);
                    }else if(event->mask & IN_Q_OVERFLOW){
                        sprintf(new_event.event_str,"%s IN_CLOSE_NOWRITE IN_Q_OVERFLOW", event->name);
                    }else if(event-> mask & IN_UNMOUNT){
                        sprintf(new_event.event_str,"%s IN_CLOSE_NOWRITE IN_UNMOUNT", event->name);
                    }else{
                        sprintf(new_event.event_str,"%s IN_CLOSE_NOWRITE", event->name);
                    }
                }else if ((event->mask & IN_DELETE_SELF)){
                    //end program
                    raise(SIGINT);
                }else if ((event->mask & IN_MODIFY)){
                    if (event->mask & IN_ISDIR){
                        sprintf(new_event.event_str,"%s IN_MODIFY IN_ISDIR", event->name);
                    }else if(event->mask & IN_IGNORED){
                        sprintf(new_event.event_str,"%s IN_MODIFY IN_IGNORED", event->name);
                    }else if(event->mask & IN_Q_OVERFLOW){
                        sprintf(new_event.event_str,"%s IN_MODIFY IN_Q_OVERFLOW", event->name);
                    }else if(event-> mask & IN_UNMOUNT){
                        sprintf(new_event.event_str,"%s IN_MODIFY IN_IN_UNMOUNT", event->name);
                    }else{
                        sprintf(new_event.event_str,"%s IN_MODIFY", event->name);
                    }
                }else if ((event->mask & IN_MOVE_SELF)){
                    if (event->mask & IN_ISDIR){
                        sprintf(new_event.event_str,"%s IN_MOVE_SELF IN_ISDIR", event->name);
                    }else if(event->mask & IN_IGNORED){
                        sprintf(new_event.event_str,"%s IN_MOVE_SELF IN_IGNORED", event->name);
                    }else if(event->mask & IN_Q_OVERFLOW){
                        sprintf(new_event.event_str,"%s IN_MOVE_SELF IN_Q_OVERFLOW", event->name);
                    }else if(event-> mask & IN_UNMOUNT){
                        sprintf(new_event.event_str,"%s IN_MOVE_SELF IN_IN_UNMOUNT", event->name);
                    }else{
                        sprintf(new_event.event_str,"%s IN_MOVE_SELF", event->name);
                    }
                }else if ((event->mask & IN_MOVED_FROM)){
                    if (event->mask & IN_ISDIR){
                        sprintf(new_event.event_str,"%s IN_MOVED_FROM IN_ISDIR", event->name);
                    }else if(event->mask & IN_IGNORED){
                        sprintf(new_event.event_str,"%s IN_MOVED_FROM IN_IGNORED", event->name);
                    }else if(event->mask & IN_Q_OVERFLOW){
                        sprintf(new_event.event_str,"%s IN_MOVED_FROM IN_Q_OVERFLOW", event->name);
                    }else if(event-> mask & IN_UNMOUNT){
                        sprintf(new_event.event_str,"%s IN_MOVED_FROM IN_IN_UNMOUNT", event->name);
                    }else{
                        sprintf(new_event.event_str,"%s IN_MOVED_FROM", event->name);
                    }
                }else if ((event->mask & IN_OPEN)){
                    if (event->mask & IN_ISDIR){
                        sprintf(new_event.event_str,"%s IN_OPEN IN_ISDIR", event->name);
                    }else if(event->mask & IN_IGNORED){
                        sprintf(new_event.event_str,"%s IN_OPEN IN_IGNORED", event->name);
                    }else if(event->mask & IN_Q_OVERFLOW){
                        sprintf(new_event.event_str,"%s IN_OPEN IN_Q_OVERFLOW", event->name);
                    }else if(event-> mask & IN_UNMOUNT){
                        sprintf(new_event.event_str,"%s IN_OPEN IN_IN_UNMOUNT", event->name);
                    }else{
                        sprintf(new_event.event_str,"%s IN_OPEN", event->name);
                    }
                }       

                //if name was start of header replace it
                if(new_event.event_str[0]== 0x01){
                    new_event.event_str[0] = ' ';
                }

                //make length for buffer
                int sbuffer_len = sizeof(unsigned int)*3 + strlen(new_event.event_str);
                //buffer for event
                unsigned char sbuffer[sbuffer_len];
                //serialize the event
                bptr = serialize_ulong(sbuffer, sbuffer_len);
                bptr = serialize_ulong(bptr, new_event.sec);
                bptr = serialize_ulong(bptr, new_event.usec);
                bptr = serialize_chr_stream(bptr, new_event.event_str, strlen(new_event.event_str));
                //send sbuffer to server
                send(sock, sbuffer, sbuffer_len, 0);
                bytesProcessed += EVENT_STRUCT_SIZE + event->len;
            }
        }
    }else if(result == 1){
        //notfify server that this observer is being terminated
        unsigned int goodbye = 0;
        unsigned char ebufffer[4];
        serialize_ulong(ebuffer, goodbye);
        send(sock, ebuffer, 4, 0);
    }
    
	inotify_rm_watch(inotify_fd, watch_des);
    close(inotify_fd); // close the fd, will remove all remaining watches 
	return 0;
}
