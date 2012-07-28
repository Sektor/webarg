/*  exec.c
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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <glib.h>
#include "../json.h"
#include "../session.h"
#include "exec.h"

#define LOG_MODULE "Exec"
#include "logging.h"

/* defines */

#define PIPE_BUFFER_SIZE 1024 * 1
#define RESP_BUFFER_SIZE (LWS_SEND_BUFFER_PRE_PADDING + JSON_PADDING + \
	PIPE_BUFFER_SIZE + LWS_SEND_BUFFER_POST_PADDING)

#define EXEC_ERROR_EXIT_CODE 127
#define SIGNAL_TIMEOUT_SEC 5

/* global vars */

static GHashTable *exec_hash = NULL;

/* types */

typedef struct
{
	int session_id;
	struct libwebsocket_context *context;
	struct libwebsocket *wsi;
	GArray *resp_queue;
	bool finished;
	int quit_signal;
	int kill_timeout;
	bool signal_sent;
	bool sigkill_sent;
	time_t signal_time;
	char **cmd_args;
	pid_t pid;
	int exit_code;
	
	/* This holds the fd for the input & output of the pipes */
	int	pipe_in[2];
	int	pipe_out[2];
	int	pipe_err[2];
}
exec_entry;

/* logic */

static bool exec_proc(exec_entry *entry)
{
	if (pipe(entry->pipe_in))
	{
		log_error("Error creating stdin replacement pipe");
		return false;
	}

	if (pipe(entry->pipe_out))
	{
		log_error("Error creating stdout replacement pipe");
		return false;
	}

	if (pipe(entry->pipe_err))
	{
		log_error("Error creating stderr replacement pipe");
		return false;
	}
	
	/* Attempt to fork and check for errors */
	
	if ((entry->pid = fork()) == -1)
	{
		log_error("Fork error");
		return false;
	}

	if (entry->pid)
	{
		/* A positive PID indicates the parent process */
		
		/* Close unused side of pipe_in (in side) */
		close(entry->pipe_in[0]);
		entry->pipe_in[0] = 0;

		/* Close unused side of pipe_out (out side) */
		close(entry->pipe_out[1]);
		entry->pipe_out[1] = 0;
		
		/* Close unused side of pipe_err (out side) */
		close(entry->pipe_err[1]);
		entry->pipe_err[1] = 0;
	}
	else
	{
		/* A zero PID indicates that this is the child process */

		/* Replace stdin with the in side of the pipe_in */
		dup2(entry->pipe_in[0], 0);

		/* Replace stdout with the out side of the pipe_out */
		dup2(entry->pipe_out[1], 1);

		/* Replace stderr with the out side of the pipe_err */
		dup2(entry->pipe_err[1], 2);
		
		/* Close unused side of pipe_in (out side) */
		close(entry->pipe_in[1]);

		/* Close unused side of pipe_out (in side) */
		close(entry->pipe_out[0]);

		/* Close unused side of pipe_err (in side) */
		close(entry->pipe_err[0]);
		
		/* Set non-buffered output on pipes */
		setvbuf(stdout, (char*)NULL, _IONBF, 0);
		setvbuf(stderr, (char*)NULL, _IONBF, 0);
		
		/* Replace the child fork with a new process */
		execv(entry->cmd_args[0], &entry->cmd_args[1]);
		
		exit(EXEC_ERROR_EXIT_CODE);
	}
	
	return true;
}

static const char EXEC_RESP_PID_TEMPLATE[] = \
"{" \
	"\"event\": \"exec\"," \
	"\"params\": {" \
		"\"pid\": %d" \
	"}" \
"}";

static const char EXEC_RESP_EXIT_TEMPLATE[] = \
"{" \
	"\"event\": \"exit\"," \
	"\"params\": {" \
		"\"code\": %d" \
	"}" \
"}";

static const char EXEC_RESP_OUT_TEMPLATE[] = \
"{" \
	"\"event\": \"output\"," \
	"\"params\": {" \
		"\"stream\": \"%s\"," \
		"\"size\": %d" \
	"}" \
"}";

static const char EXEC_RESP_IN_TEMPLATE[] = \
"{" \
	"\"event\": \"input\"," \
	"\"params\": {" \
		"\"size\": %d" \
	"}," \
	"\"ticket\": %d" \
