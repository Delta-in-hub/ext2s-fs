#ifndef __EXT2M_H__
#define __EXT2M_H__
#include <tuple>
#include "cache.hpp"
#include "config.hpp"
#include <time.h>
#include "bitmap.hpp"
#include <memory>
#include <functional>
#include <string>
#include <iostream>

/*
 * TODOLISTS:
 *   Not modify *_free_* variable
 */

#define EXT2M_I_BLOCK_END 0
#define EXT2M_I_BLOCK_SPARSE 1

namespace Ext2m
{

    constexpr auto ceil(uint64_t x, uint64_t y) -> uint64_t { return (x + y - 1) / y; }
    constexpr auto roundup(uint64_t x, uint64_t y) -> uint64_t { return ((x + y - 1) / y) * y; }
    constexpr auto log2(uint64_t x) -> uint64_t { return __builtin_ctzll(x); }

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

        // constexpr auto MAX_INODES_PER_GROUP = 8 * BLOCK_SIZE;

        constexpr auto FULL_GROUP_COUNT = AVAILABLE_BLOCK / MAX_BLOCKS_PER_GROUP;
        constexpr auto REMAINING_BLOCK = AVAILABLE_BLOCK - MAX_BLOCKS_PER_GROUP * FULL_GROUP_COUNT;
        constexpr auto GROUP_COUNT = FULL_GROUP_COUNT + (REMAINING_BLOCK > 0 ? 1 : 0);

        constexpr auto blocks_per_group = (AVAILABLE_BLOCK - REMAINING_BLOCK) / FULL_GROUP_COUNT;
        constexpr auto blocks_last_group = REMAINING_BLOCK;

        constexpr auto group_desc_size = FULL_GROUP_COUNT * GROUP_DESC_SIZE;

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
    struct entry
    {
        __le32 inode; /* Inode number */
        __u8 file_type;
        std::string name;
    };
    class Ext2m
    {
        uint8_t _buf[BLOCK_SIZE];

        size_t full_group_count;
        size_t blocks_per_group;
        size_t group_desc_block_count;
        size_t inodes_table_block_count;

        ext2_super_block _superb;
        ext2_group_desc *_group_desc;

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
            flag &= (1024 << (sb->s_log_block_size)) == BLOCK_SIZE;
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
        unsigned get_super_block_index(size_t group_index)
        {
            return get_group_index(group_index) + 0;
        }

        /**
         * @brief Get the group's {group descriptor table} first block index
         * @param group_index
         * @return group descriptor table first block index
         */
        unsigned get_group_desc_table_index(size_t group_index)
        {
            return get_group_index(group_index) + 1;
        }

        /**
         * @brief Get the group's {block bitmap} block index
         * @param group_index
         * @return block index
         */
        unsigned get_block_bitmap_index(size_t group_index)
        {
            return get_group_desc_table_index(group_index) + group_desc_block_count;
        }

        /**
         * @brief Get the group's {inode bitmap} block index
         * @param group_index
         * @return block index
         */
        unsigned get_inode_bitmap_index(size_t group_index)
        {
            return get_block_bitmap_index(group_index) + 1;
        }
        /**
         * @brief Get the group's {inode table} first block index
         * @param group_index
         * @return block index
         */
        unsigned get_inode_table_index(size_t group_index)
        {
            return get_inode_bitmap_index(group_index) + 1;
        }

        /**
         * @brief Get the group's {data table} first block index
         * @param group_index
         * @return block index
         */
        unsigned get_data_table_index(size_t group_index)
        {
            return get_inode_table_index(group_index) + inodes_table_block_count;
        }

        /**
         * @brief Write the block-group's super block to disk
         *
         * @param group_index
         * @param _block the content of super block to write
         */
        void write_super_block(size_t group_index, void *_block)
        {
            _disk.write_block(get_super_block_index(group_index), _block);
        }

        /**
         * @brief Write the block-group's group descriptor table to disk
         *
         * @param group_index
         * @param _block
         */
        void write_group_desc_table(size_t group_index, void *_block)
        {
            for (size_t i = 0; i < group_desc_block_count; i++)
            {
                _disk.write_block(get_group_desc_table_index(group_index) + i, (uint8_t *)_block + i * BLOCK_SIZE);
            }
        }

