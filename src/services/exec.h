/*  exec.h
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

#ifndef EXEC_H
#define EXEC_H

#include "../types.h"
#include "../../libwebsockets/libwebsockets.h"

bool service_exec_receive(struct libwebsocket_context *context,
	struct libwebsocket *wsi, per_session_data__webarg_api *pss,
	char *event, char *ext_text, int ext_text_len);
bool service_exec_close(int session_id);
bool service_exec_send(struct libwebsocket *wsi,
	per_session_data__webarg_api *pss);
void service_exec_tick();
void service_exec_init();
void service_exec_final();

#endif // EXEC_H
