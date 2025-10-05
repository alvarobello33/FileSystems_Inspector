#include "fat16.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include <stdint.h>

// ----------------------------------------
// --------- Funciones Privadas -----------
// ----------------------------------------

// FASE 1
uint32_t calculate_cluster_count(const FAT16_BPB *bpb) {
    // Calcular sectores del directorio raíz
    uint32_t RootDirSectors = ((bpb->RootEntries * 32) + (bpb->BytesPerSector - 1)) / bpb->BytesPerSector;

    // Determinar total de sectores (usando 16-bit o 32-bit según corresponda)
    uint32_t TotalSectors = (bpb->TotalSectors16 != 0) ? bpb->TotalSectors16 : bpb->TotalSectors32;

    // Calcular sectores en la región de datos
    uint32_t DataSec = TotalSectors - (bpb->ReservedSectors + (bpb->NumFATs * bpb->FATSize16) + RootDirSectors);

    // Calcular número de clusters (en región de datos)
    return DataSec / bpb->SectorsPerCluster;
}

// FASE 2

void print_indent(int level) {
    for (int i = 0; i < level; ++i) {
        printf("│   ");
    }
}

void print_entry_name(const uint8_t *entry) {
    char name[12];
    memset(name, 0, sizeof(name));

    memcpy(name, entry, 8);
    for (int i = 7; i >= 0 && name[i] == ' '; i--) name[i] = '\0';

    if (entry[8] != ' ') {
        strcat(name, ".");
        strncat(name, (char*)&entry[8], 3);
    }

    printf("%s\n", name);
}


void read_directory(int fd, const FAT16_BPB *bpb, uint16_t cluster, int level, int is_root) {
    uint32_t root_dir_sectors = ((bpb->RootEntries * 32) + (bpb->BytesPerSector - 1)) / bpb->BytesPerSector;
    uint32_t first_data_sector = bpb->ReservedSectors + (bpb->NumFATs * bpb->FATSize16) + root_dir_sectors;

    if (is_root) {
        // El directorio raíz está en una ubicación fija
        uint32_t dir_sector = bpb->ReservedSectors + (bpb->NumFATs * bpb->FATSize16);
        size_t dir_size = root_dir_sectors * bpb->BytesPerSector;

        // Mover el puntero al sector del directorio
        uint8_t *buffer = malloc(dir_size);
        lseek(fd, dir_sector * bpb->BytesPerSector, SEEK_SET);
        read(fd, buffer, dir_size);

        // Leer las entradas del directorio
        for (size_t i = 0; i < dir_size; i += DIR_ENTRY_SIZE) {
            uint8_t *entry = buffer + i;

            if (entry[0] == 0x00) break; // fin de entradas
            if (entry[0] == 0xE5) continue; // entrada eliminada
            if (entry[11] == 0x0F) continue; // entrada LFN (long filename)

            print_indent(level);
            printf("├── ");
            print_entry_name(entry);

            // Si es un directorio, leerlo recursivamente (excepto para "." y "..")
            if ((entry[11] & ATTR_DIRECTORY) && entry[0] != '.') {
                uint16_t firstCluster = entry[26] | (entry[27] << 8);
                if (firstCluster != 0) {
                    read_directory(fd, bpb, firstCluster, level + 1, 0);
                }
            }
        }

        free(buffer);
        return;
    }

    // --- Subdirectorios: seguir la cadena de clusters ---
    uint16_t current_cluster = cluster;

    while (current_cluster < 0xFFF8) {  // Fin de cadena en FAT16
        // Calcular el sector inicial del clúster
        uint32_t first_sector = ((current_cluster - 2) * bpb->SectorsPerCluster) + first_data_sector;
        size_t cluster_size = bpb->SectorsPerCluster * bpb->BytesPerSector;

        // Mover el puntero al sector del clúster correspondiente y leerlo
        uint8_t *buffer = malloc(cluster_size);
        lseek(fd, first_sector * bpb->BytesPerSector, SEEK_SET);
        read(fd, buffer, cluster_size);

        // Recorrer las entradas del directorio (dentro del clúster)
        for (size_t i = 0; i < cluster_size; i += DIR_ENTRY_SIZE) {
            uint8_t *entry = buffer + i;

            if (entry[0] == 0x00) break;
            if (entry[0] == 0xE5) continue;
            if (entry[11] == 0x0F) continue;

            print_indent(level);
            printf("├── ");
            print_entry_name(entry);

            if ((entry[11] & ATTR_DIRECTORY) && entry[0] != '.') {
                uint16_t firstCluster = entry[26] | (entry[27] << 8);
                if (firstCluster != 0) {
                    read_directory(fd, bpb, firstCluster, level + 1, 0);
                }
            }
        }

        free(buffer);

        // Obtener el siguiente clúster desde la FAT
        off_t fat_offset = bpb->ReservedSectors * bpb->BytesPerSector;
        off_t next_cluster_offset_in_fat = fat_offset + current_cluster * 2;
        uint16_t next_cluster;
        lseek(fd, next_cluster_offset_in_fat, SEEK_SET);
        read(fd, &next_cluster, 2);

        if (next_cluster >= 0xFFF8) break;  // Fin de cadena
        current_cluster = next_cluster;
    }
}


