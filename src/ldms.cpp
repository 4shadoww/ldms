/*
  Copyright (C) 2020-2021 Noa-Emil Nissinen

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
#include <string.h>
#include <cstdio>
#include <fstream>
#include <sys/stat.h>
#include <termios.h>

#include "config_loader.hpp"
#include "version_info.hpp"
#include "hash_utils.hpp"

inline bool file_exists(std::string& location){
    struct stat buffer;
    return (stat(location.c_str(), &buffer) == 0);
}

std::string modules_as_string(){
    std::string f = "";

    for(std::vector<std::pair<std::string, func_ptr>>::iterator it = config.modules.begin(); it != config.modules.end(); it++){
        f += it->first;
        if(it != config.modules.end()) f += " ";
    }
    return f;
}

bool disarm(){
    std::remove(config.lock_path.c_str());
    if(file_exists(config.lock_path)){
        std::cerr << "error: failed to disarm ldms" << std::endl;
        std::cerr << "error: couldn't delete file \"" << config.lock_path <<  "\"" << std::endl;
        return false;
    }

    return true;
}

bool arm(){
    std::ofstream lock(config.lock_path);
    if(!lock.is_open()){
        std::cerr << "error: couldn't create lock file \"" << config.lock_path  << "\" to arm ldms" << std::endl;
        return false;
    }
    lock.close();

    return true;
}

bool read_checkin_timestamp(std::string& location){
    std::ifstream timestamp_file(location);

    if(!timestamp_file.is_open()){
        std::cerr << "could not open timestamp file" << std::endl;
        return false;
    }

    std::string time_now;
    std::string time_next;
    std::string line;
    int line_num = 0;
    while(getline(timestamp_file, line)){
        switch(line_num){
            case 0:
                time_now = line;
                break;
            case 1:
                time_next = line;
                break;
            default:
                std::cerr << "error: timestamp file may be corrupted" << std::endl;
                goto endloop;
        }
        line_num++;
    }
endloop:
    timestamp_file.close();

    std::cout << "last hash check: " << time_now << std::endl;
    std::cout << "next hash check: " << time_next << std::endl;

    return true;
}

int getch(){
    int ch;
    struct termios t_old, t_new;

    tcgetattr(STDIN_FILENO, &t_old);
    t_new = t_old;
    t_new.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &t_new);

    ch = getchar();

    tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
    return ch;
}





std::string getpass(const char *prompt, bool show_asterisk){
    const char BACKSPACE=127;
    const char RETURN=10;

    std::string password;
    unsigned char ch=0;

    std::cout << prompt;

    while((ch=getch()) != RETURN){
        if(ch==BACKSPACE){
            if(password.length() != 0){
                if(show_asterisk) std::cout << "\b \b";
                password.resize(password.length() - 1);
            }
        }else{
             password += ch;
             if(show_asterisk) std::cout <<'*';
        }
    }
    std::cout << std::endl;
    return password;
}


bool auth_hash(std::string& location){
    std::fstream hash_file;
    std::string salt;
    std::string hash;
    bool got_salt = false;
    bool generate_new_salt = true;

    if(file_exists(config.pwdhash_path)){
        hash_file.open(location, std::fstream::in);
        std::string line;

        while(getline(hash_file, line)){
            salt = line;
            break;
        }
        if(salt.length() > 0 && salt != " " && salt != "\n"){
            got_salt = true;
        }

        hash_file.close();

    }

    if(got_salt){
        std::string answer;
        while(true){
            std::cout << "do you want to use existing salt or generate new? [y/N] ";
            getline(std::cin, answer);

            if(answer == ""){
                generate_new_salt = false;
                break;
            }else if(answer == "n" || answer == "N"){
                generate_new_salt = false;
                break;
            }else if(answer == "y" || answer == "Y"){
                generate_new_salt = true;
                break;
            }else{
                std::cout << "please enter y or n" << std::endl;
            }
        }
    }

    if(generate_new_salt){
        salt = generate_salt(16);
    }

    std::string pwd = getpass("password: ", false);

    hash = crypt(salt.c_str(), pwd.c_str());

    hash_file.open(location, std::fstream::trunc | std::fstream::out);

    if(!hash_file.is_open()){
        std::cerr << "could not write to hash file \"" << location << "\"" << std::endl;
        return false;
    }

    hash_file << salt << "\n" << hash;

    hash_file.close();

    chmod(location.c_str(), S_IRUSR|S_IWUSR);

    return true;
}

bool checkin_module(char* command){
    if(strcmp(command, "status") == 0){
        if(!read_checkin_timestamp(config.checkin_timestamp_path)){
            std::cerr << "failed to read timestamp" << std::endl;
            return false;
        }
    }else if(strcmp(command, "auth") == 0){
        if(!auth_hash(config.pwdhash_path)){
            std::cerr << "failed to create hash" << std::endl;
            return false;
        }
        std::cout << "hash updated" << std::endl;
    }else{
        std::cerr << "error: unknown option" << std::endl;
        return false;
    }

    return true;
}

void print_version(){
    std::cout << "ldms " << ldms_version << std::endl;
    std::cout << "ldms daemon " << ldmsd_version << std::endl;
}

void print_usage(char* argv0){
    std::cout << "Usage: " << argv0 << " [options...] parameter1\n\nOptions:\n-h, --help\t\tshow help\n-v, --version\t\tshow version\n-c, --config\t\tconfig file location\narm\t\t\tarm dead man's switch\ndisarm\t\t\tdisarm dead man's switch\nstatus\t\t\tcheck is switch armed\ncheckin [auth|status]\tcheckin module commands" << std::endl;
}

int main(int argc, char** argv){
    std::string config_location = "/etc/ldms/ldmsd.conf";
    int parameter1_pos = 1;

    // Parse arguments
    if(argc < 2){
        print_usage(argv[0]);
        return 1;
    }else if(strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0){
        print_usage(argv[0]);
        return 0;
    }else if(strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0){
        print_version();
        return 0;
    }else if(strcmp(argv[1], "--config") == 0 || strcmp(argv[1], "-c") == 0){
        if(argc < 3){
            std::cerr << "please give the config location" << std::endl;
            return 1;
        }else if(argc < 4){
            print_usage(argv[0]);
            std::cerr << std::endl << "please give the parameter1" << std::endl;
            return 1;
        }
        parameter1_pos = 3;
        config_location = std::string(argv[2]);
    }

    // Load config
    if(!load_config(config_location)){
        std::cerr << "failed to config" << std::endl;
        return 1;
    }

    // Parse action
    if(strcmp(argv[parameter1_pos], "arm") == 0){
        if(!arm()){
            std::cerr << "error: could not arm ldms" << std::endl;
            return 1;
        }
        std::cout << "ldms is now armed and command \"" << config.command << "\" will be ran upon triggering!!!" << std::endl;
        std::cout << "make sure ldms daemon is running!!!" << std::endl;

    }else if(strcmp(argv[parameter1_pos], "disarm") == 0){
        if(!disarm()){
            std::cerr << "error: could not disarm ldms" << std::endl;
            return 1;
        }
        std::cout << "ldms is now disarmed!!!" << std::endl;
    }else if(strcmp(argv[parameter1_pos], "status") == 0){
        std::cout << "check ldms daemon status with service manager" << std::endl;
        if(file_exists(config.lock_path)){
            std::cout << "switch is armed" << std::endl;
            return 0;
        }
        std::cout << "switch is disarmed" << std::endl;
        return 0;
    }else if(strcmp(argv[parameter1_pos], "checkin") == 0){
        if(argc - parameter1_pos < 2){
            std::cerr << "error: too few arguments" << std::endl;
            print_usage(argv[0]);
            return 1;
        }

        checkin_module(argv[parameter1_pos + 1]);

        return 0;
    }else{
        std::cerr << "unknown command \"" << argv[parameter1_pos] << "\"" << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    return 0;
}
