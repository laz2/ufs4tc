/**
 * Copyright (c) 2007-2008 Dude03
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

#include <shlwapi.h>

#include "debugm.h"
#include "detect_devices.h"
#include "dinode.h"
#include "dir.h"
#include "diskio.h"
#include "diskmbr.h"
#include "errors.h"
#include "fs.h"
#include "fsplugin.h"
#include "list.h"
#include "misc.h"
#include "ufs.h"
#include "ufs1.h"
#include "ufs2.h"
#include "ufs4tc.h"

PStorageDevice g_DevList = NULL;

HICON g_hIconDisk; /**<	Partition icon handle */
HICON g_hIconSymLinkDirValid; /**< Valid symlink icon handle(directory)	*/
HICON g_hIconSymLinkFileValid; /**<	Valid symlink icon handle(file)	*/
HICON g_hIconSymLinkInvalid; /**< Valid	symlink	icon handle(file) */

HANDLE g_hProcessHeap; /**<	Current	process	heap */

extern HANDLE g_hDevStorageHeap;

DWORD PluginNumber;
tProgressProc ProgressProc;
tLogProc LogProc;
tRequestProc RequestProc;

/**
 * FsInit is called	when loading the plugin.
 */
int __stdcall FsInit(int PluginNr, tProgressProc pProgressProc, tLogProc pLogProc, tRequestProc pRequestProc)
{
    HMODULE hShell32Dll;
    ProgressProc = pProgressProc;
    LogProc = pLogProc;
    RequestProc = pRequestProc;
    PluginNumber = PluginNr;
    InitDetecter();
    g_hProcessHeap = GetProcessHeap();

    hShell32Dll = LoadLibrary("shell32.dll");
    g_hIconDisk = LoadIcon(hShell32Dll, MAKEINTRESOURCE(ICON_DISK));
    g_hIconSymLinkDirValid = LoadIcon(hShell32Dll, MAKEINTRESOURCE(ICON_SYMLINK_DIR_VALID));
    g_hIconSymLinkFileValid = LoadIcon(hShell32Dll, MAKEINTRESOURCE(ICON_SYMLINK_FILE_VALID));
    g_hIconSymLinkInvalid = LoadIcon(hShell32Dll, MAKEINTRESOURCE(ICON_SYMLINK_INVALID));
    FreeLibrary(hShell32Dll);

    g_DevList = FsFindBsdPartitions();

    return 0;
}

PStorageDevice FsFindBsdPartitions()
{
    PStorageDevice pDev = NULL;
    PStorageDevice pDevList;
    BOOL fFindOk = FALSE;

    pDevList = DetectStorageDevices();
    if (pDevList)
    {
        pDev = pDevList;
        while (pDev != NULL)
        {
            HANDLE hDisk = OpenDisk(pDev->DevicePath);
            if (hDisk != INVALID_HANDLE_VALUE)
            {
                pDev->bpPartList = FindBsdPartitions(hDisk, DOSBBSECTOR);
                CloseDisk(hDisk);
            }
            if (pDev->bpPartList)
            {
                if (!fFindOk)
                {
                    fFindOk = TRUE;
                }
                pDev = pDev->pNext;
            }
            else
            {
                PStorageDevice pTemp = (PStorageDevice)DelListItem((PPListItem) &pDevList, (PListItem)pDev);
                pDev = pDev->pNext;
                HeapFree(g_hDevStorageHeap, 0, pTemp);
            }
        }

        if (fFindOk)
        {
            return pDevList;
        }
        else
        {
            FreeDevStorageList();
        }
    }

    return NULL;
}

HANDLE FsLookupPath(LPSTR pPath)
{
    HANDLE fh;
    WIN32_FIND_DATA FindData;
    CHAR pPathTemp[MAX_PATH + 1], pFileName[MAX_PATH + 1];
    BOOL fOk;

    StrCpy(pPathTemp, pPath);
    TailDir(pPathTemp, pFileName);

    fh = FsFindFirst(pPathTemp, &FindData);

    while (strcmp(FindData.cFileName, pFileName) && (fOk = FsFindNext((HANDLE)fh, &FindData)));

    if (fOk)
    {
        return fh;
    }
    else
    {
        return INVALID_HANDLE_VALUE;
    }
}

/**
 * FsFindFirst is called to	retrieve the first file	in a directory of the plugin's file	system.
 */
