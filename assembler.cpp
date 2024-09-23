// Om Prakash Behera (IIT Bhubaneswar)

#include <bits/stdc++.h>
using namespace std;

// Class to handle immediate values in RISC-V instructions
class Immediate {
private:
    string value;
    int numericValue;

public:
    // Constructor for Immediate class
    Immediate(const string &val) : value(val), numericValue(stoi(val)) {}

    string toBinary(int bits) const {
        return bitset<32>(numericValue).to_string().substr(32 - bits);
    }

    int toInt() const { return numericValue; }

    string getUpperBits(int count) const {
        string binary = toBinary(12);
        return binary.substr(0, binary.size() - count);
    }

    string getLowerBits(int count) const {
        string binary = toBinary(12);
        return binary.substr(binary.size() - count);
    }
};

// Class to handle registers in RISC-V instructions
class Register {
private:
    string name;
    int number;

public:
    // Constructor for Register Class
    Register(const string &regName) : name(regName) {
        number = stoi(regName.substr(1));
    }

    string toBinary() const {
        return bitset<5>(number).to_string();
    }

    int getNumber() const { return number; }
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
    Instruction(const string &op, const string &source1, const string &source2, const string &dest, const string &immediate, const string &f3, const string &f7)
        : opcode(op), rs1(source1), rs2(source2), rd(dest), imm(immediate), funct3(f3), funct7(f7) {}

    // Convert R-type instruction (e.g., ADD, SUB)
    string convertRType() const {
        Register dest(rd), source1(rs1), source2(rs2);
        return funct7 + source2.toBinary() + source1.toBinary() + funct3 + dest.toBinary() + opcode;
    }

    // Convert I-type instruction (e.g., ADDI)
    string convertIType() const {
        Register dest(rd), source1(rs1);
        Immediate immediate(imm);
        return immediate.toBinary(12) + source1.toBinary() + funct3 + dest.toBinary() + opcode;
    }

    // Convert I-type shifting instruction (e.g., SLLI, SRLI)
    string convertIShiftType() const {
        Register dest(rd), source1(rs1);
        Immediate shamt(imm);
        return funct7 + shamt.toBinary(5) + source1.toBinary() + funct3 + dest.toBinary() + opcode;
    }

    // Convert Load-type instruction (e.g., LW, LH)
    string convertLoadType() const {
        Register dest(rd), base(rs1);
        Immediate offset(imm);
        return offset.toBinary(12) + base.toBinary() + funct3 + dest.toBinary() + opcode;
    }

    // Convert S-type instruction (e.g., SW, SH)
    string convertSType() const {
        Register base(rs1), source(rs2);
        Immediate offset(imm);
        string immBinary = offset.toBinary(12);
        return immBinary.substr(0, 7) + source.toBinary() + base.toBinary() + funct3 + immBinary.substr(7) + opcode;
    }

    // Convert B-type instruction (e.g., BEQ, BNE)
    string convertBType() const {
        Register source1(rs1), source2(rs2);
        Immediate offset(imm);
        string immBinary = offset.toBinary(13);
        return immBinary[0] + immBinary.substr(2, 6) + source2.toBinary() + source1.toBinary() + funct3 + immBinary.substr(8, 4) + immBinary[1] + opcode;
    }

    // Convert U-type instruction (e.g., LUI, AUIPC)
    string convertUType() const {
        Register dest(rd);
        Immediate immediate(imm);
        return immediate.toBinary(20) + dest.toBinary() + opcode;
    }

    // Convert J-type instruction (e.g., JAL)
    string convertJType() const {
        Register dest(rd);
        Immediate offset(imm);
        string immBinary = offset.toBinary(21);
        return immBinary[0] + immBinary.substr(10, 10) + immBinary[9] + immBinary.substr(1, 8) + dest.toBinary() + opcode;
    }
};

