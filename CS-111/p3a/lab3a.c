
#include<stdio.h>
#include<stdlib.h>
#include "ext2_fs.h"
#include<unistd.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<string.h>
#include<fcntl.h>
#include<math.h>
#include<inttypes.h>
#include<time.h>
#include<errno.h>


int BLOCKSIZE = 1024;
int fd;
int logfd;
struct ext2_super_block superblock;
struct ext2_inode inode;

void super_block()
{
	size_t superblock_bufsize = sizeof(struct ext2_super_block);
	if (pread(fd, &superblock, superblock_bufsize, 1024) < 0)
	{
		fprintf(stderr, "Cannot read superblock info.\n");
		exit(2);
	}
	dprintf(logfd, "%s,%d,%d,%d,%d,%d,%d,%d\n", "SUPERBLOCK", superblock.s_blocks_count, superblock.s_inodes_count, 1024<<superblock.s_log_block_size, superblock.s_inode_size, superblock.s_blocks_per_group, superblock.s_inodes_per_group, superblock.s_first_ino);
}

void bfree (int group_count, struct ext2_group_desc* group)
{
	/* bsize = EXT2_MIN_BLOCK_SIZE << s_log_block_size	*/
 	int bpg = superblock.s_blocks_per_group;
 	//int ipg = superblock.s_inodes_per_group;
 	uint8_t buffer;
 	int bsize = 1024 << superblock.s_log_block_size;
 	int j, k;
 	int mask = 1;
 	for(j = 0; j != group_count; j++)
 	{
 		int group_start = bpg * j;
 		for(k = 0; k != bsize; k++)
 		{
 			int buff_start = group_start + 8 * k;
 			if (pread(fd, &buffer, 1, group[j].bg_block_bitmap * bsize + k) < 0)
 			{
 				fprintf(stderr, "Cannot read info for free blocks\n");
 				exit(2);
 			}
 			int n;
 			for(n = 0; n != 8; n++)
 			{
 				if ((buffer & (mask << n)) == 0)
 					dprintf(logfd, "%s,%d\n", "BFREE", buff_start+n+1);
 			}
 		}
 	}
}

void ifree (int group_count, struct ext2_group_desc* group)
{
	/* bsize = EXT2_MIN_BLOCK_SIZE << s_log_block_size	*/
 	int bpg = superblock.s_blocks_per_group;
 	//int ipg = superblock.s_inodes_per_group;
 	uint8_t buffer;
 	int bsize = 1024 << superblock.s_log_block_size;
 	int j, k;
 	int mask = 1;
 	for(j = 0; j != group_count; j++)
 	{
 		int group_start = bpg * j;
 		for(k = 0; k != bsize; k++)
 		{
 			int buff_start = group_start + 8 * k;
 			if(pread(fd, &buffer, 1, group[j].bg_inode_bitmap * bsize + k) < 0)
 			{
 				fprintf(stderr, "Cannot read info for free inodes.\n");
 				exit(2);
 			}
 			int n;
 			for(n = 0; n != 8; n++)
 			{
 				if ((buffer & (mask << n)) == 0)
 					dprintf(logfd, "%s,%d\n", "IFREE", buff_start+n+1);
 			}
 		}
 	}
}

void directory_entry(int k, char filetype)
{
	if(filetype == 'd')
 		{
 			int p = 0;
 			int indicator = 0;
 			for (p = 0; p != 12; p++)
 			{
 				struct ext2_dir_entry directory;
 				int dir_bufsize = sizeof(struct ext2_dir_entry);
 				if (inode.i_block[p] == 0)
 					break;
				int DIRECTORY_START = BLOCKSIZE * inode.i_block[p];
 				int directory_offset = 0;

 				while(directory_offset < BLOCKSIZE)
 				{
 					if(pread(fd, &directory, dir_bufsize, DIRECTORY_START + directory_offset) < 0)
 					{
 						fprintf(stderr, "Cannot read DIRENT info.\n");
 						exit(2);
 					}
 					if(directory.inode == 0)
 					{
 						indicator = 1;
 						break;
 					}
 					dprintf(logfd, "%s,%d,%d,%d,%d,%d,'%s'\n", "DIRENT", k, directory_offset, directory.inode, directory.rec_len, directory.name_len, directory.name);
 					directory_offset += directory.rec_len;
 				}
 				if(indicator == 0)
 					break;
 			}
 		}
}

