/* it's all too easy to start over-complicating this: for example quotations. Here you woul dneed to check the last 2 characters of everyword, not just the last one, i.e "stop!", that adds new layers. */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#ifdef DBG
#define CBUF 2
#define WABUF 1
#define LBUF 2
#else
#define CBUF 12
#define WABUF 20
#define LBUF 32
#endif

typedef unsigned char boole;
typedef enum
{
    STRG, /* default to string */
    NINT, /* word will all ints */
    NFLT, /* word with a ., so a float */
    NTIM /* word with timing ... i.e. : and . */
} t_t;

typedef struct /* word type */
{
    char *w;
    t_t t; /* number or not */
    unsigned lp1; /* length of word plus one (i.e. includes null char */
} w_c;

typedef struct /* aw_c: array of words container */
{
    w_c **aw;
    unsigned ab;
    unsigned al;
} aw_c;

typedef struct /* aaw_c: array of array of words container */
{
    size_t numl; /* number of lines, i.e. rows */
    aw_c **aaw; /* an array of pointers to aw_c */
} aaw_c;

/* checking each character can be comptiue-intensive, so I've offloaded off to MACROS */

/* Macro fo GET Leading Char TYPE */
/* so, this refers to the first character: "+-.0123456789" only these are allowed. These is how we infer
 * a quantity of some sort ... except for currency */
#define GETLCTYPE(c, typ); \
            if(((c) == '.') | ((c) == ':') | (((c) >= 0x30) && ((c) <= 0x39))) { \
                if((c) == '.') \
                    typ = NFLT; \
                else if((c) == ':') \
                    typ = NTIM; \
                else \
                    typ = NINT; \
            }

/* Macro for InWord MODify TYPE */
#define IWMODTYPEIF(c, typ); \
            if(((c) == '.') | ((c) == ':') | (((c) >= 0x30) && ((c) <= 0x39))) { \
                if( ((typ) == NINT) & ((c) == '.')) \
                    typ = NFLT; \
                else if( (((typ) == NINT) | ((typ) == NFLT)) & (c == ':')) \
                    typ = NTIM; \
            } else \
                    typ = STRG;
