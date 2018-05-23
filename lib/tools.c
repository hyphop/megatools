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

#include <curl/curl.h>
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <locale.h>

#include "config.h"
#include "tools.h"

#ifdef G_OS_WIN32
#include <windows.h>
#else
#include <signal.h>
#include <termios.h>
#include <unistd.h>
#endif

#ifdef G_OS_WIN32
#define MEGA_RC_FILENAME "mega.ini"
#else
#define MEGA_RC_FILENAME ".megarc"
#endif

#define BOOLEAN_UNSET_BUT_TRUE 2

static GOptionContext* opt_context;
static gchar* opt_username;
static gchar* opt_password;
static gchar* opt_config;
static gboolean opt_reload_files;
static gboolean opt_version;
static gboolean opt_no_config;
static gboolean opt_no_ask_password;
static gint opt_speed_limit = -1; /* -1 means limit not set */
static gchar* opt_proxy;

static gchar* proxy;
static gint upload_speed_limit;
static gint download_seed_limit;
static gint cache_timout = 10 * 60;
static gboolean opt_enable_previews = BOOLEAN_UNSET_BUT_TRUE;

static gboolean opt_debug_callback(const gchar *option_name, const gchar *value, gpointer data, GError **error)
{
  if (value)
  {
    gchar** opts = g_strsplit(value, ",", 0);
    gchar** opt = opts;

    while (*opt)
    {
      if (g_ascii_strcasecmp(*opt, "api") == 0)
        mega_debug |= MEGA_DEBUG_API;
      else if (g_ascii_strcasecmp(*opt, "fs") == 0)
        mega_debug |= MEGA_DEBUG_FS;
      else if (g_ascii_strcasecmp(*opt, "cache") == 0)
        mega_debug |= MEGA_DEBUG_CACHE;
      else if (g_ascii_strcasecmp(*opt, "http") == 0)
        mega_debug |= MEGA_DEBUG_HTTP;

      opt++;
    }

    g_strfreev(opts);
  }
  else
  {
    mega_debug = MEGA_DEBUG_API;
  }

  return TRUE;
}

static GOptionEntry basic_options[] =
{
  { "config",             'c',   0,                          G_OPTION_ARG_FILENAME, &opt_config,        "Load configuration from a file",               "PATH"  },
  { "ignore-config-file", '\0',  0,                          G_OPTION_ARG_NONE,     &opt_no_config,     "Disable loading " MEGA_RC_FILENAME,            NULL    },
  { "debug",              '\0',  G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK, opt_debug_callback, "Enable debugging output",                      "OPTS"  },
  { "version",            '\0',  0,                          G_OPTION_ARG_NONE,     &opt_version,       "Show version information",                     NULL    },
  { NULL }
};

static GOptionEntry upload_options[] =
{
  { "enable-previews",    '\0',  0, G_OPTION_ARG_NONE,      &opt_enable_previews,  "Generate previews when uploading file",       NULL       },
  { "disable-previews",   '\0',  G_OPTION_FLAG_REVERSE,
	                            G_OPTION_ARG_NONE,      &opt_enable_previews,  "Never generate previews when uploading file", NULL       },
  { NULL }
};

static GOptionEntry auth_options[] =
{
  { "username",            'u',  0, G_OPTION_ARG_STRING,    &opt_username,         "Account username (email)",                    "USERNAME" },
  { "password",            'p',  0, G_OPTION_ARG_STRING,    &opt_password,         "Account password",                            "PASSWORD" },
  { "no-ask-password",    '\0',  0, G_OPTION_ARG_NONE,      &opt_no_ask_password,  "Never ask interactively for a password",      NULL       },
  { "reload",             '\0',  0, G_OPTION_ARG_NONE,      &opt_reload_files,     "Reload filesystem cache",                     NULL       },
  { NULL }
};

static GOptionEntry network_options[] =
{
  { "limit-speed",        '\0',  0, G_OPTION_ARG_INT,       &opt_speed_limit,      "Limit transfer speed (KiB/s)",    "SPEED"  },
  { "proxy",              '\0',  0, G_OPTION_ARG_STRING,    &opt_proxy,            "Proxy setup string",              "PROXY"  },
  { NULL }
};

