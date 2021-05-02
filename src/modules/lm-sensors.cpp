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
#include <sensors/sensors.h>

#include "modules/lm-sensors.hpp"
#include "globals.hpp"

std::vector<std::pair<int, const sensors_chip_name*>> chips;
std::vector<std::pair<int, const sensors_feature*>> features;
std::vector<std::pair<int, const sensors_subfeature*>> subfeatures;

int init_sensors(){
    int status = sensors_init(NULL);

    if(status != 0){
        std::cerr << "libsensors returned non-zero value " << status << std::endl;
        return 1;
    }

    int i = 0;
    // Iterate chips
    const sensors_chip_name* chip_name = sensors_get_detected_chips(NULL, &i);
    while(chip_name != NULL){
        chips.push_back(std::pair<int, const sensors_chip_name*>(i, chip_name));
        i++;
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

            i++;
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

                i++;
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
                std::cout << "error: returned non-zero " << status << std::endl;
                it1 = subfeatures.erase(it1);
                continue;
            }else if(value <= 0){
                std::cout << "got reading equal or less than zero (" << value << ")" << std::endl;
                it1 = subfeatures.erase(it1);
                continue;
            }

            it1++;
        }
    }

    std::cout << "using " << subfeatures.size() << " readings" << std::endl;

    return 0;
}

int run_lm_sensors(){
    if(!init_sensors()){
        return 1;
    }

    while(true){

    }

    return 0;
}
