#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <limits.h>
#include <signal.h>
#include <setjmp.h>
#include <sys/inotify.h>
#include <sys/time.h>


//struct definitions
//obs_event holds information relating to the last event read by
// an observer
struct obs_event{
	int connected;
	unsigned int sec;
	unsigned int usec;
	char fod[256];
	char host[64];
	char event[256];
};

//u_event is a recieved event in the user client
struct u_event{
    unsigned int sec;
	unsigned int usec;
	char fod[256];
	char host[64];
	char event[256];
};

//functions for serialization
unsigned char* serialize_ulong(unsigned char* buffer, unsigned int value);
unsigned char* deserialize_ulong(unsigned char* buffer, unsigned int *result);
unsigned char* serialize_chr_stream(unsigned char* buffer, char *str, int str_len);
unsigned char* deserialize_chr_stream(unsigned char* buffer, char *str, int str_len);
unsigned char* serialize_event(unsigned char* buffer, struct obs_event e);
unsigned char* deserialize_event(unsigned char* buffer, struct u_event *e);

//comparison function
int e_compare(const void *v1, const void *v2);

//functions for threads
void *clientThread(void *arg);

//functions for types
int server(double interval, char *port, char *logfile);
int observer(char *saddr, char *port, char *fileordir);
int user(char *saddr, char *port);