        /**
         * @brief Get the block-group's {block bitmap} object
         *
         * @param group_index
         * @return BitMap
         */
        BitMap get_block_bitmap(size_t group_index)
        {
            _disk.read_block(get_block_bitmap_index(group_index), _buf);
            return BitMap(_buf, blocks_per_group);
        }
        /**
         * @brief Get the block-group's {inode bitmap} object
         *
         * @param group_index
         * @return BitMap
         */
        BitMap get_inode_bitmap(size_t group_index)
        {
            _disk.read_block(get_inode_bitmap_index(group_index), _buf);
            return BitMap(_buf, inodes_per_group);
        }
        /**
         * @brief Write the block-group's {block bitmap} object to the disk.
         *
         * @param group_index
         * @param bitmap
         */
        void write_block_bitmap(size_t group_index, const BitMap &bitmap)
        {
            auto tmp = bitmap.data();
            const void *buf = tmp.first;
            unsigned size = tmp.second;
            memset(_buf, 0, BLOCK_SIZE);
            memcpy(_buf, buf, size);
            _disk.write_block(get_block_bitmap_index(group_index), _buf);
        }
        /**
         * @brief Write the block-group's {inode bitmap} object to the disk.
         *
         * @param group_index
         * @param bitmap
         */
        void write_inode_bitmap(size_t group_index, const BitMap &bitmap)
        {
            auto tmp = bitmap.data();
            const void *buf = tmp.first;
            unsigned size = tmp.second;
            memset(_buf, 0, BLOCK_SIZE);
            memcpy(_buf, buf, size);
            _disk.write_block(get_inode_bitmap_index(group_index), _buf);
        }
        /**
         * @brief read nessary information from the super block.
         *
         */
        void read_info()
        {
            _disk.read_block(1, _buf);
            auto *sb = (ext2_super_block *)_buf;

            this->blocks_per_group = sb->s_blocks_per_group;
            this->inodes_per_group = sb->s_inodes_per_group;
            this->full_group_count = sb->s_inodes_count / sb->s_inodes_per_group;

            auto _group_size = full_group_count * sizeof(ext2_group_desc);

            //        constexpr auto group_desc_block_count = ceil(group_desc_size, BLOCK_SIZE);
            this->group_desc_block_count = ceil(_group_size, BLOCK_SIZE);
            //        constexpr auto inodes_table_size = inodes_per_group * INODE_SIZE;
            // constexpr auto inodes_table_block_count = ceil(inodes_table_size, BLOCK_SIZE);

            this->inodes_table_block_count = ceil(inodes_per_group * INODE_SIZE, BLOCK_SIZE);
        }

        /**
         * @brief Get the inode all blocks indexes into a vector. Recursively find all the blocks.
         *
         * @param _block_ind
         * @param level
         * @param arr
         * @return true for continue traverse, false  for reach the end
         */
        bool __get_inode_all_blocks__(uint32_t _block_ind, int level, std::vector<uint32_t> &arr)
        {
            assert(level < 4 and level >= 0);
            if (_block_ind == EXT2M_I_BLOCK_END)
                return false;
            if (level == 0) // direct access block
            {
                arr.push_back(_block_ind);
                return true;
            }
            else // the all indirect block
            {
                std::unique_ptr<uint8_t[]> mbuf(new uint8_t[BLOCK_SIZE]);
                _disk.read_block(_block_ind, mbuf.get());
                uint8_t *_start = mbuf.get();
                uint8_t *_end = _start + BLOCK_SIZE;
                bool flag = true;
                while (_start != _end)
                {
                    __le32 bn = *(__le32 *)_start;
                    _start += sizeof(__le32);
                    flag &= __get_inode_all_blocks__(bn, level - 1, arr);
                    if (not flag)
                        return false;
                }
                return true;
            }
        }

