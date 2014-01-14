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

#include <shlwapi.h>
#include <windows.h>

#include "debugm.h"
#include "dir.h"
#include "fs.h"
#include "misc.h"
#include "stdint.h"
#include "ufs.h"

extern HANDLE g_hProcessHeap; /**< Current process heap */

/**
 * Free block list.
 *
 * @param[in] pBlist Pointer to block list.
 */
VOID UfsFreeBlocksList(PUfsBlocksList pBlist)
{
    if (pBlist)
    {
        if (pBlist->List)
        {
            HeapFree(g_hProcessHeap, 0, pBlist->List);
        }
        
        HeapFree(g_hProcessHeap, 0, pBlist);
    }
}

/**
 * Read next dirent.
 *
 * @param[in] pData Pointer to begin of dirent.
 * @param[out] direct Pointer to dirent struct.
 * 
 * @return Length of dirent.
 */
WORD UfsReadDirentry(PBYTE pData, struct direct *direct)
{
    MoveMemory(direct, pData, 8);
    MoveMemory(direct->d_name, pData + 8, direct->d_namlen);
    direct->d_name[direct->d_namlen] = '\0';
    return direct->d_reclen;
} 

/**
 * Lookup inode number for tail directory in pPath. It follow symlinks.
 * 
 * @param[in] pUfsPart Pointer to UFS partition	structure.
 * @param[in] Vbtl Table of UFS functions.
 * @param[in] RootIno Number of root inode.
 * @param[in] pPath Path, RootIno relative.
 * @param[out] pSymLinkIno If tail of pPath is symlink, then pSymLinkIno contain pointer to it inode number, else 0.
 * If pSymLinkIno is NULL, then UfsLookupPath do not use it.
 *
 * @return Inode number of tail when successful, 0 when failure. 
 */
UfsIno UfsLookupPath(PUfsPartition pUfsPart, PUfsVtbl Vtbl, UfsIno RootIno, PCHAR pPath, PUfsIno pSymlinkIno)
{
    PBYTE pBlock;
    UfsIno CurrentIno = 0;

    assert(StrCmp(pPath, ""));

    DBG_PRINT((DebugFile, "LookupPath = %s\n", pPath));

    pBlock = VirtualAlloc(NULL, pUfsPart->fs->fs_bsize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (pBlock)
    {
        CHAR pPathTemp[MAX_PATH + 1];
        UfsIno CurrentRootIno;

        StrCpy(pPathTemp, pPath); /**< HeadDir call changes path */

        CurrentIno = RootIno;
        CurrentRootIno = RootIno;

        while (TRUE)
        {
            DWORD dwNextDirentOffset; /**< Next dirent offset in current block */
            DWORD dwStartBlock;
            DWORD dwBytesRead;
            BOOL fFindOkInverse;
            UfsDinode dinode;
            PUfsBlocksList pBlist;
            BYTE pDir[MAX_PATH + 1]; /**< Buffer for dirent name */

            if (!HeadDir(pPathTemp, pDir))
            {
                if ((dinode.mode &IFMT) == IFLNK)
                {
                    if (pSymlinkIno)
                    {
                        *pSymlinkIno = CurrentIno;
                    }
                    
                    CurrentIno = UfsFollowSymLink(pUfsPart, Vtbl, CurrentRootIno, CurrentIno);
                }

                break;
            }

            Vtbl->UfsReadInode(pUfsPart, CurrentIno, &dinode);

            if (CurrentIno != CurrentRootIno && (dinode.mode &IFMT) == IFLNK)
            {
                CurrentIno = UfsFollowSymLink(pUfsPart, Vtbl, CurrentRootIno, CurrentIno);
                if (!CurrentIno)
                {
                    break;
                }
            }

            pBlist = Vtbl->UfsGetBlocks(pUfsPart, &dinode);
            if (!pBlist)
            {
                CurrentIno = 0;
                break;
            }

            dwStartBlock = 0;

            /**< If finding dir is ok then fFindOkInverse == FALSE(Inverse value) */
            fFindOkInverse = TRUE;

            while ((dwBytesRead = Vtbl->UfsReadData(pUfsPart, &dinode, pBlist, pBlock, dwStartBlock, 1)))
            {
                struct direct direct;

                if (dwBytesRead ==  - 1)
                {
                    break;
                }

                dwNextDirentOffset = 0;

                do
                {
                    UfsReadDirentry(pBlock + dwNextDirentOffset, &direct);
                    dwNextDirentOffset += direct.d_reclen;
                } 
                while ((fFindOkInverse = StrCmp(pDir, direct.d_name)) && dwNextDirentOffset < dwBytesRead);

                if (!fFindOkInverse)
                {
                    CurrentRootIno = CurrentIno;
                    CurrentIno = direct.d_ino;
                    break;
                }
            }

            UfsFreeBlocksList(pBlist);

            if (fFindOkInverse)
            {
                CurrentIno = 0;
                break;
            }

        }

        VirtualFree(pBlock, 0, MEM_DECOMMIT);
    }

    return CurrentIno;
}

/**
 * Lookup inode number of current symlink inode.
 *
 * @param[in] pUfsPart Pointer to UFS partition	structure.
 * @param[in] Vbtl Table of UFS functions.
 * @param[in] RootIno Number of root inode.
 * @param[in] SymLinkIno Number of symlink inode.
 *
 * @return Number of symlink's target inode when successful, 0 when failure.
 */
UfsIno UfsFollowSymLink(PUfsPartition pUfsPart, PUfsVtbl Vtbl, UfsIno RootIno, UfsIno SymLinkIno)
{
    UfsDinode dinode;
    PUfsBlocksList pBlist;
    UfsIno SymLinkTargetIno = 0;

    Vtbl->UfsReadInode(pUfsPart, SymLinkIno, &dinode);

    pBlist = Vtbl->UfsGetBlocks(pUfsPart, &dinode);
    if (pBlist)
    {
        BYTE pSymLinkTarget[MAX_PATH + 1];

        if (dinode.size <= MAX_PATH)
        {
            Vtbl->UfsReadData(pUfsPart, &dinode, pBlist, pSymLinkTarget, 0, 0);

            if (pSymLinkTarget[0] != '/')
            {
                PBYTE pCh;
                pSymLinkTarget[dinode.size] = '\0';

                /**< Change symbol '/' on '\' */
                for (pCh = pSymLinkTarget;  *pCh; pCh++)
                {
                    if (*pCh == '/')
                    {
                        *pCh = '\\';
                    }
                }

                DBG_PRINT((DebugFile, "RootIno = %d, ""SymLinkIno = %d, ""SymLinkPath = %s\n", RootIno, SymLinkIno, pSymLinkTarget));

                SymLinkTargetIno = UfsLookupPath(pUfsPart, Vtbl, RootIno, pSymLinkTarget, NULL);
            }
        }

        UfsFreeBlocksList(pBlist);
    }

    return SymLinkTargetIno;
}
