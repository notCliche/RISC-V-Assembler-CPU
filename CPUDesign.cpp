#include <bits/stdc++.h>
using namespace std;

vector<string> iMem; // Instruction memory
int pc; // Program counter
int dMem[1024] = {0}; // Data memory
int GPR[32] = {0}; // General purpose registers
int instrNum; // Number of instructions

bitset<32> regLock; // Register lock status
bool skip = false; // Flag to skip execution
bool hazard[2] = {false, false}; // {data hazard stall, control hazard stall}

// Structure to hold the state flags for different pipeline stages
struct flags {
    bool pc = true; // Program counter state
    bool fetch = false; // Fetch stage state
    bool decode = false; // Decode stage state
    bool execute = false; // Execute stage state
    bool memory = false; // Memory stage state
    bool writeback = false; // Writeback stage state
} states;

// Control word structure to hold control signals for each instruction type
struct CtrlWord {
    int RegRead, RegWrite, ALUSrc, ALUOp, Branch, Jump, MemRead, MemWrite, MemToReg;
};

// Control unit mapping opcodes to control signals
map<string, CtrlWord> ControlUnit = {
    {"0110011", {1, 1, 0, 10, 0, 0, 0, 0, 0}},  // R-Type
    {"0010011", {1, 1, 1, 11, 0, 0, 0, 0, 0}},  // I-Type
    {"0000011", {1, 1, 1, 00, 0, 0, 1, 0, 1}},  // L-Type
    {"0100011", {1, 0, 1, 00, 0, 0, 0, 1, -1}}, // S-Type
    {"1100011", {1, 0, 0, 01, 1, 0, 0, 0, -1}}, // B-Type
    {"0110111", {0, 1, 1, 00, 0, 0, 0, 0, 0}},  // U-Type
    {"1101111", {0, 1, 0, 00, 0, 1, 0, 0, 0}}   // J-Type
};

// Utility class for binary and decimal conversions
class Utility {
public:
    // Convert binary string to decimal integer
    int toDec(string binary) {
        int result = 0;
        int len = binary.length();
        bool negative = (binary[0] == '1');

        // Handle positive numbers normally
        if (!negative) {
            for (int i = 0; i < len; i++) {
                if (binary[len - 1 - i] == '1') {
                    result += (1 << i);
                }
            }
            return result;
        }
        
        // For negative numbers, use 2's complement
        string inverted = binary; // Invert all bits
        for (int i = 0; i < len; i++) {
            inverted[i] = (binary[i] == '1') ? '0' : '1';
        }
        
        // Convert inverted bits to decimal
        for (int i = 0; i < len; i++) {
            if (inverted[len - 1 - i] == '1') {
                result += (1 << i);
            }
        }
        return -(result + 1); // Add 1 and make negative
    }

    // Convert decimal integer to binary string
    string toBin(int decimal) {
        bitset<32> binary(decimal);
        return binary.to_string();
    }

    // Sign-extend the immediate value
    int signExtend(const string& immediate) {
        string extendedBits = (immediate[0] == '0') ? "000000000000" : "111111111111";
        return toDec(extendedBits + immediate);
    }
};

Utility utilities;

// Instruction Fetch/Decode structure
class IFID {
public:
    string instr; // Instruction fetched
    int CPC, NPC; // Current and next program counter
};

// Instruction Decode/Execute structure
class IDEX {
public:
    class Control {
    public:
        int RegRead, RegWrite, ALUSrc, ALUOp, Branch, Jump, MemRead, MemWrite, MemToReg;
        // Set control signals based on opcode
        void setControl(string opcode) {
            CtrlWord signals = ControlUnit[opcode];
            RegRead = signals.RegRead;
            RegWrite = signals.RegWrite;
            ALUSrc = signals.ALUSrc;
            ALUOp = signals.ALUOp;
            Branch = signals.Branch;
            Jump = signals.Jump;
            MemRead = signals.MemRead;
            MemWrite = signals.MemWrite;
            MemToReg = signals.MemToReg;
        }
    };
    string imm1, imm2, func, rds, rs1, rs2, instr; // Instruction components
    int JPC, CPC; // Jump and current program counter
    Control control; // Control signals
};

