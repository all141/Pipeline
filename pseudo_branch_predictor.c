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
	if(buff_stages[2]->type == BRANCH)
	{//Is instruction a branch
		//Instruction is a branch
		if(tr_entry->PC == buff_stages[2]->Addr) 
		{//Is the branch taken
			//Correct the PC;
			tr_entry->type = NOP;
			buff_stages[0]->type = NOP //Maybe not ID|Maybe send in noops in first place instead of squashing
			BranchTable[entryindex]->prediction = updatePrediction(BranchTable[entryIndex]->prediction, ARGV[2], 1);
			BranchTable[entryindex]->targetAddr = buff_stages[2]->Addr;
			BranchTable[entryindex]->branchPC = buff_stages[2]->PC;
		}
		else 
		{
			BranchTable[entryindex]->prediction = updatePrediction(BranchTable[entryIndex]->prediction, ARGV[2], 0);
			BranchTable[entryindex]->targetAddr = buff_stages[2]->Addr;
			BranchTable[entryindex]->branchPC = buff_stages[2]->PC;
		}
	}
	else 
	{//Instruction is not a branch
		//Proceed as normal;
	}
else{//Prediction is in BTB
	currentPC = BranchTable[entryIndex]->branchPC;
	if(buff_stages[2]->Addr == buff_stages[1]->PC) //Was prediction correct
	{
		//Proceed as normal;
	}
	else	
	{
		tr_entry->type = NOP
		buff_stages[0]->type = NOP //Maybe send in noops in first place instead of squashing
		BranchTable[entryindex]->prediction = updatePrediction(BranchTable[entryIndex]->prediction, ARGV[2], 1);
		BranchTable[entryindex]->targetAddr = buff_stages[2]->Addr;
		BranchTable[entryindex]->branchPC = buff_stages[2]->PC;
	}
    }
}

int checkPrediction
//P = prediction s = prediction style given by starting arguments.
//If hitMiss = 1 then update to hit else update to miss
int updatePrediction(int p, int s, int hitMiss)
{
	switch(s)
	{
		case 1://One Bit Predictor
		if(hitMiss==0) //Predict Not Taken
		{
			return 0;
		}elseif(hitMiss==1) //Predict Taken
		{
			return 1;
		}else
		{
			printf("Invalid hitMiss!");
		}
		break;
		case 2: //Two Bit Predictor
		switch(p)
		{
			case 0://Predict Not Taken 00
				if(hitMiss == 0)//Still predict not taken
				{
					return 0;//Predicition is 00
				}elseif(hitMiss == 1)//Predict Taken
				{
					return 1; //Prediction is 01
				}else
				{
					printf("Invalid hitMiss");
				}	
			break;
				
			case 1://Predict Not Taken 01
				if(hitMiss == 0)//Still predict not taken
				{
					return 0;//Prediction is 00
				}elseif(hitMiss == 1)//Predict Taken
				{
					return 3; //Prediction is 11
				}else
				{
					printf("Invalid hitMiss");
				}
			break;
				
			case 2: //Predict Taken 10
				if(hitMiss == 0)//Predict not taken
				{
					return 0;//Prediciton is 00
				}elseif(hitMiss == 1)//Predict Taken
				{
					return 3; //Prediction is 11
				}else
				{
					printf("Invalid hitMiss");
				}
			break;
				
			case 3:
				if(hitMiss == 0)//Predict not taken
				{
					return 2; //Prediction is 10
				}elseif(hitMiss == 1)//Predict Taken
				{
					return 3; //Prediction is 11
				}else
				{
					printf("Invalid hitMiss");
				}	
			break;
				
			default:
			printf("Invalid Prediction!");
		}
			
		break;
		default:
		printf("Invalid case!");
	}
}

*******************************************************************************************
BTB
struct BranchEntry 
{
	int prediction;
	char targetAddr[6];
	char branchPC[6];
}

struct BranchEntry BranchTable[64];
	
int getIndex(int address)
{
	int a = address;
	a = a % 1024;
	a = a / 8;
	a = a % 64;
	return a;
}
	
int hexToDec(char *address)
{
      int a = atoi(address);
      int d, i = 0, r;
      while(a > 0)
      {
            r = a % 10;
            d = d + r * pow(16, i);
             = a / 10;
            i++;
      }
      return d;
}
	