void indirect_block_reference(int fd, int indierct_start, int level, int inode_index, int block_index) 
{
    int indirect_block[BLOCKSIZE/4];
    if(block_index != 0) {
        if (pread(fd, &indirect_block, 1024, block_index*1024) < 0)
        {
        	fprintf(stderr, "Cannot read indirect block reference info.\n");
        	exit(2);
        }
        int i;
        for (i=0; i<256; i++) {
            int offset = indierct_start + i;
            if(indirect_block[i] != 0) 
                dprintf(logfd, "INDIRECT,%d,%d,%d,%d,%d\n",inode_index,level,offset,block_index,indirect_block[i]);
            if(level > 1)
                indirect_block_reference(fd, offset, level - 1, inode_index, indirect_block[i]);
        }
    }
}

void inode_summary(int group_count)
{
	int IT_START = 1024 + 4 * BLOCKSIZE;
 	int j = 0;
 	int k = 0;
 	for (j = 0; j != group_count; j++)
 	{
 		//int group_start = bpg * j;
 		for(k = 1; k <= (int)superblock.s_inodes_per_group; k++)
 		{
 			int inode_bufsize = sizeof(struct ext2_inode);
 			int it_offset = IT_START + (k - 1)*inode_bufsize;
 			if (pread(fd, &inode, inode_bufsize, it_offset) < 0)
 			{
 				fprintf(stderr, "Cannot read inode info\n" );
 				exit(2);
 			}
 			if(inode.i_mode && inode.i_links_count)
 			{
 				char filetype;
 				switch(inode.i_mode >> 12)
 				{
 					case 0xA:
 						filetype = 's';
 						break;
 					case 0x8:
 						filetype = 'f';
 						break;
 					case 0x4:
 						filetype = 'd';
 						break;
 					default:
 						filetype = '?';
 				}
 				dprintf(logfd, "%s,%d,%c,%o,%d,%d,%d,", "INODE", k, filetype, inode.i_mode & 0xFFF, inode.i_uid, inode.i_gid, inode.i_links_count);

 				time_t c_time, m_time, a_time;
 				struct tm* tmp;
 				c_time = (time_t)inode.i_ctime;
 				m_time = (time_t)inode.i_mtime;
 				a_time = (time_t)inode.i_atime;

 				//creation time
 				tmp = gmtime(&c_time);
 				dprintf(logfd, "%02d/%02d/%2d %02d:%02d:%02d,", tmp->tm_mon+1, tmp->tm_mday, (tmp->tm_year)%100, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
 				//modification time
 				tmp = gmtime(&m_time);
 				dprintf(logfd, "%02d/%02d/%2d %02d:%02d:%02d,", tmp->tm_mon+1, tmp->tm_mday, (tmp->tm_year)%100, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
 				//last access time
 				tmp = gmtime(&a_time);
 				dprintf(logfd, "%02d/%02d/%2d %02d:%02d:%02d,", tmp->tm_mon+1, tmp->tm_mday, (tmp->tm_year)%100, tmp->tm_hour, tmp->tm_min, tmp->tm_sec);

 				dprintf(logfd, "%d,%d", inode.i_size, inode.i_blocks);

 				if(filetype == 's')
 				{
 					dprintf(logfd, ",%d\n", inode.i_block[0]);
 				}
 				else
 				{
 					int l = 0;
 					for(l = 0; l != 15; l++)
 						dprintf(logfd, ",%d", inode.i_block[l]);
 					dprintf(logfd, "\n");
 				}

 				directory_entry(k, filetype);

 				indirect_block_reference(fd, 12, 1, superblock.s_inodes_per_group * j + k, inode.i_block[12]);
 				indirect_block_reference(fd, 12+256, 2, superblock.s_inodes_per_group * j + k, inode.i_block[13]);
 				indirect_block_reference(fd, 12+256+(256<<8), 3, superblock.s_inodes_per_group * j + k, inode.i_block[14]);
 			}
 		}
 	}
}

int main(int argc, char* argv[])
{
	char* filename;
	if (argc != 2)
	{
		fprintf(stderr, "Illegal Parameter.\n");
		exit(1);
	}
	else
	{
		filename = malloc((strlen(argv[1]) + 1) * sizeof(char));
		filename = argv[1];
	}

	fd = open(filename, O_RDONLY);
	if(fd == -1)
	{
		fprintf(stderr, "Illegal file name.\n");
		exit(1);
	}
	logfd = creat("log.csv", S_IRWXU);
	if(logfd == -1)
	{
		fprintf(stderr, "Cannot create log.\n");
		exit(2);
	}

	//SUPERBLOCK
	super_block();

	//GROUP
	int group_count = superblock.s_blocks_count / superblock.s_blocks_per_group;
	int last_blocks = superblock.s_blocks_count % superblock.s_blocks_per_group;
	if(last_blocks != 0)
			group_count += 1;
	int last_inodes = superblock.s_inodes_count % superblock.s_inodes_per_group;
	size_t group_bufsize = sizeof(struct ext2_group_desc);
	int block_start_offset = 1024 + BLOCKSIZE;
	struct ext2_group_desc* group = malloc(group_count * group_bufsize);
	int i;
	for (i = 0; i != group_count - 1; i++)
	{
		if(pread(fd, &group[i], group_bufsize, block_start_offset + i * group_bufsize) < 0)
		{
			fprintf(stderr, "Cannot read group info.\n");
			exit(2);
		}
		dprintf(logfd, "%s,%d,%d,%d,%d,%d,%d,%d,%d\n", "GROUP", i, superblock.s_blocks_per_group, superblock.s_inodes_per_group, group[i].bg_free_blocks_count, group[i].bg_free_inodes_count, group[i].bg_block_bitmap, group[i].bg_inode_bitmap, group[i].bg_inode_table);
	}
 	if (last_blocks != 0 && last_inodes != 0)
 	{
		if(pread(fd, &group[i], group_bufsize, block_start_offset + i * group_bufsize) < 0)
		{
			fprintf(stderr, "Cannot read group info.\n");
			exit(2);
		}
		dprintf(logfd, "%s,%d,%d,%d,%d,%d,%d,%d,%d\n", "GROUP", i, last_blocks, last_inodes, group[i].bg_free_blocks_count, group[i].bg_free_inodes_count, group[i].bg_block_bitmap, group[i].bg_inode_bitmap, group[i].bg_inode_table);
 	}
 	else if (last_blocks != 0)
 	{
 		if(pread(fd, &group[i], group_bufsize, block_start_offset + i * group_bufsize) < 0)
		{
			fprintf(stderr, "Cannot read group info.\n");
			exit(2);
		}
		dprintf(logfd, "%s,%d,%d,%d,%d,%d,%d,%d,%d\n", "GROUP", i, last_blocks, superblock.s_inodes_per_group, group[i].bg_free_blocks_count, group[i].bg_free_inodes_count, group[i].bg_block_bitmap, group[i].bg_inode_bitmap, group[i].bg_inode_table);
 	}
 	else if (last_inodes != 0)
 	{
 		if(pread(fd, &group[i], group_bufsize, block_start_offset + i * group_bufsize) < 0)
		{
			fprintf(stderr, "Cannot read group info.\n");
			exit(2);
		}
		dprintf(logfd, "%s,%d,%d,%d,%d,%d,%d,%d,%d\n", "GROUP", i, superblock.s_blocks_per_group, last_inodes, group[i].bg_free_blocks_count, group[i].bg_free_inodes_count, group[i].bg_block_bitmap, group[i].bg_inode_bitmap, group[i].bg_inode_table);
 	}
 	else
 	{
		if (pread(fd, &group[i], group_bufsize, block_start_offset + i * group_bufsize) < 0)
		{
			fprintf(stderr, "Cannot read group info.\n");
			exit(2);
		}
		dprintf(logfd, "%s,%d,%d,%d,%d,%d,%d,%d,%d\n", "GROUP", i, superblock.s_blocks_per_group, superblock.s_inodes_per_group, group[i].bg_free_blocks_count, group[i].bg_free_inodes_count, group[i].bg_block_bitmap, group[i].bg_inode_bitmap, group[i].bg_inode_table);
 	}
 	
 	//BFREE
 	bfree(group_count, group);

 	//IFREE
 	ifree(group_count, group);
 	
 	//INODE
 	inode_summary(group_count);

 	//free(filename);
 	free(group);
 	close(fd);
 	close(logfd);
 	int size;
 	char out_buf[256];
 	int outfd = open("./log.csv", O_RDONLY);
 	while((size=read(outfd, out_buf, 256))>0)
 	{
 		if(size < 0)
 		{
 			fprintf(stderr, "Cannot read from .csv.\n");
 			exit(2);
 		}
 		if(write(1, out_buf, size) < 0)
 		{
 			fprintf(stderr, "Cannot write to stdout.\n");
 			exit(2);
 		}
 	}
 	close (outfd);
 	exit(0);
}