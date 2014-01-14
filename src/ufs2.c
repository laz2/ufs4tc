/*
 * Copyright (c) 2007-2008 dude03
 * All rights reserved.
 * 
 * Redistribution and use in source	and	binary forms, with or without
 * modification, are permitted provided	that the following conditions
 * are met:
 * 1. Redistributions of source	code must retain the above copyright
 *	  notice, this list	of conditions and the following	disclaimer.
 * 2. Redistributions in binary	form must reproduce	the	above copyright
 *	  notice, this list	of conditions and the following	disclaimer in the
 *	  documentation	and/or other materials provided	with the distribution.
 * 3. The name of the author may not be	used to	endorse	or promote products
 *	  derived from this	software without specific prior	written	permission.
 * 
 * THIS	SOFTWARE IS	PROVIDED BY	THE	AUTHOR ``AS	IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A	PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR	BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL,	EXEMPLARY, OR CONSEQUENTIAL	DAMAGES	(INCLUDING,	BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;	LOSS OF	USE,
 * DATA, OR	PROFITS; OR	BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY,	WHETHER	IN CONTRACT, STRICT	LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR	OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF	SUCH DAMAGE.
 */

#include <windows.h>

#include "dinode.h"
#include "dir.h"
#include "diskio.h"
#include "fs.h"
#include "stdint.h"
#include "ufs.h"

extern HANDLE g_hProcessHeap; /**< Current process heap	*/

/**
 * Ufs2ReadInode read inode	with number	ino	from BSD partition.	For	UFS2 partitions.
 *
 * @param[in] pUfsPart Pointer to UFS partition	structure.
 * @param[in] ino Number of	inode.
 * @param[out] dinode Ufs2ReadInode	store inode	data here.
 *
 */
VOID Ufs2ReadInode(PUfsPartition pUfsPart, UfsIno ino, PUfsDinode dinode)
{
    HANDLE hDisk = pUfsPart->hDisk;
    PMyFs fs = pUfsPart->fs;
    DWORD p_offset = pUfsPart->p_offset;

    ReadDisk(pUfsPart->hDisk, (PBYTE)(&dinode->din), 
    /* Partition	offset */
    (((DWORD64)p_offset) << 9) + 
    /* Inode	block offset */
    (((DWORD64)cgimin2(fs, ino_to_cg(fs, ino))) << fs->fs_fshift) + 
    /* Current inode	offset in cg */
    ((DWORD64)(ino % fs->fs_ipg)) *sizeof(struct ufs2_dinode), sizeof(struct ufs2_dinode));

    dinode->mode = dinode->din.ufs2.di_mode; /* Store inodes mode for public use */
    dinode->mtime = dinode->din.ufs2.di_mtime; /* Store inodes mtime for public use */
    dinode->size = dinode->din.ufs2.di_size; /* Store inodes size for public use */
}

/**
 * Ufs2GetBlocks return	block list for inode "dinode". For UFS2	partitions.
 *
 * @param[in] pUfsPart Pointer to UFS partition	structure.
 * @param[in] dinode current inode.
 *
 * @return Pointer to block	list when successful, NULL when	failure. 
 *
 */
