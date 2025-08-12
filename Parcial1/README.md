# Parcial 1 – Sistemas Operativos (2025B)

Solución completa para **Manejo de Grandes Volúmenes de Datos en Linux con C/C++**.

Incluye:
- Implementación **C++**: modos *valores* y *apuntadores* (internado de cadenas).
- Implementación **C**: `struct` con variantes *valores* y *apuntadores* (pool de cadenas sencillo).
- Métricas: tiempo y memoria (**RSS** y **PeakRSS**) desde `/proc/self/status` y `getrusage`.
- Salidas: respuestas a las preguntas obligatorias, validación de calendario, y 3 preguntas adicionales.
- Streaming > 1M registros: sin cargar todo en memoria; se escriben listados por calendario a archivos.

> **Formato de entrada esperado**: CSV con cabecera (UTF-8). Campos mínimos:
> `nombre,fecha_nacimiento,ciudad,patrimonio,deudas,documento[,calendario]`.
> Fechas admitidas: `YYYY-MM-DD`, `DD/MM/YYYY` o `YYYY/MM/DD`.
> El campo `calendario` (si existe) se valida vs. dígitos del documento; si no existe, se infiere.

## Compilación (Linux)

```bash
make build        # compila todo (C y C++)
```

## Ejecución

```bash
# C++ (valores)
./bin/so_parcial_cpp_values --in data.csv --out out --topk 10

# C++ (apuntadores con internado)
./bin/so_parcial_cpp_pointers --in data.csv --out out --topk 10

# C (valores)
./bin/so_parcial_c_values --in data.csv --out out --topk 10

# C (apuntadores con internado)
./bin/so_parcial_c_pointers --in data.csv --out out --topk 10
```

Parámetros:
- `--in`: archivo CSV de entrada
- `--out`: carpeta de salida (se crea si no existe)
- `--topk`: K para listados (p.ej., *top ciudades por patrimonio promedio*)

## Salidas generadas
- `out/resumen.json` – métricas, respuestas y estadísticas.
- `out/grupo_A.csv`, `out/grupo_B.csv`, `out/grupo_C.csv` – listados por calendario.
- `out/validaciones.csv` – filas con discrepancias de calendario detectadas.
- `out/ciudades_top_promedio.csv` – 1ª pregunta adicional.
- `out/porcentaje_mayores80_por_calendario.csv` – 2ª pregunta adicional.
- `out/top5_ciudades_patrimonio_total.csv` – 3ª pregunta adicional.

## Preguntas adicionales incluidas
1. **Ciudades con patrimonio promedio más alto** (controlado por `--topk`).
2. **% de personas > 80 años por calendario**.
3. **Top 5 ciudades por patrimonio total** (sumatoria neta).

## Notas de rendimiento
- *Valores*: usa `std::string`/copias (C++) o `malloc` copias (C).
- *Apuntadores*: **internado de cadenas** (una copia única por *token*); los registros guardan `const char*` (C++) o `char*` (C).
- Se miden tiempos con `std::chrono`/`clock_gettime` y memoria con `/proc/self/status` y `getrusage`.
- Para >10M registros, se recomienda desactivar `LC_ALL` que pueda afectar `std::stod`, y compilar con `-O3 -march=native`.

## Generar informe
Rellena `report/ANALISIS_plantilla.md` con tus métricas y conclusiones y conviértelo a PDF (p. ej., con pandoc).
