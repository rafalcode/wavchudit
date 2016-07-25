/* modification of matread but operating on words instead of floats */
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "routim.h"

w_c *crea_wc(unsigned initsz)
{
    w_c *wc=malloc(sizeof(w_c));
    wc->lp1=initsz;
    wc->t=STRG;
    wc->w=malloc(wc->lp1*sizeof(char));
    return wc;
}

void reall_wc(w_c **wc, unsigned *cbuf)
{
    w_c *twc=*wc;
    unsigned tcbuf=*cbuf;
    tcbuf += CBUF;
    twc->lp1=tcbuf;
    twc->w=realloc(twc->w, tcbuf*sizeof(char));
    *wc=twc; /* realloc can often change the ptr */
    *cbuf=tcbuf;
    return;
}

void norm_wc(w_c **wc)
{
    w_c *twc=*wc;
    twc->w=realloc(twc->w, twc->lp1*sizeof(char));
    *wc=twc; /* realloc can often change the ptr */
    return;
}

void free_wc(w_c **wc)
{
    w_c *twc=*wc;
    free(twc->w);
    free(twc);
    return;
}

aw_c *crea_awc(unsigned initsz)
{
    int i;
    aw_c *awc=malloc(sizeof(aw_c));
    awc->ab=initsz;
    awc->al=awc->ab;
    awc->stsps=0;
    awc->sttbs=0;
    awc->aw=malloc(awc->ab*sizeof(w_c*));
    for(i=0;i<awc->ab;++i) 
        awc->aw[i]=crea_wc(CBUF);
    return awc;
}

void reall_awc(aw_c **awc, unsigned buf)
{
    int i;
    aw_c *tawc=*awc;
    tawc->ab += buf;
    tawc->al=tawc->ab;
    tawc->stsps=0;
    tawc->sttbs=0;
    tawc->aw=realloc(tawc->aw, tawc->ab*sizeof(aw_c*));
    for(i=tawc->ab-buf;i<tawc->ab;++i)
        tawc->aw[i]=crea_wc(CBUF);
    *awc=tawc;
    return;
}

void norm_awc(aw_c **awc)
{
    int i;
    aw_c *tawc=*awc;
    /* free the individual w_c's */
    for(i=tawc->al;i<tawc->ab;++i) 
        free_wc(tawc->aw+i);
    /* now release the pointers to those freed w_c's */
    tawc->aw=realloc(tawc->aw, tawc->al*sizeof(aw_c*));
    *awc=tawc;
    return;
}

void free_awc(aw_c **awc)
{
    int i;
    aw_c *tawc=*awc;
    for(i=0;i<tawc->al;++i) 
        free_wc(tawc->aw+i);
    free(tawc->aw); /* unbelieveable: I left this out, couldn't find where I leaking the memory! */
    free(tawc);
    return;
}

aaw_c *crea_aawc(unsigned initsz)
{
    int i;
    unsigned lbuf=initsz;
    aaw_c *aawc=malloc(sizeof(aaw_c));
    aawc->numl=0;
    aawc->aaw=malloc(lbuf*sizeof(aw_c*));
    for(i=0;i<initsz;++i) 
        aawc->aaw[i]=crea_awc(WABUF);
    /* ppa, this guys ia bit independent: will use CBUF as buffer increaser */
    aawc->ppb=CBUF;
    aawc->ppsz=0;
    aawc->ppa=malloc(CBUF*sizeof(int));
    return aawc;
}

void free_aawc(aaw_c **aw)
{
    int i;
    aaw_c *taw=*aw;
    for(i=0;i<taw->numl;++i) /* tried to release 1 more, no go */
        free_awc(taw->aaw+i);
    free(taw->ppa);
    free(taw->aaw);
    free(taw);
}

void prtaawapap(aaw_c *aawc) /* print aaw As Pure As Possible */
{
    int i, j, k, ppi=0;
    for(i=0;i<aawc->numl;++i) {
        // printf("ln%dsp%dtb%d) ", i, aawc->aaw[i]->stsps, aawc->aaw[i]->sttbs);
        /* order fo tabs and space will be messed up sure, but usually it will be one or the other. */
        for(j=0; j<aawc->aaw[i]->stsps;j++)
            putchar(' ');
        for(j=0; j<aawc->aaw[j]->sttbs;j++)
            putchar('\t');
        for(j=0;j<aawc->aaw[i]->al;++j) {
            for(k=0;k<aawc->aaw[i]->aw[j]->lp1-1; k++)
                putchar(aawc->aaw[i]->aw[j]->w[k]);
            if(j==aawc->aaw[i]->al-1)
                putchar('\n');
            else
                putchar(' ');
        }

        if( (ppi< aawc->ppsz) && (i == aawc->ppa[ppi])) { /* && means it will not evaluate second, if first is neg. */
#ifdef DBG
            putchar('\n');
            printf("%d ", ppi); 
#endif
            ppi++;
        }
    }
}