HANDLE __stdcall FsFindFirst(char *Path, WIN32_FIND_DATA *FindData)
{
    PPartFindHandle pfh;
    PUfsFindHandle ufh;
    BYTE pHeadDir[MAX_PATH + 1];
    BYTE pPath[MAX_PATH + 1];

    pfh = (PPartFindHandle)HeapAlloc(g_hProcessHeap, 0, sizeof(PartFindHandle));
    pfh->dwHdlType = PART_FIND_HANDLE;
    pfh->pPart = NULL;
    pfh->pDev = g_DevList;

    if (!StrCmp(Path, "\\"))
    {
        if (FsFindNext((HANDLE)pfh, FindData))
        {
            return pfh;
        }
        else
        {
            FsFindClose((HANDLE)pfh);
            return INVALID_HANDLE_VALUE;
        }
    }
    else
    {
        BOOL fFindOk;

        StrCpy(pPath, Path);
        HeadDir(pPath, pHeadDir);
        while ((fFindOk = FsFindNext((HANDLE)pfh, FindData)) && StrCmp(FindData->cFileName, pHeadDir))
            ;

        if (!fFindOk)
        {
            FsFindClose((HANDLE)pfh);
            return INVALID_HANDLE_VALUE;
        }
    }

    ufh = (PUfsFindHandle)HeapAlloc(g_hProcessHeap, HEAP_ZERO_MEMORY, sizeof(UfsFindHandle));
    if (ufh)
    {
        ufh->p_offset = pfh->pPart->p_offset;

        ufh->hDisk = OpenDisk(pfh->pDev->DevicePath);
        FsFindClose(pfh);
        if (ufh->hDisk != INVALID_HANDLE_VALUE)
        {

            ufh->fs = FindSblock(ufh->hDisk, ufh->p_offset);
            if (ufh->fs)
            {
                UfsIno CurrentIno;

                if (ufh->fs->fs_magic == FS_UFS1_MAGIC)
                {
                    ufh->UfsGetBlocks = Ufs1GetBlocks;
                    ufh->UfsReadData = Ufs1ReadData;
                    ufh->UfsReadInode = Ufs1ReadInode;
                }
                else if (ufh->fs->fs_magic == FS_UFS2_MAGIC)
                {
                    ufh->UfsGetBlocks = Ufs2GetBlocks;
                    ufh->UfsReadData = Ufs2ReadData;
                    ufh->UfsReadInode = Ufs2ReadInode;
                }

                CurrentIno = UfsLookupPath(ufh->hDisk, ufh->fs, ufh->p_offset, (PUfsVtbl) &(ufh->UfsReadInode), ROOTINO, pPath, NULL);
                if (CurrentIno)
                {
                    ufh->dinode = (PUfsDinode)HeapAlloc(g_hProcessHeap, 0, sizeof(UfsDinode));
                    ufh->cdinode = (PUfsDinode)HeapAlloc(g_hProcessHeap, 0, sizeof(UfsDinode));
                    if (ufh->dinode && ufh->cdinode)
                    {
                        ufh->UfsReadInode(ufh->hDisk, ufh->fs, ufh->p_offset, CurrentIno, ufh->dinode);

                        ufh->pBlist = ufh->UfsGetBlocks(ufh->hDisk, ufh->fs, ufh->p_offset, ufh->dinode);
                        if (ufh->pBlist)
                        {

                            ufh->pBlock = (PBYTE)VirtualAlloc(NULL, ufh->fs->fs_bsize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                            if (ufh->pBlock)
                            {
                                ufh->dwHdlType = UFS_FIND_HANDLE;
                                ufh->dwNextDirentOffset = 12;
                                ufh->dwDataSize = 0;

                                FsFindNext((HANDLE)ufh, FindData);

                                SetLastError(ERROR_SUCCESS);
                                return ufh;
                            }
                        }
                    }
                }
            }
        }

        FsFindClose((HANDLE)ufh);
    }

    SetLastError(ERROR_FILE_NOT_FOUND);
    return INVALID_HANDLE_VALUE;
}

BOOL FindNextPartition(PPartFindHandle pfh, WIN32_FIND_DATA *FindData)
{
    if (pfh->pDev != NULL)
    {
        if (pfh->pPart == NULL)
        {
            pfh->pPart = pfh->pDev->bpPartList;
        }
        else
        {
            if (pfh->pPart->pNext == NULL)
            {
                pfh->pDev = pfh->pDev->pNext;
                if (pfh->pDev)
                {
                    pfh->pPart = pfh->pDev->bpPartList;
                }
                else
                {
                    SetLastError(ERROR_NO_MORE_FILES);
                    return FALSE;
                }
            }
            else
            {
                pfh->pPart = pfh->pPart->pNext;
            }
        }

        FindData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
        FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
        FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;
        wsprintf(FindData->cFileName, "%s%ds%d%c", g_DiskType[pfh->pDev->wType], pfh->pDev->wNum, pfh->pPart->dwSliceNum, pfh->pPart->chPLetter);

        return TRUE;
    }
    
    return FALSE;
}

BOOL FindNextUfsEntry(PUfsFindHandle ufh, WIN32_FIND_DATA *FindData)
{
    struct direct dir;

    if (ufh->dwNextDirentOffset == ufh->dwDataSize || !ufh->dwDataSize)
    {
        ufh->dwDataSize = ufh->UfsReadData(ufh->hDisk, ufh->fs, ufh->p_offset, ufh->dinode, ufh->pBlist, ufh->pBlock, ufh->dwCurrentBlock, 1);

        if (ufh->dwDataSize <= 0)
        {
            SetLastError(ERROR_NO_MORE_FILES);
            return FALSE;
        } 

        if (ufh->dwCurrentBlock)
        {
            ufh->dwNextDirentOffset = 0;
        }

        ufh->dwCurrentBlock++;
    }

    UfsReadDirentry(ufh->pBlock + ufh->dwNextDirentOffset, &dir);
    ufh->dwNextDirentOffset += dir.d_reclen;

    if (*((WORD*)dir.d_name) == ((WORD)'..') && dir.d_name[2] == '\0')
    {
        if (dir.d_reclen != 12)
        {
            SetLastError(ERROR_NO_MORE_FILES);
            return FALSE;
        }

        UfsReadDirentry(ufh->pBlock + 12, &dir);
    }

    ufh->UfsReadInode(ufh->hDisk, ufh->fs, ufh->p_offset, dir.d_ino, ufh->cdinode);
    StrCpy(FindData->cFileName, dir.d_name);

    FindData->dwFileAttributes = 0x80000000;

    if (dir.d_type == DT_DIR)
    {
        FindData->dwFileAttributes |= FILE_ATTRIBUTE_DIRECTORY;
    }
    else if (dir.d_type == DT_LNK)
    {
        
    }
    else
    {
        FindData->nFileSizeHigh = (DWORD)((ufh->cdinode->size >> 32) &0x7FFFFFFF);
        FindData->nFileSizeLow = (DWORD)(ufh->cdinode->size &0xFFFFFFFF);
    }

    FindData->dwReserved0 = ufh->cdinode->mode;

    FindData->ftLastWriteTime.dwHighDateTime = 0xFFFFFFFF;
    FindData->ftLastWriteTime.dwLowDateTime = 0xFFFFFFFE;

    return TRUE;
}

/**
 * FsFindNext is called	to retrieve	the	next file in a directory of	the	plugin's file system.
 */
BOOL __stdcall FsFindNext(HANDLE hdl, WIN32_FIND_DATA *findData)
{
    BOOL result = FALSE;

    memset(findData, 0, sizeof(WIN32_FIND_DATA));

    if (((PFindHandle)hdl)->dwHdlType == PART_FIND_HANDLE)
    {
        result = FindNextPartition((PPartFindHandle)hdl, findData);
    }
    else if (((PFindHandle)hdl)->dwHdlType == UFS_FIND_HANDLE)
    {
        result = FindNextUfsEntry((PUfsFindHandle)hdl, findData);
    }

    return result;
}

/**
 * FsFindClose is called to	end	a FsFindFirst/FsFindNext loop, either after	retrieving all files, 
 * or when the user	aborts it.
 */
int __stdcall FsFindClose(HANDLE Hdl)
{
    if (((PFindHandle)Hdl)->dwHdlType == PART_FIND_HANDLE)
    {
        HeapFree(g_hProcessHeap, 0, Hdl);
    }
    else if (((PFindHandle)Hdl)->dwHdlType == UFS_FIND_HANDLE)
    {
        PUfsFindHandle ufh = (PUfsFindHandle)Hdl;

        CloseDisk(ufh->hDisk);

        FreeMyFs(ufh->fs);
        HeapFree(g_hProcessHeap, 0, ufh->dinode);
        HeapFree(g_hProcessHeap, 0, ufh->cdinode);

        UfsFreeBlocksList(ufh->pBlist);

        VirtualFree(ufh->pBlock, 0, MEM_DECOMMIT);

        HeapFree(g_hProcessHeap, 0, ufh);
    }

    return TRUE;
}

/**
 * FsGetFile is	called to transfer a file from the plugin's	file system	to the normal file system.
 */
int __stdcall FsGetFile(char *RemoteName, char *LocalName, int CopyFlags, RemoteInfoStruct *ri)
{
    DWORD dwResult;

    if ((CopyFlags & FS_COPYFLAGS_RESUME) || (CopyFlags & FS_COPYFLAGS_MOVE))
    {
        return FS_FILE_NOTSUPPORTED;
    }
    else
    {
        HANDLE hFile;
        hFile = CreateFile(LocalName, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

        /* check if	file exists	*/
        if (hFile == INVALID_HANDLE_VALUE)
        {
            if (GetLastError() == ERROR_FILE_EXISTS)
            {
                if (!CopyFlags)
                {
                    return FS_FILE_EXISTS;
                }
                else
                {
                    hFile = CreateFile(LocalName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                }
            }
            else
            {
                return FS_FILE_WRITEERROR;
            }
        }

        /* Show	progress bar */
        if (ProgressProc(PluginNumber, RemoteName, LocalName, 0))
        {
            CloseHandle(hFile);
            return FS_FILE_USERABORT;
        }
        else
        {
            PUfsFindHandle ufh;
            PUfsBlocksList pBlist;

            if ((ufh = (PUfsFindHandle)FsLookupPath(RemoteName)) != INVALID_HANDLE_VALUE)
            {
                pBlist = ufh->UfsGetBlocks(ufh->hDisk, ufh->fs, ufh->p_offset, ufh->cdinode);
                if (pBlist)
                {
                    DWORD dwBytesRead;
                    DWORD dwStartBlock;
                    DWORD dwBytesWritten;

                    dwResult = FS_FILE_OK;
                    dwStartBlock = 0;

                    while (dwBytesRead = ufh->UfsReadData(ufh->hDisk, ufh->fs, ufh->p_offset, ufh->cdinode, pBlist, ufh->pBlock, dwStartBlock, 1))
                    {
                        if (dwBytesRead ==  - 1)
                        {
                            dwResult = FS_FILE_READERROR;
                            break;
                        }

                        if (!WriteFile(hFile, ufh->pBlock, dwBytesRead, &dwBytesWritten, NULL))
                        {
                            dwResult = FS_FILE_WRITEERROR;
                            break;
                        }

                        dwStartBlock++;
                    }

                    UfsFreeBlocksList(pBlist);
                }
            }

            FsFindClose((HANDLE)ufh);
        }

        CloseHandle(hFile);
    }

    return dwResult;
}

/**
 * FsExtractCustomIcon is called when a	file/directory is displayed	in the file	list. 
 * It can be used to specify a custom icon for that	file/directory.	
 */
int __stdcall FsExtractCustomIcon(char *RemoteName, int ExtractFlags, HICON *TheIcon)
{
    if (!StrStr(RemoteName, "\\..\\"))
    {
        PBYTE pByte;

        pByte = StrChr(&RemoteName[1], '\\');
        if (pByte[1] != '\0')
        {
            PUfsFindHandle ufh;

            if ((ufh = FsLookupPath(RemoteName)) != INVALID_HANDLE_VALUE)
            {
                if ((ufh->cdinode->mode &IFMT) == IFLNK)
                {
                    /**< symbolic links	*/
                    if (UfsFollowSymLink(ufh->hDisk, ufh->fs, ufh->p_offset, GetVtbl(ufh), ufh->dinoden, ufh->cdinoden))
                    {
                        /**< valid symbolic	links */

                        if (RemoteName[lstrlen(RemoteName) - 1] == '\\')
                        {
                            *TheIcon = g_hIconSymLinkDirValid;
                        }
                        else
                        {
                            *TheIcon = g_hIconSymLinkFileValid;
                        }

                    }
                    else
                    {
                        /**< invalid symbolic links	*/
                        *TheIcon = g_hIconSymLinkInvalid;
                    }

                    return FS_ICON_EXTRACTED;
                }

                FsFindClose(ufh);
            }
        }
        else
        {
            /**< partitions	*/
            *TheIcon = g_hIconDisk;

            return FS_ICON_EXTRACTED;
        }
    }

    return FS_ICON_USEDEFAULT;
}

/**
 * FsExecuteFile is	called to execute a	file on	the	plugin's file system, 
 * or show its property	sheet.
 */
int __stdcall FsExecuteFile(HWND MainWin, char *RemoteName, char *Verb)
{
    if (!StrCmpI(Verb, "open"))
    {
        if (RequestProc(PluginNumber, RT_MsgYesNo, NULL, "Execute file?", NULL, 0))
        {
            return FS_EXEC_YOURSELF;
        }
        else if (!StrCmpI(Verb, "properties"))
        {
            return FS_EXEC_OK;
        }
    }
}

/**
 * FsGetDefRootName	is called only when	the	plugin is installed. 
 * It asks the plugin for the default root name	which should appear	in the Network Neighborhood.
 */
void __stdcall FsGetDefRootName(char *DefRootName, int maxlen)
{
    StrCpyN(DefRootName, PLUGIN_LABEL, (sizeof(PLUGIN_LABEL) < maxlen) ? (sizeof(PLUGIN_LABEL)): (maxlen));
}
