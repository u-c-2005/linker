/**
 * Project 2
 * LC-2K Linker
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAXSIZE 500
#define MAXLINELENGTH 1000
#define MAXFILES 6

static inline void printHexToFile(FILE *, int);
static int endsWith(char *, char *);

typedef struct FileData FileData;
typedef struct SymbolTableEntry SymbolTableEntry;
typedef struct RelocationTableEntry RelocationTableEntry;
typedef struct CombinedFiles CombinedFiles;

typedef struct {
    char label[MAXLINELENGTH];
    int address;
} GlobalSymbol;

GlobalSymbol globalSymbols[MAXLINELENGTH];
int global_symbol_count = 0;
int text_base[MAXFILES];
int data_base[MAXFILES];


struct SymbolTableEntry {
	char label[7];
	char location;
	unsigned int offset;
};

struct RelocationTableEntry {
    unsigned int file;
	unsigned int offset;
	char inst[6];
	char label[7];
};

struct FileData {
	unsigned int textSize;
	unsigned int dataSize;
	unsigned int symbolTableSize;
	unsigned int relocationTableSize;
	unsigned int textStartingLine; // in final executable
	unsigned int dataStartingLine; // in final executable
	int text[MAXSIZE];
	int data[MAXSIZE];
	SymbolTableEntry symbolTable[MAXSIZE];
	RelocationTableEntry relocTable[MAXSIZE];
};

struct CombinedFiles {
	unsigned int textSize;
	unsigned int dataSize;
	unsigned int symbolTableSize;
	unsigned int relocationTableSize;
	int text[MAXSIZE * MAXFILES];
	int data[MAXSIZE * MAXFILES];
	SymbolTableEntry symbolTable[MAXSIZE * MAXFILES];
	RelocationTableEntry relocTable[MAXSIZE * MAXFILES];
};

//helper
int find_global_symbol (char *label){
	for(int i = 0; i < global_symbol_count; i++){
		if(!strcmp(globalSymbols[i].label, label)) {
			return i;
		}
	}
	return -1;
}

int main(int argc, char *argv[]) {
	char *inFileStr, *outFileStr;
	FILE *inFilePtr, *outFilePtr; 
	unsigned int i, j;

    if (argc <= 2 || argc > 8 ) {
        printf("error: usage: %s <MAIN-object-file> ... <object-file> ... <machine-code-file>, with at most 6 object files\n",
				argv[0]);
		exit(1);
	}

	outFileStr = argv[argc - 1];
    if (endsWith(outFileStr, ".obj")) {
        printf("error: no machine code file given\n");
        exit(1);
    }

	outFilePtr = fopen(outFileStr, "w");
	if (outFilePtr == NULL) {
		printf("error in opening %s\n", outFileStr);
		exit(1);
	}

	FileData files[MAXFILES];

	// read in all files and combine into a "master" file
	for (i = 0; i < argc - 2; ++i) {
		inFileStr = argv[i+1];

		inFilePtr = fopen(inFileStr, "r");
		printf("opening %s\n", inFileStr);

		if (inFilePtr == NULL) {
			printf("error in opening %s\n", inFileStr);
			exit(1);
		}

		char line[MAXLINELENGTH];
		unsigned int textSize, dataSize, symbolTableSize, relocationTableSize;

		// parse first line of file
		fgets(line, MAXSIZE, inFilePtr);
		sscanf(line, "%u %u %u %u",
				&textSize, &dataSize, &symbolTableSize, &relocationTableSize);

		files[i].textSize = textSize;
		files[i].dataSize = dataSize;
		files[i].symbolTableSize = symbolTableSize;
		files[i].relocationTableSize = relocationTableSize;

		// read in text section
		int instr;
		for (j = 0; j < textSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			instr = strtol(line, NULL, 0);
			files[i].text[j] = instr;
		}

		// read in data section
		int data;
		for (j = 0; j < dataSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			data = strtol(line, NULL, 0);
			files[i].data[j] = data;
		}

		// read in the symbol table
		char label[7];
		char type;
		unsigned int addr;
		for (j = 0; j < symbolTableSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%s %c %u",
					label, &type, &addr);
			files[i].symbolTable[j].offset = addr;
			strcpy(files[i].symbolTable[j].label, label);
			files[i].symbolTable[j].location = type;
		}

		// read in relocation table
		char opcode[7];
		for (j = 0; j < relocationTableSize; ++j) {
			fgets(line, MAXLINELENGTH, inFilePtr);
			sscanf(line, "%u %s %s",
					&addr, opcode, label);
			files[i].relocTable[j].offset = addr;
			strcpy(files[i].relocTable[j].inst, opcode);
			strcpy(files[i].relocTable[j].label, label);
			files[i].relocTable[j].file	= i;
		}
		fclose(inFilePtr);
	} // end reading files

	// *** INSERT YOUR CODE BELOW ***
	//    Begin the linking process
	//    Happy coding!!!
	int num_files = argc - 2;

	int total_text = 0;
	int total_data = 0;

	// compute text bases
	for (int i = 0; i < num_files; i++) {
    	text_base[i] = total_text;
    	total_text += files[i].textSize;
	}

	// compute data bases
	for (int i = 0; i < num_files; i++) {
    	data_base[i] = total_text + total_data;
    	total_data += files[i].dataSize;
	}

	// add T + D symbols
	for (int i = 0; i < num_files; i++) {
    	for (int j = 0; j < files[i].symbolTableSize; j++) {
        	SymbolTableEntry s = files[i].symbolTable[j];
			if (s.location == 'T' || s.location == 'D') {
				int absolute_addy;
				if (s.location == 'T') {
                	absolute_addy = text_base[i] + s.offset;
            	}
				else{
					absolute_addy = data_base[i] + s.offset;
				}
				if(find_global_symbol(s.label) != -1){
					fprintf(stderr, "Error: duplicate symbol %s\n", s.label);
                	exit(1);
            	}	
				strcpy(globalSymbols[global_symbol_count].label, s.label);
				globalSymbols[global_symbol_count].address = absolute_addy;
				global_symbol_count++;
			}	
		}
	}

	//add stack symbol
	if (find_global_symbol("Stack") != -1) {
    fprintf(stderr, "Error: Stack cannot be defined\n");
    exit(1);
}
	strcpy(globalSymbols[global_symbol_count].label, "Stack");
	globalSymbols[global_symbol_count].address = total_text + total_data;
	global_symbol_count++;

	printf("GLOBAL SYMBOLS (%d):\n", global_symbol_count);
	for (int k = 0; k < global_symbol_count; k++) {
    	printf("  %s -> %d\n", globalSymbols[k].label, globalSymbols[k].address);
	}

	//undefined symbols
	for (int i = 0; i < num_files; i++) {
    	for (int j = 0; j < files[i].symbolTableSize; j++) {
        	SymbolTableEntry s = files[i].symbolTable[j];
        	if (s.location == 'U') {
            	if (find_global_symbol(s.label) == -1) {
                	fprintf(stderr, "Error: undefined symbol %s\n", s.label);
                	exit(1);
            	}
        	}
    	}
	}

	//relocation
	for (int i = 0; i < num_files; i++) {
    	for (int j = 0; j < files[i].relocationTableSize; j++) {
       	 RelocationTableEntry r = files[i].relocTable[j];

        	int symbol_addy = 0;
			int symbol_index = find_global_symbol(r.label);
			if (symbol_index != -1) {
    			//if global
    			symbol_addy = globalSymbols[symbol_index].address;
			}
			else {
				//if local 
				if (!strncmp(r.inst, ".fill", 5)) {
        			int relative_addy = files[i].data[r.offset];  
        			if (relative_addy < (int)files[i].textSize) {
            			symbol_addy = text_base[i] + relative_addy;
        			} 
					else {
            			symbol_addy = data_base[i] + (relative_addy - (int)files[i].textSize);
        			}
    			}
				else {
        			int *instr = &files[i].text[r.offset];
       	 			int imm = *instr & 0xFFFF;
					// sign extend
        			if (imm & 0x8000) imm |= 0xFFFF0000; 
        				if (!strcmp(r.inst, "beq")) {
            				//pc-relative so not necessary
            				int targetRel = (int)r.offset + 1 + imm;
            				symbol_addy = text_base[i] + targetRel;
        				} 
						else {
            				// lw or sw
            				int relative_addy = imm;
            				if (relative_addy < (int)files[i].textSize) {
                				symbol_addy = text_base[i] + relative_addy;
            				} 
							else {
                				symbol_addy = data_base[i] + (relative_addy - (int)files[i].textSize);
            				}
        				}
    			}
			}
			if (!strncmp(r.inst, ".fill", 5)) {
    			files[i].data[r.offset] = symbol_addy;
			}
			else {
    			int *instr = &files[i].text[r.offset];
    			int opcode = (*instr >> 22) & 0x7;
    			int instr_addy = text_base[i] + (int)r.offset;
    			int new_immediate;
				if (opcode == 4) { // beq
        			new_immediate = symbol_addy - (instr_addy + 1);
    			} 
				else { // lw/sw
        			new_immediate = symbol_addy;
    			}
    			*instr = (*instr & 0xFFFF0000) | (new_immediate & 0xFFFF);
			}
		}
	}

	//output machine code


for (int i = 0; i < num_files; i++) {
    for (int j = 0; j < files[i].textSize; j++) {
        printHexToFile(outFilePtr, files[i].text[j]);
    }
}

for (int i = 0; i < num_files; i++) {
    for (int j = 0; j < files[i].dataSize; j++) {
        printHexToFile(outFilePtr, files[i].data[j]);
    }
}



    /* here is an example of using printHexToFile. This will print a
       machine code word / number in the proper hex format to the output file */
   // printHexToFile(outFilePtr, 123);

} // main

// Prints a machine code word in the proper hex format to the file
static inline void 
printHexToFile(FILE *outFilePtr, int word) {
    fprintf(outFilePtr, "0x%08X\n", word);
}

// Returns 1 if string ends with substr, 0 otherwise
static int
endsWith(char *string, char *substr) {
    size_t stringLen = strlen(string);
    size_t substrLen = strlen(substr);
    if (stringLen < substrLen) {
        return 0; // string too short
    }
    char *stringEnd = string + stringLen - substrLen;
    if (strcmp(stringEnd, substr) == 0) {
        return 1;
    }
    return 0;
}
