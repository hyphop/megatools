#include "tools.h"

static GOptionEntry entries[] =
{
  { NULL }
};

#include "main.h"

#define CP "1"
#define DF "1"
#define DL "1"
#define GET "1"
#define LS "1"
#define MK "1"
#define PUT "1"
#define REG "1"
#define RM "1"

#include "copy.c"
#include "df.c"
#include "dl.c"
#include "get.c"
#include "mkdir.c"
#include "ls.c"
#include "put.c"
#include "reg.c"
#include "rm.c"

#include "main2.h"

