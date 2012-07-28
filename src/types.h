/*  types.h
 *
 *  (c) 2012 Anton Olkhovik <ant007h@gmail.com>
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

#ifndef TYPES_H
#define TYPES_H

#include <stdbool.h>

#define JSON_PADDING 512

#define ARRAYSIZE(X) (sizeof(X) / sizeof(X[0]))

#define IFNONZERO(cmd, arg) \
{ \
	if (arg) \
		cmd(arg); \
}

#define IFNONZEROEX(cmd, arg, ...) \
{ \
	if (arg) \
		cmd(arg, __VA_ARGS__); \
}

typedef enum
{
    EXEC_RESP_PID,
    EXEC_RESP_STDIN,
    EXEC_RESP_STDOUT,
    EXEC_RESP_STDERR,
    EXEC_RESP_KILL,
    EXEC_RESP_EXIT
}
service_resp_type;

typedef struct
{
    service_resp_type type;
    int value;
    int ticket;
    void *data;
}
service_resp_entry;

/*
 * one of these is auto-created for each connection and a pointer to the
 * appropriate instance is passed to the callback in the user parameter
 */

typedef struct
{
    int session_id;
}
per_session_data__webarg_api;

#endif // TYPES_H
