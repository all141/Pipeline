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


struct trace_item buff_stages[8]; //Allocation of memory for instructions
int stall_flag = 0;				  //Tells push_pipline() to stall
int prediction_method = 0;		  //Defaults to 0
int squash_flag = 0;
int squash_print = 2;

/* Function: init_pipeline
 * ------------------------
 * Initializes the buff_stages[] array with NOOPs. This is to flush out any garbage data that
 * may be in there before buff_stages[7] is printed.
 *
 * returns: NULL  (buff_stages[] is global so it should already be usable by all other methods once modified)
 */
void init_pipeline(){
	int i;
	for(i = 0; i < 8; i++){
		buff_stages[i].type = ti_NOP;
	}
}



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
	//Control Hazard
	/*if(buff_stages[3]->type == BRANCH ||
	   buff_stages[3]->type == JTYPE ||
	   buff_stages[3]->type == JRTYPE){
		   
		//PC of new instruction is same as branch/jump target address
		if(entry->PC == buff_stages[3]->Addr){
			buff_stages[0]-> type = NOP;
			buff_stages[1]-> type = NOP;
			buff_stages[2]-> type = NOP;
		}
	}*/
	
	/* What is the hierarchy of hazard checks? Data > Control > Structural 
	*/
	
	//Data Hazard A
	if(buff_stages[3].type == ti_LOAD && 
	  ((buff_stages[2].type == (ti_RTYPE||ti_STORE || ti_BRANCH) && (buff_stages[3].dReg == buff_stages[2].sReg_a || buff_stages[3].dReg == buff_stages[2].sReg_b))||
	   (buff_stages[2].type == ti_ITYPE && buff_stages[3].dReg == buff_stages[2].sReg_a))){
		   printf("DATA HAZARD A\nPC of ID/EX: (%x), PC of EX/MEM1: (%x)\n", buff_stages[2].PC, buff_stages[3].PC);
			stall_flag = 1;
		
	//Data Hazard B
	}else if(buff_stages[4].type == ti_LOAD &&
			((buff_stages[2].type == (ti_RTYPE || ti_STORE || ti_BRANCH) && (buff_stages[4].dReg == buff_stages[2].sReg_a || buff_stages[4].dReg == buff_stages[2].sReg_b))||
			 (buff_stages[2].type == ti_ITYPE && buff_stages[4].dReg == buff_stages[2].sReg_a))){
				printf("DATA HAZARD B\nPC of Id/EX: (%x), PC of MEM1/MEM2: (%x)\n", buff_stages[2].PC, buff_stages[4].PC);
		stall_flag = 2;
		
	//Control Hazard
	}else if(buff_stages[2].type == ti_BRANCH ||
	   buff_stages[2].type == ti_JTYPE ||
	   buff_stages[2].type == ti_JRTYPE){
		   
		//PC of new instruction is same as branch/jump target address
		if(buff_stages[1].PC == buff_stages[2].Addr){
			printf("CONTROL HAZARD\nPC of [1]: (%x), PC of EX/MEM1 : (%x)\n", buff_stages[1].PC, buff_stages[3].Addr);
			squash_flag = 3;
			squash_print = 5;
			/*buff_stages[0].type = ti_NOP;
			buff_stages[1].type = ti_NOP;
			entry.type = ti_NOP;
			//buff_stages[2].type = ti_NOP;*/
		}
		
	//Structural Hazard
	}else if(buff_stages[5].type == (ti_RTYPE || ti_ITYPE || ti_LOAD) && (buff_stages[1].type != ti_JTYPE)){
		printf("STRUCTURAL HAZARD\nPC of buff[5]: (%x), PC of buff[1]: (%x)\n", buff_stages[5].PC, buff_stages[1].PC);
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
void push_pipeline(struct trace_item entry){		
	
	if(stall_flag != 0){ //stallflag = 0 by default, which means no stalling would be needed
		if(stall_flag == 1){ // Stall for data hazard A	
		    //printf("Data Hazard A\n");
			buff_stages[6] = buff_stages[5];
			buff_stages[5] = buff_stages[4];
			buff_stages[4] = buff_stages[3];
			buff_stages[3].type = ti_NOP;
			
		}else if(stall_flag == 2){ //Stall for data hazard B
			//printf("Data Hazard B\n");
			//printf("PC of [2]: %x, PC of [4]: %x \n", buff_stages[2].PC, buff_stages[4].PC);
			buff_stages[6] = buff_stages[5];
			buff_stages[5] = buff_stages[4];
			buff_stages[4].type = ti_NOP;
			
		}else if(stall_flag == 3){ //Stall for structural hazard
			//printf("Structural Hazard\n");
			buff_stages[6] = buff_stages[5];
			buff_stages[5] = buff_stages[4];
			buff_stages[4] = buff_stages[3];
			buff_stages[3].type = ti_NOP;
			
		}else{
			//stallflag should never be something other than 0,1,2,3
			printf("Invalid value of stall_flag");
			exit(0);
		}	
	}else if(squash_flag > 0){ //No hazards or stalling needed
		squash_flag--;
		buff_stages[6] = buff_stages[5];
		buff_stages[5] = buff_stages[4];
		buff_stages[4] = buff_stages[3];
		buff_stages[3] = buff_stages[2];
		buff_stages[2].type = ti_NOP; //SQUASHED
	}else{
		buff_stages[6] = buff_stages[5];
		buff_stages[5] = buff_stages[4];
		buff_stages[4] = buff_stages[3];
		buff_stages[3] = buff_stages[2];
		buff_stages[2] = buff_stages[1];
		buff_stages[1] = buff_stages[0];
		buff_stages[0] = entry;
	}
	
	//check hazards here again?
}

//BTB**************************************************************************************************
/**
* Object in the BTB
* Holds a one of two bit branch predictor, the pc that the branch evaluates to and the PC of the branch instruction
*/
struct BranchEntry {
	int prediction;
	char targetAddr[6];
	char branchPC[6];
};

//Get the index to look up in the BTB using the address
int getIndex(int address){
	int a = address;
	a = a % 1024;//2048, 512 for different branch table sizes ALSO get final 8 bits
	a = a / 8; //Get bits 8-3
	a = a % 64; //INDEX
	return a;
}

//Convert Hexadecimal string address to a decimal to operate on
int hexToDec(char *address){
        unsigned char decAddress = 0;
	    decAddress = (unsigned char) strtoul(address, NULL, 16); //BASE 16 FOR HEX VALUES
	return decAddress;
}

//See what the current prediciton is of an entry in the BTB
int checkPrediction(int s, int p){
	if(s == 2){
		if(p==0 || p==1){
			return 0;//branch not taken
		}else if(p==2 || p==3){
			return 1; //branch taken
		}else{
			printf("INVALID p");
			system("exit");
			}
	}else if(s == 1){
		return p;
	}else{
		printf("INVALID PREDICTION STYLE");
		system("exit");
		}
}

//P = prediction s = prediction style given by starting arguments.
//If hitMiss = 1 then update to hit else update to miss
int updatePrediction(int p, int s, int hitMiss)
{
	switch(s)
	{
		case 1://One Bit Predictor
		if(hitMiss==0) {
			return 0;
		}else if(hitMiss==1) {
			return 1;
		}else{
			printf("Invalid hitMiss!");
			system("exit");
		}
		break;
		case 2: //Two Bit Predictor
			switch(p){
				case 0://Predict Not Taken 00
					if(hitMiss == 0){
						return 0;//Predicition is 00
					}else if(hitMiss == 1){
						return 1; //Prediction is 01
					}else{
						printf("Invalid hitMiss");
						system("exit");
					}	
				break;

				case 1://Predict Not Taken 01
					if(hitMiss == 0){
						return 0;//Prediction is 00
					}else if(hitMiss == 1){
						return 3; //Prediction is 11
					}else{
						printf("Invalid hitMiss");
						system("exit");
					}
				break;

				case 2: //Predict Taken 10
					if(hitMiss == 0){
						return 0;//Prediciton is 00
					}else if(hitMiss == 1){
						return 3; //Prediction is 11
					}else{
						printf("Invalid hitMiss");
						system("exit");
					}
				break;
				case 3:
					if(hitMiss == 0){
						return 2; //Prediction is 10
					}else if(hitMiss == 1){
						return 3; //Prediction is 11
					}else{
						printf("Invalid hitMiss");
						system("exit");
					}	
				break;

				default:
					printf("Invalid Prediction!");
					system("exit");
			}
		break;
		default:
			printf("Invalid case!");
			system("exit");
	}
}
int branch_prediction(struct trace_item tr_entry)
{
	int currentPC = tr_entry->PC;
	int entryIndex = getIndex(currentPC); 
	if(BranchTable[entryIndex]==null)
	{//No prediction available
		if(buff_stages[2].type == ti_BRANCH)
		{//Is instruction a branch
			//Instruction is a branch
			if(tr_entry->PC == buff_stages[2].Addr) 
			{//Is the branch taken
				//Correct the PC;
				tr_entry->type = ti_NOP;
				buff_stages[0].type = ti_NOP
				BranchTable[entryindex].prediction = updatePrediction(BranchTable[entryIndex].prediction, ARGV[2], 1);
				BranchTable[entryindex].targetAddr = buff_stages[2].Addr;
				BranchTable[entryindex].branchPC = buff_stages[2].PC;
				//SQUASH
			}
			else 
			{
				BranchTable[entryindex].prediction = updatePrediction(BranchTable[entryIndex].prediction, ARGV[2], 0);
				BranchTable[entryindex].targetAddr = buff_stages[2].Addr;
				BranchTable[entryindex].branchPC = buff_stages[2].PC;
			}
		}
		else 
		{//Instruction is not a branch
			break;
	else{//Prediction is in BTB
		currentPC = BranchTable[entryIndex].branchPC;
		if(buff_stages[2].Addr == buff_stages[1].PC) //Was prediction correct
		{
			break;
		}
		else	
		{
			tr_entry->type = ti_NOP
			buff_stages[0].type = ti_NOP
			BranchTable[entryindex].prediction = updatePrediction(BranchTable[entryIndex].prediction, ARGV[2], 1);
			BranchTable[entryindex].targetAddr = buff_stages[2].Addr;
			BranchTable[entryindex].branchPC = buff_stages[2].PC;
			//SQUASH
		}
		}
	}
}


//BEGIN EXECUTION********************************************************************
int main(int argc, char **argv)
{
  
  struct trace_item *tr_entry;
  size_t size;
  char *trace_file_name;
  int trace_view_on = 0;
  int empty_flag = 0;
  int empty_count = 0;
  
//BTB is an array of Branch entry structs
struct BranchEntry BranchTable[64];
  
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

  init_pipeline();
  
  //Loop until the end of the trace file
  while(1) {
	if((stall_flag == 0) || (empty_flag != 0) || (squash_flag != 0)){
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
      /*t_type = tr_entry->type;
      t_sReg_a = tr_entry->sReg_a;
      t_sReg_b = tr_entry->sReg_b;
      t_dReg = tr_entry->dReg;
      t_PC = tr_entry->PC;
      t_Addr = tr_entry->Addr;*/
    }  
	
	branch_prediction(*tr_entry);
	/* At this point, we have fetched the next instruction to be enter the pipeline.	
	 * We must check for hazards that will occur in this cycle before we push the pipeline.
	*/
	//Check for hazards before pushing the pipeline
	check_hazards(*tr_entry);
	
	//resolve hazards and push instructions in buff_stages[] to next index
	push_pipeline(*tr_entry);
	
// SIMULATION OF A SINGLE CYCLE cpu IS TRIVIAL - EACH INSTRUCTION IS EXECUTED
// IN ONE CYCLE
	
	//Cleaning out pipeline 
	if(empty_flag == 1){
		empty_count++;
		cycle_number++;
		
		//push_pipeline();
	}
    if (trace_view_on) {// print the executed instruction if trace_view_on=1 
      switch(buff_stages[6].type) {
        case ti_NOP:
		  if(squash_print > 2){
		  printf("[cycle %d] SQUASH:\n", cycle_number);
		  }else{
			  printf("[cycle %d] NOP:\n",cycle_number) ;
		  }
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
