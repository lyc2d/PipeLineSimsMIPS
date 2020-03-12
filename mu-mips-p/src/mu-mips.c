#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

#include "mu-mips.h"

/***************************************************************/
/* Print out a list of commands available                                                                  */
/***************************************************************/
void help() {        
	printf("------------------------------------------------------------------\n\n");
	printf("\t**********MU-MIPS Help MENU**********\n\n");
	printf("sim\t-- simulate program to completion \n");
	printf("run <n>\t-- simulate program for <n> instructions\n");
	printf("rdump\t-- dump register values\n");
	printf("reset\t-- clears all registers/memory and re-loads the program\n");
	printf("input <reg> <val>\t-- set GPR <reg> to <val>\n");
	printf("mdump <start> <stop>\t-- dump memory from <start> to <stop> address\n");
	printf("high <val>\t-- set the HI register to <val>\n");
	printf("low <val>\t-- set the LO register to <val>\n");
	printf("print\t-- print the program loaded into memory\n");
	printf("show\t-- print the current content of the pipeline registers\n");
	printf("?\t-- display help menu\n");
	printf("quit\t-- exit the simulator\n\n");
	printf("------------------------------------------------------------------\n\n");
}

/***************************************************************/
/* Read a 32-bit word from memory                                                                            */
/***************************************************************/
uint32_t mem_read_32(uint32_t address)
{
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) &&  ( address <= MEM_REGIONS[i].end) ) {
			uint32_t offset = address - MEM_REGIONS[i].begin;
			return (MEM_REGIONS[i].mem[offset+3] << 24) |
					(MEM_REGIONS[i].mem[offset+2] << 16) |
					(MEM_REGIONS[i].mem[offset+1] <<  8) |
					(MEM_REGIONS[i].mem[offset+0] <<  0);
		}
	}
	return 0;
}

/***************************************************************/
/* Write a 32-bit word to memory                                                                                */
/***************************************************************/
void mem_write_32(uint32_t address, uint32_t value)
{
	int i;
	uint32_t offset;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		if ( (address >= MEM_REGIONS[i].begin) && (address <= MEM_REGIONS[i].end) ) {
			offset = address - MEM_REGIONS[i].begin;

			MEM_REGIONS[i].mem[offset+3] = (value >> 24) & 0xFF;
			MEM_REGIONS[i].mem[offset+2] = (value >> 16) & 0xFF;
			MEM_REGIONS[i].mem[offset+1] = (value >>  8) & 0xFF;
			MEM_REGIONS[i].mem[offset+0] = (value >>  0) & 0xFF;
		}
	}
}

/***************************************************************/
/* Execute one cycle                                                                                                              */
/***************************************************************/
void cycle() {                                                
	handle_pipeline();
	CURRENT_STATE = NEXT_STATE;
	CYCLE_COUNT++;
}

/***************************************************************/
/* Simulate MIPS for n cycles                                                                                       */
/***************************************************************/
void run(int num_cycles) {                                      
	
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped\n\n");
		return;
	}

	printf("Running simulator for %d cycles...\n\n", num_cycles);
	int i;
	for (i = 0; i < num_cycles; i++) {
		if (RUN_FLAG == FALSE) {
			printf("Simulation Stopped.\n\n");
			break;
		}
		cycle();
	}
}

/***************************************************************/
/* simulate to completion                                                                                               */
/***************************************************************/
void runAll() {                                                     
	if (RUN_FLAG == FALSE) {
		printf("Simulation Stopped.\n\n");
		return;
	}

	printf("Simulation Started...\n\n");
	while (RUN_FLAG){
		cycle();
	}
	printf("Simulation Finished.\n\n");
}

/***************************************************************/ 
/* Dump a word-aligned region of memory to the terminal                              */
/***************************************************************/
void mdump(uint32_t start, uint32_t stop) {          
	uint32_t address;

	printf("-------------------------------------------------------------\n");
	printf("Memory content [0x%08x..0x%08x] :\n", start, stop);
	printf("-------------------------------------------------------------\n");
	printf("\t[Address in Hex (Dec) ]\t[Value]\n");
	for (address = start; address <= stop; address += 4){
		printf("\t0x%08x (%d) :\t0x%08x\n", address, address, mem_read_32(address));
	}
	printf("\n");
}

