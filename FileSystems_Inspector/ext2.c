#include "ext2.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

void print_time(uint32_t timestamp) {
    time_t t = timestamp;
    struct tm *tm_info = localtime(&t);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Y", tm_info);
    printf("%s", buffer);
}

void print_EXT2_info(const EXT2_Superblock *sb) {
    uint32_t block_size = 1024 << sb->s_log_block_size;

    printf("--- Filesystem Information ---\n");
    printf("Filesystem: EXT2\n\n");
    
    printf("INODE INFO\n");
    printf("  Size: %u\n", sb->s_inode_size);
    printf("  Num Inodes: %u\n", sb->s_inodes_count);
    printf("  First Inode: %u\n", sb->s_first_ino);
    printf("  Inodes Group: %u\n", sb->s_inodes_per_group);
    printf("  Free Inodes: %u\n\n", sb->s_free_inodes_count);
    
    printf("INFO BLOCK\n");
    printf("  Block size: %u\n", block_size);
    printf("  Reserved blocks: %u\n", sb->s_r_blocks_count);
    printf("  Free blocks: %u\n", sb->s_free_blocks_count);
    printf("  Total blocks: %u\n", sb->s_blocks_count);
    printf("  First block: %u\n", sb->s_first_data_block);
    printf("  Group blocks: %u\n", sb->s_blocks_per_group);
    printf("  Group flags: %u\n\n", sb->s_frags_per_group);
    
    printf("INFO VOLUME\n");
    printf("  Volume name: %s\n", sb->s_volume_name);
    printf("  Last Checked: ");
    print_time(sb->s_lastcheck);
    printf("\n");
    printf("  Last Mounted: ");
    print_time(sb->s_wtime);
    printf("\n");
    printf("  Last Written: ");
    print_time(sb->s_wtime);
    printf("\n");
}

int detect_EXT2(const char *device, EXT2_Superblock *sb) {
    int fd = open(device, O_RDONLY);
    if (fd == -1) {
        perror("Error al abrir el dispositivo (EXT2)");
        return -1;
    }

    if (lseek(fd, EXT2_SUPERBLOCK_OFFSET, SEEK_SET) == -1) {
        perror("Error al buscar el superbloque");
        close(fd);
        return -1;
    }

    if (read(fd, sb, sizeof(EXT2_Superblock)) != sizeof(EXT2_Superblock)) {
        perror("Error al leer el superbloque");
        close(fd);
        return -1;
    }

    if (sb->s_magic != EXT2_SUPER_MAGIC) {
        close(fd);
        return -1;
    }

    close(fd);
    return 1;
}


// Fase 2
static void print_indent(int level) {
    for (int i = 0; i < level; i++) printf("│   ");
}

// Lee un inodo específico del sistema de archivos a partir de su número en out
int read_inode(int fd, const EXT2_Superblock *sb, const EXT2_GroupDesc *gd, uint32_t inode_num, EXT2_Inode *out)
{
    uint32_t blk_sz     = EXT2_BLOCK_SIZE(sb);

    off_t table_offset  = (off_t)gd->bg_inode_table * blk_sz;
    off_t inode_offset  = table_offset + (inode_num - 1) * sb->s_inode_size;

    uint8_t buf[sb->s_inode_size];
    if (lseek(fd, inode_offset, SEEK_SET) == -1 ||
        read(fd, buf, sb->s_inode_size) != (ssize_t)sb->s_inode_size)
    {
        perror("read_inode");
        return -1;
    }

    // Copia SOLO los primeros Bytes que coinciden con el struct 
    memcpy(out, buf, sizeof(EXT2_Inode));
    return 0;
}

