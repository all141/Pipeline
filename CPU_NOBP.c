/**************************************************************/
/* CS/COE 1541				 			
   just compile with gcc -o pipeline pipeline.c			
   and execute using							
   ./pipeline  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0  
***************************************************************/
//TR_ENTRY MIGHT BE GETTING OVERWRITTEN,MAYBE
#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "CPU.h" 


struct trace_item buff_stages[8];	//Allocation of memory for instructions

int stall_flag = 0;				 	//Tells push_pipline() to stall
int squash_flag = 0;

//*******************************************************
//****MAIN HELPER FUNCTIONS******************************
//*******************************************************
/* Function: init_pipeline
 * ------------------------
 * Writes NOOPS in all the buff_stages[] indexes. This is to flush out any garbage data that
 * may be in there from when buff_stages[] is initialized.
 *
 * RETURN
 * returns: NULL  (buff_stages[] is global so it should already be usable by all other methods once modified)
 * Globals: buff_stages[]
 */
void init_pipeline()
{
	int i;
	for(i = 0; i < 8; i++){
		buff_stages[i].type = ti_NOP;
	}
}

/* Function: check_hazards
 * -----------------------
 * This function is called before buff_stages[] is pushed by push_pipeline(). It detects any hazards that may occur during the current clock cycle. 
 * If a hazard is detected, this function modifies the 'stallflag' variable to indicate which hazard must be resolved.
 *
 * PARAMETERS
 * entry: the next trace_item to be pushed into buff_stages[0]
 *
 * RETURN
 * returns: NULL
 * Globals: buff_stages[], stall_flag, squash_flag
 */
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
		if(buff_stages[1].PC == buff_stages[2].Addr){
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

/* Function: push_pipeline
 * -----------------------
 * Push instructions in the buff_stages array to the next index (stage). This method will look at the 'stallflag' variable to see if any NOOPs need to
 * be injected into the buff_stages[] due to hazards detected by the check_hazards() method. 
 *
 * PARAMETERS
 * entry: the next trace_item to be pushed into buff_stages[0] 
 *
 * RETURN
 * returns: NULL
 * Globals: buff_stages[]
 */
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

//***********MAIN*****************//
int main(int argc, char **argv)
{
  
  struct trace_item *tr_entry;
  size_t size;
  char *trace_file_name;
  int trace_view_on = 0;
  
  int empty_flag = 0;		//Flag to tell when we have reached the end of the trace file
  int empty_count = 0;      //Used to count down number of cycles need to push final instruction out of the pipeline
  
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
  trace_view_on = atoi(argv[2]);
 

  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();
  init_pipeline();		//Initialize the pipeline
  
  //Loop until the end of the trace file
  while(1) {
	if(((stall_flag == 0) || (empty_flag == 0)) && (squash_flag == 0)){
		size = trace_get_item(&tr_entry); //Fetch next instruction
	}    
	
    stall_flag = 0; //Reset stall_flag
	
    if (!size) {       /* no more instructions (trace_items) to simulate */
	  tr_entry->type = ti_NOP;
	  empty_flag = 1;
	  size = 1;
    }
    else{              /* parse the next instruction to simulate */
      cycle_number++;
    }  

	//Check for hazards before pushing the pipeline
	check_hazards(*tr_entry);
	
	//Resolve hazards and push instructions in buff_stages[] to next index
	push_pipeline(*tr_entry);
	
	//Cleaning out pipeline 
	if(empty_flag == 1){
		empty_count++;
		cycle_number++;
	}
	
    if (trace_view_on) {// print the executed instruction if trace_view_on = 1 
      switch(buff_stages[6].type) {
        case ti_NOP:
			printf("[cycle %d] NOP:\n",cycle_number);
			break;
		case 9:	//Print the 'squashed' instruction
			printf("[cycle %d] SQUASH:\n", cycle_number);
			break;
        case ti_RTYPE:
          printf("[cycle %d] RTYPE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].sReg_b, buff_stages[6].dReg);
          break;
        case ti_ITYPE:
          printf("[cycle %d] ITYPE:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].dReg, buff_stages[6].Addr);
          break;
        case ti_LOAD:
          printf("[cycle %d] LOAD:",cycle_number) ;      
          printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].dReg, buff_stages[6].Addr);
          break;
        case ti_STORE:
          printf("[cycle %d] STORE:",cycle_number) ;      
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].sReg_b, buff_stages[6].Addr);
          break;
        case ti_BRANCH:
          printf("[cycle %d] BRANCH:",cycle_number) ;
          printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].sReg_a, buff_stages[6].sReg_b, buff_stages[6].Addr);
          break;
        case ti_JTYPE:
          printf("[cycle %d] JTYPE:",cycle_number) ;
          printf(" (PC: %x)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].Addr);
          break;
        case ti_SPECIAL:
          printf("[cycle %d] SPECIAL:\n",cycle_number) ;      	
          break;
        case ti_JRTYPE:
          printf("[cycle %d] JRTYPE:",cycle_number) ;
          printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", buff_stages[6].PC, buff_stages[6].dReg, buff_stages[6].Addr);
          break;
      }
    }
	if(empty_count >= 6){
			break;
		}
  }
  trace_uninit();
  
  printf("+ Simulation terminates at cycle : %u\n", cycle_number);
  exit(0);
}