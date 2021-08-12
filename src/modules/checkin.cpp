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
#include <chrono>
#include <sstream>
#include <iomanip>

#include "modules/checkin.hpp"
#include "config_loader.hpp"
#include "logging.hpp"
#include "globals.hpp"

std::string hash;
std::string hash_str;
std::string salt;

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

bool read_hash(std::string& location){
    std::fstream hash_file(location);

    if(!hash_file.is_open()){
        csyslog(LOG_ERR, "checkin module could not open hash file");
        return false;
    }

    std::string line;
    int line_num = 0;
    while(getline(hash_file, line)){
        switch(line_num){
            case 0:
                salt = line;
                break;
            case 1:
                hash_str = line;
                break;
            default:
                csyslog(LOG_ERR, "hash file is invalid");
                return false;
        }
        line_num++;
    }

    hash_file.close();

    return true;
}

bool clear_hash(std::string& location){
    std::fstream hash_file(location, std::fstream::trunc | std::fstream::out);

    if(!hash_file.is_open()){
        csyslog(LOG_ERR, "checkin module could not open hash file");
        return false;
    }

    hash_file << salt << "\n" << std::string(hash_str.length(), '0');

    hash_file.close();

    chmod(location.c_str(), S_IRUSR|S_IWUSR);

    return true;
}

bool write_timestamp(std::string& location){
    std::ofstream timestamp_file(location);
    std::stringstream ss;

    if(!timestamp_file.is_open()){
        csyslog(LOG_ERR, "could not open timestamp_file \"" + location + "\"");
        return false;
    }

    auto time_now = std::chrono::system_clock::now();
    auto next_check = time_now + std::chrono::hours(config.checkin_interval_hours) + std::chrono::minutes(config.checkin_interval_minutes);
    std::time_t time_now_t = std::chrono::system_clock::to_time_t(time_now);
    std::time_t next_check_t = std::chrono::system_clock::to_time_t(next_check);

    ss << std::put_time(std::localtime(&time_now_t), "%Y-%m-%d %H:%M:%S") << "\n" << std::put_time(std::localtime(&next_check_t), "%Y-%m-%d %H:%M:%S");

    timestamp_file << ss.str();

    timestamp_file.close();

    return true;

}

bool got_valid_hash(){
    bool found_char = false;

    for(unsigned int i = 0; i < hash.length(); i++){
        if(hash[i] != ' ' && hash[i] != '0' && hash[i] != '\n'){
            found_char = true;
            break;
        }
    }

    return found_char;
}

void wait_until_valid_hash(std::string& location){
    while(true){
        usleep(5e6);
        if(!read_hash(location)){
            csyslog(LOG_ERR, "checkin module couldn't read hash or clear it");
            thread_crashed();
        }
        hash = hash_str;
        if(got_valid_hash()){
            csyslog(LOG_INFO, "valid hash found");
            break;
        }
        csyslog(LOG_INFO, "hash \"" + hash + "\" doesn't seem to be valid");
    }
}

int run_checkin(){
    // Initialisation
    if(!pwdhash_exists(config.pwdhash_path)){
        csyslog(LOG_INFO, "pwdhash not found waiting until created");
        wait_until_exists(config.pwdhash_path);
    }

    if(!read_hash(config.pwdhash_path)){
        csyslog(LOG_ERR, "checkin module couldn't read hash");
        thread_crashed();
        return 1;
    }

    hash = hash_str;

    if(!got_valid_hash()){
        csyslog(LOG_ERR, "hash appears to be invalid.\nwaiting for valid hash...");
        // TODO Do this properly
        wait_until_valid_hash(config.pwdhash_path);
    }

    if(!clear_hash(config.pwdhash_path)){
        csyslog(LOG_ERR, "checkin module could not clear hash");
        thread_crashed();
        return 1;
    }

    write_timestamp(config.checkin_timestamp_path);

    // Busy wait for next check
    while(true){
        usleep(config.checkin_interval_hours * 36e8 + config.checkin_interval_minutes * 6e7);
        csyslog(LOG_INFO, "checking pwdhash...");
        if(!read_hash(config.pwdhash_path) || !clear_hash(config.pwdhash_path) || hash != hash_str){
            trigger_switch("checkin");
        }else{
            csyslog(LOG_INFO, "user is authorised");
        }
        write_timestamp(config.checkin_timestamp_path);
    }

    return 0;
}
