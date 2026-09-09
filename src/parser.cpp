#include "./headers/parser.h"
#include <string>
#include <any>
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include "./headers/command.h"
#include "./builtins/declarebuiltin.h"

void Parser::parse(const std::string& command, std::vector<std::string>& tokens) {
    std::string cur = "";
    bool iq = false;   // inside single quotes
    bool idq = false;  // inside double quotes
    for (size_t i = 0; i < command.size(); i++) {
        char c = command[i];
        if (c == '\\' && !iq && !idq) {         // backslash outside quotes
            if (i + 1 < command.size()) {
                cur += command[++i];
            }
        } else if (c == '\\' && idq) {          // backslash inside double quotes
            if (i + 1 < command.size()) {
                char next = command[i + 1];
                if (next == '"' || next == '\\') {
                    cur += command[++i];
                } else {
                    cur += c;
                }
            }
        } else if (c == '\"' && !idq && !iq) {
            idq = true;
        } else if (c == '\'' && !iq && !idq) {
            iq = true;
        } else if (c == '\'' && iq) {
            iq = false;
        } else if (c == '\"' && idq) {
            idq = false;
        } else if (c == ' ' && !iq && !idq) {
            if (!cur.empty()) {
                tokens.push_back(cur);
                cur = "";
            }
        } else {
            cur += c;
        }
    }

    if (!cur.empty()) tokens.push_back(cur);
}

void Parser::variables_check(std::vector<std::string>& tokens, DeclareBuiltin& declare_builtin) {
    std::vector<std::string> res;
    std::string cur = "";
    for (const auto& s : tokens) {
        for (size_t i = 0; i < s.size(); i++) {
            char c = s[i];
            if (c == '$') {
                if (i + 1 < s.size() && s[i+1] == '{') {
                    size_t pos = s.find('}', i);
                    if (pos != std::string::npos) {
                        auto result = declare_builtin.get_var(s.substr(i + 2, pos - 2 - i));
                        if (result) {
                            std::string val = std::any_cast<std::string>(*result);
                            res.push_back(cur + val);
                            cur.clear();
                        }
                        i = pos;
                    } else {
                        cur += s.substr(i);
                        i = s.size();
                    }
                } else {
                    auto result = declare_builtin.get_var(s.substr(i + 1));
                    if (result) {
                        std::string val = std::any_cast<std::string>(*result);
                        res.push_back(cur + val);
                        cur.clear();
                    }
                    i = s.size();
                }
            } else {
                cur += c;
            }
        }
        if (!cur.empty()) res.push_back(cur);
        cur.clear();
    }
    tokens = res;
}

std::vector<CommandSegment> Parser::split_commands(const std::string& command) {
    std::vector<CommandSegment> segments;
    std::string cur = "";
    bool iq = false;
    bool idq = false;
    for (size_t i = 0; i < command.size(); i++) {
        char c = command[i];
        if (c == '\\' && !iq && !idq) {
            if (i + 1 < command.size()) {
                cur += c;
            }
        } else if (c == '\\' && idq) {          // backslash inside double quotes
            if (i + 1 < command.size()) {
                char next = command[i + 1];
                if (next == '"' || next == '\\') {
                    cur += next;
                } else {
                    cur += c;
                }
            }  
        } else if (c == '\"' && !idq && !iq) {
            idq = true;
        } else if (c == '\'' && !iq && !idq) {
            iq = true;
        } else if (c == '\'' && iq) {
            iq = false;
        } else if (c == '\"' && idq) {
            idq = false;
        } else if (c == '|' && i + 1 < command.size() && command[i+1] != '|') {
            segments.push_back({cur, "|"});
            cur.clear(); 
            i++;
            continue;
        } else if (c == '&' && i + 1 < command.size() && command[i+1] == '&' && !iq && !idq) {
            segments.push_back({cur, "&&"});
            cur.clear();
            i++;
            continue;
        } else if (c == '|' && i + 1 < command.size() && command[i+1] == '|' && !iq && !idq) {
            segments.push_back({cur, "||"});
            cur.clear();
            i++;
            continue;
        } else if (c == ';' && i + 1 < command.size() && !iq && !idq) {
            segments.push_back({cur, ";"});
            cur.clear();
            i++;
            continue;
        } else {
            cur += c;
        }
    }

    if (!cur.empty()) segments.push_back({cur, ""});
    return segments;
}

std::vector<std::string> Parser::parse_redirections(std::vector<std::string>& clean_tokens, std::vector<std::string>& tokens, std::string& redirect_file, std::string& redirect_stderr, int& FLAG_CONST)
{
    for (size_t i = 0; i < tokens.size(); i++) {
        if ((tokens[i] == ">" || tokens[i] == "1>") && i + 1 < tokens.size()) {
            redirect_file = tokens[i + 1];
            i++; // skip filename token too
        } else if ((tokens[i] == ">>" || tokens[i] == "1>>") && i + 1 < tokens.size()) {
            redirect_file = tokens[i + 1];
            FLAG_CONST = O_APPEND; //change flag for append
            i++; 
        }
        else if (tokens[i] == "2>" && i + 1 <tokens.size()) {
            redirect_stderr = tokens[i + 1];
            i++;
        } else if (tokens[i] == "2>>" && i + 1 <tokens.size()) {
            redirect_stderr = tokens[i + 1];
            FLAG_CONST = O_APPEND; //change flag for append
            i++;
        } else {
            clean_tokens.push_back(tokens[i]);
        }
    }
    return clean_tokens;
}