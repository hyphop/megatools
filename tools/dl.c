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
#ifdef G_OS_WIN32
#include <io.h>
#include <fcntl.h>
#endif

static gchar* opt_path = ".";

#if !defined OPS
static gboolean opt_stream = FALSE;
#define OPS "1"
#endif

#if !defined OPN
static gboolean opt_noprogress = FALSE;
#define OPN "1"
#endif

static gboolean opt_print_names = FALSE;

static GOptionEntry entries_dl[] =
{
  { "path",          '\0',   0, G_OPTION_ARG_FILENAME,  &opt_path,        "Local directory or file name, to save data to",  "PATH" },
  { "no-progress",   '\0',   0, G_OPTION_ARG_NONE,      &opt_noprogress,  "Disable progress bar",                           NULL  },
  { "print-names",   '\0',   0, G_OPTION_ARG_NONE,      &opt_print_names, "Print names of downloaded files",                NULL  },
  { NULL }
};

#if !defined GC
static gchar* cur_file = NULL;
#define GC "1"
#endif

static mega_session* s;

static gboolean status_callback_dl(mega_status_data* data, gpointer userdata)
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

// download operation

static gboolean dl_sync_file(mega_node* node, GFile* file, const gchar* remote_path)
{
  gc_error_free GError *local_err = NULL;
  gc_free gchar* local_path = g_file_get_path(file);

  if (g_file_query_exists(file, NULL))
  {
    g_printerr("ERROR: File already exists at %s\n", local_path);
    return FALSE;
  }

  if (!opt_noprogress)
    g_print("F %s\n", local_path);

  if (!mega_session_get_compat(s, local_path, remote_path, &local_err))
  {
    if (!opt_noprogress)
      g_print("\r" ESC_CLREOL);
    g_printerr("ERROR: Download failed for %s: %s\n", remote_path, local_err->message);
    return FALSE;
  }

  if (!opt_noprogress)
    g_print("\r" ESC_CLREOL);

  if (opt_print_names)
    g_print("%s\n", local_path);

  return TRUE;
}

static gboolean dl_sync_dir(mega_node* node, GFile* file, const gchar* remote_path)
{
  gc_error_free GError *local_err = NULL;
  gc_free gchar* local_path = g_file_get_path(file);

  if (!g_file_query_exists(file, NULL))
  {
    if (!opt_noprogress)
      g_print("D %s\n", local_path);

    if (!g_file_make_directory(file, NULL, &local_err))
    {
      g_printerr("ERROR: Can't create local directory %s: %s\n", local_path, local_err->message);
      return FALSE;
    }
  }
  else
  {
    if (g_file_query_file_type(file, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL) != G_FILE_TYPE_DIRECTORY)
    {
      g_printerr("ERROR: Can't create local directory %s: file exists\n", local_path);
      return FALSE;
    }
  }

  // sync children
  GSList* children = mega_session_get_node_chilren(s, node), *i;
  gboolean status = TRUE;
  for (i = children; i; i = i->next)
  {
    mega_node* child = i->data;
    gc_free gchar* child_remote_path = g_strconcat(remote_path, "/", child->name, NULL);
    gc_object_unref GFile* child_file = g_file_get_child(file, child->name);

    if (child->type == 0)
    {
      if (!dl_sync_file(child, child_file, child_remote_path))
        status = FALSE;
    }
    else
    {
      if (!dl_sync_dir(child, child_file, child_remote_path))
        status = FALSE;
    }
  }

  g_slist_free(children);
  return status;
}

#define DL "1"
#include "main.h"

