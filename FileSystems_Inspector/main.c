#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include "fat16.h"
#include "ext2.h"




int main(int argc, char *argv[]) {
    if (argc != 3 && argc != 4) {
        printf("Uso: %s --option <dispositivo_o_imagen>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--info") == 0) {
        // MMOSTRAR INFO DEL FILESYSTEM

		EXT2_Superblock sb;
		if (detect_EXT2(argv[2], &sb) == 1) {
			print_EXT2_info(&sb);
			return 0;
		}


        FAT16_BPB bpb;
        if (detect_FAT16(argv[2], &bpb) == 1) {
            print_FAT16_info(&bpb);
            return 0;
        }
    
        // Ninguno fue detectado
        printf("\nNot supported file system. Only FAT16 and EXT2 are supported.\n");
        return 1;
    }

    if (strcmp(argv[1], "--tree") == 0) {
        // MOSTRAR ÁRBOL DE DIRECTORIOS

        EXT2_Superblock sb;
    	if (detect_EXT2(argv[2], &sb) == 1) {
        	print_EXT2_tree(argv[2], &sb);
        	return 0;
    	}

        FAT16_BPB bpb;
        if (detect_FAT16(argv[2], &bpb) == 1) {
            print_FAT16_tree(argv[2], &bpb);
            return 0;
        }
        // Filesystem no soportado
        printf("\nNot supported file system. Only FAT16 and EXT2 are supported.\n");
        return 1;
    }

    if (strcmp(argv[1], "--cat") == 0) {
        if (argc != 4) {
            fprintf(stderr, "Uso: %s --cat <FAT16_img> <file>\n", argv[0]);
            return 1;
        }
        FAT16_BPB bpb;
        if (detect_FAT16(argv[2], &bpb) != 1) {
            fprintf(stderr, "No es FAT16: %s\n", argv[2]);
            return 1;
        }
        return cat_FAT16(argv[2], &bpb, argv[3]);
    }

    // En caso de que no se reconozca la opción
    printf("\nError: %s is not a valid option.\n", argv[1]);
    return 1;
}
