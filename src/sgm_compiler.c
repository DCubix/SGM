#include "sgm_compiler.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

enum sgmTokenType {
	sgmTokInvalid = 0,
	sgmTokLabel,
	sgmTokLabelPtr,
	sgmTokLabelNew,
	sgmTokInstruction,
	sgmTokNumber,
	sgmTokRegister,

	sgmTokDataSection,
	sgmTokTextSection
};

typedef struct sgm_token_t {
	char name[32];
	sgmByte type;
	sgmWord value;
} sgmToken;

typedef struct sgm_label_t {
	char name[32];
	sgmWord addr;
} sgmLabel;

static sgmLabel LABELS[256] = { "", 0 };
static sgmWord CURRENT_LABEL = 0;

static sgmWord DATA_POINTER = 0;

int atoix(char *str) {
	int val;
	if (strncmp(str, "0x", 2) == 0)
		sscanf(str, "%x", &val);
	else
		val = atoi(str);
	return val;
}

int hex2int(char ch) {
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	return -1;
}

int sgm_compiler_find_label(const char* name) {
	for (sgmWord i = 0; i < CURRENT_LABEL; i++) {
		if (strcmp(LABELS[i].name, name) == 0) return LABELS[i].addr;
	}
	return -1;
}

bool sgm_compiler_add_label(const char* name, sgmByte type, sgmWord addr) {
	if (sgm_compiler_find_label(name) >= 0) {
		fprintf(stderr, "ERROR: Label redefinition!\n");
		return false;
	}
	sgmLabel* lbl = &LABELS[CURRENT_LABEL++];
	strncpy(lbl->name, name, 32);
	if (type == sgmTokDataSection)
		lbl->addr = SGM_LOC_GENERAL_PURPOSE + addr;
	else
		lbl->addr = SGM_LOC_PROGRAM + addr;

	return true;
}

sgmWord sgm_compiler_add_data(char* tok, char* buf, sgmWord off, sgmByte* memory) {
	const char* delim = " ,\r\n";
	char* item;

	sgmWord addr = SGM_LOC_GENERAL_PURPOSE + off;

	if (strcmp(tok, ".ascii") == 0 || strcmp(tok, ".asciiz") == 0) {
		int beg = -1, end = -1;
		for (sgmWord i = 0; i < strlen(buf); i++) {
			if (buf[i] == '\"' && beg < 0) beg = i;
			else end = i;
		}

		for (sgmWord i = beg+1; i < end-1; i++) {
			if (buf[i] == '\\') {
				switch (buf[i+1]) {
					case 't': memory[addr++] = '\t'; break;
					case 'r': memory[addr++] = '\r'; break;
					case 'n': memory[addr++] = '\n'; break;
					case '"': memory[addr++] = '"'; break;
					case '\'': memory[addr++] = '\''; break;
					default: break;
				}
				i++;
			} else {
				memory[addr++] = buf[i];
			}
		}

		if (strcmp(tok, ".asciiz") == 0)
			memory[addr++] = '\0';
	} else if (strcmp(tok, ".word") == 0) {
		item = strtok(buf, delim);
		while (item != NULL) {
			int data = 0;

			char* p = strtok(item, " \t+");

			while (p) {
				int nd = 0;
				if ((nd = sgm_compiler_find_label(p)) < 0) {
					nd = atoix(p);
				}
				data += nd;
				p = strtok(NULL, "+");
			}

			memory[addr++] = (data)			& 0xFF;
			memory[addr++] = (data >> 8)	& 0xFF;
			item = strtok(NULL, delim);
		}
	} else if (strcmp(tok, ".byte") == 0) {
		item = strtok(buf, delim);
		while (item != NULL) {
			memory[addr++] = atoix(item);
			item = strtok(NULL, delim);
		}
	} else if (strcmp(tok, ".space") == 0) {
		item = strtok(buf, delim);
		addr += atoix(item);
	}

	return addr - (SGM_LOC_GENERAL_PURPOSE + off);
}

