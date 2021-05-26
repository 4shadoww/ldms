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

#include <iostream>
#include <vector>
#include <unistd.h>
#include <string.h>
#include <sensors/sensors.h>

#include "modules/lm_sensors.hpp"
#include "globals.hpp"
#include "config_loader.hpp"
#include "logging.hpp"

std::vector<std::pair<int, const sensors_chip_name*>> chips;
std::vector<std::pair<int, const sensors_feature*>> features;
std::vector<std::pair<int, const sensors_subfeature*>> subfeatures;

int auto_config(){
     int i = 0;
    // Iterate chips
    const sensors_chip_name* chip_name = sensors_get_detected_chips(NULL, &i);
    while(chip_name != NULL){
        chips.push_back(std::pair<int, const sensors_chip_name*>(i, chip_name));
        chip_name = sensors_get_detected_chips(NULL, &i);
    }
    // Iterate features
    for(std::vector<std::pair<int, const sensors_chip_name*>>::iterator it = chips.begin(); it != chips.end(); it++){
        i = 0;
        const sensors_feature* feature = sensors_get_features(it->second, &i);
        while(feature != NULL){
            if(feature->type == 0x02){
                features.push_back(std::pair<int, const sensors_feature*>(it->first, feature));
            }

            feature = sensors_get_features(it->second, &i);
        }
    }
    // Iterate subfeatures
    for(std::vector<std::pair<int, const sensors_chip_name*>>::iterator it0 = chips.begin(); it0 != chips.end(); it0++){
        for(std::vector<std::pair<int, const sensors_feature*>>::iterator it1 = features.begin(); it1 != features.end(); it1++){
            if(it0->first != it1->first) continue;
            i = 0;
            const sensors_subfeature* subfeature = sensors_get_subfeature(it0->second, it1->second, (sensors_subfeature_type)512);

            while(subfeature != NULL){
                if(subfeature->type == 512) subfeatures.push_back(std::pair<int, const sensors_subfeature*>(it0->first, subfeature));

                subfeature = sensors_get_all_subfeatures(it0->second, it1->second, &i);
            }
        }
    }

    // Drop out subfeatures with 0 return value or which return error
    for(std::vector<std::pair<int, const sensors_chip_name*>>::iterator it0 = chips.begin(); it0 != chips.end(); it0++){
        for(std::vector<std::pair<int, const sensors_subfeature*>>::iterator it1 = subfeatures.begin(); it1 != subfeatures.end(); ){
            if(it0->first != it1->first){
                it1++;
                continue;
            }
            double value;
            int status = sensors_get_value(it0->second, it1->second->number, &value);
            if(status != 0){
                csyslog(LOG_ERR, ("error: returned non-zero " + std::to_string(status)));
                it1 = subfeatures.erase(it1);
                continue;
            }else if(value <= 0 || value >= 120){
                csyslog(LOG_ERR, ("got unreliable reading (" + std::to_string(value) + ")"));
                it1 = subfeatures.erase(it1);
                continue;
            }

            it1++;
        }
    }
    return 0;
}

bool already_loaded(std::vector<std::string>& list, std::string key){
    for(std::vector<std::string>::iterator it = list.begin(); it != list.end(); it++){
        if(*it == key) return true;
    }
    return false;
}

int manual_config(){
    int i = 0;
    bool found = false;
    std::vector<std::string> loaded_chips;

    // Iterate chips
    for(std::vector<std::pair<std::string, std::string>>::iterator it = config.sensors.begin(); it != config.sensors.end(); it++){
        // Skip if already loaded
        if(already_loaded(loaded_chips, it->first)) continue;

        found = false;

        const sensors_chip_name* chip_name = sensors_get_detected_chips(NULL, &i);
        while(chip_name != NULL){
            if(strcmp(it->first.c_str(), chip_name->prefix) == 0){
                chips.push_back(std::pair<int, const sensors_chip_name*>(i, chip_name));
                loaded_chips.push_back(it->first);
                found = true;
                break;
            }

            chip_name = sensors_get_detected_chips(NULL, &i);
        }
        if(!found){
            csyslog(LOG_ERR, "error: adapter with prefix: \"" + it->first + "\" not found");
            return 1;
        }
    }

    // Iterate features
    for(std::vector<std::pair<std::string, std::string>>::iterator it0 = config.sensors.begin(); it0 != config.sensors.end(); it0++){
        found = false;
        for(std::vector<std::pair<int, const sensors_chip_name*>>::iterator it1 = chips.begin(); it1 != chips.end(); it1++){
            if(strcmp(it0->first.c_str(), it1->second->prefix) != 0) continue;
            i = 0;
            const sensors_feature* feature = sensors_get_features(it1->second, &i);
            while(feature != NULL){
                if(strcmp(it0->second.c_str(), feature->name) == 0){
                    features.push_back(std::pair<int, const sensors_feature*>(it1->first, feature));
                    found = true;
                    break;
                }
                feature = sensors_get_features(it1->second, &i);
            }
        }
        if(!found){
            csyslog(LOG_ERR, ("error: feature with name: \"" + it0->first + "-" + it0->second + "\" not found"));
            return 1;
        }
    }

    // Iterate subfeatures
    for(std::vector<std::pair<int, const sensors_chip_name*>>::iterator it0 = chips.begin(); it0 != chips.end(); it0++){
        for(std::vector<std::pair<int, const sensors_feature*>>::iterator it1 = features.begin(); it1 != features.end(); it1++){
            if(it0->first != it1->first) continue;
            i = 0;
            const sensors_subfeature* subfeature = sensors_get_subfeature(it0->second, it1->second, (sensors_subfeature_type)512);

            while(subfeature != NULL){
                if(subfeature->type == 512) subfeatures.push_back(std::pair<int, const sensors_subfeature*>(it0->first, subfeature));

                subfeature = sensors_get_all_subfeatures(it0->second, it1->second, &i);
            }
        }
    }

    return 0;
}

int init_sensors(){
    int status = sensors_init(NULL);

    if(status != 0){
        csyslog(LOG_ERR, ("libsensors returned non-zero value " + std::to_string(status)));
        return 1;
    }

    if(config.sensors_auto_configure){
        if(auto_config() != 0){
            csyslog(LOG_ERR, "error: failed to auto configure lm-sensors");
            return 1;
        }
    }else{
        if(manual_config() != 0){
            csyslog(LOG_ERR,  "error: failed to manually configure lm-sensors");
            return 1;
        }
    }

    return 0;
}

int run_lm_sensors(){
    int update_interval = config.sensors_update_interval * 1000;
    int status;
    double value;

    if(init_sensors() != 0){
        thread_crashed();
        return 1;
    }

    while(true){
        for(std::vector<std::pair<int, const sensors_chip_name*>>::iterator it0 = chips.begin(); it0 != chips.end(); it0++){
            for(std::vector<std::pair<int, const sensors_subfeature*>>::iterator it1 = subfeatures.begin(); it1 != subfeatures.end(); it1++){
                if(it0->first != it1->first) continue;
                status = sensors_get_value(it0->second, it1->second->number, &value);
                if(status != 0){
                    csyslog(LOG_ERR, ("error: failed to read value from device " + std::string(it1->second->name)));
                    continue;
                }

                if(value <= config.temp_low){
                    std::unique_lock<std::mutex> lg(mu);
                    triggered = true;
                    cond.notify_all();
                }
            }
        }
        usleep(update_interval);
    }

    return 0;
}