int main_dl(int ac, char* av[])
{
  gc_error_free GError *local_err = NULL;
  gc_regex_unref GRegex *file_regex = NULL, *folder_regex = NULL;
  gint i;
  int status = 0;

  tool_init(&ac, &av, "- download exported files from mega.nz", entries_dl, TOOL_INIT_AUTH_OPTIONAL);

  if (!strcmp(opt_path, "-"))
  {
    opt_noprogress = opt_stream = TRUE;

    // see https://github.com/megous/megatools/issues/38
#ifdef G_OS_WIN32
    setmode(fileno(stdout), O_BINARY);
#endif
  }

  if (ac < 2)
  {
    g_printerr("ERROR: No links specified for download!\n");
    tool_fini(NULL);
    return 1;
  }

  if (opt_stream && ac != 2)
  {
    g_printerr("ERROR: Can't stream from multiple files!\n");
    tool_fini(NULL);
    return 1;
  }

  // prepare link parsers

  file_regex = g_regex_new("^https?://mega(?:\\.co)?\\.nz/#!([a-z0-9_-]{8})!([a-z0-9_-]{43})$", G_REGEX_CASELESS, 0, NULL);
  g_assert(file_regex != NULL);

  folder_regex = g_regex_new("^https?://mega(?:\\.co)?\\.nz/#F!([a-z0-9_-]{8})!([a-z0-9_-]{22})(![a-z0-9_-]{8})?$", G_REGEX_CASELESS, 0, NULL);
  g_assert(folder_regex != NULL);

  // create session

  s = tool_start_session(TOOL_SESSION_OPEN | TOOL_SESSION_AUTH_ONLY | TOOL_SESSION_AUTH_OPTIONAL);

  mega_session_watch_status(s, status_callback_dl, NULL);

  // process links
  for (i = 1; i < ac; i++)
  {
    gc_match_info_unref GMatchInfo* m1 = NULL;
    gc_match_info_unref GMatchInfo* m2 = NULL;
    gc_free gchar* key = NULL;
    gc_free gchar* handle = NULL;
    gc_free gchar* specific = NULL;
    gc_free gchar* link = tool_convert_filename(av[i], FALSE);

    if (g_regex_match(file_regex, link, 0, &m1))
    {
      handle = g_match_info_fetch(m1, 1);
      key = g_match_info_fetch(m1, 2);

      // perform download
      if (!mega_session_dl_compat(s, handle, key, opt_stream ? NULL : opt_path, &local_err))
      {
        if (!opt_noprogress)
          g_print("\r" ESC_CLREOL "\n");
        g_printerr("ERROR: Download failed for '%s': %s\n", link, local_err->message);
        g_clear_error(&local_err);
        status = 1;
      }
      else
      {
        if (!opt_noprogress)
          g_print("\r" ESC_CLREOL "Downloaded %s\n", cur_file);
        if (opt_print_names)
          g_print("%s\n", cur_file);
      }
    }
    else if (g_regex_match(folder_regex, link, 0, &m2))
    {
      if (opt_stream)
      {
        g_printerr("ERROR: Can't stream from a directory!\n");
        tool_fini(s);
        return 1;
      }

      handle = g_match_info_fetch(m2, 1);
      key = g_match_info_fetch(m2, 2);
      specific = g_match_info_fetch(m2, 3);

      // remove first char of |specific| since it's an '!'
      if (specific)
        memmove(&specific[0], &specific[1], strlen(specific));

      // perform download
      if (!mega_session_open_exp_folder(s, handle, key, specific, &local_err))
      {
        g_printerr("ERROR: Can't open folder '%s': %s\n", link, local_err->message);
        g_clear_error(&local_err);
        status = 1;
      }
      else
      {
        mega_session_watch_status(s, status_callback_dl, NULL);

        GSList* l = mega_session_ls(s, "/", FALSE);
        if (g_slist_length(l) == 1)
        {
          mega_node* root_node = l->data;

          gc_object_unref GFile* local_dir = g_file_new_for_path(opt_path);
          if (g_file_query_file_type(local_dir, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL) == G_FILE_TYPE_DIRECTORY)
          {
            gc_free gchar* node_path = mega_node_get_path_dup(root_node);
            if (!dl_sync_dir(root_node, local_dir, node_path))
              status = 1;
          }
          else
          {
            g_printerr("ERROR: %s must be a directory\n", opt_path);
            status = 1;
          }
        }
        else
        {
          g_printerr("ERROR: EXP folder fs has multiple toplevel nodes? Weird!\n");
          status = 1;
        }

        g_slist_free(l);
      }
    }
    else
    {
      g_printerr("WARNING: Skipping invalid Mega download link: %s\n", link);
    }
  }

  tool_fini(s);
  return status;
}
#include "main2.h"