void sgm_compiler_add_instruction(const char* inst, const char* operands, int off, sgmByte* memory) {
	char* delim = " ,\t\r\n";
	char* op;

	int addr = SGM_LOC_PROGRAM + off;

	memory[addr++] = sgm_get_op(inst);

	sgmToken toks[4] = { 0 };
	sgmWord t = 0;

	op = strtok(operands, delim);
	while (op) {
		if (op[0] == ';') {
			op = strtok(NULL, "\n");
			op = strtok(NULL, delim);
		} else {
			sgmToken* tok = &toks[t++];
			if (op[0] == '$') {
				tok->type = sgmTokRegister;
				if (isalpha(op[1])) {
					tok->value = hex2int(op[1]);
				} else {
					tok->value = atoix(&op[1]);
				}
			} else if (isdigit(op[0])) {
				tok->type = sgmTokNumber;
				tok->value = atoix(op);
			} else if (isalpha(op[0])) {
				tok->type = sgmTokLabel;
				tok->value = sgm_compiler_find_label(op);
			} else if (op[0] == '&') {
				tok->type = sgmTokLabelPtr;

				char lbl[32] = {0};
				strcpy(lbl, &op[1]);
				tok->value = sgm_compiler_find_label(lbl);
			}
			strcpy(tok->name, op);
			op = strtok(NULL, delim);
		}
	}

	if (t == 1) { // J Format
		addr++;
		memory[addr++] = (toks[0].value & 0xFF00) >> 8;
		memory[addr++] = (toks[0].value & 0x00FF);
	} else if (t == 2) { // R Format / I Format
		sgmToken a = toks[0], b = toks[1];
		if (b.type == sgmTokRegister) {
			memory[addr++] = b.value;
			memory[addr++] = a.value;
			addr++;
		} else if (b.type == sgmTokNumber) {
			memory[addr++] = a.value;
			memory[addr++] = (b.value & 0xFF00) >> 8;
			memory[addr++] = (b.value & 0x00FF);
		} else if (b.type == sgmTokLabel) {
			sgmWord val = memory[b.value];
			memory[addr++] = a.value;
			memory[addr++] = (val & 0xFF00) >> 8;
			memory[addr++] = (val & 0x00FF);
		} else if (b.type == sgmTokLabelPtr) {
			sgmWord val = b.value;
			memory[addr++] = a.value;
			memory[addr++] = (val & 0xFF00) >> 8;
			memory[addr++] = (val & 0x00FF);
		}
	}
}

char *sgets(char* str, int num, char** input) {
	char* next = *input;
	int  numread = 0;

	while ( numread + 1 < num && *next ) {
		int isnewline = ( *next == '\n' );
		*str++ = *next++;

		// newline terminates the line but is included
		if ( isnewline )
			break;
		numread++;
	}

	if ( numread == 0 )
		return NULL;  // "eof"

	// must have hit the null terminator or end of line
	*str = '\0';  // null terminate this tring
	// set up input for next call
	*input = next;
	return str;
}

