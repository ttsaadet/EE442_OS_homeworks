#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

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
pthread_mutex_t mutex;
pthread_mutex_t informataion_mutex;
information_t information;
tube_t tube[3];
/********************/
double getSleepTime(int lambda);
char selectAtom();
void * tube_thread(void *arg);
void * atom_thread(void *args);
int findProperTube(char atom);


int main(int argc, char **argv)
{
    int opt;
    int c_cnt = 0,h_cnt = 0,o_cnt = 0,n_cnt = 0,gen_time;
    int total_atom = 0;
    /*
    -c: The total number of carbon atoms to be generated (default 20)
    -h: The total number of hydrogen atoms to be generated (default 20)
    -o: The total number of oxygen atoms to be generated (default 20)
    -n: The total number of nitrogen atoms to be generated (default 20)
    -g: The rate of gener{ation time (default 100)
    */
   while((opt = getopt(argc, argv, "c:h:o:n:g:")) != -1) 
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
            default:
                printf("invalid option\n"); break;
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
    
    srand(time(0));
    //create atoms
    while(c_cnt > 0 || h_cnt > 0 || o_cnt  > 0 || n_cnt > 0 )
    {
        char newAtom = selectAtom();
        bool valid = false;
        if(newAtom == 'x'){
            printf("Invalid atom type\n");
        }
        else if(newAtom == 'c' && c_cnt > 0){
            c_cnt--;
            valid = true;
        }
        else if(newAtom == 'h' && h_cnt > 0){
            h_cnt--;
            valid = true;
        }
        else if(newAtom == 'o' && o_cnt > 0){
            o_cnt--;
            valid = true;
        }
        else if(newAtom == 'n' && n_cnt > 0){
            n_cnt--;
            valid = true;
        }
        if(valid == false) continue;
        pthread_t atomThread;
        atom_t atom;
        atom.atomID = total_atom++;
        atom.atomType = newAtom;
        printf("%c %d\n",atom.atomType, atom.atomID);
        pthread_create(&atomThread, NULL, &atom_thread, (void*) &atom);
        usleep(getSleepTime(gen_time) * 1e6);
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
    case 0: return 'c';break;
    case 1: return 'h';break;
    case 2: return 'o';break;
    case 3: return 'n';break;
    default: return 'x'; break;
    }
}   

void increaseAtomCount_tube(char atom, tube_t * tube)
{
    switch (atom)
    {
    case 'c': tube->spilledAtomCnt.C_cnt++; break;
    case 'h': tube->spilledAtomCnt.H_cnt++; break;
    case 'o': tube->spilledAtomCnt.O_cnt++; break;
    case 'n': tube->spilledAtomCnt.N_cnt++; break;
    default: break;
    }
}
int findProperTube(char atom)
{
    int h_proper[2] = {NH3_MOLECULE_TYPE, H20_MOLECULE_TYPE};
    int c_proper[1] = {CO2_MOLECULE_TYPE};
    int o_proper[3] = {CO2_MOLECULE_TYPE, H20_MOLECULE_TYPE, NO2_MOLECULE_TYPE};
    int n_proper[2] = {NO2_MOLECULE_TYPE, NH3_MOLECULE_TYPE};

    int * properMoleculeArr;
    int length;
    switch (atom)
    {
        case 'c': properMoleculeArr = c_proper; length = sizeof(c_proper)/ sizeof(int); break;
        case 'h': properMoleculeArr = h_proper; length = sizeof(h_proper)/ sizeof(int); break;
        case 'o': properMoleculeArr = o_proper; length = sizeof(o_proper)/ sizeof(int); break;
        case 'n': properMoleculeArr = n_proper; length = sizeof(n_proper)/ sizeof(int); break;
        default: return -1; break;
    }
    printf("proper tube function %c\n", atom);
    int smallTimeStamp = 0x7fffffff; //max integer ts
    int smallerTsIndex = 0;
    bool foundProperCandidate = false;
    
    /* outer for loop for tubes*/
    for (int i = 0; i < 3; i++)
    {
        //inner for loop for proper molecule types
        for(int j = 0; j < length; j++)
        {
            if(tube[i].moleculeType == properMoleculeArr[j])
            {
                if(tube[i].tubeTS < smallTimeStamp)
                {
                    smallTimeStamp = tube[i].tubeTS;
                    smallerTsIndex = i;
                    foundProperCandidate = true;
                }
                break;
            }
        }//inner foor loop end
    }//outer ffor loop end
    
    if(foundProperCandidate){
        return smallerTsIndex;
    }else{
        /*if not found proper, spill to first empty*/
        for (int i = 0; i < 3; i++)
        {
            if(tube[i].firstSpilled == 'x')
            {
                return i;
            }
        }
    }

    //if there is no proper tube and empty tube, waste it
    return -1;
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
        increaseAtomCount_tube(atom->atomType, &tube[0]);
        printf("%d id %c added to tube 0\n", atom->atomID, atom->atomType); // debug

    }
    else
    {
        int properTubeIndex = findProperTube(atom->atomType);
        printf("%d id %c added to tube %d\n", atom->atomID, atom->atomType, properTubeIndex); // debug
        if(properTubeIndex != -1){
            if(tube[properTubeIndex].firstSpilled =='x')
            {
                tube[properTubeIndex].firstSpilled = atom->atomType;
                tube[properTubeIndex].tubeTS = atom->atomID;
            }
            increaseAtomCount_tube(atom->atomType, &tube[properTubeIndex]);
        }
        else{
            printf("%c with ID:%d wasted\n", atom->atomType, atom->atomID);
        }
    }
    pthread_mutex_unlock(&mutex);
}


