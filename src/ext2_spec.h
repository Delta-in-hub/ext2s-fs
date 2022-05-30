// EXT2 FILE SYSTEM SPECIFICATION
#ifndef __EXT2_SPEC_H__
#define __EXT2_SPEC_H__

#include <stdint.h>

#define ROOT_INODE 2

// little endian is default for intel x86
using __le32 = uint32_t;
using __le16 = uint16_t;
using __u8 = uint8_t;
using __u16 = uint16_t;
using __u32 = uint32_t;

#define EXT2_NAME_LEN 255

// 15 block  numbers  pointing  to  the  blocks  ontaining  the  data  for  this  inode.
#define EXT2_N_BLOCKS 15

// [0-11] is direct blocks
#define EXT2_DIRECT_BLOCKS 12
// [12] is the first indirect block
#define EXT2_INDIRECT_BLOCK (12)
// [13] is the double indirect block
#define EXT2_DOUBLY_INDIRECT_BLOCK (1 + 12)
// [14] is the triple indirect block
#define EXT2_TRIPLY_INDIRECT_BLOCK (1 + 1 + 12)

#define EXT2_SUPER_MAGIC 0xEF53
#define EXT2_VALID_FS 0x0001
#define EXT2_ERROR_FS 0x0002

#define EXT2_ERRORS_CONTINUE 0x0001
#define EXT2_ERRORS_RO 0x0002
#define EXT2_ERRORS_PANIC 0x0003

#define EXT2_OS_LINUX 0x00
#define EXT2_OS_HURD 0x01
#define EXT2_OS_MASTER 0x02
#define EXT2_OS_FREEBSD 0x03
#define EXT2_OS_LITES 0x04

#define EXT2_DEF_RESUID 0x0000
#define EXT2_DEF_RESGID 0x0000

#define EXT2_GOOD_OLD_FIRST_INO 11

// S_IFMT (0170000)
#define EXT2_S_IFMT 0170000

#define EXT2_S_IFSOCK 0xC000 // socket
#define EXT2_S_IFLNK 0xA000  // symbolic link
#define EXT2_S_IFREG 0x8000  // regular file
#define EXT2_S_IFBLK 0x6000  // block device
#define EXT2_S_IFDIR 0x4000  // directory
#define EXT2_S_IFCHR 0x2000  // character device
#define EXT2_S_IFIFO 0x1000  // fifo

#define EXT2_S_IRUSR 0x0100 // user read
#define EXT2_S_IWUSR 0x0080 // user write
#define EXT2_S_IXUSR 0x0040 // user execute
#define EXT2_S_IRGRP 0x0020 // group read
#define EXT2_S_IWGRP 0x0010 // group write
#define EXT2_S_IXGRP 0x0008 // group execute
#define EXT2_S_IROTH 0x0004 // others read
#define EXT2_S_IWOTH 0x0002 // others write
#define EXT2_S_IXOTH 0x0001 // others execute

#define EXT2_FT_UNKNOWN 0x00  // unknown file type
#define EXT2_FT_REG_FILE 0x01 // regular file
#define EXT2_FT_DIR 0x02      // directory
#define EXT2_FT_CHRDEV 0x03   // character device
#define EXT2_FT_BLKDEV 0x04   // block device
#define EXT2_FT_FIFO 0x05     // fifo
#define EXT2_FT_SOCK 0x06     // socket
#define EXT2_FT_SYMLINK 0x07  // symbolic link

/*
 * Structure of the super block
 */
struct ext2_super_block
{
    /*
        total number of inodes, both used and free, in the file system
        It must be equal to the sum of the inodes defined in each block group.
        This value must be lower or equal to (s_inodes_per_group * number of block groups)
    */
    __le32 s_inodes_count; /* Inodes count */
    /*
        the total number of blocks in the system including all used, free and reserved.
        This value must be lower or equal to (s_blocks_per_group * number of block groups).
        It can be lower than the previous calculation if the last block group has a smaller number of blocks than s_blocks_per_group du to volume size.
        It must be equal to the sum of the blocks defined in each block group
    */
    __le32 s_blocks_count; /* Blocks count */
    /*
        the total number of blocks reserved for the usage of the super user.
    */
    __le32 s_r_blocks_count; /* Reserved blocks count */

