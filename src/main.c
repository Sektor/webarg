/*  main.c
 *
 *  (c) 2012 Anton Olkhovik <ant007h@gmail.com>
 *  (C) 2010-2011 Andy Green <andy@warmcat.com>
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
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <sys/time.h>
#include <glib.h>
#include <glib/gprintf.h>
#include "services/exec.h"
#include "types.h"
#include "json.h"
#include "logging.h"
#include "../libwebsockets/libwebsockets.h"

/* defines */

#define ENABLE_FILESYSTEM_HTTP
#define ENABLE_CARET_SPLIT
#define ENABLE_EXEC_SERVICE

#define LOCAL_RESOURCE_PATH "data/"
//#define LOCAL_RESOURCE_PATH DATADIR "/webarg/"

#define PROCESSING_INTERVAL 20 //ms

/* types */

enum protocol_type
{
	/* always first */
	PROTOCOL_HTTP = 0,

	PROTOCOL_WEBARG_API,

	/* always last */
	PROTOCOL_COUNT
};

/* filename processing */

typedef struct
{
	char *ext;
	char *mime;
}
ext_mime;

static bool is_safe_file_name(const char *file_name)
{
	return (file_name && !strstr(file_name, ".."));
}

static const char *get_ext(const char *file_name)
{
	const char *ext = strrchr(file_name, '.');
	if (ext)
		ext++;
	return ext;
}

static const char *get_mime(const char *file_name)
{
	static const char default_mime[] = "text/plain";
	static const ext_mime entries[] = {
		{ .ext = "html", .mime = "text/html" },
		{ .ext = "js",   .mime = "application/x-javascript" },
		{ .ext = "ico",  .mime = "image/x-icon" },
		{ .ext = "jpg",  .mime = "image/jpeg" },
		{ .ext = "png",  .mime = "image/png" },
		{ .ext = "gif",  .mime = "image/gif" },
		{ .ext = "svg",  .mime = "image/svg+xml" },
		{ .ext = "css",  .mime = "text/css" }
	};
	int entries_count = ARRAYSIZE(entries);
	const char *ext = get_ext(file_name);
	if (ext)
	{
		for (int i = 0; i < entries_count; i++)
		{
			const ext_mime *entry = &entries[i];
			if (!strcmp(ext, entry->ext))
				return entry->mime;
		}
	}
	return default_mime;
}

/* this protocol server (always the first one) just knows how to do HTTP */

static int callback_http(struct libwebsocket_context *context,
	struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason,
	void *user, void *in, size_t len)
{
	if (reason == LWS_CALLBACK_HTTP)
	{
		char *req = in;
		log_info_verb("Serving HTTP URI %s", req);
		
		if (is_safe_file_name(req))
		{
			bool allocated = false;
			gchar *file_pathname = 0;
#ifdef ENABLE_FILESYSTEM_HTTP
			const char prefix[] = "/get/";
			if (g_str_has_prefix(req, prefix))
				file_pathname = req + strlen(prefix) - 1;
			else
#endif
			{
				const char *suffix = (g_str_has_suffix(req, "/") ? "index.html" : "");
				file_pathname = g_strdup_printf("%s%s%s", LOCAL_RESOURCE_PATH, req, suffix);
				allocated = true;
			}
			
			if (libwebsockets_serve_http_file(wsi, file_pathname, get_mime(file_pathname)))
				log_error("Failed to send file %s", file_pathname);
			if (allocated)
				g_free(file_pathname);
		}
	}
	else if (reason == LWS_CALLBACK_FILTER_NETWORK_CONNECTION)
	{
		/*
		* callback for confirming to continue with client IP appear in
		* protocol 0 callback since no websocket protocol has been agreed
		* yet.  You can just ignore this if you won't filter on client IP
		* since the default unhandled callback return is 0 meaning let the
		* connection continue.
		*/

		char client_name[128];
		char client_ip[128];

		libwebsockets_get_peer_addresses((int)(long)user, client_name,
			sizeof(client_name), client_ip, sizeof(client_ip));

		log_info_verb("Received network connect from %s (%s)", client_name, client_ip);

		/* if we returned non-zero from here, we kill the connection */
	}

	return 0;
}

/* webarg_api protocol */

static void extract_cmd(void *in, size_t len,
	char **json_text, int *json_text_len, char **ext_text, int *ext_text_len)
{
	*json_text = in;
	*json_text_len = len;
	for (int i = 0; i < len; i++)
	{
		char ch = (*json_text)[i];
#ifdef ENABLE_CARET_SPLIT
		bool caret_split = (ch == '^');
#else
		bool caret_split = false;
#endif
		if (ch == 0 || caret_split)
		{
			*json_text_len = i;
			if (i + 1 < len)
			{
				*ext_text = &((*json_text)[i + 1]);
				*ext_text_len = len - i - 1;
			}
			break;
		}
	}
}

