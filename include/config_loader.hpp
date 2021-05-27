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

#ifndef CONFIG_LOADER_HPP
#define CONFIG_LOADER_HPP

#include <iostream>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>

typedef int (*func_ptr)();

struct ldms_config{
    std::string command = "echo \"dead man's switch triggered\"";
    std::string lock_path = "/var/lib/ldms/armed.lck";
    std::vector<std::pair<std::string, func_ptr>> modules;
    bool logging = true;
    // Usbevents
    bool action_add  = false;
    bool action_bind = false;
    bool action_remove = false;
    bool action_change = false;
    bool action_unbind = false;
    bool usb_events_whitelist_enabled = false;
    std::vector<std::string> usb_events_whitelist;
    // lm-sensors
    double temp_low = 20;
    int sensors_update_interval = 500;
    bool sensors_auto_configure = true;
    std::vector<std::pair<std::string, std::string>> sensors;
    // Network
    std::vector<std::string> network_interfaces;
    bool disallow_new_interfaces = true;
};

extern ldms_config config;

bool load_config(std::string& location);

#endif
