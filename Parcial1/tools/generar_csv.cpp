#include <bits/stdc++.h>
#include "generador.h"

static int last2(const std::string& s){ int a=-1,b=-1; for(char c: s) if(isdigit((unsigned)c)){ a=b; b=c-'0'; } return (a<0? b: a*10+b); }
static char grupo(int l2){ return (l2<=39?'A': l2<=79?'B':'C'); }

int main(int argc,char**argv){
    int n=100000; std::string out="data.csv";
    for(int i=1;i<argc;i++){ std::string a=argv[i];
        if(a=="--n"&&i+1<argc) n=std::stoi(argv[++i]);
        else if(a=="--out"&&i+1<argc) out=argv[++i];
    }
    srand((unsigned)time(nullptr));
    auto v = generarColeccion(n);
    std::ofstream f(out);
    f<<"nombre,fecha_nacimiento,ciudad,patrimonio,deudas,documento,calendario\n";
    for(const auto& p: v){
        std::string doc = p.getId();
        char g = grupo(last2(doc));
        f << (p.getNombre()+" "+p.getApellido()) << ","
          << p.getFechaNacimiento() << ","
          << p.getCiudadNacimiento() << ","
          << (long long)p.getPatrimonio() << ","
          << (long long)p.getDeudas() << ","
          << doc << "," << g << "\n";
    }
}
