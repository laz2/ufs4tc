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

#ifndef _ERRORS_H_
#define _ERRORS_H_

#include <windows.h>

#define NO_IDE_FOUND     1
#define NO_SLICES_FOUND  2
#define ERROR_ALLOC_MEM  3
#define FS_SBLOCK_NFOUND 4
#define BAD_SBLOCK       5
#define NOT_SUPPORT_UFS1 6

void ErrorBox(TCHAR *ErrorMsg);

TCHAR *g_Errors[] = 
{
    "\0", "No IDE HardDrives found", "No BSD Slices found", "Error allocate memory", "Cannot find file system superblock", "Bad superblock", "Cannot support UFS1"
};

static DWORD ErrorFlag = 0;

static void ErrorBox(TCHAR *ErrorMsg)
{
    MessageBox(NULL, ErrorMsg, "Error", MB_OK | MB_ICONEXCLAMATION);
}

#endif /* !_ERRORS_H_ */