/***************************************************************/
/* Dump current values of registers to the teminal                                              */   
/***************************************************************/
void rdump() {                               
	int i; 
	printf("-------------------------------------\n");
	printf("Dumping Register Content\n");
	printf("-------------------------------------\n");
	printf("# Instructions Executed\t: %u\n", INSTRUCTION_COUNT);
	printf("# Cycles Executed\t: %u\n", CYCLE_COUNT);
	printf("PC\t: 0x%08x\n", CURRENT_STATE.PC);
	printf("-------------------------------------\n");
	printf("[Register]\t[Value]\n");
	printf("-------------------------------------\n");
	for (i = 0; i < MIPS_REGS; i++){
		printf("[R%d]\t: 0x%08x\n", i, CURRENT_STATE.REGS[i]);
	}
	printf("-------------------------------------\n");
	printf("[HI]\t: 0x%08x\n", CURRENT_STATE.HI);
	printf("[LO]\t: 0x%08x\n", CURRENT_STATE.LO);
	printf("-------------------------------------\n");
}

/***************************************************************/
/* Read a command from standard input.                                                               */  
/***************************************************************/
void handle_command() {                         
	char buffer[20];
	uint32_t start, stop, cycles;
	uint32_t register_no;
	int register_value;
	int hi_reg_value, lo_reg_value;

	printf("MU-MIPS SIM:> ");

	if (scanf("%s", buffer) == EOF){
		exit(0);
	}

	switch(buffer[0]) {
		case 'S':
		case 's':
			if (buffer[1] == 'h' || buffer[1] == 'H'){
				show_pipeline();
			}else {
				runAll(); 
			}
			break;
		case 'M':
		case 'm':
			if (scanf("%x %x", &start, &stop) != 2){
				break;
			}
			mdump(start, stop);
			break;
		case '?':
			help();
			break;
		case 'Q':
		case 'q':
			printf("**************************\n");
			printf("Exiting MU-MIPS! Good Bye...\n");
			printf("**************************\n");
			exit(0);
		case 'R':
		case 'r':
			if (buffer[1] == 'd' || buffer[1] == 'D'){
				rdump();
			}else if(buffer[1] == 'e' || buffer[1] == 'E'){
				reset();
			}
			else {
				if (scanf("%d", &cycles) != 1) {
					break;
				}
				run(cycles);
			}
			break;
		case 'I':
		case 'i':
			if (scanf("%u %i", &register_no, &register_value) != 2){
				break;
			}
			CURRENT_STATE.REGS[register_no] = register_value;
			NEXT_STATE.REGS[register_no] = register_value;
			break;
		case 'H':
		case 'h':
			if (scanf("%i", &hi_reg_value) != 1){
				break;
			}
			CURRENT_STATE.HI = hi_reg_value; 
			NEXT_STATE.HI = hi_reg_value; 
			break;
		case 'L':
		case 'l':
			if (scanf("%i", &lo_reg_value) != 1){
				break;
			}
			CURRENT_STATE.LO = lo_reg_value;
			NEXT_STATE.LO = lo_reg_value;
			break;
		case 'P':
		case 'p':
			print_program(); 
			break;
		default:
			printf("Invalid Command.\n");
			break;
	}
}

