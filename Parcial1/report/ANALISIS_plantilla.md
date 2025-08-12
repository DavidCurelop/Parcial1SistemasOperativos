# Informe – Parcial 1 (Sistemas Operativos) – 2025B

**Integrantes:** _(nombres)_  
**Entrada:** _(ruta del CSV y tamaño)_  
**Equipo/CPU/RAM:** _(para contexto de métricas)_

## 1. Funcionalidad (Respuestas)

- Persona más longeva (país): …  
- Persona más longeva **por ciudad**: …  
- Persona con mayor patrimonio (país): …  
- Persona con mayor patrimonio **por ciudad**: …  
- Persona con mayor patrimonio **por grupo (A/B/C)**: …  
- Declarantes por calendario (conteo y validación): …

## 2. Tres preguntas adicionales
1. Ciudades con patrimonio **promedio** más alto (Top K): …  
2. % de personas **> 80 años** por calendario (A/B/C): …  
3. **Top 5** ciudades por **patrimonio total** (suma neta): …

## 3. Métricas (obligatorias)

Ejecutado en fecha: 2025-08-11

| Variante | Tiempo total (s) | Mem. pico (MiB) | Mem. media (MiB) |
|---|---:|---:|---:|
| C++ – valores |  |  |  |
| C++ – apuntadores |  |  |  |
| C – valores |  |  |  |
| C – apuntadores |  |  |  |

> *Memoria pico* tomada de `VmHWM`/`ru_maxrss`. *Memoria media* aproximada con `VmRSS` al final.

## 4. Comparaciones críticas

### Valores vs. Apuntadores
- **Hipótesis**: el internado de cadenas reduce copias repetidas (p. ej., muchas filas con la misma ciudad), lo que **disminuye el uso de memoria** y mejora la **localidad** al acceder al pool.
- **Evidencia (tus números):** …
- **Conclusión:** …

### Struct (C) vs. Class (C++)
- **Hipótesis**: C puede tener **ligero** mejor uso de memoria por estructuras *plain*; C++ puede **igualar o superar** en tiempo por optimizaciones, `std::string_view` y mejores contenedores.
- **Evidencia (tus números):** …
- **Conclusión:** …

## 5. Pensamiento Crítico (respuesta breve)

1. **Memoria:** ¿Por qué usar apuntadores reduce ~75% con 10M registros?  
   _Porque elimina duplicación de cadenas (ciudad/nombre) mediante **internado**; cada registro guarda un **puntero** (8 bytes) en vez de repetir cadenas (decenas de bytes) + overhead. Si hay alta repetición (p. ej., pocas ciudades), la reducción puede acercarse a ese orden._

2. **Datos:** Si el calendario depende de los dos últimos dígitos del documento, ¿cómo optimizar búsquedas por grupo?  
   _Usar un **arreglo de 100 buckets** (00–99) → O(1). Para los grupos A/B/C mapear rangos a índices: A=[0..39], B=[40..79], C=[80..99]._

3. **Localidad:** ¿Cómo afecta el acceso a memoria al usar *array de structs* vs. *vector de clases*?  
   _Un **AoS** (array of structs) favorece acceso secuencial cuando se consumen todos los campos de cada registro. Para cálculos que solo usan algunos campos, un **SoA** (struct of arrays) mejora la **localidad** de esos campos y el **vectorization**._

4. **Escalabilidad:** Si los datos exceden la RAM, ¿cómo usar `mmap` o memoria virtual?  
   _Procesamiento **streaming** + **archivos por calendario**. `mmap` permite **paginación** por el kernel y acceso **on‑demand**; combinar con **ventanas** (chunks) y `posix_fadvise`/`madvise` para *sequential read*._

## 6. Cómo reproducir
```bash
make build
./bin/so_parcial_cpp_pointers --in data.csv --out out --topk 10
```

Adjunta este PDF con capturas y outputs.
