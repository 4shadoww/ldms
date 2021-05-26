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
#include "logging.hpp"
#include "config_loader.hpp"

bool syslog_enabled = true;

void open_logging(){
    openlog("ldms", LOG_CONS | LOG_PID | LOG_NDELAY, LOG_LOCAL1);
}

void init_logging(){
    if(!config.logging){
        syslog_enabled = false;
        closelog();
    }
}

void csyslog(int level, std::string str){
    if(level == LOG_ERR) std::cerr << str << std::endl;
    else if(level == LOG_CRIT) std::cerr << str << std::endl;
    else if(level == LOG_ALERT) std::cerr << str << std::endl;
    else if(level == LOG_WARNING) std::cerr << str << std::endl;
    else if(level == LOG_EMERG) std::cerr << str << std::endl;
    else std::cout << str << std::endl;

    if(syslog_enabled) syslog(level, str.c_str());
}