"}";

static const char EXEC_RESP_KILL_TEMPLATE[] = \
"{" \
	"\"event\": \"kill\"," \
	"\"params\": {" \
		"\"result\": %d" \
	"}," \
	"\"ticket\": %d" \
"}";

static bool exec_receive_exist(per_session_data__webarg_api *pss,
	char *event, char *ext_text, int ext_text_len)
{
	exec_entry *entry = g_hash_table_lookup(exec_hash, &pss->session_id);
	if (!entry)
		return false;
	
	int ticket = _json_object_get_member_int(json_get_root_object(), "ticket");

	service_resp_entry resp_entry = {0};
	resp_entry.ticket = ticket;
	
	if (!strcmp(event, "input"))
	{
		if (!ext_text || !ext_text_len)
			return true;
		
		int n = write(entry->pipe_in[1], ext_text, ext_text_len);

		resp_entry.type = EXEC_RESP_STDIN;
		resp_entry.value = n;

		g_array_append_val(entry->resp_queue, resp_entry);
	}
	else if (!strcmp(event, "kill"))
	{
		JsonObject *params_object = _json_object_get_member_object(json_get_root_object(), "params");
		int signal = _json_object_get_member_int(params_object, "signal");
		
		int res = kill(entry->pid, signal);
		
		resp_entry.type = EXEC_RESP_KILL;
		resp_entry.value = res;

		g_array_append_val(entry->resp_queue, resp_entry);
	}
	
	return true;
}

bool exec_receive_new(struct libwebsocket_context *context,
	struct libwebsocket *wsi, per_session_data__webarg_api *pss, char *event)
{
	if (strcmp(event, "exec"))
		return false;

	exec_entry *entry = calloc(1, sizeof(exec_entry));
	
	/* parse arguments */

	JsonObject *params_object = _json_object_get_member_object(json_get_root_object(), "params");
	int quit_signal = _json_object_get_member_int(params_object, "quit_signal");
	int kill_timeout = _json_object_get_member_int(params_object, "kill_timeout");
	char *path = _json_object_dup_member_string(params_object, "path");
	JsonNode *args_node = _json_object_get_member(params_object, "args");
	JsonArray *args_array = _json_node_get_array(args_node);
	int args_count = _json_array_get_length(args_array);
	entry->quit_signal = quit_signal;
	entry->kill_timeout = kill_timeout;
	entry->cmd_args = (char **)calloc(args_count + 2, sizeof(char *));
	entry->cmd_args[0] = path;
	for (int i = 0; i < args_count; i++)
	{
		char *arg = 0;
		JsonNode *arg_node = _json_array_get_element(args_array, i);
		arg = _json_node_dup_string(arg_node);
		entry->cmd_args[i + 1] = arg;
	}

	/* create new session */
	
	pss->session_id = session_counter_inc();

	entry->resp_queue = g_array_new(FALSE, FALSE, sizeof(service_resp_entry));
	entry->session_id = pss->session_id;
	entry->context = context;
	entry->wsi = wsi;

	if (!exec_proc(entry))
		entry->finished = true;

	service_resp_entry resp_entry = {0};
	resp_entry.type = EXEC_RESP_PID;
	g_array_append_val(entry->resp_queue, resp_entry);

	g_hash_table_insert(exec_hash, &entry->session_id, entry);
	
	return true;
}

bool service_exec_receive(struct libwebsocket_context *context,
	struct libwebsocket *wsi, per_session_data__webarg_api *pss,
	char *event, char *ext_text, int ext_text_len)
{
	if (pss->session_id)
		return exec_receive_exist(pss, event, ext_text, ext_text_len);
	return exec_receive_new(context, wsi, pss, event);
}

bool service_exec_close(int session_id)
{
	exec_entry *entry = g_hash_table_lookup(exec_hash, &session_id);
	if (!entry)
		return false;
	
	entry->context = 0;
	entry->wsi = 0;
	return true;
}