void prtaawcdbg(aaw_c *aawc)
{
    int i, j, k;
    for(i=0;i<aawc->numl;++i) {
        printf("l.%u(%u): ", i, aawc->aaw[i]->al); 
        for(j=0;j<aawc->aaw[i]->al;++j) {
            printf("w_%u: ", j); 
            if(aawc->aaw[i]->aw[j]->t == NUMS) {
                printf("NUM! "); 
                continue;
            } else if(aawc->aaw[i]->aw[j]->t == PNI) {
                printf("PNI! "); 
                continue;
            } else if(aawc->aaw[i]->aw[j]->t == STCP) {
                printf("STCP! "); 
                continue;
            }
            for(k=0;k<aawc->aaw[i]->aw[j]->lp1-1; k++)
                putchar(aawc->aaw[i]->aw[j]->w[k]);
            printf("/%u ", aawc->aaw[i]->aw[j]->lp1-1); 
        }
        printf("\n"); 
    }
}

void prtaawcdata(aaw_c *aawc) /* print line and word details, but not the words themselves */
{
    int i, j;
    for(i=0;i<aawc->numl;++i) {
        printf("L%u(%uw):", i, aawc->aaw[i]->al); 
        for(j=0;j<aawc->aaw[i]->al;++j) {
            printf("l%ut", aawc->aaw[i]->aw[j]->lp1-1);
            switch(aawc->aaw[i]->aw[j]->t) {
                case NUMS: printf("N "); break;
                case PNI: printf("I "); break;
                case STRG: printf("S "); break; /* basic string */
                case STPP: printf("P "); break; /* word is a string and ends with a period */
                case STCP: printf("C "); break; /* closing punctuation */
                case SCST: printf("Z "); break; /* starting capital */
                case SCCP: printf("Y "); break; /* starting capital and closing punctuation */
                case ALLC: printf("A "); break; /* horrid! all capitals */
            }
        }
    }
    printf("\n"); 
}

void printttype(timetype *ttypes, size_t sz)
{
    int i;
    for(i=0;i<sz;++i) 
        printf("%02dh %02dm %02.2fs\n", ttypes[i].hrs, ttypes[i].mins, ttypes[i].secs);

    return;

}

void ttypeascue(char *fname, char *audext, timetype *ttypes, size_t sz)
{
    int i, s, ss, hm;
    char *xtptr=strrchr(fname, '.');
    char xt[4]={0};

    if(!strcmp("m4a", audext))
        strcpy(xt, "M4A");
    else if(!strcmp("M4A", audext))
        strcpy(xt, "M4A");
    else if(!strcmp("MP3", audext))
        strcpy(xt, "MP3");
    else if(!strcmp("mp3", audext))
        strcpy(xt, "MP3");
    else if(!strcmp("wav", audext))
        strcpy(xt, "WAV");
    else if(!strcmp("WAV", audext))
        strcpy(xt, "WAV");
    else if(!strcmp("OGG", audext))
        strcpy(xt, "OGG");
    else if(!strcmp("ogg", audext))
        strcpy(xt, "OGG");
    else {
        printf("Error. Invalid audio extension.\n");
        exit(EXIT_FAILURE);
    }


    printf("TITLE \"%.*s\"\n", (int)(xtptr-fname), fname);
    printf("FILE \"%.*s.%s\" %s\n", (int)(xtptr-fname), fname, audext, xt);
    if( ttypes[0].hrs | ttypes[0].mins | (ttypes[0].secs >= 0.01) )
        printf("\tTRACK 01 AUDIO\n\t\tINDEX 01 00:00:00\n"); 

    for(i=0;i<sz;++i) {
        printf("\tTRACK %02d AUDIO\n", i+2);
        s=(int)ttypes[i].secs;
        ss=(int)(ttypes[i].secs*100-s*100);
        hm= ttypes[i].hrs*60 + ttypes[i].mins;
        printf("\t\tINDEX 01 %02d:%02d:%02d\n", hm, s, ss);
    }

    return;
}

void ttypeasedl(timetype *ttypes, size_t sz)
{
    int i;
    float hm1, hm2;

    for(i=0;i<sz;i+=2) {
        hm1= ttypes[i].hrs*3600 + ttypes[i].mins*60 + ttypes[i].secs;
        hm2= ttypes[i+1].hrs*3600 + ttypes[i+1].mins*60 + ttypes[i+1].secs;
        printf("%2.6f %2.6f %d\n", hm1, hm2, 0); // note the third token zero idiosyncracy of edl
    }

    return;
}

