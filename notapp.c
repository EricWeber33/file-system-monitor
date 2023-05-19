#include "notapp.h"

int main(int argc, char *argv[]){
	int c;

	//potential command line args
	int type = -1;
	char *port = NULL;
	char *interval = NULL;
	char *logf = NULL;
	char *saddr = NULL;
	char *fod;

	while((c = getopt(argc, argv, "so:u:t:p:l:")) != -1){
		switch(c){
			case 's':
				if(type != -1){
					printf("ERROR: flags for multiple roles\n");
					return 1;
				}else{
					type = 0;
				}
				break;
			case 'o':
				if(type != -1){
					printf("ERROR: flags for multiple roles\n");
					return 1;
				}else{
					type = 1;
					saddr = optarg;
				}
				break;
			case 'u':
				if(type != -1){
					printf("ERROR: flags for multiple roles\n");
					return 1;
				}else{
					type = 2;
					saddr = optarg;
				}
				break;
			case 't':
				interval = optarg;
				break;
			case 'p':
				port = optarg;
				break;
			case 'l':
				logf = optarg;
				break;
			case '?':
				break;		
		}
	}
	

	if(type==-1){
		printf("ERROR: no role specified\n");
		return 1;
	}else if (type==0){
		double interval_d = strtod(interval, NULL);
		if(interval_d < 0.1 || interval_d > 10){
			printf("ERROR: invalid interval value\n");
			return 1;
		}
		//make instance of server
		server(interval_d, port, logf);
	}else if (type==1){
		//set up observer client
		if(argc < 5){
			printf("ERROR: not enough args supplied for observer\n");
			return 1;
		}
		port = argv[3];
		fod = argv[4];
		observer(saddr, port, fod);
	}else if (type==2){
		//set up user client
		if(argc < 4){
			printf("ERROR: not enough args supplied for user\n");
			return 1;
		}
		port = argv[3];
		user(saddr, port);
	}

	return 0;
}


