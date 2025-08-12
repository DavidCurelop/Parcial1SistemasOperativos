#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/resource.h>

#ifndef USE_POINTERS
#define USE_POINTERS 0
#endif

typedef long long i64;

typedef struct { int y,m,d; } Date;
typedef struct { char* s; } Str;

static char* xstrdup(const char* s){
    size_t n=strlen(s)+1;
    char* p=(char*)malloc(n);
    if(p) memcpy(p,s,n);
    return p;
}

static char* trim(char* s){
    while(isspace((unsigned char)*s)) s++;
    if(*s==0) return s;
    char* e=s+strlen(s)-1;
    while(e>s && isspace((unsigned char)*e)) *e--=0;
    return s;
}

static int parse_date(const char* in, Date* out){
    char buf[64]; strncpy(buf,in,sizeof(buf)-1); buf[sizeof(buf)-1]=0;
    for(char* p=buf; *p; ++p) if(*p=='/') *p='-';
    int a=-1,b=-1,c=-1;
    if(sscanf(buf,"%d-%d-%d",&a,&b,&c)!=3) return 0;
    if(a>1900 && a<2100){ out->y=a; out->m=b; out->d=c; }
    else { out->d=a; out->m=b; out->y=c; }
    return 1;
}

static int ymd_key(const Date* d){ return d->y*10000 + d->m*100 + d->d; }

static int last2_digits(const char* s){
    int d1=-1,d2=-1;
    for(const char* p=s; *p; ++p){
        if(isdigit((unsigned char)*p)){ d1=d2; d2=*p-'0'; }
    }
    if(d2<0) return 0;
    if(d1<0) return d2;
    return d1*10+d2;
}

static char grupo_from_last2(int last2){
    if(last2<=39) return 'A';
    if(last2<=79) return 'B';
    return 'C';
}

// Very simple dynamic array of unique strings (pool) for pointer mode
typedef struct Pool {
    char** data; size_t n, cap;
} Pool;
static void pool_init(Pool* p){ p->data=NULL; p->n=0; p->cap=0; }
static const char* pool_intern(Pool* p, const char* s){
    for(size_t i=0;i<p->n;++i) if(strcmp(p->data[i], s)==0) return p->data[i];
    if(p->n==p->cap){
        size_t nc = p->cap? p->cap*2 : 1024;
        char** nd = (char**)realloc(p->data, nc*sizeof(char*));
        if(!nd) return s;
        p->data=nd; p->cap=nc;
    }
    p->data[p->n++] = xstrdup(s);
    return p->data[p->n-1];
}

typedef struct {
#if USE_POINTERS
    const char* nombre;
    const char* ciudad;
    const char* documento;
#else
    char* nombre;
    char* ciudad;
    char* documento;
#endif
    Date nacimiento;
    i64 patrimonio;
    i64 deudas;
    char grupo;
} Person;

typedef struct {
    char nombre[256];
    char documento[128];
    char ciudad[128];
    long long value;
    Date nacimiento;
    int set;
} BestBy;

static void best_age_update(BestBy* b, const Person* p){
    long long v = -(long long)ymd_key(&p->nacimiento);
    if(!b->set || v>b->value){
        b->value=v; b->set=1;
        strncpy(b->nombre, p->nombre, sizeof(b->nombre)-1);
        strncpy(b->documento, p->documento, sizeof(b->documento)-1);
        strncpy(b->ciudad, p->ciudad, sizeof(b->ciudad)-1);
        b->nacimiento = p->nacimiento;
    }
}
static void best_net_update(BestBy* b, const Person* p){
    long long v = p->patrimonio - p->deudas;
    if(!b->set || v>b->value){
        b->value=v; b->set=1;
        strncpy(b->nombre, p->nombre, sizeof(b->nombre)-1);
        strncpy(b->documento, p->documento, sizeof(b->documento)-1);
        strncpy(b->ciudad, p->ciudad, sizeof(b->ciudad)-1);
        b->nacimiento = p->nacimiento;
    }
}

