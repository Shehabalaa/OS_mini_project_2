#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <fstream>
#include <string>
#include <string.h>
#include<iostream>
using namespace std;

int i=0,up_id,down_id,status_empty,clk=0;

struct msgbuff
{
   long mtype; // disk process id is used to tell it's status message when sent by disk and to tell it's request when sent by kernel
   char operation; // used to determine operation 'A' for add 'D' 
   short slot; // used to tell number of empty slots in case of returing status or to determine which slot to delete 
   char mtext[64];
};
void kernelHandler(int sig);
void processHandler(int sig);
void diskHandler(int sig);


void kernel(int * childids){
    signal(SIGINT,kernelHandler);
    signal(SIGUSR2,kernelHandler);
    while(1){

        sleep(1);
        killpg(getpgrp(),SIGUSR2);
        sleep(1);
        cout<<endl;
    };
}



void process(int num){
    signal(SIGUSR2,processHandler);
    string filename = "process" + to_string(num+1) + ".txt";
    ifstream in_file;
    in_file.open(filename);
    int time;string operation,data;
    while(in_file>>time){
        in_file>>operation>>data;
        //cout<<data<<endl;
    }
    in_file.close();

    while(1);
}


void disk(){
    signal(SIGUSR1,diskHandler);
    signal(SIGUSR2,diskHandler);
    const short slots_num=10;
    char data_slots[slots_num][64];
    bool reserved[slots_num] ={0};
    status_empty = slots_num;
    msgbuff msg;
    while(1){
        int msgid = msgrcv(down_id,&msg,sizeof(msg)-sizeof(msg.mtype),getpid(),!IPC_NOWAIT);
        if(msg.operation=='A'){
            int starting_time = clk;
            --status_empty;
            for(int i = 0; i < slots_num; i++){
                if(!reserved[i]){
                    reserved[i]=true;
                    strcpy(data_slots[i],msg.mtext);
                    break;
                }
            }
            while(clk<starting_time+3);
        }else if(msg.operation=='D'){
            int starting_time = clk;
            ++status_empty;
            reserved[i]=true;
            while(clk<starting_time+1);
        }
    }
    
}


int main(){
    int pid=0;
    int child_ids[4];
    up_id = msgget(IPC_PRIVATE, IPC_CREAT|0644);
    down_id = msgget(IPC_PRIVATE, IPC_CREAT|0644);
    for(; i < 4; ++i){
        pid = fork();
        if(!pid)
            break;
        child_ids[i]=pid;
    }
    if(pid){
        kernel(child_ids);
    }
    else{
        if(i==3)
            disk();
        else
            process(i);
    }
    
    return 0;
}


void kernelHandler(int sig){
    signal(sig,kernelHandler);
    switch (sig){
        case SIGINT:
            cout<<"\nUp and Down Deleted and Kernel and all processes are\n";
            msgctl(up_id, IPC_RMID, (struct msqid_ds *) 0);
            msgctl(down_id, IPC_RMID, (struct msqid_ds *) 0);
            killpg(getpgrp(),SIGKILL);
            break;
        case SIGUSR2:
            ++clk;
            cout<<"Kernel clk"<<clk<<endl;
            break;
    }
}

void processHandler(int sig){
    signal(sig,processHandler);
    switch (sig){
        case SIGUSR2:
            ++clk;
            cout<<"Process"<<i<<" clk"<<clk<<endl;
            break;
        /*case SIGINT:
            cout<<"I am process"<<i<<" and i am killed\n";
            exit(0);
            break;*/
    }
}


void diskHandler(int sig){
    signal(sig,diskHandler);
    switch (sig){
        case SIGUSR2:
            ++clk;
            cout<<"Disk clk"<<clk<<endl;
            break;
        case SIGUSR1:
            msgbuff msg;
            msg.slot = status_empty;
            msg.mtype = getpid();
            msgsnd(up_id,&msg,sizeof(msg)-sizeof(msg.mtype),!IPC_NOWAIT);
            break;

        /*
        case SIGINT:
            cout<<"I am disk and i am killed\n";
            exit(0);
            break;*/
    }
}
