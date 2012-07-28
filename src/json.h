/*  json.h
 *
 *  (c) 2011-2012 Anton Olkhovik <ant007h@gmail.com>
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

#ifndef JSON_H
#define JSON_H

#include <stdbool.h>
#include <json-glib/json-glib.h>

JsonNode *_json_object_get_member(JsonObject *obj, const char *member_name);
JsonArray *_json_node_get_array(JsonNode *node);
int _json_array_get_length(JsonArray *array);
JsonNode *_json_array_get_element(JsonArray *array, int i);
JsonObject *_json_node_get_object(JsonNode *node);
JsonObject *_json_object_get_member_object(JsonObject *obj, const char *member_name);
bool _json_object_get_member_boolean(JsonObject *obj, const char *member_name);
int _json_object_get_member_int(JsonObject *obj, const char *member_name);
double _json_object_get_member_double(JsonObject *obj, const char *member_name);
char *_json_node_dup_string(JsonNode *member_node);
char *_json_object_dup_member_string(JsonObject *obj, const char *member_name);
JsonObject *json_get_root_object();
bool json_load(const char *str, int len);
void json_init();
void json_final();

#endif // JSON_H