/***************************************************************/
/* reset registers/memory and reload program                                                    */
/***************************************************************/
void reset() {   
	int i;
	/*reset registers*/
	for (i = 0; i < MIPS_REGS; i++){
		CURRENT_STATE.REGS[i] = 0;
	}
	CURRENT_STATE.HI = 0;
	CURRENT_STATE.LO = 0;
	
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
	
	/*load program*/
	load_program();
	
	/*reset PC*/
	INSTRUCTION_COUNT = 0;
	CURRENT_STATE.PC =  MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/***************************************************************/
/* Allocate and set memory to zero                                                                            */
/***************************************************************/
void init_memory() {                                           
	int i;
	for (i = 0; i < NUM_MEM_REGION; i++) {
		uint32_t region_size = MEM_REGIONS[i].end - MEM_REGIONS[i].begin + 1;
		MEM_REGIONS[i].mem = malloc(region_size);
		memset(MEM_REGIONS[i].mem, 0, region_size);
	}
}

/**************************************************************/
/* load program into memory                                                                                      */
/**************************************************************/
void load_program() {                   
	FILE * fp;
	int i, word;
	uint32_t address;

	/* Open program file. */
	fp = fopen(prog_file, "r");
	if (fp == NULL) {
		printf("Error: Can't open program file %s\n", prog_file);
		exit(-1);
	}

	/* Read in the program. */

	i = 0;
	while( fscanf(fp, "%x\n", &word) != EOF ) {
		address = MEM_TEXT_BEGIN + i;
		mem_write_32(address, word);
		printf("writing 0x%08x into address 0x%08x (%d)\n", word, address, address);
		i += 4;
	}
	PROGRAM_SIZE = i/4;
	printf("Program loaded into memory.\n%d words written into memory.\n\n", PROGRAM_SIZE);
	fclose(fp);
}

/************************************************************/
/* maintain the pipeline                                                                                           */ 
/************************************************************/
void handle_pipeline()
{
	/*INSTRUCTION_COUNT should be incremented when instruction is done*/
	/*Since we do not have branch/jump instructions, INSTRUCTION_COUNT should be incremented in WB stage */
	
	WB();
	MEM();
	EX();
	ID();
	IF();
}

/************************************************************/
/* writeback (WB) pipeline stage:                                                                          */ 
/************************************************************/
void WB()
{
        uint32_t opcode = (MEM_WB.IR & 0xFC000000) >> 26;
	uint32_t function = MEM_WB.IR & 0x0000003F;
	uint32_t rs = (MEM_WB.IR & 0x03E00000) >> 21;
	uint32_t rt = (MEM_WB.IR & 0x001F0000) >> 16;
	uint32_t rd = (MEM_WB.IR & 0x0000F800) >> 11;
	uint32_t sa = (MEM_WB.IR & 0x000007C0) >> 6;
	uint32_t immediate = MEM_WB.IR & 0x0000FFFF;
	uint32_t target = MEM_WB.IR & 0x03FFFFFF;
	
	if(opcode == 0x00){
		switch(function){
				case 0x00: //SLL
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				
				break;

			case 0x02: //SRL
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				
				break;
			case 0x03: //SRA 
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				break;
				
			case 0x0C: //SYSCALL

				break;
			case 0x10: //MFHI
				NEXT_STATE.REGS[rd] = CURRENT_STATE.HI;
		
				break;
			case 0x11: //MTHI
				NEXT_STATE.HI = MEM_WB.ALUOutput;
				
				break;
			case 0x12: //MFLO
				EX_MEM.ALUOutput  = CURRENT_STATE.LO;
				
				break;
			case 0x13: //MTLO
				NEXT_STATE.LO = MEM_WB.ALUOutput;
				
				break;
			case 0x18: //MULT
				NEXT_STATE.HI = CURRENT_STATE.HI;
				NEXT_STATE.LO = CURRENT_STATE.LO;
				break;
			case 0x19: //MULTU
				NEXT_STATE.HI = CURRENT_STATE.HI;
				NEXT_STATE.LO = CURRENT_STATE.LO;
				
				break;
			case 0x1A: //DIV 
				NEXT_STATE.HI = CURRENT_STATE.HI;
				NEXT_STATE.LO = CURRENT_STATE.LO;
				break;
			case 0x1B: //DIVU
				NEXT_STATE.HI = CURRENT_STATE.HI;
				NEXT_STATE.LO = CURRENT_STATE.LO;
				break;
			case 0x20: //ADD
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
			
				break;
			case 0x21: //ADDU 
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				
				break;
			case 0x22: //SUB
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				break;
			case 0x23: //SUBU
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				break;
			case 0x24: //AND
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				break;
			case 0x25: //OR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				break;
			case 0x26: //XOR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				break;
			case 0x27: //NOR
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				
				break;
			case 0x2A: //SLT
				NEXT_STATE.REGS[rd] = MEM_WB.ALUOutput;
				
				break;
			default:
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	else{
		switch(code){
			case 0x08: //ADDI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				break;
			case 0x09: //ADDIU
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				break;
			case 0x0A: //SLTI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				break;
			case 0x0C: //ANDI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				break;
			case 0x0D: //ORI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;
				break;
			case 0x0E: //XORI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;

				break;
			case 0x0F: //LUI
				NEXT_STATE.REGS[rt] = MEM_WB.ALUOutput;

				break;
			case 0x20: //LB
				NEXT_STATE.REGS[rt] = MEM_WB.LMD;
				break;
			case 0x21: //LH
				NEXT_STATE.REGS[rt] = MEM_WB.LMD;

				break;
			case 0x23: //LW
				NEXT_STATE.REGS[rt] = MEM_WB.LMD;
				break;
			case 0x28: //SB, don't need
				
				break;
			case 0x29: //SH, don't need
				
				break;
			case 0x2B: //SW, don't need
				

				break;
			default:
				
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;	
				
				
		}		
				
	}
}

/************************************************************/
/* memory access (MEM) pipeline stage:                                                          */ 
/************************************************************/
void MEM()
{        
        MEM_WB.PC=EX_MEM.PC;
	MEM_WB.IR=EX_MEM.IR;
	MEM_WB.A=EX_MEM.A;
	MEM_WB.B=EX_MEM.B;
	MEM_WB.imm=EX_MEM.imm;
	MEM_WB.ALUOutput=EX_MEM.ALUOutput;//load
        MEM_WB.LMD=0;//store
	CURRENT_STATE.HI=0;
	CURRENT_STATE.LO=0;

     uint32_t opcode = (MEM_WB.IR & 0xFC000000) >> 26;
     uint32_t function= MEM_WB.IR & 0x0000003F;
     if(opcode == 0x00){
		switch(function){
               case 0x0C: { //SYSTEMCALL
			       break;// we don't need to do anything with R insutuction
	       }
	        default: {
		           printf(" Wrong\t");
				//No R type
			}	
     }
    else{
         switch(code){//I/J type
		case 0x20: { //LB
				uint32_t a = 0xFF & mem_read_32(EX_MEM.ALUOutput);
				if(a >> 7) {	// then negative number
					byte = (0xFFFFFF00 | a); //sign extend with 1's
				}
				MEM_WB.LMD = byte;
				break;
			}
			case 0x21: { //LH
				uint32_t b = 0xFFFF & mem_read_32(EX_MEM.ALUOutput);
				if(b >> 15) {	// then negative number
					b = (0xFFFF0000 | b); //sign extend with 1's
				}
				MEM_WB.LMD = b;
				break;
			}
			case 0x23: { //LW
				uint32_t c = mem_read_32(EX_MEM.ALUOutput);
				MEM_WB.LMD = c;
				break;
			}
			case 0x28: { //SB
				mem_write_32(EX_MEM.ALUOutput,EX_MEM.B);
				break;
			}
			case 0x29: { //SH
				mem_write_32(EX_MEM.ALUOutput,EX_MEM.B);
				break;
			}
			case 0x2B: { //SW
				mem_write_32(EX_MEM.ALUOutput,EX_MEM.B);
				break;
			}
			default: {
				printf("this instruction has not been handled\t");
				//Not an instruction accessing memory
			}	     
			     
			     
	     }
			     
	}
}

/************************************************************/
/* execution (EX) pipeline stage:                                                                          */ 
/************************************************************/
void EX()
{
	/*IMPLEMENT THIS*/
	uint32_t instruction, opcode, function, rs, rt, rd, sa, immediate, target;

	EX_MEM.IR=ID_EX.IR;
	EX_MEM.A=ID_EX.A;//rt
	EX_MEM.B=ID_EX.B;//sa
	EX_MEM.imm=ID_EX.imm;
	EX_MEM.ALUOutput=0;//rd
	CURRENT_STATE.HI=0;
	CURRENT_STATE.LO=0;


	opcode = (EX_MEM.IR & 0xFC000000) >> 26;
	function = EX_MEM.IR & 0x0000003F;
	rs = (EX_MEM.IR & 0x03E00000) >> 21;
	rt = (EX_MEM.IR & 0x001F0000) >> 16;
	rd = (EX_MEM.IR & 0x0000F800) >> 11;
	sa = (EX_MEM.IR & 0x000007C0) >> 6;
	immediate = EX_MEM.IR & 0x0000FFFF;
	target = EX_MEM.IR & 0x03FFFFFF;



	if(opcode == 0x00){
		switch(function){
			case 0x00: //SLL
				EX_MEM.ALUOutput = EX_MEM.B << EX_MEM.imm;
				
				break;

			case 0x02: //SRL
				EX_MEM.ALUOutput = EX_MEM.B >> EX_MEM.imm;
				
				break;
			case 0x03: //SRA 
				if ((EX_MEM.A & 0x80000000) == 1)
				{
					EX_MEM.ALUOutput =  ~(~EX_MEM.B >> EX_MEM.imm );
				}
				else{
					EX_MEM.ALUOutput= EX_MEM.B >> EX_MEM.imm;
				}
				break;
			case 0x08: //JR
				NEXT_STATE.PC = EX_MEM.B;
				break;
			case 0x09: //JALR
				EX_MEM.B = CURRENT_STATE.PC + 4;
				NEXT_STATE.PC = EX_MEM.B;
				break;
			case 0x0C: //SYSCALL

				break;
			case 0x10: //MFHI
				EX_MEM.ALUOutput = CURRENT_STATE.HI;
				break;
			case 0x11: //MTHI
				NEXT_STATE.HI =EX_MEM.B;
				
				break;
			case 0x12: //MFLO
				EX_MEM.ALUOutput  = CURRENT_STATE.LO;
				
				break;
			case 0x13: //MTLO
				NEXT_STATE.LO = EX_MEM.B;
				
				break;
			case 0x18: //MULT
				if ((EX_MEM.B & 0x80000000) == 0x80000000){
					p1 = 0xFFFFFFFF00000000 |EX_MEM.B;
				}else{
					p1 = 0x00000000FFFFFFFF & EX_MEM.B;
				}
				if ((EX_MEM.A & 0x80000000) == 0x80000000){
					p2 = 0xFFFFFFFF00000000 | EX_MEM.A ;
				}else{
					p2 = 0x00000000FFFFFFFF & EX_MEM.A ;
				}
				product = p1 * p2;
				NEXT_STATE.LO = (product & 0X00000000FFFFFFFF);
				NEXT_STATE.HI = (product & 0XFFFFFFFF00000000)>>32;
				
				break;
			case 0x19: //MULTU
				product = (uint64_t)EX_MEM.B * (uint64_t)EX_MEM.A ;
				NEXT_STATE.LO = (product & 0X00000000FFFFFFFF);
				NEXT_STATE.HI = (product & 0XFFFFFFFF00000000)>>32;
				
				break;
			case 0x1A: //DIV 
				if(EX_MEM.A  != 0)
				{
					NEXT_STATE.LO = (int32_t)EX_MEM.B / (int32_t)EX_MEM.A ;
					NEXT_STATE.HI = (int32_t)EX_MEM.B  % (int32_t)EX_MEM.A ;
				}
				
				break;
			case 0x1B: //DIVU
				if(EX_MEM.A  != 0)
				{
					NEXT_STATE.LO = EX_MEM.B  / EX_MEM.A ;
					NEXT_STATE.HI =EX_MEM.B  % EX_MEM.A ;
				}
				
				break;
			case 0x20: //ADD
				EX_MEM.ALUOutput = EX_MEM.B  + EX_MEM.A;
			
				break;
			case 0x21: //ADDU 
				EX_MEM.ALUOutput = EX_MEM.B  + EX_MEM.A;
				
				break;
			case 0x22: //SUB
				EX_MEM.ALUOutput = EX_MEM.B  - EX_MEM.A;
				break;
			case 0x23: //SUBU
				EX_MEM.ALUOutput = EX_MEM.B  - EX_MEM.A;
				break;
			case 0x24: //AND
				EX_MEM.ALUOutput = EX_MEM.B  & EX_MEM.A;
				break;
			case 0x25: //OR
				EX_MEM.ALUOutput = EX_MEM.B  | EX_MEM.A;
				break;
			case 0x26: //XOR
				EX_MEM.ALUOutput = EX_MEM.B  ^ EX_MEM.A;
				break;
			case 0x27: //NOR
				EX_MEM.ALUOutput  = ~( EX_MEM.B  |  EX_MEM.A );
				
				break;
			case 0x2A: //SLT
				if( EX_MEM.B  < EX_MEM.A ){
					EX_MEM.ALUOutput= 0x1;
				}
				else{
					EX_MEM.ALUOutput = 0x0;
				}
				
				break;
			default:
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
	else{//I/J type
		switch(opcode){
			case 0x01:
				if(rt == 0x00000){ //BLTZ
					if((EX_MEM.B & 0x80000000) > 0){
						NEXT_STATE.PC = CURRENT_STATE.PC + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
						branch_jump = TRUE;
					}

				}
				else if(rt == 0x00001){ //BGEZ
					if((EX_MEM.B & 0x80000000) == 0x0){
						NEXT_STATE.PC = CURRENT_STATE.PC + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);

					}

				}
				break;
			case 0x02: //J
				NEXT_STATE.PC = (CURRENT_STATE.PC & 0xF0000000) | (target << 2);
				break;
			case 0x03: //JAL
				NEXT_STATE.PC = (CURRENT_STATE.PC & 0xF0000000) | (target << 2);
				EX_MEM.ALUOutput = CURRENT_STATE.PC + 4;
				break;
			case 0x04: //BEQ
				if(EX_MEM.B == EX_MEM.A){
					NEXT_STATE.PC = CURRENT_STATE.PC + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);

				}

				break;
			case 0x05: //BNE
				if(EX_MEM.B != EX_MEM.A){
					NEXT_STATE.PC = CURRENT_STATE.PC + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);

				}

				break;
			case 0x06: //BLEZ
				if((EX_MEM.B & 0x80000000) > 0 || EX_MEM.B == 0){
					NEXT_STATE.PC = CURRENT_STATE.PC +  ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);

				}

				break;
			case 0x07: //BGTZ
				if((EX_MEM.B & 0x80000000) == 0x0 || EX_MEM.B != 0){
					NEXT_STATE.PC = CURRENT_STATE.PC +  ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000)<<2 : (immediate & 0x0000FFFF)<<2);
					branch_jump = TRUE;
				}

				break;
			case 0x08: //ADDI
				EX_MEM.ALUOutput = EX_MEM.B + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));

				break;
			case 0x09: //ADDIU
				EX_MEM.ALUOutput = EX_MEM.B + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));

				break;
			case 0x0A: //SLTI
				if ( (  (int32_t)EX_MEM.B - (int32_t)( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF))) < 0){
					EX_MEM.ALUOutput = 0x1;
				}else{
					EX_MEM.ALUOutput = 0x0;
				}

				break;
			case 0x0C: //ANDI
				EX_MEM.ALUOutput = EX_MEM.B & (immediate & 0x0000FFFF);

				break;
			case 0x0D: //ORI
				EX_MEM.ALUOutput = EX_MEM.B | (immediate & 0x0000FFFF);

				break;
			case 0x0E: //XORI
				EX_MEM.ALUOutput = EX_MEM.B ^ (immediate & 0x0000FFFF);

				break;
			case 0x0F: //LUI
				EX_MEM.ALUOutput = immediate << 16;

				break;
			case 0x20: //LB
				data = mem_read_32( EX_MEM.B + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF)) );
				EX_MEM.ALUOutput = ((data & 0x000000FF) & 0x80) > 0 ? (data | 0xFFFFFF00) : (data & 0x000000FF);

				break;
			case 0x21: //LH
				data = mem_read_32( EX_MEM.B + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF)) );
				EX_MEM.ALUOutput = ((data & 0x0000FFFF) & 0x8000) > 0 ? (data | 0xFFFF0000) : (data & 0x0000FFFF);

				break;
			case 0x23: //LW
				EX_MEM.ALUOutput = mem_read_32( EX_MEM.B + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF)) );

				break;
			case 0x28: //SB
				addr =  EX_MEM.B + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				data = mem_read_32( addr);
				data = (data & 0xFFFFFF00) | ( EX_MEM.A & 0x000000FF);
				mem_write_32(addr, data);

				break;
			case 0x29: //SH
				addr =  EX_MEM.B + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				data = mem_read_32( addr);
				data = (data & 0xFFFF0000) | ( EX_MEM.A & 0x0000FFFF);
				mem_write_32(addr, data);
				break;
			case 0x2B: //SW
				addr =  EX_MEM.B + ( (immediate & 0x8000) > 0 ? (immediate | 0xFFFF0000) : (immediate & 0x0000FFFF));
				mem_write_32(addr,  EX_MEM.A);

				break;
			default:
				/
				printf("Instruction at 0x%x is not implemented!\n", CURRENT_STATE.PC);
				break;
		}
	}
}

