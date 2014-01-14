/**
 * Copyright (c) 2007-2008 dude03
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Partition-find-handle
 */
typedef struct
{
    DWORD dwHdlType; /**< Handle type */
    PStorageDevice pDev; /**< Pointer to device struct */
    PBsdPart pPart; /**< List of BSD partitions */
} PartFindHandle,  *PPartFindHandle;

/**
 * Ufs-find-handle
 */
typedef struct
{
    DWORD dwHdlType; /**< Handle type */

    HANDLE hDisk; /**< Disk handle */
    PMyFs fs; /**< Pointer to MyFs */
    DWORD p_offset; /**< LBA of BSD partition */

    UfsIno dinoden; /**< Current "root" inode number */
    UfsIno cdinoden; /**< Current inode number */
    PUfsDinode dinode; /**< Current "root" inode struct */
    PUfsDinode cdinode; /**< Current inode struct  */
    PUfsBlocksList pBlist; /**< Current "root" inodes block list  */
    DWORD dwNextDirentOffset; /**< Next dirent offset in current block */
    DWORD dwDataSize; /**< Bytes allocated in pBlock */
    DWORD dwCurrentBlock; /**< Current block number */
    PBYTE pBlock; /**< Current block data */

    VOID(*UfsReadInode)(HANDLE hDisk, PMyFs fs, DWORD p_offset, UfsIno ino, PUfsDinode dinode); /**< Pointer to function for read inode */

    PUfsBlocksList(*UfsGetBlocks)(HANDLE hDisk, PMyFs fs, DWORD p_offset, PUfsDinode dinode); /**< Pointer to function for get block list of inode */

    DWORD(*UfsReadData)(HANDLE hDisk, PMyFs fs, DWORD p_offset, PUfsDinode dinode, PUfsBlocksList blist, PBYTE lpBuffer, DWORD dwStartBlock, DWORD dwNumOfBlocks); /**< Pointer to function for read data */

} UfsFindHandle,  *PUfsFindHandle;

#define PLUGIN_LABEL "BSD fs" /**< Plugin label in in the Network Neighborhood */

#define PART_FIND_HANDLE 0 /**< Pointer to function for read data */
#define UFS_FIND_HANDLE 1 /**< Pointer to function for read data */

#define ICON_DISK 171 /**< Partition icon number */

#define ICON_SYMLINK_DIR_VALID 241 /**< Valid symlink icon number(directory) */
#define ICON_SYMLINK_FILE_VALID 246 /**< Valid symlink icon number(file) */
#define ICON_SYMLINK_INVALID 240 /**< Invalid symlink icon number */

#define GetVtbl(ufh) ((PUfsVtbl)&(ufh->UfsReadInode))

BOOL LookupPath(PUfsFindHandle ufh, char *Path);
PStorageDevice FsFindBsdPartitions();
HANDLE FsLookupPath(LPSTR pPath);

/**
 * Find Handle
 */
typedef struct
{
    DWORD dwHdlType; /**< Handle type */
} FindHandle,  *PFindHandle;
