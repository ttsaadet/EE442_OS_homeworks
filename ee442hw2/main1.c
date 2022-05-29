#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

/*DEFINES*/
#define CPU_BURST_STOP_IND 2
#define IO_BURST_STOP_IND 2
#define TIMER_INTERVAL 1
#define STACK_SIZE 8192

/*TYPEDEF DECLERATIONS*/
typedef enum
{
    READY,
    RUNNING,
    FINISHED,
    IO,
    EMPTY
} ThreadStatus_t;

typedef struct
{
    ucontext_t context;
    int state;
    int ioburst_time;
    int ioburst[3];
    int cpuburst_time;
    int cpuburst[3];
} ThreadInfo_t;

/*GLOBAL DECLERATIONS */
static int critical = 0;
ThreadInfo_t threadArray[6];
int counter = 0;

/* function declerations */
void initializeThread();
void createThread(int threadIndex, int n);
void runThread();
void exitThread();
int scheduleThread();
void io_work();
void thread_func(int n);

int main()
{

    srand(time(0));
    
    initializeThread();
    for (int i = 1; i < 6; i++)
    {
        createThread(i, 6);
    }

    signal(SIGALRM, runThread);
    alarm(TIMER_INTERVAL);
    getcontext(&threadArray[0].context);
    while(1);
    return 0;
}

void initializeThread()
{

    for (int i = 0; i < 6; i++)
    {
        threadArray[i].state = EMPTY;

        threadArray[i].cpuburst_time = 0;
        threadArray[i].ioburst_time = 0;
        // TODO: TAKE INPUT FROM USER FOR BURST COUTS
        threadArray[i].cpuburst[0] = 2;
        threadArray[i].ioburst[0] = 2;
        threadArray[i].cpuburst[1] = 2;
        threadArray[i].ioburst[1] = 2;

        threadArray[i].cpuburst[2] = 2;
        threadArray[i].ioburst[2] = 2;
    }
    printf("T1\tT2\tT3\tT4\tT5\n");
}

void createThread(int threadIndex, int n)
{
    enum
    {
        MAIN_CONTEXT
    };
    getcontext(&threadArray[threadIndex].context);
    threadArray[threadIndex].context.uc_link = &threadArray[0].context;
    threadArray[threadIndex].context.uc_link = &threadArray[MAIN_CONTEXT].context;
    threadArray[threadIndex].context.uc_stack.ss_sp = malloc(STACK_SIZE);
    threadArray[threadIndex].context.uc_stack.ss_size = STACK_SIZE;
    threadArray[threadIndex].state = READY;
    makecontext(&threadArray[threadIndex].context, (void (*)(void))thread_func, 1, n);
}

int scheduleThread()
{
    int readyThreadCount = 0;
    int readyThreadsIndexArr[5];

    for (int i = 1; i < 6; i++)
    {
        if (threadArray[i].state == READY)
        {
            readyThreadsIndexArr[readyThreadCount++] = i;
        }
        else if (threadArray[i].state == RUNNING)
        {
            threadArray[i].state = READY;
        }
    }
    if(readyThreadCount == 0) 
        return -1;
    int nextThreadIndex = rand() % readyThreadCount;
    nextThreadIndex = readyThreadsIndexArr[nextThreadIndex];
    threadArray[nextThreadIndex].state = RUNNING;
    
    return nextThreadIndex;
}

void log_thread_status()
{
    FILE *logfd = fopen("log.txt", "a");
    for (int i = 1; i < 6; i++)
    {
        fprintf(logfd, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", i, threadArray[i].state, threadArray[i].cpuburst_time, threadArray[i].ioburst_time, threadArray[i].cpuburst[0],
                threadArray[i].cpuburst[1],
                threadArray[i].cpuburst[2],
                threadArray[i].ioburst[0],
                threadArray[i].ioburst[1],
                threadArray[i].ioburst[2]);
    }
    fprintf(logfd, "***********************\n");
    fclose(logfd);
}

