#include "sgm_cpu.h"

#include <stdio.h>
#include <assert.h>
#include <memory.h>
#include <math.h>

static sgmByte sgm_cpu_next_byte(sgmCPU* cpu) {
	return cpu->ram[SGM_LOC_PROGRAM + (cpu->pc++)] & 0xFF;
}

static sgmIFMT cpu_fetchI(sgmCPU* cpu) {
	sgmIFMT fmt;
	fmt.dst = sgm_cpu_next_byte(cpu);
	fmt.imm = sgm_cpu_next_byte(cpu) << 8 | sgm_cpu_next_byte(cpu);
	return fmt;
}

static sgmRFMT cpu_fetchR(sgmCPU* cpu) {
	sgmRFMT fmt;
	fmt.src = sgm_cpu_next_byte(cpu);
	fmt.dst = sgm_cpu_next_byte(cpu);
	sgm_cpu_next_byte(cpu);
	return fmt;
}

static sgmJFMT cpu_fetchJ(sgmCPU* cpu) {
	sgmJFMT fmt;
	sgm_cpu_next_byte(cpu);
	sgmByte a = sgm_cpu_next_byte(cpu);
	sgmByte b = sgm_cpu_next_byte(cpu);
	fmt.addr = a << 8 | b;
	return fmt;
}

SGM_DEF_INSTR(sys) { // sys call (performs a system call)
	sgmIFMT fmt = cpu_fetchI(cpu);
	cpu->ram[SGM_LOC_SYSCALL] = (fmt.imm & 0xFF);
}

/// LOAD FUNCTIONS
SGM_DEF_INSTR(ldi) { // ldi dest, imm (load immediate to register)
	sgmIFMT fmt = cpu_fetchI(cpu);
	cpu->registers[fmt.dst] = fmt.imm;
}

SGM_DEF_INSTR(ldm) { // ldm dest, mem (load value from memory to register)
	sgmIFMT fmt = cpu_fetchI(cpu);
	cpu->registers[fmt.dst] = cpu->ram[fmt.imm];
}

/// STORE FUNCTIONS
SGM_DEF_INSTR(stm) { // stm src, mem (store register value into memory)
	sgmIFMT fmt = cpu_fetchI(cpu);
	cpu->ram[fmt.imm] = cpu->registers[fmt.dst];
}

SGM_DEF_INSTR(mov) { // mov dest, src (move/copy a value from a register to another)
	sgmRFMT fmt = cpu_fetchR(cpu);
	cpu->registers[fmt.dst] = cpu->registers[fmt.src];
}

/// Math
#define SGM_DEF_BINOP(name, op) \
	SGM_DEF_INSTR(name) { \
		sgmRFMT fmt = cpu_fetchR(cpu); \
		cpu->registers[fmt.dst] op##= cpu->registers[fmt.src]; \
	} \
	SGM_DEF_INSTR(name##i) { \
		sgmIFMT fmt = cpu_fetchI(cpu); \
		cpu->registers[fmt.dst] op##= fmt.imm; \
	}

SGM_DEF_INSTR(sub) {
	sgmRFMT fmt = cpu_fetchR(cpu);
	cpu->registers[fmt.dst] -= cpu->registers[fmt.src];
}
SGM_DEF_INSTR(subi) {
	sgmIFMT fmt = cpu_fetchI(cpu);
	cpu->registers[fmt.dst]-= fmt.imm;
}

SGM_DEF_INSTR(mul) {
	sgmRFMT fmt = cpu_fetchR(cpu);
	cpu->registers[fmt.dst] *= cpu->registers[fmt.src];
}
SGM_DEF_INSTR(muli) {
	sgmIFMT fmt = cpu_fetchI(cpu);
	cpu->registers[fmt.dst]*= fmt.imm;
}

SGM_DEF_BINOP(add, +)
//SGM_DEF_BINOP(sub, -)
//SGM_DEF_BINOP(mul, *)
SGM_DEF_BINOP(div, /)
SGM_DEF_BINOP(and, &)
SGM_DEF_BINOP(or, |)
SGM_DEF_BINOP(xor, ^)
SGM_DEF_BINOP(lsh, <<)
SGM_DEF_BINOP(rsh, >>)

/// Control flow
SGM_DEF_INSTR(cmp) {
	sgmRFMT fmt = cpu_fetchR(cpu);
	sgmWord b = cpu->registers[fmt.dst];
	sgmWord a = cpu->registers[fmt.src];
	if (a > b) {
		cpu->flag = SGM_FLAG_GREATER;
	} else if (a < b) {
		cpu->flag = SGM_FLAG_LESS;
	} else if (a == b) {
		cpu->flag = SGM_FLAG_EQUAL;
	}
}

SGM_DEF_INSTR(cmpi) {
	sgmIFMT fmt = cpu_fetchI(cpu);
	sgmWord a = cpu->registers[fmt.dst];
	sgmWord b = fmt.imm;
	if (a > b) {
		cpu->flag = SGM_FLAG_GREATER;
	} else if (a < b) {
		cpu->flag = SGM_FLAG_LESS;
	} else if (a == b) {
		cpu->flag = SGM_FLAG_EQUAL;
	}
}

#define SGM_DEF_COND(name, cond) \
	SGM_DEF_INSTR(name) {\
		sgmJFMT fmt = cpu_fetchJ(cpu); \
		sgmByte F = cpu->flag; \
		if (cond) cpu->pc = fmt.addr; \
	}