aaw_c *processinpf(char *fname)
{
    /* declarations */
    FILE *fp=fopen(fname,"r");
    int i;
    size_t couc /*count chars per line */, couw=0 /* count words */;
    int c, oldc='\0', ooldc='\0';
    boole inword=0;
    boole linestart=1; /* want to catch number of stating spaces or tabs */
    unsigned lbuf=LBUF /* buffer for number of lines */, cbuf=CBUF /* char buffer for size of w_c's: reused for every word */;
    aaw_c *aawc=crea_aawc(lbuf); /* array of words per line */

    while( (c=fgetc(fp)) != EOF) {
        if( (c== '\n') | (c == ' ') | (c== '\t') | (c == ':') ) {
            if( inword==1) { /* cue word-ending procedure */
                aawc->aaw[aawc->numl]->aw[couw]->w[couc++]='\0';
                aawc->aaw[aawc->numl]->aw[couw]->lp1=couc;
                SETCPTYPE(oldc, aawc->aaw[aawc->numl]->aw[couw]->t);
                norm_wc(aawc->aaw[aawc->numl]->aw+couw);
                couw++; /* verified: this has to be here */
            }
            if(c=='\n') { /* cue line-ending procedure */
                if(aawc->numl ==lbuf-1) {
                    lbuf += LBUF;
                    aawc->aaw=realloc(aawc->aaw, lbuf*sizeof(aw_c*));
                    for(i=lbuf-LBUF; i<lbuf; ++i)
                        aawc->aaw[i]=crea_awc(WABUF);
                }
                aawc->aaw[aawc->numl]->al=couw;
                norm_awc(aawc->aaw+aawc->numl);
                if(oldc=='\n') {
                    CONDREALLOC(aawc->ppsz, aawc->ppb, CBUF, aawc->ppa, int);
                    aawc->ppa[aawc->ppsz++]=aawc->numl;
                }
                aawc->numl++;
                couw=0;
                linestart=1;
            } else if (linestart) /* must be a space or a tab */
                (c == ' ')? aawc->aaw[aawc->numl]->stsps++ : aawc->aaw[aawc->numl]->sttbs++;
            inword=0;
        } else if(inword==0) { /* a normal character opens word */
            if(couw ==aawc->aaw[aawc->numl]->ab-1) /* new word opening */
                reall_awc(aawc->aaw+aawc->numl, WABUF);
            couc=0;
            cbuf=CBUF;
            aawc->aaw[aawc->numl]->aw[couw]->w[couc++]=c;
            GETLCTYPE(c, aawc->aaw[aawc->numl]->aw[couw]->t); /* MACRO: the firt character gives a clue */
            inword=1;
            linestart=0;
        } else if(inword) { /* simply store */
            if(couc == cbuf-1)
                reall_wc(aawc->aaw[aawc->numl]->aw+couw, &cbuf);
            aawc->aaw[aawc->numl]->aw[couw]->w[couc++]=c;
            /* if word is a candidate for a NUM or PNI (i.e. via its first character), make sure it continues to obey rules: a MACRO */
            IWMODTYPEIF(c, aawc->aaw[aawc->numl]->aw[couw]->t);
        }
        ooldc=oldc;
        oldc=c;
    } /* end of big for statement */
    fclose(fp);

    /* normalization stage */
    for(i=aawc->numl; i<lbuf; ++i) {
        free_awc(aawc->aaw+i);
    }
    aawc->aaw=realloc(aawc->aaw, aawc->numl*sizeof(aw_c*));
    aawc->ppa=realloc(aawc->ppa, aawc->ppsz*sizeof(int));

    return aawc;
}

int main(int argc, char *argv[])
{
    /* argument accounting */
    if(argc!=4) {
        printf("Error. Pls supply argument (name of text file) and the audio extension (mp3, m4a, wav) of the file it refers to. and finally whether cue or edl output\n");
        exit(EXIT_FAILURE);
    }
#ifdef DBG2
    printf("typeszs: aaw_c: %zu aw_c: %zu w_c: %zu\n", sizeof(aaw_c), sizeof(aw_c), sizeof(w_c));
#endif

    aaw_c *aawc=processinpf(argv[1]);

    int i;
    timetype *ttypes=calloc(aawc->numl, sizeof(timetype));
    for(i=0;i<aawc->numl;++i) {
        switch (aawc->aaw[i]->al) {
            case 1:
                ttypes[i].hrs=0;
                ttypes[i].mins=0;
                ttypes[i].secs=atof(aawc->aaw[i]->aw[0]->w);
                break;
            case 2:
                ttypes[i].hrs=0;
                ttypes[i].mins=atoi(aawc->aaw[i]->aw[0]->w);
                ttypes[i].secs=atof(aawc->aaw[i]->aw[1]->w);
                break;
            case 3:
                ttypes[i].hrs=atoi(aawc->aaw[i]->aw[0]->w);
                ttypes[i].mins=atoi(aawc->aaw[i]->aw[1]->w);
                ttypes[i].secs=atof(aawc->aaw[i]->aw[2]->w);
                break;
        }
    }
    if(!strcmp("cue", argv[3]))
        ttypeascue(argv[1], argv[2], ttypes, aawc->numl);
    else if(!strcmp("edl", argv[3]))
        ttypeasedl(ttypes, aawc->numl);
    else {
        printf("Error. Invalid output format: choose between cue or edl.\n");
        exit(EXIT_FAILURE);
    }
    free(ttypes);

#ifdef DBG2
    prtaawcdata(aawc);
#elif DBG
    prtaawcdbg(aawc);
    printf("Numlines: %zu\n", aawc->numl); 
    printf("Numparas: %d\n", aawc->ppsz); 
#endif

    free_aawc(&aawc);

    return 0;
}
