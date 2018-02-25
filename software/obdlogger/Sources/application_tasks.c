/*
Open OBD2 datalogger
Copyright (C) 2018 Artur Langner

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include "application_tasks.h"

StackType_t STACK_ACQUISITION_TASK[STACK_BYTES_TO_WORDS(STACK_SIZE_ACQUISITION_TASK)];
StackType_t STACK_STORAGE_TASK[STACK_BYTES_TO_WORDS(STACK_SIZE_STORAGE_TASK)];

StaticTask_t acquisition_task_handle;
StaticTask_t storage_task_handle;