SGM_DEF_COND(jmp, true)
SGM_DEF_COND(jeq, F == SGM_FLAG_EQUAL)
SGM_DEF_COND(jne, F != SGM_FLAG_EQUAL)
SGM_DEF_COND(jgt, F == SGM_FLAG_GREATER)
SGM_DEF_COND(jlt, F == SGM_FLAG_LESS)
SGM_DEF_COND(jge, F == SGM_FLAG_GREATER || F == SGM_FLAG_EQUAL)
SGM_DEF_COND(jle, F == SGM_FLAG_LESS || F == SGM_FLAG_EQUAL)

SGM_DEF_INSTR(out) {
	sgmIFMT fmt = cpu_fetchI(cpu);
	printf("%d", cpu->registers[fmt.imm]);
}

SGM_DEF_INSTR(call) {
	sgmJFMT fmt = cpu_fetchJ(cpu);
	cpu->stack[cpu->sp++] = cpu->pc;
	cpu->pc = fmt.addr;
}

SGM_DEF_INSTR(ret) {
	assert(cpu->sp > 0 && "Invalid return.");
	cpu->pc = cpu->stack[--cpu->sp];
}

static const sgmInstruction SGM_INSTRUCTIONS[] = {
	//// General purpose instructions
	{ "sys",			 sgm_instr_sys },
	{ "ldi",			 sgm_instr_ldi },
	{ "ldm",			 sgm_instr_ldm },
	{ "stm",			 sgm_instr_stm },
	{ "mov",			 sgm_instr_mov },
	//// Math instructions
	{ "add",			 sgm_instr_add },
	{ "addi",			 sgm_instr_addi },
	{ "sub",			 sgm_instr_sub },
	{ "subi",			 sgm_instr_subi },
	{ "mul",			 sgm_instr_mul },
	{ "muli",			 sgm_instr_muli },
	{ "div",			 sgm_instr_div },
	{ "divi",			 sgm_instr_divi },
	{ "and",			 sgm_instr_and },
	{ "andi",			 sgm_instr_andi },
	{ "or",				 sgm_instr_or },
	{ "ori",			 sgm_instr_ori },
	{ "xor",			 sgm_instr_xor },
	{ "xori",			 sgm_instr_xori },
	{ "lsh",			 sgm_instr_lsh },
	{ "lshi",			 sgm_instr_lshi },
	{ "rsh",			 sgm_instr_rsh },
	{ "rshi",			 sgm_instr_rshi },
	//// Control flow
	{ "cmp",			 sgm_instr_cmp },
	{ "cmpi",			 sgm_instr_cmpi },
	{ "jmp",			 sgm_instr_jmp },
	{ "jeq",			 sgm_instr_jeq },
	{ "jne",			 sgm_instr_jne },
	{ "jgt",			 sgm_instr_jgt },
	{ "jlt",			 sgm_instr_jlt },
	{ "jge",			 sgm_instr_jge },
	{ "jle",			 sgm_instr_jle },
	{ "call",			 sgm_instr_call },
	{ "ret",			 sgm_instr_ret },
	//// Other
	{ "out",			 sgm_instr_out },
	{ "", NULL }
};

sgmCPU* sgm_cpu_new() {
	sgmCPU* cpu = (sgmCPU*) malloc(sizeof(sgmCPU));
	memset(cpu->registers, 0, sizeof(sgmWord) * RCount);
	memset(cpu->stack, 0, sizeof(sgmWord) * 32);
	memset(cpu->ram, 0, sizeof(sgmByte) * SGM_RAM_SIZE);
	cpu->pc = cpu->sp = 0;
	cpu->flag = 0;
	return cpu;
}

void sgm_cpu_free(sgmCPU* cpu) {}

void sgm_cpu_tick(sgmCPU* cpu) {
	sgmByte opcode = sgm_cpu_next_byte(cpu);
//	printf("EXEC. INSTR: %s\n", SGM_INSTRUCTIONS[opcode].name);
	SGM_INSTRUCTIONS[opcode].exec(cpu);
}

void sgm_cpu_load(sgmCPU* cpu, sgmByte* program, sgmWord n) {
	memcpy(cpu->ram + SGM_LOC_PROGRAM, program, sizeof(sgmByte) * n);
	memset(cpu->registers, 0, sizeof(sgmWord) * RCount);
	cpu->pc = 0;
	cpu->sp = 0;
	cpu->flag = 0;
}

void sgm_cpu_run(sgmCPU* cpu) {
	while (cpu->ram[SGM_LOC_SYSCALL] != SGM_SYSCALL_STOP) {
		cpu->ram[SGM_LOC_SYSCALL] = 0;
		sgm_cpu_tick(cpu);
		if (cpu->ram[SGM_LOC_SYSCALL] == SGM_SYSCALL_RESET) {
			memset(cpu->registers, 0, sizeof(sgmWord) * RCount);
			cpu->pc = cpu->sp = 0;
			cpu->flag = 0;
		}
		if (cpu->ram[SGM_LOC_SYSCALL] == SGM_SYSCALL_PRINT) {
			sgmWord param = cpu->registers[0xF];
			char c = cpu->ram[param];
			while (c != 0) {
				printf("%c", c);
				c = cpu->ram[++param];
			}
		}
//		printf("[ ");
//		for (sgmInt i = 0; i < RCount; i++) {
//			printf("%04xh, ", cpu->registers[i]);
//		}
//		printf("]\n");
	}
}

sgmByte sgm_get_op(const char* name) {
	sgmInt isz = sizeof(SGM_INSTRUCTIONS) / sizeof(sgmInstruction);
	for (sgmInt i = 0; i < isz; i++) {
		if (strcmp(SGM_INSTRUCTIONS[i].name, name) == 0)
			return i;
	}
	return 0;
}

const char* sgm_get_op_name(sgmByte op) {
	return SGM_INSTRUCTIONS[op].name;
}
