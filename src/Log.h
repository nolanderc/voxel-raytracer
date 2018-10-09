//
// Created by christofer on 2018-06-17.
//

#pragma once
#include <sstream>
#include <iostream>


enum LoggingLevel {
    INFO,
    DEBUG,
    WARNING,
    ERROR,
};


class Log {
    /// The logging output
    std::ostringstream os;

    /// The loggging level of the message
    LoggingLevel messageLevel;

protected:

    /// The level we are printing at
    static LoggingLevel reportingLevel;

public:
    Log();
    virtual ~Log();

    std::ostringstream& get(LoggingLevel level);

    static void setReportingLevel(LoggingLevel level);
};



