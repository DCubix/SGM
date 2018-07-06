#ifndef SGM_CPU_H
#define SGM_CPU_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

#include "sgm_mem.h"

#define SGM_VIDEO_WIDTH 128
#define SGM_VIDEO_HEIGHT 96

#define SGM_PROGRAM_MAX_SIZE 0xC00 // 3KB
#define SGM_GENERAL_PURPOSE_SIZE 0x400 // 1KB
#define SGM_VIDEO_SIZE (SGM_VIDEO_WIDTH * SGM_VIDEO_HEIGHT) // 12 KB
#define SGM_RAM_SIZE (SGM_PROGRAM_MAX_SIZE + SGM_GENERAL_PURPOSE_SIZE + SGM_VIDEO_SIZE + 3)

#define SGM_LOC_PROGRAM 0
#define SGM_LOC_GENERAL_PURPOSE SGM_PROGRAM_MAX_SIZE
#define SGM_LOC_VIDEO (SGM_LOC_GENERAL_PURPOSE + SGM_GENERAL_PURPOSE_SIZE)
#define SGM_LOC_SYSCALL (SGM_LOC_VIDEO + SGM_VIDEO_SIZE)
#define SGM_LOC_SYSCALL_PARAM (SGM_LOC_SYSCALL+1)
#define SGM_LOC_SYSCALL_PARAM_H (SGM_LOC_SYSCALL_PARAM+1)

#define SGM_SYSCALL_STOP			0x1
#define SGM_SYSCALL_RESET			0x2
#define SGM_SYSCALL_VIDEO_CLEAR		0x3
#define SGM_SYSCALL_DRAW_SPRITE		0x4
#define SGM_SYSCALL_PRINT			0x5
#define SGM_SYSCALL_FLIP			0x6

#define SGM_FLAG_EQUAL		0x1
#define SGM_FLAG_GREATER	0x2
#define SGM_FLAG_LESS		0x3

#define SGM_LINE(x) (x * sizeof(sgmInt))

#define SGM_DEF_INSTR(name) static void sgm_instr_##name(sgmCPU* cpu)

sgmByte sgm_get_op(const char* name);
const char* sgm_get_op_name(sgmByte op);

#define MINT(a, b, c, d) (sgmInt)(a << 24 | b << 16 | c << 8 | d)
#define MINTs(a, b, c) MINT((a & 0xFF), (a & 0xFF00)>>8, b, c)

#define OPI(name, dst, imm) MINTs(imm, dst, sgm_get_op(name))
#define OPO(name, imm) OPI(name, 0, imm)
#define OPR(name, dst, src) MINT(src, dst, 0, sgm_get_op(name))
#define OPJ(name, addr) MINTs(addr, 0, sgm_get_op(name))
#define OPN(name) MINT(sgm_get_op(name), 0, 0, 0)

// I Format
// ===========================================
// [ OP CODE ][ REGISTER ][ IMM ]
//    8bit        8bit     16bit
//
// R Format
// ===========================================
// [ OP CODE ][ REGISTER ][ REGISTER ]
//    8bit        8bit       8bit
//
// J Format
// ===========================================
// [ OP CODE ][ IMM ]
//    8bit     16bit

typedef struct sgm_ifmt_t {
	sgmByte dst;
	sgmWord imm;
} sgmIFMT;

typedef struct sgm_rfmt_t {
	sgmByte dst;
	sgmByte src;
} sgmRFMT;

typedef struct sgm_jfmt_t {
	sgmWord addr;
} sgmJFMT;

enum sgmRegisters {
	R0 = 0,
	R1,
	R2,
	R3,
	R4,
	R5,
	R6,
	R7,
	R8,
	R9,
	R10,
	R11,
	R12,
	R13,
	R14,
	R15,
	RCount
};

typedef struct sgm_cpu_t {
	sgmByte ram[SGM_RAM_SIZE];
	sgmWord registers[RCount];
	sgmWord stack[32];

	sgmWord pc, sp;
	sgmByte flag;

	bool flip, stop;
} sgmCPU;

typedef struct sgm_instruction_t {
	char name[6];
	void (*exec)(sgmCPU*);
} sgmInstruction;

sgmCPU* sgm_cpu_new();
void sgm_cpu_free(sgmCPU* cpu);

void sgm_cpu_load(sgmCPU* cpu, sgmByte* program, sgmWord n);

void sgm_cpu_tick_base(sgmCPU* cpu);
void sgm_cpu_tick(sgmCPU* cpu);
void sgm_cpu_run(sgmCPU* cpu);

#endif // SGM_CPU_H
