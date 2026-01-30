#include <bits/stdc++.h>
#include <dirent.h>
#include <sys/stat.h>
using namespace std;

string trim(const string &s){
    size_t a=0,b=s.size();
    while(a<b && isspace((unsigned char)s[a])) ++a;
    while(b>a && isspace((unsigned char)s[b-1])) --b;
    return s.substr(a,b-a);
}
string xmlDecode(const string &s){
    string r;
    for(size_t i=0;i<s.size();++i){
        if(s[i]=='&') {
            if(s.compare(i,4,"&lt;")==0){ r.push_back('<'); i+=3; }
            else if(s.compare(i,4,"&gt;")==0){ r.push_back('>'); i+=3; }
            else if(s.compare(i,5,"&amp;")==0){ r.push_back('&'); i+=4; }
            else if(s.compare(i,6,"&quot;")==0){ r.push_back('"'); i+=5; }
            else { r.push_back('&'); }
        } else r.push_back(s[i]);
    }
    return r;
}

struct XMLTokenizer {
    vector<string> tokens;
    size_t idx=0;
    XMLTokenizer() = default;
    XMLTokenizer(const string &xml) { 
        parse(xml);
    }
    void parse(const string &s) {
        tokens.clear();
        size_t i=0,n=s.size();
        while(i<n){
            size_t open = s.find('<', i);
            if(open==string::npos) {
                break;
            }
            size_t gt = s.find('>', open+1);
            if(gt==string::npos) break;
            string tag = trim(s.substr(open+1, gt-open-1));
            if(tag.size() && tag[0]=='/') {
                i = gt+1; 
                continue; 
            }
            if(tag=="keyword" || tag=="symbol" || tag=="identifier" || tag=="integerConstant" || tag=="stringConstant"){
                string endTag = "</" + tag + ">";
                size_t close = s.find(endTag, gt+1);
                if(close==string::npos) break;
                string content = s.substr(gt+1, close-(gt+1));
                content = trim(content);
                content = xmlDecode(content);
                if(tag=="stringConstant"){
                    tokens.push_back(string("\"") + content + string("\""));
                } 
                else {
                    tokens.push_back(content);
                }
                i = close + endTag.size();
            } 
            else {
                i = gt+1;
            }
        }
    }
    bool hasMoreTokens() { 
        return idx < tokens.size(); 
    }
    string peek() { 
        return idx < tokens.size() ? tokens[idx] : string(); 
    }
    string advance() { 
        return idx < tokens.size() ? tokens[idx++] : string(); 
    }
};

struct Symbol { 
    string type; 
    string kind; 
    int index; 
};

struct SymbolTable {
    unordered_map<string,Symbol> classScope;
    unordered_map<string,Symbol> subScope;
    int staticCount=0, fieldCount=0, argCount=0, varCount_=0;
    void startSubroutine(){ subScope.clear(); argCount=0; varCount_=0; }
    void define(const string &name,const string &type,const string &kind){
        if(kind=="static"){ 
            classScope[name]={type,kind,staticCount++}; 
        }
        else if(kind=="field"){ 
            classScope[name]={type,kind,fieldCount++}; 
        }
        else if(kind=="arg"){ 
            subScope[name]={type,kind,argCount++}; 
        }
        else if(kind=="var"){ 
            subScope[name]={type,kind,varCount_++}; 
        }
    }
    int varCount(const string &kind) const {
        if(kind=="static") {
            return staticCount;
        }
        if(kind=="field") {
            return fieldCount;
        }
        if(kind=="arg") {
            return argCount;
        }
        if(kind=="var") {
            return varCount_;
        }
        return 0;
    }
    bool exists(const string &name) {
        return subScope.find(name)!=subScope.end() || classScope.find(name)!=classScope.end();
    }
    const Symbol* get(const string &name) const {
        auto it = subScope.find(name);
        if(it!=subScope.end()) {
            return &it->second;
        }
        it = classScope.find(name);
        if(it!=classScope.end()) {
            return &it->second;
        }
        return nullptr;
    }
    string kindOf(const string &name) { 
        auto s=get(name); 
        return s? s->kind : "none"; 
    }
    string typeOf(const string &name) {
        auto s=get(name); 
        return s? s->type : ""; 
    }
    int indexOf(const string &name) { 
        auto s=get(name); 
        return s? s->index : -1; 
    }
};

