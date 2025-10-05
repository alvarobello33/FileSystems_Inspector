# Compilador y banderas
CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = 

# Nombres de los ejecutables
TARGET = program.exe

# Archivos fuente
SRCS = main.c fat16.c ext2.c

# Archivos objeto (se generan automáticamente)
OBJS = $(SRCS:.c=.o)

# Regla por defecto
all: $(TARGET)

# Regla para compilar el ejecutable principal
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

# Regla genérica para compilar cualquier .c a .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Reglas para limpieza
clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean