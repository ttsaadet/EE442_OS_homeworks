#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <semaphore.h>

/** DEFINE */
#define H20_MOLECULE_TYPE 1
#define CO2_MOLECULE_TYPE 2
#define NO2_MOLECULE_TYPE 3
#define NH3_MOLECULE_TYPE 4

/*** STRUCT DECLERATIONS ****/ 
typedef struct
{
    int C_cnt;
    int H_cnt;
    int O_cnt;
    int N_cnt;
}spilledAtoms_t;

typedef struct
{
    int atomID;
    char atomType;
}atom_t;

typedef struct
{
    int tubeID;
    int tubeTS;
    int moleculeType; // 1:H2O, 2:CO2 , 3:NO 2 , 4:NH3
    char firstSpilled;
    spilledAtoms_t spilledAtomCnt;
}tube_t;


typedef struct 
{
    int tubeID;
    tube_t tube;
}information_t;

/********************************/

/* GLOBAL VARIABLES */
information_t information;
sem_t sem_O;
sem_t sem_N;
sem_t sem_C;
sem_t sem_H;
sem_t sem_H2O;
sem_t sem_CO2;
sem_t sem_NO2;
sem_t sem_NH3;
sem_t sem_info;
sem_t sem_atom_sync;
int atomID;
/********************/
double getSleepTime(int lambda);
char selectAtom();


void * produce_C(void *arg);
void * produce_H(void *arg);
void * produce_O(void *arg);
void * produce_N(void *arg);

void * compose_H2O();
void * compose_CO2();
void * compose_NO2();
void * compose_NH3();


int main(int argc, char **argv)
{
    int opt;
    int m_cnt = 40 ,c_cnt = 0,h_cnt = 0,o_cnt = 0,n_cnt = 0,gen_time = 100;
    int total_atom = 0;
    
    sem_init(&sem_C, 0, 0);
    sem_init(&sem_H, 0, 0);
    sem_init(&sem_O, 0, 0);
    sem_init(&sem_N, 0, 0);
    sem_init(&sem_H2O, 0, 0);
    sem_init(&sem_CO2, 0, 0);
    sem_init(&sem_NO2, 0, 0);
    sem_init(&sem_NH3, 0, 0);
    sem_init(&sem_info, 0, 1);
    sem_init(&sem_atom_sync, 0, 1);
    /*
    -c: The total number of carbon atoms to be generated (default 20)
    -h: The total number of hydrogen atoms to be generated (default 20)
    -o: The total number of oxygen atoms to be generated (default 20)
    -n: The total number of nitrogen atoms to be generated (default 20)
    -g: The rate of gener{ation time (default 100)
    */
   
   while((opt = getopt(argc, argv, "m:g:")) != -1) 
    { 
        switch(opt) 
        { 
            case 'm':
                m_cnt = atoi(optarg); break;
            case 'g':  
                gen_time = atoi(optarg); break;
            default:
                printf("invalid option\n"); break;
        } 
    } //handle the options
    

    n_cnt =m_cnt/ 4;
    c_cnt =m_cnt /4;
    h_cnt =m_cnt /4;
    o_cnt = m_cnt /4 ;
    //init compoosers
    pthread_t compose_tid;

    srand(time(0));
    printf("GENERATION_RATE: %d\n",gen_time);
  
  
    pthread_t atom_tid;

    pthread_create(&compose_tid, NULL, &compose_CO2, NULL);
    pthread_create(&compose_tid, NULL, &compose_H2O, NULL);
    pthread_create(&compose_tid, NULL, &compose_NH3, NULL);
    pthread_create(&compose_tid, NULL, &compose_NO2, NULL);

    int atom_id[10000];
    while(c_cnt > 0 || h_cnt > 0 || o_cnt  > 0 || n_cnt > 0 )
    {
        char newAtom = selectAtom();
        //char newAtom = atom_arr[total_atom];
        bool valid = false;
        if(newAtom == 'x'){
            printf("Invalid atom type\n");
            continue;
        }
        
        atom_id[total_atom] = total_atom + 1;
       
        if(newAtom == 'C' && c_cnt > 0){
            c_cnt--;
            valid = true;
            pthread_create(&atom_tid, NULL, &produce_C, (void*)&atom_id[total_atom]);
        }
        else if(newAtom == 'H' && h_cnt > 0){
            h_cnt--;
            valid = true;
            pthread_create(&atom_tid, NULL, &produce_H, (void*)&atom_id[total_atom]);
        }
        else if(newAtom == 'O' && o_cnt > 0){
            o_cnt--;
            valid = true;
            pthread_create(&atom_tid, NULL, &produce_O, (void*)&atom_id[total_atom]);

        }
        else if(newAtom == 'N' && n_cnt > 0){
            n_cnt--;
            valid = true;
            pthread_create(&atom_tid, NULL, &produce_N, (void*)&atom_id[total_atom]);

        }
        total_atom++;
        if(valid == false) continue;
        usleep(getSleepTime(gen_time) * 1e6);
        
        sem_wait(&sem_info);
        if(information.tube.moleculeType != 0)
        {   
            int molType = information.tube.moleculeType;
            char * mol;
            switch (molType)
            {
            case H20_MOLECULE_TYPE: mol = "H2O"; break;
            case CO2_MOLECULE_TYPE: mol = "CO2"; break;
            case NO2_MOLECULE_TYPE: mol = "NO2"; break;
            case NH3_MOLECULE_TYPE: mol = "NH3"; break;
            default: mol = "inv";break;
            }
            printf("%s is generated.\n",mol );
            information.tube.moleculeType = 0;
        }
        sem_post(&sem_info);
       
    }
}

