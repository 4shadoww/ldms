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
#include "globals.hpp"
#include "logging.hpp"

bool triggered = false;
bool crashed = false;
std::mutex mu;
std::condition_variable cond;

void thread_crashed(){
    std::unique_lock<std::mutex> lg(mu);
    crashed = true;
    cond.notify_all();
}

void trigger_switch(std::string who){
    csyslog(LOG_INFO, ("triggered by module " + who));
    std::unique_lock<std::mutex> lg(mu);
    triggered = true;
    cond.notify_all();
}
