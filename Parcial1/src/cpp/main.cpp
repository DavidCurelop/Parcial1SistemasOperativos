#include <bits/stdc++.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>

// Compilar con/ sin -DUSE_POINTERS=1
#ifndef USE_POINTERS
#define USE_POINTERS 0
#endif

using i64 = long long;

struct MemStats {
    long vm_rss_kb = -1;
    long vm_hwm_kb = -1;
    long ru_maxrss_kb = -1;
};

static inline std::string ltrim(const std::string& s){
    size_t i=0; while(i<s.size() && std::isspace((unsigned char)s[i])) ++i; return s.substr(i);
}
static inline std::string rtrim(const std::string& s){
    size_t j=s.size(); while(j>0 && std::isspace((unsigned char)s[j-1])) --j; return s.substr(0,j);
}
static inline std::string trim(const std::string& s){ return rtrim(ltrim(s)); }
static inline std::string lower(std::string s){ for(char& c:s) c= (char)std::tolower((unsigned char)c); return s; }

struct Interner {
    std::deque<std::string> storage;
    struct Shash {
        using is_transparent = void;
        size_t operator()(std::string_view sv) const noexcept { return std::hash<std::string_view>{}(sv); }
        size_t operator()(const std::string& s) const noexcept { return std::hash<std::string>{}(s); }
    };
    struct Seq {
        using is_transparent = void;
        bool operator()(std::string_view a, std::string_view b) const noexcept { return a==b; }
        bool operator()(const std::string& a, const std::string& b) const noexcept { return a==b; }
        bool operator()(std::string_view a, const std::string& b) const noexcept { return a==b; }
        bool operator()(const std::string& a, std::string_view b) const noexcept { return a==b; }
    };
    std::unordered_map<std::string_view,const char*,Shash,Seq> map;
    const char* intern(std::string s){
        auto it = map.find(s);
        if(it!=map.end()) return it->second;
        storage.push_back(std::move(s));
        const char* p = storage.back().c_str();
        map.emplace(std::string_view(storage.back()), p);
        return p;
    }
};

struct ParsedDate { int y=0,m=0,d=0; };
static inline bool parse_date(const std::string& s, ParsedDate& out){
    // supports YYYY-MM-DD, YYYY/MM/DD, DD/MM/YYYY
    std::string t=s; for(char& c:t){ if(c=='/') c='-'; }
    std::vector<int> nums; nums.reserve(3);
    std::string cur;
    for(char c: t){
        if(c=='-'){ if(cur.empty()) return false; nums.push_back(std::stoi(cur)); cur.clear(); }
        else if(std::isdigit((unsigned char)c)) cur.push_back(c);
        else return false;
    }
    if(!cur.empty()) nums.push_back(std::stoi(cur));
    if(nums.size()!=3) return false;
    if(nums[0]>1900 && nums[0]<2100){ out.y=nums[0]; out.m=nums[1]; out.d=nums[2]; }
    else { out.d=nums[0]; out.m=nums[1]; out.y=nums[2]; }
    return true;
}

static inline int ymd_to_key(const ParsedDate& dt){
    return dt.y*10000 + dt.m*100 + dt.d;
}

static inline int compute_age_years(const ParsedDate& birth, const ParsedDate& ref){
    int age = ref.y - birth.y;
    if(ref.m < birth.m || (ref.m==birth.m && ref.d<birth.d)) age--;
    return age;
}

struct PersonValues {
    std::string nombre;
    ParsedDate   nacimiento;
    std::string  ciudad;
    i64 patrimonio=0;
    i64 deudas=0;
    std::string documento;
    char grupo='X';
    i64 neto() const { return patrimonio - deudas; }
};

struct PersonPtrs {
    const char* nombre;
    ParsedDate  nacimiento;
    const char* ciudad;
    i64 patrimonio=0;
    i64 deudas=0;
    const char* documento;
    char grupo='X';
    i64 neto() const { return patrimonio - deudas; }
};

#if USE_POINTERS
using Person = PersonPtrs;
#else
using Person = PersonValues;
#endif

struct BestBy {
    std::string nombre;
    std::string documento;
    std::string ciudad;
    i64 value = LLONG_MIN; // net worth or -date key negated
    ParsedDate nacimiento{};
};

