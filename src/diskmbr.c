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

#include <stdlib.h>

#include "list.h"
#include "diskio.h"
#include "my_diskmbr.h"
#include "disklabel.h"

extern HANDLE g_hDevStorageHeap;

/**
 * Find BSD partitions in primary and extended slices.
 *
 * @param hDisk disk handle.
 * @param dwMbrOffset offset of MBR.
 *
 * @return NULL if an error occurs, or a list of BSD partitions.
 */
PBsdPart FindBsdPartitions(HANDLE hDisk, DWORD dwMbrOffset)
{
    DWORD dwSliceNum;
    BYTE pMbr[DISK_BLOCK_SIZE];
    DWORD dwOffset = DOSPARTOFF;
    PBsdPart bpPartList;
    DWORD dwNdospart;

    if (!ReadDisk(hDisk, (PBYTE)pMbr, ((DWORD64)dwMbrOffset) << 9, DISK_BLOCK_SIZE))
    {
        return NULL;
    }

    /* Check MBR magic */
    if (*((WORD*) &pMbr[DOSMAGICOFFSET]) != DOSMAGIC)
    {
        return NULL;
    }

    /* if dwMbrOffset is 0 then we have primary "first extended partition" */
    dwNdospart = (dwMbrOffset == DOSBBSECTOR) ? (NDOSPART): (2);

    bpPartList = NULL;
    dwSliceNum = 0;
    while (dwSliceNum < dwNdospart)
    {
        PBsdPart bpPart = NULL;

        if (((struct dos_partition*) &pMbr[dwOffset])->dp_typ == DOSPTYP_386BSD)
        {
             /* slice with bsd fs */
            struct disklabel diskLabel;

            DWORD dwSliceStart = ((struct dos_partition*) &pMbr[dwOffset])->dp_start;

            /* Read disklabel from begin of current slice plus one sector */
            ReadDisk(hDisk, (PBYTE) &diskLabel, ((DWORD64)dwSliceStart + 1) << 9, sizeof(struct disklabel));

            if (diskLabel.d_magic == DISKMAGIC && diskLabel.d_magic2 == DISKMAGIC)
            {
                int i;
                for (i = 0; i < MAXPARTITIONS; i++)
                {
                    if (i == 1)
                    {
                        /* skip 'b'-partition and 'c'-partition */
                        i++, i++;
                    }
                    
                    if (diskLabel.d_partitions[i].p_fstype == FS_BSDFFS)
                    {
                        bpPart = (PBsdPart)HeapAlloc(g_hDevStorageHeap, 0, sizeof(BsdPart));
                        if (bpPart == NULL)
                        {
                            /* TODO: error handler */
                            continue;
                        } 

                        INIT_LITEM(bpPart);
                        bpPart->dwSliceNum = dwSliceNum + 1;
                        bpPart->chPLetter = 'a' + (CHAR)i;
                        bpPart->p_size = diskLabel.d_partitions[i].p_size;
                        bpPart->p_offset = diskLabel.d_partitions[i].p_offset;
                        AddListItem((PPListItem) &bpPartList, (PListItem)bpPart);
                    }
                }
            }
        }
        else if (((struct dos_partition*) &pMbr[dwOffset])->dp_typ == DOSPTYP_EXT || ((struct dos_partition*) &pMbr[dwOffset])->dp_typ == DOSPTYP_EXTLBA || ((struct dos_partition*) &pMbr[dwOffset])->dp_typ == 0x85)
        {
            /* extended dos slice*/

            /* recursion */
            bpPart = FindBsdPartitions(hDisk, ((struct dos_partition*) &pMbr[dwOffset])->dp_start);

            if (bpPart != NULL)
            {
                AddListItem((PPListItem) &bpPartList, (PListItem)bpPart);
            } 
        }

        /* move to next slice */
        dwSliceNum++;
        dwOffset += DOSPARTSIZE;
    }

    return bpPartList;
}
