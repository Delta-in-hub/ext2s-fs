#ifndef __EXT2M_H__
#define __EXT2M_H__
#include <tuple>
#include "cache.h"
#include "config.h"
#include <time.h>

namespace Ext2m
{

    constexpr auto ceil(uint64_t x, uint64_t y) { return (x + y - 1) / y; }
    constexpr auto roundup(uint64_t x, uint64_t y) { return ((x + y - 1) / y) * y; }
    constexpr auto log2(uint64_t x) { return __builtin_ctzll(x); }

    // https://docs.oracle.com/cd/E19504-01/802-5750/fsfilesysappx-14/index.html
    // The default number of bytes per inode is 2048 bytes (2 Kbytes), which assumes the average size of each file is 2 Kbytes or greater.
    constexpr auto bytes_per_inode = 2 * KB;
    constexpr auto MAX_BLOCKS_PER_GROUP = 8 * BLOCK_SIZE;
    /**
     * @brief calculate the arguments of ext2
     *
     * @return <GROUP_COUNT, blocks_per_group, blocks_last_group, group_desc_block_count, inodes_per_group, inodes_table_block_count, data_block_count>;
     */
    constexpr auto block_group_calculation()
    {
        constexpr auto TOTAL_BLOCK = DISK_SIZE / BLOCK_SIZE;
        // only for BLOCK_SIZE = 1KB
        constexpr auto AVAILABLE_BLOCK = TOTAL_BLOCK - 1;

        constexpr auto MAX_INODES_PER_GROUP = 8 * BLOCK_SIZE;

        constexpr auto FULL_GROUP_COUNT = AVAILABLE_BLOCK / MAX_BLOCKS_PER_GROUP;
        constexpr auto REMAINING_BLOCK = AVAILABLE_BLOCK - MAX_BLOCKS_PER_GROUP * FULL_GROUP_COUNT;
        constexpr auto GROUP_COUNT = FULL_GROUP_COUNT + (REMAINING_BLOCK > 0 ? 1 : 0);

        constexpr auto blocks_per_group = (AVAILABLE_BLOCK - REMAINING_BLOCK) / FULL_GROUP_COUNT;
        constexpr auto blocks_last_group = REMAINING_BLOCK;

        constexpr auto group_desc_size = GROUP_COUNT * GROUP_DESC_SIZE;

        constexpr auto group_desc_block_count = ceil(group_desc_size, BLOCK_SIZE);

        // assume indes_per_group  = m
        // m * bytes_per_inode = data size
        // m * sizeof(ext2_inode) = inode size
        // remain block size = (blocks_per_group - 3 - group_desc_block_count) * BLOCK_SIZE
        // data size + inode size = remain block size

        constexpr auto remain_block_size = (blocks_per_group - 3 - group_desc_block_count) * BLOCK_SIZE;
        constexpr auto m = remain_block_size / (bytes_per_inode + INODE_SIZE);

        constexpr auto inodes_per_group = m;
        constexpr auto inodes_table_size = inodes_per_group * INODE_SIZE;
        constexpr auto inodes_table_block_count = ceil(inodes_table_size, BLOCK_SIZE);

        constexpr auto data_block_count = blocks_per_group - 3 - group_desc_block_count - inodes_table_block_count;

        static_assert(3 + group_desc_block_count + data_block_count + inodes_table_block_count == blocks_per_group, "Sum of block count is not equal to blocks_per_group");

        return std::make_tuple(FULL_GROUP_COUNT, GROUP_COUNT, blocks_per_group, blocks_last_group, group_desc_block_count, inodes_per_group, inodes_table_block_count, data_block_count);
    }

    class Ext2m
    {
    private:
        Cache &_disk;
        uint8_t _buf[BLOCK_SIZE];
        size_t full_group_count;
        size_t blocks_per_group;
        size_t inodes_per_group;
        size_t group_desc_block_count;
        size_t inodes_table_block_count;

        /**
         * @brief check if the disk is ext2-format disk
         * @return boolean
         */
        bool check_is_ext2_format()
        {
            // only works for BLOCK_SIZE = 1KB
            _disk.read_block(1, _buf);
            auto *sb = (ext2_super_block *)_buf;
            bool flag = true;
            flag &= sb->s_magic == EXT2_SUPER_MAGIC;
            flag &= sb->s_log_block_size == log2(BLOCK_SIZE);
            flag &= sb->s_first_data_block == 1;
            flag &= sb->s_inodes_per_group <= 8 * BLOCK_SIZE;
            flag &= sb->s_first_ino == EXT2_GOOD_OLD_FIRST_INO;
            flag &= sb->s_inode_size == INODE_SIZE;
            return flag;
        }