bool service_exec_send(struct libwebsocket *wsi,
	per_session_data__webarg_api *pss)
{
	static unsigned char resp_buf[LWS_SEND_BUFFER_PRE_PADDING +
		JSON_PADDING + LWS_SEND_BUFFER_POST_PADDING];
	unsigned char *resp_buf_json = &resp_buf[LWS_SEND_BUFFER_PRE_PADDING];
	
	if (!pss->session_id)
		return false;

	exec_entry *entry = g_hash_table_lookup(exec_hash, &pss->session_id);
	if (!entry)
		return false;
	if (!entry->resp_queue->len)
		return true;
	
	service_resp_entry resp_entry = g_array_index(entry->resp_queue, service_resp_entry, 0);
	service_resp_type type = resp_entry.type;
	unsigned char *buf = resp_buf_json;
	int resp_size = 0;

	if (type == EXEC_RESP_STDOUT)
	{
		if (resp_entry.value)
		{
			resp_size = resp_entry.value;
			int json_size = sprintf((char *)resp_buf_json,
				EXEC_RESP_OUT_TEMPLATE, "stdout", resp_size);
			unsigned char *data = resp_entry.data;
			buf = &data[LWS_SEND_BUFFER_PRE_PADDING + JSON_PADDING - json_size - 1];
			strcpy((char *)buf, (char *)resp_buf_json);
			resp_size += json_size + 1;
		}
	}
	else if (type == EXEC_RESP_STDERR)
	{
		if (resp_entry.value)
		{
			resp_size = resp_entry.value;
			int json_size = sprintf((char *)resp_buf_json,
				EXEC_RESP_OUT_TEMPLATE, "stderr", resp_size);
			unsigned char *data = resp_entry.data;
			buf = &data[LWS_SEND_BUFFER_PRE_PADDING + JSON_PADDING - json_size - 1];
			strcpy((char *)buf, (char *)resp_buf_json);
			resp_size += json_size + 1;
		}
	}
	else if (type == EXEC_RESP_STDIN)
	{
		resp_size = sprintf((char *)buf, EXEC_RESP_IN_TEMPLATE, resp_entry.value, resp_entry.ticket);
	}
	else if (type == EXEC_RESP_KILL)
	{
		resp_size = sprintf((char *)buf, EXEC_RESP_KILL_TEMPLATE, resp_entry.value, resp_entry.ticket);
	}
	else if (type == EXEC_RESP_PID)
	{
		resp_size = sprintf((char *)buf, EXEC_RESP_PID_TEMPLATE, entry->pid);
	}
	else if (type == EXEC_RESP_EXIT)
	{
		resp_size = sprintf((char *)buf, EXEC_RESP_EXIT_TEMPLATE, entry->exit_code);
	}

	if (buf && resp_size)
	{
		log_info_verb("#%d: Tx: %s", entry->session_id, buf);
		int n = libwebsocket_write(wsi, buf, resp_size, LWS_WRITE_TEXT);
		if (n < 0)
			log_error("#%d: Error writing to socket", entry->session_id);
	}

	resp_queue_remove_index(entry->resp_queue, 0);
	
	return true;
}

static bool exec_read_pipe(int fd, unsigned char **buf, int *read_bytes)
{
	if (!fd || !buf)
		return false;
	
	fd_set fds;
	struct timeval t;

	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	t.tv_sec = 0;
	t.tv_usec = 0;
	select(1 + fd, &fds, NULL, NULL, &t);

	if (FD_ISSET(fd, &fds))
	{
		unsigned char *resp_buf = malloc(RESP_BUFFER_SIZE);
		unsigned char *pipe_buf = &resp_buf[LWS_SEND_BUFFER_PRE_PADDING + JSON_PADDING];
		int rval = read(fd, pipe_buf, PIPE_BUFFER_SIZE);
		if (read_bytes)
			*read_bytes = rval;
		if (rval > 0)
		{
			*buf = resp_buf;
			return true;
		}
		else
			free(resp_buf);
	}
	
	return false;
}

