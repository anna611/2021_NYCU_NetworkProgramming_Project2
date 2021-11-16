#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <string>
#include<sstream>
#include <vector>
#include <cstdlib>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include "memory.h"
#include <algorithm>
using namespace std;

#define MAX_CLIENT_SIZE 30
#define BUFSIZE 100

typedef struct{
	int fd[2];
	int index = -1;
}pipe_data;

typedef struct{
	int fd;
	int id;
	string name;
	char ip[INET_ADDRSTRLEN];
	int port;
	int finish;
	string env;
	string env_v;
	vector<pipe_data> numpipe;
}client_info;

typedef struct{
	int fd[2];
	int source_id;
	int dest_id;
	bool is_used;
}userpipe;

fd_set rfds;
fd_set afds;
int msock;
int record_id[MAX_CLIENT_SIZE];
vector<userpipe> userpipe_record;
vector<client_info> client_record;
class NpShell{
	private:
		int sockfd;
	public:
		static void handle_child(int);
		vector<string> spilt_input(const string&);
		int printenv(string&);
		void redirection(string&);
		vector<string> parse(string&);
		int operation(vector<string>,client_info);
		int exec(string,client_info);
		void who(client_info);
		void tell(client_info,vector<string>);
		void yell(client_info,vector<string>);
		void name(client_info,vector<string>);
		//  int GetUnusedID();

};
void welcome(int sockfd){
	string msg = "";
	msg += "****************************************\n";
	msg += "** Welcome to the information server. **\n";
	msg += "****************************************\n";
	if(write(sockfd,msg.c_str(),msg.length()) == -1)
		perror("send error");
}
void login(client_info &cli){
	int nfds = getdtablesize();
	string msg = "*** User '"+cli.name+"' entered from "+string(cli.ip)+":"+to_string(cli.port)+". ***\n";
	for(int fd = 0;fd < nfds; ++fd){
		if(fd != msock && FD_ISSET(fd,&afds)){
			if(write(fd,msg.c_str(),msg.length()) == -1)
				perror("send error");
		}
	}
}
void logout(client_info &cli){
	int nfds = getdtablesize();
	string msg = "*** User '"+cli.name+"' left. ***\n";
	for(int fd = 0;fd < nfds; ++fd){
		if(fd != msock && FD_ISSET(fd,&afds)){
			if(write(fd,msg.c_str(),msg.length()) == -1)
				perror("send error");
		}
	}
	for(int i = 0;i < userpipe_record.size();++i){
		if(userpipe_record[i].source_id == cli.id || userpipe_record[i].dest_id == cli.id){
			userpipe_record.erase(userpipe_record.begin()+i);
			i--;
		}
	}
}
void NpShell::who(client_info cli){
	cout << "<ID>\t" << "<nickname>\t" << "<IP:port>\t"<<"<indicate me>"<< endl;
	for(int i = 0;i < MAX_CLIENT_SIZE;++i){
		if(record_id[i] == 1 ){
			for(int j =0;j < client_record.size();++j){
				if(client_record[j].id == i+1 && client_record[j].finish == 0){
					cout << i+1<<"\t"<<client_record[j].name<<"\t"<<client_record[j].ip;
					cout << ":" << client_record[j].port << "\t";
					if(cli.id == client_record[j].id)
						cout << "<-me" << endl;
					else
						cout << endl;
				}
			}
		}
	}
	return;
}
void NpShell::tell(client_info cli,vector<string> s){
	int id = stoi(s[1]);
	string msg = "";
	for(int i = 0;i < client_record.size();++i){
		if(client_record[i].id == id && client_record[i].finish == 0){
			msg = "*** " + cli.name+" told you ***: ";
			for(int i = 2;i < s.size();++i){
				msg += s[i];
				if(i != s.size()-1)
					msg += " ";
			}
			msg += "\n";
			if(write(client_record[i].fd,msg.c_str(),msg.length()) == -1)
				perror("send error");
			return;
		}
	}
	msg = "*** Error: user #"+ s[1] +" does not exist yet. ***\n";
	if(write(cli.fd,msg.c_str(),msg.length()) == -1)
		perror("send error");
	return;
}
void NpShell::yell(client_info cli,vector<string> s){
	string msg = "";
	msg = "*** "+ cli.name + " yelled ***: ";
	int nfds = getdtablesize();
	for(int i = 1;i < s.size();++i){
		msg += s[i];
		if(i != s.size()-1)
			msg += " ";
	}
	msg += "\n";
	for(int fd = 0;fd < nfds; ++fd){
		if(fd != msock && FD_ISSET(fd,&afds)){
			if(write(fd,msg.c_str(),msg.length()) == -1)
				perror("send error");
		}
	}
	return;
}
void NpShell::name(client_info cli,vector<string> s){
	int index;
	string name = "";
	for(int i = 1;i < s.size();++i){
		name += s[i];
		if(i != s.size()-1)
			name += " ";
	}
	string msg = "";
	int nfds = getdtablesize();
	//check name exist or not
	for(int i = 0;i < client_record.size();++i){
		if(client_record[i].name == name && client_record[i].finish == 0){
			index = -1;
			break;
		}
		else{
			if(client_record[i].id == cli.id && client_record[i].finish == 0){
				index = i;
			}
		}
	}
	if(index != -1){
		client_record[index].name = name;
		msg = "*** User from " + string(cli.ip)+":"+to_string(cli.port) + " is named '"+ name +"'. ***\n";
		for(int fd = 0;fd < nfds; ++fd){
			if(fd != msock && FD_ISSET(fd,&afds)){
				if(write(fd,msg.c_str(),msg.length()) == -1)
					perror("send error");
			}
		}
	}
	else{
		msg = "*** User '"+ s[1] +"' already exists. ***\n";
		if(write(cli.fd,msg.c_str(),msg.length()) == -1)
			perror("send error");
	}

	return;
}
void NpShell::handle_child(int signo) {
	/* Declare function as [static] to purge the hidden [this] pointer
	 *          * C library does not know what the heck is this pointer.*/
	int status;
	while (waitpid(-1, &status, WNOHANG) > 0) {

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
int NpShell::operation(vector<string> s,client_info cli){
	char *commands[256];
	int redirectout = 0;
	int number_pipe = 0;
	int error_pipe = 0;
	int user_pipe = 0;
	int user_pipe_recv = 0;
	char buf[10240];
	vector<int> num;
	size_t pos;
	string deli = "|";
	string deli2 = "!";
	string deli3 = ">";
	string deli4 = "<";
	vector<pipe_data> record;
	pipe_data p;
	userpipe u;
	string msg = "";
	int nfds;
	int dest = -1;
	int source = -1;
	bool exist = false;
	bool exist_r = false;
	string cmd = "";
	bool userpipe_error = false;
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
			cli.numpipe.push_back(p);
		}
		vector<string> tmp = spilt_input(s[i]);
		//check whether need userpipe
		for(int j = 0;j < tmp.size();++j){
			cmd += tmp[j];
			if(j!= tmp.size()-1)
				cmd += " ";
			if(((pos = tmp[j].find(deli3)) != string::npos) && tmp[j].length()>1){
				dest = stoi(tmp[j].substr(pos+1));
				tmp.erase(tmp.begin()+j);
				j--;
			}
			if(((pos = tmp[j].find(deli4)) != string::npos) && tmp[j].length()>1){
				source = stoi(tmp[j].substr(pos+1));
				tmp.erase(tmp.begin()+j);
				j--;
			}
		}
		if(i != s.size()-1){
			cmd += " | ";
		}
		//source id exist
		if(source != -1){
			if(record_id[source-1] == 0 || source > 30){
				userpipe_error = true;
				msg = "*** Error: user #"+to_string(source)+" does not exist yet. ***\n";
				if(write(cli.fd,msg.c_str(),msg.length()) == -1)
					perror("send error");
			}
			else{
				for(int j = 0;j < userpipe_record.size();++j){
					if(userpipe_record[j].source_id == source && userpipe_record[j].dest_id == cli.id){
						if(userpipe_record[j].is_used == true){
							exist_r = true;
							userpipe_record[j].is_used = false;
						}
					}
				}
				//check whether exist
				if(exist_r){
					string source_name = "";
					user_pipe_recv = 1;
					for(int j = 0; j < client_record.size();++j){
						if(client_record[j].id == source && client_record[j].finish == 0)
							source_name = client_record[j].name;
					}
					if(i == s.size()-1){
						msg = "*** "+cli.name+" (#"+to_string(cli.id)+") just received from "+source_name+" (#"+to_string(source)+") by '"+cmd+"' ***\n";
						nfds = getdtablesize();
						//broadcast
						for(int fd = 0;fd < nfds; ++fd){
							if(fd != msock && FD_ISSET(fd,&afds)){
								if(write(fd,msg.c_str(),msg.length()) == -1)
									perror("send error");
							}
						}
					}
				}
				else{
					userpipe_error = true;
					msg += "*** Error: the pipe #"+to_string(source)+"->#"+to_string(cli.id)+" does not exist yet. ***\n";
					if(write(cli.fd,msg.c_str(),msg.length()) == -1)
						perror("send error");
				}
			}
		}
		//dest id exist 
		if(dest != -1){
			if(record_id[dest-1] == 0 || dest > 30){
				userpipe_error = true;
				msg = "*** Error: user #"+to_string(dest)+" does not exist yet. ***\n";
				if(write(cli.fd,msg.c_str(),msg.length()) == -1)
					perror("send error");
			}
			else{
				for(int j = 0;j < userpipe_record.size();++j){
					if(userpipe_record[j].source_id == cli.id && userpipe_record[j].dest_id == dest){
						if(userpipe_record[j].is_used == true)
							exist = true;
					}
				}
				//check whether exist
				if(!exist){
					if(pipe(u.fd) < 0)
						cout << "pipe create error"<<endl;
					u.source_id = cli.id;
					u.dest_id = dest;
					u.is_used = true;
					userpipe_record.push_back(u);
					string dest_name = "";
					user_pipe = 1;
					for(int j = 0; j < client_record.size();++j){
						if(client_record[j].id == dest && client_record[j].finish == 0)
							dest_name = client_record[j].name;
					}
					if(i == s.size()-1){
						msg = "*** "+cli.name+" (#"+to_string(cli.id)+") just piped '"+cmd+"' to "+dest_name+" (#"+to_string(dest)+") ***\n";
						nfds = getdtablesize();
						//broadcast
						for(int fd = 0;fd < nfds; ++fd){
							if(fd != msock && FD_ISSET(fd,&afds)){
								if(write(fd,msg.c_str(),msg.length()) == -1)
									perror("send error");
							}
						}
					}
				}
				else{
					userpipe_error = true;
					msg += "*** Error: the pipe #"+to_string(cli.id)+"->#"+to_string(dest)+" already exists. ***\n";
					if(write(cli.fd,msg.c_str(),msg.length()) == -1)
						perror("send error");
				}
			}
		}
		if(i == 0){
			for(int j = 0;j<cli.numpipe.size();++j){
				if(cli.numpipe[j].index == 0){
					num.push_back(j);
					cli.numpipe[j].index = -1;
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
						size_t nread = read(cli.numpipe[num[j]].fd[0],buf,10240);
						size_t nwrite = write(cli.numpipe[num[j-1]].fd[1],buf,nread);
					}
				}
				num.erase(num.begin(),num.end());
			}
			for(int j = 0;j<cli.numpipe.size();++j){
				if(cli.numpipe[j].index == -1){
					close(cli.numpipe[j].fd[0]);
					close(cli.numpipe[j].fd[1]);
					cli.numpipe.erase(cli.numpipe.begin()+j);
					j--;
				}
			}
			for(int j = 0;j < client_record.size();++j){
				if(cli.fd == client_record[j].fd && client_record[j].finish == 0)
					client_record[j] = cli;
			}
			for(int j = 0;j<userpipe_record.size();++j){
				if(userpipe_record[j].is_used == false){
					close(userpipe_record[j].fd[0]);
					close(userpipe_record[j].fd[1]);
					userpipe_record.erase(userpipe_record.begin()+j);
					j--;
				}
			}
			if(i == s.size()-1 && !(x || y) && !user_pipe){
				waitpid(c_pid,nullptr,0);
			}
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
				int count = cli.numpipe.size()-1;
				dup2(cli.numpipe[count].fd[1],STDOUT_FILENO);
				if(error_pipe)
					dup2(cli.numpipe[count].fd[1],STDERR_FILENO);
				close(cli.numpipe[count].fd[1]);
			}
			if(user_pipe == 1){
				int count = userpipe_record.size()-1;
				dup2(userpipe_record[count].fd[1],STDOUT_FILENO);
			}
			if(user_pipe_recv == 1){
				for(int j = 0;j < userpipe_record.size();++j){
					if(userpipe_record[j].source_id == source && userpipe_record[j].dest_id == cli.id){
						dup2(userpipe_record[j].fd[0],STDIN_FILENO);
						userpipe_record[j].is_used = false;
						close(userpipe_record[j].fd[0]);
					}
				}
			}
			if(i == 0){
				if(!num.empty()){
					dup2(cli.numpipe[num[0]].fd[0],STDIN_FILENO);
				}
			}
			for(int j = 0;j < cli.numpipe.size();++j){
				close(cli.numpipe[j].fd[0]);
				close(cli.numpipe[j].fd[1]);
			}
			for(int j = 0;j < client_record.size();++j){
				if(cli.fd == client_record[j].fd && client_record[j].finish == 0)
					client_record[j] = cli;
			}
			for(int j = 0;j < record.size(); ++j){
				close(record[j].fd[0]);
				close(record[j].fd[1]);
			}
			for(int j = 0;j < userpipe_record.size(); ++j){
				close(userpipe_record[j].fd[0]);
				close(userpipe_record[j].fd[1]);
			}
			if(userpipe_error){
				int null_fd0 = open("/dev/null", O_RDONLY);
				int null_fd1 = open("/dev/null", O_WRONLY);
				dup2(null_fd0,STDIN_FILENO);
				dup2(null_fd1,STDOUT_FILENO);
				close(null_fd0);
				close(null_fd1);
			}
			execvp(commands[0],commands);
			cerr << "Unknown command: [" << commands[0] << "]." << endl;
			exit(EXIT_SUCCESS);
		}
	}
	return 0;	
}
int NpShell::exec(string str,client_info cli){
	setenv(cli.env.c_str(),cli.env_v.c_str(),1);
	sockfd = cli.fd;
	dup2(sockfd,STDOUT_FILENO);
	dup2(sockfd,STDERR_FILENO);
	vector<string> results = spilt_input(str);
	vector<string> cmds = parse(str);
	if(!results.empty()){
		if(results[0] == "printenv"){
			printenv(results[1]);
			for(int i = 0;i < client_record.size();++i){
				if(cli.fd == client_record[i].fd && client_record[i].finish == 0){
					for(int j = 0;j < client_record[i].numpipe.size();++j){
						if(client_record[i].numpipe[j].index > 0 )
							client_record[i].numpipe[j].index--;
					}
				}
			}
		}
		else if(results[0] == "setenv"){
			setenv(results[1].c_str(),results[2].c_str(),1);	
			for(int i = 0;i < client_record.size();++i){
				if(cli.fd == client_record[i].fd && client_record[i].finish == 0){
					for(int j = 0;j < client_record[i].numpipe.size();++j){
						if(client_record[i].numpipe[j].index > 0 )
							client_record[i].numpipe[j].index--;
					}
					client_record[i].env = results[1];
					client_record[i].env_v = results[2];
				}
			}
		}
		else if(results[0] == "exit" || results[0] == "EOF" ){
			return -1;
		}
		else if(results[0] == "who"){
			who(cli);	
			for(int i = 0;i < client_record.size();++i){
				if(cli.fd == client_record[i].fd && client_record[i].finish == 0){
					for(int j = 0;j < client_record[i].numpipe.size();++j){
						if(client_record[i].numpipe[j].index > 0 )
							client_record[i].numpipe[j].index--;
					}
				}
			}
		}
		else if(results[0] == "tell"){
			tell(cli,results);	
			for(int i = 0;i < client_record.size();++i){
				if(cli.fd == client_record[i].fd && client_record[i].finish == 0){
					for(int j = 0;j < client_record[i].numpipe.size();++j){
						if(client_record[i].numpipe[j].index > 0 )
							client_record[i].numpipe[j].index--;
					}
				}
			}
		}
		else if(results[0] == "yell"){
			yell(cli,results);	
			for(int i = 0;i < client_record.size();++i){
				if(cli.fd == client_record[i].fd && client_record[i].finish == 0){
					for(int j = 0;j < client_record[i].numpipe.size();++j){
						if(client_record[i].numpipe[j].index > 0 )
							client_record[i].numpipe[j].index--;
					}
				}
			}
		}
		else if(results[0] == "name"){
			name(cli,results);	
			for(int i = 0;i < client_record.size();++i){
				if(cli.fd == client_record[i].fd && client_record[i].finish == 0){
					for(int j = 0;j < client_record[i].numpipe.size();++j){
						if(client_record[i].numpipe[j].index > 0 )
							client_record[i].numpipe[j].index--;
					}
				}
			}
		}
		else {
			operation(cmds,cli);	
			for(int i = 0;i < client_record.size();++i){
				if(cli.fd == client_record[i].fd && client_record[i].finish == 0){
					for(int j = 0;j < client_record[i].numpipe.size();++j){
						if(client_record[i].numpipe[j].index > 0 )
							client_record[i].numpipe[j].index--;
					}
				}
			}
		}
	}
	cout << "% ";
	fflush(stdout);		
	return 0;
}
