/*
  Copyright (C) 2020 Noa-Emil Nissinen

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.    If not, see <https://www.gnu.org/licenses/>.
*/

#include "config_loader.hpp"
#include <fstream>
#include <algorithm>
#include <sstream>

#include <modules/usb_events.hpp>

ldms_config config;
bool triggered = false;

static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

static inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}

std::vector<std::string> split_string(std::string str, char delimiter){
    std::vector<std::string> internal;
    std::stringstream ss(str);
    std::string tok;

    while(getline(ss, tok, delimiter)) {
        trim(tok);
        internal.push_back(tok);
    }

    return internal;
}

bool parse_triggers(std::vector<std::string>& triggers){
    for(std::vector<std::string>::iterator it = triggers.begin(); it != triggers.end(); it++){
        if(*it == "add"){
            config.action_add = true;
        }else if(*it == "bind"){
            config.action_bind = true;
        }else if(*it == "remove"){
            config.action_remove = true;
        }else if(*it == "change"){
            config.action_change = true;
        }else if(*it == "unbind"){
            config.action_unbind = true;
        }else{
            std::cerr << "error: unknown trigger \"" << *it << "\"" << std::endl;
            return false;
        }
    }

    return true;
}

bool load_modules(std::vector<std::string>& modules){
    for(std::vector<std::string>::iterator it = modules.begin(); it != modules.end(); it++){
        if(*it == "usbevents"){
            config.modules.push_back(std::pair<std::string, func_ptr>("usbevents", &run_usb_events));
        }else if(*it == "lm-sensors"){
            // Implement
        }else{
            std::cerr << "error: unknown module \"" << *it << "\"" << std::endl;
            return false;
        }
    }


    return true;
}

bool load_config(std::string& location){
    std::ifstream reader;
    std::string line;
    std::string key;
    std::vector<std::string> values;
    std::vector<std::string> triggers;
    std::vector<std::string> modules;

    reader.open(location);
    if(!reader.is_open()){
        std::cerr << "error: config file \"" << location << "\" not found" << std::endl;
        return false;
    }

    // Parse config
    while(getline(reader, line)){
        values = split_string(line, '=');
        if(values.size() == 1 && values.at(0).empty()) continue;
        if(values.size() > 2){
            std::cerr << "config parsing error: multiple \"=\" in line" << std::endl;
            return false;
        }

        if(values.at(0) == "command"){
            config.command = values.at(1);
            continue;
        }else if(values.at(0) == "lock_path"){
            config.lock_path = values.at(1);
            continue;
        }else if(values.at(0) == "triggers"){
            // Do more parsing
            triggers = split_string(values.at(1), ' ');
            if(!parse_triggers(triggers)) return false;
        }else if(values.at(0) == "modules"){
            modules = split_string(values.at(1), ' ');
            if(!load_modules(modules)) return false;
        }else{
            std::cerr << "error: invalid option \"" << values.at(0) << "\"" << std::endl;
            return false;
        }
    }

    reader.close();

    return true;
}