        /**
         * @brief Add a block to an inode. Recursively.
         *
         * @param _block_ind the inode's block index
         * @param level 0 for direct access block, 1 for the first indirect block, 2 for the second indirect block, 3 for the third indirect block.
         * @param group_index
         * @return ssize_t the block index added to the inode.
         */
        ssize_t __add_block_to_inode__(uint32_t _block_ind, int level, uint32_t group_index)
        {
            switch (level)
            {
            case 1:
            {
                _disk.read_block(_block_ind, _buf);
                uint32_t *_start = (uint32_t *)_buf;
                uint32_t *_end = _start + BLOCK_SIZE / sizeof(uint32_t);
                while (_start != _end)
                {
                    if (*_start == EXT2M_I_BLOCK_END)
                    {
                        auto n = ballocs(group_index, 1).front();
                        *_start = n;
                        _disk.write_block(_block_ind, _buf);
                        return n;
                    }
                    _start++;
                }
                return -1;
                break;
            }
            case 2:
            {
                std::unique_ptr<uint8_t[]> mbuf(new uint8_t[BLOCK_SIZE]);
                _disk.read_block(_block_ind, mbuf.get());
                uint32_t *_start = (uint32_t *)mbuf.get();
                uint32_t *_end = _start + BLOCK_SIZE / sizeof(uint32_t);
                while (_start != _end)
                {
                    if (*_start == EXT2M_I_BLOCK_END)
                    {
                        auto n = ballocs(group_index, 1).front();
                        *_start = n;
                        _disk.write_block(_block_ind, mbuf.get());
                        memset(_buf, 0, BLOCK_SIZE);
                        _disk.write_block(n, _buf);
                        auto ret = __add_block_to_inode__(n, level - 1, group_index);
                        return ret;
                    }
                    _start++;
                }
                return -1;
                break;
            }
            case 3:
            {
                std::unique_ptr<uint8_t[]> mbuf(new uint8_t[BLOCK_SIZE]);
                _disk.read_block(_block_ind, mbuf.get());
                uint32_t *_start = (uint32_t *)mbuf.get();
                uint32_t *_end = _start + BLOCK_SIZE / sizeof(uint32_t);
                while (_start != _end)
                {
                    if (*_start == EXT2M_I_BLOCK_END)
                    {
                        auto n = ballocs(group_index, 1).front();
                        *_start = n;
                        _disk.write_block(_block_ind, mbuf.get());
                        memset(_buf, 0, BLOCK_SIZE);
                        _disk.write_block(n, _buf);
                        auto ret = __add_block_to_inode__(n, level - 1, group_index);
                        return ret;
                    }
                    _start++;
                }
                return -1;
                break;
            }
            default:
                assert(0);
                break;
            }
            return -1;
        }

        struct entry_block
        {
        private:
            uint8_t *_block, *_now, *_end;

        public:
            entry_block() = delete;
            entry_block(uint8_t *block) : _block(block), _now(block), _end(block + BLOCK_SIZE) {}

            /**
             * @brief Get the entry of the block.
             * @param e entry to be filled.
             * @return true for success.
             * @return false for reach end of block.
             */
            bool next_entry(entry &e)
            {
                if (_now == _end)
                    return false;
                ext2_dir_entry_2 *ent = (ext2_dir_entry_2 *)_now;
                e.inode = ent->inode;
                e.file_type = ent->file_type;
                e.name = std::string((char *)ent->name, ent->name_len);
                _now += ent->rec_len;
                return true;
            }

            /**
             * @brief Add an entry to the block
             *
             * @param e entry to be added.
             */
            bool add_entry(const entry &e)
            {
                size_t e_size = roundup(e.name.size() + 1 + sizeof(ext2_dir_entry_2), 4);
                uint8_t *_now = _block;
                uint8_t *_end = _block + BLOCK_SIZE;
                while (_now != _end)
                {
                    ext2_dir_entry_2 *ent = (ext2_dir_entry_2 *)_now;
                    size_t ent_size = roundup(ent->name_len + 1 + sizeof(ext2_dir_entry_2), 4);
                    assert(ent->rec_len >= ent_size);
                    size_t remain_size = ent->rec_len - ent_size;
                    if (remain_size >= e_size)
                    {
                        ent->rec_len = ent_size;
                        ext2_dir_entry_2 *tar = (ext2_dir_entry_2 *)(_now + ent_size);
                        tar->inode = e.inode;
                        tar->file_type = e.file_type;
                        tar->name_len = e.name.size();
                        strcpy((char *)tar->name, e.name.c_str());
                        tar->rec_len = remain_size;
                        return true;
                    }
                    _now += ent->rec_len;
                }
                return false;
            }