int checkReaction_tube(tube_t *tube)
{
    if(tube->spilledAtomCnt.C_cnt == 1 && tube->spilledAtomCnt.O_cnt == 2 )
    {
        tube->firstSpilled = 'x';
        tube->spilledAtomCnt.C_cnt = 0;
        tube->spilledAtomCnt.O_cnt = 0;
        return CO2_MOLECULE_TYPE;
    }
    else if(tube->spilledAtomCnt.N_cnt == 1 && tube->spilledAtomCnt.H_cnt == 3)
    {   tube->firstSpilled = 'x';
        tube->spilledAtomCnt.N_cnt = 0;
        tube->spilledAtomCnt.H_cnt = 0;
        return NO2_MOLECULE_TYPE;
    }
    else if(tube->spilledAtomCnt.H_cnt == 2 && tube->spilledAtomCnt.O_cnt == 1)
    {   
        tube->firstSpilled = 'x';
        tube->spilledAtomCnt.H_cnt = 0;
        tube->spilledAtomCnt.O_cnt = 0;
        return H20_MOLECULE_TYPE;
    }
    else if (tube->spilledAtomCnt.N_cnt == 2 && tube->spilledAtomCnt.O_cnt == 1)
    {
        tube->firstSpilled = 'x';
        tube->spilledAtomCnt.N_cnt = 0;
        tube->spilledAtomCnt.O_cnt = 0;
        return NH3_MOLECULE_TYPE;
    }
    else{
        return -1;
    }  
}    
void * tube_thread(void *arg)
{   
    tube_t* tube = (tube_t*) arg;

    while (true)
    {
        pthread_mutex_lock(&mutex);
        if(tube->spilledAtomCnt.C_cnt > 0){
            printf("%d C atoms in tube %d\n", tube->spilledAtomCnt.C_cnt, tube->tubeID);
            tube->moleculeType = CO2_MOLECULE_TYPE;
        }
        else if(tube->spilledAtomCnt.N_cnt > 0 && tube->spilledAtomCnt.H_cnt > 0)
        {
             printf("%d C atoms in tube %d\n", tube->spilledAtomCnt.H_cnt, tube->tubeID); 
            tube->moleculeType = NH3_MOLECULE_TYPE;
        }
        else if(tube->spilledAtomCnt.H_cnt > 0 && tube->spilledAtomCnt.O_cnt > 0)
        {
            tube->moleculeType = H20_MOLECULE_TYPE;
        }
        else if(tube->spilledAtomCnt.N_cnt > 0 && tube->spilledAtomCnt.O_cnt > 0)
        {
            tube->moleculeType = NO2_MOLECULE_TYPE;
        }
        else{
            //-1 means no molecule type
            tube->moleculeType = -1;
        }
        int reactionType = checkReaction_tube(tube);
        
        if(reactionType != -1)
        {
            pthread_mutex_lock(&informataion_mutex);
            information.tube = *tube;
            information.tubeID = tube->tubeID;
            pthread_mutex_unlock(&informataion_mutex);    
        }
        pthread_mutex_unlock(&mutex);
    }

}