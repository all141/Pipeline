INSTRUCTIONS
If prediction_method = 1, your simulation should assume that the architecture uses a one-bit
branch predictor which records the last branch condition and address. This predictor is
consulted in the IF1 stage which means that if the prediction is correct, the instruction
following the branch in the pipeline is the correct one. Otherwise, the wrong prediction will
be discovered in the EX stage and the three instructions following the branch in the pipeline
will be flushed. When no prediction can be made, the “predict not taken” policy should be
assumed. Use a Branch Prediction Hash Table with 64 entries and index this table with bits
8-3 of the branch instruction address (note that some addresses will collide and thus some
stored information will be lost – that is OK).
*******************************************************************************************
LOGIC
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
			BranchTable[entryindex].prediction = updatePrediction(BranchTable[entryIndex].prediction, ARGV[1], 0);
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
		BranchTable[entryindex].prediction = updatePrediction(BranchTable[entryIndex].prediction, ARGV[1], 1);
		BranchTable[entryindex].targetAddr = buff_stages[2].Addr;
		BranchTable[entryindex].branchPC = buff_stages[2].PC;
		//SQUASH
	}
    }
}

int checkPrediction(int s,int p){
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
*******************************************************************************************
//BTB
/**
* Object in the BTB
* Holds a one of two bit branch predictor, the pc that the branch evaluates to and the PC of the branch instruction
*/
struct BranchEntry 
{
	int prediction;
	char targetAddr[6];
	char branchPC[6];
}

//BTB is an array of Branch entry structs
struct BranchEntry BranchTable[64];

//Get the index to look up in the BTB using the address
int getIndex(int address)
{
	int a = address;
	a = a % 1024;//2048, 4096 for different branch table sizes ALSO get final 8 bits
	a = a / 8; //Get bits 8-3
	a = a % 64; //INDEX
	return a;
}

//Convert Hexadecimal string address to a decimal to operate on
int hexToDec(char *address)
{
        unsigned char decAddress = 0;
	decAddress = (unsigned char) strtoul(address, NULL, 16); //BASE 16 FOR HEX VALUES
	return decAddress;
}
	