            /**
             * @brief Free an entry whose inode is inode_num.
             *
             * @param inode_num
             * @return true
             * @return false
             */
            bool free(uint32_t inode_num)
            {
                uint8_t *_pre = nullptr;
                uint8_t *_now = _block;
                uint8_t *_end = _block + BLOCK_SIZE;
                while (_now != _end)
                {
                    ext2_dir_entry_2 *ent = (ext2_dir_entry_2 *)_now;
                    if (ent->inode == inode_num)
                    {
                        assert(strcmp((char *)ent->name, ".") != 0);
                        assert(strcmp((char *)ent->name, "..") != 0);

                        ext2_dir_entry_2 *precd = (ext2_dir_entry_2 *)_pre;
                        precd->rec_len += ent->rec_len;

                        ent->rec_len = 0;
                        ent->inode = 0;
                        return true;
                    }
                    _pre = _now;
                    _now += ent->rec_len;
                }
                return false;
            }
        };

        uint32_t get_father_inode_num(uint32_t inode_num)
        {
            ext2_inode inode;
            get_inode(inode_num, inode);
            auto n = inode.i_block[0];
            assert(n != 0);
            _disk.read_block(n, _buf);
            entry_block eb(_buf);
            entry e;
            while (eb.next_entry(e))
            {
                if (e.name == "..")
                    return e.inode;
            }
            assert(0);
            return -1;
        }

    public:
        size_t inodes_per_group;
        Cache &_disk;

