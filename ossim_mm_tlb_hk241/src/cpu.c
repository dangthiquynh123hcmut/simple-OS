
#include "cpu.h"
#include "mem.h"
#include "mm.h"
#include <pthread.h>



int calc(struct pcb_t * proc) {
	return ((unsigned long)proc & 0UL);
}

int alloc(struct pcb_t * proc, uint32_t size, uint32_t reg_index) {
	addr_t addr = alloc_mem(size, proc);
	if (addr == 0) {
		return 1;
	}else{
		proc->regs[reg_index] = addr;
		return 0;
	}
}

int free_data(struct pcb_t * proc, uint32_t reg_index) {
	return free_mem(proc->regs[reg_index], proc);
}

int read(
		struct pcb_t * proc, // Process executing the instruction
		uint32_t source, // Index of source register
		uint32_t offset, // Source address = [source] + [offset]
		uint32_t destination) { // Index of destination register
	
	BYTE data;
	if (read_mem(proc->regs[source] + offset, proc,	&data)) {
		proc->regs[destination] = data;
		return 0;		
	}else{
		return 1;
	}
}

int write(
		struct pcb_t * proc, // Process executing the instruction
		BYTE data, // Data to be wrttien into memory
		uint32_t destination, // Index of destination register
		uint32_t offset) { 	// Destination address =
					// [destination] + [offset]
	return write_mem(proc->regs[destination] + offset, proc, data);
} 

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

int run(struct pcb_t * proc) {
	/* Check if Program Counter point to the proper instruction */
	if (proc->pc >= proc->code->size) {
		return 1;
	}
	
	struct inst_t ins = proc->code->text[proc->pc];
	proc->pc++;
	int stat = 1;

	pthread_mutex_lock(&lock);

	switch (ins.opcode) {
	case CALC:
		printf("                                                Type of instruction: CALC\n");
		stat = calc(proc);
		break;
	case ALLOC:
		printf("                                                Type of instruction: ALLOC\n");
#ifdef CPU_TLB 
		stat = tlballoc(proc, ins.arg_0, ins.arg_1);
#elif defined(MM_PAGING)
		stat = pgalloc(proc, ins.arg_0, ins.arg_1);
#else
		stat = alloc(proc, ins.arg_0, ins.arg_1);
#endif
		break;
	case FREE:
		printf("                                                Type of instruction: FREE\n");
#ifdef CPU_TLB
		stat = tlbfree_data(proc, ins.arg_0);
#elif defined(MM_PAGING)
		stat = pgfree_data(proc, ins.arg_0);
#else
		stat = free_data(proc, ins.arg_0);
#endif
		break;
	case READ:
		printf("                                                Type of instruction: READ\n");
#ifdef CPU_TLB
		stat = tlbread(proc, ins.arg_0, ins.arg_1, ins.arg_2);
#elif defined(MM_PAGING)
		stat = pgread(proc, ins.arg_0, ins.arg_1, ins.arg_2);
#else
		stat = read(proc, ins.arg_0, ins.arg_1, ins.arg_2);
#endif
		break;
	case WRITE:
		printf("                                                Type of instruction: WRITE\n");
#ifdef CPU_TLB
		stat = tlbwrite(proc, ins.arg_0, ins.arg_1, ins.arg_2);
		// printf("Check params: data = %d, destination = %d, offset = %d\n", ins.arg_0, ins.arg_1, ins.arg_2);
#elif defined(MM_PAGING)
		stat = pgwrite(proc, ins.arg_0, ins.arg_1, ins.arg_2);
#else
		stat = write(proc, ins.arg_0, ins.arg_1, ins.arg_2);
#endif
		break;
	default:
		stat = 1;
	}

	pthread_mutex_unlock(&lock);

	if (stat == -1) // Handle instruction fault.
                    // If an instruction does not execute
                    // as expected, the program aborts
    {
    	printf("\n\n");
        printf ("STOP: in cpu.c/ run(): An error occurred.\n");
        exit(0);
    }

	return stat;

}


