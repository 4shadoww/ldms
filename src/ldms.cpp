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

#include <iostream>
#include <string.h>
#include <cstdio>
#include <fstream>
#include <sys/stat.h>

#include "config_loader.hpp"

inline bool switch_armed(std::string& location){
    struct stat buffer;
    return (stat(location.c_str(), &buffer) == 0);
}

std::string modules_as_string(){
    std::string f = "";

    for(std::vector<std::pair<std::string, func_ptr>>::iterator it = config.modules.begin(); it != config.modules.end(); it++){
        f += it->first;
        if(it != config.modules.end()) f += " ";
    }
    return f;
}

bool disarm(){
    std::remove(config.lock_path.c_str());
    if(switch_armed(config.lock_path)){
        std::cerr << "error: failed to disarm ldms" << std::endl;
        return false;
    }

    return true;
}

bool arm(){
    std::ofstream lock(config.lock_path);
    if(!lock.is_open()){
        std::cerr << "error: couldn't create lock file to arm ldms" << std::endl;
        return false;
    }
    lock.close();

    return true;
}

void print_version(){
    std::cout << "ldmswitch 1.0" << std::endl;
}

void print_usage(char* argv0){
    std::cout << "Usage: " << argv0 << " [options...] parameter1\n\nOptions:\n-h, --help\t\tshow help\n-v, --version\t\tshow version\n-c, --config\t\tconfig file location\narm\t\t\tarm dead man's switch\ndisarm\t\t\tdisarm dead man's switch\nstatus\t\t\tcheck is switch armed" << std::endl;
}

int main(int argc, char** argv){
    std::string config_location = "/etc/ldms/ldmsd.conf";
    int parameter1_pos = 1;

    // Parse arguments
    if(argc < 2){
        print_usage(argv[0]);
        return 1;
    }else if(strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0){
        print_usage(argv[0]);
        return 0;
    }else if(strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0){
        print_version();
        return 0;
    }else if(strcmp(argv[1], "--config") == 0 || strcmp(argv[1], "-c") == 0){
        if(argc < 3){
            std::cerr << "please give the config location" << std::endl;
            return 1;
        }else if(argc < 4){
            print_usage(argv[0]);
            std::cerr << std::endl << "please give the parameter1" << std::endl;
            return 1;
        }
        parameter1_pos = 3;
        config_location = std::string(argv[2]);
    }

    // Load config
    if(!load_config(config_location)){
        std::cerr << "failed to config" << std::endl;
        return 1;
    }

    // Parse action
    if(strcmp(argv[parameter1_pos], "arm") == 0){
        if(!arm()){
            std::cerr << "error: could not arm ldms" << std::endl;
            return 1;
        }
        std::cout << "ldms is now armed and command \"" << config.command << "\" will be ran upon triggering!!!" << std::endl;
        std::cout << "make sure ldms daemon is running!!!" << std::endl;

    }else if(strcmp(argv[parameter1_pos], "disarm") == 0){
        if(!disarm()){
            std::cerr << "error: could not disarm ldms" << std::endl;
            return 1;
        }
        std::cout << "ldms is now disarmed!!!" << std::endl;
    }else if(strcmp(argv[parameter1_pos], "status") == 0){
        if(switch_armed(config.lock_path)){
            std::cout << "switch is armed" << std::endl;
            return 0;
        }
        std::cout << "switch is disarmed" << std::endl;
        return 0;
    }else{
        std::cerr << "unknown command \"" << argv[parameter1_pos] << "\"" << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
