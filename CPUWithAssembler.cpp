#include<bits/stdc++.h>
using namespace std;

// Assembler begins here
// Class to handle immediate values
class Immediate {
private:
    string value;
    int numericValue;

public:
    // Constructor for Immediate Class
    Immediate(const string &val) : value(val), numericValue(stoi(val)) {}
    
    string toBinary(int bits) const {
        return bitset<32>(numericValue).to_string().substr(32 - bits);
    }
    
    int toInt() const {
        return numericValue;
    }
    
    string getUpperBits(int count) const {
        string binary = toBinary(12);
        return binary.substr(0, binary.size() - count);
    }
    
    string getLowerBits(int count) const {
        string binary = toBinary(12);
        return binary.substr(binary.size() - count);
    }
};

// Class to handle registers
class Register {
private:
    string name;
    int number;
public:
    // Constructor for Register Class
    Register (const string &regName) : name(regName) {
        number = stoi(regName.substr(1));
    }
    
    string toBinary() {
        return bitset<5>(number).to_string();
    }
    
    int getNumber() {
        return number;
    }
};

// Class to represent and convert RISC-V instructions
class Instruction {
private:
    string opcode;
    string rs1, rs2, rd;
    string imm;
    string funct3, funct7;
    
public:
    // Constructor for Instruction Class
    Instruction(const string &op, const string &src1, const string &src2, const string &dest, const string &immediate, const string &f3, const string &f7)
    : opcode(op), rs1(src1), rs2(src2), rd(dest), imm(immediate), funct3(f3), funct7(f7) {}
    
    // Convert R-Type instruction
    string convertRType() {
        Register dest(rd), src1(rs1), src2(rs2);
        return funct7 + src2.toBinary() + src1.toBinary() + funct3 + dest.toBinary() + opcode;
    }
    
    // Convert I-Type instruction
    string convertIType() {
        Register dest(rd), src1(rs1);
        Immediate immediate(imm);
        return immediate.toBinary(12) + src1.toBinary() + funct3 + dest.toBinary() + opcode;
    }
    
    // Convert I-Type shifting instruction
    string convertIShiftType() {
        Register dest(rd), src1(rs1);
        Immediate shamt(imm);
        return funct7 + shamt.toBinary(5) + src1.toBinary() + funct3 + dest.toBinary() + opcode;
    }
    
    // Convert L(Load)-Type instruction
    string convertLType() {
        Register dest(rd), base(rs1);
        Immediate offset(imm);
        return offset.toBinary(12) + base.toBinary() + funct3 + dest.toBinary() + opcode;
    }
    
    // Convert S-Type Instruction
    string convertSType() {
        Register base(rs1), source(rs2);
        Immediate offset(imm);
        string immBinary = offset.toBinary(12);
        return immBinary.substr(0, 7) + source.toBinary() + base.toBinary() + funct3 + immBinary.substr(7) + opcode;
    }
    
    // Convert B-Type Instruction
    string convertBType() {
        Register src1(rs1), src2(rs2);
        Immediate offset(imm);
        string immBinary = offset.toBinary(12);
        return immBinary.substr(0, 7) + src2.toBinary() + src1.toBinary() + funct3 + immBinary.substr(7, 5) + opcode;
    }
    
    // Convert U-Type instruction
    string convertUType() {
        Register dest(rd);
        Immediate immediate(imm);
        return immediate.toBinary(20) + dest.toBinary() + opcode;
    }
    
    // Convert J-Type instruction
    string convertJType() {
        Register dest(rd);
        Immediate offset(imm);
        return offset.toBinary(20) + dest.toBinary() + opcode;
    }
};