static void sgm_compiler_parse(sgmCPU* cpu, const char* fileName) {
	const char* delim = " \r\t\n";

	char* tok;
	sgmByte curSeg = sgmTokTextSection;

	sgmWord textOff = 0, dataOff = 0;

	sgmWord line = 0;
	char p[BUFSIZ] = { 0 };

	bool prevLabel = false;

	FILE* fp = fopen(fileName, "r");
	if (!fp) return;

	// Pre-Defined labels
	sgm_compiler_add_label("_screen", sgmTokDataSection, dataOff);
	cpu->ram[SGM_LOC_GENERAL_PURPOSE + dataOff++] = (SGM_LOC_VIDEO)			& 0xFF;
	cpu->ram[SGM_LOC_GENERAL_PURPOSE + dataOff++] = (SGM_LOC_VIDEO >> 8)	& 0xFF;

	sgm_compiler_add_label("_gp", sgmTokDataSection, dataOff);
	cpu->ram[SGM_LOC_GENERAL_PURPOSE + dataOff++] = (SGM_LOC_GENERAL_PURPOSE)			& 0xFF;
	cpu->ram[SGM_LOC_GENERAL_PURPOSE + dataOff++] = (SGM_LOC_GENERAL_PURPOSE >> 8)		& 0xFF;

	sgm_compiler_add_label("_w", sgmTokDataSection, dataOff);
	cpu->ram[SGM_LOC_GENERAL_PURPOSE + dataOff++] = (SGM_VIDEO_WIDTH)	& 0xFF;

	sgm_compiler_add_label("_h", sgmTokDataSection, dataOff);
	cpu->ram[SGM_LOC_GENERAL_PURPOSE + dataOff++] = (SGM_VIDEO_HEIGHT) & 0xFF;

	// Pass 1: Variable and Labels
	while (fgets(p, BUFSIZ, fp)) {
		for (tok = strtok(p, delim); tok; tok = strtok(NULL, delim)) {
			if (tok[0] == ';')
				break;

			if (strcmp(tok, ".text") == 0) {
				curSeg = sgmTokTextSection;
				break;
			} else if (strcmp(tok, ".data") == 0) {
				curSeg = sgmTokDataSection;
				break;
			}

			if (strcmp(tok, ".globl") == 0)
				break;

			// LABEL
			if (tok[strlen(tok)-1] == ':') {
				tok[strlen(tok)-1] = 0;
				sgm_compiler_add_label(tok, curSeg, curSeg == sgmTokTextSection ? textOff : dataOff);
				if (curSeg == sgmTokTextSection)
					prevLabel = true;
				continue;
			}

			// VARIABLE
			if (tok[0] == '.') {
				dataOff += sgm_compiler_add_data(tok, tok+strlen(tok)+1, dataOff, cpu->ram);
				break;
			} else {
				if (prevLabel) {
					textOff += SGM_LINE(1);
				}
				break;
			}
		}
		line++;
	}

	rewind(fp);
	memset(p, 0, BUFSIZ);
	textOff = 0;

	// Pass 2: Actual Parsing
	while (fgets(p, BUFSIZ, fp)) {
		for (tok = strtok(p, delim); tok; tok = strtok(NULL, delim)) {
			if (tok[0] == ';')
				break;

			if (strcmp(tok, ".text") == 0) {
				curSeg = sgmTokTextSection;
				break;
			} else if (strcmp(tok, ".data") == 0) {
				curSeg = sgmTokDataSection;
				break;
			}

			if (strcmp(tok, ".globl") == 0)
				break;

			// Skip Data
			if (curSeg == sgmTokDataSection)
				break;

			// Skip New Labels
			if (tok[strlen(tok)-1] == ':')
				continue;

			// Instruction
			sgm_compiler_add_instruction(tok, tok+strlen(tok)+1, textOff, cpu->ram);
			textOff += SGM_LINE(1);
		}
		memset(p, 0, BUFSIZ);
	}

	fclose(fp);

	// Check
//	for (sgmWord i = 0; i < textOff; i+=4) {
//		printf("%s ", sgm_get_op_name(cpu->ram[i+0]));
//		printf("%02d ", cpu->ram[i+1]);
//		printf("%02d ", cpu->ram[i+2]);
//		printf("%02d ", cpu->ram[i+3]);
//		printf("\n");
//	}
}

void sgm_compiler_load(sgmCPU* cpu, const char* fileName) {
	memset(LABELS, 0, sizeof(sgmLabel) * 256);
	CURRENT_LABEL = 0;

	memset(cpu->ram, 0, sizeof(sgmByte) * SGM_RAM_SIZE);
	sgm_compiler_parse(cpu, fileName);
}
