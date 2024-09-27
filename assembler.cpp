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
    
public:
    vector<string> assembleMultiple(const vector<string>& instructions) {
        vector<string> results;
        for (const auto& instruction : instructions) {
            results.push_back(assemble(instruction));
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
            // Handle L-type instructions
            arg1.pop_back();
            size_t openBracket = arg2.find('[');
            size_t closeBracket = arg2.find(']');
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
            // Hande B-Type instructions
            arg1.pop_back();
            arg2.pop_back();
            Instruction instr(opcode, arg1, arg2, "", arg3, funct3, "");
            return instr.convertBType();
        }
        else if (type == "U" || type == "J") {
            // Handle U-Type and J-Type instructions
            arg1.pop_back();
            Instruction instr(opcode, "", "", arg1, arg2, "", "");
            return type == "U" ? instr.convertUType() : instr.convertJType();
        } else {
            return "Unsupported Instruction Type";
        }
    }
};

int main() {
    Assembler assembler;
    vector<string> instructions = {
        "SUB x1, x2, x3",
        "ADDI x1, x2, 10",
        "SRAI x1, x2, 5",
        "LB x1, 10[x12]",
        "BNE x1, x2, 5",
    };
    
    vector<string> machineCodes = assembler.assembleMultiple(instructions);
    for (size_t i = 0; i < instructions.size(); ++i) {
        cout << machineCodes[i] << endl;
    }
}
