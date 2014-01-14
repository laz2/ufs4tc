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
#include <shlwapi.h>

#include "misc.h"

PCHAR HeadDir(PCHAR pPath, PCHAR pHeadDir)
{
    if (pPath[0] != '\0')
    {
        PCHAR pTemp;
        PCHAR pSlash;
        DWORD dwNumOfBytes;

        pTemp = pPath;
        if (*pPath == '\\')
        {
            pTemp++;
        }

        pSlash = StrChr(pTemp, '\\');

        if (pSlash)
        {
            dwNumOfBytes = pSlash - pTemp;
            MoveMemory(pHeadDir, pTemp, dwNumOfBytes);
            pHeadDir[dwNumOfBytes] = '\0';
            StrCpy(pPath, pSlash + 1);
        }
        else
        {
            StrCpy(pHeadDir, pTemp);
            pPath[0] = '\0';
        }

        return pHeadDir;
    }
    else
    {
        *pHeadDir = '\0';
        return NULL;
    }
}

PCHAR TailDir(PCHAR pPath, PCHAR pTailDir)
{
    if (pPath[0] != '\0')
    {
        DWORD dwLength;
        PCHAR pLastBackSlash;

        dwLength = lstrlen(pPath) - 1;

        if (pPath[dwLength] == '\\')
        {
            pPath[dwLength] = '\0';
        }

        pLastBackSlash = StrRChr(pPath, NULL, '\\');
        if (pLastBackSlash)
        {
            StrCpy(pTailDir, pLastBackSlash + 1);
            pLastBackSlash[0] = '\0';
        }
        else
        {
            StrCpy(pTailDir, pPath);
            pPath[0] = '\0';
        }

        return pTailDir;
    }
    else
    {
        *pTailDir = '\0';
        return NULL;
    }
}
