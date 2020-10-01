/*
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date  : 2/8/20
 */
/*
    Edited by:
    Reo Matsuda
    825001809
*/
#include "common.h"
#include "FIFOreqchannel.h"

using namespace std;

int buffersize = MAX_MESSAGE;

double calcTime(timeval start, timeval end) {
    double time_taken;
    
    time_taken = (end.tv_sec - start.tv_sec) * 1e6;
    time_taken = (time_taken + (end.tv_usec - start.tv_usec)) * 1e-6;

    return time_taken;
}

void getMessage(FIFORequestChannel* chan, int person, int eno, int startTime, int points) {
    double ret;
    double time = startTime;

    ofstream rec("received/" + to_string(person) + "_" + to_string(eno) + ".csv");

    timeval start, end;
    gettimeofday(&start, NULL);

    for(int i=0;i<points;++i){
        datamsg msg = datamsg(person, time, eno);
        chan->cwrite (&msg, sizeof (datamsg));

        chan->cread (&ret, sizeof(double));
        
        rec << ret << endl;

        time += .004;
    }
    gettimeofday(&end, NULL);

    double time_taken = calcTime(start, end);

    cout << "Time taken: " << time_taken << endl;
    rec.close();
}

__int64_t getFileSize(FIFORequestChannel* chan, char* name) {
    filemsg msg = filemsg(0, 0);
    char pckg[sizeof(msg) + sizeof(name)];
    memmove(pckg, &msg, sizeof(msg));
    memmove(pckg+sizeof(msg), name, sizeof(name));

    chan->cwrite(&pckg, sizeof(pckg));

    __int64_t filesize;
    chan->cread(&filesize, sizeof(__int64_t));

    return filesize;
}

void getFile(FIFORequestChannel* chan, char* name) {
    ofstream rec("received/"+(string) name);

    timeval start, end;
    gettimeofday(&start, NULL);
    __int64_t filesize = getFileSize(chan, name);

    __int64_t windowNum = filesize / buffersize;
    cout << windowNum << endl;
    char windowbuffer[buffersize];

    for(__int64_t i=0; i < windowNum; ++i) {
        __int64_t offset = i*buffersize;
        filemsg msg = filemsg(offset, buffersize);

        char pckg[sizeof(msg) + sizeof(name)];
        memmove(pckg, &msg, sizeof(msg));
        memmove(pckg+sizeof(msg), name, sizeof(name));

        chan->cwrite(&pckg, sizeof(pckg));
        chan->cread(windowbuffer, buffersize);

        rec.write(windowbuffer, buffersize);
    }

    if(filesize % buffersize != 0) {
        int leftover = filesize % buffersize;
        __int64_t offset = windowNum * buffersize;
        filemsg msg = filemsg(offset, leftover);

        char pckg[sizeof(msg) + sizeof(name)];
        memmove(pckg, &msg, sizeof(msg));
        memmove(pckg+sizeof(msg), name, sizeof(name));

        chan->cwrite(&pckg, sizeof(pckg));
        chan->cread(windowbuffer, leftover);

        rec.write(windowbuffer, leftover);
    }
    gettimeofday(&end, NULL);

    double time_taken = calcTime(start, end);
    cout << "Time taken: " << time_taken << endl;

    rec.close();
}

void makeNewChannel(FIFORequestChannel* chan) {
    MESSAGE_TYPE msg = NEWCHANNEL_MSG;
    chan->cwrite(&msg, sizeof(MESSAGE_TYPE));
    char name[30];
    chan->cread(name, sizeof(name));

    FIFORequestChannel *data_channel = new FIFORequestChannel ((string) name, FIFORequestChannel::CLIENT_SIDE);

    getMessage(data_channel, 1, 1, 0, 1000);

    MESSAGE_TYPE m = QUIT_MSG;
    data_channel->cwrite (&m, sizeof (MESSAGE_TYPE));

    delete data_channel;
}

int main(int argc, char *argv[]){
    pid_t pid = fork();
    int status;
    int opt; 
    string size;
    if(pid == 0) {
        while((opt = getopt(argc, argv, "p:t:e:m:f:c")) != -1){
            if((char) opt == 'm'){
                char *args[] = {"./server", "-m", optarg, NULL};

                execvp("./server", args);
            }
        }
        char *args[] = {"", NULL};
        execvp("./server", args);
    }
    else {
        FIFORequestChannel* chan = new FIFORequestChannel ("control", FIFORequestChannel::CLIENT_SIDE);

        int opt;
        int patient = -1, time = -1, ecgno = -1;
        char *name;
        while ((opt = getopt(argc, argv, "p:t:e:m:f:c")) != -1) {
            switch (opt) {
                case 'p':
                    patient = atoi (optarg);
                    break;
                case 't':
                    time = atoi (optarg);
                    break;
                case 'e':
                    ecgno = atoi (optarg);
                    break;
                case 'm':
                    buffersize = atoi (optarg);
                    break;
                case 'f':
                    name = new char[strlen(optarg)];
                    strncpy(name, optarg, strlen(optarg));
                    getFile(chan, name);
                    break;
                case 'c':
                    makeNewChannel(chan);
                    break;
                default:
                    abort();
            }
        }

        if(patient != -1 && time != -1 && ecgno != -1) {
            getMessage(chan, patient, ecgno, time, 1000);
        }
        // getMessage(chan, 4, 1, 1000);

        // closing the channel
        MESSAGE_TYPE m = QUIT_MSG;
        chan->cwrite (&m, sizeof (MESSAGE_TYPE));

        wait(&status);

        delete chan;
    }

    return 0;
}
