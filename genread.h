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
    STR, /* unknown type, so default to string */
    NUM, /* number: float or int, but treat as float, an int requires more information */
    PNI /* pos or beg int */
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

#define GETTYPE(c, typ); \
            if(((c) == 0x2B) | ((c) == 0x2D) | ((c) == 0x2E) | (((c) >= 0x30) && ((c) <= 0x39))) { \
                if( ((c) == 0x2B) | ((c) == 0x2D) | (((c) >= 0x30) && ((c) <= 0x39))) \
                    typ = PNI; \
                else \
                    typ = NUM; \
            }

#define MODTYPEIF(c, typ); \
            if( ((typ) == NUM) & (((c) != 0x2E) & (((c) < 0x30) || ((c) > 0x39)))) \
                typ = STR; \
            else if( ((typ) == PNI) & (c == 0x2E)) \
                typ = NUM; \
            else if( ((typ) == PNI) & ((c < 0x30) || (c > 0x39)) ) \
                typ = STR;
