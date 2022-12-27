/*
 * John Mathews
 * Project 1 - 360 Subset Simulation
 * CPSC 3300
 *
 */

//include statements
#include <stdio.h>
#include <stdlib.h>

//program constants
#define INST_LR 0
#define INST_CR 1
#define INST_AR 2
#define INST_SR 3
#define INST_LA 4
#define INST_BCT 5
#define INST_BC 6
#define INST_ST 7
#define INST_L 8
#define INST_C 9
#define MEMORY 4096

//global variables - memory & registers
int reg[16] = {0};
int ram[MEMORY] = {0};
int halt     = 0,
    pc       = 0,
    op       = -1,
    r1       = 0,
    r2       = 0,
    x2       = 0,
    b2       = 0,
    disp     = 0;
unsigned int instruction;

//global variables - other
int verbose = 0;
int myIndex = 0;
int conditionCode = 0;
int effectiveAddress = 0;
int instructionAddress = 0;
int mask = 0;
int typeInst = 0; //0 = RR & 1 = RX
int temp = 0;
int temp2 = 0;

//global variables - summary exec statistics
int instructionFetchCount = 0,
    instructionCount[10] = {0},
    countBCTaken = 0,
    countBCTTaken = 0,
    countMemRead = 0,
    countMemWrite = 0;

//function prototypes
void loadRam();
void fetch();
void decode();
void printRegisters();

