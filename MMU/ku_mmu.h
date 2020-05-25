//pte: 1byte PTE(PDE)
struct ku_pte{
	char pte_val;
};
//PCB struct Definition
struct PCB{
        char pid;
        struct ku_pte* PDBR; //page directory base register
	struct PCB* prevPCB;
        struct PCB* nextPCB;
};
//Page Frame info (in FIFO list)
struct pf_info{
	char pid;
	struct ku_pte* PDBR; 
	//char* page_pa; //PF physical address
	//char* pte_pa; //PTE physical address
	//int PFN; //page's PFN
	char va;
	struct pf_info* prev_info;
	struct pf_info* next_info;
};



static char* ku_mmu_pmem; //start address of allocated physical memory (included)
static char* ku_mmu_sw; //start address of allocated swap space (included)

static struct PCB* ku_mmu_phead; //process PCB head
static struct PCB* ku_mmu_ptail; //process PCB tail

static unsigned int ku_mmu_psize=4; //page size:4Bytes
static unsigned int ku_mmu_pmem_pf_num; //the number of page in physical memory
static unsigned int ku_mmu_swap_pf_num; //the number of page in swap space

static char* ku_mmu_pmem_free_list; //free list of physical memeory (page frame unit) (0: not used, other: used) 
static char* ku_mmu_swap_free_list; //free list of swap space(page frame unit)

static struct pf_info* ku_mmu_pf_head; //page Frame info's head (used in Swapping)
static struct pf_info* ku_mmu_pf_tail; //page Frame info's tail (used in Swapping)

int ku_traverse(void *, char, void *);




int ku_page_fault(char pid, char va);
void *ku_mmu_init(unsigned int mem_size, unsigned int swap_size);
int ku_run_proc(char pid, struct ku_pte** ku_cr3);
struct PCB* findProcess(char pid);
int findPM(int isPM);
int swapout(int SW);
int swapin(struct ku_pte* pte_pointer);
struct ku_pte* findEntry(int degree, int va, struct ku_pte* PDBR, int* PFN);
int allocatePF();




void *ku_mmu_init(unsigned int mem_size, unsigned int swap_size){
	
	//process list(linked list) initialize
	if(!ku_mmu_phead){
		free(ku_mmu_phead);
		ku_mmu_phead = NULL;
	}
	if(!ku_mmu_ptail){
		free(ku_mmu_ptail);
		ku_mmu_ptail = NULL;
	}
	ku_mmu_phead = malloc(sizeof(struct PCB));
	ku_mmu_ptail = malloc(sizeof(struct PCB));
	ku_mmu_phead->prevPCB = ku_mmu_phead;
	ku_mmu_phead->nextPCB = ku_mmu_ptail;
	ku_mmu_ptail->prevPCB = ku_mmu_phead;
	ku_mmu_ptail->nextPCB = ku_mmu_ptail;

	//Free List initialize
	ku_mmu_pmem_pf_num = mem_size/ku_mmu_psize;
        ku_mmu_swap_pf_num = swap_size/ku_mmu_psize;
	if(!ku_mmu_pmem_free_list){
		free(ku_mmu_pmem_free_list);
		ku_mmu_pmem_free_list = NULL;
	}
	if(!ku_mmu_swap_free_list){
		free(ku_mmu_swap_free_list);
		ku_mmu_swap_free_list = NULL;
	}
	ku_mmu_pmem_free_list = malloc(sizeof(char)*ku_mmu_pmem_pf_num*ku_mmu_psize);
	ku_mmu_swap_free_list = malloc(sizeof(char)*ku_mmu_swap_pf_num*ku_mmu_psize);
	ku_mmu_pmem_free_list[0] = 1; //0th page frame of PMEM(OS) and SWAP SPACE is not used
	ku_mmu_swap_free_list[0] = 1;
	for(int i=1; i<ku_mmu_pmem_pf_num; i++)
		ku_mmu_pmem_free_list[i] = 0; //0: not used (free page frame)
	for(int i=1; i<ku_mmu_swap_pf_num; i++)
		ku_mmu_swap_free_list[i] = 0;

	//PF List(used in swap out)
	if(!ku_mmu_pf_head){
		free(ku_mmu_pf_head);
		ku_mmu_pf_head = NULL;
	}
	if(!ku_mmu_pf_tail){
		free(ku_mmu_pf_tail);
		ku_mmu_pf_tail = NULL;
	}
	ku_mmu_pf_head = malloc(sizeof(struct pf_info));
	ku_mmu_pf_tail = malloc(sizeof(struct pf_info));
	ku_mmu_pf_head->prev_info = ku_mmu_pf_head;
	ku_mmu_pf_head->next_info = ku_mmu_pf_tail;
	ku_mmu_pf_tail->prev_info = ku_mmu_pf_head;
	ku_mmu_pf_tail->next_info = ku_mmu_pf_tail;

	//allocate Physical Memory & Swap Space (continuously)
        unsigned int pf_num = ku_mmu_pmem_pf_num + ku_mmu_swap_pf_num;
        char* ret = (char*)malloc(sizeof(char)*pf_num*ku_mmu_psize);
	
	//FAIL: fail allocate
	if(!ret)
		return 0;

	//start&end address of Physical Memory & Swap Space
	ku_mmu_pmem = malloc(sizeof(char));
	ku_mmu_sw = malloc(sizeof(char));
	ku_mmu_pmem = ret;
	ku_mmu_sw = ret+(ku_mmu_pmem_pf_num*ku_mmu_psize);
	

	return ret;
}




