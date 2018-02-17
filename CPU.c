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

printf("h file working");
struct trace_item buff_stages[6];
int stall_flag = 0;

/* Function: check_hazards
 * -----------------------
 * This method is called before buff_stages[] is pushed by push_pipeline(). It detects any hazards that may occur during the current clock cycle. 
 * If a hazard is detected, this method modifies the 'stallflag' variable to indicate which hazard must be resolved.
 *
 * entry: the next trace_item to be pushed into buff_stages[0]
 *
 * returns: NULL (buff_stages[] is global so it should already be usable by all other methods once modified)
 */
void check_hazards(struct trace_item entry){
	
	//This needs to play nice with the branch prediction
	/*Control Hazard
	if(buff_stages[3]->type == BRANCH ||
	   buff_stages[3]->type == JTYPE ||
	   buff_stages[3]->type == JRTYPE ||){
		   
		//PC of new instruction is same as branch/jump target address
    //  vvv - what is size??? , is this supposed to be comparing predicted PC to BTB???
		if(size->PC == buff_stages[3]->Addr){
			buff_stages[0]-> type = NOP;
			buff_stages[1]-> type = NOP;
			buff_stages[2]-> type = NOP;
		}
		
	
		size = new instr at branch/jump target address
		buff_stages[0-2] = no ops (flushed)
		buff_stages[3-5] = unchanged instructions
	}*/
	
	//Data Hazard A
	if(buff_stages[3].type==ti_LOAD&&((buff_stages[2].type==(ti_RTYPE||ti_STORE||ti_BRANCH)&&buff_stages[3].dReg==buff_stages[2].sReg_a||buff_stages[3].dReg==buff_stages[2].sReg_b)||(buff_stages[2].type==ti_ITYPE&&buff_stages[3].dReg==buff_stages[2].sReg_a))){
		stall_flag = 1;
		
	//Data Hazard B
	}else if(buff_stages[4].type==ti_LOAD&&((buff_stages[2].type==(ti_RTYPE||ti_STORE||ti_BRANCH)&&buff_stages[4].dReg==buff_stages[2].sReg_a||buff_stages[3].dReg==buff_stages[2].sReg_b)||(buff_stages[2].type==ti_ITYPE&&buff_stages[4].dReg==buff_stages[2].sReg_a))){
		stall_flag = 2;
		
	//Structural Hazard	
	}else if(buff_stages[4].type==(ti_RTYPE||ti_ITYPE||ti_LOAD) && (buff_stages[1].type!=ti_JTYPE)){
		stall_flag = 3;
	
	//No hazard detected
	}else{
		stall_flag = 0;
	}
	
}

// ******NOTE***** What if there is more than one hazard during a cycle? Should we have a way to loop to check for and then resolve multiple hazards?
// Possible Answer?: Rerun the hazard check after any hazard is detected and dealt with. Should work as long as there is a correct heiarchy of hazard detection.
// Possible Answer? Cont.: Maybe put in call to hazard check after every stage of this if-else for the stall flags unless there is no flag.
/* Function: push_pipeline
 * -----------------------
 * Push instructions in the buff_stages array to the next index (stage). This method will look at the 'stallflag' variable to see if any NOOPs need to
 * be injected into the buff_stages[] due to hazards detected by the check_hazards() method. 
 *
 * entry: the next trace_item to be pushed into buff_stages[0] 
 * exiting: the trace_item that is being pushed out of the pipeline (pushed out of buff_stage[5])
 *
 * returns: exiting (buff_stages[] is global so it should already be usable by all other methods once modified)
 *
 * Function Call: push_pipeline(*tr_entry, &tr_exit)
 */
