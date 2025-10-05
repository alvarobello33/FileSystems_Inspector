# Visor de Sistemas de Archivos FAT16 y EXT2

Este proyecto esta implementado en **C** con Visual Studio Code. Este código es capaz de:

- Mostrar la información básica del sistema de archivos.  
- Mostrar el árbol de directorios.  
- Mostrar el contenido de un archivo dado por su ruta.

Compatible con los sistemas de archivos **FAT16** (antiguamente usado en Windows) y **EXT2** (antiguamente usado en Linux). Ambos sistemas, aunque ya obsoletos en la mayoría de los casos, son fundamentales para entender la evolución de los sistemas de archivos y su funcionamiento interno.

---

## Funcionalidades

- **Visualización de la información general**  
El programa permite obtener datos esenciales como el tamaño de bloque, el número de inodos, el nombre del volumen y otra información relevante del sistema de archivos.

- **Exploración del árbol de directorios**  
Permite recorrer y mostrar el árbol de directorios y archivos de la imagen del sistema de archivos, facilitando una comprensión clara de su estructura interna.

- **Visualización del contenido de archivos**  
Esta funcionalidad permite acceder al contenido de archivos de texto o binarios directamente desde la imagen del sistema de archivos, especificando la ruta del archivo deseado.

---

## Compilación y ejecución

Compila el programa con `gcc` utilizando el archivo makefile proporcionado:

```bash
make
```

Esto creará un ejecutable llamado program (además de los archivos .o generados por cada módulo).

Luego, puedes ejecutar los siguientes comandos:
- Para ver la información general del sistema de archivos:
```
./program --info <filesystem>
```

- Para mostrar el árbol de directorios:
```
./program --tree <filesystem>
```

- Para ver el contenido de un archivo dentro del sistema de archivos:
```
./program --cat <filesystem> <ruta_archivo>
```


## Compatibilidad con sistemas de archivos

Imágenes de sistemas de archivos FAT16.
Imágenes de sistemas EXT2.