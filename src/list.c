/**
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

#include "list.h"

VOID AddListItemBackward(PPListItem ppList, PListItem pItem)
{
    PListItem pTemp =  *ppList;

    if (pTemp == NULL)
    {
        *ppList = pItem;
    }
    else
    {
        while (TRUE)
        {
            if (NEXT_LITEM(pTemp) == NULL)
            {
                break;
            }
            else
            {
                pTemp = NEXT_LITEM(pTemp);
            }
        }
        pTemp->pNext = pItem;
        pItem->pPrev = pTemp;
    }
}

VOID AddListItemForward(PPListItem ppList, PListItem pItem)
{
    if (*ppList == NULL)
    {
        *ppList = pItem;
    }
    else
    {
        pItem->pNext =  *ppList;
        (*ppList)->pPrev = pItem;
        *ppList = pItem;
    }
}

PListItem DelListItem(PPListItem ppList, PListItem pItem)
{
    PListItem pTemp =  *ppList;

    if (pTemp != NULL)
    {
        while (TRUE)
        {
            if (pTemp == pItem)
            {

                if (*ppList == pTemp)
                {
                    *ppList = pTemp->pNext;
                }

                if (pTemp->pPrev)
                {
                    pTemp->pPrev->pNext = pTemp->pNext;
                }

                if (pTemp->pNext)
                {
                    pTemp->pNext->pPrev = pTemp->pPrev;
                }

                return pTemp;
            }

            if (NEXT_LITEM(pTemp) == NULL)
            {
                return NULL;
            }
            else
            {
                pTemp = NEXT_LITEM(pTemp);
            }
        }
    }

    return NULL;
}
