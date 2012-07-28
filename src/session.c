/*  session.c
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

#include <stdio.h>
#include "session.h"
#include "types.h"

/* session counter */

static int session_counter = 0;

int session_counter_get()
{
	return session_counter;
}

int session_counter_inc()
{
	return ++session_counter;
}

void session_counter_reset()
{
	session_counter = 0;
}

/* response queue */

void resp_queue_free_index(GArray *resp_queue, int n)
{
	service_resp_entry resp_entry = g_array_index(resp_queue, service_resp_entry, n);
	//free resp_entry.data depending of resp_entry.type
	IFNONZERO(free, resp_entry.data);
}

void resp_queue_remove_index(GArray *resp_queue, int n)
{
	resp_queue_free_index(resp_queue, n);
	g_array_remove_index(resp_queue, n);
}

void resp_queue_clear(GArray *resp_queue)
{
	if (!resp_queue || !resp_queue->len)
		return;
	for (int i = 0; i < resp_queue->len; i++)
		resp_queue_free_index(resp_queue, i);
	g_array_remove_range(resp_queue, 0, resp_queue->len);
}	
