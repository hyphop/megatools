/*
 *  megatools - Mega.nz client library and tools
 *  Copyright (C) 2013  Ondřej Jirman <megous@megous.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "tools.h"

static gchar* opt_path_get = ".";

#if !defined OPS
static gboolean opt_stream = FALSE;
#define OPS "1"
#endif

#if !defined OPN
static gboolean opt_noprogress = FALSE;
#define OPN "1"
#endif

static GOptionEntry entries_get[] =
{
  { "path",          '\0',   0, G_OPTION_ARG_FILENAME,  &opt_path_get,  "Local directory or file name, to save data to",  "PATH" },
  { "no-progress",   '\0',   0, G_OPTION_ARG_NONE,    &opt_noprogress,  "Disable progress bar",   NULL},
  { NULL }
};

#if !defined GC
static gchar* cur_file = NULL;
#define GC "1"
#endif

static gboolean status_callback_get(mega_status_data* data, gpointer userdata)
{
  if (opt_stream && data->type == MEGA_STATUS_DATA)
  {
    fwrite(data->data.buf, data->data.size, 1, stdout);
    fflush(stdout);
  }

  if (data->type == MEGA_STATUS_FILEINFO)
  {
    cur_file = g_strdup(data->fileinfo.name);
  }

  if (!opt_noprogress && data->type == MEGA_STATUS_PROGRESS)
    tool_show_progress(cur_file, data);

  return FALSE;
}

#define GET "1"
#include "main.h"

int main_get(int ac, char* av[])
{
  gc_error_free GError *local_err = NULL;
  mega_session* s;
  gint i, status = 0;

  tool_init(&ac, &av, "- download individual files from mega.nz", entries_get, TOOL_INIT_AUTH);

  if (!strcmp(opt_path_get, "-"))
    opt_noprogress = opt_stream = TRUE;

  if (ac < 2)
  {
    g_printerr("ERROR: No files specified for download!\n");
    tool_fini(NULL);
    return 1;
  }

  if (opt_stream && ac != 2)
  {
    g_printerr("ERROR: Can't stream from multiple files!\n");
    tool_fini(NULL);
    return 1;
  }

  s = tool_start_session(TOOL_SESSION_OPEN);
  if (!s)
  {
    tool_fini(NULL);
    return 1;
  }

  // stream file
  if (opt_stream)
  {
    mega_node* n = mega_session_stat(s, av[1]);
    if (!n)
    {
      g_printerr("ERROR: Remote file not found: %s", av[1]);
      return FALSE;
    }

    if (!mega_session_get(s, NULL, n, &local_err))
    {
      g_printerr("ERROR: Download failed for '%s': %s\n", av[1], local_err->message);
      g_clear_error(&local_err);
      status = 1;
    }

    tool_fini(s);
    return status;
  }

  // download files
  mega_session_watch_status(s, status_callback_get, NULL);

  for (i = 1; i < ac; i++)
  {
    gc_free gchar* path = tool_convert_filename(av[i], FALSE);

    // perform download
    if (!mega_session_get_compat(s, opt_path_get, path, &local_err))
    {
      if (!opt_noprogress)
        g_print("\r" ESC_CLREOL "\n");
      g_printerr("ERROR: Download failed for '%s': %s\n", path, local_err->message);
      g_clear_error(&local_err);
      status = 1;
    }
    else
    {
      if (!opt_noprogress)
        g_print("\r" ESC_CLREOL "Downloaded %s\n", cur_file);
    }
  }

  tool_fini(s);
  return status;
}
#include "main2.h"