#if OPENSSL_VERSION_NUMBER >= 0x10100004L
static void init_openssl_locking()
{
  // OpenSSL >= 1.1.0-pre4 doesn't require specific callback setup
}
#else
#if GLIB_CHECK_VERSION(2, 32, 0)

static GMutex* openssl_mutexes = NULL;

static void openssl_locking_callback(int mode, int type, const char *file, int line)
{
  if (mode & CRYPTO_LOCK)
    g_mutex_lock(openssl_mutexes + type);
  else
    g_mutex_unlock(openssl_mutexes + type);
}

static unsigned long openssl_thread_id_callback()
{
  unsigned long ret;
  ret = (unsigned long)g_thread_self();
  return ret;
}

static void init_openssl_locking()
{
  gint i;

  // initialize OpenSSL locking for multi-threaded operation
  openssl_mutexes = g_new(GMutex, CRYPTO_num_locks());
  for (i = 0; i < CRYPTO_num_locks(); i++)
    g_mutex_init(openssl_mutexes + i);

  SSL_library_init();
  CRYPTO_set_id_callback(openssl_thread_id_callback);
  CRYPTO_set_locking_callback(openssl_locking_callback);
}

#else

static GMutex** openssl_mutexes = NULL;

static void openssl_locking_callback(int mode, int type, const char *file, int line)
{
  if (mode & CRYPTO_LOCK)
    g_mutex_lock(openssl_mutexes[type]);
  else
    g_mutex_unlock(openssl_mutexes[type]);
}

static unsigned long openssl_thread_id_callback()
{
  unsigned long ret;
  ret = (unsigned long)g_thread_self();
  return ret;
}

static void init_openssl_locking()
{
  gint i;

  // initialize OpenSSL locking for multi-threaded operation
  openssl_mutexes = g_new(GMutex*, CRYPTO_num_locks());
  for (i = 0; i < CRYPTO_num_locks(); i++)
    openssl_mutexes[i] = g_mutex_new();

  SSL_library_init();
  CRYPTO_set_id_callback(openssl_thread_id_callback);
  CRYPTO_set_locking_callback(openssl_locking_callback);
}

#endif
#endif

#ifdef G_OS_WIN32
static gchar* get_tools_dir(void)
{
  gchar *path = NULL;
  wchar_t *wpath;
  DWORD len = PATH_MAX;

  HMODULE handle = GetModuleHandleW(NULL);

  wpath = g_new0(wchar_t, len);
  if (GetModuleFileNameW(handle, wpath, len) < len)
    path = g_utf16_to_utf8(wpath, -1, NULL, NULL, NULL);
  g_free(wpath);

  if (path == NULL)
    path = g_strdup("");

  gchar* dir = g_path_get_dirname(path);
  g_free(path);
  return dir;
}
#endif

static void init(void)
{
#if !GLIB_CHECK_VERSION(2, 32, 0)
  if (!g_thread_supported())
    g_thread_init(NULL);
#endif

  setlocale(LC_ALL, "");

#if !GLIB_CHECK_VERSION(2, 36, 0)
  g_type_init();
#endif

  g_setenv("GSETTINGS_BACKEND", "memory", TRUE);

#ifndef G_OS_WIN32
  signal(SIGPIPE, SIG_IGN);
#endif

  init_openssl_locking();

#ifdef G_OS_WIN32
  gchar* tools_dir = get_tools_dir();

  gchar* tmp = g_build_filename(tools_dir, "gio", NULL);
  g_setenv("GIO_EXTRA_MODULES", tmp, TRUE);
  g_free(tmp);

  gchar* certs = g_build_filename(tools_dir, "ca-certificates.crt", NULL);
  g_setenv("CA_CERT_PATH", certs, TRUE);
  g_free(certs);

  g_free(tools_dir);
#endif
}

