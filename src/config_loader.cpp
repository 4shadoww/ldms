/*
  Copyright (C) 2020-2021 Noa-Emil Nissinen

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

#include <fstream>
#include <algorithm>
#include <sstream>

#include "config_loader.hpp"
#include "logging.hpp"

#ifdef LDMS_DAEMON

#include "modules/usb_events.hpp"
#include "modules/network.hpp"
#include "modules/checkin.hpp"

#ifndef NO_LMSENSORS
#include "modules/lm_sensors.hpp"
#endif

#endif

struct Option{
    std::string name;
    bool (*cmd_ptr)(std::vector<std::string>&, std::string&, int);
};

#ifdef LDMS_DAEMON

struct Module{
    std::string name;
    int (*func_ptr)();
};

#endif

ldms_config config;

Option options[] = {
{"command", &option_command},
{"disarm_after", &option_disarm_after},
{"lock_path", &option_lock_path},
{"modules", &option_modules},
{"logging", &option_logging},
{"ue_triggers", &option_ue_triggers},
{"ue_whitelist", &option_ue_whitelist},
{"sensors", &option_sensors},
{"temp_low", &option_temp_low},
{"sensors_update_interval", &option_sensors_update_interval},
{"disallow_new_interfaces", &option_disallow_new_interfaces},
{"network_interfaces", &option_network_interfaces}
};

#ifdef LDMS_DAEMON

Module modules[] = {
{"usbevents", &run_usb_events},
{"network", &run_network},
{"checkin", &run_checkin},
#ifndef NO_LMSENSORS
{"lm-sensors", &run_lm_sensors}
#endif
};

#endif

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
            csyslog(LOG_ERR, ("error: unknown trigger \"" + *it + "\""));
            return false;
        }
    }

    return true;
}

bool parse_whitelist(std::vector<std::string>& list){
    // No fancy parsing currently
    config.ue_whitelist = list;
    return true;
}

bool parse_sensors(std::vector<std::string>& sensors){
    std::vector<std::string> temp;
    for(std::vector<std::string>::iterator it = sensors.begin(); it != sensors.end(); it++){
        temp = split_string(*it, '-');
        if(temp.size() != 2){
            csyslog(LOG_ERR, ("error: invalid syntax on sensor \"" + *it + "\""));

            return false;
        }
        config.sensors.push_back(std::pair<std::string, std::string>(temp.at(0), temp.at(1)));
    }

    return true;
}

bool parse_bool(std::string str, bool* val){
    if(str == "true"){
        *val = true;
        return true;
    }else if(str == "false"){
        *val = false;
        return true;
    }

    return false;
}

#ifdef LDMS_DAEMON

bool load_modules(std::vector<std::string>& modules_list){
    const unsigned int modules_size = (sizeof(modules) / sizeof(Module));

    for(std::vector<std::string>::iterator it = modules_list.begin(); it != modules_list.end(); it++){
        bool found = false;

        for(unsigned int i = 0; i < modules_size; i++){
            if(*it == modules[i].name){
                found = true;
                config.modules.push_back(std::pair<std::string, func_ptr>(modules[i].name, modules[i].func_ptr));
            }
        }

        if(found) continue;

        csyslog(LOG_ERR, ("error: unknown module \"" + *it + "\""));
        return false;
    }

    return true;
}

#endif

void error_on_line(int line_number, std::string line){
    csyslog(LOG_ERR, ("error on line " + std::to_string(line_number) + ": " + line));
}

bool option_command(std::vector<std::string>& values, std::string& line, int line_number){
    config.command = values.at(1);
    return true;
}

bool option_disarm_after(std::vector<std::string>& values, std::string& line, int line_number){
    if(!parse_bool(values.at(1), &config.disarm_after)){
        error_on_line(line_number, line);
        csyslog(LOG_ERR, "error: logging value");
        return false;
    }

    return true;
}

bool option_lock_path(std::vector<std::string>& values, std::string& line, int line_number){
     config.lock_path = values.at(1);
    return true;
}

bool option_modules(std::vector<std::string>& values, std::string& line, int line_number){
#ifdef LDMS_DAEMON
    std::vector<std::string> temp = split_string(values.at(1), ' ');
    if(!load_modules(temp)){
        error_on_line(line_number, line);
        return false;
    }
#endif

    return true;
}

bool option_logging(std::vector<std::string>& values, std::string& line, int line_number){
    if(!parse_bool(values.at(1), &config.disallow_new_interfaces)){
        error_on_line(line_number, line);
        csyslog(LOG_ERR, "error: logging value");
        return false;
    }

    return true;
}

bool option_ue_triggers(std::vector<std::string>& values, std::string& line, int line_number){
    // Do more parsing
    std::vector<std::string> temp = split_string(values.at(1), ' ');
    if(!parse_triggers(temp)){
        error_on_line(line_number, line);
        return false;
    }

    return true;
}

bool option_ue_whitelist(std::vector<std::string>& values, std::string& line, int line_number){
    // Do more parsing
    std::vector<std::string> temp = split_string(values.at(1), ' ');
    config.ue_whitelist_enabled = true;
    if(!parse_whitelist(temp)){
        error_on_line(line_number, line);
        return false;
    }

    return true;
}

bool option_sensors(std::vector<std::string>& values, std::string& line, int line_number){
    if(values.at(1) == "auto"){
        config.sensors_auto_configure = true;
        return true;
    }
    config.sensors_auto_configure = false;
    std::vector<std::string> temp = split_string(values.at(1), ' ');
    if(!parse_sensors(temp)){
        error_on_line(line_number, line);
        return false;
    }

    return true;
}

bool option_temp_low(std::vector<std::string>& values, std::string& line, int line_number){
    try{
        config.temp_low = std::stod(values.at(1));
    }catch(const std::invalid_argument& ia){
        error_on_line(line_number, line);
        csyslog(LOG_ERR, "error: invalid low_temp value");
        return false;
    }

    return true;
}

bool option_sensors_update_interval(std::vector<std::string>& values, std::string& line, int line_number){
    try{
        config.sensors_update_interval = std::stoi(values.at(1));
    }catch(const std::invalid_argument& ia){
        error_on_line(line_number, line);
        csyslog(LOG_ERR, "error: invalid sensors_update_interval value");
        return false;
    }

    return true;
}

bool option_disallow_new_interfaces(std::vector<std::string>& values, std::string& line, int line_number){
    if(!parse_bool(values.at(1), &config.disallow_new_interfaces)){
        error_on_line(line_number, line);
        csyslog(LOG_ERR, "error: disallow_new_interfaces value");
        return false;
    }

    return true;
}

bool option_network_interfaces(std::vector<std::string>& values, std::string& line, int line_number){
    std::vector<std::string> temp = split_string(values.at(1), ' ');
    if(temp.size() < 1){
        error_on_line(line_number, line);
        return false;
    }
    config.network_interfaces = temp;

    return true;
}

bool load_config(std::string& location){
    std::ifstream reader;
    std::string line;
    std::string key;
    std::vector<std::string> values;
    std::vector<std::string> temp;
    int line_number = 1;

    reader.open(location);
    if(!reader.is_open()){
        csyslog(LOG_ERR, ("error: config file \"" + location + "\" not found"));
        return false;
    }

    // Parse config
    while(getline(reader, line)){
        trim(line);
        // Skip empty and comments
        if(line == "" || line.empty() || line[0] == '#'){
            line_number++;
            continue;
        }

        values = split_string(line, '=');

        if(values.size() == 1 && values.at(0).empty()){
            line_number++;
            continue;
        }else if(values.size() > 2){
            error_on_line(line_number, line);
            csyslog(LOG_ERR, "config parsing error: multiple \"=\" in line");
            return false;
        }else if(values.size() < 2){
            error_on_line(line_number, line);
            csyslog(LOG_ERR, "config parsing error: no value defined to variable");
            return false;
        }

        bool found = false;
        const unsigned int options_size = (sizeof(options) / sizeof(Option));
        for(unsigned int i = 0; i < options_size; i++){
            if(values.at(0) == options[i].name){
                found = true;
                if(!options[i].cmd_ptr(values, line, line_number)){
                    return false;
                }
                break;
            }
        }

        if(found){
            line_number++;
            continue;
        }

        error_on_line(line_number, line);
        csyslog(LOG_ERR, ("error: invalid option \"" + values.at(0) + "\""));
        return false;

    }

    reader.close();

    return true;
}