struct VMWriter {
    vector<string> out;
    void writePush(const string &segment,int index){ 
        out.push_back("push "+segment+" "+to_string(index)); 
    }
    void writePop(const string &segment,int index){ 
        out.push_back("pop "+segment+" "+to_string(index)); 
    }
    void writeArithmetic(const string &cmd){ 
        out.push_back(cmd); 
    }
    void writeLabel(const string &label){ 
        out.push_back("label "+label); 
    }
    void writeGoto(const string &label){ 
        out.push_back("goto "+label); 
    }
    void writeIf(const string &label){ 
        out.push_back("if-goto "+label); 
    }
    void writeCall(const string &name,int nArgs){ 
        out.push_back("call "+name+" "+to_string(nArgs)); 
    }
    void writeFunction(const string &name,int nLocals){ 
        out.push_back("function "+name+" "+to_string(nLocals)); 
    }
    void writeReturn(){ 
        out.push_back("return"); 
    }
    string getSegment(const string &kind) const {
        if(kind=="static") {
            return "static";
        }
        if(kind=="field") {
            return "this";
        }
        if(kind=="arg") {
            return "argument";
        }
        if(kind=="var") {
            return "local";
        }
        return "";
    }
    void dump(ostream &os) {
        for(auto &s:out) os<<s<<"\n"; 
    }
};

struct TokenStream {
    vector<string> tokens;
    size_t idx=0;
    TokenStream() = default;
    TokenStream(vector<string> t):tokens(move(t)),idx(0){}
    string peek() { 
        return idx < tokens.size() ? tokens[idx] : string(); 
    }
    string advance(){ 
        return idx < tokens.size() ? tokens[idx++] : string(); 
    }
    bool hasMore() { 
        return idx < tokens.size(); 
    }
};

struct CompilationEngine {
    TokenStream tk;
    string className;
    SymbolTable st;
    VMWriter vm;
    int labelCount=0;
    CompilationEngine(const TokenStream &ts):tk(ts){}
    string next(){
        return tk.advance(); 
    }
    string peek(){ 
        return tk.peek(); 
    }
    void rollback(){ 
        if(tk.idx>0) {
            tk.idx--; 
        }
    }
    string eat() { 
        return next();
    }
    bool isType(const string &s){
        if(s=="int"||s=="char"||s=="boolean") {
            return true;
        }
        if(!s.empty() && isalpha((unsigned char)s[0])) {
            return true;
        }
        return false;
    }

    void compileClass(){
        eat(); // class
        className = eat();
        eat(); // {
        while(peek()=="static"||peek()=="field") {
            compileClassVarDec();
        }
        while(peek()=="constructor"||peek()=="function"||peek()=="method") {
            compileSubroutine();
        }
        eat(); // }
    }

    void compileClassVarDec(){
        string kind = eat();
        string type = eat();
        string name = eat();
        st.define(name,type,kind);
        while(peek()==","){ 
            eat(); 
            string n=eat(); 
            st.define(n,type,kind); 
        }
        eat(); // ;
    }

    void compileSubroutine(){
        string subKind = eat();
        string retType = eat();
        string name = eat();
        st.startSubroutine();
        if(subKind=="method") {
            st.define("this", className, "arg");
        }
        eat(); // (
        compileParameterList();
        eat(); // )
        eat(); // {
        while(peek()=="var") {
            compileVarDec();
        }
        string fname = className + "." + name;
        int nLocals = st.varCount("var");
        vm.writeFunction(fname, nLocals);
        if(subKind=="constructor"){
            int fields = st.varCount("field");
            vm.writePush("constant", fields);
            vm.writeCall("Memory.alloc", 1);
            vm.writePop("pointer", 0);
        } 
        else if(subKind=="method"){
            vm.writePush("argument", 0);
            vm.writePop("pointer", 0);
        }
        compileStatements();
        eat(); // }
    }

    void compileParameterList(){
        if(peek()==")") {
            return;
        }
        string type = eat();
        string name = eat();
        st.define(name,type,"arg");
        while(peek()==","){ 
            eat(); 
            string t=eat(); 
            string n=eat(); 
            st.define(n,t,"arg"); 
        }
    }

    void compileVarDec(){
        eat(); // var
        string type = eat();
        string name = eat();
        st.define(name,type,"var");
        while(peek()==","){ 
            eat(); 
            string n=eat(); 
            st.define(n,type,"var"); 
        }
        eat(); // ;
    }

    void compileStatements(){
        while(true){
            string p = peek();
            if(p=="let") {
                compileLet();
            }
            else if(p=="if") {
                compileIf();
            }
            else if(p=="while") {
                compileWhile();
            }
            else if(p=="do") {
                compileDo();
            }
            else if(p=="return") {
                compileReturn();
            }
            else break;
        }
    }

