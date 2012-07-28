/*  logging.h
 *
 *  Logging related functions.
 *
 *  (c) 2010-2012 Anton Olkhovik <ant007h@gmail.com>
 *  (c) 2009 Peter Tworek <tworaz666@gmail.com>
 *
 *  This file is part of WebArg.
 *
 *  WebArg is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  WebArg is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with WebArg.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LOGGING_H
#define LOGGING_H

// Supported message types.
typedef enum {
    MSG,
    INFO,
    WARNING,
    ERROR,
    DBG
} MESSAGE_TYPE;

// Supported logging levels.
typedef enum {
    LOG_NORMAL,
    LOG_VERBOSE
} LOGGING_LEVEL;

// Main logging routine. Should not be used directly.
// Please use log_<type> macros.
void log_message(MESSAGE_TYPE type, LOGGING_LEVEL level, char *fmt, ...);

#ifdef LOG_MODULE
#define LOG_PREFIX LOG_MODULE ": "
#else
#define LOG_PREFIX
#endif

// Logging macros.
#define log(...)         log_message(MSG, LOG_NORMAL, LOG_PREFIX __VA_ARGS__)
#define log_info(...)    log_message(INFO, LOG_NORMAL, LOG_PREFIX __VA_ARGS__)
#define log_warning(...) log_message(WARNING, LOG_NORMAL, LOG_PREFIX __VA_ARGS__)
#define log_error(...)   log_message(ERROR, LOG_NORMAL, LOG_PREFIX __VA_ARGS__)
// Verbose mode.
#define log_verb(...)      log_message(MSG, LOG_VERBOSE, LOG_PREFIX __VA_ARGS__)
#define log_info_verb(...) log_message(INFO, LOG_VERBOSE, LOG_PREFIX __VA_ARGS__)
#ifdef DEBUG
#define log_debug(...)   log_message(DBG, LOG_NORMAL, LOG_PREFIX __VA_ARGS__)
#else
#define log_debug(...)
#endif // DEBUG

void log_level_set(LOGGING_LEVEL level);
LOGGING_LEVEL log_level_get();

#endif // LOGGING_H
