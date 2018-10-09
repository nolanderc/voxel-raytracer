//
// Created by christofer on 2018-06-17.
//

#include "Log.h"


LoggingLevel Log::reportingLevel = LoggingLevel ::WARNING;


std::string toString(LoggingLevel level) {
    switch (level) {
        case INFO: return "INFO";
        case DEBUG: return "DEBUG";
        case WARNING: return "WARNING";
        case ERROR: return "ERROR";
        default: return "[UNKNOWN]";
    }
}


Log::Log() {}





std::ostringstream &Log::get(LoggingLevel level) {
    this->os << std::endl << toString(level) << ": \t";

    this->messageLevel = level;

    return this->os;
}

void Log::setReportingLevel(LoggingLevel level) {
    Log::reportingLevel = level;
}

Log::~Log() {
    if (messageLevel >= Log::reportingLevel) {
        if (messageLevel >= LoggingLevel::WARNING) {
            std::clog << this->os.str();
        } else {
            std::cout << this->os.str();
        }
    }
}