// ----------------------------------------
// --------- Funciones Publicas -----------
// ----------------------------------------

int detect_FAT16(const char *device, FAT16_BPB *bpb) {

    //----FAT16----
    int fd = open(device, O_RDONLY);
    if (fd == -1) {
        perror("Error al abrir el dispositivo");
        return -1;
    }

    // Ir al offset del BPB (es 0 pero para entender mejor el funcionamiento)
    if (lseek(fd, FAT16_BPB_OFFSET, SEEK_SET) == -1) {
        perror("Error al hacer lseek para FAT16");
        close(fd);
        return -1;
    }

    // Leer 512 bytes del sector de arranque
    unsigned char boot_sector[512];
    if (read(fd, boot_sector, sizeof(boot_sector)) != sizeof(boot_sector)) {
        perror("Error al leer el sector de arranque");
        close(fd);
        return -1;
    }

    // Obtener el BPB desde el sector leído
    memcpy(bpb, boot_sector, sizeof(FAT16_BPB));


    // Calcular número de clusters
    uint32_t cluster_count = calculate_cluster_count(bpb);

    // Verificar si el tipo de sistema de archivos es FAT16
    if ((cluster_count >= 4085 && cluster_count < 65525)) {
        // Es FAT16
        close(fd);
        return 1;

    } else {
        // No es FAT16
        //printf("No es un sistema de archivos FAT16 (CountofClusters: %u)\n", cluster_count);
        close(fd);
        return -1;
    }
    
}

void print_FAT16_info(const FAT16_BPB *bpb) {
    printf("\n--- Filesystem Information ---\n\n");
    printf("Filesystem: FAT16\n\n");
    
    printf("System name: %.8s\n", bpb->OEMName);
    printf("Sector size: %u\n", bpb->BytesPerSector);
    printf("Sectors per cluster: %u\n", bpb->SectorsPerCluster);
    printf("Reserved sectors: %u\n", bpb->ReservedSectors);
    printf("# of FATs: %u\n", bpb->NumFATs);
    printf("Max root entries: %u\n", bpb->RootEntries);
    printf("Sectors per FAT: %u\n", bpb->FATSize16);
    printf("Label: %.11s\n\n", bpb->VolumeLabel);
}

void print_FAT16_tree(const char *image_path, const FAT16_BPB *bpb) {
    int fd = open(image_path, O_RDONLY);
    if (fd < 0) {
        perror("No se pudo abrir la imagen/dispositivo");
        return;
    }

    printf(".\n"); // raíz del sistema
    read_directory(fd, bpb, 0, 0, 1);

    close(fd);
}


//FASE 3
// Formatea nombre a formato FAT16
void format_name(const char *input, uint8_t out11[11]) {
    char name[9] = {0};
    char ext[4]  = {0};
    int nameLen = 0, extLen = 0;

    const char *dot = strchr(input, '.');
    if (dot) {
        nameLen = dot - input;
        if (nameLen > 8) nameLen = 8;
        memcpy(name, input, nameLen);

        extLen = strlen(dot + 1);
        if (extLen > 3) extLen = 3;
        memcpy(ext, dot + 1, extLen);
    } else {
        nameLen = strlen(input);
        if (nameLen > 8) nameLen = 8;
        memcpy(name, input, nameLen);
    }

    // Mayúsculas
    for (int i = 0; i < nameLen; i++)  name[i] = toupper((unsigned char)name[i]);
    for (int i = 0; i < extLen;  i++)  ext[i]  = toupper((unsigned char)ext[i]);

    // Rellena out11: 8 bytes de name (espacios a la derecha), luego 3 de extension
    memset(out11, ' ', 11);
    memcpy(out11,      name, nameLen);
    memcpy(out11 + 8,  ext,  extLen);
}


