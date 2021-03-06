#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "cache.h" 
#include "CPU.h" 

/*GOALS
	1. FIX ORIGINAL PIPELINE
		-All hazards need to detect and handle properly
	2. ROUGH CACHE IMPLEMENTATION

*/
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

struct trace_item buff_stages[8];	//Allocation of memory for instructions

int stall_flag = 0;				 	//Tells push_pipline() to stall
int squash_flag = 0;
int latency = 0;

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
/*****************************************************************/

int main(int argc, char **argv)
{
  struct trace_item *tr_entry;
  size_t size;
  char *trace_file_name;
  int trace_view_on = 0;
  FILE *ptr_file;	//File to be read for cache config (cache_config.txt)
  char buf[10];
  int i;
  
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
  if (argc == 3) trace_view_on = atoi(argv[2]) ;
  
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
  /*********************************************/
  
  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();
  struct cache_t *I_cache, *D_cache;
  I_cache = cache_create(L1_Isize, block_size, L1_Iassoc, mem_accesstime); 
  D_cache = cache_create(L1_Dsize, block_size, L1_Dassoc, mem_accesstime);

  while(1) {
    size = trace_get_item(&tr_entry);
   
    if (!size ) {  /* no more instructions (trace_items) to simulate */
      printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      printf("I-cache accesses %u and misses %u\n", I_accesses, I_misses);
      printf("D-cache Read accesses %u and misses %u\n", D_read_accesses, D_read_misses);
      printf("D-cache Write accesses %u and misses %u\n", D_write_accesses, D_write_misses);
      break ;
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

// SIMULATION OF A SINGLE CYCLE cpu IS TRIVIAL - EACH INSTRUCTION IS EXECUTED
// IN ONE CYCLE, EXCEPT IF THERE IS A CACHE MISS.

	if (trace_view_on) printf("\n");
	//latency = cache_access(I_cache, tr_entry->PC, 0); /* simulate instruction fetch */
	//cycle_number = cycle_number + latency ;
        I_accesses ++ ;
      //  if(latency > 0) I_misses ++ ;

      switch(tr_entry->type) {
        case ti_NOP:
		  if (trace_view_on) printf("[cycle %d] NOP:", cycle_number);
          break;
        case ti_RTYPE:
		  if (trace_view_on) {
			printf("[cycle %d] RTYPE:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->dReg);
		  };
          break;
        case ti_ITYPE:
		  if (trace_view_on){
			printf("[cycle %d] ITYPE:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
		  };
          break;
        case ti_LOAD:
		  if (trace_view_on){
			printf("[cycle %d] LOAD:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
		  };
		  latency = cache_access(D_cache, tr_entry->Addr, 0);
		  cycle_number = cycle_number + latency ;
		  D_read_accesses ++ ;
		  if (latency > 0) D_read_misses ++ ;
		  break;
        case ti_STORE:
		  if (trace_view_on){
			printf("[cycle %d] STORE:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
		  };
		  latency = cache_access(D_cache, tr_entry->Addr, 1);
		  cycle_number = cycle_number + latency ;
		  D_write_accesses ++ ;
		  if (latency > 0) D_write_misses ++ ;
		  break;
        case ti_BRANCH:
		  if (trace_view_on) {
			printf("[cycle %d] BRANCH:", cycle_number);
			printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
		  };
          break;
        case ti_JTYPE:
		  if (trace_view_on) {
			printf("[cycle %d] JTYPE:", cycle_number);
			printf(" (PC: %x)(addr: %x)\n", tr_entry->PC, tr_entry->Addr);
		  };
          break;
        case ti_SPECIAL:
		  if (trace_view_on) printf("[cycle %d] SPECIAL:", cycle_number);
          break;
        case ti_JRTYPE:
		  if (trace_view_on) {
			printf("[cycle %d] JRTYPE:", cycle_number);
			printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_entry->PC, tr_entry->dReg, tr_entry->Addr);
		  };
          break;
      }
   
  }

  trace_uninit();

  exit(0);
}

