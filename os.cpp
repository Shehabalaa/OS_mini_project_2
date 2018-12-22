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
#include <queue>
using namespace std;

int process_id= 0, up_id, down_id, status_empty, clk = 0;
const short slots_num = 10;
const short max_str_len=65;
bool reserved[slots_num] = {0};
ofstream out;

struct msgbuff {
    long mtype;      // disk process id is used to tell it's status message when sent by disk and to tell it's request when sent by kernel
    char mtext[max_str_len]; // first character will be the mtext[0] or ther resporence
    short slot;      // used to tell number of empty slots in case of returing status or to determine which slot to delete

};

void kernelHandler(int sig);
void processHandler(int sig);
void diskHandler(int sig);

void kernel(long *childids) {
    signal(SIGINT, kernelHandler);
    signal(SIGUSR2, SIG_IGN);
    char trash[0];
    while (1) {
        msgbuff process_msg;
        msgbuff disk_msg;
        int msgid = msgrcv(up_id, &process_msg, sizeof(process_msg) - sizeof(process_msg.mtype), childids[3], IPC_NOWAIT | MSG_EXCEPT);
        if (msgid == -1){
            out << "no mesg recived from any process at clk " << clk << endl;
        }
        else {
            int which_process = 0;
            for (; which_process < 4 && childids[which_process] != process_msg.mtype; ++which_process);
            out << process_msg.mtext[0]<< " request recived in kernel from process " << which_process << endl;

            kill(childids[3], SIGUSR1); //ask for status
            int msgid = msgrcv(up_id, &disk_msg, sizeof(disk_msg) - sizeof(disk_msg.mtype), childids[3], !IPC_NOWAIT);
            if (process_msg.mtext[0] == 'A') {
                if (disk_msg.slot > 0) {
                    strcpy(disk_msg.mtext, process_msg.mtext);
                    int msgid1 = msgsnd(down_id, &disk_msg, sizeof(disk_msg) - sizeof(disk_msg.mtype), !IPC_NOWAIT);
                    process_msg.mtext[0] = '0';
                    int msgid2 = msgsnd(down_id, &process_msg, sizeof(process_msg) - sizeof(process_msg.mtype), !IPC_NOWAIT);
                } else {
                    process_msg.mtext[0] = '2';
                    int msgid1 = msgsnd(down_id, &process_msg, sizeof(process_msg) - sizeof(process_msg.mtype), !IPC_NOWAIT);
                    //cout<<"OOOOOOOOOOBBBBBBBBBBBBBBAAAAAAAAAAAAAAAAAA\n";
                }
            } else if (process_msg.mtext[0] == 'D') {
                if (disk_msg.mtext[process_msg.slot] == '1') {
                    disk_msg.slot = process_msg.slot;
                    int msgid1 = msgsnd(down_id, &disk_msg, sizeof(disk_msg) - sizeof(disk_msg.mtype), !IPC_NOWAIT);
                    process_msg.mtext[0]='1';
                    int msgid2 = msgsnd(down_id, &process_msg, sizeof(process_msg) - sizeof(process_msg.mtype), !IPC_NOWAIT);
                } else {
                    cout<<"OOOOOOOOOOBBBBBBBBBBBBBBAAAAAAAAAAAAAAAAAA\n";
                    process_msg.mtext[0]='3';
                    int msgid1 = msgsnd(down_id, &process_msg, sizeof(process_msg) - sizeof(process_msg.mtype), !IPC_NOWAIT);
                }
            }
        };
        for(int i=0;i<50000000;++i);
        out<<"--------------------------------------\n";
        ++clk;
        out << "Kernel clk " << clk << endl;
        killpg(getpgrp(), SIGUSR2);
        for(int i=0;i<50000000;++i);
    }
}

void process(int num) {
    signal(SIGUSR2, processHandler);
    string filename = "process" + to_string(num + 1) + ".txt";
    ifstream in_file;
    in_file.open(filename);
    string line, request, data;
    int time_clk;
    while (getline(in_file, line)) {
        stringstream ss(line);
        ss >>time_clk>>request;
        getline(ss,data);
        if(data[0]=='\"')
            data = data.substr(2,data.size()-3);
        while (time_clk > clk);
        msgbuff proces_msg;
        proces_msg.mtype = getpid();

        if (request == "ADD") {
            proces_msg.mtext[0] = 'A';
            strcpy(proces_msg.mtext+1, data.c_str());

            int msgid1 = msgsnd(up_id, &proces_msg, sizeof(proces_msg) - sizeof(proces_msg.mtype), !IPC_NOWAIT);
            if (msgid1 == -1)
                out << "error in sending messege to kernel" << endl;
            else {
                out << "send ADD reqest to kernel from process "<<process_id<< endl;
                int msgid2 = -1;
                while(msgid2==-1)
                    msgid2 = msgrcv(down_id, &proces_msg, sizeof(proces_msg) - sizeof(proces_msg.mtype), (long)getpid(), !IPC_NOWAIT); // wait even clk changend as kernel respond to only one process
                    
                if (proces_msg.mtext[0] == '0')
                    out << "successful ADD for proccess " << process_id << endl;
                else if (proces_msg.mtext[0] == '2'){
                    out << "unable to ADD for proccess " << process_id << endl;
                }
            }
        } else if (request == "DEL") {
            proces_msg.mtext[0] = 'D';
            proces_msg.slot =stoi(data);
            int msgid = msgsnd(up_id, &proces_msg, sizeof(proces_msg) - sizeof(proces_msg.mtype), !IPC_NOWAIT);
            if (msgid == -1)
                out << "error in send messege to kernel" << endl;
            else {
                out << "send DEL reqest to kernel from process "<<process_id<< endl;
                int msgid2=-1;
                while(msgid2==-1)
                    msgid2 = msgrcv(down_id, &proces_msg, sizeof(proces_msg) - sizeof(proces_msg.mtype), (long)getpid(), !IPC_NOWAIT); // wait even clk changend as kernel respond to only one process
                
                if (proces_msg.mtext[0] == '1')
                    out << "successful DEL for proccess " << process_id << endl;
                else if (proces_msg.mtext[0] == '3')
                    out << "unable to DEL for proccess " << process_id << endl;
                
            }
        }
    }

    in_file.close();
    while(1);
}

