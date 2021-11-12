#include "np_multi_proc.h"

using namespace std;
int CreateServTCP(int port)
{
    int LISTENQ = 0;
    int sockfd = 0;
    struct sockaddr_in sock_addr;
    // open TCP socket
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0))<0)
    {
        cerr << "server: can't open stream socket" << endl;
        return 0;
    }
        
    bzero((char *) &sock_addr, sizeof(sock_addr));
    // set TCP
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port = htons(port);

    // set socket
	int optval = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int)) == -1) 
    {
		cerr << "Error: set socket failed" << endl;
		return 0;
	}
    // bind socket
	if (bind(sockfd, (struct sockaddr*)&sock_addr, sizeof(sock_addr)) < 0)
    {
		cerr << "Error: bind socket failed" << endl;
        return 0;
	}
	listen(sockfd, LISTENQ);
	return sockfd;
}

/* initialize shared memory store client info */
void client_info_shared_memory(){	
	info_fd = shm_open("store_client_info", O_CREAT | O_RDWR, 0666);
	ftruncate(info_fd, sizeof(client_info) * MAX_CLIENT_SIZE);
	client_info *c =  (client_info *)mmap(NULL, sizeof(client_info) * MAX_CLIENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, info_fd, 0);
	for(int i = 0;i < MAX_CLIENT_SIZE;++i){
		c[i].valid = 0;
	}
	munmap(c, sizeof(client_info) * MAX_CLIENT_SIZE);
    return;
}

void broadcast_shared_memory(){
    broadcast_fd = shm_open("store_broadcast", O_CREAT | O_RDWR, 0666);
	ftruncate(broadcast_fd, 0x400000);
    return;
}

int main(int argc, char* argv[])
{
    client_info_shared_memory();
    broadcast_shared_memory();
    // server variable
    int sockfd, newsockfd, childpid;
    socklen_t clilen;
    struct sockaddr_in cli_addr;
    if (argc != 2)
    {
        cerr << "./np_multi_proc [port]" << endl;
        exit(-1);
    }
    int port = atoi(argv[1]);
    // create tcp
    sockfd = CreateServTCP(port);
    //cout << "Sockfd: " << sockfd << endl;
    setenv("PATH", "bin:.", 1);
    while (true)
    {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        int cli_id = getMinID();
        if(cli_id < 0){
            perror("Error:Client Max");
            return 0;
        }
        if (newsockfd < 0)
        {
            cerr << "Error: accept failed" << endl;
            continue;
        }
        childpid = fork();
        while (childpid < 0){
            usleep(500);
            childpid = fork();
        }
        if (childpid == 0)
        {
            //client ip
            dup2(newsockfd, STDIN_FILENO);
            dup2(newsockfd, STDOUT_FILENO);
            dup2(newsockfd, STDERR_FILENO);
            close(newsockfd);
            close(sockfd);
            /*set client info in share memory*/
			void *p = mmap(NULL, sizeof(client_info) * MAX_CLIENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, info_fd, 0);
			client_info *cf = (client_info *) p;
			char ip[INET6_ADDRSTRLEN];
            sprintf(ip, "%s:%d", inet_ntoa(cli_addr.sin_addr), ntohs(cli_addr.sin_port));
			strncpy(cf[cli_id-1].ip,ip,INET6_ADDRSTRLEN);
			strcpy(cf[cli_id-1].name,"(no name)");
			cf[cli_id-1].cpid = getpid();
			cf[cli_id-1].valid = 1;
			munmap(p, sizeof(client_info) * MAX_CLIENT_SIZE);
            /* Set signals for boradcast msg */
			signal(SIGUSR1, SignalHandler);
            /*Send welcome message*/
            welcome(newsockfd);
            login(cli_id);
            NpShell shell;
            if(shell.exec() == -1){
                close(STDIN_FILENO);
                close(STDOUT_FILENO);
                close(STDERR_FILENO);
                client_info *c =  (client_info *)mmap(NULL, sizeof(client_info) * MAX_CLIENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, info_fd, 0);
				c[cli_id-1].valid = 0;
                logout(cli_id);
                munmap(c, sizeof(client_info) * MAX_CLIENT_SIZE);
                exit(0);
            }
        }
        else
        {
            close(newsockfd);
        }
    }
    return 0;
}