    void compileDo(){
        eat(); // do
        compileSubroutineCall();
        vm.writePop("temp", 0);
        eat(); // ;
    }

    void compileLet(){
        eat(); // let
        string varName = eat();
        bool isArray = false;
        if(peek()=="["){
            isArray = true;
            eat();
            compileExpression();
            eat();
            const Symbol* s = st.get(varName);
            vm.writePush(vm.getSegment(s->kind), s->index);
            vm.writeArithmetic("add");
        }
        eat(); // =
        compileExpression();
        eat(); // ;
        if(isArray){
            vm.writePop("temp",0);
            vm.writePop("pointer",1);
            vm.writePush("temp",0);
            vm.writePop("that",0);
        } else {
            const Symbol* s = st.get(varName);
            vm.writePop(vm.getSegment(s->kind), s->index);
        }
    }

    void compileWhile(){
        eat(); // while
        string L1 = "WHILE_EXP" + to_string(labelCount);
        string L2 = "WHILE_END" + to_string(labelCount++);
        vm.writeLabel(L1);
        eat(); // (
        compileExpression();
        eat(); // )
        vm.writeArithmetic("not");
        vm.writeIf(L2);
        eat(); // {
        compileStatements();
        eat(); // }
        vm.writeGoto(L1);
        vm.writeLabel(L2);
    }

    void compileReturn(){
        eat(); // return
        if(peek()==";"){
            vm.writePush("constant",0);
            vm.writeReturn();
            eat(); // ;
            return;
        }
        compileExpression();
        vm.writeReturn();
        eat(); // ;
    }

    void compileIf(){
        eat(); // if
        eat(); // (
        compileExpression();
        eat(); // )
        string Ltrue = "IF_TRUE" + to_string(labelCount);
        string Lfalse = "IF_FALSE" + to_string(labelCount);
        string Lend = "IF_END" + to_string(labelCount++);
        vm.writeIf(Ltrue);
        vm.writeGoto(Lfalse);
        vm.writeLabel(Ltrue);
        eat(); // {
        compileStatements();
        eat(); // }
        if(peek()=="else"){
            vm.writeGoto(Lend);
            vm.writeLabel(Lfalse);
            eat(); // else
            eat(); // {
            compileStatements();
            eat(); // }
            vm.writeLabel(Lend);
        } 
        else {
            vm.writeLabel(Lfalse);
        }
    }

    void compileExpression(){
        compileTerm();
        while(true){
            string op = peek();
            if(op=="+"||op=="-"||op=="*"||op=="/"||op=="&"||op=="|"||op=="<"||op==">"||op=="="){
                eat();
                compileTerm();
                if(op=="+") {
                    vm.writeArithmetic("add");
                }
                else if(op=="-") {
                    vm.writeArithmetic("sub");
                }
                else if(op=="&") {
                    vm.writeArithmetic("and");
                }
                else if(op=="|") {
                    vm.writeArithmetic("or");
                }
                else if(op=="<") {
                    vm.writeArithmetic("lt");
                }
                else if(op==">") {
                    vm.writeArithmetic("gt");
                }
                else if(op=="=") {
                    vm.writeArithmetic("eq");
                }
                else if(op=="*") {
                    vm.writeCall("Math.multiply",2);
                }
                else if(op=="/") {
                    vm.writeCall("Math.divide",2);
                }
            } 
            else {
                break;
            }
        }
    }

    void compileTerm(){
        string t = peek();
        if(t.empty()) {
            return;
        }
        if(t[0]=='\"'){
            string s = eat();
            string content = s.size()>=2 ? s.substr(1, s.size()-2) : string();
            vm.writePush("constant", (int)content.size());
            vm.writeCall("String.new",1);
            for(char ch: content){
                vm.writePush("constant", (int)ch);
                vm.writeCall("String.appendChar",2);
            }
            return;
        }
        if(isdigit(t[0])){
            int val = stoi(eat());
            vm.writePush("constant", val);
            return;
        }
        if(t=="true"){ 
            eat(); 
            vm.writePush("constant",0); 
            vm.writeArithmetic("not"); 
            return; 
        }
        if(t=="false"||t=="null"){ 
            eat(); 
            vm.writePush("constant",0);
            return;
        }
        if(t=="this"){ 
            eat(); 
            vm.writePush("pointer",0); 
            return; 
        }
        if(t=="("){ 
            eat(); 
            compileExpression(); 
            eat(); 
            return; 
        }
        if(t=="-"||t=="~"){ 
            string op=eat(); 
            compileTerm(); 
            if(op=="-") {
                vm.writeArithmetic("neg");
            } 
            else {
                vm.writeArithmetic("not");
            }
            return; 
        }
        if(isalpha(t[0]) || t[0]=='_'){
            string name = eat();
            if(peek()=="["){
                eat(); compileExpression(); eat();
                const Symbol* s = st.get(name);
                vm.writePush(vm.getSegment(s->kind), s->index);
                vm.writeArithmetic("add");
                vm.writePop("pointer",1);
                vm.writePush("that",0);
                return;
            } 
            else if(peek()=="(" || peek()=="."){
                rollback();
                compileSubroutineCall();
                return;
            } 
            else {
                const Symbol* s = st.get(name);
                vm.writePush(vm.getSegment(s->kind), s->index);
                return;
            }
        }
    }