void push_pipeline(struct trace_item entry, struct trace_item* exiting){
	
	if(stall_flag != 0){ //stallflag = 0 by default, which means no stalling would be needed
		if(stall_flag == 1){ // Stall for data hazard A	
			buff_stages[5] = buff_stages[4];
			buff_stages[4] = buff_stages[3];
			buff_stages[3].type = ti_NOP;
			
		}else if(stall_flag == 2){ //Stall for data hazard B
			buff_stages[5] = buff_stages[4];
			buff_stages[4].type = ti_NOP;
			
		}else if(stall_flag == 3){ //Stall for structural hazard
			buff_stages[5] = buff_stages[4];
			buff_stages[4] = buff_stages[3];
			buff_stages[3].type = ti_NOP;
			
		}else{
			//stallflag should never be something other than 0,1,2,3
			printf("Invalid value of stallflag");
			exit(0);
		}	
	}else{ //No hazards or stalling needed
		*exiting = buff_stages[5];
		buff_stages[5] = buff_stages[4];
		buff_stages[4] = buff_stages[3];
		buff_stages[3] = buff_stages[2];
		buff_stages[2] = buff_stages[1];
		buff_stages[1] = buff_stages[0];
		buff_stages[0] = entry;
	}
	
	//check hazards here again?
	
	stall_flag = 0; //Reset stallflag
	//return exiting;
}

int main(int argc, char **argv)
{
  struct trace_item *tr_entry;
  size_t size;
  char *trace_file_name;
  int trace_view_on = 0;
  
  struct trace_item *tr_exit; //Instruction that exited pipeline
  tr_exit->type = NULL;
  tr_exit->sReg_a = NULL;
  tr_exit->sReg_b = NULL;
  tr_exit->dReg = NULL;
  tr_exit->PC = NULL;
  tr_exit->Addr = NULL;
  
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
  if (argc == 4)
  {
	  trace_view_on = atoi(argv[3]);
	  prediction_method = atoi(argv[2]);
  }

  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();

  //Loop until the end of the trace file
  while(1) {
    size = trace_get_item(&tr_entry); //Fetch next instruction
   
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
	
	/* At this point, we have fetched the next instruction to be enter the pipeline.	
	 * We must check for hazards that will occur in this cycle before we push the pipeline.
	*/
	//Check for hazards before pushing the pipeline
	//check_struct_hazards(&tr_entry);
	
	//resolve hazards and push instructions in buff_stages[] to next index
  // tr_entry and tr_exit are pointers in this scope to trace_item structs
	push_pipeline(*tr_entry, tr_exit);
	
// SIMULATION OF A SINGLE CYCLE cpu IS TRIVIAL - EACH INSTRUCTION IS EXECUTED
// IN ONE CYCLE
    if (trace_view_on) {// print the executed instruction if trace_view_on=1 
      switch(tr_exit->type) {
        case ti_NOP:
          printf("[cycle %d] NOP\n:",cycle_number) ;
          break;
        case ti_RTYPE:
          printf("[cycle %d] RTYPE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_exit->PC, tr_exit->sReg_a, tr_exit->sReg_b, tr_exit->dReg);
          break;
        case ti_ITYPE:
          printf("[cycle %d] ITYPE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_exit->PC, tr_exit->sReg_a, tr_exit->dReg, tr_exit->Addr);
          break;
        case ti_LOAD:
          printf("[cycle %d] LOAD:",cycle_number) ;      
          printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_exit->PC, tr_exit->sReg_a, tr_exit->dReg, tr_exit->Addr);
          break;
        case ti_STORE:
          printf("[cycle %d] STORE:",cycle_number) ;      
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_exit->PC, tr_exit->sReg_a, tr_exit->sReg_b, tr_exit->Addr);
          break;
        case ti_BRANCH:
          printf("[cycle %d] BRANCH:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_exit->PC, tr_exit->sReg_a, tr_exit->sReg_b, tr_exit->Addr);
          break;
        case ti_JTYPE:
          printf("[cycle %d] JTYPE:",cycle_number) ;
          printf(" (PC: %x)(addr: %x)\n", tr_exit->PC,tr_exit->Addr);
          break;
        case ti_SPECIAL:
          printf("[cycle %d] SPECIAL:\n",cycle_number) ;      	
          break;
        case ti_JRTYPE:
          printf("[cycle %d] JRTYPE:",cycle_number) ;
          printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_exit->PC, tr_exit->dReg, tr_exit->Addr);
          break;
      }
    }
  }
  
  trace_uninit();

  exit(0);
}


