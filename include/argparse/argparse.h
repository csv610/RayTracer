#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <functional>
#include <stdexcept>

namespace argparse {

class ArgumentParser {
public:
    ArgumentParser(const std::string& prog_name, const std::string& description = "")
        : prog(prog_name), description(description) {}

    struct Argument {
        std::string help;
        std::string default_val;
        std::string value;
        bool set = false;
        bool is_positional = false;
        std::string short_name;
        std::string long_name;
    };

    void add_argument(const std::string& name_or_flags, const std::string& help = "",
                      const std::string& default_val = "") {
        std::string name = name_or_flags;
        std::string short_flag, long_flag;

        if (name_or_flags.substr(0, 2) == "--") {
            long_flag = name_or_flags.substr(2);
            name = long_flag;
        } else if (name_or_flags.substr(0, 1) == "-") {
            short_flag = name_or_flags.substr(1);
            name = short_flag;
        }

        Argument arg;
        arg.help = help;
        arg.default_val = default_val;
        arg.long_name = long_flag;
        arg.short_name = short_flag;
        
        arguments[name] = arg;
        
        if (!short_flag.empty()) {
            arguments[short_flag] = arg;
            arguments[short_flag].long_name = name;
        }
    }

    void add_positional(const std::string& name, const std::string& help = "") {
        positional.push_back(name);
        add_argument(name, help);
    }

    void parse(int argc, char* argv[]) {
        prog = argv[0];
        size_t pos_idx = 0;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];

            if (arg == "-h" || arg == "--help") {
                print_help();
                exit(0);
            }

            bool handled = false;

            if (arg.substr(0, 2) == "--") {
                std::string key = arg.substr(2);
                size_t eq_pos = key.find('=');
                std::string value;
                std::string name;

                if (eq_pos != std::string::npos) {
                    value = key.substr(eq_pos + 1);
                    name = key.substr(0, eq_pos);
                } else if (i + 1 < argc && argv[i + 1][0] != '-') {
                    value = argv[++i];
                    name = key;
                } else {
                    value = "true";
                    name = key;
                }

                if (arguments.find(name) != arguments.end()) {
                    arguments[name].value = value;
                    arguments[name].set = true;
                    handled = true;
                }
            } else if (arg.substr(0, 1) == "-" && arg.length() > 1) {
                std::string flag = arg.substr(1);
                if (arguments.find(flag) != arguments.end()) {
                    std::string value;
                    if (i + 1 < argc && argv[i + 1][0] != '-') {
                        value = argv[++i];
                    } else {
                        value = "true";
                    }
                    arguments[flag].value = value;
                    arguments[flag].set = true;
                    handled = true;
                }
            }

            if (!handled && pos_idx < positional.size()) {
                std::string pos_name = positional[pos_idx];
                arguments[pos_name].value = arg;
                arguments[pos_name].set = true;
                pos_idx++;
            }
        }

        for (auto& [name, arg] : arguments) {
            if (!arg.set && !arg.default_val.empty()) {
                arg.value = arg.default_val;
                arg.set = true;
            }
        }
    }

    std::string get(const std::string& name) const {
        auto it = arguments.find(name);
        if (it != arguments.end()) {
            return it->second.value;
        }
        return "";
    }

    int get_int(const std::string& name, int default_val = 0) const {
        std::string val = get(name);
        if (val.empty()) return default_val;
        try {
            return std::stoi(val);
        } catch (...) {
            return default_val;
        }
    }

    float get_float(const std::string& name, float default_val = 0.0f) const {
        std::string val = get(name);
        if (val.empty()) return default_val;
        try {
            return std::stof(val);
        } catch (...) {
            return default_val;
        }
    }

    bool exists(const std::string& name) const {
        auto it = arguments.find(name);
        return it != arguments.end() && it->second.set;
    }

    void print_help() const {
        std::cout << "Usage: " << prog << " [options] ";
        for (const auto& p : positional) {
            std::cout << p << " ";
        }
        std::cout << std::endl;
        std::cout << std::endl;
        
        if (!description.empty()) {
            std::cout << description << std::endl;
            std::cout << std::endl;
        }
        
        std::cout << "Options:" << std::endl;
        
        std::vector<std::string> printed;
        for (const auto& [name, arg] : arguments) {
            bool already_printed = false;
            for (const auto& p : printed) {
                if (p == name || (arg.long_name != "" && p == arg.long_name)) {
                    already_printed = true;
                    break;
                }
            }
            if (already_printed) continue;
            printed.push_back(name);
            
            if (arg.short_name.empty() && arg.long_name.empty()) {
                continue;
            }
            
            std::cout << "  ";
            if (!arg.short_name.empty()) {
                std::cout << "-" << arg.short_name;
            }
            if (!arg.short_name.empty() && !arg.long_name.empty()) {
                std::cout << ", --" << arg.long_name;
            } else if (!arg.long_name.empty()) {
                std::cout << "--" << arg.long_name;
            }
            
            if (!arg.default_val.empty()) {
                std::cout << " (default: " << arg.default_val << ")";
            }
            std::cout << std::endl;
            
            if (!arg.help.empty()) {
                std::cout << "      " << arg.help << std::endl;
            }
        }
    }

private:
    std::string prog;
    std::string description;
    std::vector<std::string> positional;
    std::map<std::string, Argument> arguments;
};

}

#endif