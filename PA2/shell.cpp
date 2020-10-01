#include <iostream>
#include <cctype>
#include <sstream>
#include <iterator>
#include <vector>
#include <string>
#include <chrono>

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>

using namespace std;

// Trims given string's leading and trailing spaces
string trim(const string &s) {
    if(s.length() == 0){
        return s;
    }

    size_t startPos =  s.find_first_not_of(" \t\n");
    size_t endPos = s.find_last_not_of(" \t\n");

    return s.substr(startPos, endPos - startPos + 1);
}

//Convert vector to char**
char** vec_to_char_array(vector<string> &in) {
    char** ret = new char*[in.size()+1];
    for(int i = 0; i < in.size(); ++i) {
        const char* cpyStr = in[i].c_str();
        ret[i] = new char[strlen(cpyStr)];
        strcpy(ret[i], cpyStr);
    }
    ret[in.size()] = NULL;
    return ret;
}

// Splits string by spaces into vector
vector<string> split(string in) {
    vector<string> ret;
    istringstream ss(in);

    while(ss) {
        string op;
        ss >> op;



        if(op != "")
            ret.push_back(op);
    } 
    return ret;
}

// Splits string by given delimeter into vector
vector<string> split(string in, string del) {
    vector<string> ret;

    size_t pos;
    while ((pos = in.find(del)) != string::npos) {
        string tok = in.substr(0, pos);
        ret.push_back(trim(tok));
        in.erase(0, pos + del.length());
    }

    ret.push_back(trim(in));

    return ret;
}

int main(){
    vector<string> hist;

    char* name = new char[30];
    getlogin_r(name, 30);

    while(true) {
        // Custom prompt
        time_t t = chrono::system_clock::to_time_t(chrono::system_clock::now());
        auto time = localtime(&t);

        cout << time->tm_hour << ":" << time->tm_min << "-" 
             << time->tm_mon << "/" << time->tm_mday << "-";
        cout << name << " $ ";

        //Get input
        string inLine;
        getline(cin, inLine);

        if(inLine == string("exit")) {
            cout << "Shell teminated..." << endl;
            break;
        }

        hist.push_back(inLine);

        vector<string> pipeParts = split(inLine, "|");
        int defStdIn = dup(0);

        for(int i = 0; i < pipeParts.size(); ++i) {

            // Set up pipe
            int fd[2];
            pipe(fd);

            inLine = pipeParts[i];

            // Create child
            int pid = fork();
            if(pid == 0) {
                int pos = inLine.find('>');
                if(pos != -1) {
                    string command = trim(inLine.substr(0,pos));
                    string fileName = trim(inLine.substr(pos+1));

                    inLine = command;

                    int fd = open(fileName.c_str(), O_WRONLY|O_CREAT, S_IWUSR|S_IRUSR);
                    dup2(fd, 1);
                    close(fd);
                }      
                pos = inLine.find('<');
                if(pos != -1) {
                    string command = trim(inLine.substr(0,pos));
                    string fileName = trim(inLine.substr(pos+1));

                    inLine = command;

                    int fd = open(fileName.c_str(), O_RDONLY, S_IWUSR|S_IRUSR);
                    dup2(fd, 0);
                    close(fd);
                }            

                if(i < pipeParts.size() - 1) {
                    dup2(fd[1], 1);
                }

                vector<string> ops = split(inLine);
                char** args = vec_to_char_array(ops);

                // for(int i=0;i<ops.size();++i){
                //     printf("%s\n", args[i]);
                // }

                execvp(args[0], args);
            }
            else {
                if(i == pipeParts.size() - 1) {
                    waitpid(pid, 0, 0);
                }
                dup2(fd[0], 0);
                close(fd[1]);
            }
        }
        dup2(defStdIn,0);
    }
}