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
int currentPC = buff_stages[0]->PC;
int entryIndex = getIndex(currentPC); 
if(BranchTable[entryIndex]==null)
{//No prediction available
	currentPC = currentPC + 4; //Maybe TR_entry instead??
	if(buff_stages[3]->type == BRANCH)
	{//Is instruction a branch
		//Instruction is a branch
		if(buff_stages[0]->PC == buff_stages[3]->Addr) 
		{//Is the branch taken
			tr_entry->PC = buff_stages[0]->PC + 4 //Correct the PC;
			buff_stages[0]->type = NOP //Maybe not ID|Maybe send in noops in first place instead of squashing
			buff_stages[1]->type = NOP
			buff_stages[2]->type = NOP
			BranchTable[entryindex]->prediction = 1;
			BranchTable[entryindex]->targetAddr = buff_stages[3]->Addr;
			BranchTable[entryindex]->branchPC = buff_stages[3]->PC;
		}
		else 
		{
			BranchTable[entryindex]->prediction = 0;
			BranchTable[entryindex]->targetAddr = buff_stages[3]->Addr;
			BranchTable[entryindex]->branchPC = buff_stages[3]->PC;
		}
	}
	else 
	{//Instruction is not a branch
		Proceed as normal;
	}
else{//Prediction is in BTB
	currentPC = BranchTable[entryIndex]->branchPC;
	if(buff_stages[3]->Addr == buff_stages[2]->PC) //Was prediction correct
	{
		Proceed as normal;
	}
	else	
	{
		tr_entry->PC = buff_stages[0]->PC + 4 //Correct the PC;
		buff_stages[0]->type = NOP //Maybe not ID|Maybe send in noops in first place instead of squashing
		buff_stages[1]->type = NOP
		buff_stages[2]->type = NOP
		BranchTable[entryindex]->prediction = 1;
		BranchTable[entryindex]->targetAddr = buff_stages[3]->Addr;
		BranchTable[entryindex]->branchPC = buff_stages[3]->PC;
	}
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
	
