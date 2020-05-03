#include "ku_cfs.h"


int main(int argc, char* argv[])
{	
	//argument check
	exception_preprocess(argc, argv);	
	
	//no process or timeslice
	int sum = atoi(argv[1])+atoi(argv[2])+atoi(argv[3])+atoi(argv[4])+atoi(argv[5]);
	if(sum==0 || atoi(argv[6])==0)
		exit(0);

	//initialize Ready Queue
	mkReadyQueue();
	
	//make child process & push to Ready Queue
	mkProcess(argv);
	
	//timer def
	defTimer();
	
	//SIGALRM handler
	signal(SIGALRM, alrmhandler);

	//sleep 5 sec
	usleep(5*1000*1000);



	//start scheduling while timeslice numbers
	int timeSlice = atoi(argv[6]);
	parent = getpid();
	CFS(timeSlice);
	
	free_queue();
	
	return 0;
}



//free malloc (ready queue)
void free_queue(){
	
	struct NODE* fp = head->next;
	while(fp!=tail){
		struct NODE* temp = fp;
		fp = fp->next;
		free(temp);
	}

	free(head);
	free(tail);
}


//SIGNAL HANDLER: ALRM
void alrmhandler(int sig){
        kill(current_child->PCB.id, SIGSTOP);
}


//make Ready Queue(LinkedList)
void mkReadyQueue(){
        //ready Queue
        head = malloc(sizeof(struct NODE)); //head node(blank)
        tail = malloc(sizeof(struct NODE)); //tail node(blank)

        //initialize queue: head-tail
        head->before = head;
        head->next = tail;
        tail->before = head;
        tail->next = tail;
}


//make child processes by argv
void mkProcess(char* argv[]){
        int niceNum = 5; //the number nice value(-2~2)
        char iter = 'A'; //name iter
        double weights[5] = {(double)1/(1.25*1.25), (double)1/1.25, (double)1, (double)1.25, (double)1.25*1.25};
        struct NODE* prev = head;

        for(int i=0; i<niceNum; i++){
                int pcn = atoi(argv[i+1]); //the number of ith nice_values's processes
                for(int j=0; j<pcn; j++){
                        int fk = fork();
                        if(fk<0){ //exception
                                fprintf(stderr, "fork failed.\n");
                                exit(1);
                        }
                        else if(fk==0){ //child process

                                //execl child process
                                char ar[2];
                                ar[0] = iter;
                                ar[1] = '\0';
                                execl("./ku_app", "ku_app",  ar, NULL);
                        }
                        else{ //parent process

                                //make new node
                                struct NODE* new_node = malloc(sizeof(struct NODE));
                               	new_node->PCB.name = iter;
                                new_node->PCB.id = fk;
                                new_node->PCB.weight = weights[i];
                                new_node->PCB.vruntime = 0;

                                //link with prev/tail node
                                prev->next = new_node;
                                new_node->before = prev;
                                new_node->next = tail;
                                tail->before = new_node;
                                prev = new_node;
                        }
                        iter++;
                }//end of process num iter
        }//end of nice iter
}



//exception handling: main() arguments
void exception_preprocess(int argc, char* argv[]){

        //arguments number error
        if(argc!=7){
                //printf("ku_cfs: Wrong number of arguments. it's 7.\n");
                exit(1);
        }

        //arguments value
        for(int i=1; i<argc; i++){
                int ch = 0;
                while(argv[i][ch]!='\0'){
                        if(argv[i][ch]<'0' || argv[i][ch]>'9'){
                                //printf("Invalid arguments. it's non-negative integer.\n");
                                exit(1);
                        }
                        ch++;
                }
                if(atof(argv[i])-atoi(argv[i])!=0.0)
                {
                        //printf("ku_cfs: Invalid  arguments. argument is integer.\n");
                        exit(1);
                }

                if(atoi(argv[i])<0 && i<argc-1)
                {
                        //printf("ku_cfs: Invalid arguments.  the number of processes is non-negative.\n");
                        exit(1);
                }
                if(i==argc-1 && atoi(argv[i])<0)
                {
                         //printf("ku_cfs: Invalid arguments. the number of time slices to run is non-negative.\n");
                         exit(1);
                }

        }

}



//scheduling(cfs) for timeslice number
void CFS(int timeSlice){
	
	for(int i=0; i<timeSlice; i++)
        {
        	struct NODE* selected = head->next;
		current_child = selected;
		pop_process();

		//set timer (1 sec)
		setitimer(ITIMER_REAL, &tm, NULL);
		kill(current_child->PCB.id, SIGCONT);
	
		pause();
		
                //after timeslice
		current_child->PCB.vruntime += delta_exec*current_child->PCB.weight; //renew current child process's vruntime
		insert_process();
        }

	
}

void pop_process(){
	current_child->before->next = current_child->next; //cut link
	current_child->next->before = current_child->before; //cut link
}

//add current process to READY QUEUE by vruntime(ascending)
void insert_process(){
        struct NODE* locate = head->next; //inserting location
        while(locate!=tail &&  current_child->PCB.vruntime>=locate->PCB.vruntime){
		locate = locate->next;
        }
        current_child->before = locate->before;
        locate->before->next = current_child;
        current_child->next = locate;
        locate->before = current_child;
}



//timer value def
void defTimer(){
	tm.it_value.tv_sec = 1; //time slice = 1 sec
	tm.it_value.tv_usec = 0;
	delta_exec = tm.it_value.tv_sec; //delta_exec = time silce = 1 sec
}