    // This is a sum of all free blocks of all the block groups.
    __le32 s_free_blocks_count; /* Free blocks count */

    // This is a sum of all free inodes of all the block groups.
    __le32 s_free_inodes_count; /* Free inodes count */

    // the id of the block containing the superblock structure.
    __le32 s_first_data_block; /* First Data Block */

    // block size = 1024 << s_log_block_size;
    // 0 for BLOCK_SIZE = 1024
    __le32 s_log_block_size; /* Block size */

    // equals to s_log_block_size
    __le32 s_log_frag_size; /* Fragment size */

    // the total number of blocks per group
    __le32 s_blocks_per_group; /* # Blocks per group */

    // equals to s_blocks_per_group
    __le32 s_frags_per_group; /* # Fragments per group */

    // the total number of inodes per group
    // 8 * BLOCK_SIZE = 1024 * 8 = 8192
    __le32 s_inodes_per_group; /* # Inodes per group */

    __le32 s_mtime;           /* Mount time */
    __le32 s_wtime;           /* Write time */
    __le16 s_mnt_count;       /* Mount count */
    __le16 s_max_mnt_count;   /* Maximal mount count */
    __le16 s_magic;           /* Magic signature */
    __le16 s_state;           /* File system state ,16bit value indicating the file system state. When the file system is mounted, this state is set to EXT2_ERROR_FS. After the file system was cleanly unmounted, this value is set to EXT2_VALID_FS. */
    __le16 s_errors;          /* Behaviour when detecting errors , 16bit value indicating what the file system driver should do when an error is detected. */
    __le16 s_minor_rev_level; /* minor revision level */
    __le32 s_lastcheck;       /* time of last check */
    __le32 s_checkinterval;   /* max. time between checks */
    __le32 s_creator_os;      /* OS */
    __le32 s_rev_level;       /* Revision level */
    __le16 s_def_resuid;      /* Default uid for reserved blocks ,16bit value used as the default user id for reserved blocks.*/
    __le16 s_def_resgid;      /* Default gid for reserved blocks */
    /*
     * These fields are for EXT2_DYNAMIC_REV superblocks only.
     *
     * Note: the difference between the compatible feature set and
     * the incompatible feature set is that if there is a bit set
     * in the incompatible feature set that the kernel doesn't
     * know about, it should refuse to mount the filesystem.
     *
     * e2fsck's requirements are more strict; if it doesn't know
     * about a feature in either the compatible or incompatible
     * feature set, it must abort and not try to meddle with
     * things it doesn't understand...
     */
    // EXT2_GOOD_OLD_FIRST_INO  =  11
    __le32 s_first_ino; /* First non-reserved inode */

    //
    __le16 s_inode_size;             /* size of inode structure */
    __le16 s_block_group_nr;         /* block group # of this superblock */
    __le32 s_feature_compat;         /* compatible feature set */
    __le32 s_feature_incompat;       /* incompatible feature set */
    __le32 s_feature_ro_compat;      /* readonly-compatible feature set */
    __u8 s_uuid[16];                 /* 128-bit uuid for volume */
    char s_volume_name[16];          /* volume name */
    char s_last_mounted[64];         /* directory where last mounted */
    __le32 s_algorithm_usage_bitmap; /* For compression */
    /*
     * Performance hints.  Directory preallocation should only
     * happen if the EXT2_COMPAT_PREALLOC flag is on.
     */
    __u8 s_prealloc_blocks;     /* Nr of blocks to try to preallocate*/
    __u8 s_prealloc_dir_blocks; /* Nr to preallocate for dirs */
    __u16 s_padding1;
    /*
     * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
     */
    __u8 s_journal_uuid[16]; /* uuid of journal superblock */
    __u32 s_journal_inum;    /* inode number of journal file */
    __u32 s_journal_dev;     /* device number of journal file */
    __u32 s_last_orphan;     /* start of list of inodes to delete */
    __u32 s_hash_seed[4];    /* HTREE hash seed */
    __u8 s_def_hash_version; /* Default hash version to use */
    __u8 s_reserved_char_pad;
    __u16 s_reserved_word_pad;
    __le32 s_default_mount_opts;
    __le32 s_first_meta_bg; /* First metablock block group */
    __u32 s_reserved[190];  /* Padding to the end of the block */
} __attribute__((packed));

