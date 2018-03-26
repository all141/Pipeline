#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "cache.h" 
#include "CPU.h" 

/*In this version:
	-Imported functions from original CPU.c
	-Implemented reading of cache config file
*/

// to keep cache statistics
unsigned int I_accesses = 0;
unsigned int I_misses = 0;
unsigned int D_read_accesses = 0;
unsigned int D_read_misses = 0;
unsigned int D_write_accesses = 0; 
unsigned int D_write_misses = 0;
unsigned int L2_read_accesses = 0;
unsigned int L2_read_misses = 0;
unsigned int L2_write_accesses = 0; 
unsigned int L2_write_misses= 0;
float L1_D_mr = 0;
float L1_I_mr = 0;
float L2_mr = 0;


struct trace_item buff_stages[8];	//Allocation of memory for instructions

int mem_flag = 0;
int stall_flag = 0;				 	//Tells push_pipline() to stall
int squash_flag = 0;
int latency = 0;
int latency_comp = 0;


//Functions from project 1
/***********************************************************/
void init_pipeline()
{
	int i;
	for(i = 0; i < 8; i++){
		buff_stages[i].type = ti_NOP;
	}
}
void check_hazards(struct trace_item entry)
{
	
	//Data Hazard A
	if(buff_stages[3].type == ti_LOAD && 
	  ((buff_stages[2].type == (ti_RTYPE||ti_STORE || ti_BRANCH) && ((buff_stages[3].dReg == buff_stages[2].sReg_a || buff_stages[3].dReg == buff_stages[2].sReg_b)&&buff_stages[3].dReg!=255))||
	   (buff_stages[2].type == ti_ITYPE && buff_stages[3].dReg == buff_stages[2].sReg_a))){
		   //printf("DATA HAZARD A\nPC of ID/EX: (%x), PC of EX/MEM1: (%x)\n", buff_stages[2].PC, buff_stages[3].PC);
			stall_flag = 1;
		
	//Data Hazard B
	}else if(buff_stages[4].type == ti_LOAD &&
			((buff_stages[2].type == (ti_RTYPE || ti_STORE || ti_BRANCH) && ((buff_stages[4].dReg == buff_stages[2].sReg_a || buff_stages[4].dReg == buff_stages[2].sReg_b)&&buff_stages[4].dReg!=255))||
			 (buff_stages[2].type == ti_ITYPE && buff_stages[4].dReg == buff_stages[2].sReg_a))){
				//printf("DATA HAZARD B\nPC of Id/EX: (%x), PC of MEM1/MEM2: (%x)\n", buff_stages[2].PC, buff_stages[4].PC);
		stall_flag = 2;
		
	//Control Hazard
	}else if(buff_stages[2].type == ti_BRANCH ||
	   buff_stages[2].type == ti_JTYPE ||
	   buff_stages[2].type == ti_JRTYPE){
		   
		//PC of new instruction is same as branch/jump target address
		if((buff_stages[1].PC == buff_stages[2].Addr)){
			//printf("CONTROL HAZARD\nPC of [1]: (%x), PC of EX/MEM1 : (%x)\n", buff_stages[1].PC, buff_stages[3].Addr);
			squash_flag = 3;
		}
		
	//Structural Hazard
	}else if(buff_stages[5].type == (ti_RTYPE || ti_ITYPE || ti_LOAD) && (buff_stages[1].type != ti_JTYPE) && (squash_flag == 0)){
		//printf("STRUCTURAL HAZARD\nPC of buff[5]: (%x), PC of buff[1]: (%x)\n", buff_stages[5].PC, buff_stages[1].PC);
		stall_flag = 3;
	
	//No hazard detected
	}else{
		stall_flag = 0;
	}
	
}



