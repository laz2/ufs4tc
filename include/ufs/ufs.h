/*
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

#ifndef _UFS_H_
#define _UFS_H_

#include <windows.h>

#include "dinode.h"
#include "dir.h"
#include "fs.h"
#include "stdint.h"

typedef DWORD UfsIno;
typedef UfsIno *PUfsIno;

typedef struct UfsDinode
{
    WORD mode;
    DWORD64 size;
    ufs_time_t mtime;

    union
    {
        struct ufs1_dinode ufs1;
        struct ufs2_dinode ufs2;
    } din;
}

UfsDinode,  *PUfsDinode;

typedef union UfsBlock
{
    ufs1_daddr_t ufs1;
    ufs2_daddr_t ufs2;
} UfsBlock,  *PUfsBlock;

typedef struct UfsBlocksList
{
    DWORD dwCount;
    PUfsBlock List;
} UfsBlocksList,  *PUfsBlocksList;

typedef struct UfsPartition
{
    HANDLE hDisk; /**< Disk handle */
    PMyFs fs; /**< Pointer to MyFs */
    DWORD p_offset; /**< LBA of BSD partition */
} UfsPartition,  *PUfsPartition;

typedef struct UfsVtbl
{
    BOOL(*UfsReadInode)(PUfsPartition pUfsPart, UfsIno ino, PUfsDinode dinode);
    PUfsBlocksList(*UfsGetBlocks)(PUfsPartition pUfsPart, PUfsDinode dinode);
    DWORD(*UfsReadData)(PUfsPartition pUfsPart, PUfsDinode dinode, PUfsBlocksList blist, PBYTE lpBuffer, DWORD dwStartBlock, DWORD dwNumOfBlocks);
} UfsVbtl,  *PUfsVtbl;

VOID UfsFreeBlocksList(PUfsBlocksList pBlist);

WORD UfsReadDirentry(PBYTE pData, struct direct *direct);

UfsIno UfsLookupPath(PUfsPartition pUfsPart, PUfsVtbl Vtbl, UfsIno RootIno, PCHAR pPath, PUfsIno pSymlinkIno);

UfsIno UfsFollowSymLink(PUfsPartition pUfsPart, PUfsVtbl Vtbl, UfsIno RootIno, UfsIno SymLinkIno);

#endif /* !_UFS_H_ */
