#ifndef EXT2_H
#define EXT2_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>


// Fase 1
#define EXT2_SUPERBLOCK_OFFSET 1024
#define EXT2_SUPER_MAGIC 0xEF53

// Fase 2
#define EXT2_BLOCK_SIZE(sb) (1024 << (sb)->s_log_block_size)
#define EXT2_INODE_SIZE 256  // asumiendo rev 0

#define EXT2_FT_DIR 2


#define MAX_BLOCK_SIZE 4096
#define MAX_NAME_LEN 255
#define EXT2_DIRECT_BLOCKS 12  // Bloques directos en el inodo
#define EXT2_INDIRECT_BLOCK 12 // Índice del bloque indirecto
#define EXT2_DOUBLE_INDIRECT_BLOCK 13 // Índice del bloque doble indirecto
#define EXT2_TRIPLE_INDIRECT_BLOCK 14 // Índice del bloque triple indirecto
#define EXT2_N_BLOCKS 15 // Total de bloques en el inodo


// Estructura del superbloque EXT2
#pragma pack(push, 1)
typedef struct {
    uint32_t s_inodes_count;         // 0x00: Total inodes count
    uint32_t s_blocks_count;         // 0x04: Total blocks count
    uint32_t s_r_blocks_count;       // 0x08: Reserved blocks count
    uint32_t s_free_blocks_count;    // 0x0C: Free blocks count
    uint32_t s_free_inodes_count;    // 0x10: Free inodes count
    uint32_t s_first_data_block;     // 0x14: First data block
    uint32_t s_log_block_size;       // 0x18: Block size (log2)
    uint32_t s_log_frag_size;        // 0x1C: Fragment size (log2)
    uint32_t s_blocks_per_group;     // 0x20: Blocks per group
    uint32_t s_frags_per_group;      // 0x24: Fragments per group
    uint32_t s_inodes_per_group;     // 0x28: Inodes per group
    uint32_t s_mtime;                // 0x2C: Mount time
    uint32_t s_wtime;                // 0x30: Write time
    uint16_t s_mnt_count;            // 0x34: Mount count
    uint16_t s_max_mnt_count;        // 0x36: Max mount count
    uint16_t s_magic;                // 0x38: Magic signature (0xEF53)
    uint16_t s_state;                // 0x3A: File system state
    uint16_t s_errors;               // 0x3C: Behaviour when detecting errors
    uint16_t s_minor_rev_level;      // 0x3E: Minor revision lev    el
    uint32_t s_lastcheck;            // 0x40: Time of last check
    uint32_t s_checkinterval;        // 0x44: Max time between checks
    uint32_t s_creator_os;           // 0x48: OS that created the filesystem
    uint32_t s_rev_level;            // 0x4C: Revision level
    uint16_t s_def_resuid;           // 0x50: Default uid for reserved blocks
    uint16_t s_def_resgid;           // 0x52: Default gid for reserved blocks
    uint32_t s_first_ino;
    uint16_t s_inode_size;
    uint16_t s_block_group_nr;
    uint32_t s_feature_compat;
    uint32_t s_feature_incompat;
    uint32_t s_feature_ro_compat;
    uint8_t s_uuid[16];
    char s_volume_name[16];          // 0x78: Volume name
} EXT2_Superblock;

// Fase 2
typedef struct {
    uint32_t inode;
    uint16_t rec_len;
    uint8_t name_len;
    uint8_t file_type;
    char name[];
} EXT2_DirEntry;

typedef struct {
    uint16_t mode;
    uint16_t uid;
    uint32_t size;
    uint32_t atime;
    uint32_t ctime;
    uint32_t mtime;
    uint32_t dtime;
    uint16_t gid;
    uint16_t links_count;
    uint32_t blocks;
    uint32_t flags;
    uint32_t osd1;
    uint32_t block[15]; // bloques directos, indirectos, etc.
    uint32_t generation;
    uint32_t file_acl;
    uint32_t dir_acl;
    uint32_t faddr;
    uint32_t osd2[3]; // reservados
} EXT2_Inode;

typedef struct {
    uint32_t bg_block_bitmap;
    uint32_t bg_inode_bitmap;
    uint32_t bg_inode_table;
    uint16_t bg_free_blocks_count;
    uint16_t bg_free_inodes_count;
    uint16_t bg_used_dirs_count;
    uint16_t bg_pad;
    uint8_t bg_reserved[12];
} EXT2_GroupDesc;
#pragma pack(pop)

int detect_EXT2(const char *device, EXT2_Superblock *sb);
void print_EXT2_info(const EXT2_Superblock *sb);
void print_EXT2_tree(const char *image_path, const EXT2_Superblock *sb);


#endif
