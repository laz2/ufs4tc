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

#include <windows.h>

#include "diskio.h"

/**
 * Open disk
 *
 * @param lpDriveName Pointer to disk name  
 *
 * @return INVALID_HANDLE_VALUE if an error occurs, or a handle.
 */
HANDLE OpenDisk(LPCTSTR lpDriveName)
{
    return CreateFile(lpDriveName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
}

/**
 * Seek disk pointer
 *
 * @param hDisk Disk handle
 * @param dwOffset New pointer offset 
 * @param dwMoveMethod From which seek disk pointer
 *
 * @return number of bytes for skip from sectors begin
 */
DWORD SeekDisk(HANDLE hDisk, DWORD64 dwOffset, DWORD dwMoveMethod)
{
    DWORD dwLow, dwHigh;
    DWORD dwSkip;

    dwHigh = (DWORD)((dwOffset >> 32) &0x7FFFFFFF);
    dwLow = (DWORD)(dwOffset &0xFFFFFFFF);

    dwSkip = dwLow % DISK_BLOCK_SIZE;
    dwLow -= dwSkip;

    if (SetFilePointer(hDisk, dwLow, &dwHigh, dwMoveMethod) == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
        return  - 1;
    }

    return dwSkip;
}

/**
 * Read bytes from disk. 
 *
 * @param hDisk Disk handle
 * @param pOutputBuf Buffer for readed bytes
 * @param dwOffset New pointer offset
 * @param dwNumOfBytes Number of bytes for reading
 *
 * @return TRUE if read operation successful, else FALSE
 */
BOOL ReadDisk(HANDLE hDisk, PBYTE pOutputBuf, DWORD64 dwOffset, DWORD dwNumOfBytes)
{
    if (dwNumOfBytes > 0)
    {
        DWORD dwBytesRead;
        DWORD dwSkip;
        DWORD dwLeave;
        BYTE pSector[DISK_BLOCK_SIZE];

        if ((dwSkip = SeekDisk(hDisk, dwOffset, FILE_BEGIN)) ==  - 1)
        {
            return FALSE;
        }

        dwLeave = dwNumOfBytes;

        if (dwSkip)
        {
            if (!ReadFile(hDisk, pSector, DISK_BLOCK_SIZE, &dwBytesRead, NULL))
            {
                return FALSE;
            }

            MoveMemory(pOutputBuf, pSector + dwSkip, DISK_BLOCK_SIZE - dwSkip);

            pOutputBuf += DISK_BLOCK_SIZE - dwSkip;

            dwLeave -= DISK_BLOCK_SIZE - dwSkip;
        }

        if (dwLeave >= DISK_BLOCK_SIZE)
        {
            if (!ReadFile(hDisk, pOutputBuf, (dwLeave >> 9) << 9, &dwBytesRead, NULL))
            {
                return FALSE;
            }

            dwLeave -= dwBytesRead;
            pOutputBuf += dwBytesRead;
        }

        if (dwLeave)
        {
            if (!ReadFile(hDisk, pSector, DISK_BLOCK_SIZE, &dwBytesRead, NULL))
            {
                return FALSE;
            }

            MoveMemory(pOutputBuf, pSector, dwLeave);
        }
    }

    return TRUE;
}

/**
 * Close disk handle
 *
 * @param hDisk Disk handle
 *
 */
VOID CloseDisk(HANDLE hDisk)
{
    CloseHandle(hDisk);
}
