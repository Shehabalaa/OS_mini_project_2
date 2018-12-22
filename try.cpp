#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;

struct msgbuff {
    long mtype;      // disk process id is used to tell it's status message when sent by disk and to tell it's request when sent by kernel
    char operation;  // used to determine operation 'A' for add 'D'
    short slot;      // used to tell number of empty slots in case of returing status or to determine which slot to delete
    char mtext[64];
};

int main(){
    msgbuff msg;
    cout<<sizeof(msg.mtext)+sizeof(msg.operation)+sizeof(msg.slot);
}