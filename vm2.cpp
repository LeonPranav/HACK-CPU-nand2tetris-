#include <bits/stdc++.h>
using namespace std;

string fileBaseName(const string &path) {
    size_t pos = path.find_last_of("/\\");
    string fname = (pos == string::npos) ? path : path.substr(pos+1);
    size_t dot = fname.find_last_of('.');
    if (dot == string::npos) return fname;
    return fname.substr(0, dot);
}

string pushDToStack() {
    return string(
        "@SP\n"
        "A=M\n"
        "M=D\n"
        "@SP\n"
        "M=M+1\n"
    );
}

string popStackToD() {
    return string(
        "@SP\n"
        "M=M-1\n"
        "A=M\n"
        "D=M\n"
    );
}

string pushConstant(int index) {
    ostringstream out;
    out << "@" << index << "\nD=A\n" << pushDToStack();
    return out.str();
}

string pushSegment(const string &base, int index) {
    ostringstream out;
    if (base == "temp") {
        int address = 5 + index;
        out << "@" << address << "\nD=M\n" << pushDToStack();
        return out.str();
    } else if (base == "pointer") {
        string sym = (index == 0 ? "THIS" : "THAT");
        out << "@" << sym << "\nD=M\n" << pushDToStack();
        return out.str();
    } else {
        out << "@" << base << "\nD=M\n@" << index << "\nA=D+A\nD=M\n" << pushDToStack();
        return out.str();
    }
}

string popToSegment(const string &base, int index) {
    ostringstream out;
    if (base == "temp") {
        int address = 5 + index;
        out << popStackToD() << "@" << address << "\nM=D\n";
        return out.str();
    } else if (base == "pointer") {
        string sym = (index == 0 ? "THIS" : "THAT");
        out << popStackToD() << "@" << sym << "\nM=D\n";
        return out.str();
    } else {
        out << "@" << base << "\nD=M\n@" << index << "\nD=D+A\n@R13\nM=D\n"  // R13 = base+index
            << popStackToD()
            << "@R13\nA=M\nM=D\n";
        return out.str();
    }
}

string Arithmetic(const string &cmd, int &labelCounter) {
    ostringstream out;
    if (cmd == "add" || cmd == "sub" || cmd == "and" || cmd == "or") {
        out << "@SP\nM=M-1\nA=M\nD=M\n"
            << "@SP\nA=M-1\n";
        if (cmd == "add") out << "M=M+D\n";
        else if (cmd == "sub") out << "M=M-D\n";
        else if (cmd == "and") out << "M=M&D\n";
        else if (cmd == "or")  out << "M=M|D\n";
    }
    else if (cmd == "neg" || cmd == "not") {
        if (cmd == "neg") {
            out << "@SP\nM=M-1\nA=M\nM=-M\n@SP\nM=M+1\n";
        } else {
            out << "@SP\nM=M-1\nA=M\nM=!M\n@SP\nM=M+1\n";
        }
    }
    else if (cmd == "eq" || cmd == "gt" || cmd == "lt") {
        string labelTrue = "T" + to_string(labelCounter);
        string labelEnd  = "E"  + to_string(labelCounter);
        labelCounter++;
        
        out << "@SP\nM=M-1\nA=M\nD=M\n"   
            << "@SP\nA=M-1\nD=M-D\n"
            << "@" << labelTrue << "\n";
        if (cmd == "eq") out << "D;JEQ\n";
        else if (cmd == "gt") out << "D;JGT\n";
        else if (cmd == "lt") out << "D;JLT\n";
        out << "@SP\nA=M-1\nM=0\n"
            << "@" << labelEnd << "\n0;JMP\n"
            << "(" << labelTrue << ")\n"
            << "@SP\nA=M-1\nM=-1\n"
            << "(" << labelEnd << ")\n";
    }
    else {
    }
    return out.str();
}

int globalLabelCounter = 0;

string writeBootstrap() {
    ostringstream out;
    out << "@256\nD=A\n@SP\nM=D\n";
    string returnLabel = "RET_BOOT_" + to_string(globalLabelCounter++);
    out << "@" << returnLabel << "\nD=A\n" << pushDToStack();
    out << "@LCL\nD=M\n" << pushDToStack();
    out << "@ARG\nD=M\n" << pushDToStack();
    out << "@THIS\nD=M\n" << pushDToStack();
    out << "@THAT\nD=M\n" << pushDToStack();
    out << "@SP\nD=M\n@5\nD=D-A\n@ARG\nM=D\n";
    out << "@SP\nD=M\n@LCL\nM=D\n";
    out << "@Sys.init\n0;JMP\n";
    out << "(" << returnLabel << ")\n";
    return out.str();
}

string writeCall(const string &functionName, int nArgs) {
    ostringstream out;
    string returnLabel = functionName + "$ret." + to_string(globalLabelCounter++);
    out << "@" << returnLabel << "\nD=A\n" << pushDToStack();
    out << "@LCL\nD=M\n" << pushDToStack();
    out << "@ARG\nD=M\n" << pushDToStack();
    out << "@THIS\nD=M\n" << pushDToStack();
    out << "@THAT\nD=M\n" << pushDToStack();
    out << "@SP\nD=M\n@" << (5 + nArgs) << "\nD=D-A\n@ARG\nM=D\n";
    out << "@SP\nD=M\n@LCL\nM=D\n";
    out << "@" << functionName << "\n0;JMP\n";
    out << "(" << returnLabel << ")\n";
    return out.str();
}

