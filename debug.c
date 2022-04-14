#include <stdio.h>
#include <stdarg.h>

unsigned int DEBUG = 1; // incremented by each "-d|--debug" on command line

// from public source source (source as in source code, source as in font of information)

// Our implemented function
void realdbgprintf (const char *SourceFilename,
                    int SourceLineno,
		    const char *CFormatString,
                    ...) {
    va_list ap;

    (void) fprintf( stderr, "%s(%u): ", SourceFilename, SourceLineno ); // output what is not variadic

    va_start(ap, CFormatString); // init ap to location of variadic arg(s)
    (void) vfprintf( stderr, CFormatString, ap );
}