        Ext2m(Cache &cache) : _disk(cache)
        {
            if (not check_is_ext2_format())
                format();
            else
                read_info();
            _disk.read_block(1, _buf);
            this->_superb = *(ext2_super_block *)_buf;
            this->_group_desc = new ext2_group_desc[full_group_count];
            uint8_t *buf = new uint8_t[BLOCK_SIZE * group_desc_block_count];
            for (size_t i = 0; i < group_desc_block_count; i++)
            {
                _disk.read_block(2 + i, buf + i * BLOCK_SIZE);
            }
            memcpy(_group_desc, buf, sizeof(ext2_group_desc) * full_group_count);
            delete[] buf;
        };
        ~Ext2m()
        {
            sync();
            delete[] _group_desc;
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
            // constexpr size_t group_count = std::get<1>(block_group_calculation());
            constexpr size_t blocks_per_group = std::get<2>(block_group_calculation());
            // constexpr size_t blocks_last_group = std::get<3>(block_group_calculation());
            constexpr size_t group_desc_block_count = std::get<4>(block_group_calculation());
            constexpr size_t inodes_per_group = std::get<5>(block_group_calculation());
            constexpr size_t inodes_table_block_count = std::get<6>(block_group_calculation());
            // constexpr size_t data_block_count = std::get<7>(block_group_calculation());

            this->blocks_per_group = blocks_per_group;
            this->full_group_count = full_group_count;
            this->group_desc_block_count = group_desc_block_count;
            this->inodes_per_group = inodes_per_group;
            this->inodes_table_block_count = inodes_table_block_count;

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
                // TODO:  maybe more than 1024
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

            // Initialize the group descriptor table
            // see ext2.pdf page 16
            ext2_group_desc group_desc[full_group_count];
            {
                for (size_t i = 0; i < full_group_count; i++)
                {
                    auto &&desc = group_desc[i];
                    desc.bg_block_bitmap = get_block_bitmap_index(i);
                    desc.bg_inode_bitmap = get_inode_bitmap_index(i);
                    desc.bg_inode_table = get_inode_table_index(i);
                    desc.bg_free_blocks_count = blocks_per_group - 3 - group_desc_block_count - inodes_table_block_count;
                    desc.bg_free_inodes_count = inodes_per_group;
                    desc.bg_used_dirs_count = 0;
                }
            }

            // Some block used for supber block , group descriptor table, inode table, etc.
            super_block.s_free_blocks_count -= (3 + group_desc_block_count + inodes_table_block_count) * full_group_count;

            // Write the super block to the disk
            memset(_buf, 0, BLOCK_SIZE);
            memcpy(_buf, &super_block, sizeof(super_block));
            for (size_t i = 0; i < full_group_count; i++)
            {
                write_super_block(i, _buf);
            }

            // Write the group descriptor table to the disk
            {
                auto *ptr = new uint8_t[BLOCK_SIZE * group_desc_block_count];
                memset(ptr, 0, BLOCK_SIZE * group_desc_block_count);
                memcpy(ptr, group_desc, sizeof(group_desc));
                for (size_t i = 0; i < full_group_count; i++)
                {
                    write_group_desc_table(i, ptr);
                }
                delete[] ptr;
            }

            // Initialize each block group
            for (size_t i = 0; i < full_group_count; i++)
            {
                memset(_buf, 0, BLOCK_SIZE);
                size_t start_ind = get_inode_bitmap_index(i);
                size_t end_ind = get_group_index(i) + blocks_per_group;
                while (start_ind < end_ind)
                {
                    _disk.write_block(start_ind, _buf);
                    start_ind++;
                }
                auto &&bm = get_block_bitmap(i);
                size_t _end = 3 + group_desc_block_count + inodes_table_block_count;
                for (size_t _j = 0; _j < _end; _j++)
                {
                    bm.set(_j);
                }
                write_block_bitmap(i, bm);
            }

            sync();

            // // super_block.s_first_ino == 11

            // Add root directory
            auto &&bm = get_inode_bitmap(0);
            // The root directory is Inode 2
            bm.set(2);
            write_inode_bitmap(0, bm);
            // see ext2.pdf page 18
            ext2_inode root_ino;
            {
                // Chmod 0755 (chmod a+rwx,g-w,o-w,ug-s,-t) sets permissions so that, (U)ser / owner can read, can write and can execute. (G)roup can read, can't write and can execute. (O)thers can read, can't write and can execute.
                root_ino.i_mode = EXT2_S_IFDIR | 0755;
                // root user always has uid =0 and gid = 0
                root_ino.i_uid = 0;
                root_ino.i_size = 0;
                root_ino.i_atime = time(NULL);
                root_ino.i_ctime = time(NULL);
                root_ino.i_mtime = time(NULL);
                root_ino.i_dtime = 0;
                root_ino.i_gid = 0;
                // root 's father is root itself
                root_ino.i_links_count = 2;

                root_ino.i_blocks = 1;
                root_ino.i_flags = 0;

                root_ino.i_generation = 0;
                root_ino.i_file_acl = 0;
                root_ino.i_dir_acl = 0;
                root_ino.i_faddr = 0;

                /*
                In the original implementation of Ext2, a value of 0 in this array effectively terminated it with no further
                block defined.
                In sparse files, it is possible to have some blocks allocated and some others not yet allocated
                with the value 0 being used to indicate which blocks are not yet allocated for this file.
                */
                memset(root_ino.i_block, 0, sizeof(root_ino.i_block));
                root_ino.i_block[0] = ballocs(0).front();
                init_entry_block(_buf, 2, 2);
                _disk.write_block(root_ino.i_block[0], _buf);
            }
            write_inode(2, root_ino);
            sync();
        };

        /**
         * @brief Get all entrys belong to the inode.
         *
         * @param inode_num
         * @return std::vector<entry>
         */
        std::vector<entry> get_inode_all_entry(uint32_t inode_num)
        {
            std::vector<entry> ret;
            auto all_blocks = get_inode_all_blocks(inode_num);
            for (auto &&i : all_blocks)
            {
                _disk.read_block(i, _buf);
                entry_block eb(_buf);
                entry e;
                while (eb.next_entry(e))
                {
                    if (ret.size() > 2 and (e.name == "." or e.name == ".."))
                        continue;
                    ret.push_back(e);
                }
            }
            return ret;
        }