static void webarg_api_receive(struct libwebsocket_context *context,
	struct libwebsocket *wsi, per_session_data__webarg_api *pss,
	void *in, size_t len)
{
	json_init();

	char *json_text = 0;
	int json_text_len = 0;
	char *ext_text = 0;
	int ext_text_len = 0;
	extract_cmd(in, len, &json_text, &json_text_len, &ext_text, &ext_text_len);
	
	if (json_load(json_text, json_text_len))
	{
		char *event = _json_object_dup_member_string(json_get_root_object(), "event");
#ifdef ENABLE_EXEC_SERVICE
		service_exec_receive(context, wsi, pss, event, ext_text, ext_text_len);
#endif
	}

	json_final();
}

static int callback_webarg_api(struct libwebsocket_context *context,
	struct libwebsocket *wsi, enum libwebsocket_callback_reasons reason,
	void *user, void *in, size_t len)
{
	per_session_data__webarg_api *pss = user;

	if (reason == LWS_CALLBACK_ESTABLISHED)
	{
		log_info_verb("callback_webarg_api: LWS_CALLBACK_ESTABLISHED");
		pss->session_id = 0;
	}
	else if (reason == LWS_CALLBACK_CLOSED)
	{
		log_info_verb("callback_webarg_api: LWS_CALLBACK_CLOSED");
		if (pss->session_id)
		{
#ifdef ENABLE_EXEC_SERVICE
			service_exec_close(pss->session_id);
#endif
			pss->session_id = 0;
		}
	}
	else if (reason == LWS_CALLBACK_RECEIVE)
	{
		webarg_api_receive(context, wsi, pss, in, len);
	}
	else if (reason == LWS_CALLBACK_SERVER_WRITEABLE)
	{
#ifdef ENABLE_EXEC_SERVICE
		service_exec_send(wsi, pss);
#endif
	}
	else if (reason == LWS_CALLBACK_FILTER_PROTOCOL_CONNECTION)
	{
	}

	return 0;
}

/* list of supported protocols and callbacks */

static struct libwebsocket_protocols protocols[] = {
	/* first protocol must always be HTTP handler */
	{
		"http-only",   /* name */
		callback_http, /* callback */
		0              /* per_session_data_size */
	},
	{
		"webarg-api",
		callback_webarg_api,
		sizeof(per_session_data__webarg_api)
	},
	{
		NULL, NULL, 0  /* End of list */
	}
};

static struct option options[] = {
	{ "help",	no_argument,		NULL, 'h' },
	{ "port",	required_argument,	NULL, 'p' },
	{ "ssl",	no_argument,		NULL, 's' },
	{ "killmask",	no_argument,		NULL, 'k' },
	{ "interface",  required_argument, 	NULL, 'i' },
	{ NULL, 0, 0, 0 }
};

//void sig_handler()
//{
//  exit(0);
//}

int main(int argc, char **argv)
{
	g_type_init();
	
	int port = 7681;
	bool use_ssl = false;
	const char *cert_path = LOCAL_RESOURCE_PATH"/webarg-server.pem";
	const char *key_path = LOCAL_RESOURCE_PATH"/webarg-server.key.pem";
	char interface_name[128] = "";
	const char *interface = NULL;
	int opts = 0;

	struct libwebsocket_context *context = 0;
	
	//signal(SIGINT, sig_handler);
	
#ifdef ENABLE_EXEC_SERVICE
	service_exec_init();
#endif
	
	log_level_set(LOG_VERBOSE);
	
	log("WebArg server");

	int n = 0;
	while (n >= 0)
	{
		n = getopt_long(argc, argv, "i:khsp:", options, NULL);
		if (n < 0)
			continue;
		switch (n)
		{
		case 's':
			use_ssl = 1;
			break;
		case 'k':
			opts = LWS_SERVER_OPTION_DEFEAT_CLIENT_MASK;
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'i':
			strncpy(interface_name, optarg, sizeof interface_name);
			interface_name[(sizeof interface_name) - 1] = '\0';
			interface = interface_name;
			break;
		case 'h':
			log("Usage: webarg "
				"[--port=<p>] [--ssl]");
			exit(1);
		}
	}

	if (!use_ssl)
		cert_path = key_path = NULL;

	context = libwebsocket_create_context(port, interface, protocols,
		libwebsocket_internal_extensions, cert_path, key_path, -1, -1, opts);
	if (context == NULL)
	{
		log_error("libwebsocket init failed");
		return -1;
	}

	/*
	 * This code works without libwebsocket forked service loop
	 */

	log_info("Using no-fork service loop");

	unsigned int oldus = 0;
	while (true)
	{
		struct timeval tv;

		gettimeofday(&tv, NULL);

		if (((unsigned int)tv.tv_usec - oldus) > PROCESSING_INTERVAL * 1000)
		{
			oldus = tv.tv_usec;
#ifdef ENABLE_EXEC_SERVICE
			service_exec_tick();
#endif
		}

		/*
		 * This server does not fork or create a thread for
		 * websocket service, it all runs in this single loop. So,
		 * we have to give the websockets an opportunity to service
		 * "manually".
		 *
		 * If no socket is needing service, the call below returns
		 * immediately and quickly.
		 */

		libwebsocket_service(context, PROCESSING_INTERVAL);
	}

#ifdef ENABLE_EXEC_SERVICE
	service_exec_final();
#endif
	
	libwebsocket_context_destroy(context);
	
	return 0;
}
