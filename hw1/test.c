#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

#define DEBUG 0
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
pthread_mutex_t kill_tube_mutex;
bool kill_tube = false;
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
    int c_cnt = 20,h_cnt = 20,o_cnt = 20,n_cnt = 20,gen_time = 100;
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
    
    char atom_test[30] = "OCHOOHNCHHOONCHOONHHONCOHCHHON";  

    //init tubes
    pthread_t tube_tid[3];
    for (int i = 0; i < 3; i++)
    {
        tube[i].tubeID = i+1;
        tube[i].tubeTS = 0;
        tube[i].firstSpilled = 'x';
        tube[i].moleculeType = 0;
    }
    
    pthread_create(&tube_tid[0], NULL, tube_thread, (void*)&tube[0]);
    pthread_create(&tube_tid[1], NULL, tube_thread, (void*)&tube[1]);
    pthread_create(&tube_tid[2], NULL, tube_thread, (void*)&tube[2]);
    
    srand(time(0));
    printf("GENERATION_RATE: %d\n",gen_time);
    pthread_t atom_tid;
    //create atoms
    while(c_cnt > 0 || h_cnt > 0 || o_cnt  > 0 || n_cnt > 0 )
    {
        char newAtom = selectAtom();
        
        //char newAtom = atom_test[total_atom];
        bool valid = false;
        if(newAtom == 'x'){
            printf("Invalid atom type\n");
        }
        else if(newAtom == 'C' && c_cnt > 0){
            c_cnt--;
            valid = true;
        }
        else if(newAtom == 'H' && h_cnt > 0){
            h_cnt--;
            valid = true;
        }
        else if(newAtom == 'O' && o_cnt > 0){
            o_cnt--;
            valid = true;
        }
        else if(newAtom == 'N' && n_cnt > 0){
            n_cnt--;
            valid = true;
        }
        if(valid == false) continue;

        atom_t * atom = malloc(sizeof(atom_t));
        atom->atomID = ++total_atom;
        atom->atomType = newAtom;
        printf("%c with ID:%d created.\n",atom->atomType, atom->atomID);
        pthread_create( &atom_tid, NULL, &atom_thread, (void*) atom);
        usleep(getSleepTime(gen_time) * 1e6);
        pthread_mutex_lock(&informataion_mutex);
        
        if(information.tubeID != 0)
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
            printf("%s is created in tube %d.\n", mol, information.tubeID);
            information.tubeID = 0;
        }
        pthread_mutex_unlock(&informataion_mutex);
    }
   
   return 0;

}

//return random sleep type
double getSleepTime(int lambda)
{
    return  -log(1.0 - (double)rand()/RAND_MAX) / lambda;
}

//return random atom type
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

//increase atom count in the tube struct according to the atom type
void increaseAtomCount_tube(char atom, tube_t * tube)
{
    switch (atom)
    {
    case 'C': tube->spilledAtomCnt.C_cnt++; break;
    case 'H': tube->spilledAtomCnt.H_cnt++; break;
    case 'O': tube->spilledAtomCnt.O_cnt++; break;
    case 'N': tube->spilledAtomCnt.N_cnt++; break;
    default: break;
    }
}