void tool_show_progress(const gchar* file, const mega_status_data *data)
{
  if (data->progress.total <= 0)
    return;

  gc_free gchar* done_str = g_format_size_full(data->progress.done, G_FORMAT_SIZE_IEC_UNITS | G_FORMAT_SIZE_LONG_FORMAT);
  gc_free gchar* total_str = g_format_size_full(data->progress.total, G_FORMAT_SIZE_IEC_UNITS);
  const guint64 rate = (data->progress.done - data->progress.last) * 1e6 / data->progress.span;
  gc_free gchar* rate_str = g_format_size_full(rate, G_FORMAT_SIZE_IEC_UNITS);
  const double percentage = (double)(data->progress.done * 100 * 1000 / data->progress.total) / 1000.0;

  g_print(
    ESC_WHITE "%s"
    ESC_NORMAL ": " ESC_YELLOW "%.2f%%"
    ESC_NORMAL " - " ESC_GREEN "%s"
    ESC_BLUE " of %s"
    ESC_NORMAL " (%s/s)" ESC_CLREOL "\r",
    file,
    percentage,
    done_str,
    total_str,
    rate_str
  );
}

static gchar* input_password(void)
{
  gint tries = 3;
  gchar buf[1025];
  gchar* password = NULL;

#ifdef G_OS_WIN32
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
  DWORD mode = 0;
  GetConsoleMode(hStdin, &mode);
  SetConsoleMode(hStdin, mode & (~ENABLE_ECHO_INPUT));
#else
  struct termios oldt;
  tcgetattr(STDIN_FILENO, &oldt);
  struct termios newt = oldt;
  newt.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
#endif

again:
  g_print("Enter password for (%s): ", opt_username);
  if (fgets(buf, 1024, stdin))
  {
    if (strlen(buf) > 1)
    {
      password = g_strndup(buf, strcspn(buf, "\r\n"));
    }
    else
    {
      if (--tries > 0)
      {
        g_print("\n");
        goto again;
      }

      g_print("\nYou need to provide non-empty password!\n");
      exit(1);
    }
  }
  else
  {
    g_printerr("\nERROR: Can't read password from the input!\n");
    exit(1);
  }

#ifdef G_OS_WIN32
  SetConsoleMode(hStdin, mode);
#else
  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
#endif

  g_print("\nGood, signing in...\n");

  return password;
}

static void print_version(void)
{
  if (opt_version)
  {
    g_print("megatools " VERSION " - command line tools for Mega.nz\n\n");
    g_print("Written by Ondřej Jirman <megous@megous.com>, 2013\n");
    g_print("Go to http://megatools.megous.com for more information\n");
    exit(0);
  }
}

gchar* tool_convert_filename(const gchar* path, gboolean local)
{
  gchar* locale_path;

#ifdef G_OS_WIN32
  locale_path = g_locale_to_utf8(path, -1, NULL, NULL, NULL);
#else
  if (local)
    locale_path = g_strdup(path);
  else
    locale_path = g_locale_to_utf8(path, -1, NULL, NULL, NULL);
#endif

  if (locale_path == NULL)
  {
    g_printerr("ERROR: Invalid filename locale, can't convert file names specified on the command line to UTF-8.\n");
    exit(1);
  }

  return locale_path;
}

