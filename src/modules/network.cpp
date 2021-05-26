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
#include <ifaddrs.h>
#include <string.h>
#include <vector>

#include "modules/network.hpp"
#include "config_loader.hpp"
#include "globals.hpp"
#include "logging.hpp"

std::vector<std::pair<std::string, bool>> addresses;

bool new_interface(char* name){
    for(std::vector<std::pair<std::string, bool>>::iterator it = addresses.begin(); it != addresses.end(); it++){
        if(strcmp(name, it->first.c_str()) == 0) return false;
    }

    return true;
}

bool illegal_change(char* name){
    for(std::vector<std::pair<std::string, bool>>::iterator it = addresses.begin(); it != addresses.end(); it++){
        if(strcmp(name, it->first.c_str()) == 0 && it->second) return true;
    }

    return false;
}

void trigger_switch(){
    std::unique_lock<std::mutex> lg(mu);
    triggered = true;
    cond.notify_all();
}

int init_network(){
    struct ifaddrs* ifaddr;
    if(getifaddrs(&ifaddr) == -1){
        csyslog(LOG_ERR,  "error: failed to list network interfaces");
        return -1;
    };

    bool on_list = false;

    for(struct ifaddrs* ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL || ifa->ifa_addr->sa_family != AF_PACKET) continue;
        on_list = false;

        for(std::vector<std::string>::iterator it = config.network_interfaces.begin(); it != config.network_interfaces.end(); it++){
            if(*it == ifa->ifa_name){
                on_list = true;
                break;
            }
        }

        addresses.push_back(std::pair<std::string, bool>(ifa->ifa_name, on_list));
    }

    freeifaddrs(ifaddr);

    return 0;
}

int run_network(){
    int fd, len;
    struct sockaddr_nl sa;
    char buffer[4096];
    struct nlmsghdr* nh;

    if((fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE)) == -1){
        csyslog(LOG_ERR, "error: couldn't open NETLINK_ROUTE socket");
        return 1;
    }

    memset(&sa, 0, sizeof(sa));
    sa.nl_family = AF_NETLINK;
    sa.nl_groups = RTMGRP_LINK | RTMGRP_IPV4_IFADDR | RTMGRP_IPV6_IFADDR;

    if(bind(fd, (struct sockaddr*)&sa, sizeof(sa)) == -1){
        csyslog(LOG_ERR, "error: couldn't bind");
        return 1;
    }

    while(true){
        len = recv(fd, buffer, sizeof(buffer), 0);
        if(len == 0){
            csyslog(LOG_ERR, "error: failed to load network event");
            break;
        }else if(len == -1){
            csyslog(LOG_ERR, "error: error occurred while receiving network event");
            break;
        }
        nh = (struct nlmsghdr*)buffer;

        while((NLMSG_OK(nh, len)) && (nh->nlmsg_type != NLMSG_DONE)){
            if(nh->nlmsg_type == RTM_NEWLINK || nh->nlmsg_type == RTM_DELLINK){
                struct ifaddrmsg* ifa = (struct ifaddrmsg*) NLMSG_DATA(nh);
                char name[IFNAMSIZ];
                if_indextoname(ifa->ifa_index, name);

                if(config.disallow_new_interfaces && new_interface(name)) trigger_switch();

                if(illegal_change(name)) trigger_switch();
            }
            nh = NLMSG_NEXT(nh, len);
        }
    }
    return 0;
}