void push_pipeline(struct trace_item entry)
{		
	
	if(stall_flag != 0){ //stallflag = 0 by default, which means no stalling would be needed
		if(stall_flag == 1){ // Stall for data hazard A	
			buff_stages[6] = buff_stages[5];
			buff_stages[5] = buff_stages[4];
			buff_stages[4] = buff_stages[3];
			buff_stages[3].type = ti_NOP;
			
		}else if(stall_flag == 2){ //Stall for data hazard B
			buff_stages[6] = buff_stages[5];
			buff_stages[5] = buff_stages[4];
			buff_stages[4].type = ti_NOP;
			
		}else if(stall_flag == 3){ //Stall for structural hazard
			buff_stages[6] = buff_stages[5];
			buff_stages[5] = buff_stages[4];
			buff_stages[4] = buff_stages[3];
			buff_stages[3].type = ti_NOP;
			
		}else{ //stall_flag should never be something other than 0,1,2,3
			printf("Invalid value of stall_flag");
			exit(0);
		}	
	}else if(squash_flag > 0){ //No hazards or stalling needed
		squash_flag--;
		buff_stages[6] = buff_stages[5];
		buff_stages[5] = buff_stages[4];
		buff_stages[4] = buff_stages[3];
		buff_stages[3] = buff_stages[2];
		buff_stages[2].type = 9; //SQUASHED
	}else{
		buff_stages[6] = buff_stages[5];
		buff_stages[5] = buff_stages[4];
		buff_stages[4] = buff_stages[3];
		buff_stages[3] = buff_stages[2];
		buff_stages[2] = buff_stages[1];
		buff_stages[1] = buff_stages[0];
		buff_stages[0] = entry;
	}
}

void check_config(unsigned int a, unsigned int b, unsigned int c, unsigned int d, unsigned int e, unsigned int f, unsigned int g){
	int stop_program_flag = 0;
	if((a%2!=0)&&(a!=0)&&(a!=1)){
		printf("Size of L1 instruction cache is not power of 2\n");
		stop_program_flag = 1;
	}
	if((b%2!=0)&&(b!=0)&&(b!=1)){
		printf("Associativity of L1 instruction cache is not power of 2\n");
		stop_program_flag = 1;
	}
	if((c%2!=0)&&(c!=0)&&(c!=1)){
		printf("Size of the L1 data cache is not power of 2\n");
		stop_program_flag = 1;
	}
	if((d%2!=0)&&(c!=0)&&(d!=1)){
		printf("Associativity of L1 data cache is not power of 2\n");
		stop_program_flag = 1;
	}
	if((e%2!=0)&&(e!=0)&&(e!=1)){
		printf("Size of the L2 cache is not power of 2\n");
		stop_program_flag = 1;
	}
	if((f%2!=0)&&(f!=0)&&(f!=1)){
		printf("Associativity of the L2 cache is not power of 2\n");
		stop_program_flag = 1;
	}
	if((g%2!=0)&&(g!=0)&&(g!=1)){
		printf("The cache block size is not a power of 2\n");
		stop_program_flag = 1;
	}
	
	if(stop_program_flag==1){
		exit(0);
	}
}
/*****************************************************************/