    int compileExpressionList(){
        int n=0;
        if(peek()==")") {
            return 0;
        }
        compileExpression();
        n++;
        while(peek()==","){ 
            eat(); 
            compileExpression(); 
            n++; 
        }
        return n;
    }

    void compileSubroutineCall(){
        string name = eat();
        string fullName;
        int nArgs = 0;
        if(peek()=="("){
            eat(); // (
            vm.writePush("pointer",0);
            nArgs += 1;
            nArgs += compileExpressionList();
            eat(); // )
            fullName = className + "." + name;
            vm.writeCall(fullName, nArgs);
        } 
        else if(peek()=="."){
            eat(); // .
            string sub = eat();
            eat(); // (
            if(st.exists(name)) {
                const Symbol* s = st.get(name);
                vm.writePush(vm.getSegment(s->kind), s->index);
                int more = compileExpressionList();
                nArgs = 1 + more;
                fullName = s->type + "." + sub;
                eat(); // )
                vm.writeCall(fullName, nArgs);
            } 
            else {
                int more = compileExpressionList();
                nArgs = more;
                fullName = name + "." + sub;
                eat(); // )
                vm.writeCall(fullName, nArgs);
            }
        }
    }

    void run(){ 
        compileClass(); 
    }
};

string readFile(const string &path) {
    ifstream fin(path);
    if(!fin.is_open()) {
        return "";
    }
    ostringstream ss; 
    ss<<fin.rdbuf(); 
    fin.close(); 
    return ss.str();
}

vector<string> listXmlFiles(const string &path){
    vector<string> res;
    struct stat s;
    if(stat(path.c_str(), &s)==0 && (s.st_mode & S_IFDIR)) {
        DIR *dir = opendir(path.c_str());
        if(dir){
            struct dirent *ent;
            while((ent = readdir(dir))!=nullptr) {
                string name = ent->d_name;
                if(name.size()>=4 && name.substr(name.size()-4)==".xml") {
                    string full = path;
                    if(full.back()!='/') {
                        full.push_back('/');
                    }
                    full += name;
                    res.push_back(full);
                }
            }
            closedir(dir);
        }
    } 
    else {
        if(path.size()>=4 && path.substr(path.size()-4)==".xml") {
            res.push_back(path);
        }
        else {
            ifstream fin(path);
            if(fin.is_open()){ 
                fin.close(); 
                res.push_back(path); 
            }
        }
    }
    return res;
}

string replace_extension_with_vm(const string &path){
    size_t pos = path.find_last_of("/\\");
    string filename = (pos==string::npos)? path : path.substr(pos+1);
    size_t dot = filename.find_last_of('.');
    if(dot==string::npos) {
        return path + ".vm";
    }
    string basepath = (pos==string::npos)? string("") : path.substr(0,pos+1);
    string basename = filename.substr(0,dot);
    return basepath + basename + ".vm";
}

int main(int argc,char**argv){
    vector<string> inputs;
    for(int i=1;i<argc;i++) {
        vector<string> v = listXmlFiles(argv[i]);
        for(auto &x:v) {
            inputs.push_back(x);
        }
    }
    for(auto &f: inputs){
        string xml = readFile(f);
        if(xml.empty()) {
            continue;
        }
        XMLTokenizer xt(xml);
        TokenStream ts(xt.tokens);
        CompilationEngine ce(ts);
        ce.run();
        string out = replace_extension_with_vm(f);
        ofstream fout(out);
        if(fout.is_open()) {
            ce.vm.dump(fout);
            fout.close();
        }
    }
    return 0;
}