// Execute/Memory structure
class EXMO {
public:
    class Control {
    public:
        int RegRead, RegWrite, ALUSrc, ALUOp, Branch, Jump, MemRead, MemWrite, MemToReg;
        // Copy control signals from IDEX
        void copyFrom(IDEX &idex) {
            RegRead = idex.control.RegRead;
            RegWrite = idex.control.RegWrite;
            ALUSrc = idex.control.ALUSrc;
            ALUOp = idex.control.ALUOp;
            Branch = idex.control.Branch;
            Jump = idex.control.Jump;
            MemRead = idex.control.MemRead;
            MemWrite = idex.control.MemWrite;
            MemToReg = idex.control.MemToReg;
        }
    };
    string rds, rs2;    // Destination and source registers
    int aluResult;      // ALU result
    Control control;    // Control signals
};

// Memory/Writeback structure
class MOWB {
public:
    class Control {
    public:
        int RegRead, RegWrite, ALUSrc, ALUOp, Branch, Jump, MemRead, MemWrite, MemToReg;
        // Copy control signals from EXMO
        void copyFrom(EXMO exmo) {
            RegRead = exmo.control.RegRead;
            RegWrite = exmo.control.RegWrite;
            ALUSrc = exmo.control.ALUSrc;
            ALUOp = exmo.control.ALUOp;
            Branch = exmo.control.Branch;
            Jump = exmo.control.Jump;
            MemRead = exmo.control.MemRead;
            MemWrite = exmo.control.MemWrite;
            MemToReg = exmo.control.MemToReg;
        }
    };
    string rds; // Destination register
    int aluResult, memoryData; // ALU result and memory data
    Control control; // Control signals
};

// Check for data and control hazards
void checkHazards(string opcode, string rd, string rs1, string rs2) {
    // Data Hazards
    if (opcode == "0110011" || opcode == "1100011") {
        if (rd == rs1 || rd == rs2) {
            hazard[0] = true;
            regLock[stoi(rd, NULL, 2)] = 1;
        }
    }
    if (opcode == "0010011" || opcode == "0000011") {
        if (rd == rs1) {
            hazard[0] = true;
            regLock[stoi(rd, NULL, 2)] = 1;
        }
    }
    // Control Hazard
    if (opcode == "1100011" || opcode == "1101111") {
        hazard[1] = true;
    }
}

// Fetch the instruction from memory
void fetch(IFID &ifid) {
    // Early return for blocking condition
    if (hazard[0] || hazard[1]) {
        return;
    }

    // Fetch the instruction
    if (pc < instrNum * 4) {
        int index = pc / 4;
        ifid.instr = iMem[index];
        ifid.CPC = pc;
        pc = pc + 4;
        states.fetch = true;
    } else {
        states.pc = false; // Halt the pipeline
    }
}

// Decode the instruction and prepare for execution
void decode(IDEX &idex, IFID &ifid, EXMO &exmo) {
    // Early return for blocking condition
    if (!states.pc || skip || hazard[0] || hazard[1]) {
        if (skip) skip = false;
        if (!states.pc) states.fetch = false;
        return;
    }

    string instr = ifid.instr;
    // Validate instruction length
    if (instr.length() != 32) {
        cout << "Invalid instruction length" << endl;
        return;
    }

    // Extract instruction components
    idex.instr = instr;
    idex.CPC = ifid.CPC;
    idex.JPC = ifid.CPC + 4 * utilities.signExtend(instr.substr(0, 20));
    
    // Extract immediate values and control bits
    idex.imm1 = instr.substr(0, 12);
    idex.imm2 = instr.substr(0, 7) + instr.substr(20, 5);
    idex.func = instr.substr(17, 3);
    idex.rds = instr.substr(20, 5);
    
    // Set control signals and check for hazards
    string opcode = instr.substr(25, 7);
    idex.control.setControl(opcode);
    checkHazards(opcode, exmo.rds, instr.substr(12, 5), instr.substr(7, 5));
    
    states.decode = true;
}

// Determine the ALU control signal based on operation type
string ALUCtrl(int ALUOp, const string& func, const string& func7) {
    string ALUSel;

    if (ALUOp == 0) ALUSel = "0010"; // ADD
    else if (ALUOp == 1) {
        if (func == "000" || func == "001") ALUSel = "0110"; // SUB
        else ALUSel = "1111"; // Invalid
    } 
    else if (ALUOp == 10) {
        if (func == "000") ALUSel = (func7 == "0000000") ? "0010" : "0110"; // ADD/SUB
        else if (func == "001") ALUSel = "0011"; // SLL
        else if (func == "111") ALUSel = "0000"; // AND
        else if (func == "110") ALUSel = "0001"; // OR
        else ALUSel = "1111";
    } 
    else if (ALUOp == 11) {
        if (func == "000") ALUSel = "0010"; // ADD
        else if (func == "010") ALUSel = "0111"; // SLT
        else if (func == "100") ALUSel = "0011"; // SLL
        else ALUSel = "1111";
    } 
    else ALUSel = "1111";
    return ALUSel;
}