        /**
         * @brief Find an avaialble inode index, and modify the inode bitmap.
         *
         * @return inode num , if failed , return 0.
         */
        size_t ialloc()
        {
            for (size_t i = 0; i < full_group_count; i++)
            {
                // inode num starts from 1
                size_t start_inode_n = i * inodes_per_group + 1;
                auto &&bitmap = get_inode_bitmap(i);
                uint32_t start = 0;
                while (start != (uint32_t)-1)
                {
                    start = bitmap.nextBit(start);
                    if (start != (uint32_t)-1)
                    {
                        if (start + start_inode_n < _superb.s_first_ino)
                        {
                            start++;
                            continue;
                        }
                        bitmap.set(start);
                        write_inode_bitmap(i, bitmap);
                        return start + start_inode_n;
                    }
                }
            }
            return 0;
        }

        void init_inode(ext2_inode &inode, __le16 mode, __le16 uid, __le16 gid)
        {
            memset(&inode, 0, sizeof(ext2_inode));
            inode.i_mode = mode;
            inode.i_uid = uid;
            inode.i_gid = gid;
            inode.i_links_count = 2;
            inode.i_blocks = 1;
            inode.i_dtime = 0;
            inode.i_ctime = time(NULL);
            inode.i_mtime = inode.i_ctime;
            inode.i_atime = inode.i_ctime;
            inode.i_size = 0;
        }

        /**
         * @brief Get the free block indexes, and modify the block bitmap. Try to find the consecutive blocks in the same group firstly.
         *
         * @param group_id
         * @param count
         * @return std::vector<size_t> , if failed , return empty vector.
         */
        std::vector<uint32_t> ballocs(size_t group_id, size_t count = 1)
        {
            // For simplicity, just find any free block in any group
            std::vector<uint32_t> ret;
            for (size_t i = group_id; (i + 1) % full_group_count != group_id; i = (i + 1) % full_group_count)
            {
                size_t group_ind = get_group_index(i);
                auto &&bitmap = get_block_bitmap(i);
                bool _bitmap_changed = false;
                uint32_t start = 0;
                while (start != (uint32_t)-1)
                {
                    start = bitmap.nextBit(start);
                    if (start != (uint32_t)-1)
                    {
                        ret.push_back(start + group_ind);
                        bitmap.set(start);
                        _bitmap_changed = true;
                        count--;
                        if (count == 0)
                        {
                            write_block_bitmap(i, bitmap);
                            return ret;
                        }
                    }
                }
                if (_bitmap_changed)
                {
                    write_block_bitmap(i, bitmap);
                }
            }
            assert(0);
            return {};
        }

        /**
         * @brief Get a free block index, and modify the block bitmap.
         *
         * @param group_id
         * @return uint32_t
         */
        uint32_t balloc(size_t group_id)
        {
            return ballocs(group_id, 1).at(0);
        }

        /**
         * @brief Free a block, and modify the block bitmap.
         *
         * @param block_idx
         */
        void bfree(uint32_t block_idx)
        {
            if (block_idx == 0)
                return;
            auto group_idx = (block_idx - 1) / blocks_per_group;
            auto offset = (block_idx - 1) % blocks_per_group;
            assert(group_idx < full_group_count);
            assert(offset >= 3 + group_desc_block_count + inodes_table_block_count);
            auto &&bitmap = get_block_bitmap(group_idx);
            bitmap.reset(offset);
            write_block_bitmap(group_idx, bitmap);
        }

        /**
         * @brief Free an inode, and modify the inode bitmap.
         *
         * @param inode_num
         */

        void ifree(uint32_t inode_num)
        {
            if (inode_num < _superb.s_first_ino)
                return;
            inode_num--;
            size_t group_index = inode_num / inodes_per_group;
            size_t ind = inode_num % inodes_per_group;
            assert(group_index < full_group_count);

            auto &&all_blocks = get_inode_all_blocks(inode_num);
            for (auto &&i : all_blocks)
            {
                bfree(i);
            }
            auto &&bm = get_inode_bitmap(group_index);
            bm.reset(ind);
            write_inode_bitmap(group_index, bm);
        }

