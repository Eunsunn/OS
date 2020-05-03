#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <unistd.h>


//process info
struct pcs{
        char name; //alphabet
	double weight; //weight (by nice value) 
	double vruntime;
	pid_t id; //process id
};

//ready queue instance
struct NODE{
	struct NODE* before;
	struct NODE* next; //pointer of next node
	struct pcs PCB; //process info(child process)
};



static pid_t parent; //parent pid(OS process)
static struct NODE*  current_child; //current running child pid(by scheduling)
static struct NODE* head; //ready Queue's head
static struct NODE* tail; //ready Queue's tail
static struct itimerval tm; //timer
static int delta_exec;


void exception_preprocess(int argc, char* argv[]); //exception handling: main()'s arguments
void mkReadyQueue(); //initialize ready queue
void mkProcess(char* argv[]); //make child process & push to ready queue
void defTimer(); //timer definition
void alrmhandler(int sig); //SIGALRM handler
void pop_process();
void insert_process();
void CFS(int timeSlice);
void free_queue(); //free malloc (ready queue)