double getSleepTime(int lambda)
{
    return  -log(1.0 - (double)rand()/RAND_MAX) / lambda;
}

char selectAtom(){
    
    int atom =  rand() % 4;
    switch (atom)
    {
    case 0: return 'C';break;
    case 1: return 'H';break;
    case 2: return 'O';break;
    case 3: return 'N';break;
    default: return 'x'; break;
    }
} 

void * produce_C(void *arg)
{
    
    sem_wait(&sem_atom_sync);
    
    printf("C with ID:%d is created.\n", atomID++);
    sem_post(&sem_C);
    sem_post(&sem_atom_sync);
    
}


void * produce_H(void *arg)
{
    //int atomID = *(int*)arg;
    sem_wait(&sem_atom_sync);
    atomID++;
    printf("H with ID:%d is created.\n", atomID+;
    sem_post(&sem_H);
    sem_post(&sem_atom_sync);
    
}   

void * produce_O(void *arg)
{
    //int atomID = *(int*)arg;
    sem_wait(&sem_atom_sync);
    printf("O with ID:%d is created.\n", atomID++);
    sem_post(&sem_O);
    sem_post(&sem_atom_sync);
    
}


void * produce_N(void *arg)
{
    //int atomID = *(int*)arg;
    sem_wait(&sem_atom_sync);
    printf("N with ID:%d is created.\n", atomID++);
    sem_post(&sem_N);
    sem_post(&sem_atom_sync);
    
}

void * compose_CO2()
{
    
    while(1)
    {
        int C_cnt = 0;
        int O_cnt = 0;
        sem_getvalue(&sem_C, &C_cnt);
        sem_getvalue(&sem_O, &O_cnt);
        if(C_cnt >= 1 && O_cnt >= 2)
        {
            sem_wait(&sem_C);
            sem_wait(&sem_O);
            sem_wait(&sem_O);
            sem_wait(&sem_info);
            information.tube.moleculeType = CO2_MOLECULE_TYPE;
            sem_post(&sem_info);
        }
    }
}
    

void * compose_H2O()
{
    while(1)
    {
        int H_cnt;
        int O_cnt;
        sem_getvalue(&sem_H, &H_cnt);
        sem_getvalue(&sem_O, &O_cnt);

        if(H_cnt >= 2 && O_cnt >= 1)
        {
            sem_wait(&sem_H);
            sem_wait(&sem_H);
            sem_wait(&sem_O);
            sem_post(&sem_H2O);
            sem_wait(&sem_info);
            information.tube.moleculeType = H20_MOLECULE_TYPE;
            sem_post(&sem_info);
        }
    }
}

void * compose_NH3()
{
    while(1)
    {
        int N_cnt = 0;
        int H_cnt = 0;
        sem_getvalue(&sem_N, &N_cnt);
        sem_getvalue(&sem_H, &H_cnt);
        if(N_cnt >= 1 && H_cnt >= 3)
        {
            sem_wait(&sem_N);
            sem_wait(&sem_H);
            sem_wait(&sem_H);
            sem_wait(&sem_H);
            sem_post(&sem_NH3);
            sem_wait(&sem_info);
            information.tube.moleculeType = NH3_MOLECULE_TYPE;
            sem_post(&sem_info);
        }
    }
}

void * compose_NO2()
{
    while(1)
    {
        int N_cnt = 0;
        int O_cnt = 0;
        sem_getvalue(&sem_N, &N_cnt);
        sem_getvalue(&sem_O, &O_cnt);
        if(N_cnt >= 1 && O_cnt >= 2)
        {
            sem_wait(&sem_N);
            sem_wait(&sem_O);
            sem_wait(&sem_O);
            sem_post(&sem_NO2);
            sem_wait(&sem_info);
            information.tube.moleculeType = NO2_MOLECULE_TYPE;
            sem_post(&sem_info);
        }
    }
}
