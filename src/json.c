/*  json.c
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

#include <string.h>
#include "json.h"

#define LOG_MODULE "Json"
#include "logging.h"

static JsonParser *json_parser = NULL;
static JsonNode *json_root_node = NULL;
static JsonObject *json_root_object = NULL;

JsonNode *_json_object_get_member(JsonObject *obj, const char *member_name)
{
    return (obj ? json_object_get_member(obj, member_name) : NULL);
}

JsonArray *_json_node_get_array(JsonNode *node)
{
    if (!node || JSON_NODE_TYPE(node) != JSON_NODE_ARRAY)
        return NULL;
    return json_node_get_array(node);
}

int _json_array_get_length(JsonArray *array)
{
    return (array ? json_array_get_length(array) : 0);
}

JsonNode *_json_array_get_element(JsonArray *array, int i)
{
    return (array ? json_array_get_element(array, i) : NULL);
}

JsonObject *_json_node_get_object(JsonNode *node)
{
    return (node ? json_node_get_object(node) : NULL);
}

#define JSON_GET_MEMBER_NODE JsonNode *member_node = _json_object_get_member(obj, member_name);

JsonObject *_json_object_get_member_object(JsonObject *obj, const char *member_name)
{
    JSON_GET_MEMBER_NODE
    return _json_node_get_object(member_node);
}

bool _json_object_get_member_boolean(JsonObject *obj, const char *member_name)
{
    JSON_GET_MEMBER_NODE
    if (!member_node || json_node_get_value_type(member_node) != G_TYPE_BOOLEAN)
        return false;
    return json_node_get_boolean(member_node);
}

int _json_object_get_member_int(JsonObject *obj, const char *member_name)
{
    JSON_GET_MEMBER_NODE
    if (member_node)
    {
        GType vtype = json_node_get_value_type(member_node);
        if (vtype == G_TYPE_INT || vtype == G_TYPE_INT64)
            return json_node_get_int(member_node);
    }
    return 0;
}

double _json_object_get_member_double(JsonObject *obj, const char *member_name)
{
    JSON_GET_MEMBER_NODE
    if (!member_node || json_node_get_value_type(member_node) != G_TYPE_DOUBLE)
        return 0.0;
    return json_node_get_double(member_node);
}

char *_json_node_dup_string(JsonNode *member_node)
{
    if (!member_node || json_node_get_value_type(member_node) != G_TYPE_STRING)
        return strdup("");
    return json_node_dup_string(member_node);
}

char *_json_object_dup_member_string(JsonObject *obj, const char *member_name)
{
    JSON_GET_MEMBER_NODE
	return _json_node_dup_string(member_node);
}

JsonObject *json_get_root_object()
{
	return json_root_object;
}

bool json_load(const char *str, int len)
{
    GError *error = NULL;
	json_parser_load_from_data(json_parser, str, len, &error);
    if (error)
    {
        log_error("Unable to parse: %s", error->message);
        g_error_free(error);
        return false;
    }
    json_root_node = json_parser_get_root(json_parser);
    json_root_object = _json_node_get_object(json_root_node);
    if (!json_root_object)
        log_error("Unable to get root element");
    return json_root_object;
}

void json_init()
{
	json_parser = json_parser_new();
}

void json_final()
{
	g_object_unref(json_parser);
}