static gboolean exec_iterator(gpointer key, gpointer value, gpointer user_data)
{
	exec_entry *entry = value;
	
	char *zero_arg = (entry->cmd_args ? entry->cmd_args[0] : 0);
	char *proc_name = (zero_arg ? zero_arg : "<noname>");
	
	struct libwebsocket_context *context = entry->context;
	struct libwebsocket *wsi = entry->wsi;
	bool has_session = (context && wsi);

	if (has_session)
	{
		int fd = 0;
		unsigned char *buf = NULL;
		int read_bytes = 0;

		fd = entry->pipe_out[0];
		while (exec_read_pipe(fd, &buf, &read_bytes))
		{
			service_resp_entry resp_entry = {0};
			resp_entry.type = EXEC_RESP_STDOUT;
			resp_entry.value = read_bytes;
			resp_entry.data = buf;
			g_array_append_val(entry->resp_queue, resp_entry);
		}

		fd = entry->pipe_err[0];
		while (exec_read_pipe(fd, &buf, &read_bytes))
		{
			service_resp_entry resp_entry = {0};
			resp_entry.type = EXEC_RESP_STDERR;
			resp_entry.value = read_bytes;
			resp_entry.data = buf;
			g_array_append_val(entry->resp_queue, resp_entry);
		}
	}

	if (!entry->finished)
	{
		int status = 0;
		if (waitpid(entry->pid, &status, WNOHANG) == entry->pid)
		{
			/* Child process is not running */
			log_info_verb("#%d: Process %s is finished", entry->session_id, proc_name);
			entry->finished = true;
			entry->exit_code = status;
			
			service_resp_entry resp_entry = {0};
			resp_entry.type = EXEC_RESP_EXIT;
			g_array_append_val(entry->resp_queue, resp_entry);
		}
		
		if (!has_session && !entry->finished)
		{
			if (!entry->signal_sent)
			{
				/* Session closed. Terminating associated child process. */
				int quit_signal = (entry->quit_signal ? entry->quit_signal : SIGINT);
				const char *sig_name = strsignal(quit_signal);
				log_info_verb("#%d: Sending %s to %s", entry->session_id, sig_name, proc_name);
				entry->signal_sent = true;
				entry->signal_time = time(0);
				entry->sigkill_sent = (entry->kill_timeout < 0);
				if (kill(entry->pid, quit_signal) == -1)
					log_error("#%d: Error sending %s to child process", entry->session_id, sig_name);
			}
			else if (!entry->sigkill_sent)
			{
				int kill_timeout = (entry->kill_timeout > 0 ? entry->kill_timeout : SIGNAL_TIMEOUT_SEC);
				time_t cur_time = time(0);
				if (entry->signal_time > cur_time)
					entry->signal_time = cur_time;
				if (cur_time - entry->signal_time > kill_timeout)
				{	
					entry->sigkill_sent = true;
					log_info_verb("#%d: Process %s still not finished. Sending SIGKILL.", entry->session_id, proc_name);
					if (kill(entry->pid, SIGKILL) == -1)
						log_error("#%d: Error sending SIGKILL to child process", entry->session_id);
				}
			}
		}
	}

	if (!has_session)
		resp_queue_clear(entry->resp_queue);

	if (entry->finished && !entry->resp_queue->len)
	{
		log_info_verb("#%d: Removing entry for process %s", entry->session_id, proc_name);
		return TRUE;
	}

	if (entry->resp_queue->len)
		libwebsocket_callback_on_writable(context, wsi);
	
	return FALSE;
}

static void exec_entry_destroy(gpointer data)
{
	exec_entry *entry = data;
	IFNONZERO(g_strfreev, entry->cmd_args);
	IFNONZERO(resp_queue_clear, entry->resp_queue);
	IFNONZEROEX(g_array_free, entry->resp_queue, FALSE);
	IFNONZERO(close, entry->pipe_in[0]);
	IFNONZERO(close, entry->pipe_in[1]);
	IFNONZERO(close, entry->pipe_out[0]);
	IFNONZERO(close, entry->pipe_out[1]);
	IFNONZERO(close, entry->pipe_err[0]);
	IFNONZERO(close, entry->pipe_err[1]);
	free(entry);
}

void service_exec_tick()
{
	g_hash_table_foreach_remove(exec_hash, (GHRFunc)exec_iterator, NULL);
}

void service_exec_init()
{
	exec_hash = g_hash_table_new_full(g_int_hash, g_int_equal, NULL, exec_entry_destroy);
}

void service_exec_final()
{
	g_hash_table_destroy(exec_hash);
}