void printThreadsStatus()
{
    log_thread_status();
    typedef struct
    {
        char string[60];
        int pos;
    } threadStr_t;

    threadStr_t runningStr = {"running>", strlen(runningStr.string)};
    threadStr_t readyStr = {"ready>", strlen(readyStr.string)};
    threadStr_t finishedStr = {"finished>", strlen(finishedStr.string)};
    threadStr_t ioStr = {"IO>", strlen(ioStr.string)};

    for (int i = 1; i < 6; i++)
    {
        switch (threadArray[i].state)
        {
        case READY:
            readyStr.pos += sprintf(&readyStr.string[readyStr.pos], "T%d,", i);
            break;
        case RUNNING:
            runningStr.pos += sprintf(&runningStr.string[runningStr.pos], "T%d,", i);
            break;
        case FINISHED:
            finishedStr.pos += sprintf(&finishedStr.string[finishedStr.pos], "T%d,", i);
            break;
        case IO:
            ioStr.pos += sprintf(&ioStr.string[ioStr.pos], "T%d,", i);
            break;
        }
    }
    // remove commas end of string
    if (readyStr.string[readyStr.pos - 1] == ',')
        readyStr.string[readyStr.pos - 1] = '\0';
    if (runningStr.string[runningStr.pos - 1] == ',')
        runningStr.string[runningStr.pos - 1] = '\0';
    if (finishedStr.string[finishedStr.pos - 1] == ',')
        finishedStr.string[finishedStr.pos - 1] = '\0';
    if (ioStr.string[ioStr.pos - 1] == ',')
        ioStr.string[ioStr.pos - 1] = '\0';

    printf("%s %s %s %s\n", runningStr.string, readyStr.string, finishedStr.string, ioStr.string);
}

void runThread()
{
    //getcontext(&threadArray[0].context);
    for(int i = 1; i <6 ; i++){
        if(threadArray[i].state != FINISHED){
            signal(SIGALRM, runThread);
            alarm(TIMER_INTERVAL);
            break;
        }
    }
        
    io_work();

    int nextThreadIndex = scheduleThread();
    printThreadsStatus();
    if(nextThreadIndex != -1 ){    
        
        setcontext(&threadArray[nextThreadIndex].context);
    }
    else{
        setcontext(&threadArray[0].context);
    }
}

void exitThread(int thread_id)
{
    free(threadArray[thread_id].context.uc_stack.ss_sp);   
}

void thread_func(int n)
{
    int i = 0;
    int threadIndex = 0;
    for (int j = 1; j < 6; j++)
    {
        if (threadArray[j].state == RUNNING)
        {
            threadIndex = j;
            break;
        }
    }
    
    while (i < n)
    {
        usleep(495000);
        int bursttime = threadArray[threadIndex].cpuburst_time;
        threadArray[threadIndex].cpuburst[bursttime] = threadArray[threadIndex].cpuburst[bursttime] - 1;
        if (threadArray[threadIndex].cpuburst[bursttime] == 0)
        {
            threadArray[threadIndex].state = IO;
            threadArray[threadIndex].cpuburst_time++;
        }
        switch (threadIndex)
        {
        case 1: break;
        case 2: printf("\t");break;
        case 3:printf("\t\t");break;
        case 4:printf("\t\t\t");break;
        case 5:printf("\t\t\t\t"); break;
        default:break;
        }
        printf("%d\n", ++i);
        getcontext(&threadArray[threadIndex].context);
      
    }
    exitThread(threadIndex);
}

void io_work()
{
    for (int i = 1; i < 6; i++)
    {
        if (threadArray[i].state == IO)
        {
            int bursttime = threadArray[i].ioburst_time;
            threadArray[i].ioburst[bursttime]--;
            if (threadArray[i].ioburst[bursttime] <= 0)
            {
                if (threadArray[i].ioburst_time == IO_BURST_STOP_IND)
                {
                    threadArray[i].state = FINISHED;
                }
                else
                {
                    threadArray[i].state = READY;
                    threadArray[i].ioburst_time++;
                }
            }
        }
    }
}