// Execute the ALU operation based on control signals
int ALUExec(string ALUSel, string rs1, string rs2) {
    // Convert binary string operands to integers
    int op1 = stoi(rs1, nullptr, 2);
    int operand2 = stoi(rs2, nullptr, 2);
    int result;
    if (ALUSel == "0000") result = op1 & operand2;      // AND
    else if (ALUSel == "0001") result = op1 | operand2; // OR
    else if (ALUSel == "0010") result = op1 + operand2; // ADD
    else if (ALUSel == "0011") result = op1 << (operand2 & 31); // SLL
    else if (ALUSel == "0110") result = op1 - operand2;         // SUB
    else if (ALUSel == "0111") result = (op1 < operand2);       // SLT
    else if (ALUSel == "1000") result = ((unsigned)op1 < (unsigned)operand2);  // SLTU
    else if (ALUSel == "1111") {
        cout << "Invalid ALU operation." << endl;
        return -1;
    }
    return result;
}

// Execute the instruction based on control signals
void execute(EXMO &exmo, IDEX &idex) {
    // Check if the fetch stage is active; if not, reset decode state and exit
    if (!states.fetch) {
        states.decode = false;
        return;
    }
    if (hazard[0]) return;  // Return if there is a data hazard
    if (skip) return; // Return if execution should be skipped

    string instr = idex.instr; // Get the instruction from the decode stage
    if (idex.control.RegRead) { // Read the first source register if RegRead control signal is active
        idex.rs1 = utilities.toBin(GPR[utilities.toDec(instr.substr(12, 5))]);
    }
    // Determine the second source register or immediate value based on ALUSrc control signal
    if (idex.control.ALUSrc && (instr.substr(25, 7) == "0010011" || instr.substr(25, 7) == "0000011")) {
        if (idex.control.RegRead) {
            idex.rs2 = idex.imm1; // Use immediate value
        }
    } else {
        if (idex.control.RegRead) idex.rs2 = utilities.toBin(GPR[utilities.toDec(instr.substr(7, 5))]); // Use second source register
    }

    // Get the ALU control signal based on the operation type
    string aluControl = ALUCtrl(idex.control.ALUOp, idex.func, idex.instr.substr(0, 7));
    string opcode = idex.instr.substr(25, 7); // Extract opcode from instruction
    // Execute ALU operation based on opcode
    if (opcode == "0100011" || opcode == "1100011") exmo.aluResult = ALUExec(aluControl, idex.rs1, idex.imm2);
    else exmo.aluResult = ALUExec(aluControl, idex.rs1, idex.rs2);
    
    int zeroFlag = (idex.rs1 == idex.rs2); // Check if the two source registers are equal
    exmo.control.copyFrom(idex); // Copy control signals to the execute stage

    // Handle branch instruction
    if (idex.control.Branch) {
        if (zeroFlag) pc = (utilities.toDec(idex.imm2) * 4 + idex.CPC); // Update program counter if branch is taken
        if (hazard[1]) skip = true; // Set skip flag if there is a control hazard
        hazard[1] = false; // Reset control hazard flag
    }

    // Handle jump instruction
    if (idex.control.Jump) {
        pc = idex.JPC; // Update program counter to jump address
        if (hazard[1]) skip = true; // Set skip flag if there is a control hazard
        hazard[1] = false; // Reset control hazard flag
    }

    exmo.rds = idex.rds; // Set destination register
    exmo.rs2 = idex.rs2; // Set second source register
    states.execute = true; // Mark execute stage as active
}

