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

#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

#include "modules/checkin.hpp"
#include "config_loader.hpp"
#include "logging.hpp"
#include "globals.hpp"

std::string hash;

inline bool pwdhash_exists(std::string& location){
    struct stat buffer;
    return (stat(location.c_str(), &buffer) == 0);
}

void wait_until_exists(std::string& location){
    // Busy wait until file is created
    while(true){
        usleep(5e6);
        if(pwdhash_exists(location)) break;
    }

    csyslog(LOG_INFO, "hash found from \"" + location + "\"");

}

bool read_clear_hashfile(std::string& location, bool init){
    std::fstream hash_file(location);
    std::string hash_str;
    std::string salt;

    if(!hash_file.is_open()){
        csyslog(LOG_ERR, "checkin module could not open hash file");
        return false;
    }

    std::string line;
    int line_num = 0;
    bool got_hash = false;
    while(getline(hash_file, line)){
        switch(line_num){
            case 0:
                salt = line;
                break;
            case 1:
                hash_str = line;
                got_hash = false;
                break;
            default:
                csyslog(LOG_ERR, "hash file is invalid");
                return false;
        }
    }

    if(init){

    }


    chmod(location.c_str(), S_IRUSR|S_IWUSR);

    return true;
}

int run_checkin(){

    if(!pwdhash_exists(config.pwdhash_path)){
        csyslog(LOG_INFO, "pwdhash not found waiting until created");
        wait_until_exists(config.pwdhash_path);
    }

    if(!read_clear_hashfile(config.pwdhash_path, false)){
        csyslog(LOG_ERR, "checkin module couldn't read hash or clear it");
        thread_crashed();
        return 1;
    }

    return 0;
}