int ku_run_proc(char pid, struct ku_pte** ku_cr3){
		
	//Check if process=(pid) exists
	struct PCB* ptarget = findProcess(pid);
	
	//Process=(pid) doesn't exist: create process
	if(!ptarget){
		struct PCB* newProcess = malloc(sizeof(struct PCB));
		
		//allocate new PD(page directory)
		int freePM = allocatePF();
		if(freePM == -1)
			return -1;

		//initialized PDBR
		char* PDBR_addr = ku_mmu_pmem + freePM*ku_mmu_psize;
		newProcess->PDBR = PDBR_addr;
		//initialize PD: unmapped
		for(int i=0; i<4; i++)
			*(PDBR_addr+i) = 0;
	
		//initialize process information
		newProcess->pid = pid;
                newProcess->prevPCB = malloc(sizeof(struct PCB));
		newProcess->nextPCB = malloc(sizeof(struct PCB));
		newProcess->prevPCB = ku_mmu_ptail->prevPCB;
                newProcess->nextPCB = ku_mmu_ptail;
		ku_mmu_ptail->prevPCB->nextPCB = newProcess;
                ku_mmu_ptail->prevPCB = newProcess;
		
		//renew free pmem list
		ku_mmu_pmem_free_list[freePM] = 1;
		
		//switch ku_cr3
		(*ku_cr3) = newProcess->PDBR;
		return 0;
	}
	
	//Process=(pid) already exists
	(*ku_cr3) = ptarget->PDBR;
	return 0;
}


