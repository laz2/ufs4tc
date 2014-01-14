/*
 * Copyright (c) 2007-2008 dude03
 * All rights reserved.
 * 
 * Redistribution and use in	source and binary forms, with or without
 * modification,	are	permitted provided that	the	following conditions
 * are met:
 * 1. Redistributions of	source code	must retain	the	above copyright
 *	 notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in	binary form	must reproduce the above copyright
 *	 notice, this list of conditions and the following disclaimer in the
 *	 documentation and/or other	materials provided with	the	distribution.
 * 3. The name of the author	may	not	be used	to endorse or promote products
 *	 derived from this software	without	specific prior written permission.
 * 
 * THIS SOFTWARE	IS PROVIDED	BY THE AUTHOR ``AS IS''	AND	ANY	EXPRESS	OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO	EVENT SHALL	THE	AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE	GOODS OR SERVICES; LOSS	OF USE,
 * DATA,	OR PROFITS;	OR BUSINESS	INTERRUPTION) HOWEVER CAUSED AND ON	ANY
 * THEORY OF	LIABILITY, WHETHER IN CONTRACT,	STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE	OR OTHERWISE) ARISING IN ANY WAY OUT OF	THE	USE	OF
 * THIS SOFTWARE, EVEN IF ADVISED OF	THE	POSSIBILITY	OF SUCH	DAMAGE.
 */

#include "dinode.h"
#include "diskio.h"
#include "fs.h"

extern HANDLE g_hProcessHeap; /**< Current process heap */

/**
 * Find UFS super block, start from LBA address dwOffset.
 *
 * @param[in] hDisk Disk handle.
 * @param[in] dwOffset LBA of start finding.
 *
 * @return NULL if superblock not found or other error, or a pointer to MyFs.
 */
PMyFs FindSblock(HANDLE hDisk, DWORD dwOffset)
{
    int i;
    static int sblock_try[] = SBLOCKSEARCH; /* places for search */
    BYTE pSblock[SBLOCKSIZE];

    for (i = 0; sblock_try[i] !=  - 1; i++)
    {
        struct fs *fs;

        ReadDisk(hDisk, pSblock, (((DWORD64)dwOffset) << 9) + ((DWORD64)sblock_try[i]), SBLOCKSIZE);

        fs = (struct fs*)pSblock;

        if (fs->fs_magic == FS_BAD_MAGIC)
        {
            break;
        }

        if ((fs->fs_magic == FS_UFS1_MAGIC || (fs->fs_magic == FS_UFS2_MAGIC && fs->fs_sblockloc == sblock_try[i])) && fs->fs_ncg >= 1 && fs->fs_bsize >= MINBSIZE && fs->fs_bsize >= SBLOCKSIZE)
        {
             /* check superblock */

            PMyFs my_fs = (PMyFs)HeapAlloc(g_hProcessHeap, 0, sizeof(MyFs));
            if (my_fs)
            {
                return MakeMyFs((struct fs*)pSblock, my_fs);
            }
        } 
    }

    return NULL;
}

/**
 * Make "my superblock" from UFS super block.
 *
 * @param[in] fs UFS super block.
 * @param[out] my_fs pointer to "my superblock".
 *
 * @return pointer to "my superblock".
 */
PMyFs MakeMyFs(struct fs *fs, PMyFs my_fs)
{
    my_fs->fs_cblkno = fs->fs_cblkno;
    my_fs->fs_iblkno = fs->fs_iblkno;
    my_fs->fs_dblkno = fs->fs_dblkno;
    my_fs->fs_old_cgoffset = fs->fs_old_cgoffset;
    my_fs->fs_old_cgmask = fs->fs_old_cgmask;
    my_fs->fs_bsize = fs->fs_bsize;
    my_fs->fs_fsize = fs->fs_fsize;
    my_fs->fs_frag = fs->fs_frag;
    my_fs->fs_bmask = fs->fs_bmask;
    my_fs->fs_fmask = fs->fs_fmask;
    my_fs->fs_bshift = fs->fs_bshift;
    my_fs->fs_fshift = fs->fs_fshift;
    my_fs->fs_fragshift = fs->fs_fragshift;
    my_fs->fs_fsbtodb = fs->fs_fsbtodb;
    my_fs->fs_inopb = fs->fs_inopb;
    my_fs->fs_cgsize = fs->fs_cgsize;
    my_fs->fs_ipg = fs->fs_ipg;
    my_fs->fs_fpg = fs->fs_fpg;
    my_fs->fs_qbmask = fs->fs_qbmask;
    my_fs->fs_qfmask = fs->fs_qfmask;
    my_fs->fs_magic = fs->fs_magic;

    return my_fs;
} 

/**
 * Free "my superblock" memory.
 *
 * @param[in] my_fs pointer to "my superblock".
 */
VOID FreeMyFs(PMyFs my_fs)
{
    HeapFree(g_hProcessHeap, 0, my_fs);
}