// Perform memory operations based on control signals
void memOperation(MOWB &mowb, EXMO &exmo) {
    // Check if the decode stage is active; if not, reset memory state and exit
    if (!states.decode) {
        states.execute = false;
        return;
    }

    // Perform memory write operation if enabled
    if (exmo.control.MemWrite) dMem[exmo.aluResult] = utilities.toDec(exmo.rs2);
    // Perform memory read operation if enabled
    if (exmo.control.MemRead) mowb.memoryData = dMem[exmo.aluResult];

    // Store the ALU result and copy control signals for the next stage
    mowb.aluResult = exmo.aluResult;
    mowb.control.copyFrom(exmo);
    mowb.rds = exmo.rds;
    states.memory = true; // Indicate that the memory stage is active
}

// Write back the results to the register file
void writeback(MOWB &mowb) {
    // Check if the execute stage is active; if not, reset memory state and exit
    if (!states.execute) {
        states.memory = false;
        return;
    }

    bool unlockRegister = false; // Flag to determine if the register should be unlocked
    // Check if a register write operation is needed
    if (mowb.control.RegWrite) {
        // If writing from memory, store the memory data in the register
        if (mowb.control.MemToReg) {
            GPR[utilities.toDec(mowb.rds)] = mowb.memoryData;
            // Check if the register is locked
            if (regLock[stoi(mowb.rds, NULL, 2)] == 1) unlockRegister = true;
        } else {
            // Otherwise, write the ALU result to the register
            GPR[utilities.toDec(mowb.rds)] = mowb.aluResult;
            // Check if the register is locked
            if (regLock[stoi(mowb.rds, NULL, 2)] == 1) unlockRegister = true;
        }
    }

    // If the register was unlocked, reset hazard and skip flags, and unlock the register
    if (unlockRegister) {
        hazard[0] = false;
        skip = true;
        regLock[stoi(mowb.rds, NULL, 2)] = 0;
    }
}

int main() {    
    vector<string> machineCode = {
        // // Sum of 10 numbers
        // "00000000101100110010001010000011",
        // "00000000000100001000000010010011",
        // "00000000000100101000001010010011",
        // "00000000010100001000001001100011",
        // "00000000000100010000000100110011",
        // "00000000000100001000000010010011",
        // "11111111111111111101000111101111",
        // "00000000001011111010000000100011"

        // Fibonacci Sequence
        "00000000000000000010000010000011",
        "00000000000000001000010111100011",
        "00000000000100011000000110010011",
        "00000000001100001000010011100011",
        "00000000000100000000000100010011",
        "00000000000100100000001000010011",
        "00000000000100010000001101100011",
        "00000000010100100000000110110011",
        "00000000000000100000001010110011",
        "00000000000000011000001000110011",
        "00000000000100010000000100010011",
        "11111111111111111011001101101111",
        "00000000001100000010000010100011"
    };
    
    IFID ifid;
    IDEX idex;
    EXMO exmo;
    MOWB mowb;

    iMem = machineCode;
    pc = 0;
    dMem[0] = 10;
    dMem[1] = 1;
    dMem[11] = 10;
    int n = machineCode.size();
    instrNum = n;

    int cycleCount = 1;
    while (pc < n * 4 || states.pc || states.fetch || states.decode || states.execute || states.memory) {
        if (states.memory) {
            writeback(mowb);
            cout << endl;
            // cout << "Stage 5 (writeBack)" << endl;
            cout << "Cycle " << cycleCount << " Complete." << endl;
            for (int i = 0; i < 8; i++) {
                cout << " R[" << i << "]: " << GPR[i];
            }
            cout << endl;
            cout << "dMem[0]: " << dMem[0] << ", dMem[1]: " << dMem[1] << endl;
            cycleCount++;
        }
        if (states.execute) {
            memOperation(mowb, exmo);
            // cout << endl;
            // cout << "Stage 4 (memOperation)" << endl;
        }
        if (states.decode) {
            execute(exmo, idex);
            // cout << endl;
            // cout << "Stage 3 (execute)" << endl;
        }
        if (states.fetch) {
            decode(idex, ifid, exmo);
            // cout << endl;
            // cout << "Stage 2 (decode)" << endl;
        }
        if (states.pc) {
            fetch(ifid);
            // cout << endl;
            // cout << "Stage 1 (fetch)" << endl
            // cout << "Instruction: " << pc / 4 << endl;
        }
    }
    cout << endl;
    cout << "Execution Complete." << endl;
    for (int i = 0; i < 8; i++) {
        cout << "R[" << i << "]: " << GPR[i] << endl;
    }
    cout << "dMem[0] " << dMem[0] << endl;
    cout << "dMem[1] " << dMem[1] << endl;
    return 0;
}