//call main()
int main(int argc, char* argv[]){

    //print title of program
    printf("\nbehavioral simulation of S/360 subset\n");

    //check command-line arguments for -v mode
    if (argc > 1){

        //put program into verbose mode
        if (argv[1][0] == '-' && (argv[1][1] == 'v' || argv[1][1] == 'V')) { verbose = 1; }
    }

    //if number of c-l id wrong print instructions
    if (argc < 1 || argc > 2){

        printf("usage: either %s or %s -v with input taken from stdin\n", argv[0], argv[0]);
        exit(-1);
    }

    //print information to terminal
    if (verbose){

        printf("\n(memory is limited to 4096 bytes in this simulation)\n");
        printf("(addresses, register values, and memory values are shown ");
        printf("in hexadecimal)\n\n");
    }

    //read bytes in from stdin & place into ram[]
    loadRam();

    //print information to terminal
    if (verbose){

        printf("initial pc, condition code, ");
        printf("and register values are all zero\n\n");
        printf("updated pc, condition code, and register ");
        printf("values are shown after\n");
        printf(" each instruction has been executed\n");
    }

    //print each instruction based off of opcode while !halt
    while (!halt){

        //get full instruction from ram
        fetch();

        //split instruction into parts
        decode();

        //catch oor error
        if (instructionAddress >= MEMORY){

            printf("\nout of range instruction address %x\n", instructionAddress);
            exit(0);
        }

        //determine what to do with op specified
        switch(op){

            //RR opcodes

            //opcode 0x18 LR: load contents of R2 into R1
            case 0x18:

                //load contents
                reg[r1] = reg[r2];

                //print verbose information
                if (verbose){

                    printf("\nLR instruction, ");
                    printf("operand 1 is R%x, ", r1);
                    printf("operand 2 is R%x\n", r2);
                    printRegisters();
                }

                instructionCount[INST_LR]++;
                break;

            //opcode 0x19 CR: compare contents of R1 with contents of R2 & set cond code
            case 0x19:

                //set condition code
                if (reg[r1] == reg[r2]) conditionCode = 0;
                else if (reg[r1] < reg[r2]) conditionCode = 1;
                else conditionCode = 2;

                //print verbose information
                if (verbose){

                    printf("\nCR instruction, ");
                    printf("operand 1 is R%x, ", r1);
                    printf("operand 2 is R%x\n", r2);
                    printRegisters();
                }

                instructionCount[INST_CR]++;
                break;

            //opcode 0x1A AR: add contents of R2 to contents of R1, put result in R1, and set cond code
            case 0x1a:

                //add registers
                reg[r1] = reg[r1] + reg[r2];

                //set condition code
                if (reg[r1] == 0) conditionCode = 0;
                else if (reg[r1] < 0) conditionCode = 1;
                else if (reg[r1] > 0) conditionCode = 2;
                else conditionCode = 3;

                //print verbose information
                if (verbose){

                    printf("\nAR instruction, ");
                    printf("operand 1 is R%x, ", r1);
                    printf("operand 2 is R%x\n", r2);
                    printRegisters();
                }

                instructionCount[INST_AR]++;
                break;

            //opcode 0x18 SR: subtract contents of R2 from contents of R1, put result in R1, and set cond code
            case 0x1b:

                //subtract registers
                reg[r1] = reg[r1] - reg[r2];

                //set condition code
                if (reg[r1] == 0) conditionCode = 0;
                else if (reg[r1] < 0) conditionCode = 1;
                else if (reg[r1] > 0) conditionCode = 2;
                else conditionCode = 3;

                //print verbose information
                if (verbose){

                    printf("\nSR instruction, ");
                    printf("operand 1 is R%x, ", r1);
                    printf("operand 2 is R%x\n", r2);
                    printRegisters();
                }

                instructionCount[INST_SR]++;
                break;

            //RX opcodes

            //opcode 0x41 LA: load 24-bit effadd into R1 & set high 8bits to 0s
            case 0x41:

                //load eff add into register specified
                reg[r1] = effectiveAddress;

                //print verbose information
                if (verbose){

                    printf("\nLA instruction, ");
                    printf("operand 1 is R%x, ", r1);
                    printf("operand 2 at address %06x\n", effectiveAddress);
                    printRegisters();
                }

                instructionCount[INST_LA]++;
                break;

            //case: 0x46 BCT: decrement contents of R1 and branch to eff add when its nonzero
            case 0x46:

                //subtract 1 from register specified
                reg[r1]--;

                //branch
                if (reg[r1] != 0){

                    instructionAddress = effectiveAddress;
                    countBCTTaken++;
                    pc = effectiveAddress;
                }

                //print verbose info
                if (verbose){

                    printf("\nBCT instruction, ");
                    printf("operand 1 is R%x, ", r1);
                    printf("branch target is address %06x\n", effectiveAddress);
                    printRegisters();
                }

                instructionCount[INST_BCT]++;
                break;

            //opcode 0x47 BC: treat r1 as mask and branch to the eff add based on conditionCode
            case 0x47:

                //unconditional branch
                if (r1 == 0xf){

                    instructionAddress = effectiveAddress;
                    countBCTaken++;
                    pc = effectiveAddress;
                }

                //branch on condition
                if (((r1 >> 2 == 1) && conditionCode == 1) || ((r1 >> 1 == 1) && conditionCode == 2) || ((r1 >> 0 == 1) && conditionCode == 3)){
                    instructionAddress = effectiveAddress;
                    countBCTaken++;
                    pc = effectiveAddress;
                }

                //print verbose info
                if (verbose){

                    printf("\nBC instruction, ");
                    printf("mask is %x, ", r1);
                    printf("branch target is address %06x\n", effectiveAddress);
                    printRegisters();
                }

                instructionCount[INST_BC]++;
                break;

            //opcode 0x50 ST: store contents of R1 into memory word at eff add
            case 0x50:

                //store the contents
                ram[effectiveAddress + 0] = (reg[r1] & 0xff000000) >> 24;
                ram[effectiveAddress + 1] = (reg[r1] & 0x00ff0000) >> 16;
                ram[effectiveAddress + 2] = (reg[r1] & 0x0000ff00) >> 8;
                ram[effectiveAddress + 3] = reg[r1] & 0x000000ff;

                //range check address for opcode
                if (effectiveAddress >= myIndex){

                    printf("\nout of range data address\n");
                    exit(0);
                }

                //check for data access alignment error: when the data word being accessed is not divisible by 4
                if ((effectiveAddress % 4) != 0){

                    printf("\ndata access alignment error at %x to %x\n", pc-4, effectiveAddress);
                    exit(-1);
                }

                //print verbose info
                if (verbose){

                    printf("\nST instruction, ");
                    printf("operand 1 is R%x, ", r1);
                    printf("operand 2 at address %06x\n", effectiveAddress);
                    printRegisters();
                }

                countMemWrite++;
                instructionCount[INST_ST]++;
                break;

            //opcode 58 L: load contents of memory word at eff add into R1
            case 0x58:

                //maybe take out the + reg[r1] if problems occur
                reg[r1] = ((ram[effectiveAddress] << 24) | (ram[effectiveAddress+1] << 16) | (ram[effectiveAddress+2] << 8) | ram[effectiveAddress+3]);

                //range check address for opcode
                if (effectiveAddress >= myIndex){

                    printf("\nout of range data address\n");
                    exit(0);
                }

                //check for data access alignment error: when the data word being accessed is not divisible by 4
                if ((effectiveAddress % 4) != 0){

                    printf("\ndata access alignment error at %x to %x\n", pc-4, effectiveAddress);
                    exit(-1);
                }

                //print verbose info
                if (verbose){

                    printf("\nL instruction, ");
                    printf("operand 1 is R%x, ", r1);
                    printf("operand 2 at address %06x\n", effectiveAddress);
                    printRegisters();
                }

                countMemRead++;
                instructionCount[INST_L]++;
                break;

            //opcode 0x59 C: compare contents of r1 with contents of mem word at eff add
            case 0x59:

                //all bits are equal
                if ((ram[effectiveAddress] == (reg[r1] & 0xff000000)) && (ram[effectiveAddress + 1] == (reg[r1] & 0x00ff0000)) && (ram[effectiveAddress + 2] == (reg[r1] & 0x0000ff00)) && (ram[effectiveAddress + 3] == (reg[r1] & 0x000000ff))){
                    conditionCode = 0;
                }
                //register is less than memory word
                else if ((ram[effectiveAddress] > (reg[r1] & 0xff000000)) || (ram[effectiveAddress + 1] > (reg[r1] & 0x00ff0000)) || (ram[effectiveAddress + 2] > (reg[r1] & 0x0000ff00)) || (ram[effectiveAddress + 3] > (reg[r1] & 0x000000ff))){
                    conditionCode = 1;
                }
                //register is greater than memory word
                else{
                    conditionCode = 2;
                }

                //range check address for opcode
                if (effectiveAddress >= myIndex){

                    printf("\nout of range data address\n");
                    exit(0);
                }

                //check for data access alignment error: when the data word being accessed is not divisible by 4
                if ((effectiveAddress % 4) != 0){

                    printf("\ndata access alignment error at %x to %x\n", pc-4, effectiveAddress);
                    exit(-1);
                }

                //print verbose info
                if (verbose){

                    printf("\nC instruction, ");
                    printf("operand 1 is R%x, ", r1);
                    printf("operand 2 at address %06x\n", effectiveAddress);
                    printRegisters();
                }

                countMemRead++;
                instructionCount[INST_C]++;
                break;

            //opcode 0x0: set halt to 1 and end loop
            case 0x0:

                //set halt to 1
                instructionAddress+=2;
                halt = 1;

                //print verbose info
                if (verbose){

                    printf("\nHalt encountered\n");
                    printRegisters();
                }

                break;

            //unknown opcodes
            default:

                if (typeInst) printf("\nunknown opcode %x at %03x\n", op, pc-4);
                else printf("\nunknown opcode %x at %03x\n", op, pc-2);

                exit(-1);

                break;
        }
    }

    //print final contents in verbose mode
    if (verbose){

        printf("\nfinal contents of memory arranged by words\n");
        printf("addr value\n");
        for (int i = 0; i < myIndex; i+=4){

            instruction = (ram[i] << 24) | (ram[i+1] << 16) | (ram[i+2] << 8) | ram[i+3];
            printf("%03x: %08x\n", i, instruction);
        }
    }

    //print summary execution statistics of program
    printf("\nexecution statistics\n");
    printf("  instruction fetches = %d\n", instructionFetchCount);
    printf("    LR  instructions  = %d\n", instructionCount[INST_LR]);
    printf("    CR  instructions  = %d\n", instructionCount[INST_CR]);
    printf("    AR  instructions  = %d\n", instructionCount[INST_AR]);
    printf("    SR  instructions  = %d\n", instructionCount[INST_SR]);
    printf("    LA  instructions  = %d\n", instructionCount[INST_LA]);

    printf("    BCT instructions  = %d", instructionCount[INST_BCT]);
    if (instructionCount[INST_BCT] > 0) printf(", taken = %d (%.1f%%)\n", countBCTTaken, 100.0 * ((float)countBCTTaken)/((float)instructionCount[INST_BCT]));
    else printf("\n");

    printf("    BC  instructions  = %d", instructionCount[INST_BC]);
    if (instructionCount[INST_BC] > 0) printf(", taken = %d (%.1f%%)\n", countBCTaken, 100.0 * ((float)countBCTaken)/((float)instructionCount[INST_BC]));
    else printf("\n");

    printf("    ST  instructions  = %d\n", instructionCount[INST_ST]);
    printf("    L   instructions  = %d\n", instructionCount[INST_L]);
    printf("    C   instructions  = %d\n", instructionCount[INST_C]);
    printf("  memory data reads   = %d\n", countMemRead);
    printf("  memory data writes  = %d\n", countMemWrite);

    return 0;
}

