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
    STRG, /* unknown type, so default to string */
    NUMS, /* NUMberS: but does not include currency. Date, time, float or int, i.e. just a sequence of numbers with maybe some special symbils.*/
    PNI, /* pos or neg int */
    STPP, /* string with closing period symbol */
    STCP, /* string with closing punctuation attached.. a comma, or a full stop, semicolon, !? etc */
    SCST, /* string with starting capital */
    SCCP, /* string with starting capital AND closing punctuation */
    ALLC /* string with all caps */
} t_t;

typedef struct /* time a group of ints */
{
    int hrs, mins;
    float secs;
} timetype;

typedef struct /* word type */
{
    char *w;
    t_t t; /* number or not */
    unsigned lp1; /* length of word plus one (i.e. includes null char */
} w_c;

typedef struct /* aw_c: array of words container: essentially, a line of words */
{
    w_c **aw;
    unsigned ab;
    unsigned al;
    short stsps; /* number of starting spaces */
    short sttbs; /* number of starting tabs */
} aw_c;

typedef struct /* aaw_c: array of array of words container */
{
    size_t numl; /* number of lines, i.e. rows */
    aw_c **aaw; /* an array of pointers to aw_c */
    int *ppa; /* paragraph point array: will record lines which have double newlines */
    int ppb, ppsz; /* buffer and size for our ppa */
} aaw_c;

/* checking each character can be compute-intensive, so I've offloaded off to MACROS, see below */

/* Macro fo GET Leading Char TYPE */
/* so, this refers to the first character: "+-.0123456789" only these are allowed. These is how we infer
 * a quantity of some sort ... except for currency */
#define GETLCTYPE(c, typ); \
            if(((c) == 0x2B) | ((c) == 0x2D) | ((c) == 0x2E) | (((c) >= 0x30) && ((c) <= 0x39))) { \
                if( ((c) == 0x2B) | ((c) == 0x2D) | (((c) >= 0x30) && ((c) <= 0x39))) \
                    typ = PNI; \
                else \
                    typ = NUMS; \
            } else if(((c) >= 0x41) && ((c) <= 0x5A)) \
                    typ = SCST;

/* Macro for InWord MODify TYPE */
#define IWMODTYPEIF(c, typ); \
            if( ((typ) == NUMS) & (((c) != 0x2E) & (((c) < 0x30) || ((c) > 0x39)))) \
                typ = STRG; \
            else if( ((typ) == PNI) & (c == 0x2E)) \
                typ = NUMS; \
            else if( ((typ) == PNI) & ((c < 0x30) || (c > 0x39)) ) \
                typ = STRG;

/* Macro for SETting CLosing Punctuation TYPE, based on oldc (oc) not c-var */
/* 21=! 29=) 2C=, 2E=. 3B=; 3F=? 5D=] 7D=}*/
#define SETCPTYPE(oc, typ); \
            if( ((oc)==0x21)|((oc)==0x29)|((oc)==0x2C)|((oc)==0x2E)|((oc)==0x3B)|((oc)==0x3F)|((oc)==0x5D)|((oc)==0x7D) ) { \
                if( ((oc)==0x2E) & ((typ) == STRG) ) \
                    typ = STPP; \
                else if((typ) == STRG) \
                    typ = STCP; \
                else if((typ) == SCST) \
                    typ = SCCP; \
            }

#define CONDREALLOC(x, b, c, a, t); \
    if((x)>=((b)-0)) { \
        (b) += (c); \
        (a)=realloc((a), (b)*sizeof(t)); \
    }