        /**
         * @brief Write the inode object to the disk.
         * ! Caution: Not modify the inode bitmap
         * @param inode_num
         * @param inode
         */
        void write_inode(size_t inode_num, const struct ext2_inode &inode)
        {
            assert(inode_num >= 1);
            inode_num--;
            size_t group_index = inode_num / inodes_per_group;
            size_t ind = inode_num % inodes_per_group;
            assert(group_index < full_group_count);
            auto inode_table_block_ind = get_inode_table_index(group_index);

            //  BLOCK_SIZE / INODE_SIZE; // 8 inodes per block
            size_t block_index = ind / 8;
            size_t offset = ind % 8;

            _disk.read_block(inode_table_block_ind + block_index, _buf);
            auto *inode_table = (ext2_inode *)_buf;
            inode_table[offset] = inode;

            _disk.write_block(inode_table_block_ind + block_index, _buf);
        }

        void init_entry_block(void *_block, uint32_t inode_num, uint32_t father_inode_num)
        {
            memset(_block, 0, BLOCK_SIZE);
            ext2_dir_entry_2 *_start = (ext2_dir_entry_2 *)_block;
            _start->inode = inode_num;
            _start->name_len = 1;
            _start->file_type = EXT2_FT_DIR;
            _start->name[0] = '.';
            _start->name[1] = '\0';
            _start->rec_len = 12;

            _start = (ext2_dir_entry_2 *)((uint8_t *)_start + 12);
            _start->inode = father_inode_num;
            _start->name_len = 2;
            _start->file_type = EXT2_FT_DIR;
            _start->name[0] = '.';
            _start->name[1] = '.';
            _start->name[2] = '\0';
            _start->rec_len = BLOCK_SIZE - 12;
        }

        /**
         * @brief Add an entry to the inode.
         *
         * @param inode_num
         * @param ent
         */
        void add_entry_to_inode(uint32_t inode_num, const entry &ent)
        {
            assert(ent.name.size() <= EXT2_NAME_LEN);
            auto &&all_blocks = get_inode_all_blocks(inode_num);
            for (auto &&i : all_blocks)
            {
                _disk.read_block(i, _buf);
                entry_block eb(_buf);
                if (eb.add_entry(ent))
                {
                    _disk.write_block(i, _buf);
                    return;
                }
            }
            auto n = add_block_to_inode(inode_num);
            assert(n != (uint32_t)-1);
            init_entry_block(_buf, inode_num, get_father_inode_num(inode_num));
            entry_block eb(_buf);
            bool flag = eb.add_entry(ent);
            assert(flag);
            _disk.write_block(n, _buf);
        }
        /**
         * @brief Get the inode object with its inode num.
         * ! Caution: Not modify the inode bitmap
         * @param inode_num
         * @param inode
         */
        void get_inode(size_t inode_num, struct ext2_inode &inode)
        {
            assert(inode_num >= 1);
            inode_num--;
            size_t group_index = inode_num / inodes_per_group;
            size_t ind = inode_num % inodes_per_group;
            assert(group_index < full_group_count);
            auto inode_table_block_ind = get_inode_table_index(group_index);

            //     BLOCK_SIZE / INODE_SIZE; // 8 inodes per block
            size_t block_index = ind / 8;
            size_t offset = ind % 8;

            _disk.read_block(inode_table_block_ind + block_index, _buf);
            auto *inode_table = (ext2_inode *)_buf;
            inode = inode_table[offset];
        }

        /**
         * @brief Free an entry of the inode.
         *
         * @param inode_num the directory inode.
         * @param free_inode the inode to be freed.
         */
        void free_entry_to_inode(uint32_t inode_num, uint32_t free_inode)
        {
            auto &&all_blocks = get_inode_all_blocks(inode_num);
            for (auto &&i : all_blocks)
            {
                _disk.read_block(i, _buf);
                entry_block eb(_buf);
                if (eb.free(free_inode))
                {
                    _disk.write_block(i, _buf);
                    return;
                }
            }
            assert(0);
        }

