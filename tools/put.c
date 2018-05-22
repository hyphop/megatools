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

static gchar* opt_path_put = "/Root";

#if !defined OPN
static gboolean opt_noprogress = FALSE;
#define OPN "1"
#endif

static GOptionEntry entries_put[] =
{
  { "path",             '\0',   0, G_OPTION_ARG_STRING,  &opt_path_put,         "Remote path to save files to",          "PATH" },
  { "no-progress",      '\0',   0, G_OPTION_ARG_NONE,    &opt_noprogress,   "Disable progress bar",                  NULL   },
  { NULL }
};

#if !defined GC
static gchar* cur_file = NULL;
#define GC "1"
#endif

static gboolean status_callback_put(mega_status_data* data, gpointer userdata)
{
  if (data->type == MEGA_STATUS_FILEINFO)
  {
    g_free(cur_file);
    cur_file = g_strdup(data->fileinfo.name);
  }

  if (!opt_noprogress && data->type == MEGA_STATUS_PROGRESS)
    tool_show_progress(cur_file, data);

  return FALSE;
}

#define PUT "1"
#include "main.h"

int main_put(int ac, char* av[])
{
  gc_error_free GError *local_err = NULL;
  mega_session* s;

  tool_init(&ac, &av, "- upload files to mega.nz", entries_put, TOOL_INIT_AUTH | TOOL_INIT_UPLOAD_OPTS);

  if (ac < 2)
  {
    g_printerr("ERROR: No files specified for upload!\n");
    tool_fini(NULL);
    return 1;
  }

  s = tool_start_session(TOOL_SESSION_OPEN);
  if (!s)
    return 1;

  mega_session_watch_status(s, status_callback_put, NULL);

  gint status = 0;
  gint i;
  for (i = 1; i < ac; i++)
  {
    gc_free gchar* path = tool_convert_filename(av[i], TRUE);

    g_free(cur_file);
    cur_file = g_path_get_basename(path);

    // perform download
    if (!mega_session_put_compat(s, opt_path_put, path, &local_err))
    {
      if (!opt_noprogress)
        g_print("\r" ESC_CLREOL "\n");

      g_printerr("ERROR: Upload failed for '%s': %s\n", path, local_err->message);
      g_clear_error(&local_err);

      status = 1;
    }
    else
    {
      if (!opt_noprogress)
        g_print("\r" ESC_CLREOL "Uploaded %s\n", cur_file);
    }
  }

  mega_session_save(s, NULL);

  tool_fini(s);
  return status;
}
#include "main2.h"