// find the proper tube to spill the atom
int findProperTube(char atom)
{
    int h_proper[2] = {NH3_MOLECULE_TYPE | (3 << 8), H20_MOLECULE_TYPE | 2 << 8};
    int c_proper[1] = {CO2_MOLECULE_TYPE | (1 << 8)};
    int o_proper[3] = {CO2_MOLECULE_TYPE , H20_MOLECULE_TYPE, NO2_MOLECULE_TYPE};
    int n_proper[2] = {NO2_MOLECULE_TYPE, NH3_MOLECULE_TYPE};

    int * properMoleculeArr;
    int length;
    switch (atom)
    {
        case 'C': properMoleculeArr = c_proper; length = sizeof(c_proper)/ sizeof(int); break;
        case 'H': properMoleculeArr = h_proper; length = sizeof(h_proper)/ sizeof(int); break;
        case 'O': properMoleculeArr = o_proper; length = sizeof(o_proper)/ sizeof(int); break;
        case 'N': properMoleculeArr = n_proper; length = sizeof(n_proper)/ sizeof(int); break;
        default: return -1; break;
    }
    
    int smallTimeStamp = 0x7fffffff; //max integer ts
    int smallerTsIndex = 0;
    bool foundProperCandidate = false;
    
    /* outer for loop for tubes*/
    for (int i = 0; i < 3; i++)
    {
        //inner for loop for proper molecule types
        for(int j = 0; j < length; j++)
        {
            //if tube's molecule type same with atom's molecule type that can react
            if(tube[i].moleculeType & (1 << properMoleculeArr[j]))
            {
                //tubes cannot include more than 1 N 
                if(atom == 'N' && tube[i].spilledAtomCnt.N_cnt == 1)
                    continue;
                //tubes cannnot inc. > 1C    
                if(atom == 'C' && tube[i].spilledAtomCnt.C_cnt == 1)
                    continue;
    
                if(atom == 'H'){
                    //tubes cant inc. more than 2H if mol type is h20
                    if(tube[i].moleculeType == (1 << H20_MOLECULE_TYPE) && tube[i].spilledAtomCnt.H_cnt == 2)
                        continue;
                    //tubes cant inc. more than 3H if mol type is nh3
                    else if(tube[i].moleculeType == (1 << NH3_MOLECULE_TYPE) && tube[i].spilledAtomCnt.H_cnt == 3)
                        continue;
                }
                if(atom == 'O'){
                    if(tube[i].moleculeType == (1 << CO2_MOLECULE_TYPE) && tube[i].spilledAtomCnt.O_cnt == 2)
                        continue;
                    else if(tube[i].moleculeType == (1 << H20_MOLECULE_TYPE) && tube[i].spilledAtomCnt.O_cnt == 1)
                        continue;
                    else if(tube[i].moleculeType == (1 << NO2_MOLECULE_TYPE) && tube[i].spilledAtomCnt.O_cnt == 2)
                        continue;
                }
                //find smaller timestamp from proper candidates
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
    
    //if proper candidate found return index
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

//atom thread function, determines place that atom will be spilled
void * atom_thread(void *args)
{
    atom_t* atom = (atom_t*) args;
    
    if(DEBUG) printf("%c %d in atom thread befre mutex\n", atom->atomType, atom->atomID);
    int mutVal = pthread_mutex_lock(&mutex);
    if(DEBUG) printf("mutVal %d mutex gained %c %d\n",mutVal, atom->atomType, atom->atomID);
    /* if all tubes are empty, spill to first one */
    if(tube[0].firstSpilled == 'x' && tube[1].firstSpilled == 'x' && tube[2].firstSpilled == 'x')
    {
        tube[0].firstSpilled = atom->atomType;
        tube[0].tubeTS = atom->atomID;
        increaseAtomCount_tube(atom->atomType, &tube[0]);
        if(DEBUG) printf("atom %c with id %d spilled to tube 0\n", atom->atomType, atom->atomID);       
    }
    else
    {
        int properTubeIndex = findProperTube(atom->atomType);
        if(properTubeIndex != -1){
            if(tube[properTubeIndex].firstSpilled =='x')
            {                    
                tube[properTubeIndex].firstSpilled = atom->atomType;
                tube[properTubeIndex].tubeTS = atom->atomID;
            }
            increaseAtomCount_tube(atom->atomType, &tube[properTubeIndex]);
            if(DEBUG) printf("atom %c with id %d spilled to tube%d \n", atom->atomType, atom->atomID, properTubeIndex);
            
        }
        else{
            printf("%c with ID:%d wasted\n", atom->atomType, atom->atomID);
        }
    }
    free(atom);
    //realloc(atom, 0); //free atom
    pthread_mutex_unlock(&mutex);
}

// checks if reaction is possible, if so return which molecule type will be reacted
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
        return NH3_MOLECULE_TYPE;
    }
    else if(tube->spilledAtomCnt.H_cnt == 2 && tube->spilledAtomCnt.O_cnt == 1)
    {   
        tube->firstSpilled = 'x';
        tube->spilledAtomCnt.H_cnt = 0;
        tube->spilledAtomCnt.O_cnt = 0;
        return H20_MOLECULE_TYPE;
    }
    else if (tube->spilledAtomCnt.N_cnt == 1 && tube->spilledAtomCnt.O_cnt == 2)
    {
        tube->firstSpilled = 'x';
        tube->spilledAtomCnt.N_cnt = 0;
        tube->spilledAtomCnt.O_cnt = 0;
        return NO2_MOLECULE_TYPE;
    }
    else{
        return -1;
    }  
}   

//tube thread function, determines the molecule type of the tube and checks reaction conditions.
void * tube_thread(void *arg)
{   
    tube_t* tube = (tube_t*) arg;

    while (1)
    {
        pthread_mutex_lock(&mutex);
        if(tube->spilledAtomCnt.O_cnt > 0)
        {
            tube->moleculeType = (1 << CO2_MOLECULE_TYPE) | (1 << NO2_MOLECULE_TYPE) | (1 << H20_MOLECULE_TYPE);
        }
        if(tube->spilledAtomCnt.N_cnt > 0)
        {
            tube->moleculeType = (1 << NO2_MOLECULE_TYPE) | (1 << NH3_MOLECULE_TYPE);
        }
        if(tube->spilledAtomCnt.H_cnt > 0)
        {
            tube->moleculeType = (1 << H20_MOLECULE_TYPE) | (1 << NH3_MOLECULE_TYPE);
        }
        if(tube->spilledAtomCnt.C_cnt > 0){
            tube->moleculeType = 1 << CO2_MOLECULE_TYPE;
        }
        else if(tube->spilledAtomCnt.N_cnt > 0 && tube->spilledAtomCnt.H_cnt > 0)
        {
            tube->moleculeType = 1 << NH3_MOLECULE_TYPE;
        }
        else if(tube->spilledAtomCnt.H_cnt > 0 && tube->spilledAtomCnt.O_cnt > 0)
        {
            tube->moleculeType = 1 << H20_MOLECULE_TYPE;
        }
        else if(tube->spilledAtomCnt.N_cnt > 0 && tube->spilledAtomCnt.O_cnt > 0)
        {
            tube->moleculeType = 1 << NO2_MOLECULE_TYPE;
        }
    
        int reactionType = checkReaction_tube(tube);
        if(reactionType != -1)
        {   
            //if reaction occured, modify information struct
            tube->moleculeType = 0;
            pthread_mutex_lock(&informataion_mutex);
            information.tube = *tube;
            information.tube.moleculeType = reactionType;
            information.tubeID = tube->tubeID;
            pthread_mutex_unlock(&informataion_mutex);    
        }

        pthread_mutex_unlock(&mutex);
    }

}