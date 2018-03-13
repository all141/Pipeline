#include <stdio.h>

void print_statistics(int L1_D_acc, int L1_D_hits, int L1_D_miss, int L1_D_m_rate,
						 int L1_I_acc, int L1_I_hits, int L1_I_miss, int L1_I_m_rate,
						 int L2_acc, int L2_hits, int L2_miss, int L2_m_rate){
							
							printf("-\tL1 Data cache:\t\t%d accesses, %d hits, %d misses, %d miss rate\n", L1_D_acc, L1_D_hits, L1_D_miss, L1_D_m_rate);
							printf("-\tL1 Instruction cache:\t%d accesses, %d hits, %d misses, %d miss rate\n", L1_I_acc, L1_I_hits, L1_I_miss, L1_I_m_rate);
							printf("-\tL2 cache:\t\t%d accesses, %d hits, %d misses, %d miss rate\n", L2_acc, L2_hits, L2_miss, L2_m_rate);
						 }

int main(int argc, char **argv){
	
	int L1_D_acc = 1;
	int L1_D_hits = 2;
	int L1_D_miss = 3;
	int L1_D_m_rate = 4; 
	int L1_I_acc = 5;
	int L1_I_hits = 6;
	int L1_I_miss = 7;
	int L1_I_m_rate = 8;
	int L2_acc = 9;
	int L2_hits = 10;
	int L2_miss = 11;
	int L2_m_rate = 12;
	
	print_statistics(L1_D_acc, L1_D_hits, L1_D_miss, L1_D_m_rate, L1_I_acc, L1_I_hits, L1_I_miss, L1_I_m_rate, L2_acc, L2_hits, L2_miss, L2_m_rate);
}

