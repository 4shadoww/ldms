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
#include <string>
#include <string.h>
#include <thread>
#include <sys/stat.h>

#include "config_loader.hpp"
#include "globals.hpp"

inline bool switch_armed(std::string& location){
    struct stat buffer;
    return (stat(location.c_str(), &buffer) == 0);
}

bool initialise_modules(){
    //std::thread* temp;
    for(std::vector<std::pair<std::string, func_ptr>>::iterator it = config.modules.begin(); it != config.modules.end(); it++){
        new std::thread(*(it->second));
    }

    return true;
}

int listen_for_events(){
    while(true){
        std::unique_lock<std::mutex> lock(mu);
        cond.wait(lock, []{return triggered;});
        // Check is switch armed
        if(switch_armed(config.lock_path)){
            // Run the command on shell
            std::system(config.command.c_str());
            break;
        }
        triggered = false;
    }

    return 0;
}

void print_version(){
    std::cout << "ldms 1.0" << std::endl;
}

void print_usage(char* arg0){
    std::cout << "Usage: " << arg0 << " [options]\n\nOptions:\n-h, --help\t\tshow help\n-v, --version\t\tshow version\n-c, --config\t\tconfig file location" << std::endl;
}

int main(int argc, char** argv){
    int return_status = 0;
    std::string config_location = "/etc/ldms/ldms.conf";

    // Parse arguments
    if(argc >= 2){
        if(strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0){
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
    }

    // Load config
    if(!load_config(config_location)){
        std::cerr << "failed to config" << std::endl;
        return 1;
    }

    if(config.modules.size() == 0){
        std::cerr << "no modules defined" << std::endl;
        return 1;
    }

    initialise_modules();

    // Main loop
    return_status = listen_for_events();

    return return_status;
}
