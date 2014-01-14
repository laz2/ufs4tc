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
#include <tlhelp32.h>
#include <shlwapi.h>

#include "debugm.h"

FILE *DebugFile; /**< File for debug output */

/**
 * Start debug session.
 *
 * @param[in] pFileName Debug file name.
 * @param[in] hModule Dll base address.
 *
 * @return TRUE when success else FALSE.
 */
BOOL StartDebugSession(PCHAR pFileName, HMODULE hModule)
{
    HANDLE hSnapshot;
    MODULEENTRY32 me;
    PCHAR pLastSlash;
    BOOL fResult = FALSE;

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());

    Module32First(hSnapshot, &me);
    do
    {
        if (hModule == me.hModule)
        {
            pLastSlash = StrRChr(me.szExePath, NULL, '\\');
            *(++pLastSlash) = '\0';

            if ((lstrlen(me.szExePath) + lstrlen("d.out")) <= MAX_MODULE_NAME32)
            {
                StrCpy(pLastSlash, pFileName);

                DebugFile = fopen(me.szExePath, "w");
                if (DebugFile != NULL)
                {
                    fResult = TRUE;
                    break;
                }
            }
        }
    }
    while (Module32Next(hSnapshot, &me));
    
    CloseHandle(hSnapshot);

    return fResult;
}

/**
 * Close debug session.
 */
VOID CloseDebugSession()
{
    fclose(DebugFile);
}
