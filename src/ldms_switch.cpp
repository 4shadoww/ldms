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

#include <iostream>
#include <string.h>

#include "config_loader.hpp"

void print_config(){

}

bool disarm(){

    return true;
}

bool arm(){

    return true;
}

void print_version(){
    std::cout << "ldmswitch 1.0" << std::endl;
}

void print_usage(char* argv0){
    std::cout << "Usage: " << argv0 << " parameter1\n\nOptions:\n-h, --help\t\tshow help\n-v, --version\t\tshow version\narm\t\t\tarm dead man's switch\ndisarm\t\t\tdisarm dead man's switch" << std::endl;
}

int main(int argc, char** argv){
    std::string config_location = "/etc/ldms/ldms.conf";

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
        }
        config_location = std::string(argv[2]);
    }

    // Load config
    if(!load_config(config_location)){
        std::cerr << "failed to config" << std::endl;
        return 1;
    }

    // Parse action
    if(strcmp(argv[1], "arm")){
        if(!arm()){
            std::cerr << "error: could not arm ldms" << std::endl;
            return 1;
        }
        std::cout << "ldms is now armed and command \"" << config.command << "\" will be ran upon triggering!!!" << std::endl;

    }else if(strcmp(argv[1], "disarm")){
        if(!disarm()){
            std::cerr << "error: could not disarm ldms" << std::endl;
            return 1;
        }
        std::cout << "ldms is now disarmed!!!" << std::endl;
    }

    return 0;
}
