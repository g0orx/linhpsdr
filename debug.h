extern unsigned int DEBUG;

// from public source source (source as in source code, source as in font of information)

// Our implemented function
void realdbgprintf (const char *SourceFilename,
                    int SourceLineno,
		    const char *CFormatString,
                    ...);

// Due to limitations of the variadic macro support in C++11 the following
// straightforward solution can fail and should thus be avoided:
//
//   #define dbgprintf(cformat, ...) \
//     realdbgprintf (__FILE__, __LINE__, cformat, __VA_ARGS__)
//
// The reason is that
//
//   dbgprintf("Hallo")
//
// gets expanded to
//
//   realdbgprintf (__FILE__, __LINE__, "Hallo", )
//
// where the comma before the closing brace will result in a syntax error.
//
// GNU C++ supports a non-portable extension which solves this.
//
//   #define dbgprintf(cformat, ...) \
//     realdbgprintf (__FILE__, __LINE__, cformat, ##__VA_ARGS__)
//
// C++20 eventually supports the following syntax.
//
//   #define dbgprintf(cformat, ...) \
//     realdbgprintf (__FILE__, __LINE__, cformat __VA_OPT__(,) __VA_ARGS__)
//
// By using the 'cformat' string as part of the variadic arguments we can
// circumvent the abovementioned incompatibilities.  This is tricky but
// portable.
#define dbgprintf(...) realdbgprintf (__FILE__, __LINE__, __VA_ARGS__)
