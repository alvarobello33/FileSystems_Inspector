# FAT16 and EXT2 File System Viewer

This project is implemented in C with Visual Studio Code. The code can:

- Display basic file system information.
- Display the directory tree.
- Display the contents of a file given its path.

It is compatible with the FAT16 file system (formerly used in Windows) and EXT2 (formerly used in Linux). Although largely obsolete today, both are fundamental to understanding the evolution of file systems and their internal workings.

---

## Features

- **General information display**  
  The program retrieves essential data such as block size, number of inodes, volume name, and other relevant file system information.

- **Directory tree exploration**  
  It traverses and displays the directory and file tree of the file system image, providing a clear view of its internal structure.

- **File content display**  
  This feature allows accessing the content of text or binary files directly from the file system image by specifying the desired file path.

---

## Build and run

Compile the program with `gcc` using the provided makefile:

```bash
make
```


This will create an executable named `program` (in addition to the `.o` files generated for each module).

Then, run the following commands:
- To view general file system information:

```
./program --info <filesystem>
```

- To display the directory tree:
```
./program --tree <filesystem>
```

- To display the contents of a file within the file system:
```
./program --cat <filesystem> <ruta_archivo>
```

---

## File system compatibility

- FAT16 file system images.
- EXT2 file system images.