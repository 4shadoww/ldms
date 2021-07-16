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

#include <crypt.h>
#include <unistd.h>

#include "modules/checkin.hpp"

char* generate_salt(size_t size){

    if(size < 10){
        return nullptr;
    }

    const char *const saltchars =
        "./0123456789ABCDEFGHIJKLMNOPQRST"
        "UVWXYZabcdefghijklmnopqrstuvwxyz";

    unsigned char* ubytes = new unsigned char[size];
    char* salt = new char[size + 4];

    if(getentropy(ubytes, size)){
        delete[] ubytes;
        delete[] salt;
        return nullptr;
    }

    salt[0] = '$';
    salt[1] = '5'; /* SHA-256 */
    salt[2] = '$';

    int i = 0;

    for(i = 0; i < size; i++){
        salt[3+i] = saltchars[ubytes[i] & 0x3f];
    }
    salt[3+i] = '\0';

    delete[] ubytes;

    return salt;
}

int run_checkin(){


    return 0;
}