        /**
         * @brief Get the block-group's first block index
         * @param group_index
         * @return block index
         */
        unsigned get_group_index(size_t group_index)
        {
            // only works for BLOCK_SIZE = 1KB
            return group_index * blocks_per_group + 1;
        }

        /**
         * @brief Get the group's {super block} index
         *
         * @param group_index
         * @return group's super block index
         */
        unsigned get_group_super_block_index(size_t group_index)
        {
            return get_group_index(group_index) + 0;
        }

        /**
         * @brief Get the group's {group descriptor table} first block index
         * @param group_index
         * @return group descriptor table first block index
         */
        unsigned get_group_group_desc_table_index(size_t group_index)
        {
            return get_group_index(group_index) + 1;
        }

        /**
         * @brief Get the group's {block bitmap} block index
         * @param group_index
         * @return block index
         */
        unsigned get_group_block_bitmap_index(size_t group_index)
        {
            return get_group_group_desc_table_index(group_index) + group_desc_block_count;
        }

        /**
         * @brief Get the group's {inode bitmap} block index
         * @param group_index
         * @return block index
         */
        unsigned get_group_inode_bitmap_index(size_t group_index)
        {
            return get_group_block_bitmap_index(group_index) + 1;
        }
        /**
         * @brief Get the group's {inode table} first block index
         * @param group_index
         * @return block index
         */
        unsigned get_group_inode_table_index(size_t group_index)
        {
            return get_group_inode_bitmap_index(group_index) + 1;
        }

        /**
         * @brief Get the group's {data table} first block index
         * @param group_index
         * @return block index
         */
        unsigned get_group_data_table_index(size_t group_index)
        {
            return get_group_inode_table_index(group_index) + inodes_table_block_count;
        }

        void write_super_block_to_group(size_t group_index, void *_block)
        {
            _disk.write_block(get_group_super_block_index(group_index), _block);
        }

        void write_group_desc_table_to_group(size_t group_index, void *_block)
        {
            for (size_t i = 0; i < group_desc_block_count; i++)
            {
                _disk.write_block(get_group_group_desc_table_index(group_index) + i, _block + i * BLOCK_SIZE);
            }
        }

    public:
        Ext2m(Cache &cache) : _disk(cache)
        {
            full_group_count = std::get<0>(block_group_calculation());
            blocks_per_group = std::get<2>(block_group_calculation());
            inodes_per_group = std::get<5>(block_group_calculation());
            group_desc_block_count = std::get<4>(block_group_calculation());
            inodes_table_block_count = std::get<6>(block_group_calculation());

            if (not check_is_ext2_format())
                format();
        };
        ~Ext2m()
        {
            sync();
        }
        /**
         * @brief Synchronize cached writes to persistent storage
         */
        void sync()
        {
            _disk.flush_all();
        }