// Main class for assembling RISC-V instructions
class Assembler {
private:
    unordered_map<string, tuple<string, string, string>> instructionMap = {
        {"ADD", {"R", "000", "0000000"}},
        {"SUB", {"R", "000", "0100000"}},
        {"XOR", {"R", "100", "0000000"}},
        {"OR", {"R", "110", "0000000"}},
        {"SLL", {"R", "001", "0000000"}},
        {"SRL", {"R", "101", "0000000"}},
        {"SRA", {"R", "101", "0100000"}},
        {"SLT", {"R", "010", "0000000"}},
        {"SLTU", {"R", "011", "0000000"}},
        
        {"ADDI", {"I", "000", ""}},
        {"XORI", {"I", "100", ""}},
        {"ORI", {"I", "110", ""}},
        {"ANDI", {"I", "111", ""}},
        {"SLTI", {"I", "010", ""}},
        {"SLTIU", {"I", "011", ""}},
        
        {"SLLI", {"IS", "001", "0000000"}},
        {"SRLI", {"IS", "101", "0000000"}},
        {"SRAI", {"IS", "101", "0100000"}},
        
        {"LB", {"L", "000", ""}},
        {"LH", {"L", "001", ""}},
        {"LW", {"L", "010", ""}},
        {"LBU", {"L", "100", ""}},
        {"LHU", {"L", "101", ""}},
        
        {"SB", {"S", "000", ""}},
        {"SH", {"S", "001", ""}},
        {"SW", {"S", "010", ""}},
        
        {"BEQ", {"B", "000", ""}},
        {"BNE", {"B", "001", ""}},
        {"BLT", {"B", "100", ""}},
        {"BGE", {"B", "101", ""}},
        {"BLTU", {"B", "110", ""}},
        {"BGEU", {"B", "111", ""}},
        
        {"LUI", {"U", "", ""}},
        {"AUIPC", {"U", "", ""}},
        
        {"JAL", {"J", "", ""}},
        {"JALR", {"I", "000", ""}},
    };
    
    unordered_map<string, string> opcodeMap = {
        {"R", "0110011"},
        {"I", "0010011"},
        {"IS", "0010011"},
        {"L", "0000011"},
        {"S", "0100011"},
        {"B", "1100011"},
        {"U", "0110111"},
        {"J", "1101111"},
    };
    
    unordered_map<string, int> labelAddresses; // Maps label names to instruction addresses
    vector<pair<int, string>> labelReferences; // Stores positions needing label resolution
    int currentAddress; // Tracks current instruction address
    
    bool isLabelDefinition(const string& str) {
        return str.back() == ':';
    }
    
    bool isLabelReference(const string& str) {
        if (str.empty() || isdigit(str[0])) return false;
        return str[0] != 'x' && str[0] != 'X';
    }
    
    // To calculate relative offset between current instruction and label
    int calculateOffset(int fromAddress, int toAddress) {
        return (toAddress - fromAddress) >> 2;
    }

public:
    Assembler() : currentAddress(0) {}

    // First pass: collect label positions
    void firstPass(const vector<string>& instructions) {
        currentAddress = 0;
        labelAddresses.clear();
        labelReferences.clear();
        
        for (const string& line : instructions) {
            istringstream iss(line);
            string firstToken;
            iss >> firstToken;
            
            // Handle label definitions
            if (isLabelDefinition(firstToken)) {
                string labelName = firstToken.substr(0, firstToken.length() - 1);
                transform(labelName.begin(), labelName.end(), labelName.begin(), ::toupper);
                labelAddresses[labelName] = currentAddress;
                
                // Check if there's an instruction after the label on the same line
                string instruction;
                if (getline(iss, instruction) && !instruction.empty()) {
                    currentAddress += 4;  // Each instruction is 4 bytes
                }
            } else {
                currentAddress += 4;  // Each instruction is 4 bytes
            }
        }
    }

    vector<string> assembleMultiple(const vector<string>& instructions) {
        // First pass to collect label positions
        firstPass(instructions);
        
        // Second pass to assemble instructions
        vector<string> results;
        currentAddress = 0;
        
        for (const string& line : instructions) {
            if (line.empty() || line.find_first_not_of(" \t") == string::npos) {
                continue;  // Skip empty lines
            }
            
            istringstream iss(line);
            string firstToken;
            iss >> firstToken;
            
            // Skip pure label lines
            if (isLabelDefinition(firstToken)) {
                string remaining;
                if (!getline(iss, remaining) || remaining.find_first_not_of(" \t") == string::npos) {
                    continue;
                }
                // If there's an instruction after the label, process it
                results.push_back(assemble(remaining));
            } else results.push_back(assemble(line));
            currentAddress += 4;
        }
        
        return results;
    }
    
