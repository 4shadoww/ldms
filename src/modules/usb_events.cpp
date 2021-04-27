/*
  Copyright (C) 2021 Noa-Emil Nissinen

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

#include <sys/epoll.h>
#include <linux/netlink.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "modules/usb_events.hpp"
#include "config_loader.hpp"

#define SOCK_BUFFER_SIZE 8192

struct device_event{
    char* action = nullptr;
    char* device = nullptr;

    ~device_event(){
        if(this->action != nullptr) delete[] action;
        if(this->device != nullptr) delete[] device;
    }
};

device_event* parse_uevent(char* msg){
    device_event* d_event = new device_event;
    int action_end = -1;
    int end = -1;

    // Parse message
    for(unsigned int i = 0; i < SOCK_BUFFER_SIZE; i++){
        if(msg[i] == '\0'){
            end = i;
            break;
        }else if(msg[i] == '@'){
            action_end = i;
            continue;
        }
    }

    // Allocate memory and copy parsed data to struct
    if(action_end > 0 && end > 0){
        d_event->action = new char[action_end + 1];
        d_event->device = new char[end - action_end];
        strncpy(d_event->action, msg, action_end);
        d_event->action[action_end] = '\0';
        strncpy(d_event->device, &msg[action_end + 1], end - action_end - 1);
        d_event->device[end - action_end - 1] = '\0';
    }

    return d_event;
}

bool trigger_dms(const char* action){
    if(strcmp(action, "add") == 0 && config.action_add) return true;
    else if(strcmp(action, "bind") == 0 && config.action_bind) return true;
    else if(strcmp(action, "remove") == 0 && config.action_remove) return true;
    else if(strcmp(action, "change") == 0 && config.action_change) return true;
    else if(strcmp(action, "unbind") == 0 && config.action_unbind) return true;

    return false;
}

int run_usb_events(std::mutex& mu, std::condition_variable& cond){
    epoll_event ep_kernel;
    int fd_ep = -1;
    int nl_socket = -1;
    struct sockaddr_nl src_addr;
    char buffer[SOCK_BUFFER_SIZE];
    int ret;
    device_event* d_event;

    // Lock
    std::unique_lock<std::mutex> lock(mu);

    // Prepare source address
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.nl_family = AF_NETLINK;
    src_addr.nl_pid = getpid();
    src_addr.nl_groups = -1;

    nl_socket = socket(AF_NETLINK, (SOCK_DGRAM | SOCK_CLOEXEC), NETLINK_KOBJECT_UEVENT);

    if(nl_socket < 0){
        std::cerr << "error: failed to open socket" << std::endl;
        return 1;
    }

    ret = bind(nl_socket, (struct sockaddr*) &src_addr, sizeof(src_addr));
    if(ret){
        std::cerr << "error: failed to bind netlink socket" << std::endl;
        close(nl_socket);
        return 1;
    }

    fd_ep = epoll_create1(EPOLL_CLOEXEC);
    if(fd_ep < 0){
        std::cerr << "error: epoll_create1() failed" << std::endl;
        return 1;
    }

    ep_kernel.events = EPOLLIN;
    ep_kernel.data.fd = nl_socket;

    if (epoll_ctl(fd_ep, EPOLL_CTL_ADD, nl_socket, &ep_kernel) < 0) {
        std::cerr << "error: failed to add socket to epoll" << std::endl;
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
            int read_status = read(ev[i].data.fd, buffer, SOCK_BUFFER_SIZE);
            if(read_status < 0){
                std::cerr << "error: failed to read socket ("  << std::endl;
                continue;
            }
            d_event = parse_uevent(buffer);

            if(d_event->device == nullptr){
                delete d_event;
                continue;
            }

            // Check are the requirements met
            if(trigger_dms(d_event->action)){
                std::cout << "hello there" << std::endl;
                triggered = true;
                cond.notify_all();
            }

            delete d_event;
        }
    }

    return 0;
}
