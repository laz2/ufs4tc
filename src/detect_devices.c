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
#include <setupapi.h>
#include <initguid.h>
#include <winioctl.h>
#include <cfgmgr32.h>

#include "debugm.h"
#include "detect_devices.h"
#include "list.h"

HANDLE g_hDevStorageHeap = NULL;

extern HANDLE g_hProcessHeap;

PCHAR g_DiskType[] = 
{
    "ad", "da", "fd"
};

#define DEVCLASS_COL 2

static GUID g_DeviceClass[DEVCLASS_COL];

void InitDetecter()
{
    g_DeviceClass[0] = GUID_DEVINTERFACE_DISK;
    g_DeviceClass[1] = GUID_DEVINTERFACE_FLOPPY;
}

WORD GetStorageDeviceType(LPGUID pDeviceInterfaceGuid, HDEVINFO DeviceInfoSet, DWORD MemberIndex)
{
    SP_DEVINFO_DATA DeviceInfoData;

    if (IsEqualGUID(pDeviceInterfaceGuid, &GUID_DEVINTERFACE_FLOPPY))
    {
        return TYPE_FD;
    }

    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    if (SetupDiEnumDeviceInfo(DeviceInfoSet, MemberIndex, &DeviceInfoData))
    {
        DWORD dwRemovalPolicy;

        if (SetupDiGetDeviceRegistryProperty( /* Heck =) */
        DeviceInfoSet, &DeviceInfoData, SPDRP_REMOVAL_POLICY_HW_DEFAULT, NULL, (PBYTE) &dwRemovalPolicy, sizeof(dwRemovalPolicy), NULL))
        {
            switch (dwRemovalPolicy)
            {
                case (CM_REMOVAL_POLICY_EXPECT_NO_REMOVAL): return TYPE_AD;
                case (CM_REMOVAL_POLICY_EXPECT_ORDERLY_REMOVAL): return -1;
                case (CM_REMOVAL_POLICY_EXPECT_SURPRISE_REMOVAL): return TYPE_DA;
                default:
                    return -1;
            }
        }
    }
    return -1;
}

PStorageDevice DetectStorageDevices()
{
    DWORD dwDeviceNum[3] = 
    {
         -1,  /* TYPE_AD */
         -1,  /* TYPE_DA */
         -1   /* TYPE_FD */
    };
    PStorageDevice DevList = NULL;
    DWORD dwDevClassNum;

    if (g_hDevStorageHeap != NULL)
    {
        HeapDestroy(g_hDevStorageHeap);
    }
    g_hDevStorageHeap = HeapCreate(0, 0, 0);
    if (!g_hDevStorageHeap)
    {
        return NULL;
    }

    for (dwDevClassNum = 0; dwDevClassNum < DEVCLASS_COL; dwDevClassNum++)
    {
        HDEVINFO hDeviceInfo;
        DWORD dwDevNum;

        DBG_PRINT((DebugFile, "Checking storage device class number %d\n\n", dwDevClassNum + 1));

        hDeviceInfo = SetupDiGetClassDevs((LPGUID) &g_DeviceClass[dwDevClassNum], 0, 0, DIGCF_PRESENT | DIGCF_INTERFACEDEVICE);

        if (hDeviceInfo == INVALID_HANDLE_VALUE)
        {
            return NULL;
        }

        for (dwDevNum = 0; TRUE; dwDevNum++)
        {

            SP_INTERFACE_DEVICE_DATA DevData = 
            {
                sizeof(SP_INTERFACE_DEVICE_DATA)
            };
            if (SetupDiEnumDeviceInterfaces(hDeviceInfo, 0, &g_DeviceClass[dwDevClassNum], dwDevNum, &DevData))
            {

                PStorageDevice DevItem;
                DWORD dwType;

                PSP_INTERFACE_DEVICE_DETAIL_DATA pDevDetailData;
                DWORD dwPLen = 0;
                DWORD dwRLen = 0;
                SetupDiGetInterfaceDeviceDetail(hDeviceInfo, &DevData, 0, 0, &dwRLen, 0);

                if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
                {
                    continue;
                }

                dwPLen = dwRLen;
                pDevDetailData = (PSP_INTERFACE_DEVICE_DETAIL_DATA)HeapAlloc(g_hProcessHeap, 0, dwRLen);
                pDevDetailData->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
                if (SetupDiGetInterfaceDeviceDetail(hDeviceInfo, &DevData, pDevDetailData, dwPLen, &dwRLen, 0))
                {

                    dwType = GetStorageDeviceType(&g_DeviceClass[dwDevClassNum], hDeviceInfo, dwDevNum);

                    if (dwType !=  - 1)
                    {
                        DevItem = (PStorageDevice)HeapAlloc(g_hDevStorageHeap, 0, sizeof(StorageDevice));

                        INIT_LITEM(DevItem);
                        DevItem->wType = (WORD)dwType;
                        DevItem->wNum = (WORD)++dwDeviceNum[dwType];
                        DevItem->DevicePath = (PCHAR)HeapAlloc(g_hDevStorageHeap, 0, strlen(pDevDetailData->DevicePath) + 1);
                        strcpy(DevItem->DevicePath, pDevDetailData->DevicePath);
                        AddListItem((PPListItem) &DevList, (PListItem)DevItem);

                        DBG_PRINT((DebugFile, "NAME: \"%s\",  ""NUMBER: %d,  ""TYPE: %d\n\n", DevItem->DevicePath, DevItem->wNum, DevItem->wType));
                    }
                }

                HeapFree(g_hProcessHeap, 0, pDevDetailData);
            }
            else
            {
                SetupDiDestroyDeviceInfoList(hDeviceInfo);
                break;
            }
        }
    }
    DBG_PRINT((DebugFile, "All devices detected\n\n"));
    return DevList;
}

VOID FreeDevStorageList()
{
    if (g_hDevStorageHeap != NULL)
    {
        HeapDestroy(g_hDevStorageHeap);
    }
    g_hDevStorageHeap = NULL;
}