int main(int argc, char **argv)
{
  struct trace_item *tr_entry;
  size_t size;
  char *trace_file_name;
  int trace_view_on = 0;
  FILE *ptr_file;	//File to be read for cache config (cache_config.txt)
  int i;
  int stores = 0;
  int loads = 0;
  int I_latency = 0;
  int D_latency = 0;
  
  
  unsigned char t_type = 0;
  unsigned char t_sReg_a= 0;
  unsigned char t_sReg_b= 0;
  unsigned char t_dReg= 0;
  unsigned int t_PC = 0;
  unsigned int t_Addr = 0;

  unsigned int cycle_number = 0;

 if (argc == 1) {
    fprintf(stdout, "\nUSAGE: tv <trace_file> <switch - any character>\n");
    fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
    exit(0);
  }
  
  trace_file_name = argv[1];
  if (argc == 4) trace_view_on = atoi(argv[3]) ;

  
  // here you should extract the cache parameters from the configuration file 
  /*********************************************/
  ptr_file = fopen("cache_config.txt","r");
  if(!ptr_file) return 1;
  
  unsigned int L1_Isize;  		//the size of the L1 instruction cache in Kilo Bytes (0 means a perfect cache with miss penalty = 0 )
  unsigned int L1_Iassoc; 		//the associativity of the L1 instruction cache
  unsigned int L1_Dsize; 		//the size of the L1 data cache in Kilo Bytes (0 means a perfect cache with miss penalty = 0)
  unsigned int L1_Dassoc;		//the associativity of the L1 data cache
  unsigned int L2_size;			//the size of the L2 cache in Kilo Bytes (0 means no L2 cache)
  unsigned int L2_assoc;		//the associativity of the L2 cache
  unsigned int block_size;		//the cache block size in bytes (the same for all three caches)
  unsigned int L2_accesstime; 	//the L2 access time (in cycles)
  unsigned int mem_accesstime;	//the memory access time (in cycles)
  
  fscanf(ptr_file, "%d", &L1_Isize);	//Grab each parameter
  fscanf(ptr_file, "%d", &L1_Iassoc);
  fscanf(ptr_file, "%d", &L1_Dsize);
  fscanf(ptr_file, "%d", &L1_Dassoc);
  fscanf(ptr_file, "%d", &L2_size);
  fscanf(ptr_file, "%d", &L2_assoc);
  fscanf(ptr_file, "%d", &block_size);
  fscanf(ptr_file, "%d", &L2_accesstime);
  fscanf(ptr_file, "%d", &mem_accesstime);

  fclose(ptr_file);	//Close cache_config.txt
  
  check_config(L1_Isize, L1_Iassoc, L1_Dsize, L1_Dassoc, L2_size, L2_assoc, block_size);
  /*********************************************/
  
  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();
  init_pipeline();		//Initialize the pipeline
  
  struct cache_t *I_cache, *D_cache, *L2_cache;
  
  if(L2_size != 0) //If an L2 cache is requested to exist, make it
  {
	L2_cache = cache_create(L2_size, block_size, L2_assoc, mem_accesstime);	//Miss penalty for L2 is mem access time
	I_cache = cache_create(L1_Isize, block_size, L1_Iassoc, L2_accesstime); //Miss penalty for L1 is L2 access time
	D_cache = cache_create(L1_Dsize, block_size, L1_Dassoc, L2_accesstime);
  } else	//Else, just make the two L1 caches
  {
	  I_cache = cache_create(L1_Isize, block_size, L1_Iassoc, mem_accesstime); //Miss penalty for L1 is mem access time
	  D_cache = cache_create(L1_Dsize, block_size, L1_Dassoc, mem_accesstime);
  }
  

  while(1) {
	 
	size = trace_get_item(&tr_entry);	//Fetch next instruction

    if (!size ) {  /* no more instructions (trace_items) to simulate */
	  break;
    }
    else{              /* parse the next instruction to simulate */
      cycle_number++;
      t_type = tr_entry->type;
      t_sReg_a = tr_entry->sReg_a;
      t_sReg_b = tr_entry->sReg_b;
      t_dReg = tr_entry->dReg;
      t_PC = tr_entry->PC;
      t_Addr = tr_entry->Addr;
    }  
	
	if (trace_view_on) printf("\n");
	
	I_latency = cache_access(I_cache, tr_entry->PC, 0); /* simulate instruction fetch */
	I_accesses ++;
	
	if(I_latency > 0)//If L1_I_cache miss, check L2
	{
		I_misses++;
		if(L2_size != 0)//If L2 exists
		{
			latency_comp = I_latency;	//Save I_latency before accessing L2
			I_latency += cache_access(L2_cache, tr_entry->PC, 0);	//Access L2 cache
			L2_read_accesses++;
			if(I_latency > latency_comp) //I_latency increased, then we missed the L2 caache also
			{
				L2_read_misses++;
			}
		}
	}
	if(buff_stages[3].type == ti_LOAD)//If we need to access L1_D_cache 
	{
		D_latency += cache_access(D_cache, buff_stages[3].Addr, 0); //Access L1_D_cache
		D_read_accesses ++;
		if(D_latency > 0)//If L1_D_cache miss, check L2
		{
			D_read_misses++;
			if(L2_size != 0)//If L2 exists
			{
				latency_comp = D_latency;//Save D_latency before accessing L2
				D_latency += cache_access(L2_cache, buff_stages[3].Addr, 0);
				L2_read_accesses++;
				if(D_latency > latency_comp)
				{
					L2_read_misses++;
				}
			}
		}
	} else if(buff_stages[3].type == ti_STORE)//If we need to access L1_D_cache 
	{
		D_latency += cache_access(D_cache, buff_stages[3].Addr, 1);//Access L1_D_cache
		D_write_accesses ++ ;
		if(D_latency > 0)
		{
			D_write_misses++;
			if(L2_size != 0)
			{
				latency_comp = D_latency;//Save D_latency before accessing L2
				D_latency += cache_access(L2_cache, buff_stages[3].Addr, 1);
				L2_write_accesses++;
				if(D_latency > latency_comp)
				{
					L2_write_misses++;
				}
			}
		}
	}
	if(I_latency > D_latency){
		cycle_number += I_latency;
	}else if(D_latency > I_latency){
		cycle_number += D_latency;
	}
	push_pipeline(*tr_entry);
	I_latency = 0;
    D_latency = 0;

      switch(buff_stages[6].type) 
	  {
        case ti_NOP:
		  if (trace_view_on) printf("[cycle %d] NOP:", cycle_number);
          break;
        case ti_RTYPE:
		  if (trace_view_on) {
			printf("[cycle %d] RTYPE:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].sReg_b, buff_stages[6].dReg);
		  };
          break;
        case ti_ITYPE:
		  if (trace_view_on){
			printf("[cycle %d] ITYPE:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].dReg, buff_stages[6].Addr);
		  };
          break;
        case ti_LOAD:
		  if (trace_view_on){
			printf("[cycle %d] LOAD:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].dReg, buff_stages[6].Addr);
			
		  };
		  loads++;
		  break;
        case ti_STORE:
		  if (trace_view_on){
			printf("[cycle %d] STORE:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].sReg_b, buff_stages[6].Addr);
			
		  };
		  stores++;
		  break;
        case ti_BRANCH:
		  if (trace_view_on) {
			printf("[cycle %d] BRANCH:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].sReg_b, buff_stages[6].Addr);
		  };
          break;
        case ti_JTYPE:
		  if (trace_view_on) {
			printf("[cycle %d] JTYPE:", cycle_number);
			printf(" (PC: %x)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].Addr);
		  };
          break;
        case ti_SPECIAL:
		  if (trace_view_on) printf("[cycle %d] SPECIAL:", cycle_number);
          break;
        case ti_JRTYPE:
		  if (trace_view_on) {
			printf("[cycle %d] JRTYPE:", cycle_number);
			printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].dReg, buff_stages[6].Addr);
		  };
          break;
      }
   
  }
int p = 0;
push_pipeline(buff_stages[7]);
	for(p;p<6;p++)
	{
		if (trace_view_on) printf("\n");
		cycle_number++;
	switch(buff_stages[6].type) 
	  {
        case ti_NOP:
		  if (trace_view_on) printf("[cycle %d] NOP:", cycle_number);
          break;
        case ti_RTYPE:
		  if (trace_view_on) {
			printf("[cycle %d] RTYPE:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].sReg_b, buff_stages[6].dReg);
		  };
          break;
        case ti_ITYPE:
		  if (trace_view_on){
			printf("[cycle %d] ITYPE:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].dReg, buff_stages[6].Addr);
		  };
          break;
        case ti_LOAD:
		  if (trace_view_on){
			printf("[cycle %d] LOAD:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].dReg, buff_stages[6].Addr);
			
		  };
		  loads++;
		  break;
        case ti_STORE:
		  if (trace_view_on){
			printf("[cycle %d] STORE:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].sReg_b, buff_stages[6].Addr);
			
		  };
		  stores++;
		  break;
        case ti_BRANCH:
		  if (trace_view_on) {
			printf("[cycle %d] BRANCH:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].sReg_b, buff_stages[6].Addr);
		  };
          break;
        case ti_JTYPE:
		  if (trace_view_on) {
			printf("[cycle %d] JTYPE:", cycle_number);
			printf(" (PC: %x)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].Addr);
		  };
          break;
        case ti_SPECIAL:
		  if (trace_view_on) printf("[cycle %d] SPECIAL:", cycle_number);
          break;
        case ti_JRTYPE:
		  if (trace_view_on) {
			printf("[cycle %d] JRTYPE:", cycle_number);
			printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].dReg, buff_stages[6].Addr);
		  };
          break;
      }
	  push_pipeline(buff_stages[7]);
	}
	printf("+ Simulation terminates at cycle : %u\n", cycle_number);
    printf("I-cache accesses %u and misses %u\n", I_accesses, I_misses);
    printf("D-cache Read accesses %u and misses %u\n", D_read_accesses, D_read_misses);
    printf("D-cache Write accesses %u and misses %u\n", D_write_accesses, D_write_misses);
	printf("L2-cache Read accesses %u and misses %u\n", L2_read_accesses, L2_read_misses);
    printf("L2-cache Write accesses %u and misses %u\n", L2_write_accesses, L2_write_misses);
	
	L1_D_mr = ((float)D_read_misses + (float)D_write_misses) / ((float)D_read_accesses + (float)D_write_accesses);
	L1_I_mr = 100000*((float)I_misses / (float)I_accesses);
	if(L2_size != 0){
		L2_mr = ((float)L2_read_misses + (float)L2_write_misses) / ((float)L2_read_accesses + (float)L2_write_accesses);
	}
	
	printf("L1 Data Cache miss rate %f\n", L1_D_mr);
	printf("L1 Instruction Cache miss rate %fe-5\n", L1_I_mr);
	printf("L2 Cache miss rate %f\n", L2_mr);

  trace_uninit();

  exit(0);
}
