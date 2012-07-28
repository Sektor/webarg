/*  session.h
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

#ifndef SESSION_H
#define SESSION_H

#include <glib.h>

int session_counter_get();
int session_counter_inc();
void session_counter_reset();
void resp_queue_free_index(GArray *resp_queue, int n);
void resp_queue_remove_index(GArray *resp_queue, int n);
void resp_queue_clear(GArray *resp_queue);

#endif // SESSION_H
