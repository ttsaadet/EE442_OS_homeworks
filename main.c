#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>


/*** STRUCT DECLERATIONS ****/ 
typedef struct
{
    int atomID;
    char atomType;
}atom_t;

typedef struct
{
    int tubeID;
    int tubeTS;
    int moleculeType; // 1:H2O, 2:CO 2 , 3:NO 2 , 4:NH3
    char firstSpilled;
    char *atoms;
}tube_t;


typedef struct 
{
    int tubeID;
    atom_t atom;
}information_t;

/********************************/

/* GLOBAL VARIABLES */
pthread_mutex_t mutex;
information_t information;
tube_t tube[3];
/********************/
double getSleepTime(int lambda);
char selectAtom();
void * tube_thread(void *arg);
void * atom_thread(void *args);


int main(int argc, char *argv[])
{
    int opt;
    int c_cnt,h_cnt,o_cnt,n_cnt,gen_time;
    int total_atom;
    /*
    -c: The total number of carbon atoms to be generated (default 20)
    -h: The total number of hydrogen atoms to be generated (default 20)
    -o: The total number of oxygen atoms to be generated (default 20)
    -n: The total number of nitrogen atoms to be generated (default 20)
    -g: The rate of gener{ation time (default 100)
    */
   while((opt = getopt(argc, argv, ":c:h:o:n:g")) != -1) 
    { 
        switch(opt) 
        { 
            case 'c': 
                c_cnt = atoi(optarg); break;
            case 'h':
                h_cnt = atoi(optarg); break;
            case 'o':
                o_cnt = atoi(optarg); break;  
            case 'n': 
                n_cnt = atoi(optarg); break; 
            case 'g':  
                gen_time = atoi(optarg); break;
        } 
    } //handle the options

    //init tubes
    pthread_t tube_tid;
    for (int i = 0; i < 3; i++)
    {
        tube[i].tubeID = i+1;
        tube[i].tubeTS = 0;
        tube[i].firstSpilled = 'x';
        tube[i].moleculeType = 0;
    }
    
    pthread_create(&tube_tid, NULL, tube_thread, (void*)&tube[0]);
    pthread_create(&tube_tid, NULL, tube_thread, (void*)&tube[1]);
    pthread_create(&tube_tid, NULL, tube_thread, (void*)&tube[2]);
    
    while (1)
    {
        /* code */
    }
    
    while(c_cnt | h_cnt | o_cnt | n_cnt )
    {
        char newAtom = selectAtom();
        if(newAtom == 'x'){
            printf("Invalid atom type\n");
            return 0;
        }
        else if(newAtom == 'c'){
            c_cnt--;
        }
        else if(newAtom == 'h'){
            h_cnt--;
        }
        else if(newAtom == 'o'){
            o_cnt--;
        }
        else if(newAtom == 'n'){
            n_cnt--;
        }
        pthread_t atomThread;
        atom_t atom;
        atom.atomID = total_atom;
        atom.atomType = newAtom;
        pthread_create(&atomThread, NULL, &atom_thread, (void*) &atom);
        usleep(getSleepTime(gen_time) * 1e6);
    }

}

double getSleepTime(int lambda)
{
    return  -log(1.0 - (double)rand()/RAND_MAX) / lambda;
}

char selectAtom(){
    int atom = (int)((float)rand() / (RAND_MAX * 4));
    switch (atom)
    {
    case 0: return 'c';break;
    case 1: return 'h';break;
    case 2: return 'o';break;
    case 3: return 'h';break;
    default: return 'x'; break;
    }
}   

void * atom_thread(void *args)
{
    atom_t* atom = (atom_t*) args;
    pthread_mutex_lock(&mutex);
    /* if all tubes are empty, spill to first one */
    if(tube[0].firstSpilled == 'x' && tube[1].firstSpilled == 'x' && tube[2].firstSpilled == 'x')
    {
        tube[0].firstSpilled = atom->atomType;
        tube[0].tubeTS = atom->atomID;
    }
    else
    {
        if(atom->atomType == 'c')
        
    }
    for (int i = 0; i < 3; i++)
    {
        if(atom->atomType == "c")
        {
            tube[0].moleculeType = 
        }
        else if(atom->atomType == "h")
        {

        }
        else if(atom->atomType == "o")
        {

        }
        else if(atom->atomType == "n")
        {

        }
    }
    
 
    
}
void * tube_thread(void *arg)
{   
    tube_t* tube = (tube_t*) arg;
    
    if(tube->moleculeType == 1)
    {
        printf("H2O\n");
    }
    else if(tube->moleculeType == 2)
    {
        printf("CO2\n");
    }
    else if(tube->moleculeType == 3)
    {
        printf("NO2\n");
    }
    else if(tube->moleculeType == 4)
    {
        printf("NH3\n");
    }
    
}