    string assemble(string instructionStr) {
        // Converts all instructions to upper case
        transform(instructionStr.begin(), instructionStr.end(), instructionStr.begin(), ::toupper);
        
        istringstream iss(instructionStr);
        string mnemonic;
        iss >> mnemonic;
        
        auto it = instructionMap.find(mnemonic);
        if (it == instructionMap.end()) {
            return "Invalid Instruction";
        }
        
        auto [type, funct3, funct7] = it->second;
        string opcode = opcodeMap[type];
        
        string arg1, arg2, arg3;
        iss >> arg1 >> arg2 >> arg3;
        
        if (type == "R") {
            // Handle R-type instructions
            arg1.pop_back();
            arg2.pop_back();
            Instruction instr(opcode, arg2, arg3, arg1, "", funct3, funct7);
            return instr.convertRType();
        }
        else if (type == "I") {
            // Handle I-type instructions
            arg1.pop_back();
            arg2.pop_back();
            Instruction instr(opcode, arg2, "", arg1, arg3, funct3, "");
            return instr.convertIType();
        }
        else if (type == "IS") {
            // Handle I-Shift type instructions
            arg1.pop_back();
            arg2.pop_back();
            Instruction instr(opcode, arg2, "", arg1, arg3, funct3, funct7);
            return instr.convertIShiftType();
        }
        else if (type == "L" || type == "S") {
            // Handle L-type and S-type instructions
            arg1.pop_back();
            size_t openBracket = arg2.find('(');
            size_t closeBracket = arg2.find(')');
            if (openBracket == string::npos || closeBracket == string::npos) {
                return "Invalid Instruction";
            }
            string imm = arg2.substr(0, openBracket);
            string rs1 = arg2.substr(openBracket + 1, closeBracket - openBracket - 1);
            
            if (type == "L") {
                Instruction instr(opcode, rs1, "", arg1, imm, funct3, "");
                return instr.convertLType();
            } else { // type = "S"
                Instruction instr(opcode, rs1, arg1, "", imm, funct3, "");
                return instr.convertSType();
            }
        }
        else if (type == "B") {
            // Handle B-type instructions
            arg1.pop_back();
            arg2.pop_back();
            
            if (isLabelReference(arg3)) {
                string labelName = arg3;
                transform(labelName.begin(), labelName.end(), labelName.begin(), ::toupper);
                
                if (labelAddresses.find(labelName) != labelAddresses.end()) {
                    int offset = calculateOffset(currentAddress, labelAddresses[labelName]);
                    arg3 = to_string(offset);
                } else {
                    return "Undefined Label: " + labelName;
                }
            }
            
            Instruction instr(opcode, arg1, arg2, "", arg3, funct3, "");
            return instr.convertBType();
        }
        else if (type == "J") {
            arg1.pop_back();
            
            if (isLabelReference(arg2)) {
                string labelName = arg2;
                transform(labelName.begin(), labelName.end(), labelName.begin(), ::toupper);
                
                if (labelAddresses.find(labelName) != labelAddresses.end()) {
                    int offset = calculateOffset(currentAddress, labelAddresses[labelName]);
                    arg2 = to_string(offset);
                } else {
                    return "Undefined Label: " + labelName;
                }
            }
            
            Instruction instr(opcode, "", "", arg1, arg2, "", "");
            return instr.convertJType();
        }
        else if (type == "U") {
            // Handle U-Type instructions
            arg1.pop_back();
            Instruction instr(opcode, "", "", arg1, arg2, "", "");
            return instr.convertUType();
        }
        else {
            return "Unsupported Instruction Type";
        }
    }
};
// Assembler Design Ends Here

// CPU Design Starts Here
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
    Assembler assembler;
    vector<string> instructions = {
        // Two Sample Codes given
        // Comment out the code not to be executed.
        // Please use normal brackets "()" only!
        
        // // Sum of n numbers
        // "lw x5, 11(x6)", //int n = 10
        // "addi x1, x1, 1",
        // "addi x5, x5, 1",
        // "sum_loop:",
        // "beq x1, x5, done",
        // "add x2, x2, x1",
        // "addi x1, x1, 1",
        // "jal x3, sum_loop",
        // "done:",
        // "sw x2, 0(x31)"

        // Fibonacci Sequence
        "lw x1, 0(x0)",
        "beq x1, x0, done",
        "addi x3, x3, 1",
        "beq x1, x3, done",
        "addi x2, x0, 1",
        "addi x4, x4, 1",
        "for:",
        "beq x2, x1, done",
        "add x3, x4, x5",
        "add x5, x4, x0",
        "add x4, x3, x0",
        "addi x2, x2, 1",
        "jal x6, for",
        "done:",
        "sw x3, 1(x0)"
    };
    
    vector<string> machineCode = assembler.assembleMultiple(instructions);
    cout << "\nGenerated Machine Code:" << endl;
    for (size_t i = 0; i < machineCode.size(); ++i) {
        cout << machineCode[i] << endl;
    }
    
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