string writeFunction(const string &functionName, int nLocals) {
    ostringstream out;
    out << "(" << functionName << ")\n";
    for (int i = 0; i < nLocals; ++i) {
        out << "@0\nD=A\n" << pushDToStack();
    }
    return out.str();
}

string writeReturn() {
    ostringstream out;
    out << "@LCL\nD=M\n@R13\nM=D\n";
    out << "@5\nA=D-A\nD=M\n@R14\nM=D\n";
    out << popStackToD() << "@ARG\nA=M\nM=D\n";
    out << "@ARG\nD=M\n@SP\nM=D+1\n";
    out << "@R13\nAM=M-1\nD=M\n@THAT\nM=D\n";
    out << "@R13\nAM=M-1\nD=M\n@THIS\nM=D\n";
    out << "@R13\nAM=M-1\nD=M\n@ARG\nM=D\n";
    out << "@R13\nAM=M-1\nD=M\n@LCL\nM=D\n";
    out << "@R14\nA=M\n0;JMP\n";
    return out.str();
}

int main(int argc, char **argv) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <file.vm>\n";
        return 1;
    }

    string inpath = argv[1];
    ifstream fin(inpath);
    if (!fin) {
        cerr << "Cannot open input file " << inpath << "\n";
        return 1;
    }

    string outpath;
    size_t dot = inpath.find_last_of('.');
    if (dot != string::npos) outpath = inpath.substr(0, dot) + ".asm";
    else outpath = inpath + ".asm";

    ofstream fout(outpath);
    if (!fout) {
        cerr << "Cannot open output file " << outpath << "\n";
        return 1;
    }

    string base = fileBaseName(inpath);
    string line;
    int labelCounter = 0;

    fout << writeBootstrap();

    while (getline(fin, line)) {
        size_t cpos = line.find("//");
        if (cpos != string::npos) line.erase(cpos);
        auto ltrim = [](string &s) {
            size_t p = s.find_first_not_of(" \t\r\n");
            if (p == string::npos) { s.clear(); return; }
            s.erase(0, p);
        };
        auto rtrim = [](string &s) {
            size_t p = s.find_last_not_of(" \t\r\n");
            if (p == string::npos) { s.clear(); return; }
            s.erase(p+1);
        };
        ltrim(line); rtrim(line);
        if (line.empty()) continue;
        istringstream iss(line);
        string cmd; iss >> cmd;

        if (cmd == "push") {
            string seg; int idx;
            iss >> seg >> idx;
            if (seg == "constant") {
                fout << pushConstant(idx);
            } else if (seg == "local") {
                fout << pushSegment("LCL", idx);
            } else if (seg == "argument") {
                fout << pushSegment("ARG", idx);
            } else if (seg == "this") {
                fout << pushSegment("THIS", idx);
            } else if (seg == "that") {
                fout << pushSegment("THAT", idx);
            } else if (seg == "temp") {
                fout << pushSegment("temp", idx);
            } else if (seg == "pointer") {
                fout << pushSegment("pointer", idx);
            } else if (seg == "static") {
                fout << "@" << base << "." << idx << "\nD=M\n" << pushDToStack();
            }
        }
        else if (cmd == "pop") {
            string seg; int idx;
            iss >> seg >> idx;
            if (seg == "local") {
                fout << popToSegment("LCL", idx);
            } else if (seg == "argument") {
                fout << popToSegment("ARG", idx);
            } else if (seg == "this") {
                fout << popToSegment("THIS", idx);
            } else if (seg == "that") {
                fout << popToSegment("THAT", idx);
            } else if (seg == "temp") {
                fout << popToSegment("temp", idx);
            } else if (seg == "pointer") {
                fout << popToSegment("pointer", idx);
            } else if (seg == "static") {
                fout << popStackToD() << "@" << base << "." << idx << "\nM=D\n";
            }
        }
        else if (cmd == "label") {
            string seg;
            iss >> seg;
            fout << "(" << seg << ")\n";
        }
        else if (cmd == "goto") {
            string seg;
            iss >> seg;
            fout << "@" << seg << "\n0;JMP\n";
        }
        else if (cmd == "if-goto") {
            string seg;
            iss >> seg;
            fout << "@SP\nM=M-1\nA=M\nD=M\n@" << seg << "\nD;JNE\n";
        }
        else if (cmd == "call") {
            string func; int nArgs;
            iss >> func >> nArgs;
            fout << writeCall(func, nArgs);
        }
        else if (cmd == "function") {
            string func; int nLocals;
            iss >> func >> nLocals;
            fout << writeFunction(func, nLocals);
        }
        else if (cmd == "return") {
            fout << writeReturn();
        }
        else {
            string asmBlock = Arithmetic(cmd, labelCounter);
            fout << asmBlock;
        }
    }

    fout << "(END)\n@END\n0;JMP\n";
    return 0;
}
