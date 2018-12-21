#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sstream>
#include <fstream>
#include <string>
#include <string.h>
#include <iostream>
using namespace std;
/////////////////////////////////////////////////////////////////////////
int i = 0, up_id, down_id, status_empty, clk = 0;
const short slots_num = 10;
bool reserved[slots_num] = {0};
//////////////////////////////////////////////////////////////////////////
struct msgbuff
{
    long mtype;     // disk process id is used to tell it's status message when sent by disk and to tell it's request when sent by kernel
    char operation; // used to determine operation 'A' for add 'D'
    short slot;     // used to tell number of empty slots in case of returing status or to determine which slot to delete
    char mtext[64];
};
////////////////////////////////////////////////////////////////////////
void kernelHandler(int sig);
void processHandler(int sig);
void diskHandler(int sig);
//////////////////////////////////////////////////////////////////////
void kernel(int *childids)
{
    signal(SIGINT, kernelHandler);
    signal(SIGUSR2, kernelHandler);
 
    while (1)
    {
        string responce;
        msgbuff msg1;
        msgbuff msg2;
        int msgid = msgrcv(up_id, &msg1, sizeof(msg1) - sizeof(msg1.mtype), childids[3], IPC_NOWAIT|MSG_EXCEPT);
        if (msgid == -1)
            cout << "no mesg recived from any process at clk"<<clk << endl;
        else
        {
            kill(childids[3], SIGUSR1);
            int which_process = 0;
            for (; which_process < 4 && childids[which_process] != msg1.mtype; ++which_process);
            cout << "successful reciveing in kernel from process "<< which_process<< endl;

            if (msg1.operation == 'A')
            {
                int msgid = msgrcv(up_id, &msg2, sizeof(msg2) - sizeof(msg2.mtype), childids[3], !IPC_NOWAIT);
                if (msgid == -1)
                    cout << "error in recieve messege from disk in kernel" << endl;
                else {
                      cout << "successful recieve from disk in kernel case add" << endl;
                if (msg2.slot > 0)
                {
                    strcpy(msg2.mtext, msg1.mtext);
                    int msgid1 = msgsnd(down_id, &msg2, sizeof(msg2) - sizeof(msg2.mtype), !IPC_NOWAIT);
                    if (msgid1 == -1)
                        cout << "error in sending messege to disk in kernel" << endl;
                    else{
                        msg1.mtext[0]='0';
                        int msgid2 = msgsnd(down_id, &msg1, sizeof(msg1) - sizeof(msg1.mtype), !IPC_NOWAIT);
                        if (msgid2 == -1)
                            cout << "error in sending messege to procces in kernel" << endl;
                    }
                }
                else
                {
                    msg1.mtext[0]='2';
                    int msgid1 = msgsnd(down_id, &msg1, sizeof(msg1) - sizeof(msg1.mtype), !IPC_NOWAIT);
                }
            }
            }
            else if (msg1.operation == 'D')
            {
                int msgid = msgrcv(up_id, &msg2, sizeof(msg2) - sizeof(msg2.mtype), childids[3], !IPC_NOWAIT);
                if (msgid == -1)
                    cout << "error in recieve messege from disk in kernel" << endl;
                else
                { 
                    cout << "successful recieve from disk in kernel case delete" << endl;
                    
                    if (msg2.mtext[msg1.slot] == '1')
                    {
                        msg2.slot = msg1.slot;
                        int msgid1 = msgsnd(down_id, &msg2, sizeof(msg2) - sizeof(msg2.mtype), !IPC_NOWAIT);
                        if (msgid1 == -1)
                            cout << "error in sending messege to disk in kernel" << endl;
                        responce = "1";
                        strcpy(msg1.mtext, responce.c_str());
                        int msgid2 = msgsnd(down_id, &msg1, sizeof(msg1) - sizeof(msg1.mtype), !IPC_NOWAIT);
                        if (msgid2 == -1)
                            cout << "error in sending messege to procces in kernel" << endl;
                    }
                    else
                    {
                        responce = "3";
                        strcpy(msg1.mtext, responce.c_str());
                        int msgid1 = msgsnd(down_id, &msg1, sizeof(msg1) - sizeof(msg1.mtype), !IPC_NOWAIT);
                    }
                }
            }
        };
        sleep(1);
        killpg(getpgrp(), SIGUSR2);
        sleep(1);
        cout << endl;
    }
}
/////////////////////////////////////////////////////////////////////////////////
void process(int num)
{
    signal(SIGUSR2, processHandler);
    string filename = "process" + to_string(num + 1) + ".txt";
    ifstream in_file;
    in_file.open(filename);
    int time_clk;
    string operation, data;
    while (in_file >> time_clk){
        in_file >> operation >> data;
        cout<<operation<<"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\n";
        
        while (time_clk > clk);
        msgbuff msg;
        msgbuff msg2;
        msg.mtype = getpid();
        if (operation == "ADD")
        {
            msg.operation = 'A';
            strcpy(msg.mtext, data.c_str());
            int msgid1 = msgsnd(up_id, &msg, sizeof(msg) - sizeof(msg.mtype), !IPC_NOWAIT);
            if (msgid1 == -1)
                cout << "error in sending messege to kernel" << endl;
            else
            {
                cout << "successful sending to kernel from process case Add" << endl;

                int msgid2 = -1;
                while(msgid2==-1){
                    cout<<"LOOPING\n";
                    msgid2 = msgrcv(down_id, &msg2, sizeof(msg2) - sizeof(msg2.mtype), getpid(), !IPC_NOWAIT);
                }
                //if (msgid2 == -1)
                //    cout << "error in recieve messege from kernel in process" << endl;
               // else
                //{
                if (msg2.mtext[0]=='0')
                    cout << "successful ADD  "<< i <<endl;
                else if (msg2.mtext[0]=='2')
                    cout << "unable to ADD  "<< i <<endl;
                //}
            }
        }
        else if (operation == "DEL")
        {
            msg.operation = 'D';
            stringstream str(data);
            str >> msg.slot;
            cout << msg.slot << endl;
            int msgid = msgsnd(up_id, &msg, sizeof(msg) - sizeof(msg.mtype), !IPC_NOWAIT);
            if (msgid == -1)
                cout << "error in send messege to kernel" << endl;
            else
            {
                cout << "successful sending to kernel from process case Delete" << endl;
                int msgid2 = msgrcv(down_id, &msg2, sizeof(msg2) - sizeof(msg2.mtype), getpid(), !IPC_NOWAIT);
                if (msgid2 == -1)
                    cout << "error in recieve messege from kernel" << endl;
                else
                {
                    if (!strcmp(msg2.mtext, "1"))
                        cout << "successful DEL"<< i<<endl;
                    else if (!strcmp(msg2.mtext,"3"))
                        cout << "unable to DEL"<< i<<endl;
                }
            }
        }
    }

        cout<<"CLLLLLLLLLLLLLLLLLLOOOOOOOOOOOOOOOOOOOOOOOOSSSSSSSSSSSSSSSSSSSEEEEEEEEEEEEEE\n";
    
    in_file.close();
    exit(0);
}
//////////////////////////////////////////////////////////////////////////////////////////////////
void disk()
{
    signal(SIGUSR1, diskHandler);
    signal(SIGUSR2, diskHandler);
    char data_slots[slots_num][64];
    
    while (1)
    {
        status_empty = slots_num;
        msgbuff msg;
        int msgid = msgrcv(down_id, &msg, sizeof(msg) - sizeof(msg.mtype), getpid(), !IPC_NOWAIT);
        if (msgid == -1)
            continue;
        if (msg.operation == 'A')
        {
            cout << "successful recieving msg from kernel in disk" << endl;

            int starting_time = clk;
            --status_empty;
            for (int i = 0; i < slots_num; i++)
            {
                if (!reserved[i])
                {
                    reserved[i] = true;
                    strcpy(data_slots[i], msg.mtext);
                    break;
                }
            }
            int c=0;
            for (int i=0;i<slots_num;i++)
            {
                if(!reserved[i])
                c++;
            }
            while (clk < starting_time + 3);
            cout << "disk add done at" + to_string(clk) << endl;
        }
        else if (msg.operation == 'D')
        {
            int starting_time = clk;
            ++status_empty;
            reserved[i] = true;
            while (clk < starting_time + 1);
            cout << "disk delete at" + to_string(clk) << endl;
        }
    }
}
//////////////////////////////////////////////////////////////////////
int main()
{
    int pid = 0;
    int child_ids[4];
    up_id = msgget(IPC_PRIVATE, IPC_CREAT | 0644);
    down_id = msgget(IPC_PRIVATE, IPC_CREAT | 0644);
    for (; i < 4; ++i)
    {
        pid = fork();
        if (!pid)
            break;

        child_ids[i] = pid;
    }
    if (pid)
    {
        kernel(child_ids);
    }
    else
    {
        if (i == 3)
            disk();
        else
            process(i);
    }
    return 0;
}
/////////////////////////////////////////////////////////////////////
void kernelHandler(int sig)
{
    signal(sig, kernelHandler);
    switch (sig)
    {
    case SIGINT:
        cout << "\nUp and Down Deleted and Kernel and all processes are\n";
        msgctl(up_id, IPC_RMID, (struct msqid_ds *)0);
        msgctl(down_id, IPC_RMID, (struct msqid_ds *)0);
        killpg(getpgrp(), SIGKILL);
        break;
    case SIGUSR2:
        ++clk;
        cout << "Kernel clk" << clk << endl;
        break;  
    }
}
/////////////////////////////////////////////////////////////////
void processHandler(int sig)
{
    signal(sig, processHandler);
    switch (sig)
    {
    case SIGUSR2:
        ++clk;
        cout << "Process" << i << " clk" << clk << endl;
        break;
    case SIGINT:
        cout << "I am process" << i << " and i am killed\n";
        exit(0);
        break;
    }
}
//////////////////////////////////////////////////////////////////
void diskHandler(int sig)
{
    signal(sig, diskHandler);
    switch (sig)
    {
    case SIGUSR2:
        ++clk;
        cout << "Disk clk" << clk << endl;
        break;
    case SIGUSR1:
        msgbuff msg;
        msg.slot = status_empty;
        msg.mtype = getpid();
        for(int i=0;i<10;++i)
            msg.mtext[i] = (reserved[i])? '1':'0';
        int msgid = msgsnd(up_id, &msg, sizeof(msg) - sizeof(msg.mtype), !IPC_NOWAIT);
        break;
        /*
        case SIGINT:
            cout<<"I am disk and i am killed\n";
            exit(0);
            break;*/
    }
}
/////////////////////////////////////////////////////////////////////