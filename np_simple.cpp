#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>

#include "npshell.h"

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


int main(int argc, char* argv[])
{
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
    cout << "Sockfd: " << sockfd << endl;
    setenv("PATH", "bin:.", 1);
    while (true)
    {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (newsockfd < 0)
        {
            cerr << "Error: accept failed" << endl;
            continue;
        }
        cout << "New sockfd: " << newsockfd << endl;
        childpid = fork();
        while (childpid < 0)
            childpid = fork();
        if (childpid == 0)
        {
            NpShell shell;
            dup2(newsockfd, STDIN_FILENO);
            dup2(newsockfd, STDOUT_FILENO);
            dup2(newsockfd, STDERR_FILENO);
            close(newsockfd);
            close(sockfd);
            shell.exec();
            exit(0);
        }
        else
        {
            close(newsockfd);
        }
    }
    return 0;
}