/* mwhashminer.c — first-party hash-name miner for NFS:MW RE.
 *
 * Reads candidate strings (one per line, optionally "hexoffset<TAB>string" as
 * produced by `strings -a -t x`) from stdin, computes the engine hashes with
 * OUR verified implementations, and prints any candidate whose hash matches a
 * target. No external dictionaries — candidates come from retail game bytes.
 *
 *   lookup2  : Bob Jenkins 1996, seed 0xABCDEF00 (vault/AttribStringKey hash;
 *              verified vs speed.exe 0x5CC090 and inline const 0xEEC2271A)
 *   binhash  : h=0xFFFFFFFF; h=h*33+c (asset-path hash; verified vs 0x460BF0)
 *
 * Usage: strings -a -t x -n 4 FILE | ./mwhashminer targets.txt FILELABEL
 *   targets.txt: one hex hash per line (0x... or bare hex)
 * Output: TSV  hash<TAB>matched_variant<TAB>algorithm<TAB>file<TAB>offset
 *
 * Variants tried per token: as-is, lowercase, UPPERCASE, Firstcap.
 * Also each whitespace-free token of the line, and the whole line.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>

#define MAXT 65536
static uint32_t targets[MAXT];
static int ntargets = 0;

/* simple open-addressing set */
#define HSIZE (1<<18)
static uint32_t hset[HSIZE];
static uint8_t  hused[HSIZE];

static void hadd(uint32_t v){
    uint32_t i = (v * 2654435761u) & (HSIZE-1);
    while (hused[i]) { if (hset[i]==v) return; i = (i+1)&(HSIZE-1); }
    hused[i]=1; hset[i]=v;
}
static int hhas(uint32_t v){
    uint32_t i = (v * 2654435761u) & (HSIZE-1);
    while (hused[i]) { if (hset[i]==v) return 1; i=(i+1)&(HSIZE-1); }
    return 0;
}

/* ---- lookup2, seed 0xABCDEF00 ---- */
#define MIX(a,b,c) { \
  a-=b; a-=c; a^=(c>>13); \
  b-=c; b-=a; b^=(a<<8);  \
  c-=a; c-=b; c^=(b>>13); \
  a-=b; a-=c; a^=(c>>12); \
  b-=c; b-=a; b^=(a<<16); \
  c-=a; c-=b; c^=(b>>5);  \
  a-=b; a-=c; a^=(c>>3);  \
  b-=c; b-=a; b^=(a<<10); \
  c-=a; c-=b; c^=(b>>15); }

static uint32_t lookup2(const unsigned char *k, size_t length){
    uint32_t a=0x9E3779B9u, b=0x9E3779B9u, c=0xABCDEF00u;
    size_t len=length;
    while (len>=12){
        a+= (k[0]|((uint32_t)k[1]<<8)|((uint32_t)k[2]<<16)|((uint32_t)k[3]<<24));
        b+= (k[4]|((uint32_t)k[5]<<8)|((uint32_t)k[6]<<16)|((uint32_t)k[7]<<24));
        c+= (k[8]|((uint32_t)k[9]<<8)|((uint32_t)k[10]<<16)|((uint32_t)k[11]<<24));
        MIX(a,b,c); k+=12; len-=12;
    }
    c+=(uint32_t)length;
    switch(len){
        case 11: c+=((uint32_t)k[10]<<24); /* fall through */
        case 10: c+=((uint32_t)k[9]<<16);
        case 9:  c+=((uint32_t)k[8]<<8);
        case 8:  b+=((uint32_t)k[7]<<24);
        case 7:  b+=((uint32_t)k[6]<<16);
        case 6:  b+=((uint32_t)k[5]<<8);
        case 5:  b+=k[4];
        case 4:  a+=((uint32_t)k[3]<<24);
        case 3:  a+=((uint32_t)k[2]<<16);
        case 2:  a+=((uint32_t)k[1]<<8);
        case 1:  a+=k[0];
    }
    MIX(a,b,c);
    return c;
}

static uint32_t binhash(const unsigned char *k){
    uint32_t h=0xFFFFFFFFu;
    for (; *k; k++) h = h*33u + *k;
    return h;
}

static const char *g_file = "?";
static char g_off[32] = "-";

static void try_str(const char *s){
    size_t n = strlen(s);
    if (n<2 || n>96) return;
    uint32_t h = lookup2((const unsigned char*)s, n);
    if (hhas(h)) printf("0x%08X\t%s\tlookup2\t%s\t%s\n", h, s, g_file, g_off);
    h = binhash((const unsigned char*)s);
    if (hhas(h)) printf("0x%08X\t%s\tbinhash\t%s\t%s\n", h, s, g_file, g_off);
}

static void try_variants(const char *tok){
    char buf[128];
    size_t n = strlen(tok);
    if (n<2 || n>96) return;
    try_str(tok);
    /* lowercase */
    size_t i;
    for (i=0;i<n;i++) buf[i]=(char)tolower((unsigned char)tok[i]);
    buf[n]=0; if (strcmp(buf,tok)) try_str(buf);
    /* UPPERCASE */
    for (i=0;i<n;i++) buf[i]=(char)toupper((unsigned char)tok[i]);
    buf[n]=0; if (strcmp(buf,tok)) try_str(buf);
    /* Firstcap */
    buf[0]=(char)toupper((unsigned char)tok[0]);
    for (i=1;i<n;i++) buf[i]=(char)tolower((unsigned char)tok[i]);
    buf[n]=0; if (strcmp(buf,tok)) try_str(buf);
}

int main(int argc, char **argv){
    if (argc<2){ fprintf(stderr,"usage: %s targets.txt [filelabel]\n", argv[0]); return 2; }
    FILE *tf = fopen(argv[1],"r");
    if (!tf){ perror("targets"); return 2; }
    char line[4096];
    while (fgets(line,sizeof line,tf)){
        uint32_t v = (uint32_t)strtoul(line[0]=='0'&&(line[1]=='x'||line[1]=='X')?line+2:line, NULL, 16);
        if (v){ if (ntargets<MAXT) targets[ntargets++]=v; hadd(v); }
    }
    fclose(tf);
    if (argc>2) g_file = argv[2];

    while (fgets(line,sizeof line,stdin)){
        size_t n=strlen(line);
        while (n && (line[n-1]=='\n'||line[n-1]=='\r')) line[--n]=0;
        char *s = line;
        /* strings -t x prefix: "  offset string" */
        char *sp = line;
        while (*sp==' ') sp++;
        char *tab = sp;
        while (*tab && !isspace((unsigned char)*tab)) tab++;
        int allhex = 1; char *p;
        for (p=sp; p<tab; p++) if (!isxdigit((unsigned char)*p)) { allhex=0; break; }
        if (allhex && *tab && tab>sp){
            size_t on = (size_t)(tab-sp) < 31 ? (size_t)(tab-sp) : 31;
            memcpy(g_off, sp, on); g_off[on]=0;
            s = tab+1;
        } else {
            strcpy(g_off,"-");
        }
        /* whole line */
        try_variants(s);
        /* tokens: split on non-identifier chars */
        char tok[128]; int ti=0;
        for (p=s;;p++){
            unsigned char c=(unsigned char)*p;
            if (isalnum(c)||c=='_'){
                if (ti<127) tok[ti++]=(char)c;
            } else {
                if (ti>=2){ tok[ti]=0; try_variants(tok); }
                ti=0;
                if (!c) break;
            }
        }
    }
    return 0;
}
