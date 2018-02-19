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
struct BranchEntry BranchTable[128]; //BTB is an array of Branch entry structs

int prediction_method = 0;		  	//Defaults to 0
int prediction_correct = 0;			//Set by branch_prediction() / read by check_hazards(); 0 = Branch prediction was incorrect / 1 = Branch prediction was correct

int stall_flag = 0;				 	//Tells push_pipline() to stall
int squash_flag = 0;

struct BranchEntry {	//Struct to hold an entry in the branch prediction table
	int prediction;
	int targetAddr;
	int branchPC;
};

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
		if((buff_stages[1].PC == buff_stages[2].Addr) && prediction_correct == 0){
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


//*****************************************************
//****BRANCH PREDICTION TABLE AND FUNCTIONS************
//*****************************************************
/* Function: getIndex
 * ------------------
 * This function hashes the prediction table index from the instruction program counter.
 *
 * PARAMETERS
 * address: the memory address of the instruction being checked in the table
 *
 * RETURN
 * returns: a: the hashed index of 'address'
 * Globals: None
 */
int getIndex(int address)
{
	int a = address;
	a = a % 2048;	//2048, 512 for different branch table sizes ALSO get final 8 bits
	a = a / 8; 		//Get bits 8-3
	a = a % 128; 	//INDEX
	return a;
}

/* Function: hexToDec
 * ------------------
 * This function converts a hexadecimal value to a decimal value.
 *
 * PARAMETERS
 * address: the hex value (in our case the PC address) to be converted to decimal
 *
 * RETURN
 * returns: decAddress: original hex 'address' value in decimal
 * Globals: none
 */
int hexToDec(char address)
{
        unsigned char decAddress = 0;
	    decAddress = (unsigned char) strtoul(address, NULL, 16); //BASE 16 FOR HEX VALUES
	return decAddress;
}

/* Function: checkPrediction
 * -------------------------
 * This function makes a prediction based on the prediction style specified by the user
 *
 * PARAMETERS
 * s: prediction style given by the starting arguements (no prediction, 1-bit, or 2-bit)
 * p: actual prediction
 *
 * RETURN
 * returns: p: the prediction that was made
 * Globals: none
 */
int checkPrediction(int s, int p)
{
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

/* Function: updatePrediction 
 * --------------------------
 * This function updates a prediction for a branch prediction table entry. It determines the style of predictor (1-bit, 2-bit) and whether
 * the branch was taken or not the previous time the branch was seen by the table.
 *
 * PARAMETERS
 * p: value of the prediction from the table
 * s: prediction style determined by program arguments
 * hitMiss: whether the branch is taken
 *
 * RETURN
 * returns: an integer literal based on what the new prediction is
 */
int updatePrediction(int p, int s, int hitMiss)
{
	//If hitMiss = 1 then update to hit else update to miss
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

/* Function: branch_prediction
 * ---------------------------
 * This function will first check if a prediction is available for the current branch instructions. It will then determine if the existing prediction was correct
 * or add the new instruction to the prediction table if no previous prediction exists.
 *
 * PARAMETERS
 * entry: the next trace_item to be pushed into buff_stages[0]
 *
 * RETURN
 * returns: NULL
 * Globals: BranchTable[]
 */
void branch_prediction(struct trace_item entry)
{
	int currentPrediction = 0;
	int currentPC = buff_stages[0].PC;
	int entryIndex = getIndex(currentPC); 
	int isTaken = 0;
	if(entry.PC == buff_stages[0].Addr)
	{
		isTaken = 1;
	}
	if(buff_stages[0].type == ti_BRANCH)
	{//Is instruction a branch
		if((BranchTable[entryIndex].prediction == 0)&&(BranchTable[entryIndex].targetAddr == -1)&&(BranchTable[entryIndex].branchPC == -1))
		{//No prediction available
		//Instruction is a branch
			BranchTable[entryIndex].prediction =updatePrediction(BranchTable[entryIndex].prediction, prediction_method, isTaken);
			BranchTable[entryIndex].targetAddr = buff_stages[0].Addr;
			BranchTable[entryIndex].branchPC = buff_stages[0].PC;
			prediction_correct = 0;
		}else{//Prediction is in BTB
			currentPrediction = checkPrediction(prediction_method,BranchTable[entryIndex].prediction);
			if((currentPrediction == 1 && isTaken == 1)||(currentPrediction == 0 && isTaken == 0))//Was prediction correct
			{
				BranchTable[entryIndex].prediction = updatePrediction(BranchTable[entryIndex].prediction, prediction_method, isTaken);
				prediction_correct = 1;
			}
			else	
			{
				BranchTable[entryIndex].prediction = updatePrediction(BranchTable[entryIndex].prediction, prediction_method, isTaken);
				prediction_correct = 0;
			}
		}
	}else if(buff_stages[0].type == ti_JTYPE || buff_stages[0].type == ti_JRTYPE)
	{//Is instruction a jump
		if((BranchTable[entryIndex].prediction == -1)&&(BranchTable[entryIndex].targetAddr == -1)&&(BranchTable[entryIndex].branchPC == -1))
		{//No prediction available
			BranchTable[entryIndex].prediction =updatePrediction(BranchTable[entryIndex].prediction, prediction_method, 1);
			BranchTable[entryIndex].targetAddr = buff_stages[0].Addr;
			BranchTable[entryIndex].branchPC = buff_stages[0].PC;
			prediction_correct = 0;
		}else{//Prediction is in BTB
			BranchTable[entryIndex].prediction = updatePrediction(BranchTable[entryIndex].prediction, prediction_method, 1);
			prediction_correct = 1;
		}
	}else{
	}
}

/* Function: init_branch_table
 * ---------------------------
 * This function initializes the BranchTable[] array to be used as the branch prediction table.
 * 
 * RETURN
 * returns: NULL
 * Globals: BranchTable[]
 */
void init_branch_table()
{
	int i = 0;
	for(i; i<128;i++)
	{
		BranchTable[i].prediction = 0;
		BranchTable[i].targetAddr= -1;
		BranchTable[i].branchPC = -1;
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
  init_branch_table();	//Initialize the branch table
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
	
	if(prediction_method != 0){
		branch_prediction(*tr_entry);
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