int main(int argc, char** argv){
    const char* in_path=NULL; const char* out_dir="out";
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i],"--in")==0 && i+1<argc) in_path=argv[++i];
        else if(strcmp(argv[i],"--out")==0 && i+1<argc) out_dir=argv[++i];
    }
    if(!in_path){ fprintf(stderr,"ERROR: use --in data.csv\n"); return 2; }
    FILE* f = fopen(in_path,"r");
    if(!f){ fprintf(stderr,"ERROR: cannot open %s\n", in_path); return 2; }
    // make out dir
    char cmd[512]; snprintf(cmd,sizeof(cmd),"mkdir -p \"%s\"", out_dir); system(cmd);

    char* line = NULL; size_t cap=0; ssize_t len;
    // header
    len = getline(&line,&cap,f);
    if(len<=0){ fprintf(stderr,"ERROR: empty file\n"); return 2; }

    int i_nombre=-1,i_fnac=-1,i_ciudad=-1,i_patr=-1,i_deuda=-1,i_doc=-1,i_cal=-1;
    // parse header indices (very simple split by ',')
    {
        int idx=0; char* p=line; 
        while(p && *p){
            char* q=strchr(p,',');
            if(q) *q=0;
            char* t=trim(p);
            for(char* c=t; *c; ++c) *c=(char)tolower((unsigned char)*c);
            if(strstr(t,"nombre")) i_nombre=idx;
            else if(strstr(t,"fecha")||strstr(t,"nacim")) i_fnac=idx;
            else if(strstr(t,"ciudad")||strstr(t,"residencia")) i_ciudad=idx;
            else if(strstr(t,"patrim")) i_patr=idx;
            else if(strstr(t,"deud")) i_deuda=idx;
            else if(strstr(t,"doc")||strstr(t,"ident")) i_doc=idx;
            else if(strstr(t,"calend")||strcmp(t,"grupo")==0) i_cal=idx;
            idx++;
            if(!q) break; p=q+1;
        }
    }
    if(i_nombre<0||i_fnac<0||i_ciudad<0||i_patr<0||i_deuda<0||i_doc<0){
        fprintf(stderr,"ERROR: faltan columnas requeridas\n"); return 2;
    }

    char pathA[256], pathB[256], pathC[256], pathV[256];
    snprintf(pathA,sizeof(pathA),"%s/grupo_A.csv",out_dir);
    snprintf(pathB,sizeof(pathB),"%s/grupo_B.csv",out_dir);
    snprintf(pathC,sizeof(pathC),"%s/grupo_C.csv",out_dir);
    snprintf(pathV,sizeof(pathV),"%s/validaciones.csv",out_dir);
    FILE* fA=fopen(pathA,"w"); FILE* fB=fopen(pathB,"w"); FILE* fC=fopen(pathC,"w"); FILE* fVal=fopen(pathV,"w");
    fprintf(fA,"%s", line); fprintf(fB,"%s", line); fprintf(fC,"%s", line);
    fprintf(fVal,"linea,doc_last2,grupo_calculado,grupo_input\n");

    BestBy oldest_country={0}, richest_country={0};
    BestBy richest_group[3]={{0},{0},{0}};
    long long count_group[3]={0,0,0};

#if USE_POINTERS
    Pool pool; pool_init(&pool);
#endif

    struct timespec t0,t1; clock_gettime(CLOCK_MONOTONIC,&t0);

    long long line_no=1;
    while((len=getline(&line,&cap,f))>0){
        line_no++;
        // naive split, enough for generator CSV
        int col=0; char* p=line;
        char* cols[64]={0}; int ncols=0;
        while(p && *p){
            char* q=strchr(p,',');
            if(q) *q=0;
            cols[ncols++] = trim(p);
            if(!q) break; p=q+1;
        }
        if(ncols<=i_doc) continue;

        const char* s_nombre = cols[i_nombre];
        const char* s_fnac   = cols[i_fnac];
        const char* s_ciudad = cols[i_ciudad];
        const char* s_patr   = cols[i_patr];
        const char* s_deuda  = cols[i_deuda];
        const char* s_doc    = cols[i_doc];
        const char* s_cal    = (i_cal>=0? cols[i_cal] : "");

        Person pz;
#if USE_POINTERS
        pz.nombre = pool_intern(&pool, s_nombre);
        pz.ciudad = pool_intern(&pool, s_ciudad);
        pz.documento = pool_intern(&pool, s_doc);
#else
        pz.nombre = xstrdup(s_nombre);
        pz.ciudad = xstrdup(s_ciudad);
        pz.documento = xstrdup(s_doc);
#endif
        if(!parse_date(s_fnac, &pz.nacimiento)) continue;
        pz.patrimonio = (i64) llround(strtold(s_patr,NULL));
        pz.deudas     = (i64) llround(strtold(s_deuda,NULL));
        int last2 = last2_digits(s_doc);
        char g = grupo_from_last2(last2);
        pz.grupo = g;

        if(s_cal && *s_cal){
            char gin = toupper((unsigned char)s_cal[0]);
            if(gin!='A' && gin!='B' && gin!='C'){
                if(strstr(s_cal,"A")||strstr(s_cal,"a")) gin='A';
                else if(strstr(s_cal,"B")||strstr(s_cal,"b")) gin='B';
                else if(strstr(s_cal,"C")||strstr(s_cal,"c")) gin='C';
            }
            if(gin=='A'||gin=='B'||gin=='C'){
                if(gin!=g){
                    fprintf(fVal,"%lld,%02d,%c,%c\n", line_no, last2, g, gin);
                }
            }
        }

        if(g=='A') fprintf(fA,"%s", line);
        else if(g=='B') fprintf(fB,"%s", line);
        else fprintf(fC,"%s", line);

        int gi = (g=='A'?0:(g=='B'?1:2));
        count_group[gi]++;

        best_age_update(&oldest_country, &pz);
        best_net_update(&richest_country, &pz);
        best_net_update(&richest_group[gi], &pz);
    }

    clock_gettime(CLOCK_MONOTONIC,&t1);
    double secs = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec)/1e9;

    struct rusage ru; getrusage(RUSAGE_SELF,&ru);
    long vmrss=-1, vmhwm=-1;
    FILE* fs = fopen("/proc/self/status","r");
    if(fs){
        char b[512];
        while(fgets(b,sizeof(b),fs)){
            long v;
            if(sscanf(b,"VmRSS: %ld kB",&v)==1) vmrss=v;
            if(sscanf(b,"VmHWM: %ld kB",&v)==1) vmhwm=v;
        }
        fclose(fs);
    }

    printf("OK tiempo=%.6f s VmRSS=%ld kB VmHWM=%ld kB MaxRSS=%ld kB\n",
           secs, vmrss, vmhwm,
#ifdef __APPLE__
           ru.ru_maxrss/1024
#else
           ru.ru_maxrss
#endif
    );

    return 0;
}