//function definitions

//take input from stdin and place into ram[]
void loadRam(){

    //print additional info in verbose mode
    if (verbose){

        printf("initial contents of memory arranged by bytes\n");
        printf("addr value\n");
    }

    //place input from stdin into ram[]
    while (scanf("%x", &ram[myIndex]) != EOF){

        //catch overflow error & exit program
        if (myIndex >= MEMORY){

            printf("program file overflows available memory\n");
            exit(-1);
        }

        //mask each input to 12-bit word size
        ram[myIndex] = ram[myIndex] & 0xfff;

        //echo bytes when in verbose mode
        if (verbose) printf("%03x: %02x\n", myIndex, ram[myIndex]);

        //increase index
        myIndex++;
    }

    //set the rest of the memory to 0 to avoid junk
    for (int i = myIndex; i < MEMORY; i++) ram[i] = 0;

    if (verbose) printf("\n");
}

//get instruction from memory
void fetch(){

    //grab opcode to get type & set instructionAddress
    temp = ram[pc];
    temp2 = pc;

    //if halt was encountered return
    if (temp == 0x0){

        op = 0x0;
        return;
    }

    //get instruction in RR format - 2 bytes
    if (temp == 0x18 || temp == 0x19 || temp == 0x1a || temp == 0x1b){

        //concatenate hex digits together
        instruction = (ram[pc] << 8) | ram[pc+1];
        pc+=2;

        //set instructionAddress
        instructionAddress = pc;

        //type of code being read
        typeInst = 0;

        //increment counter
        instructionFetchCount++;
    }
    //get instruction in RX format - 4 bytes
    else{

        //concatenate hex digits together
        instruction = (ram[pc] << 24) | (ram[pc+1] << 16) | (ram[pc+2] << 8) | ram[pc+3];
        pc+=4;

        //set instructionAddress
        instructionAddress = pc;

        //type of code being read
        typeInst = 1;

        //increment counter
        instructionFetchCount++;
    }
}