//handling a page fault caused by demanding page or swapping
//return: success=0, fail=-1
int ku_page_fault(char pid, char va){
	
	struct PCB* ptarget = findProcess(pid);
	//FAIL: no process
	if(!ptarget)
		return -1;

	struct ku_pte* PDBR = ptarget->PDBR;
	
	//check PD
	struct ku_pte* PDE_pointer = findEntry(0, va, PDBR, NULL);
	char PDE_val = PDE_pointer->pte_val;
	if(PDE_val==0){ //unmmaped PDE
		int allocating_PFN = allocatePF();

		//FAIL: can't allocate PF for PMD
		if(allocating_PFN==-1)
			return -1;

		ku_mmu_pmem_free_list[allocating_PFN] = 1;
		PDE_pointer->pte_val = (allocating_PFN<<2) | (1); //renew PDE
		char* PMD_addr = ku_mmu_pmem + allocating_PFN*ku_mmu_psize;
		//initialize PMDEs
		for(int i=0; i<4; i++)
			*(PMD_addr+i) = 0;
	}


	//check PMD
	struct ku_pte* PMDE_pointer = findEntry(1, va, PDBR, NULL);
	char PMDE_val = PMDE_pointer->pte_val;
	if(PMDE_val==0){//unmapped PMDE
		int allocating_PFN = allocatePF();

		//FAIL: can't allocate PF for PT
		if(allocating_PFN==-1)
			return -1;

		ku_mmu_pmem_free_list[allocating_PFN] = 1;
		PMDE_pointer->pte_val = (allocating_PFN<<2) | (1); //renew PMDE
		char* PT_addr = ku_mmu_pmem + allocating_PFN*ku_mmu_psize;
		//iniitalized PTEs
		for(int i=0; i<4; i++)
			*(PT_addr+i) = 0;
	}


	//check PT
	struct ku_pte* PTE_pointer = findEntry(2, va, PDBR, NULL);
	char PTE_val = PTE_pointer->pte_val;
	char present = PTE_val & 1;
	if(PTE_val==0){//unmapped PTE
		int allocating_PFN = allocatePF();

		//FAIL: can't allocate PF for page
		if(allocating_PFN==-1)
			return -1;

		ku_mmu_pmem_free_list[allocating_PFN] = 1;
		PTE_pointer->pte_val = (allocating_PFN<<2) | (1);

		//make new pf_info
		struct pf_info* new_page = malloc(sizeof(struct pf_info));
		new_page->pid = pid;
		new_page->PDBR = PDBR;
		new_page->va = va;

		//insert into page frame list
		struct pf_info* latest_info = ku_mmu_pf_tail->prev_info;
		new_page->prev_info = latest_info;
		new_page->next_info = ku_mmu_pf_tail;
		latest_info->next_info = new_page;
		ku_mmu_pf_tail->prev_info = new_page;
	}
	else if(present==0){//swapped out state
		int swapin_PFN = swapin(PTE_pointer);

		//FAIL: fail to swap in
		if(swapin_PFN==-1)
			return -1;

		//renew pf_info(make new pf_info)
		struct pf_info* new_page = malloc(sizeof(struct pf_info));
		new_page->pid = pid;
		new_page->PDBR = PDBR;
		new_page->va = va;
		//renew pf_info(insert into page frame list)
		struct pf_info* latest_info = ku_mmu_pf_tail->prev_info;
		new_page->prev_info = latest_info;
		new_page->next_info = ku_mmu_pf_tail;
		latest_info->next_info = new_page;
		ku_mmu_pf_tail->prev_info = new_page;
	}
	

	//success
	return 0;
}




//Check if Process(=pid) exist
//return: fail=NULL, else=struct PCB*
struct PCB* findProcess(char pid){
	struct PCB* ptemp = ku_mmu_phead->nextPCB;
	while(ptemp!=ku_mmu_ptail){
		if(ptemp->pid==pid)
			return ptemp;
		ptemp = ptemp->nextPCB;
	}
	return NULL;
}

//Find not used space
//isPM=0: Swap Space, isPM=else: Physical Memory
//return: fail=(-1), else:index
int findPM(int isPM){
	char* arr=NULL;
	int len=0;
	if(!isPM){
		arr = ku_mmu_swap_free_list;
		len = ku_mmu_swap_pf_num;
	}
	else{
		arr = ku_mmu_pmem_free_list;
		len = ku_mmu_pmem_pf_num;
	}

	for(int i=0; i<len; i++)
		if(!arr[i])
			return i;
	return -1;
}


