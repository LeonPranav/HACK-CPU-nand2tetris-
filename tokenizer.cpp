#include <bits/stdc++.h>
using namespace std;

const unordered_set<string> KEYWORDS={"class","constructor","function","method","field","static","var","int","char","boolean","void","true","false","null","this","let","do","if","else","while","return"};

const unordered_set<char> SYMBOLS={'{','}','(',')','[',']','.',',',';','+','-','*','/','&','|','<','>','=','~'};

string xmlEscape(const string &s) {
    string out;
    for (char c : s) {
        switch (c) {
            case '&': 
                out += "&amp;"; 
                break;
            case '<': 
                out += "&lt;"; 
                break;
            case '>': 
                out += "&gt;"; 
                break;
            case '\"': 
                out += "&quot;"; 
                break;
            default: 
                out.push_back(c);
        }
    }
    return out;
}

string readFileAll(ifstream &ifs) {
    ostringstream ss;
    ss << ifs.rdbuf();
    return ss.str();
}

string stripComments(const string &src) {
    string out;
    size_t i=0, n=src.size();
    while (i<n) {
        if (i+1<n && src[i]=='/' && src[i+1]=='/') {
            i+=2;
            while (i<n && src[i]!='\n') ++i;
        }
        else if (i+1<n && src[i]=='/' && src[i+1]=='*') {
            i+=2;
            while (i+1<n && !(src[i]=='*' && src[i+1]=='/')) {
                i++;
            }
            if (i+1<n) {
                i+=2;
            }
        } 
        else {
            out.push_back(src[i]);
            i++;
        }
    }
    return out;
}

int main(int argc, char **argv) {
    string inPath=argv[1];
    string outPath;
    if (argc>=3) {
        outPath = argv[2];
    }
    else {
        size_t pos = inPath.rfind('.');
        if (pos != string::npos) {
            outPath = inPath.substr(0, pos) + ".xml";
        }
        else {
            outPath = inPath + ".xml";
        }
    }
    ifstream ifs(inPath);
    ofstream ofs(outPath);
    string code = stripComments(readFileAll(ifs));
    size_t i=0,n=code.size();

    ofs << "<tokens>\n";
    while (true) {
        while (i<n && isspace((char)code[i])) {
            i++;
        }
        if (i >= n) break;
        char c = code[i];
        if (c == '"') {
            i++;
            string s;
            while (i<n && code[i]!='"') {
                s.push_back(code[i]);
                i++;
            }
            if (i<n && code[i]=='"') {
                i++;
            }
            ofs << "<stringConstant> " << xmlEscape(s) << " </stringConstant>\n";
            continue;
        }

        if (SYMBOLS.count(c)) {
            string symbolStr(1,c);
            string esc = xmlEscape(symbolStr);
            ofs << "<symbol> " << esc << " </symbol>\n";
            i++;
            continue;
        }

        if (isdigit((char)c)) {
            string num;
            while (i<n && isdigit((char)code[i])) {
                num.push_back(code[i]);
                i++;
            }
            ofs << "<integerConstant> " << num << " </integerConstant>\n";
            continue;
        }

        if (isalpha((char)c) || c == '_') {
            string id;
            while (i<n && (isalnum((char)code[i]) || code[i]=='_')) {
                id.push_back(code[i]);
                i++;
            }
            if (KEYWORDS.count(id)) {
                ofs << "<keyword> " << id << " </keyword>\n";
            } else {
                ofs << "<identifier> " << id << " </identifier>\n";
            }
            continue;
        }
        string stray(1,c);
        ofs << "<symbol> " << xmlEscape(stray) << " </symbol>\n";
        i++;
    }
    ofs << "</tokens>\n";
    ofs.close();
    ifs.close();
    // cout << "Tokenization complete. Wrote: " << outPath << "\n";
    return 0;
}
