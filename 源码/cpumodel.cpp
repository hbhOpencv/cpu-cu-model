#include <stdio.h>
#include <stdint.h>

#define REG_NUM 32
#define MEM_SIZE 102400
#define OPCODE_R_TYPE 0x00
#define OPCODE_ADDI 0x08
#define OPCODE_BNE 0x05
#define OPCODE_J 0x02
#define OPCODE_HLT 0x3F
#define FUNC_ADD 0x20
#define FUNC_SUB 0x22
uint32_t reg[REG_NUM];  
uint32_t mem[MEM_SIZE];
uint32_t PC = 0;

typedef enum {
   ADD, SUB, SLT, BNE, HLT
} ALUOperation;

typedef struct {
    uint8_t RegDst : 1;
    uint8_t ALUSrc : 1;
    uint8_t MemtoReg : 1;
    uint8_t RegWrite : 1;
    uint8_t MemRead : 1;
    uint8_t MemWrite : 1;
    uint8_t Branch : 1;
    uint8_t Jump : 1;
    ALUOperation ALUOp;
} ControlSignals;


uint32_t ALU(uint32_t A, uint32_t B, ALUOperation op) {
    switch (op) {
        case ADD: return A + B;
        case SUB: return A - B;
        //case AND: return A & B;
        //case OR:  return A | B;
        //case SLT: return (A < B) ? 1 : 0;
        default:  return 0;
    }
}

uint32_t fetch() {
    if (PC / 4 >= MEM_SIZE) {
        printf("PC out of bounds!\n");
        return 0;
    }
    return mem[PC / 4];
  
}

void decode(uint32_t instruction, ControlSignals *ctrl, uint32_t *rs, uint32_t *rt, uint32_t *rd, uint32_t *imm, uint32_t *opcode) {
    *opcode = (instruction >> 26) & 0x3F;
    uint32_t funct = instruction & 0x3F; 
    *rs = (instruction >> 21) & 0x1F;
    *rt = (instruction >> 16) & 0x1F;
    *rd = (instruction >> 11) & 0x1F;
    *imm = instruction & 0xFFFF;
    if (*imm & 0x8000) {
        *imm |= 0xFFFF0000; 
    }
    ctrl->RegDst = 0;
    ctrl->ALUSrc = 0;
    ctrl->MemtoReg = 0;
    ctrl->RegWrite = 0;
    ctrl->MemRead = 0;
    ctrl->MemWrite = 0;
    ctrl->Branch = 0;
    ctrl->Jump = 0;
    switch (*opcode) {
        case OPCODE_R_TYPE: // R-type
            ctrl->RegDst = 1;
            ctrl->ALUSrc = 0;
            ctrl->RegWrite = 1;
            switch (funct) {
                case FUNC_ADD: // ADD
                    ctrl->ALUOp = ADD;
                    break;
                case FUNC_SUB: // SUB
                    ctrl->ALUOp = SUB;
                    break;
               // case FUNC_JR: // JR
                    //ctrl->Jump = 1;
                    //break;
            }
            break;
        case OPCODE_ADDI: // ADDI
            ctrl->ALUSrc = 1;
            ctrl->RegWrite = 1;
            ctrl->ALUOp = ADD;
            break;
        case OPCODE_BNE: // BNE
            ctrl->Branch = 1;
            ctrl->ALUOp = SUB; 
            break;
        case OPCODE_J: // J
            ctrl->Jump = 1;
            break;
        case OPCODE_HLT: // HLT
            ctrl->RegWrite = 0;
            ctrl->Jump = 0;
            break;
    }
}


void execute(uint32_t rs, uint32_t rt, uint32_t rd, uint32_t imm, ControlSignals ctrl) {
    uint32_t ALUResult;
    if (ctrl.Jump) {
        PC = reg[rs]; 
        return;
    }
    if (ctrl.Branch) {
        ALUResult = ALU(reg[rs], reg[rt], ctrl.ALUOp);
        printf("BNE: ALUResult: %d, rs: %d, rt: %d, imm: %d\n", (int32_t)ALUResult, reg[rs], reg[rt], (int32_t)imm);
        
        if (ALUResult != 0) {
            PC += (int32_t)imm << 2; 
            return;
        }
    } 
	else if (ctrl.ALUSrc == 1) {
        ALUResult = ALU(reg[rs], imm, ctrl.ALUOp); // 处理 ADDI
    } 
	else {
        ALUResult = ALU(reg[rs], reg[rt], ctrl.ALUOp); 
    }
    if (ctrl.RegWrite) {
        if (ctrl.RegDst) {
            reg[rd] = ALUResult;
        } else {
            reg[rt] = ALUResult; // I-type 指令
        }
    }
    printf("After instruction execution:\n");
    if (ctrl.RegWrite) {
        printf("Register $%d (written back) value: %u\n", ctrl.RegDst ? rd : rt, reg[ctrl.RegDst ? rd : rt]);
    }
}

void next_instruction(ControlSignals ctrl, uint32_t imm, uint32_t opcode) {
    if (ctrl.Jump) {
        if (opcode == OPCODE_J) { // J 
            PC = (PC & 0xF0000000) | (imm << 2); 
        }
    } else {
        PC += 4; 
    }
    printf("PC now is: %u\n", PC); 
}

int main() {
    for (int i = 0; i < REG_NUM; i++) reg[i] = 0;
    for (int i = 0; i < MEM_SIZE; i++) mem[i] = 0;
    reg[1] = 1;   
    reg[2] = 0;   
    reg[3] = 101;
    mem[0] = 0x00411020; // ADD $2, $2, $1
    mem[1] = 0x20210001; // ADDI $1, $1, 1
    mem[2] = 0x1423FFFD; // BNE $1, $3, -3 (跳转到0)
    mem[3] = 0xFC000000; // HLT
    while (PC < MEM_SIZE * 4) {
        uint32_t instruction = fetch();
        ControlSignals ctrl;
        uint32_t rs, rt, rd, imm, opcode;
        decode(instruction, &ctrl, &rs, &rt, &rd, &imm, &opcode);
        // 检查是否为 HLT 指令
        if (opcode == OPCODE_HLT) {
            printf("HLT instruction encountered. Halting execution.\n");
            break;
        }
        execute(rs, rt, rd, imm, ctrl);
        next_instruction(ctrl, imm, opcode);
    }
    printf("Final register values:\n");
    for (int i = 0; i < REG_NUM; i++) {
        printf("$%d: %u\n", i, reg[i]);
    }
    getchar();  // 等待用户按下回车键
    return 0;
}