// Lee un bloque de datos del sistema de  en buf
int read_block(int fd, uint32_t block_size, uint32_t block_num, void *buf) {
    off_t offset = (off_t)block_num * block_size;
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Error seeking to block");
        return -1;
    }

    ssize_t bytes_read = read(fd, buf, block_size);
    if (bytes_read == -1) {
        perror("Error reading block");
        return -1;
    }
    if (bytes_read == 0) {
        // Bloque vacío: no es un error, solo no hay datos (probablemente porque han sido borrados)
        return 0;
    }
    if (bytes_read != (ssize_t)block_size) {
        fprintf(stderr, "Short read: expected %u, got %zd\n", block_size, bytes_read);
        return -1;
    }

    return 0;
}


void print_directory_recursive(int fd,
                              const EXT2_Superblock *sb,
                              const EXT2_GroupDesc *gd,
                              const EXT2_Inode *inode,
                              int depth);
                      
                              
// Procesa un bloque directo de directorio, mostrando sus entradas y recorriendolas si son directorios.
void process_directory_block(int fd, const EXT2_Superblock *sb, const EXT2_GroupDesc *gd,
                            uint8_t *buf, uint32_t block_size, int depth) {
    uint32_t pos = 0;
    // Recorremos entradas
    while (pos < block_size) {
        EXT2_DirEntry *e = (EXT2_DirEntry *)(buf + pos);
        if (e->inode == 0 || e->rec_len == 0) break;

        char name[MAX_NAME_LEN + 1] = {0};
        memcpy(name, e->name, e->name_len);
        name[e->name_len] = '\0';

        // Comprobamos que no sea ni . ni ..
        if (strcmp(name, ".") && strcmp(name, "..")) {
            print_indent(depth);
            printf("|__ %s\n", name);

            if (e->file_type == EXT2_FT_DIR) {
                EXT2_Inode child;
                if (read_inode(fd, sb, gd, e->inode, &child) == 0) {
                    print_directory_recursive(fd, sb, gd, &child, depth + 1);
                }
            }
        }
        pos += e->rec_len;
    }
}

// Procesa recursivamente bloques indirectos (nivel 2 o 3)
void process_indirect_blocks(int fd, const EXT2_Superblock *sb, const EXT2_GroupDesc *gd,
                            uint32_t block_num, int level, const EXT2_Inode *inode, int depth) {
    uint32_t block_size = EXT2_BLOCK_SIZE(sb);
    uint32_t *block_ptrs = malloc(block_size);
    if (!block_ptrs) {
        perror("malloc failed");
        return;
    }

    if (read_block(fd, block_size, block_num, block_ptrs) < 0) {
        free(block_ptrs);
        return;
    }

    int entries = block_size / sizeof(uint32_t);
    for (int i = 0; i < entries; i++) {
        uint32_t blk = block_ptrs[i];
        if (!blk || blk >= sb->s_blocks_count) continue;

        if (level > 1) {
            // Si es nivel 2 o 3, procesamos recursivamente
            process_indirect_blocks(fd, sb, gd, blk, level - 1, inode, depth);
        } else {
            // Si es nivel 1, leemos el bloque de datos
            uint8_t *buf = malloc(block_size);
            if (!buf) {
                perror("malloc failed");
                continue;
            }

            if (read_block(fd, block_size, blk, buf) == 0) {
                process_directory_block(fd, sb, gd, buf, block_size, depth);
            }

            free(buf);
        }
    }

    free(block_ptrs);
}

