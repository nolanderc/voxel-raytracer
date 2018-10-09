//
// Created by christofer on 2018-06-17.
//

#pragma once

#include <CL/cl.h>


#include <stdexcept>
#include <vector>
#include <iostream>


#include "Log.h"



std::string clErrorToString(cl_int error);
void checkCLError(cl_int error);

