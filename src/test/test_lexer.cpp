#include <vector>
#include <map>
#include <string>
#include <stdio.h>
#include "include/lexical_analyzer.h"

std::map<int, std::string> _m;

int main(int argc, char *argv[]) {
    _m[259] = "ADD";
    _m[260] = "SUB";
    _m[261] = "MUL";
    _m[262] = "DIV";
    _m[263] = "LT";
    _m[264] = "LTE";
    _m[265] = "GT";
    _m[266] = "GTE";
    _m[267] = "EQ";
    _m[268] = "NEQ";
    _m[269] = "ASSIN";
    _m[293] = "MOD";
    _m[296] = "NOT";
    _m[297] = "AND";
    _m[298] = "OR";
    _m[270] = "SEMICOLON";
    _m[271] = "COMMA";
    _m[272] = "LPARENTHESE";
    _m[273] = "RPARENTHESE";
    _m[274] = "LBRACKET";
    _m[275] = "RBRACKET";
    _m[276] = "LBRACE";
    _m[277] = "RBRACE";
    _m[278] = "ELSE";
    _m[279] = "IF";
    _m[280] = "INT";
    _m[281] = "CONTINUE";
    _m[282] = "RETURN";
    _m[283] = "VOID";
    _m[284] = "WHILE";
    _m[294] = "BREAK";
    _m[295] = "CONST";
    _m[285] = "IDENT";
    _m[286] = "INT";
    _m[287] = "FLOATPOINT";
    _m[288] = "ARRAY";
    _m[289] = "LETTER";
    _m[290] = "EOL";
    _m[291] = "COMMENT";
    _m[292] = "BLANK";
    _m[258] = "ERROR";
    if (argc != 2) {
        printf("Usage: lexer /path/to/.sysy/file\n");
        return 0;
    }
    std::vector<Token_Node> node_list;
    analyzer(argv[1], node_list);
    for(auto node: node_list){
        printf("[L:%d P:%d ~ %d] [%s]: %s\n", node.lines, node.pos_start, node.pos_end, _m[node.token].c_str(), node.text);
    }
}