void disk() {
    signal(SIGUSR1, diskHandler);
    signal(SIGUSR2, diskHandler);
    queue<msgbuff> disk_buffer;
    char data_slots[slots_num][64];
    status_empty = slots_num;
    int busy=0;
    msgbuff buff_msg,procc_msg;
    int time_clk = 0;
    while (1) {
        time_clk = clk+1;
        int msgid = -1;
        if(busy>0){
            --busy;
            if(!busy){
                string op = (procc_msg.mtext[0]=='A')? "ADD ":"DEL ";
                out<< op <<"done in disk at clk "<<clk<<endl;
            }
        }
        while(time_clk>clk and msgid==-1){
            msgid = msgrcv(down_id, &buff_msg, sizeof(buff_msg) - sizeof(buff_msg.mtype), getpid(),IPC_NOWAIT); 
            if (msgid != -1){
                disk_buffer.push(buff_msg);
                if(buff_msg.mtext[0] == 'A'){
                    --status_empty;
                    for (int i = 0; i < slots_num; i++) {
                        if (!reserved[i]) {
                            reserved[i] = true;
                            strcpy(data_slots[i], buff_msg.mtext+1);
                            break;
                        }
                    } 
                } else if (buff_msg.mtext[0] == 'D') {
                    reserved[buff_msg.slot] = false;
                    ++status_empty;
                }

            }
        }
        if(!busy){
            if(!disk_buffer.empty()){
                procc_msg = disk_buffer.front();
                busy = (procc_msg.mtext[0] == 'A')? 3:1;
                disk_buffer.pop();
            }
        } 
        while(time_clk>clk);
        cout<<"BBBBBBBBBBBBBBBBUUUUUSSSSSSSSSSSSSSSSSSSSYYYYYYY "<<busy<<" CLKKKKKKKKKKK "<<time_clk-1<<endl;

    }
}

int main() {
    out.open("log.txt");
    int pid = 0;
    long child_ids[4];
    up_id = msgget(IPC_PRIVATE, IPC_CREAT | 0644);
    down_id = msgget(IPC_PRIVATE, IPC_CREAT | 0644);
    for (; process_id < 4; ++process_id) {
        pid = fork();
        if (!pid)
            break;
        child_ids[process_id] = pid;
    }
    if (pid) {
        kernel(child_ids);
    } else {
        if (process_id == 3)
            disk();
        else
            process(process_id);
    }
    
    return 0;
}

void kernelHandler(int sig) {
    signal(sig, kernelHandler);
    switch (sig) {
        case SIGINT:
            out << "\nUp and Down Deleted and Kernel and all processes are\n";
            msgctl(up_id, IPC_RMID, (struct msqid_ds *)0);
            msgctl(down_id, IPC_RMID, (struct msqid_ds *)0);
            killpg(getpgrp(), SIGKILL);
            out.close();
            break;
    }
}

void processHandler(int sig) {
    signal(sig, processHandler);
    switch (sig) {
        case SIGUSR2:
            ++clk;
            //out << "Process" << process_id << " clk" << clk << endl;
            break;
        case SIGINT:
            //out << "process_id am process" << process_id << " and process_id am killed\n";
            exit(0);
            break;
    }
}

void diskHandler(int sig) {
    signal(sig, diskHandler);
    switch (sig) {
        case SIGUSR2:
            ++clk;
            break;
        case SIGUSR1:
            msgbuff msg;
            msg.slot = status_empty;
            msg.mtype = getpid();
            for (int i = 0; i < slots_num; ++i)
                msg.mtext[i] = (reserved[i]) ? '1' : '0';
            int msgid = msgsnd(up_id, &msg, sizeof(msg) - sizeof(msg.mtype), !IPC_NOWAIT);
            break;
            /*
        case SIGINT:
            out<<"I am disk and i am killed\n";
            exit(0);
            break;*/
    }
}