/************************************************************/
/* instruction decode (ID) pipeline stage:                                                         */ 
/************************************************************/
void ID()
{
	/*IMPLEMENT THIS*/
/*
ID/EX.IR <= IF/ID.IR
ID/EX.A <= REGS[ IF/ID.IR[rs] ]
ID/EX.B <= REGS[ IF/ID.IR[rt] ]
ID/EX.imm <= sign-extend( IF/ID.IR[imm. Field])
*/


	ID_EX.IR = IF_ID.IR;
	ID_EX.A  = CURRENT_STATE.REGS[IF_ID.IR[rs]];
	ID_EX.b  = CURRENT_STATE.REGS[IF_ID.IR[rt]];
	ID_EX.imm = ( (IF_ID.IR.imm & 0x8000) > 0 ? (IF_ID.IR.imm | 0xFFFF0000) : (IF_ID.IR.imm & 0x0000FFFF));

}

/************************************************************/
/* instruction fetch (IF) pipeline stage:                                                              */ 
/************************************************************/
void IF()
{
	/*IMPLEMENT THIS*/
/*
IR <= Mem[PC]
PC <= PC + 4
*/

	IF_ID.IR = mem_read_32(CURRENT_STATE.PC);
	IF_ID.PC = CURRENT_STATE.PC + 4;

	
}


/************************************************************/
/* Initialize Memory                                                                                                    */ 
/************************************************************/
void initialize() { 
	init_memory();
	CURRENT_STATE.PC = MEM_TEXT_BEGIN;
	NEXT_STATE = CURRENT_STATE;
	RUN_FLAG = TRUE;
}

/************************************************************/
/* Print the program loaded into memory (in MIPS assembly format)    */ 
/************************************************************/
void print_program(){
	/*IMPLEMENT THIS*/
}

/************************************************************/
/* Print the current pipeline                                                                                    */ 
/************************************************************/
void show_pipeline(){
	/*IMPLEMENT THIS*/
}

/***************************************************************/
/* main                                                                                                                                   */
/***************************************************************/
int main(int argc, char *argv[]) {                              
	printf("\n**************************\n");
	printf("Welcome to MU-MIPS SIM...\n");
	printf("**************************\n\n");
	
	if (argc < 2) {
		printf("Error: You should provide input file.\nUsage: %s <input program> \n\n",  argv[0]);
		exit(1);
	}

	strcpy(prog_file, argv[1]);
	initialize();
	load_program();
	help();
	while (1){
		handle_command();
	}
	return 0;
}