PUfsBlocksList Ufs2GetBlocks(PUfsPartition pUfsPart, PUfsDinode dinode)
{
    struct ufs2_dinode *di;
    PUfsBlocksList pBlist;

    HANDLE hDisk = pUfsPart->hDisk;
    PMyFs fs = pUfsPart->fs;
    DWORD p_offset = pUfsPart->p_offset;

    di = &(dinode->din.ufs2);

    pBlist = (PUfsBlocksList)HeapAlloc(g_hProcessHeap, 0, sizeof(UfsBlocksList));
    if (pBlist)
    {

        if (!di->di_blocks)
        {
            ZeroMemory(pBlist, sizeof(UfsBlocksList));
        } 
        else
        {
            pBlist->dwCount = (DWORD)(lblkno(fs, di->di_size) + 1);

            pBlist->List = (PUfsBlock)HeapAlloc(g_hProcessHeap, 0, pBlist->dwCount *sizeof(ufs2_daddr_t));
            if (pBlist->List)
            {
                DWORD i;
                DWORD dwCount = 0; /* Count of blocks */
                DWORD64 dwTotalSize = 0;
                ufs2_daddr_t *bl;
                ufs2_daddr_t *singleb;
                ufs2_daddr_t *doubleb;
                ufs2_daddr_t *tripleb;

                bl = &(pBlist->List[0].ufs2);

                /* Direct blocks */
                for (i = 0; i < NDADDR; i++)
                {
                    bl[dwCount] = di->di_db[i];
                    dwTotalSize += fs->fs_bsize;

                    if (dwTotalSize >= di->di_size)
                    {
                        return pBlist;
                    }

                    dwCount++;
                }

                singleb = (ufs2_daddr_t*)VirtualAlloc(NULL, fs->fs_bsize *3, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                if (singleb)
                {
                    /**< 
                     * Count	of ufs2_daddr_t	values in block. 
                     * sizeof(ufs2_daddr_t) == 8	- shift	on 3 bits.
                     * Depend on	sizeof(ufs2_daddr_t).
                     */
                    DWORD dwAddrCount = fs->fs_bsize >> 3;

                    /**< Single	indirect blocks	*/
                    ReadDisk(hDisk, (PBYTE)singleb, (((DWORD64)p_offset) << 9) + (di->di_ib[0] << fs->fs_fshift), fs->fs_bsize);

                    for (i = 0; i < dwAddrCount; i++)
                    {
                        bl[dwCount] = singleb[i];
                        dwTotalSize += fs->fs_bsize;
                        dwCount++;
                        if (dwTotalSize >= di->di_size)
                        {
                            goto _free;
                        }
                    }

                    /**< Double	indirect blocks	*/
                    doubleb = singleb + fs->fs_bsize;

                    ReadDisk(hDisk, (PBYTE)doubleb, (p_offset << 9) + (di->di_ib[1] << fs->fs_fshift), fs->fs_bsize);

                    for (i = 0; i < dwAddrCount; i++)
                    {
                        DWORD j;

                        ReadDisk(hDisk, (PBYTE)singleb, (((DWORD64)p_offset) << 9) + (doubleb[i] << fs->fs_fshift), fs->fs_bsize);

                        for (j = 0; j < dwAddrCount; j++)
                        {
                            bl[dwCount] = singleb[j];
                            dwTotalSize += fs->fs_bsize;

                            if (dwTotalSize >= di->di_size)
                            {
                                goto _free;
                            }

                            dwCount++;
                        }
                    }

                    /**< Triple	indirect blocks	*/
                    tripleb = doubleb + fs->fs_bsize;

                    ReadDisk(hDisk, (PBYTE)tripleb, (((DWORD64)p_offset) << 9) + (di->di_ib[2] << fs->fs_fshift), fs->fs_bsize);

                    for (i = 0; i < dwAddrCount; i++)
                    {
                        DWORD j;

                        ReadDisk(hDisk, (PBYTE)doubleb, (((DWORD64)p_offset) << 9) + (tripleb[i] << fs->fs_fshift), fs->fs_bsize);

                        for (j = 0; j < dwAddrCount; j++)
                        {
                            DWORD k;

                            ReadDisk(hDisk, (PBYTE)singleb, (((DWORD64)p_offset) << 9) + (doubleb[j] << fs->fs_fshift), fs->fs_bsize);

                            for (k = 0; k < dwAddrCount; k++)
                            {
                                bl[dwCount] = singleb[k];
                                dwTotalSize += fs->fs_bsize;

                                if (dwTotalSize >= di->di_size)
                                {
                                    goto _free;
                                }

                                dwCount++;
                            }
                        }
                    }

                    _free: VirtualFree(singleb, 0, MEM_DECOMMIT);
                }
                else
                {
                    UfsFreeBlocksList(pBlist);
                    pBlist = NULL;
                }
            }
            else
            {
                HeapFree(g_hProcessHeap, 0, pBlist);
            }
        }
    }

    return pBlist;
}

/**
 * Ufs2ReadData	read dinodes blocks. Read maximum 0xFFFFFFFF bytes.	For	UFS2 partitions.
 *
 * @param[in] pUfsPart Pointer to UFS partition	structure.
 * @param[in] dinode Current inode.
 * @param[in] pBlist Block list	for	dinode.
 * @param[out] pBuffer Buffer for storing.
 * @param[in] dwStartBlock Block number	for	starting(first - 0).
 * @param[in] dwNumOfBlocks	Ufs2ReadData must read dwNumOfBlocks. if dwNumOfBlocks contain 0 then read all data(max	== 4GB).
 *
 * @return The number of bytes is total	read when successful, -1 when failure. 
 *
 */
DWORD Ufs2ReadData(PUfsPartition pUfsPart, PUfsDinode dinode, PUfsBlocksList pBlist, PBYTE pBuffer, DWORD dwStartBlock, DWORD dwNumOfBlocks)
{
    struct ufs2_dinode *di;
    ufs2_daddr_t *bl;
    DWORD dwTotal;

    HANDLE hDisk = pUfsPart->hDisk;
    PMyFs fs = pUfsPart->fs;
    DWORD p_offset = pUfsPart->p_offset;

    di = &(dinode->din.ufs2);
    bl = &(pBlist->List[0].ufs2);

    /* out of blocks range */
    if (dwStartBlock >= pBlist->dwCount)
    {
        return 0;
    } 

    if (!di->di_blocks)
    {
        /* no blocks, data in dinode.di_db */
        dwTotal = (DWORD)di->di_size;
        RtlMoveMemory(pBuffer, di->di_db, dwTotal);
    }
    else
    {
        DWORD dwLeave;

        /* if dwNumOfBlocks contain 0 then read full data(max == 4GB) */
        if (!dwNumOfBlocks)
        {
            dwLeave = (DWORD)di->di_size;
        }
        else
        {
            dwLeave = lblktosize(fs, dwNumOfBlocks);

            if ((dwStartBlock + dwNumOfBlocks + 1) >= pBlist->dwCount)
            {
                dwLeave -= (fs->fs_bsize - (DWORD)blkoff(fs, di->di_size));
            }
        }

        dwTotal = dwLeave;

        while (dwLeave)
        {
            DWORD dwBytesRead;

            if (bl[dwStartBlock])
            {
                dwBytesRead = (dwLeave > ((DWORD)fs->fs_bsize)) ? (fs->fs_bsize): (dwLeave);

                ReadDisk(hDisk, pBuffer, 
                /* Partition offset */
                (((DWORD64)p_offset) << 9) + 
                /* dwStartBlock offset in current partition */
                (DWORD64)(bl[dwStartBlock] << fs->fs_fshift), dwBytesRead);

            }
            else
            {
                /* zero-block */
                dwBytesRead = fs->fs_bsize;
                RtlZeroMemory(pBuffer, dwBytesRead);
            }

            pBuffer += dwBytesRead;
            dwLeave -= dwBytesRead;
            dwStartBlock++;
        }
    }

    return dwTotal;
}
