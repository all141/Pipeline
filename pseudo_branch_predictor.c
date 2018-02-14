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
if(PC is not in BTB)  //No prediction available
	PC = PC + 4;
	if(buff_stages[3]>type == BRANCH) //Is instruction a branch
		//Instruction is a branch
		if(buff_stages[0]->PC == buff_stages[3]->Addr) //Is the branch taken
			Correct the PC;
			Kill Instruction in IF1 and IF2 and ID //Maybe not ID
			Correct the entry in BTB;
		else //Instruction is not a branch
			Record entry in BTB;
	else
		Proceed as normal;
else				//Prediction is in BTB
	Set PC to PC in BTB;
	if(prediction = correct) //Was prediction correct
		Proceed as normal;
	else	
		Correct the PC;
		Kill instruction in IF1 and IF2 and ID //Maybe not ID
		Correct the entry in BTB;
*******************************************************************************************
BTB
struct BranchEntry 
{
	int prediction;
	char targetAddr[6];
	char branchPC[6];
}
struct BranchEntry BranchTable[64];


To index an entry:
	//Must get bits 8-3 of address.
	Mod result by 1024
	Divide the result by 8
	Mod this by 64 to get hash index
	= branchIndex
	
int getindex(int address)
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
	
