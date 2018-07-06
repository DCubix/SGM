#include <stdio.h>
#include <string.h>

#include "sgm.h"
#include "sgm_cpu.h"
#include "sgm_compiler.h"

int main(int argc, const char** argv) {
	if (argc == 1) {
help:
		printf("HOW TO USE SGM:\n");
		printf("\tsgm <option> <in> [<out>]\n");
		printf("WHERE <option> IS:\n");
		printf("\t-c:\tCOMPILE\n");
		printf("\t-r:\tRUN FILE DIRECTLY\n");
		printf("\t-b:\tRUN BINARY FILE\n");
		return 0;
	}

	if (strcmp(argv[1], "-r") == 0) {
		if (argc != 3) {
			printf("INVALID ARGUMENTS.\n");
			goto help;
		}
		sgm_run_file(argv[2]);
	} else if (strcmp(argv[1], "-c") == 0) {
		if (argc != 3) {
			printf("INVALID ARGUMENTS.\n");
			goto help;
		}
		sgmCPU* cpu = sgm_cpu_new();
		sgm_compiler_load(cpu, argv[2]);

		char* out = (char*) calloc(strlen(argv[2]), sizeof(char));
		strcpy(out, argv[2]);
		int i = strlen(out);
		while (out[i] != '.')
			out[i--] = 0;
		out[++i] = 's';
		out[++i] = 'b';

		FILE* fp = fopen(out, "wb");
		if (fp) {
			fwrite(cpu->ram, sizeof(sgmByte), SGM_RAM_SIZE, fp);
			fclose(fp);
		}

		sgm_cpu_free(cpu);
	} else if (strcmp(argv[1], "-b") == 0) {
		if (argc != 3) {
			printf("INVALID ARGUMENTS.\n");
			goto help;
		}
		FILE* fp = fopen(argv[2], "rb");
		if (fp) {
			fseek(fp, 0, SEEK_END);
			long sz = ftell(fp);
			rewind(fp);

			sgmByte* program = (sgmByte*) calloc(sz, sizeof(sgmByte));
			fread(program, sizeof(sgmByte), sz, fp);
			fclose(fp);

			sgm_run(program, sz);
		} else {
			printf("FILE NOT FOUND.\n");
		}
	} else {
		goto help;
	}

	return 0;
}
