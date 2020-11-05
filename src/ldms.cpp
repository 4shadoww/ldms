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

#include <libudev.h>

#include "config_loader.hpp"


bool trigger_dms(const char* action){
    if(strcmp(action, "add") == 0 && config.action_add) return true;
    else if(strcmp(action, "bind") == 0 && config.action_bind) return true;
    else if(strcmp(action, "remove") == 0 && config.action_remove) return true;
    else if(strcmp(action, "change") == 0 && config.action_change) return true;
    else if(strcmp(action, "unbind") == 0 && config.action_unbind) return true;

    return false;
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
            if(switch_armed(config.lock_path) && trigger_dms(udev_device_get_action(device))){
                // Run the command on shell
                std::system(config.command.c_str());
            }

            udev_device_unref(device);
        }
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

    // Main loop
    return_status = listen_for_events();

    return return_status;
}
