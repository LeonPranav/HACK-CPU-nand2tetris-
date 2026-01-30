import sys

def retInstr(line):
    if line[0] == '@':
        return 0
    elif line[0] == '(':
        return 1
    else:
        return 2
    
def symbol(instr, line):
    if instr == 0:
        return line[1::]
    elif instr == 1:
        return line[1:-1:]
    
def cinst(line):
    line1 = line.split(";")
    line2 = line1[0].split("=")
    if len(line1) == 1:
        return [line2[0], line2[1], "null"]
    elif len(line2) == 1:
        return ["null", line2[0], line1[1]]
    else:
        return [line2[0], line2[1], line1[1]]
    
def solveDest(dest):
    a = ['0','0','0']
    for l in dest:
        if l == 'A':
            a[0] = '1'
        elif l == 'D':
            a[1] = '1'
        elif l == 'M':
            a[2] = '1'
    result = ''.join(str(element) for element in a)
    return result

def solveComp(comp):
    if comp == "0":
        return "0101010"
    if comp == "1":
        return "0111111"
    if comp == "-1":
        return "0111010"
    if comp == "D":
        return "0001100"
    if comp == "A":
        return "0110000"
    if comp == "M":
        return "1110000"
    if comp ==  "!D":
        return "0001101"
    if comp == "!A":
        return "0110001"
    if comp == "!M":
        return "1110001"
    if comp == "-D":
        return "0001111"
    if comp == "-A":
        return "0110011"
    if comp == "-M":
        return "1110011"
    if comp == "D+1":
        return "0011111"
    if comp == "A+1":
        return "0110111"
    if comp == "M+1":
        return "1110111"
    if comp == "D-1":
        return "0001110"
    if comp == "A-1":
        return "0110010"
    if comp == "M-1":
        return "1110010"
    if comp == "D+A":
        return "0000010"
    if comp == "D-A":
        return "0010011"
    if comp == "A-D":
        return "0000111"
    if comp == "D&A":
        return "0000000"
    if comp == "D|A":
        return "0010101"
    if comp == "D+M":
        return "1000010"
    if comp == "D-M":
        return "1010011"
    if comp == "M-D":
        return "1000111"
    if comp == "D&M":
        return "1000000"
    if comp == "D|M":
        return "1010101"
    
def solveJump(jump):
    if jump == "null":
        return "000"
    if jump == "JGT":
        return "001" 
    if jump == "JEQ":
        return "010"
    if jump == "JGE":
        return "011" 
    if jump == "JLT":
        return "100" 
    if jump == "JNE":
        return "101" 
    if jump == "JLE":
        return "110"
    if jump == "JMP":
        return "111" 

def addEntry(table, symbol, address):
    table[symbol] = address

def contains(table, symbol):
    return table.get(symbol, -1)

def main():
    table = {"R0": 0, "R1": 1, "R2": 2, "R3": 3, "R4": 4, "R5": 5, "R6": 6, "R7": 7, "R8": 8, "R9": 9, "R10": 10, "R11": 11, "R12": 12, "R13": 13, "R14": 14, "R15": 15, "SCREEN": 16384, "KBD": 24576, "SP": 0, "LCL": 1, "ARG": 2, "THIS": 3, "THAT": 4}
    count = 0
    strings = []

    filename = sys.argv[1]
    with open(filename, "r") as file:
        for line in file:
            line = line.strip()
            if line == "":
                continue
            elif line[0] == "/":
                continue
            instr = retInstr(line)
            sym = symbol(instr, line)
            if instr == 1:
                addEntry(table, sym, count)
            if instr != 1:
                count += 1

    temp = 16
    with open(filename, "r") as file:
        for line in file:
            line = line.strip()
            if line == "":
                continue
            elif line[0] == "/":
                continue
            instr = retInstr(line)
            sym = symbol(instr, line)

            if instr == 0:
                y = ""
                if (sym[0] >='a' and sym[0] <= 'z') or (sym[0] >='A' and sym[0] <= 'Z'):
                    x = contains(table, sym)
                    if x == -1:
                        addEntry(table, sym, temp)
                        temp += 1
                    x = contains(table, sym)
                    y = format(x, '016b')
                else:
                    y = int(sym)
                    y = format(y, '016b')
                strings.append(y)

            elif instr == 1:
                continue

            elif instr == 2:
                check = cinst(line)
                a = solveDest(check[0])
                b = solveComp(check[1])
                c = solveJump(check[2])

                out = [1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0]
                out[3] = b[0]
                out[4:10] = b[1:]
                out[10:13] = a
                out[13:] = c

                result = ''.join(str(element) for element in out)

                strings.append(result)
    
    for s in strings:
        print(s)
main()