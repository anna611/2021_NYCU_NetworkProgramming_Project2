#include "np_single_proc.h"

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

int main(int argc, char* argv[]){
    struct sockaddr_in fsin;
    if (argc != 2)
    {
        cerr << "./np_multi_proc [port]" << endl;
        exit(-1);
    }
    int port = atoi(argv[1]);
    msock = CreateServTCP(port);
    int nfds = getdtablesize();
    FD_ZERO(&afds);
    FD_SET(msock,&afds);
    //initial used id
    for(int i = 0;i < MAX_CLIENT_SIZE;++i)
        record_id[i] = 0;
    while(1){
        memcpy(&rfds,&afds,sizeof(rfds));
        int ssock;
        int select_flag = select(nfds,&rfds,(fd_set *)0,(fd_set *)0,(struct timeval *)0);
        while(select_flag < 0 && errno == EINTR){
            select_flag = select(nfds,&rfds,(fd_set *)0,(fd_set *)0,(struct timeval *)0);
        }
           // perror("select error");
        if(FD_ISSET(msock,&rfds)){
            socklen_t alen = sizeof(fsin);
            ssock = accept(msock,(struct sockaddr *)&fsin,&alen);
            if(ssock < 0)
                perror("accept error");
            else{
                client_info new_cli;
                for(int i = 0;i < MAX_CLIENT_SIZE;++i){
                    if(record_id[i] == 0){
                        new_cli.id = i+1;
                        record_id[i] = 1;
                        break;
                    }
                }
                new_cli.fd = ssock;
                new_cli.name = "(no name)";
                strncpy(new_cli.ip, inet_ntoa(fsin.sin_addr), INET_ADDRSTRLEN);
                new_cli.port = ntohs(fsin.sin_port);    
                new_cli.finish = 0;
               // new_cli.numpipe.clear(); 
                client_record.push_back(new_cli);
                FD_SET(ssock,&afds);
                welcome(new_cli.fd);
                login(new_cli);
                send(new_cli.fd, "% ", 2, 0);  
            }
        }
        for(int fd = 0;fd < nfds; ++fd){
            if(fd != msock && FD_ISSET(fd,&rfds)){
                NpShell npshell;
                char buf[BUFSIZE];
                memset(buf, '\0', sizeof(buf));
                int nRecv = recv(fd,buf,sizeof(buf),0);
                if( nRecv <= 0 ){
                    if (nRecv == 0)
                        cout << "socket: " << fd << "closed" << endl;
                    else
                        perror("receive error");
                }
                else{
                    string cli_msg(buf);
                    size_t pos = 0;
                    string token = "\r";
                    string token2 = "\n";
                    client_info cur_client;
                    int status = 0;
                    for(int i = 0;i < client_record.size();++i){
                        if(fd == client_record[i].fd)
                            cur_client = client_record[i];
                    }
                    if((pos = cli_msg.find(token)) != string::npos)
                        cli_msg.erase(pos,1);
                    if((pos = cli_msg.find(token2)) != string::npos)
                        cli_msg.erase(pos,1);
                    status = npshell.exec(cli_msg,cur_client);
                    if(status == -1){
                        close(fd);
                        close(1);
                        close(2);
                        dup2(0, 1);
                        dup2(0, 2);
                        FD_CLR(fd,&afds);
                        logout(cur_client);
                        record_id[cur_client.id-1] = 0;
                        for(int i = 0;i < client_record.size();++i){
                            if(fd == client_record[i].fd)
                                client_record[i].finish = 1;
                        }
                    }
                }
                
            }
        }

    }
    return 0;
}