//extracts fields from instruction
void decode(){

    //if halt was encountered return
    if (op == 0) return;

    //decode based on RX guidelines
    if (typeInst){

        //parse fields
        disp = (instruction & 0xf) | (instruction & 0xff) | (instruction & 0xfff);
        b2 = (instruction >> 12) & 0xf;
        x2 = (instruction >> 16) & 0xf;
        r1 = (instruction >> 20) & 0xf;
        op = (instruction >> 24) & 0xff;

        //calculate effective address & clamp to 24 bits
        if (x2 != 0 && b2 != 0) effectiveAddress = (reg[x2] + reg[b2] + disp) & 0xffffff;
        if (x2 == 0 || b2 == 0){

            if (x2 == 0 && b2 == 0) effectiveAddress = disp & 0xffffff;
            if (x2 == 0 && b2 != 0) effectiveAddress = (reg[b2] + disp) & 0xffffff;
            if (x2 != 0 && b2 == 0) effectiveAddress = (reg[x2] + disp) & 0xffffff;
        }

        //catch instruction fetch alignment error: when opcode location isnt divisible by 2
        if (temp2 % 2 != 0){

            printf("\ninstruction fetch alignment error at %x to %x\n", effectiveAddress, temp2);
            exit(0);
        }
    }
    //decode based on RR guidelines
    else{

        //parse fields
        r2 = instruction & 0xf;
        r1 = (instruction >> 4) & 0xf;
        op = (instruction >> 8) & 0xff;
    }

    //increase counter
    instructionCount[op]++;
}

//printRegisters(): prints contents of registers in verbose mode
void printRegisters(){

    printf("instruction address = %06x, ", instructionAddress);
    printf("condition code = %x\n", conditionCode);
    printf("R0 = %08x, R4 = %08x, R8 = %08x, RC = %08x\n", reg[0], reg[4], reg[8], reg[12]);
    printf("R1 = %08x, R5 = %08x, R9 = %08x, RD = %08x\n", reg[1], reg[5], reg[9], reg[13]);
    printf("R2 = %08x, R6 = %08x, RA = %08x, RE = %08x\n", reg[2], reg[6], reg[10], reg[14]);
    printf("R3 = %08x, R7 = %08x, RB = %08x, RF = %08x\n", reg[3], reg[7], reg[11], reg[15]);
}