//Swap-out: from Physical Memory(page frame info pointer) to Swap Space(SW:index)
//return: success=not null(struct pf_info), fail=NULL
//return: success=PFN, fail=-1
int swapout(int SW){

	struct pf_info* outPage = ku_mmu_pf_head->next_info;
	//FAIL: not free swap space or null page
	if(ku_mmu_swap_free_list[SW])
		return -1;
	if(outPage==ku_mmu_pf_tail)
		return -1;
	
	char va = outPage->va;
	struct ku_pte* PTE_addr = findEntry(2, va, outPage->PDBR, NULL);
	int page_PFN;
	findEntry(3, va, outPage->PDBR, &page_PFN);
	

	//renew pte
	PTE_addr->pte_val = 0 | (SW << 1);

	//renew free list
	ku_mmu_pmem_free_list[page_PFN] = 0;
	ku_mmu_swap_free_list[SW] = 1;
	
	//pop from page frame list
	ku_mmu_pf_head->next_info = outPage->next_info;
	outPage->next_info->prev_info = ku_mmu_pf_head;

	
	return page_PFN;
}

//Swap-in: from Swap space to Physical Memory(Page Frame)
//return: success=PFN, fail=-1
int swapin(struct ku_pte* pte_pointer){
	
	//allocate PF for swap-in
	int swap_num = ((pte_pointer->pte_val)&(254))>>1;
	int allocating_PFN = allocatePF();
	//FAIL: swapin from swap space[0] or no free PFN
	if(swap_num==0 || allocating_PFN==-1)
		return -1;
	
	//renew free list
	ku_mmu_swap_free_list[swap_num] = 0;
	ku_mmu_pmem_free_list[allocating_PFN] = 1;

	//renew pte
	pte_pointer->pte_val = (allocating_PFN<<2) | (1);


	return allocating_PFN;	
}

//allocate new PF
//return: success=PFN, fail=-1
int allocatePF(){
	
	//find Free Page Frame (from Physcial Memory)
	int free_PFN = findPM(1);
	if(free_PFN>=0)
		return free_PFN;

	//no free PF: swap out
	int free_SW = findPM(0);
	if(free_SW>=0){
		free_PFN = swapout(free_SW);
		if(free_PFN>=0)
			return free_PFN;
	}
	
	//FAIL: no free swap space & PF
	return -1;
}


//Find PDE, PMDE, PTE, Page address
//degree: 0(PDE), 1(PMDE), 2(PTE), 3(Page), else-fail
//output param: int* PFN=PFN of Page
//return: address of pde(pmde, pte)
struct ku_pte* findEntry(int degree, int va, struct ku_pte* PDBR, int* PFN){
	//invalid arguments
	if(degree<0 || degree>3 || !PDBR)
		return NULL;

	int PD_index = (va & (3<<6))>>6;
	int PMD_index = (va & (3<<4))>>4;
	int PT_index = (va & (3<<2))>>2;
	int Page_offset = va & 3;
	

	//return PDE pointer
	struct ku_pte* PDE_pointer = PDBR + PD_index;
	char PDE_val = PDE_pointer->pte_val;
	if(degree==0)
		return PDE_pointer;

	//return PMDE pointer
	char PDE_PFN = (PDE_val & (63<<2))>>2;
	char* PMD_addr = ku_mmu_pmem + PDE_PFN*ku_mmu_psize;
	struct ku_pte* PMDE_pointer = PMD_addr + PMD_index;
	char PMDE_val = PMDE_pointer->pte_val;
	if(degree==1)
		return PMDE_pointer;

	//return PTE pointer
	char PMDE_PFN = (PMDE_val & (63<<2))>>2;
	char* PT_addr = ku_mmu_pmem + PMDE_PFN*ku_mmu_psize;
	struct ku_pte* PTE_pointer = PT_addr + PT_index;
	char PTE_val = PTE_pointer->pte_val;
	if(degree==2)
		return PTE_pointer;

	//return Page pointer
	char PTE_PFN = (PTE_val & (63<<2))>>2;
	char* Page_addr = ku_mmu_pmem + PTE_PFN*ku_mmu_psize;
	if(degree==3){
		*PFN = PTE_PFN;
		return Page_addr;
	}

	return NULL;
}
