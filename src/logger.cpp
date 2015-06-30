/**
 *  The MIT License (MIT)
 *
 * Copyright (c) 2015 Andrew Kallmeyer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <type_traits>
#include <ctime>
#include <cstring>

#include "logger.h"

const char *Logger::consolePrintFormat = "\r%s %s [%s]\t- %s\n";
const char *Logger::filePrintFormat = "%s %s [%s]\t- %s\n";
const char *Logger::timeFormat = "%Y-%m-%d %H:%M:%S";
static const int MAX_TIME_STR_LEN = 21;

//must be in the same order as the enum
const char *Logger::levelStr[] = { "ERROR", "WARNING", "INFO", "DEBUG" };
Logger::LogLevel Logger::globalLevel = LVL_INFO;

//The lambda here is the deleter for the unique_ptr
//it is a destructor for the FILE pointer basically
decltype(Logger::logFile) Logger::logFile(nullptr, [](FILE *ptr) {
    fprintf(ptr,
        "\n========================================"
        "========================================\n\n");
    fclose(ptr);
});

static Logger logger("Logger");

void Logger::log(LogLevel level, const char *message) {
    if (willLog(level)) {
        //get time string
        time_t currentTime;
        time(&currentTime);
        char timeStr[MAX_TIME_STR_LEN];
        strftime(timeStr, MAX_TIME_STR_LEN - 1,
                 Logger::timeFormat, localtime(&currentTime));

        //print to console
        printf(Logger::consolePrintFormat, timeStr, Logger::getLevelName(level), name,
               message);

        if (Logger::logFile.get()) {
            //print to the file
            fprintf(Logger::logFile.get(), Logger::filePrintFormat, timeStr,
                    Logger::getLevelName(level), name, message);
			
            //flush if we're at either warn or error
            if (forceFlush || level <= LVL_WARN) {
                fflush(Logger::logFile.get());
            }
        }
    }
}

void Logger::setLogFile(const char *file) {
    if (strcmp(file, "NULL") != 0) {
        Logger::logFile.reset(fopen(file, "a"));

        if (!Logger::logFile.get()) {
            logger.warn("Could not open log file"); //goes to the console
        }
    }else{ //if the file name was null just don't have a file
        Logger::logFile.reset(nullptr);
    }
}

bool Logger::getForceFlush() const {
	return forceFlush;
}

void Logger::setForceFlush(bool forceFlush) {
	this->forceFlush = forceFlush;
}

bool Logger::willLog(LogLevel level) const {
    return level <= globalLevel || level <= localLevel;
}

Logger::LogLevel Logger::getGlobalLogLevel() {
    return Logger::globalLevel;
}

Logger::LogLevel Logger::getLocalLogLevel() const {
    return this->localLevel;
}

void Logger::setGlobalLogLevel(LogLevel level) {
    Logger::globalLevel = level;
}

void Logger::setLocalLogLevel(LogLevel level) {
    this->localLevel = level;
}

const char *Logger::getLevelName(LogLevel level) {
    if (level >= 0 && level < (int)std::extent<decltype(levelStr)>()) {
        return levelStr[level];
    }else{
        return "INVALID";
    }
}

Logger::LogLevel Logger::parseLogLevel(const char *level) {
    for (unsigned int i = 0; i < std::extent<decltype(levelStr)>(); ++i) {
        if (strcmp(level, levelStr[i]) == 0) {
            return (LogLevel) i;
        }
    }

    return (LogLevel) -1;
}