// Procesa las entradas de un bloque de directorio e imprime nombres de archivos/directorios
void print_directory_recursive(int fd,
                              const EXT2_Superblock *sb,
                              const EXT2_GroupDesc *gd,
                              const EXT2_Inode *inode,
                              int depth) {
    uint32_t block_size = EXT2_BLOCK_SIZE(sb);
    uint8_t *buf = malloc(block_size);
    if (!buf) {
        perror("malloc failed");
        return;
    }

    // 1. Procesar bloques directos (0-11)
    uint32_t blocks_to_read = (inode->size + block_size - 1) / block_size;
    uint32_t direct_blocks = (blocks_to_read > EXT2_DIRECT_BLOCKS) ? EXT2_DIRECT_BLOCKS : blocks_to_read;

    for (uint32_t i = 0; i < direct_blocks; i++) {
        uint32_t blk = inode->block[i];
        if (!blk || blk >= sb->s_blocks_count + sb->s_first_data_block) continue;

        if (read_block(fd, block_size, blk, buf) == 0) {
            process_directory_block(fd, sb, gd, buf, block_size, depth);
        }
    }

    // 2. Procesar bloque indirecto simple (nivel 1)
    if (blocks_to_read > EXT2_INDIRECT_BLOCK && inode->block[EXT2_INDIRECT_BLOCK]) {
        process_indirect_blocks(fd, sb, gd, inode->block[EXT2_INDIRECT_BLOCK], 1, inode, depth);
    }

    // 3. Procesar bloque doble indirecto (nivel 2)
    if (blocks_to_read > EXT2_DOUBLE_INDIRECT_BLOCK) {
        if (inode->block[EXT2_DOUBLE_INDIRECT_BLOCK]) {
            process_indirect_blocks(fd, sb, gd, inode->block[EXT2_DOUBLE_INDIRECT_BLOCK], 2, inode, depth);
        }
    }

    // 4. Procesar bloque triple indirecto (nivel 3)
    if (blocks_to_read > EXT2_TRIPLE_INDIRECT_BLOCK) {
        if (inode->block[EXT2_TRIPLE_INDIRECT_BLOCK]) {
            process_indirect_blocks(fd, sb, gd, inode->block[EXT2_TRIPLE_INDIRECT_BLOCK], 3, inode, depth);
        }
    }

    free(buf);
}



// Lee el descriptor del primer grupo de bloques desde el disco
int read_group_descriptors(int fd, EXT2_GroupDesc *gd, const EXT2_Superblock *sb) {
    uint32_t block_size = EXT2_BLOCK_SIZE(sb);

    // Calculamos el offset de GD:
    // - Si block_size == 1024, GD empieza en byte 1024 (offset superbloque) + 1024 = 2048
    // - Si block_size > 1024, GD ocupa el bloque 1, es decir offset = block_size
    off_t gd_offset = (block_size == 1024) ? (EXT2_SUPERBLOCK_OFFSET + block_size) : block_size;

    // Posicionarnos al inicio de la tabla de descriptores
    if (lseek(fd, gd_offset, SEEK_SET) == -1) {
        perror("Error seeking to group descriptors");
        return -1;
    }

    // Leer primer descriptor de grupo
    ssize_t bytes_read = read(fd, gd, sizeof(EXT2_GroupDesc));
    if (bytes_read != sizeof(EXT2_GroupDesc)) {
        perror("Error reading group descriptor");
        return -2;
    }

    printf("Bloque del mapa de inodos: %u\n", gd->bg_inode_bitmap);
    printf("Bloque del mapa de bloques: %u\n", gd->bg_block_bitmap);
    printf("Primer bloque de inodos: %u\n", gd->bg_inode_table);

    return 0;
}


void print_EXT2_tree(const char *image_path, const EXT2_Superblock *sb) {
    int fd = open(image_path, O_RDONLY);
    if (fd < 0) {
        perror("No se pudo abrir la imagen");
        return;
    }

    // 1) Leer descriptor de grupo
    EXT2_GroupDesc gd;
    if (read_group_descriptors(fd, &gd, sb) < 0) {
        close(fd);
        return;
    }

    // 2) Leer el inodo raíz (2) y arrancar la recursión desde él
    EXT2_Inode root;
    if (read_inode(fd, sb, &gd, 2, &root) < 0) {
        fprintf(stderr, "No se pudo leer el inodo raíz\n");
        close(fd);
        return;
    }

    // 3) Mostrar el nodo raíz y su contenido
    printf(".\n");
    print_directory_recursive(fd, sb, &gd, &root, 1);

    close(fd);
}