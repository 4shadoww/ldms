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
#include <sys/epoll.h>

#include <libudev.h>

bool listen_for_events(){
    udev* context = udev_new();
    udev_monitor* kernel_monitor = nullptr;
    udev_device* device;
    epoll_event ep_kernel;
    int fd_ep = -1;
    int fd_kernel = -1;

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
        std::cerr << "error" << std::endl;
        return 1;
    }

    fd_kernel = udev_monitor_get_fd(kernel_monitor);

    ep_kernel.events = EPOLLIN;
    ep_kernel.data.fd = fd_kernel;

    if (epoll_ctl(fd_ep, EPOLL_CTL_ADD, fd_kernel, &ep_kernel) < 0) {
        std::cerr << "error: failed to add fd to epoll" << std::endl;
        return 5;
    }

    while(true){
        struct epoll_event ev[4];
        int fdcount = epoll_wait(fd_ep, ev, 4, -1);

        if (fdcount < 0) {
            if (errno != EINTR)
                std::cerr << "error while receiving uevent message" << std::endl;
            continue;
        }
        for(int i = 0; i < fdcount; i++){
            device = udev_monitor_receive_device(kernel_monitor);

            if(device == nullptr) continue;

            std::cout << udev_device_get_action(device) << std::endl;

            udev_device_unref(device);
        }
    }

    return true;
}

int main(int argc, char** argv){

    listen_for_events();

    return 0;
}
