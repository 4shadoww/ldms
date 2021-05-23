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
#include <netinet/in.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <string.h>
#include <vector>

#include "modules/network.hpp"
#include "config_loader.hpp"
#include "globals.hpp"

// TODO: Get rid of busy waiting

std::vector<std::pair<std::string, sockaddr>> addresses;

int init_network(){

    return 0;
}

int run_network(){
    int fd;
    int len;
    struct sockaddr_nl sa;
    char buffer[4096];
    struct msghdr msg;
    struct nlmsghdr* nh;

    if((fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1){
        std::cerr << "error: couldn't open NETLINK_ROUTE socket" << std::endl;
        return 1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;

    if(bind(fd, (struct sockaddr*)&sa, sizeof(sa)) == -1){
        std::cerr << "error: couldn't bind" << std::endl;
        return 1;
    }
    nh = (struct nlmsghdr *)buffer;
    while ((len = recv(fd, nh, 4096, 0)) > 0) {
        while ((NLMSG_OK(nh, len)) && (nh->nlmsg_type != NLMSG_DONE)){
            if(nh->nlmsg_type == RTM_NEWLINK || nh->nlmsg_type == RTM_DELLINK){
                struct ifaddrmsg *ifa = (struct ifaddrmsg *) NLMSG_DATA(nh);
                char name[IFNAMSIZ];
                if_indextoname(ifa->ifa_index, name);
                std::cout << name << std::endl;
            }
            nh = NLMSG_NEXT(nh, len);
        }
    }
    return 0;
}