        /**
         * @brief Format the disk to ext2 format and add root directory
         */
        void format()
        {
            // Boot sector
            strcpy((char *)_buf, "EXT2FS , THIS THE FIRST BLOCK FOR BLCOK SIZE = 1KB , THIS IS THE BOOT SECTOR");
            _disk.write_block(0, _buf);

            // Calculate the arguments of ext2
            constexpr size_t full_group_count = std::get<0>(block_group_calculation());
            constexpr size_t group_count = std::get<1>(block_group_calculation());
            constexpr size_t blocks_per_group = std::get<2>(block_group_calculation());
            constexpr size_t blocks_last_group = std::get<3>(block_group_calculation());
            constexpr size_t group_desc_block_count = std::get<4>(block_group_calculation());
            constexpr size_t inodes_per_group = std::get<5>(block_group_calculation());
            constexpr size_t inodes_table_block_count = std::get<6>(block_group_calculation());
            constexpr size_t data_block_count = std::get<7>(block_group_calculation());

            /*
                group_count maybe greater than full_group_count
                caused by the last block group maybe smaller than the rest block group.
                that's to say blocks_last_group less than blocks_per_group
                For simplicity, we ignore the last block group in this case.
            */

            // Initialize the super block
            // see ext2.pdf page 7
            ext2_super_block super_block;
            {
                super_block.s_inodes_count = inodes_per_group * full_group_count;
                super_block.s_blocks_count = MAX_BLOCKS_PER_GROUP * full_group_count;
                super_block.s_r_blocks_count = 0;
                super_block.s_free_blocks_count = blocks_per_group * full_group_count;
                super_block.s_free_inodes_count = inodes_per_group * full_group_count;
                super_block.s_first_data_block = (BLOCK_SIZE == 1 * KB) ? 1 : 0;
                // 0 for BLOCK_SIZE = 1KB
                super_block.s_log_block_size = log2(BLOCK_SIZE / 1024);
                super_block.s_log_frag_size = super_block.s_log_block_size;
                super_block.s_blocks_per_group = blocks_per_group;
                super_block.s_frags_per_group = super_block.s_blocks_per_group;
                super_block.s_inodes_per_group = inodes_per_group;
                super_block.s_mtime = time(NULL);
                super_block.s_wtime = time(NULL);
                super_block.s_mnt_count = 0;
                super_block.s_max_mnt_count = 1024;
                super_block.s_magic = EXT2_SUPER_MAGIC;
                super_block.s_state = EXT2_VALID_FS;
                super_block.s_errors = EXT2_ERRORS_CONTINUE;
                super_block.s_minor_rev_level = 0;
                super_block.s_lastcheck = time(NULL);
                super_block.s_checkinterval = UINT32_MAX;
                super_block.s_creator_os = EXT2_OS_LINUX;
                // not strictly revision 0
                super_block.s_rev_level = 0;
                super_block.s_def_resuid = EXT2_DEF_RESUID;
                super_block.s_def_resgid = EXT2_DEF_RESGID;
                super_block.s_first_ino = EXT2_GOOD_OLD_FIRST_INO;
                super_block.s_inode_size = INODE_SIZE;
                super_block.s_block_group_nr = 0;
                super_block.s_feature_compat = 0;
                super_block.s_feature_incompat = 0;
                super_block.s_feature_ro_compat = 0;
                memset(super_block.s_uuid, 0, sizeof(super_block.s_uuid));
                strcpy((char *)super_block.s_volume_name, "*.img");
                memset(super_block.s_last_mounted, 0, sizeof(super_block.s_last_mounted));
                super_block.s_algorithm_usage_bitmap = 0;

                // TODO: may be changed later
                super_block.s_prealloc_blocks = 0;
                super_block.s_prealloc_dir_blocks = 0;

                memset(super_block.s_journal_uuid, 0, sizeof(super_block.s_journal_uuid));
                super_block.s_journal_inum = 0;
                super_block.s_journal_dev = 0;
                super_block.s_last_orphan = 0;
                memset(super_block.s_hash_seed, 0, sizeof(super_block.s_hash_seed));
                super_block.s_def_hash_version = 0;
                super_block.s_default_mount_opts = 0;
                super_block.s_first_meta_bg = 0;
            }

            // Write the super block to the disk
            memset(_buf, 0, BLOCK_SIZE);
            memcpy(_buf, &super_block, sizeof(super_block));
            for (size_t i = 0; i < full_group_count; i++)
            {
                write_super_block_to_group(i, _buf);
            }

            // Initialize the group descriptor table
            // see ext2.pdf page 16
            ext2_group_desc group_desc[full_group_count];
            {
                for (size_t i = 0; i < full_group_count; i++)
                {
                    auto &&desc = group_desc[i];
                    desc.bg_block_bitmap = get_group_block_bitmap_index(i);
                    desc.bg_inode_bitmap = get_group_inode_bitmap_index(i);
                    desc.bg_inode_table = get_group_inode_table_index(i);
                    desc.bg_free_blocks_count = blocks_per_group - 3 - group_desc_block_count - inodes_table_block_count;
                    desc.bg_free_inodes_count = inodes_per_group;
                    desc.bg_used_dirs_count = 0;
                }
            }

            // Write the group descriptor table to the disk
            {
                void *ptr = new uint8_t[BLOCK_SIZE * group_desc_block_count];
                memset(ptr, 0, BLOCK_SIZE * group_desc_block_count);
                memcpy(ptr, group_desc, sizeof(group_desc));
                // sizeof(ext2_group_desc);
                for (size_t i = 0; i < full_group_count; i++)
                {
                    write_group_desc_table_to_group(i, ptr);
                }
                delete[] ptr;
            }

            // Initialize each block group
            for (size_t i = 0; i < full_group_count; i++)
            {
                memset(_buf, 0, BLOCK_SIZE);
                size_t start_ind = get_group_block_bitmap_index(i);
                size_t end_ind = get_group_index(i) + blocks_per_group;
                while (start_ind < end_ind)
                {
                    _disk.write_block(start_ind, _buf);
                    start_ind++;
                }
            }

            // Add root directory
            // The root directory is Inode 2
            // super_block.s_first_ino == 11
            
        }
    };

} // namespace EXT2M

#endif