#ifndef FAT16_H
#define FAT16_H

#include <stdint.h>


#define FAT16_BPB_OFFSET 0
#define ATTR_DIRECTORY 0x10
#define DIR_ENTRY_SIZE 32


// Estructura del BPB (BIOS Parameter Block) para FAT16
#pragma pack(push, 1)       // Guarda la configuración actual de alineación y Establece alineación a 1 byte (sin padding)
typedef struct {
    uint8_t jmpBoot[3];
    char OEMName[8];
    uint16_t BytesPerSector;
    uint8_t SectorsPerCluster;
    uint16_t ReservedSectors;
    uint8_t NumFATs;
    uint16_t RootEntries;
    uint16_t TotalSectors16;
    uint8_t Media;
    uint16_t FATSize16;
    uint16_t SectorsPerTrack;
    uint16_t NumHeads;
    uint32_t HiddenSectors;
    uint32_t TotalSectors32;
    uint8_t DriveNumber;
    uint8_t Reserved1;
    uint8_t BootSignature;
    uint32_t VolumeID;
    char VolumeLabel[11];
    char FileSystemType[8];
} FAT16_BPB;
#pragma pack(pop)       // Restaura la configuración de alineación previa (antes del push)

typedef struct {
    char name[12];      // Nombre legible del archivo/directorio
    uint8_t attr;       // Atributos (archivo, directorio, permisos)
    uint16_t firstCluster; // Cluster inicial del archivo/directorio
} FAT16_DirEntry;


int detect_FAT16(const char *device, FAT16_BPB *bpb);
void print_FAT16_info(const FAT16_BPB *bpb);

void print_FAT16_tree(const char *image_path, const FAT16_BPB *bpb);

int cat_FAT16(const char *image_path, const FAT16_BPB *bpb, const char *filepath);

#endif