static inline int last2_digits(const std::string& s){
    int d1=-1,d2=-1;
    for(char c: s){
        if(std::isdigit((unsigned char)c)){
            d1=d2; d2=c-'0';
        }
    }
    if(d2==-1){ return 0; }
    if(d1==-1){ return d2; }
    return (d1*10 + d2);
}
static inline char grupo_from_last2(int last2){
    if(last2<=39) return 'A';
    if(last2<=79) return 'B';
    return 'C';
}

struct CsvIndices {
    int i_nombre=-1, i_fnac=-1, i_ciudad=-1, i_patr=-1, i_deuda=-1, i_doc=-1, i_cal=-1;
};

static std::vector<std::string> split_csv(const std::string& line){
    std::vector<std::string> out;
    std::string cur; bool inq=false;
    for(size_t i=0;i<line.size();++i){
        char c=line[i];
        if(c=='"'){ inq=!inq; }
        else if(c==',' && !inq){ out.push_back(cur); cur.clear(); }
        else cur.push_back(c);
    }
    out.push_back(cur);
    for(auto& s: out) s=trim(s);
    return out;
}

static MemStats read_mem(){
    MemStats ms;
    FILE* f = fopen("/proc/self/status","r");
    if(f){
        char buf[4096];
        while(fgets(buf,sizeof(buf),f)){
            long v;
            if(sscanf(buf,"VmRSS: %ld kB",&v)==1) ms.vm_rss_kb=v;
            if(sscanf(buf,"VmHWM: %ld kB",&v)==1) ms.vm_hwm_kb=v;
        }
        fclose(f);
    }
    struct rusage ru{};
    if(getrusage(RUSAGE_SELF,&ru)==0){
#ifdef __APPLE__
        ms.ru_maxrss_kb = ru.ru_maxrss/1024;
#else
        ms.ru_maxrss_kb = ru.ru_maxrss; // already in KB on Linux
#endif
    }
    return ms;
}

struct Aggregates {
    // oldest country (min birthdate)
    bool have_oldest=false;
    BestBy oldest_country;

    // richest country
    BestBy richest_country;

    // per-city richest / oldest
    std::unordered_map<std::string, BestBy> richest_by_city;
    std::unordered_map<std::string, BestBy> oldest_by_city;

    // per-group richest
    BestBy richest_by_group[3];

    // counts by calendar
    long long count_group[3]{0,0,0};

    // additional: city avg and totals
    std::unordered_map<std::string, std::pair<long double,long long>> city_sum_count;
};

static void update_best_age(BestBy& best, const Person& p){
    // value: smaller date => older; store negative key to use max compare
    int key = ymd_to_key(p.nacimiento);
    i64 val = -(i64)key;
    if(val>best.value){
        best.value=val;
#if USE_POINTERS
        best.nombre = p.nombre;
        best.documento = p.documento;
        best.ciudad = p.ciudad;
#else
        best.nombre = p.nombre;
        best.documento = p.documento;
        best.ciudad = p.ciudad;
#endif
        best.nacimiento = p.nacimiento;
    }
}
static void update_best_net(BestBy& best, const Person& p){
    i64 v=p.neto();
    if(v>best.value){
        best.value=v;
#if USE_POINTERS
        best.nombre = p.nombre;
        best.documento = p.documento;
        best.ciudad = p.ciudad;
#else
        best.nombre = p.nombre;
        best.documento = p.documento;
        best.ciudad = p.ciudad;
#endif
        best.nacimiento = p.nacimiento;
    }
}