// Main class for assembling RISC-V instructions
class Assembler {
private:
    unordered_map<string, tuple<string, string, string>> instructionMap = {
        {"ADD",  {"R",  "000", "0000000"}},
        {"SUB",  {"R",  "000", "0100000"}},
        {"XOR",  {"R",  "100", "0000000"}},
        {"OR",   {"R",  "110", "0000000"}},
        {"AND",  {"R",  "111", "0000000"}},
        {"SLL",  {"R",  "001", "0000000"}},
        {"SRL",  {"R",  "101", "0000000"}},
        {"SRA",  {"R",  "101", "0100000"}},
        {"SLT",  {"R",  "010", "0000000"}},
        {"SLTU", {"R",  "011", "0000000"}},
        {"ADDI", {"I",  "000", ""}},
        {"XORI", {"I",  "100", ""}},
        {"ORI",  {"I",  "110", ""}},
        {"ANDI", {"I",  "111", ""}},
        {"SLLI", {"IS", "001", "0000000"}},
        {"SRLI", {"IS", "101", "0000000"}},
        {"SRAI", {"IS", "101", "0100000"}},
        {"SLTI", {"I",  "010", ""}},
        {"SLTIU",{"I",  "011", ""}},
        {"LB",   {"L",  "000", ""}},
        {"LH",   {"L",  "001", ""}},
        {"LW",   {"L",  "010", ""}},
        {"LBU",  {"L",  "100", ""}},
        {"LHU",  {"L",  "101", ""}},
        {"SB",   {"S",  "000", ""}},
        {"SH",   {"S",  "001", ""}},
        {"SW",   {"S",  "010", ""}},
        {"BEQ",  {"B",  "000", ""}},
        {"BNE",  {"B",  "001", ""}},
        {"BLT",  {"B",  "100", ""}},
        {"BGE",  {"B",  "101", ""}},
        {"BLTU", {"B",  "110", ""}},
        {"BGEU", {"B",  "111", ""}},
        {"LUI",  {"U",  "",    ""}},
        {"AUIPC",{"U",  "",    ""}},
        {"JAL",  {"J",  "",    ""}},
        {"JALR", {"I",  "000", ""}}
    };

    unordered_map<string, string> opcodeMap = {
        {"R",  "0110011"},
        {"I",  "0010011"},
        {"IS", "0010011"},
        {"L",  "0000011"},
        {"S",  "0100011"},
        {"B",  "1100011"},
        {"U",  "0110111"},
        {"J",  "1101111"}
    };

public:
    string assemble(string instructionStr) {
        // Converts all instructions to upper case
        transform(instructionStr.begin(), instructionStr.end(), instructionStr.begin(), ::toupper);
        
        istringstream iss(instructionStr);
        string mnemonic;
        iss >> mnemonic;

        auto it = instructionMap.find(mnemonic);
        if (it == instructionMap.end()) {
            return "Invalid instruction";
        }

        auto [type, funct3, funct7] = it->second;
        string opcode = opcodeMap[type];

        string arg1, arg2, arg3;
        iss >> arg1 >> arg2 >> arg3;

        if (type == "R") {
            // Handle R-Type instructions
            arg1.pop_back(); arg2.pop_back();
            Instruction instr(opcode, arg2, arg3, arg1, "", funct3, funct7);
            return instr.convertRType();
        } else if (type == "I") {
            // Handle I-Type instructions
            arg1.pop_back(); arg2.pop_back();
            Instruction instr(opcode, arg2, "", arg1, arg3, funct3, "");
            return instr.convertIType();
        } else if (type == "IS") {
            // Handle I-Shift type instructions
            arg1.pop_back(); arg2.pop_back();
            Instruction instr(opcode, arg2, "", arg1, arg3, funct3, funct7);
            return instr.convertIShiftType();
        } else if (type == "L" || type == "S") {
            // Handle L-type & I-Type instructions
            arg1.pop_back();
            size_t openBracket = arg2.find('[');
            size_t closeBracket = arg2.find(']');
            if (openBracket == string::npos || closeBracket == string::npos) {
                return "Invalid load/store instruction format";
            }
            string imm = arg2.substr(0, openBracket);
            string rs1 = arg2.substr(openBracket + 1, closeBracket - openBracket - 1);
            
            if (type == "L") {
                Instruction instr(opcode, rs1, "", arg1, imm, funct3, "");
                return instr.convertLoadType();
            } else { // type == "S"
                Instruction instr(opcode, rs1, arg1, "", imm, funct3, "");
                return instr.convertSType();
            }
        } else if (type == "B") {
            // Handle B-Type Instructions
            arg1.pop_back(); arg2.pop_back();
            Instruction instr(opcode, arg1, arg2, "", arg3, funct3, "");
            return instr.convertBType();
        } else if (type == "U" || type == "J") {
            // Handle U-Type and J-Type Instructions
            arg1.pop_back();
            Instruction instr(opcode, "", "", arg1, arg2, "", "");
            return type == "U" ? instr.convertUType() : instr.convertJType();
        } else {
            return "Unsupported instruction type";
        }
    }
};

int main() {
    Assembler assembler;
    string instruction = "SW X1, 8[X2]"; // Example instruction
    string machineCode = assembler.assemble(instruction);
    cout << machineCode << endl;
    return 0;
}
