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

#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <cstdio>
#include <string>
#include <memory>
#include <functional>

/**
 * Logger class with shared global file
 *
 * There are different logging levels and they're ordered as follows:
 * ERROR < WARN < INFO < DEBUG. If when you log the level you're logging at is
 * less than or equal to either the local log level or the global log level it
 * will get printed to the console and to the file if there is one. Otherwise
 * the statement will be skipped.
 *
 * Because of the nature of this logger it is not recommended to permanently
 * print high frequency updates, such as position updates from the PPT, even in
 * the debug level. If however you are debugging it would be fine to
 * temporarily print like that if needed. However the HADDIDON would probably
 * suit your needs.
 *
 * Usage: include this header, then create a file-level global instance 
 * with a name for your class. For example: "static Logger logger("my class")"
 * Then in any method of that class call the appropriate logging method with
 * your message.
 */
class Logger {
public:
    enum LogLevel { LVL_ERROR, LVL_WARN, LVL_INFO, LVL_DEBUG };

    /**
     * Construct a new instance with a "class name" to use
     *
     * @param name the name to identify this instance
     * @param localLoglevel the local maximum logging level
	 * @param forceFlush force the log file to flush every time this logger
	 *        instance prints.
     */
    Logger(const char *name, LogLevel localLogLevel = LVL_ERROR,
		   bool forceFlush = false)
        : name(name), localLevel(localLogLevel), forceFlush(forceFlush) {}
    ~Logger() {}

    /**
     * Log to stdout and the log file if set at the specified log level
     *
     * prints in the format: YYYY-MM-DD HH:MM:SS LEVEL [name] - Message
     * @param level the level to log at
     * @param message the message to log
     */
    void log(LogLevel level, const char *message);

    /**
     * Equivalent to log(LVL_ERROR, message)
     * @param message the message to log
     */
    inline void error(const std::string &message) {
        log(LVL_ERROR, message.c_str());
    }

    /**
     * Equivalent to log(LVL_WARN, message)
     * @param message the message to log
     */
    inline void warn(const std::string &message) {
        log(LVL_WARN, message.c_str());
    }

    /**
     * Equivalent to log(LVL_INFO, message)
     * @param message the message to log
     */
    inline void info(const std::string &message) {
        log(LVL_INFO, message.c_str());
    }

    /**
     * Equivalent to log(LVL_DEBUG, message)
     * @param message the message to log
     */
    inline void debug(const std::string &message) {
        log(LVL_DEBUG, message.c_str());
    }

    /**
     * Determines whether or not the logger will log at a given level or not
     *
     * @param level the level to check
     * @return whether or not the logger will log
     */
    bool willLog(LogLevel level) const;

    /**
     * Returns the local log level
     * @return the local log level
     */
    LogLevel getLocalLogLevel() const;

    /**
     * Sets maximum local log level to output
     * ERROR < WARN < INFO < DEBUG
     *
     * @param level the cutoff level
     */
    void setLocalLogLevel(LogLevel level);

	/**
	 * Returns whether or not this instance forces flushing the log file
	 * @returns whether we force flushing or not
	 */
	bool getForceFlush() const;

	/**
	 * Sets whether or not this instance forces flushing the log file
	 *
	 * This is a slight performance hit, but if your class is infrequently
	 * logging important information this may be what you need.
	 *
	 * @param forceFlush whether or not we should force flushing
	 */
	void setForceFlush(bool forceFlush);

    /**
     * Sets the file to log to
     *
     * if nullptr do not log to a file
     * @param file file to log to
     */
    static void setLogFile(const char *file);

    /**
     * Returns the global log level
     * @returns the global log level
     */
    static LogLevel getGlobalLogLevel();

    /**
     * Sets maximum global log level to output
     * ERROR < WARN < INFO < DEBUG
     *
     * @param level the cutoff level
     */
    static void setGlobalLogLevel(LogLevel level);

    /**
     * Takes a string and converts it to a log level enum value.
     *
     * @param levelstr the string to parse
     * @return the parsed value, or -1 on error;
     */
    static LogLevel parseLogLevel(const char *levelstr);

    /**
     * Takes a LogLevel and converts it to a string
     *
     * @param level level to convert
     * @return a constant string representing the log level
     */
    static const char *getLevelName(LogLevel level);

private:
    //!The format sent to printf for each log message.
	//!When it is sent there are 4 strings in the following order:
    //!time, level, class name, message
    static const char *printFormat;

    //!The format for the time string in the log messages.
    //!It is sent to strftime, so the string should be what strftime expects
    //!If you change this you'll also have to change the MAX_TIME_STR_LEN
    //!define in Logger.cpp
	static const char *timeFormat;

	//!A list of the names for each log level in order.
    //!Used for converting enums to strings
    static const char *levelStr[4];

    //!The global log level shared by all instances
    static LogLevel globalLevel;

    //!The global log file which all instances log to
    static std::unique_ptr<FILE, std::function<void(FILE*)>> logFile;

    //!The name of this logger instance which gets printed in each message
    const char *name;
    //!The log level for this instance which may override the global level
    LogLevel localLevel;
	//!If true this instance flushes the file every time it prints
	bool forceFlush;
};

#endif