void tool_init(gint* ac, gchar*** av, const gchar* tool_name, GOptionEntry* tool_entries, ToolInitFlags flags)
{
  GError *local_err = NULL;

  init();

  opt_context = g_option_context_new(tool_name);
  if (tool_entries)
    g_option_context_add_main_entries(opt_context, tool_entries, NULL);

  GOptionGroup* basic_group = g_option_group_new("basic", "Basic Options:", "Show basic options", NULL, NULL);
  g_option_group_add_entries(basic_group, basic_options);
  g_option_context_add_group(opt_context, basic_group);

  GOptionGroup* network_group = g_option_group_new("network", "Network Options:", "Show network options", NULL, NULL);
  g_option_group_add_entries(network_group, network_options);
  g_option_context_add_group(opt_context, network_group);

  if (flags & (TOOL_INIT_AUTH | TOOL_INIT_AUTH_OPTIONAL))
  {
    GOptionGroup* auth_group = g_option_group_new("auth", "Authentication Options:", "Show authentication options", NULL, NULL);
    g_option_group_add_entries(auth_group, auth_options);
    g_option_context_add_group(opt_context, auth_group);
  }

  if (flags & TOOL_INIT_UPLOAD_OPTS)
  {
    GOptionGroup* upload_group = g_option_group_new("upload", "Upload Options:", "Show upload options", NULL, NULL);
    g_option_group_add_entries(upload_group, upload_options);
    g_option_context_add_group(opt_context, upload_group);
  }

  if (!g_option_context_parse(opt_context, ac, av, &local_err))
  {
    g_printerr("ERROR: Option parsing failed: %s\n", local_err->message);
    g_clear_error(&local_err);
    exit(1);
  }

  print_version();

  if (!opt_no_config || opt_config) {
//    gboolean status = TRUE;
    gboolean status = FALSE;
    gc_key_file_unref GKeyFile* kf = g_key_file_new();


//////// UGLY )))

    if ( opt_config ) {
    if ( opt_config[0] == '-' ) {

    fprintf(stderr, "[i] try config from  %s\n", "STDIN");

#define BUF_SIZE 4096
    char config_str[BUF_SIZE];
    config_str[0] = '\0';
//    char* config_str = malloc(BUF_SIZE);

    int len;
    while(len  =read( 0 , config_str, BUF_SIZE) > 0)
    {
	//fprintf(stderr, "> %d %d %s\n", len, strlen(config_str), config_str);
    }

//    if (
     status = g_key_file_load_from_data (kf, config_str, strlen(config_str)-1 ,
//		   0, &local_err))
		   0, NULL)
    ;

	fprintf(stderr, "[i] config status %d\n", status);

//    )
//      {
//        g_printerr("ERROR: Failed to open config file: %s: %s\n", opt_config, local_err->message);
//        g_clear_error(&local_err);
//        exit(1);
//      }
    } else {

	fprintf(stderr, "[i] try config form %s\n", opt_config);
	

    //if (
     status = g_key_file_load_from_file(kf, opt_config, 0, NULL);
    //)
//      {
//        g_printerr("ERROR: Failed to open config file: %s: %s\n", opt_config, local_err->message);
//        g_clear_error(&local_err);
//        exit(1);
//      }
    	fprintf(stderr, "[i] config status %d\n", status);

    }


    }

      if (!status) {

	fprintf(stderr, "[i] try config form %s\n", MEGA_RC_FILENAME);
        status = g_key_file_load_from_file(kf,MEGA_RC_FILENAME , 0, NULL);
	fprintf(stderr, "[i] config status %d\n", status);

      if (!status)
      {
        gc_free gchar* tmp = g_build_filename(g_get_home_dir(), MEGA_RC_FILENAME, NULL);
	fprintf(stderr, "[i] try config form %s\n", tmp);
        status = g_key_file_load_from_file(kf, tmp, 0, NULL);

	fprintf(stderr, "[i] config status %d\n", status);

      }

      }


    if (status)
    {
      // load username/password from ini file
      if (!opt_username)
        opt_username = g_key_file_get_string(kf, "Login", "Username", NULL);

//	fprintf(stderr, "[i] %s\n", opt_username);

      if (!opt_password)
        opt_password = g_key_file_get_string(kf, "Login", "Password", NULL);

      gint to = g_key_file_get_integer(kf, "Cache", "Timeout", &local_err);
      if (local_err == NULL)
        cache_timout = to;
      else
        g_clear_error(&local_err);

      // Load speed limits from settings file
      if (g_key_file_has_key(kf, "Network", "SpeedLimit", NULL))
      {
        download_seed_limit = upload_speed_limit = g_key_file_get_integer(kf, "Network", "SpeedLimit", &local_err);
        if (local_err)
        {
          g_printerr("WARNING: Invalid speed limit set in the config file: %s\n", local_err->message);
          g_clear_error(&local_err);
        }
      }

      if (g_key_file_has_key(kf, "Network", "UploadSpeedLimit", NULL))
      {
        upload_speed_limit = g_key_file_get_integer(kf, "Network", "UploadSpeedLimit", &local_err);
        if (local_err)
        {
          g_printerr("WARNING: Invalid upload speed limit set in the config file: %s\n", local_err->message);
          g_clear_error(&local_err);
        }
      }

      if (g_key_file_has_key(kf, "Network", "DownloadSpeedLimit", NULL))
      {
        download_seed_limit = g_key_file_get_integer(kf, "Network", "DownloadSpeedLimit", &local_err);
        if (local_err)
        {
          g_printerr("WARNING: Invalid download speed limit set in the config file: %s\n", local_err->message);
          g_clear_error(&local_err);
        }
      }

      proxy = g_key_file_get_string(kf, "Network", "Proxy", NULL);
      
      if (opt_enable_previews == BOOLEAN_UNSET_BUT_TRUE)
      {
        gboolean enable = g_key_file_get_boolean(kf, "Upload", "CreatePreviews", &local_err);
        if (local_err == NULL)
          opt_enable_previews = enable;
        else
          g_clear_error(&local_err);
      }
    }
  }

  if (opt_speed_limit >= 0)
  {
    upload_speed_limit = opt_speed_limit;
    download_seed_limit = opt_speed_limit;
  }

  if (opt_proxy)
  {
    if (!strcmp(opt_proxy, "none"))
      proxy = NULL;
    else
      proxy = opt_proxy;
  }

  if (!(flags & TOOL_INIT_AUTH))
    return;

  if (!opt_username)
  {
    g_printerr("ERROR: You must specify your mega.nz username (email)\n");
    exit(1);
  }

  if (!opt_password && opt_no_ask_password)
  {
    g_printerr("ERROR: You must specify your mega.nz password\n");
    exit(1);
  }

  if (!opt_password)
    opt_password = input_password();
}

