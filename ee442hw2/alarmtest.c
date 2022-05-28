#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
void sig_hanlder(){
    printf("in test\n");
    signal(SIGALRM, sig_hanlder);
    alarm(2);
    
}

int main(){
    typedef struct 
    {
        char string[60];
        int pos;
    }threadStr_t;
    
    threadStr_t runningStr = {"running>", strlen(runningStr.string)};
    printf("%d", runningStr.pos);
    char readyQueueStr[60] = "MYVAL";
    int pos = strlen(readyQueueStr);
    int k;
    int n = 5;

    int readyThreadCount = 3;
    int readyThreadsIndexArr[3] = {1,2,3};
    int nextThreadIndex = 2;
       
    for(int i = 0; i<readyThreadCount; i++)
    {
        if(readyThreadsIndexArr[i] != nextThreadIndex)
        {
            pos += (i != readyThreadCount -1 ) ? sprintf(&readyQueueStr[pos], "val%d,", readyThreadsIndexArr[i])
                : sprintf(&readyQueueStr[pos], "val%d", readyThreadsIndexArr[i]); 
        }
    }
    printf("%s\n", readyQueueStr);
}