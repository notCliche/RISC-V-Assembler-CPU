// Author: Om Prakash Behera
#include<bits/stdc++.h>
using namespace std;

//Class to handle immediate values
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
                cout << "Label defined: " << labelName << " at address " << currentAddress << endl; // Debug output
                
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

int main() {
    Assembler assembler;
    vector<string> instructions = {
        // Sum of n numbers
        "lw x5, 11(x6)", //int n = 10
        "addi x1, x1, 1",
        "addi x5, x5, 1",
        "sum_loop:",
        "beq x1, x5, done",
        "add x2, x2, x1",
        "addi x1, x1, 1",
        "jal x3, sum_loop",
        "done:",
        "sw x2, 0(x31)"

        // // Fibonacci
        // "lw x1, 0(x0)",
        // "beq x1, x0, done",
        // "addi x3, x3, 1",
        // "beq x1, x3, done",
        // "addi x2, x0, 1",
        // "addi x4, x4, 1",
        // "for:",
        // "beq x2, x1, done",
        // "add x3, x4, x5",
        // "add x5, x4, x0",
        // "add x4, x3, x0",
        // "addi x2, x2, 1",
        // "jal x6, for",
        // "done:",
        // "sw x3, 1(x0)"
    };
    
    try {
        vector<string> machineCodes = assembler.assembleMultiple(instructions);
        cout << "\nGenerated Machine Codes:" << endl;
        for (size_t i = 0; i < machineCodes.size(); ++i) {
            cout << "Instruction " << i << ": " << instructions[i] << endl;
            cout << "Machine Code: " << machineCodes[i] << endl << endl;
        }
    } catch (const exception& e) {
        cout << "Error during assembly: " << e.what() << endl;
    }
}