/*
 * Structure of a blocks group descriptor
 */
struct ext2_group_desc
{
    __le32 bg_block_bitmap;      /* Blocks bitmap block */
    __le32 bg_inode_bitmap;      /* Inodes bitmap block */
    __le32 bg_inode_table;       /* Inodes table block */
    __le16 bg_free_blocks_count; /* Free blocks count */
    __le16 bg_free_inodes_count; /* Free inodes count */
    __le16 bg_used_dirs_count;   /* Directories count */
    __le16 bg_pad;
    __le32 bg_reserved[3];
} __attribute__((packed));

/*
 * Structure of an inode on the disk
 */
struct ext2_inode
{
    __le16 i_mode;        /* File mode */
    __le16 i_uid;         /* Low 16 bits of Owner Uid */
    __le32 i_size;        /* Size in bytes , Offset of the highest byte written */
    __le32 i_atime;       /* Access time */
    __le32 i_ctime;       /* Creation time */
    __le32 i_mtime;       /* Modification time */
    __le32 i_dtime;       /* Deletion Time */
    __le16 i_gid;         /* Low 16 bits of Group Id */
    __le16 i_links_count; /* Links count */
    __le32 i_blocks;      /* Blocks count,Count of disk sectors (not Ext2 blocks) in use by this inode, not counting the actual inode structure nor directory entries linking to the inode. */
    __le32 i_flags;       /* File flags */
    union
    {
        struct
        {
            __le32 l_i_reserved1;
        } linux1;
        struct
        {
            __le32 h_i_translator;
        } hurd1;
        struct
        {
            __le32 m_i_reserved1;
        } masix1;
    } osd1;                        /* OS dependent 1 */
    __le32 i_block[EXT2_N_BLOCKS]; /* Pointers to blocks */

    __le32 i_generation; /* File version (for NFS) */
    __le32 i_file_acl;   /* File ACL */
    __le32 i_dir_acl;    /* Directory ACL */
    __le32 i_faddr;      /* Fragment address */
    union
    {
        struct
        {
            __u8 l_i_frag;  /* Fragment number */
            __u8 l_i_fsize; /* Fragment size */
            __u16 i_pad1;
            __le16 l_i_uid_high; /* these 2 fields    */
            __le16 l_i_gid_high; /* were reserved2[0] */
            __u32 l_i_reserved2;
        } linux2;
        struct
        {
            __u8 h_i_frag;  /* Fragment number */
            __u8 h_i_fsize; /* Fragment size */
            __le16 h_i_mode_high;
            __le16 h_i_uid_high;
            __le16 h_i_gid_high;
            __le32 h_i_author;
        } hurd2;
        struct
        {
            __u8 m_i_frag;  /* Fragment number */
            __u8 m_i_fsize; /* Fragment size */
            __u16 m_pad1;
            __u32 m_i_reserved2[2];
        } masix2;
    } osd2; /* OS dependent 2 */
} __attribute__((packed));

/*
 * The new version of the directory entry.  Since EXT2 structures are
 * stored in intel byte order, and the name_len field could never be
 * bigger than 255 chars, it's safe to reclaim the extra byte for the
 * file_type field.
 *  Each entry is supplemented with \0 to multiples of 4.
 *  Sum of rec_len is BLOCK_SIZE.
 */
struct ext2_dir_entry_2
{
    __le32 inode;   /* Inode number */
    __le16 rec_len; /* Directory entry length */
    __u8 name_len;  /* Name length */
    __u8 file_type;
    char name[]; /* File name, up to EXT2_NAME_LEN */
                 // The name must be no longer than 255 bytes after encoding
} __attribute__((packed));

constexpr auto SUPER_BLOCK_SIZE = sizeof(ext2_super_block);
constexpr auto GROUP_DESC_SIZE = sizeof(ext2_group_desc);
constexpr auto INODE_SIZE = sizeof(ext2_inode);
constexpr auto DIR_ENTRY_SIZE = sizeof(ext2_dir_entry_2);

#endif // EXT2_H_INCLUDED