int main(int argc, char** argv){
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string in_path;
    std::string out_dir="out";
    int topk=10;
    for(int i=1;i<argc;++i){
        std::string a=argv[i];
        if(a=="--in" && i+1<argc) in_path=argv[++i];
        else if(a=="--out" && i+1<argc) out_dir=argv[++i];
        else if(a=="--topk" && i+1<argc) topk=std::stoi(argv[++i]);
        else if(a=="-h"||a=="--help"){
            std::cerr<<"Uso: "<<argv[0]<<" --in data.csv [--out outdir] [--topk 10]\n";
            return 0;
        }
    }
    if(in_path.empty()){
        std::cerr<<"ERROR: especifique --in data.csv\n";
        return 2;
    }
    std::filesystem::create_directories(out_dir);

    std::ifstream fin(in_path);
    if(!fin){ std::cerr<<"ERROR: no se pudo abrir "<<in_path<<"\n"; return 2; }

    std::string header;
    if(!std::getline(fin, header)){ std::cerr<<"ERROR: archivo vacío\n"; return 2; }
    auto cols = split_csv(header);
    CsvIndices idx;
    for(size_t i=0;i<cols.size();++i){
        std::string h = lower(trim(cols[i]));
        if(h.find("nombre")!=std::string::npos) idx.i_nombre=(int)i;
        else if(h.find("fecha")!=std::string::npos) idx.i_fnac=(int)i;
        else if(h.find("nacim")!=std::string::npos) idx.i_fnac=(int)i;
        else if(h.find("ciudad")!=std::string::npos || h.find("residencia")!=std::string::npos) idx.i_ciudad=(int)i;
        else if(h.find("patrim")!=std::string::npos) idx.i_patr=(int)i;
        else if(h.find("deud")!=std::string::npos) idx.i_deuda=(int)i;
        else if(h.find("doc")!=std::string::npos || h.find("ident")!=std::string::npos) idx.i_doc=(int)i;
        else if(h.find("calend")!=std::string::npos || h=="grupo") idx.i_cal=(int)i;
    }
    if(idx.i_nombre<0||idx.i_fnac<0||idx.i_ciudad<0||idx.i_patr<0||idx.i_deuda<0||idx.i_doc<0){
        std::cerr<<"ERROR: cabecera no contiene los campos requeridos\n";
        return 2;
    }

    std::ofstream fA(out_dir+"/grupo_A.csv");
    std::ofstream fB(out_dir+"/grupo_B.csv");
    std::ofstream fC(out_dir+"/grupo_C.csv");
    std::ofstream fVal(out_dir+"/validaciones.csv");
    fA<<header<<"\n"; fB<<header<<"\n"; fC<<header<<"\n";
    fVal<<"linea,doc_last2,grupo_calculado,grupo_input\n";

    Aggregates aggr;
    BestBy emptyBB; emptyBB.value = LLONG_MIN;
    aggr.richest_by_group[0]=aggr.richest_by_group[1]=aggr.richest_by_group[2]=emptyBB;

#if USE_POINTERS
    Interner intern;
#endif

    auto t0 = std::chrono::steady_clock::now();

    std::string line;
    long long line_no=1; // header was 0
    while(std::getline(fin,line)){
        ++line_no;
        if(line.empty()) continue;
        auto v = split_csv(line);
        if((int)v.size() < std::max({idx.i_nombre, idx.i_fnac, idx.i_ciudad, idx.i_patr, idx.i_deuda, idx.i_doc, idx.i_cal})+1){
            continue; // línea malformada
        }
        std::string s_nombre = v[idx.i_nombre];
        std::string s_fnac   = v[idx.i_fnac];
        std::string s_ciudad = v[idx.i_ciudad];
        std::string s_patr   = v[idx.i_patr];
        std::string s_deuda  = v[idx.i_deuda];
        std::string s_doc    = v[idx.i_doc];
        std::string s_cal    = (idx.i_cal>=0? v[idx.i_cal] : "");

        Person p{};
#if USE_POINTERS
        p.nombre = intern.intern(s_nombre);
        p.ciudad = intern.intern(s_ciudad);
        p.documento = intern.intern(s_doc);
#else
        p.nombre = s_nombre;
        p.ciudad = s_ciudad;
        p.documento = s_doc;
#endif
        ParsedDate fn{};
        if(!parse_date(s_fnac, fn)) continue;
        p.nacimiento = fn;
        p.patrimonio = (i64) std::llround(std::stold(s_patr));
        p.deudas     = (i64) std::llround(std::stold(s_deuda));
        int last2 = last2_digits(s_doc);
        char g = grupo_from_last2(last2);
        p.grupo = g;

        if(!s_cal.empty()){
            char gin = std::toupper((unsigned char)s_cal[0]);
            if(gin!='A' && gin!='B' && gin!='C'){
                // intentar interpretar como "Grupo A" etc
                auto sl = lower(s_cal);
                if(sl.find('a')!=std::string::npos) gin='A';
                else if(sl.find('b')!=std::string::npos) gin='B';
                else if(sl.find('c')!=std::string::npos) gin='C';
            }
            if(gin=='A'||gin=='B'||gin=='C'){
                if(gin!=g){
                    fVal<<line_no<<","<<std::setw(2)<<std::setfill('0')<<last2<<","<<g<<","<<gin<<"\n";
                }
            }
        }

        // write to group file (streaming)
        if(g=='A') fA<<line<<"\n";
        else if(g=='B') fB<<line<<"\n";
        else fC<<line<<"\n";

        // counts
        int gi = (g=='A'?0:(g=='B'?1:2));
        aggr.count_group[gi]++;

        // oldest country
        if(!aggr.have_oldest){
            aggr.oldest_country.value = -(i64)ymd_to_key(p.nacimiento);
            aggr.oldest_country.nombre = USE_POINTERS? std::string(p.nombre): p.nombre;
            aggr.oldest_country.documento = USE_POINTERS? std::string(p.documento): p.documento;
            aggr.oldest_country.ciudad = USE_POINTERS? std::string(p.ciudad): p.ciudad;
            aggr.oldest_country.nacimiento = p.nacimiento;
            aggr.have_oldest=true;
        } else {
            update_best_age(aggr.oldest_country, p);
        }

        // richest
        update_best_net(aggr.richest_country, p);

        // richest by city
        {
            std::string key = USE_POINTERS? std::string(p.ciudad): p.ciudad;
            auto& bb = aggr.richest_by_city[key];
            update_best_net(bb, p);
        }
        // oldest by city
        {
            std::string key = USE_POINTERS? std::string(p.ciudad): p.ciudad;
            auto& bb = aggr.oldest_by_city[key];
            update_best_age(bb, p);
        }
        // richest by group
        update_best_net(aggr.richest_by_group[gi], p);

        // additional aggregates: avg per city
        {
            std::string key = USE_POINTERS? std::string(p.ciudad): p.ciudad;
            auto& sc = aggr.city_sum_count[key];
            sc.first += (long double)p.neto();
            sc.second += 1;
        }
    }

    auto t1 = std::chrono::steady_clock::now();
    double secs = std::chrono::duration<double>(t1-t0).count();
    auto ms = read_mem();

    // Additional outputs
    // 1) ciudades con patrimonio promedio más alto
    std::vector<std::pair<std::string,long double>> city_avg;
    city_avg.reserve(aggr.city_sum_count.size());
    for(auto& kv : aggr.city_sum_count){
        auto sum = kv.second.first;
        auto cnt = kv.second.second;
        if(cnt>0) city_avg.emplace_back(kv.first, sum / (long double)cnt);
    }
    std::sort(city_avg.begin(), city_avg.end(), [](auto& a, auto& b){ return a.second>b.second; });
    {
        std::ofstream f(out_dir+"/ciudades_top_promedio.csv");
        f<<"ciudad,patrimonio_promedio\n";
        for(int i=0;i<(int)city_avg.size() && i<topk; ++i){
            f<<city_avg[i].first<<","<<std::fixed<<std::setprecision(2)<< (double)city_avg[i].second <<"\n";
        }
    }

    // 2) % >80 por calendario
    auto today = [](){
        // fixed ref date for reproducibility
        return ParsedDate{2025,8,11};
    }();
    long long gt80[3]{0,0,0};
    // We did not store all persons; recompute by scanning group files quickly
    auto count_gt80_in = [&](const std::string& path, int gi){
        std::ifstream f(path);
        std::string line;
        if(!f.good()) return;
        std::getline(f,line); // skip header
        while(std::getline(f,line)){
            auto v=split_csv(line);
            if((int)v.size()<=0) continue;
            ParsedDate fn{};
            if(idx.i_fnac<(int)v.size() && parse_date(v[idx.i_fnac], fn)){
                if(compute_age_years(fn,today) >= 80) gt80[gi]++;
            }
        }
    };
    count_gt80_in(out_dir+"/grupo_A.csv",0);
    count_gt80_in(out_dir+"/grupo_B.csv",1);
    count_gt80_in(out_dir+"/grupo_C.csv",2);
    {
        std::ofstream f(out_dir+"/porcentaje_mayores80_por_calendario.csv");
        f<<"grupo,mayores80,total,porcentaje\n";
        for(int gi=0;gi<3;++gi){
            long long tot = aggr.count_group[gi];
            double pct = (tot>0)? (100.0* (double)gt80[gi] / (double)tot) : 0.0;
            char g = (gi==0?'A':gi==1?'B':'C');
            f<<g<<","<<gt80[gi]<<","<<tot<<","<<std::fixed<<std::setprecision(2)<<pct<<"\n";
        }
    }

    // 3) Top 5 ciudades por patrimonio total
    std::vector<std::pair<std::string,long double>> city_total;
    city_total.reserve(aggr.city_sum_count.size());
    for(auto& kv : aggr.city_sum_count){
        city_total.emplace_back(kv.first, kv.second.first);
    }
    std::sort(city_total.begin(), city_total.end(), [](auto& a, auto& b){ return a.second>b.second; });
    {
        std::ofstream f(out_dir+"/top5_ciudades_patrimonio_total.csv");
        f<<"ciudad,patrimonio_total\n";
        for(int i=0;i<(int)city_total.size() && i<5; ++i){
            f<<city_total[i].first<<","<<std::fixed<<std::setprecision(2)<<(double)city_total[i].second<<"\n";
        }
    }

    // resumen.json
    auto bb_to_json = [&](const BestBy& b){
        std::ostringstream ss;
        ss<< "{"
           << "\"nombre\":\""<< b.nombre <<"\","
           << "\"documento\":\""<< b.documento <<"\","
           << "\"ciudad\":\""<< b.ciudad <<"\","
           << "\"valor\":"<< b.value <<","
           << "\"nacimiento\":\""<< b.nacimiento.y <<"-"<< std::setw(2) << std::setfill('0') << b.nacimiento.m
           << "-" << std::setw(2) << std::setfill('0') << b.nacimiento.d <<"\""
           << "}";
        return ss.str();
    };
    std::ofstream fr(out_dir+"/resumen.json");
    fr<<"{\n";
    fr<<"  \"modo\":\""<< (USE_POINTERS? "apuntadores":"valores") <<"\",\n";
    fr<<"  \"tiempo_s\":"<< std::fixed << std::setprecision(6) << secs << ",\n";
    fr<<"  \"mem_kb\": {\"VmRSS\":"<< ms.vm_rss_kb <<", \"VmHWM\":"<< ms.vm_hwm_kb <<", \"ru_maxrss\":"<< ms.ru_maxrss_kb <<"},\n";
    fr<<"  \"cuenta_por_grupo\": {\"A\":"<< aggr.count_group[0] <<", \"B\":"<< aggr.count_group[1] <<", \"C\":"<< aggr.count_group[2] <<"},\n";
    fr<<"  \"mas_longeva_pais\": "<< bb_to_json(aggr.oldest_country) <<",\n";
    fr<<"  \"mas_rica_pais\": "<< bb_to_json(aggr.richest_country) <<",\n";
    fr<<"  \"mas_rica_por_grupo\": {\"A\": "<< bb_to_json(aggr.richest_by_group[0]) <<", \"B\": "<< bb_to_json(aggr.richest_by_group[1]) <<", \"C\": "<< bb_to_json(aggr.richest_by_group[2]) <<"}\n";
    fr<<"}\n";
    fr.close();

    // por ciudad (richest & oldest) -> CSV
    {
        std::ofstream f(out_dir+"/mas_longeva_por_ciudad.csv");
        f<<"ciudad,nombre,documento,fecha_nacimiento\n";
        for(auto& kv: aggr.oldest_by_city){
            auto& b=kv.second;
            f<<kv.first<<","<<b.nombre<<","<<b.documento<<","
             << b.nacimiento.y << "-" << std::setw(2) << std::setfill('0') << b.nacimiento.m
             << "-" << std::setw(2) << std::setfill('0') << b.nacimiento.d << "\n";
        }
    }
    {
        std::ofstream f(out_dir+"/mas_rica_por_ciudad.csv");
        f<<"ciudad,nombre,documento,neto\n";
        for(auto& kv: aggr.richest_by_city){
            auto& b=kv.second;
            f<<kv.first<<","<<b.nombre<<","<<b.documento<<","<<b.value<<"\n";
        }
    }

    // done
    std::cerr<<"OK: procesado. Tiempo "<<secs<<" s. VmHWM="<<ms.vm_hwm_kb<<" kB\n";
    return 0;
}
