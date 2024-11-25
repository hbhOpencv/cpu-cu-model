#include <stdio.h>
#include <stdint.h>

#define REG_NUM 32
#define MEM_SIZE 102400
#define OPCODE_R_TYPE 0x00
#define OPCODE_ADDI 0x08
#define OPCODE_BNE 0x05
#define OPCODE_LW 0x23
#define OPCODE_HLT 0x3F
#define FUNC_ADD 0x20
#define FUNC_SUB 0x22

uint32_t reg[REG_NUM];  
uint32_t mem[MEM_SIZE];
uint32_t regg[1];
uint32_t PC = 0;

typedef enum {
    ADD, SUB
} ALUOperation;

typedef struct {
    uint8_t RegDst : 1;
    uint8_t ALUSrc : 1;
    uint8_t MemtoReg : 1;
    uint8_t RegWrite : 1;
    uint8_t Branch : 1;
    ALUOperation ALUOp;
} ControlSignals;

uint32_t ALU(uint32_t A, uint32_t B, ALUOperation op) {
    switch (op) {
        case ADD: return A + B;
        case SUB: return A - B;
        default:  return 0;
    }
}

uint32_t fetch() {
    if (PC / 4 >= MEM_SIZE) {
        printf("PC out of bounds!\n");
        return 0;
    }
    uint32_t instruction = mem[PC / 4];
    PC += 4; 
    return instruction;
}

void decode(uint32_t instruction, ControlSignals *ctrl, uint32_t *rs, uint32_t *rt, uint32_t *rd, uint32_t *imm, uint32_t *opcode) {
    *opcode = (instruction >> 26) & 0x3F; 
    uint32_t funct = instruction & 0x3F; 
    *rs = (instruction >> 21) & 0x1F;
    *rt = (instruction >> 16) & 0x1F;
    *rd = (instruction >> 11) & 0x1F;
    *imm = instruction & 0xFFFF;
    if (*imm & 0x8000) {
        *imm |= 0xFFFF0000;  // Sign extend
    }
    ctrl->RegDst = 0;
    ctrl->ALUSrc = 0;
    ctrl->MemtoReg = 0;
    ctrl->RegWrite = 0;
    ctrl->Branch = 0;
    switch (*opcode) {
        case OPCODE_LW:  // LW 指令
            ctrl->ALUSrc = 1;
            ctrl->MemtoReg = 1;
            ctrl->RegWrite = 1;
            ctrl->ALUOp = ADD;
            break;
        case OPCODE_R_TYPE:  // R-type
            ctrl->RegDst = 1;
            ctrl->ALUSrc = 0;
            ctrl->RegWrite = 1;
            switch (funct) {
                case FUNC_ADD: // ADD
                    ctrl->ALUOp = ADD;
                    break;
                case FUNC_SUB: // SUB
                    ctrl->ALUOp = SUB;
                    break;}
            break;
        case OPCODE_ADDI:  // ADDI
            ctrl->ALUSrc = 1;
            ctrl->RegWrite = 1;
            ctrl->ALUOp = ADD;
            break;
        case OPCODE_BNE:  // BNE
            ctrl->Branch = 1;
            ctrl->ALUOp = SUB;
            break;
        case OPCODE_HLT:  // HLT
            ctrl->RegWrite = 0;
            break;
    }
}

void execute(uint32_t rs, uint32_t rt, uint32_t rd, uint32_t imm, ControlSignals ctrl) {
    uint32_t ALUResult;
    if (ctrl.ALUSrc) {
        ALUResult = ALU(reg[rs], imm, ctrl.ALUOp);  // 处理立即数
    } 
	else {
        ALUResult = ALU(reg[rs], reg[rt], ctrl.ALUOp);  // 处理寄存器操作
    }

    if (ctrl.MemtoReg) {  // 读取内存
        reg[rt] = mem[(regg[0]+ALUResult)/4];
    } 
	else if (ctrl.RegWrite) {
        if (ctrl.RegDst) {
            reg[rd] = ALUResult;
        } else {
            reg[rt] = ALUResult;
        }
    }
    if (ctrl.Branch) {  // 分支
        ALUResult = ALU(reg[rs], reg[rt], ctrl.ALUOp);
        printf("BNE: ALUResult: %d, rs: %d, rt: %d, imm: %d\n", (int32_t)ALUResult, reg[rs], reg[rt], (int32_t)imm);
        if (ALUResult != 0) {
            PC += (int32_t)imm << 2; 
            return;
        }
    }
    printf("After instruction execution:\n");
    if (ctrl.RegWrite) {
        printf("Register $%d (written back) value: %u\n", ctrl.RegDst ? rd : rt, reg[ctrl.RegDst ? rd : rt]);
    }
}
void next_instruction() {
    printf("PC now is the next instruction address: %u\n", PC); 
}
int main() {
    mem[25] = 1;       // 内存地址 100：值 1
    mem[26] = 0;       // 内存地址 104：值 0
    mem[27] = 101;     // 内存地址 108：值 101
    mem[0] = 0x8C010064; // LW $1, 100($0) -> $1 = mem[100]
    mem[1] = 0x8C020068; // LW $2, 104($0) -> $2 = mem[104]
    mem[2] = 0x8C03006C; // LW $3, 108($0) -> $3 = mem[108]
    mem[3] = 0x00411020; // ADD $2, $2, $1
    mem[4] = 0x20210001; // ADDI $1, $1, 1
    mem[5] = 0x1423FFFD; // BNE $1, $3, -3
    mem[6] = 0xFC000000; // HLT
    //printf("Size of ALUOperation: %zu bytes\n", sizeof(ALUOperation));
    while (PC < MEM_SIZE * 4) {
        uint32_t instruction = fetch();
        ControlSignals ctrl;
        uint32_t rs, rt, rd, imm, opcode;
        decode(instruction, &ctrl, &rs, &rt, &rd, &imm, &opcode);
        if (opcode == OPCODE_HLT) {
            printf("HLT instruction encountered. Halting execution.\n");
            break;
        }
        execute(rs, rt, rd, imm, ctrl);
        next_instruction();
    }
    printf("Final register values:\n");
    for (int i = 0; i < REG_NUM; i++) {
        printf("$%d: %u\n", i, reg[i]);
    }
     getchar();  // 等待用户按下回车键
    return 0;
}
    
