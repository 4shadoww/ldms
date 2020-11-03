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
#include <sys/epoll.h>
#include <sys/stat.h>
#include <fstream>
#include <vector>
#include <algorithm>
#include <sstream>

#include <libudev.h>

struct ldms_config{
    std::string command = "shutdown now";
    std::string lock_path = "/var/lib/ldms/armed.lck";
    bool action_add  = true;
    bool action_bind = true;
    bool action_remove = true;
    bool action_change = false;
    bool action_unbing = false;

} config;

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

static inline void trim(std::string &s) {
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

bool load_config(std::string& location){
    std::ifstream reader;
    std::string line;
    std::string key;
    std::vector<std::string> values;
    std::vector<std::string> triggers;

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
        // TODO: Could be done in a loop
        if(values.at(0) == "command"){
            config.command = values.at(1);
            continue;
        }else if(values.at(0) == "lock_path"){
            config.lock_path = values.at(1);
            continue;
        }else if(values.at(0) == "triggers"){
            // Do more parsing
            triggers = split_string(values.at(1), ' ');
            for(std::vector<std::string>::iterator it = triggers.begin(); it != triggers.end(); it++){
                std::cout << *it << std::endl;
            }
        }else{
            std::cerr << "error: invalid option \"" << values.at(0) << "\"" << std::endl;
            return false;
        }
    }

    reader.close();

    return true;
}

inline bool switch_armed(std::string& location){
    struct stat buffer;
    return (stat(location.c_str(), &buffer) == 0);
}


int listen_for_events(){
    udev* context = udev_new();
    udev_monitor* kernel_monitor = nullptr;
    udev_device* device;
    epoll_event ep_kernel;
    int fd_ep = -1;
    int fd_kernel = -1;

    // Set up monitor and get kernel socket to monitor
    if(context == nullptr){
        std::cerr << "error: couldn't create udev context" << std::endl;
    }
    kernel_monitor = udev_monitor_new_from_netlink(context, "kernel");
    if(kernel_monitor == nullptr){
        std::cerr << "error: couldn't create monitor" << std::endl;
    }
    udev_monitor_set_receive_buffer_size(kernel_monitor, 128*1024*1024);

    udev_monitor_enable_receiving(kernel_monitor);

    fd_ep = epoll_create1(EPOLL_CLOEXEC);
    if(fd_ep < 0){
        std::cerr << "error: epoll_create1() failed" << std::endl;
        return 1;
    }

    fd_kernel = udev_monitor_get_fd(kernel_monitor);

    ep_kernel.events = EPOLLIN;
    ep_kernel.data.fd = fd_kernel;

    if (epoll_ctl(fd_ep, EPOLL_CTL_ADD, fd_kernel, &ep_kernel) < 0) {
        std::cerr << "error: failed to add fd to epoll" << std::endl;
        return 5;
    }

    // Main loop
    while(true){
        struct epoll_event ev[4];
        int fdcount = epoll_wait(fd_ep, ev, 4, -1);

        if (fdcount < 0) {
            if (errno != EINTR)
                std::cerr << "error while receiving uevent message" << std::endl;
            continue;
        }
        // Iterate events
        for(int i = 0; i < fdcount; i++){
            device = udev_monitor_receive_device(kernel_monitor);

            if(device == nullptr) continue;

            // Check is switch armed and are the requirements met
            if(switch_armed(config.lock_path)){
                std::cout << udev_device_get_action(device) << std::endl;
            }

            udev_device_unref(device);
        }
    }

    return 0;
}

void print_version(){
    std::cout << "ldms 1.0" << std::endl;
}

void print_usage(){
    std::cout << "Usage: [options]\n\nOptions:\n-h, --help\t\tshow help\n-v, --version\t\tshow version\n-c, --config\t\tconfig file location" << std::endl;
}

int main(int argc, char** argv){
    int return_status = 0;
    std::string config_location = "/etc/ldms/ldms.conf";

    // Parse arguments
    if(argc >= 2){
        if(strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0){
            print_usage();
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

    return 0;
    // Main loop
    return_status = listen_for_events();

    return return_status;
}
