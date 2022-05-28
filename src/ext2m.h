#ifndef __EXT2M_H__
#define __EXT2M_H__
#include "ext2_spec.h"
#include "disk.h"
#include <tuple>



namespace ext2m
{

    constexpr auto ceil(uint64_t x, uint64_t y) { return (x + y - 1) / y; }
    constexpr auto roundup(uint64_t x, uint64_t y) { return ((x + y - 1) / y) * y; }

    // https://docs.oracle.com/cd/E19504-01/802-5750/fsfilesysappx-14/index.html
    // The default number of bytes per inode is 2048 bytes (2 Kbytes), which assumes the average size of each file is 2 Kbytes or greater.
    constexpr auto bytes_per_inode = 2 * KB;

    /**
     * @brief calculate the arguments of ext2
     *
     * @return (GROUP_COUNT, blocks_per_group, blocks_last_group, group_desc_block_count, inodes_per_group, inodes_table_block_count, data_block_count);
     */
    constexpr auto block_group_calculation()
    {
        constexpr auto TOTAL_BLOCK = DISK_SIZE / BLOCK_SIZE;
        // only for BLOCK_SIZE = 1KB
        constexpr auto AVAILABLE_BLOCK = TOTAL_BLOCK - 1;

        constexpr auto MAX_BLOCKS_PER_GROUP = 8 * BLOCK_SIZE;
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

        return std::make_tuple(GROUP_COUNT, blocks_per_group, blocks_last_group, group_desc_block_count, inodes_per_group, inodes_table_block_count, data_block_count);
    }

    class ext2m
    {
    private:
        Disk &_disk;

    public:
        ext2m(/* args */);
        ~ext2m();
    };

} // namespace EXT2M

#endif