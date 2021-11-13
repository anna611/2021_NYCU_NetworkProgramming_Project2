#include <iostream>
#include <string>
#include<sstream>
#include <vector>
#include <cstdlib>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>

using namespace std;

#define MAX_CLIENT_SIZE 30
#define BUFSIZE 4096

typedef struct{
	int fd[2];
	int index = -1;
}pipe_data;

typedef struct{
    int cpid;
    int id;
    char name[20];
    char ip[INET6_ADDRSTRLEN];
	int valid;
}client_info;
int info_fd;
int broadcast_fd;
vector<client_info> client_record;

int getMinID(){
    client_info *c =  (client_info *)mmap(NULL, sizeof(client_info) * MAX_CLIENT_SIZE, PROT_READ, MAP_SHARED, info_fd, 0);
	for(int i = 0;i < MAX_CLIENT_SIZE;++i){
		if(!c[i].valid){
			munmap(c, sizeof(client_info) * MAX_CLIENT_SIZE);
			return i+1;
		}
	}
	munmap(c, sizeof(client_info) * MAX_CLIENT_SIZE);
	return -1;
}
void welcome(int sockfd){
    string msg = "";
    msg += "****************************************\n";
    msg += "** Welcome to the information server. **\n";
    msg += "****************************************";
    cout << msg <<endl;
    return;
}
void login(int id){
    char buf[BUFSIZE];
	memset( buf, 0, sizeof(char)*BUFSIZE );
	client_info *c =  (client_info *)mmap(NULL, sizeof(client_info) * MAX_CLIENT_SIZE, PROT_READ, MAP_SHARED, info_fd, 0);
	sprintf(buf,"*** User '(no name)' entered from %s. ***",c[id-1].ip);
	//broadcast
    char *p = static_cast<char*>(mmap(NULL, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, broadcast_fd, 0));
	string tmp(buf);
	/* end of string */
	tmp += '\0';
	strncpy(p, tmp.c_str(),tmp.length());
	munmap(p, 0x400000);
	for(int i = 0;i < MAX_CLIENT_SIZE;++i){
		/* check id valid and send signo*/
		if(c[i].valid == 1){
			kill(c[i].cpid,SIGUSR1);
		}
	}
	munmap(c, sizeof(client_info) * MAX_CLIENT_SIZE);
	return;
}
void logout(int id){
    char buf[BUFSIZE];
	memset( buf, 0, sizeof(char)*BUFSIZE );
	client_info *c =  (client_info *)mmap(NULL, sizeof(client_info) * MAX_CLIENT_SIZE, PROT_READ, MAP_SHARED, info_fd, 0);
	sprintf(buf,"*** User '%s' left. ***",c[id-1].name);
	//broadcast
    char *p = static_cast<char*>(mmap(NULL, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, broadcast_fd, 0));
	string tmp(buf);
	/* end of string */
	tmp += '\0';
	strncpy(p, tmp.c_str(),tmp.length());
	munmap(p, 0x400000);
    for(int i = 0;i < MAX_CLIENT_SIZE;++i){
		/* check id valid and send signo*/
		if(c[i].valid == 1){
			kill(c[i].cpid,SIGUSR1);
		}
	}
	munmap(c, sizeof(client_info) * MAX_CLIENT_SIZE);
    //clear userpipe when logout
	return;
}
static void SignalHandler(int signo){
     //receive msg from broadcast 
	if (signo == SIGUSR1)
    {
		char *p = static_cast<char*>(mmap(NULL, 0x400000, PROT_READ, MAP_SHARED, broadcast_fd, 0));
		string tmp(p);
		cout << tmp << endl;
		munmap(p, 0x400000);
	}
}
class NpShell{
    private:
        vector<pipe_data> record_n;
    public:
        static void handle_child(int);
        vector<string> spilt_input(const string&);
        int printenv(string&);
        void redirection(string&);
        vector<string> parse(string&);
        int operation(vector<string>);
        int exec(int);
        void who();
		void tell(int,int,string);
		void yell(int,string);
		void name(int,string);
};
void NpShell::who(){
    printf("<ID>\t<nickname>\t<IP:port>\t<indicate me>\n");
	client_info *c =  (client_info *)mmap(NULL, sizeof(client_info) * MAX_CLIENT_SIZE, PROT_READ, MAP_SHARED, info_fd, 0);
	for(int i = 0;i < MAX_CLIENT_SIZE;i++){
		if(c[i].valid == 1){
			if(c[i].cpid == getpid()){
				printf("%d\t%s\t%s\t<-me\n",i+1,c[i].name,c[i].ip);
			}
			else
			{
				printf("%d\t%s\t%s\t\n",i+1,c[i].name,c[i].ip);
			}
		}
	}
	fflush(stdout);
	munmap(c, sizeof(client_info) * MAX_CLIENT_SIZE);
    return;
}
void NpShell::tell(int id,int target,string msg){
	char buf[BUFSIZE];
	memset( buf, 0, sizeof(char)*BUFSIZE );
	client_info *c =  (client_info *)mmap(NULL, sizeof(client_info) * MAX_CLIENT_SIZE, PROT_READ, MAP_SHARED, info_fd, 0);
	//check whether target exist
	if(c[target-1].valid){
		sprintf(buf,"*** %s told you ***: %s",c[id-1].name,msg.c_str());
		kill(c[target-1].cpid,SIGUSR1);
	}
	else{
		sprintf(buf,"*** Error: user #%s does not exist yet. ***",msg.c_str());
		string tmp(buf);
		cout << tmp << endl;
		return;
	}
	char *p = static_cast<char*>(mmap(NULL, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, broadcast_fd, 0));
	string tmp(buf);
	/* end of string */
	tmp += '\0';
	strncpy(p, tmp.c_str(),tmp.length());
	munmap(p, 0x400000);
	usleep(50);
	munmap(c, sizeof(client_info) * MAX_CLIENT_SIZE);
	return;
}
void NpShell::yell(int id,string s){
	char buf[BUFSIZE];
	memset( buf, 0, sizeof(char)*BUFSIZE );
	client_info *c =  (client_info *)mmap(NULL, sizeof(client_info) * MAX_CLIENT_SIZE, PROT_READ, MAP_SHARED, info_fd, 0);
	s += '\0';
	sprintf(buf,"*** %s yelled ***: %s",c[id-1].name,s.c_str());
	//broadcast
    char *p = static_cast<char*>(mmap(NULL, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, broadcast_fd, 0));
	string tmp(buf);
	/* end of string */
	tmp += '\0';
	strncpy(p, tmp.c_str(),tmp.length());
	munmap(p, 0x400000);
	usleep(50);
    for(int i = 0;i < MAX_CLIENT_SIZE;++i){
		/* check id valid and send signo*/
		if(c[i].valid == 1){
			kill(c[i].cpid,SIGUSR1);
		}
	}
	munmap(c, sizeof(client_info) * MAX_CLIENT_SIZE);
    return;
}
void NpShell::name(int id,string s){
	char buf[BUFSIZE];
	memset( buf, 0, sizeof(char)*BUFSIZE );
	client_info *c =  (client_info *)mmap(NULL, sizeof(client_info) * MAX_CLIENT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, info_fd, 0);
	for(int i = 0;i < MAX_CLIENT_SIZE;++i){
		if(c[i].valid == 1 && c[i].name == s){
			if(c[i].cpid != getpid()){
				sprintf(buf,"*** User '%s' already exists. ***",s.c_str());
				string tmp(buf);
				cout << tmp << endl;
				munmap(c, sizeof(client_info) * MAX_CLIENT_SIZE);
				return;
			}
		}
	}
	s += '\0';
	strncpy(c[id-1].name,s.c_str(),s.length());
	sprintf(buf,"*** User from %s is named '%s'. ***",c[id-1].ip,s.c_str());
	//broadcast
    char *p = static_cast<char*>(mmap(NULL, 0x400000, PROT_READ | PROT_WRITE, MAP_SHARED, broadcast_fd, 0));
	string tmp(buf);
	/* end of string */
	tmp += '\0';
	strncpy(p, tmp.c_str(),tmp.length());
	munmap(p, 0x400000);
	usleep(50);
    for(int i = 0;i < MAX_CLIENT_SIZE;++i){
		/* check id valid and send signal*/
		if(c[i].valid == 1){
			kill(c[i].cpid,SIGUSR1);
		}
	}
	munmap(c, sizeof(client_info) * MAX_CLIENT_SIZE);
    return;
}
void NpShell::handle_child(int signo) {
	/* Declare function as [static] to purge the hidden [this] pointer
	 *          * C library does not know what the heck is this pointer.*/
	int status;
    if(signo == SIGCHLD){
        while (waitpid(-1, &status, WNOHANG) > 0) {
        }
    }
}
vector<string> NpShell::spilt_input(const string& s){	//spilt commands with space
	istringstream stream(s);
	vector<string> inputs;
	string input;
	while(stream>>input){
		inputs.push_back(input);
	}
	return inputs;	
}
int NpShell::printenv(string& s){
	char* char_arr;
	char_arr = &s[0];
	if(const char* env_p = getenv(char_arr))
		cout  << env_p <<endl;
	return 0;	
}
void NpShell::redirection(string& s){	
	int out = open(s.c_str(),O_WRONLY|O_CREAT| O_TRUNC, S_IRUSR | S_IWUSR);
	if (out == -1){
		cout << "File open error." << endl;
		return;
	}

	dup2(out, STDOUT_FILENO);
	return;
}
vector<string> NpShell::parse(string& s){	//split commands with pipe
	vector<string> res;
	size_t pos = 0;
	string token;
	string deli = " | ";
	while ((pos = s.find(deli)) != string::npos) {
		token = s.substr(0, pos);
		res.push_back(token);
		s.erase(0, pos + 3);
	}	
	res.push_back(s);
	return res;

}
int NpShell::operation(vector<string> s){
	char *commands[256];
	int redirectout = 0;
	int number_pipe = 0;
	int error_pipe = 0;
	char buf[10240];
	vector<int> num;
	size_t pos;
	string deli = "|";
	string deli2 = "!";
	vector<pipe_data> record;
	pipe_data p;
	for(int i = 0; i < s.size();++i){
		bool x = false;
		bool y = false;
		if(pos = s[i].find(deli) != string::npos) x = true;
		if(pos = s[i].find(deli2) != string::npos) y = true;
		if(s.size() > 1 && i != s.size()-1){		//create pipe
			if(pipe(p.fd) < 0)
				cout << "pipe create error"<<endl;
			record.push_back(p);
		}
		if((pos = s[i].find(deli)) != string::npos || (pos = s[i].find(deli2)) != string::npos ){	//create number pipe
			if(pipe(p.fd) < 0)
				cout << "pipe create error"<<endl;
			p.index = stoi(s[i].substr(pos+1));
			record_n.push_back(p);	
		}
		vector<string> tmp = spilt_input(s[i]);
		if(i == 0){
			for(int j = 0;j<record_n.size();++j){
					if(record_n[j].index == 0){
						num.push_back(j);
						record_n[j].index = -1;
					}
				}
		}
		signal(SIGCHLD,handle_child);
		pid_t c_pid = fork();
		while(c_pid < 0){	//for fork error
			sleep(1000);
			c_pid = fork();
		}
		if (c_pid > 0) {		//parent process
			if(i != 0){
				close(record[i-1].fd[0]);
				close(record[i-1].fd[1]);
			}
			if(i == 0){
				if(!num.empty()&& num.size() >1){
					for(int j = num.size()-1;j >= 0; --j){
						size_t nread = read(record_n[num[j]].fd[0],buf,10240);
						size_t nwrite = write(record_n[num[j-1]].fd[1],buf,nread);
					}
				}
				num.erase(num.begin(),num.end());
			}
			for(int j = 0;j<record_n.size();++j){
					if(record_n[j].index == -1){
						close(record_n[j].fd[0]);
						close(record_n[j].fd[1]);
						record_n.erase(record_n.begin()+j);
					}
				}
			if(i == s.size()-1 && !(x || y))
				waitpid(c_pid,nullptr,0);	

		} else if(c_pid == 0){	//child process
			for(int j = 0;j < tmp.size();++j){	//process command 
				if(tmp[j] == ">"){
					redirection(tmp[j+1]);
					redirectout = 1;
				}
				else if(pos = tmp[j].find(deli) != string::npos){
					number_pipe = 1;	
				}
				else if(pos = tmp[j].find(deli2) != string::npos){
					number_pipe = 1;
					error_pipe = 1;
				}	
				else{
					if(!redirectout){
						commands[j] = const_cast<char*>(tmp[j].c_str());
					}
				}
			}
			if(redirectout)
				commands[tmp.size()-2] = NULL;
			else if(number_pipe)
				commands[tmp.size()-1] = NULL;
			else
				commands[tmp.size()] = NULL;
			if(s.size() > 1){		//need to pipe
				if(i != 0){
					dup2(record[i-1].fd[0], STDIN_FILENO);
				}
				if(i != s.size()-1){
					dup2(record[i].fd[1], STDOUT_FILENO);	
				}

			}
			if(number_pipe == 1 ){	//number pipe
				int count = record_n.size()-1;
				dup2(record_n[count].fd[1],STDOUT_FILENO);
				if(error_pipe)
					dup2(record_n[count].fd[1],STDERR_FILENO);
				close(record_n[count].fd[1]);
			}
			if(i == 0){
				if(!num.empty()){
					dup2(record_n[num[0]].fd[0],STDIN_FILENO);
				}
			}
			for(int j = 0;j < record_n.size();++j){
				close(record_n[j].fd[0]);
				close(record_n[j].fd[1]);
			}
			for(int j = 0;j < record.size(); ++j){
				close(record[j].fd[0]);
				close(record[j].fd[1]);
			}
			execvp(commands[0],commands);
			cerr << "Unknown command: [" << commands[0] << "]." << endl;
			exit(EXIT_SUCCESS);
		}
	}
	return 0;	
}
int NpShell::exec(int id){
	string str;	
	cout << "% ";
	setenv("PATH","bin:.",1);
	while(getline(cin,str)){	
		vector<string> results = spilt_input(str);
		vector<string> cmds = parse(str);
		if(!results.empty()){
			if(results[0] == "printenv"){
				printenv(results[1]);
				for(int i = 0;i < record_n.size();++i){
					if(record_n[i].index > 0 )
						record_n[i].index--;
				}
			}
			else if(results[0] == "setenv"){
				setenv(results[1].c_str(),results[2].c_str(),1);	
				for(int i = 0;i < record_n.size();++i){
					if(record_n[i].index > 0 )
						record_n[i].index--;
				}

			}
			else if(results[0] == "exit" || results[0] == "EOF" ){
				return -1;	
			}
            else if(results[0] == "who"){
				who();
                for(int i = 0;i < record_n.size();++i){
					if(record_n[i].index > 0 )
						record_n[i].index--;
				}
            }
            else if(results[0] == "tell"){
				int target = stoi(results[1]);
				string msg = "";
				for(int i = 2;i < results.size();++i){
					msg += results[i];
					if(i != results.size()-1)
						msg += " ";
				}
				tell(id,target,msg);
                for(int i = 0;i < record_n.size();++i){
					if(record_n[i].index > 0 )
						record_n[i].index--;
				}
            }
            else if(results[0] == "yell"){
				string msg = "";
				for(int i = 1;i < results.size();++i){
					msg += results[i];
					if(i != results.size()-1)
						msg += " ";
				}
				yell(id,msg);
                for(int i = 0;i < record_n.size();++i){
					if(record_n[i].index > 0 )
						record_n[i].index--;
				}
            }
            else if(results[0] == "name"){
				string s = results[1];
				name(id,s);
                for(int i = 0;i < record_n.size();++i){
					if(record_n[i].index > 0 )
						record_n[i].index--;
				}
            }
			else {
				operation(cmds);	
				for(int i = 0;i < record_n.size();++i){
					if(record_n[i].index > 0 )
						record_n[i].index--;
				}

			}
		}		
		cout << "% ";
	}
	return 0;
}
