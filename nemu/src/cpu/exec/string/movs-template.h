#include "cpu/exec/template-start.h"

#define instr movs

make_helper(concat(movs_n_,SUFFIX)){
	swaddr_write (reg_l(R_EDI),2,swaddr_read (reg_l(R_ESI),4));
	if (cpu.DF == 0)
	{
		reg_l(R_EDI) += DATA_BYTE;
		reg_l(R_ESI) += DATA_BYTE;
	}
	else
	{
		reg_l(R_EDI) -= DATA_BYTE;
		reg_l(R_ESI) -= DATA_BYTE;
	}
	print_asm("movs");
	return 1;
}


#include "cpu/exec/template-end.h"
