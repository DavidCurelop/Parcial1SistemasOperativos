# Makefile para compilación automatizada

# Configuración del compilador
CXX := g++ # Usa el compilador g++
CXXFLAGS := -Wall -Wextra -pedantic -std=c++11 # Opciones de compilación

# Archivos fuente y objetos
SRCS := persona.cpp generador.cpp main.cpp # Todos los archivos fuente
OBJS := $(SRCS:.cpp=.o) # Genera lista de objetos (.o)
EXEC := programa # Nombre del ejecutable final

# Objetivo principal: compilar el ejecutable
all: $(EXEC)

# Enlaza todos los objetos en el ejecutable
$(EXEC): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Reglas específicas para cada objeto con sus dependencias
# persona.o depende de persona.cpp y persona.h
persona.o: persona.cpp persona.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# generador.o depende de generador.cpp y sus headers
generador.o: generador.cpp generador.h persona.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# main.o depende de main.cpp y sus headers
main.o: main.cpp persona.h generador.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpia archivos generados
clean:
	rm -f $(OBJS) $(EXEC)

# Ejecuta el programa después de compilar
run: $(EXEC)
	./$(EXEC)