        /**
         * @brief Get all blocks belongs to an inode.
         *
         * @param inode_num
         * @return std::vector<uint32_t> blocks indexes.
         */
        std::vector<uint32_t> get_inode_all_blocks(size_t inode_num)
        {
            ext2_inode inode;
            get_inode(inode_num, inode);
            bool flag = true;
            std::vector<uint32_t> indexs;
            for (int i = 0; i < EXT2_N_BLOCKS; i++)
            {
                auto n = inode.i_block[i];
                if (i < EXT2_DIRECT_BLOCKS) // direct access block
                {
                    flag &= __get_inode_all_blocks__(n, 0, indexs);
                }
                else if (i == EXT2_INDIRECT_BLOCK) // the first indirect block
                {
                    flag &= __get_inode_all_blocks__(n, 1, indexs);
                }
                else if (i == EXT2_DOUBLY_INDIRECT_BLOCK) // the second indirect block
                {
                    flag &= __get_inode_all_blocks__(n, 2, indexs);
                }
                else if (i == EXT2_TRIPLY_INDIRECT_BLOCK) // the third indirect block
                {
                    flag &= __get_inode_all_blocks__(n, 3, indexs);
                }
                else
                {
                    assert(0);
                }
                if (not flag)
                    break;
            }
            return indexs;
        }

        /**
         * @brief Add a block to an inode.
         *
         * @param inode_num
         * @return uint32_t
         */
        uint32_t add_block_to_inode(size_t inode_num)
        {
            size_t group_index = inode_num / inodes_per_group;

            ext2_inode inode;
            get_inode(inode_num, inode);

            // direct access
            for (int i = 0; i < EXT2_DIRECT_BLOCKS; i++)
            {
                auto nb = inode.i_block[i];
                if (nb == EXT2M_I_BLOCK_END)
                {
                    auto pos = ballocs(group_index, 1).front();
                    inode.i_block[i] = pos;
                    write_inode(inode_num, inode);
                    return pos;
                }
            }

            // first indirect access
            if (inode.i_block[EXT2_INDIRECT_BLOCK] == EXT2M_I_BLOCK_END)
            {
                auto pos = ballocs(group_index, 1).front();
                inode.i_block[EXT2_INDIRECT_BLOCK] = pos;
                write_inode(inode_num, inode);

                memset(_buf, 0, BLOCK_SIZE);
                _disk.write_block(pos, _buf);
            }
            ssize_t ret = __add_block_to_inode__(inode.i_block[EXT2_INDIRECT_BLOCK], 1, group_index);
            if (ret != -1)
                return ret;

            // second indirect access
            if (inode.i_block[EXT2_DOUBLY_INDIRECT_BLOCK] == EXT2M_I_BLOCK_END)
            {
                auto pos = ballocs(group_index, 1).front();
                inode.i_block[EXT2_DOUBLY_INDIRECT_BLOCK] = pos;
                write_inode(inode_num, inode);
                memset(_buf, 0, BLOCK_SIZE);
                _disk.write_block(pos, _buf);
            }
            ret = __add_block_to_inode__(inode.i_block[EXT2_DOUBLY_INDIRECT_BLOCK], 2, group_index);
            if (ret != -1)
                return ret;

            // third indirect access
            if (inode.i_block[EXT2_TRIPLY_INDIRECT_BLOCK] == EXT2M_I_BLOCK_END)
            {
                auto pos = ballocs(group_index, 1).front();
                inode.i_block[EXT2_TRIPLY_INDIRECT_BLOCK] = pos;
                write_inode(inode_num, inode);
                memset(_buf, 0, BLOCK_SIZE);
                _disk.write_block(pos, _buf);
            }
            ret = __add_block_to_inode__(inode.i_block[EXT2_TRIPLY_INDIRECT_BLOCK], 3, group_index);
            return ret;
        }
    };

} // namespace EXT2M

#endif