#include <iostream>
#include <sstream>
#include <unistd.h>
#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

int main(){
    while(true) {
        cout << "My Shell$";
        string in;
        getline(cin, in);

        if(in == string("exit")) {
            cout << "Shell teminated..." << endl;
            break;
        }

        int pid = fork();
        if(pid == 0) {
            char* args[] = {(char *) in.c_str(), NULL};
            execvp(args[0], args);
        }
        else {
            waitpid(pid, 0, 0);
        }

        // int fd = open ("foo.txt", O_CREAT|O_WRONLY|O_TRUNC,
        // S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        // dup2 (fd, 1); // overwriting stdout with the new file
        // execlp ("ls", "ls", "-l", "-a", "-w", NULL); // execute
        // return 0;
    }
}