mega_session* tool_start_session(ToolSessionFlags flags)
{
  GError *local_err = NULL;
  gchar* sid = NULL;
  gboolean loaded = FALSE;

  mega_session* s = mega_session_new();

  mega_session_set_speed(s, upload_speed_limit, download_seed_limit);

  if (proxy)
    mega_session_set_proxy(s, proxy);

  mega_session_enable_previews(s, TRUE);

  if (!(flags & TOOL_SESSION_OPEN))
    return s;

  // allow unatuhenticated session      
  if (!opt_password || !opt_username)
  {
    if (flags & TOOL_SESSION_AUTH_OPTIONAL)
      return s;      
    
    g_printerr("ERROR: Authentication is required\n");
    goto err;
  }

  // try to load cached session data (they are valid for 10 minutes since last
  // user_get or refresh)
  if (!mega_session_load(s, opt_username, opt_password, cache_timout, &sid, &local_err))
  {
    g_clear_error(&local_err);

    if (!mega_session_open(s, opt_username, opt_password, sid, &local_err))
    {
      g_printerr("ERROR: Can't login to mega.nz: %s\n", local_err->message);
      goto err;
    }

    if (!(flags & TOOL_SESSION_AUTH_ONLY))
    {
      if (!mega_session_refresh(s, &local_err))
      {
        g_printerr("ERROR: Can't read filesystem info from mega.nz: %s\n", local_err->message);
        goto err;
      }

      loaded = TRUE;
    }

    mega_session_save(s, NULL);
  }

  if (!(flags & TOOL_SESSION_AUTH_ONLY) && opt_reload_files && !loaded)
  {
    if (!mega_session_refresh(s, &local_err))
    {
      g_printerr("ERROR: Can't read filesystem info from mega.nz: %s\n", local_err->message);
      goto err;
    }

    mega_session_save(s, NULL);
  }

  mega_session_enable_previews(s, !!opt_enable_previews);

  g_free(sid);
  return s;

err:
  mega_session_free(s);
  g_clear_error(&local_err);
  g_free(sid);
  return NULL;
}

void tool_fini(mega_session* s)
{
  if (s)
    mega_session_free(s);

  g_option_context_free(opt_context);
  curl_global_cleanup();
  CRYPTO_cleanup_all_ex_data();
  ERR_free_strings();

#if OPENSSL_VERSION_NUMBER < 0x10100004L
  CRYPTO_set_id_callback(NULL);
  CRYPTO_set_locking_callback(NULL);
#endif
}
