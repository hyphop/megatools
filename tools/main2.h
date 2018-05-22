//
// ## hyphop ## //
//

#include <string.h>

#include "main.h"


#if !defined MAIN

#define MEGA "mega"

int main(int ac, char* av[])
{

    int i;
    i = strcspn (av[0],MEGA);
    i+= strlen(MEGA);
    char * name = av[0]+i;

    fprintf(stderr, "[i] MEGA.%s\n", name );

    if        ( strcmp( name, "copy" ) == 0 ) {
#if defined CP
	main_copy(ac,av);
#endif
    } else if ( strcmp( name, "df" ) == 0 ) {
#if defined DF
	main_df(ac,av);
#endif
    } else if ( strcmp( name, "get" ) == 0 ) {
#if defined GET
	main_get(ac,av);
#endif
    } else if ( strcmp( name, "ls" ) == 0 ) {
#if defined LS
	main_ls(ac,av);
#endif
    } else if ( strcmp( name, "mkdir" ) == 0 ) {
#if defined MK
	main_mkdir(ac,av);
#endif
    } else if ( strcmp( name, "put" ) == 0 ) {
#if defined PUT
	main_put(ac,av);
#endif
    } else if ( strcmp( name, "reg" ) == 0 ) {
#if defined REG
	main_reg(ac,av);
#endif
    } else if ( strcmp( name, "rm" ) == 0 ) {
#if defined RM
	main_rm(ac,av);
#endif
    } else if ( strcmp( name, "dl" ) == 0 ) {
#if defined DL
	main_dl(ac,av);
#endif
    }
    else {
//	main_copy(ac,av);	
	if (ac > 1) {
	name = av[1];
	} else {
        fprintf(stderr,"[i] USAGE: %s [ ls | dl | get | rm | mkdir | copy | reg | df ]\n" , av[0]);
	    exit(1);
	}
	ac--;
	av++;
        fprintf(stderr,"[i] %s\n", name);
	
    if        ( strcmp( name, "copy" ) == 0 ) {
#if defined CP
	main_copy(ac,av);
#endif
    } else if ( strcmp( name, "df" ) == 0 ) {
#if defined DF
	main_df(ac,av);
#endif
    } else if ( strcmp( name, "get" ) == 0 ) {
#if defined GET
	main_get(ac,av);
#endif
    } else if ( strcmp( name, "ls" ) == 0 ) {
#if defined LS
	main_ls(ac,av);
#endif
    } else if ( strcmp( name, "mkdir" ) == 0 ) {
#if defined MK
	main_mkdir(ac,av);
#endif
    } else if ( strcmp( name, "put" ) == 0 ) {
#if defined PUT
	main_put(ac,av);
#endif
    } else if ( strcmp( name, "reg" ) == 0 ) {
#if defined REG
	main_reg(ac,av);
#endif
    } else if ( strcmp( name, "rm" ) == 0 ) {
#if defined RM
	main_rm(ac,av);
#endif
    } else if ( strcmp( name, "dl" ) == 0 ) {
#if defined DL
	main_dl(ac,av);
#endif
    } else {
        fprintf(stderr,"[i] %s\n", "UNDEF");
	
    }
    }
    return 0;
}

#define MAIN "1"

#endif