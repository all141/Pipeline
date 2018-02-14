/**************************************************************/
/* CS/COE 1541				 			
   just compile with gcc -o pipeline pipeline.c			
   and execute using							
   ./pipeline  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0  
***************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h" 

struct trace_item buff_stages[6];
int stall_flag = 0;

void check_hazards(size){
	
	//Control Hazard
	if(buff_stages[3]->type == BRANCH ||
	   buff_stages[3]->type == JTYPE ||
	   buff_stages[3]->type == JRTYPE ||){
		   
		//PC of new instruction is same as branch/jump target address
		if(size->PC == buff_stages[3]->Addr){
			buff_stages[0]-> type = NOP;
			buff_stages[1]-> type = NOP;
			buff_stages[2]-> type = NOP;
		}
		
		/*
		size = new instr at branch/jump target address
		buff_stages[0-2] = no ops (flushed)
		buff_stages[3-5] = unchanged instructions
		*/
		
	}
	//Data Hazard A
	if(buff_stages[3].type==LOAD&&((buff_stages[2].type==(RTYPE||STORE||BRANCH)&&buff_stages[3].dReg==buff_stages[2].sReg_a||buff_stages[3].dReg==buff_stages[2].sReg_b)||(buff_stages[2].type==ITYPE&&buff_stages[3].dReg==buff_stages[2].sReg_a))){
		
		//Data Hazard B
	}else if(buff_stages[4].type==LOAD&&((buff_stages[2].type==(RTYPE||STORE||BRANCH)&&buff_stages[4].dReg==buff_stages[2].sReg_a||buff_stages[3].dReg==buff_stages[2].sReg_b)||(buff_stages[2].type==ITYPE&&buff_stages[4].dReg==buff_stages[2].sReg_a))){
		
		//Structural Hazard
	}else if(buff_stages[4].type==(RTYPE||ITYPE||LOAD) && buff_stages[1].type!=JTYPE)){
		
	}else{
		
	}
	
	return NULL;
}

void push_pipeline(size){
	
	if(/*stallflag = 1*/){
		
	}else{
		buff_stages[5] = buff_stages[4];
		buff_stages[4] = buff_stages[3];
		buff_stages[3] = buff_stages[2];
		buff_stages[2] = buff_stages[1];
		buff_stages[1] = buff_stages[0];
		buff_stages[0] = size;
	}
	
	
}

int main(int argc, char **argv)
{
  struct trace_item *tr_entry;
  size_t size;
  char *trace_file_name;
  int trace_view_on = 0;
  
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

  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();

  while(1) {
    size = trace_get_item(&tr_entry);
   
    if (!size) {       /* no more instructions (trace_items) to simulate */
      printf("+ Simulation terminates at cycle : %u\n", cycle_number);
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
	
	/*Need method(s) to check for hazard conditions before pushing in new instructions*/
	check_struct_hazards(size);
	
	
	/*Need method(s) to insert a new instruction in pipeline and push existing instructions
	further in*/
	
	
// SIMULATION OF A SINGLE CYCLE cpu IS TRIVIAL - EACH INSTRUCTION IS EXECUTED
// IN ONE CYCLE
    if (trace_view_on) {// print the executed instruction if trace_view_on=1 
      switch(tr_entry->type) {
        case ti_NOP:
          printf("[cycle %d] NOP\n:",cycle_number) ;
          break;
        case ti_RTYPE:
          printf("[cycle %d] RTYPE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->dReg);
          break;
        case ti_ITYPE:
          printf("[cycle %d] ITYPE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
          break;
        case ti_LOAD:
          printf("[cycle %d] LOAD:",cycle_number) ;      
          printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
          break;
        case ti_STORE:
          printf("[cycle %d] STORE:",cycle_number) ;      
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
          break;
        case ti_BRANCH:
          printf("[cycle %d] BRANCH:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
          break;
        case ti_JTYPE:
          printf("[cycle %d] JTYPE:",cycle_number) ;
          printf(" (PC: %x)(addr: %x)\n", tr_entry->PC,tr_entry->Addr);
          break;
        case ti_SPECIAL:
          printf("[cycle %d] SPECIAL:\n",cycle_number) ;      	
          break;
        case ti_JRTYPE:
          printf("[cycle %d] JRTYPE:",cycle_number) ;
          printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_entry->PC, tr_entry->dReg, tr_entry->Addr);
          break;
      }
    }
	
	push_pipeline(size);
  }
  
  trace_uninit();

  exit(0);
}


