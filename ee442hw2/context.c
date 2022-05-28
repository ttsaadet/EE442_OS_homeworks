#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdarg.h>


#define STACK_SIZE 4096
static int critical = 0;
ucontext_t c1, c2, c3;
int thread =0;
int finish1 = 0;
void func1(int arg) { 
     int i = 0;
     printf("passade arg. %d\n",arg);
    printf("func1 start\n");
    while (i <50)
    {

        critical = 1;
        printf("In func1: %d %d\n", i++, thread);
        if(thread == 1){
            getcontext(&c2);
        }
        else{
            getcontext(&c1);
        }
        usleep(3e4);
        critical = 0;
    }
    printf("func1 end\n");
    finish1++;
}


void sig_handler()

{
    if(finish1 == 2)return;
    signal(SIGALRM, sig_handler);
    alarm(1);
   
    static int i = 0;
    if(thread == 0)
    {
        thread = 1;

        setcontext(&c2);
        printf("x");
    }
    else{
        thread = 0;
        //getcontext(&c1);
        printf("sitch to c1\n");
        setcontext(&c1);
    } 

    
}

int main()
{   
    
    
   
 

    int argument = 442;
    int arg2 = 1;
    
    getcontext(&c1);
    c1.uc_link = &c3;
    c1.uc_stack.ss_sp = malloc(STACK_SIZE);
    c1.uc_stack.ss_size = STACK_SIZE;
    makecontext(&c1, (void (*)(void))func1,1, argument);
    
    getcontext(&c2);
    c2.uc_link = &c3;
    c2.uc_stack.ss_sp = malloc(STACK_SIZE);
    c2.uc_stack.ss_size = STACK_SIZE;
    makecontext(&c2, (void (*)(void))func1,1,arg2);

    //getcontext(&c3);
     
    
    getcontext(&c3);
    c3.uc_stack.ss_sp = malloc(STACK_SIZE);
    c3.uc_stack.ss_size = STACK_SIZE;
    sig_handler();

    //swapcontext(&c3, &c1);
    //printf("reutnr c1\n");
    
    //signal(SIGALRM, sig_handler);
    //alarm(2);
    
    
    while(1);
    swapcontext(&c3, &c1);
    sleep(1);
    printf("Switching to thread 2\n");
    swapcontext(&c3, &c2);
    printf("Exiting\n");
    free(c1.uc_stack.ss_sp);
    free(c2.uc_stack.ss_sp);
    
    return 0;
}