static int find_entry(int fd, const FAT16_BPB *bpb,uint16_t cluster, const uint8_t name11[11], uint8_t entry_out[32]) {
    // Cálculo de sectores en directorio raíz y primer sector con datos
    uint32_t root_dir_sectors = ((bpb->RootEntries * 32) + bpb->BytesPerSector - 1) / bpb->BytesPerSector;
    uint32_t first_data_sector = bpb->ReservedSectors + bpb->NumFATs * bpb->FATSize16 + root_dir_sectors;

    if (cluster == 0) {
        // Ruta fija de la raíz
        uint32_t dir_sector = bpb->ReservedSectors + bpb->NumFATs * bpb->FATSize16;
        size_t dir_size = root_dir_sectors * bpb->BytesPerSector;
        uint8_t *buf = malloc(dir_size);
        lseek(fd, dir_sector * bpb->BytesPerSector, SEEK_SET);
        read(fd, buf, dir_size);
        for (size_t off = 0; off < dir_size; off += 32) {
            uint8_t *e = buf + off;
            if (e[0] == 0x00) break;
            if (e[0] == 0xE5 || e[11] == 0x0F) continue;
            if (memcmp(e, name11, 11) == 0) {
                memcpy(entry_out, e, 32);
                free(buf);
                return 0;
            }
        }
        free(buf);
        return -1;
    }

    // Subdirectorios: recorrer cadena de clusters
    uint16_t cur = cluster;
    while (cur < 0xFFF8) {
        uint32_t first_sector = ((cur - 2) * bpb->SectorsPerCluster) + first_data_sector;
        size_t cl_sz = bpb->SectorsPerCluster * bpb->BytesPerSector;
        uint8_t *buf = malloc(cl_sz);
        lseek(fd, first_sector * bpb->BytesPerSector, SEEK_SET);
        read(fd, buf, cl_sz);
        for (size_t off = 0; off < cl_sz; off += 32) {
            uint8_t *e = buf + off;
            if (e[0] == 0x00) { free(buf); return -1; }
            if (e[0] == 0xE5 || e[11] == 0x0F) continue;
            if (memcmp(e, name11, 11) == 0) {
                memcpy(entry_out, e, 32);
                free(buf);
                return 0;
            }
        }
        free(buf);
        // Leer siguiente cluster de la FAT 
        off_t fat_off = bpb->ReservedSectors * bpb->BytesPerSector + cur * 2;
        lseek(fd, fat_off, SEEK_SET);
        read(fd, &cur, 2);
    }
    return -1;
}

static void dump_file(int fd, const FAT16_BPB *bpb, uint16_t start, uint32_t size) {
    uint32_t root_dir_sectors = ((bpb->RootEntries * 32) + bpb->BytesPerSector - 1) / bpb->BytesPerSector;
    uint32_t first_data_sector = bpb->ReservedSectors + bpb->NumFATs * bpb->FATSize16 + root_dir_sectors;
    uint32_t remaining = size;
    uint16_t cur = start;
    while (cur < 0xFFF8 && remaining > 0) {
        uint32_t first_sector = ((cur - 2) * bpb->SectorsPerCluster) + first_data_sector;
        size_t cl_sz = bpb->SectorsPerCluster * bpb->BytesPerSector;
        size_t toread = remaining < cl_sz ? remaining : cl_sz;
        uint8_t *buf = malloc(cl_sz);
        lseek(fd, first_sector * bpb->BytesPerSector, SEEK_SET);
        read(fd, buf, toread);
        fwrite(buf, 1, toread, stdout);
        free(buf);
        remaining -= toread;
        // siguiente cluster
        off_t fat_off = bpb->ReservedSectors * bpb->BytesPerSector + cur * 2;
        lseek(fd, fat_off, SEEK_SET);
        read(fd, &cur, 2);
    }
}

// --cat para FAT16
int cat_FAT16(const char *image_path, const FAT16_BPB *bpb, const char *filepath) {
    int fd = open(image_path, O_RDONLY);
    if (fd < 0) { perror("open"); return -1; }

    // Tokenizar ruta por '/' y buscar recursivamente
    char *path = strdup(filepath);
    char *tok = strtok(path, "/");      // Separa ruta con '\0' para recorrerla
    uint16_t cluster = 0;
    uint8_t entry[32];
    uint8_t name11[11];

    // Recorrer cada componente de la ruta
    while (tok) {
        format_name(tok, name11);   // Formatear nombre a FAT16

        // Buscamos la entrada del archivo o directorio en el cluster actual
        if (find_entry(fd, bpb, cluster, name11, entry) < 0) {
            fprintf(stderr, "No encontrado: %s\n", tok);
            free(path);
            close(fd);
            return -1;
        }
        // Para siguiente, si es directorio, actualizar cluster; sino, mostrar archivo
        cluster = entry[26] | (entry[27] << 8);
        tok = strtok(NULL, "/");
        if (!tok) {
            // última componente: extraer tamaño y dumps
            uint32_t fsize = entry[28] | (entry[29]<<8) | (entry[30]<<16) | (entry[31]<<24);
            dump_file(fd, bpb, cluster, fsize);
        }
    }
    free(path);
    close(fd);
    return 0;
}
