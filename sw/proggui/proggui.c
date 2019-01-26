/*
 * This file is part of the scope-footswitch project.
 *
 * Copyright (C) 2018 Paul Roukema <paul@paulroukema.com>
 * Some functions borrowed from Rufus
 * https://github.com/pbatard/rufus/
 * Copyright © 2018 Pete Batard <pete@akeo.ie>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#define WINVER 0x0601
#include <Windows.h>
#include <windowsx.h>
#include <CommCtrl.h>
#include <richedit.h>
#include <tchar.h>
#include <stdio.h>
#include <process.h>

#include "license.h"
#include "resource.h"
#include "hidapi.h"
#include "hidprog.h"
#include "image_header.h"

#if defined(_MSC_VER)
// Disable some VS Code Analysis warnings
#pragma warning(disable : 4996) // Ignore deprecated
#endif

#define safe_stprintf(dst, count, ...)                                         \
    do {                                                                       \
        _sntprintf(dst, count, __VA_ARGS__);                                   \
        (dst)[(count)-1] = 0;                                                  \
    } while (0)
#define static_stprintf(dst, ...)                                              \
    safe_stprintf(dst, sizeof(dst) / sizeof(dst[0]), __VA_ARGS__)

enum user_message_type {
    UM_INFO_READY = WM_APP,
    UM_PROGRAM_DONE,
};

struct devInfo {
    unsigned short vid;
    unsigned short pid;
    wchar_t *      serialNumber;
};
struct progGuiInfo {
    HINSTANCE       instance;
    struct devInfo *devices;
    size_t          num_devices;
    struct devInfo  expected;
    uint32_t        expected_magic;
    HANDLE          hThread;

    hidprog_info_t      progInfo;
    struct image_header devImageInfo;
    struct image_header fwImageInfo;

    uint8_t *firmware;
    size_t   firmware_len;
};

static uint32_t get_le32(uint8_t buf[4]) {
    return buf[0] | (buf[1] << 8) | (buf[2] << 16) | (buf[3] << 24);
}

static BOOL isValidImageHeader(struct image_header *header) {
    return (memcmp(header->magic, IMAGE_HEADER_MAGIC_STR, 4) == 0);
}

static BOOL versionIsNewer(struct image_header *cur,
                           struct image_header *update) {
    uint32_t cur_ver = (cur->version_major << 16) + (cur->version_minor << 8) +
                       cur->version_patch;
    uint32_t update_ver = (update->version_major << 16) +
                          (update->version_minor << 8) + update->version_patch;

    return update_ver > cur_ver;
}

#define safe_release_dc(hDlg, hDC)                                             \
    do {                                                                       \
        if ((hDC != INVALID_HANDLE_VALUE) && (hDC != NULL)) {                  \
            ReleaseDC(hDlg, hDC);                                              \
            hDC = NULL;                                                        \
        }                                                                      \
    } while (0)
/*
 * Center a dialog with regards to the main application Window or the desktop
 * See
 * http://msdn.microsoft.com/en-us/library/windows/desktop/ms644996.aspx#init_box
 */
static void CenterDialog(HWND hDlg) {
    HWND hParent;
    RECT rc, rcDlg, rcParent;

    if ((hParent = GetParent(hDlg)) == NULL) {
        hParent = GetDesktopWindow();
    }

    GetWindowRect(hParent, &rcParent);
    GetWindowRect(hDlg, &rcDlg);
    CopyRect(&rc, &rcParent);

    // Offset the parent and dialog box rectangles so that right and bottom
    // values represent the width and height, and then offset the parent again
    // to discard space taken up by the dialog box.
    OffsetRect(&rcDlg, -rcDlg.left, -rcDlg.top);
    OffsetRect(&rc, -rc.left, -rc.top);
    OffsetRect(&rc, -rcDlg.right, -rcDlg.bottom);

    SetWindowPos(hDlg, HWND_TOP, rcParent.left + (rc.right / 2),
                 rcParent.top + (rc.bottom / 2) - 25, 0, 0, SWP_NOSIZE);
}

static void *GetResource(HMODULE hModule, LPTSTR lpName, LPTSTR lpType,
                         size_t *len) {

    HRSRC hRes = FindResource(hModule, lpName, lpType);
    if (hRes == NULL) {
        return NULL;
    }

    HGLOBAL hGlobal = LoadResource(hModule, hRes);
    if (hGlobal == NULL) {
        return NULL;
    }

    if (len) {
        *len = SizeofResource(hModule, hRes);
    }

    return LockResource(hGlobal);
}

static void ComboBox_AutoSizeDropDown(HWND hCB) {
    HDC   hDC = GetDC(hCB);
    HFONT hFont, hDefFont = NULL;

    hFont = (HFONT)SendMessage(hCB, WM_GETFONT, 0, 0);
    if (hFont != NULL)
        hDefFont = (HFONT)SelectObject(hDC, hFont);

    int num_items = ComboBox_GetCount(hCB);
    int max_width = 0;
    for (int i = 0; i < num_items; i++) {
        TCHAR text[255];
        SIZE  size;
        ComboBox_GetLBText(hCB, i, text);
        GetTextExtentPoint32(hDC, text, _tcslen(text), &size);

        if (size.cx > max_width)
            max_width = size.cx;
    }

    if (hFont != NULL)
        SelectObject(hDC, hDefFont);

    ReleaseDC(hCB, hDC);

    SendMessage(hCB, CB_SETDROPPEDWIDTH, max_width + 8, 0);
}

static void SetTitleBarIcon(HMODULE hInstance, HWND hDlg) {
    int   i16, s16, s32;
    HICON hSmallIcon, hBigIcon;
    HDC   hDC = GetDC(hDlg);

    // High DPI scaling
    i16          = GetSystemMetrics(SM_CXSMICON);
    float fScale = fScale = GetDeviceCaps(hDC, LOGPIXELSX) / 96.0f;
    safe_release_dc(hDlg, hDC);
    // Adjust icon size lookup
    s16 = i16;
    s32 = (int)(32.0f * fScale);
    if (s16 >= 54)
        s16 = 64;
    else if (s16 >= 40)
        s16 = 48;
    else if (s16 >= 28)
        s16 = 32;
    else if (s16 >= 20)
        s16 = 24;
    if (s32 >= 54)
        s32 = 64;
    else if (s32 >= 40)
        s32 = 48;
    else if (s32 >= 28)
        s32 = 32;
    else if (s32 >= 20)
        s32 = 24;

    // Create the title bar icon
    hSmallIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON),
                                  IMAGE_ICON, s16, s16, 0);
    SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hSmallIcon);
    hBigIcon = (HICON)LoadImage(hInstance, MAKEINTRESOURCE(IDI_ICON),
                                IMAGE_ICON, s32, s32, 0);
    SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hBigIcon);
}

// Stupid rich edit does laggy smooth scrolling, lets not do that
static LRESULT CALLBACK LineScrollRichEditProc(HWND hWnd, UINT uMsg,
                                               WPARAM wParam, LPARAM lParam,
                                               UINT_PTR  uIdSubclass,
                                               DWORD_PTR dwRefData) {
    (void)uIdSubclass;
    (void)dwRefData;

    // Borrow the non-smooth scrolling behavior of edit boxes from Wine
    if (uMsg == WM_MOUSEWHEEL) {
        int  wheelDelta;
        UINT pulScrollLines = 3;
        SystemParametersInfoW(SPI_GETWHEELSCROLLLINES, 0, &pulScrollLines, 0);

        if (wParam & (MK_SHIFT | MK_CONTROL)) {
            return DefSubclassProc(hWnd, uMsg, wParam, lParam);
        }

        wheelDelta = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        wheelDelta *= pulScrollLines;

        return DefSubclassProc(hWnd, EM_LINESCROLL, 0, -wheelDelta);
    } else
        return DefSubclassProc(hWnd, uMsg, wParam, lParam);
}

static INT_PTR CALLBACK HelpCallback(HWND hDlg, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam) {
    (void)lParam;

    switch (uMsg) {
        case WM_INITDIALOG: {
            HWND      hEdit[2]  = {0};
            LPCSTR    strings[] = {header_string, license_string};
            SETTEXTEX textEx    = {ST_DEFAULT, CP_UTF8};

            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)lParam);
            struct progGuiInfo *info =
                (void *)GetWindowLongPtr(hDlg, DWLP_USER);

            CenterDialog(hDlg);
            SetTitleBarIcon(info->instance, hDlg);

            hEdit[0] = GetDlgItem(hDlg, IDC_HEADER);
            hEdit[1] = GetDlgItem(hDlg, IDC_LICENSE);

            for (int i = 0; i < 2; i++) {
                SendMessage(hEdit[i], EM_AUTOURLDETECT, 1, 0);
                SendMessage(hEdit[i], EM_SETSEL, -1, -1);
                SendMessageA(hEdit[i], EM_SETTEXTEX, (WPARAM)&textEx,
                             (LPARAM)strings[i]);
                SendMessage(hEdit[i], EM_SETEVENTMASK, 0, ENM_LINK);
            }
            SendMessage(hEdit[0], EM_SETBKGNDCOLOR, 0,
                        (LPARAM)GetSysColor(COLOR_BTNFACE));
            SetWindowSubclass(hEdit[1], LineScrollRichEditProc, 0, 0);
            return TRUE;
        }
        case WM_NOTIFY: {
            LPNMHDR nmhdr = (LPNMHDR)lParam;
            if (nmhdr->code == EN_LINK) {
                ENLINK *enl = (ENLINK *)lParam;
                if (enl->msg == WM_LBUTTONUP) {
                    TEXTRANGEW tr;
                    wchar_t    url[256];
                    tr.lpstrText  = url;
                    tr.chrg.cpMin = enl->chrg.cpMin;
                    tr.chrg.cpMax = min(enl->chrg.cpMax, enl->chrg.cpMin + 255);

                    // RichEdit is always RichEdt20W
                    SendMessageW(enl->nmhdr.hwndFrom, EM_GETTEXTRANGE, 0,
                                 (LPARAM)&tr);
                    url[255] = 0;
                    ShellExecuteW(hDlg, L"open", url, NULL, NULL,
                                  SW_SHOWNORMAL);
                }
            }
        }
            return FALSE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDCANCEL:
                case IDOK:
                    EndDialog(hDlg, LOWORD(wParam));
                    return TRUE;
            }
    }
    return FALSE;
}

static void onExit(HWND hDlg) {
    SendMessage(hDlg, WM_CLOSE, 0, 0);
}

static void onClose(HWND hDlg) {
    DestroyWindow(hDlg);
}

static void onDestroy(void) {
    PostQuitMessage(0);
}

static void onRefresh(HWND hDlg) {
    HWND                hDevice = GetDlgItem(hDlg, IDC_DEVICE);
    struct progGuiInfo *info    = (void *)GetWindowLongPtr(hDlg, DWLP_USER);
    ComboBox_ResetContent(hDevice);

    SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETPOS, 0, 0);
    SendMessage(GetDlgItem(hDlg, IDC_STATUS_BAR), SB_SETTEXT, 0,
                (LPARAM) _T("Refreshing"));

    if (info->devices) {
        for (size_t i = 0; i < info->num_devices; i++) {
            if (info->devices[i].serialNumber) {
                free(info->devices[i].serialNumber);
                info->devices[i].serialNumber = NULL;
            }
        }
        free(info->devices);
        info->devices = NULL;
    }
    info->num_devices = 0;

    struct hid_device_info *start =
        hid_enumerate(info->fwImageInfo.vid, info->fwImageInfo.pid);
    if (!start) {
        ComboBox_AddString(hDevice, _T("<No Devices Found>"));
    } else {
        struct hid_device_info *hid_info = start;
        int                     count    = 0;
        // Count number of devices int the singly linked list
        while (hid_info) {
            hid_info = hid_info->next;
            count++;
        }

        info->devices = calloc(count, sizeof(*info->devices));

        // Now go over the list again and save what we care about
        hid_info = start;
        if (info->devices) {
            info->num_devices = count;
            count             = 0;
            while (hid_info) {
                TCHAR buf[255];
                static_stprintf(buf, _T("%ls - %ls : %ls : [%04X:%04X]"),
                                hid_info->product_string,
                                hid_info->manufacturer_string,
                                hid_info->serial_number, hid_info->vendor_id,
                                hid_info->product_id);

                ComboBox_AddString(hDevice, buf);

                info->devices[count].vid = hid_info->vendor_id;
                info->devices[count].pid = hid_info->product_id;
                info->devices[count].serialNumber =
                    _wcsdup(hid_info->serial_number);

                hid_info = hid_info->next;
                count++;
            }
        }
        hid_free_enumeration(start);
    }
    ComboBox_AutoSizeDropDown(hDevice);
    ComboBox_SetCurSel(hDevice, 0);
    SendMessage(GetDlgItem(hDlg, IDC_STATUS_BAR), SB_SETTEXT, 0,
                (LPARAM) _T(""));
    PostMessage(hDlg, WM_COMMAND, MAKEWPARAM(IDC_DEVICE, CBN_SELCHANGE),
                (LPARAM)hDevice);
}

enum infoResult {
    INFO_RESULT_SUCCESS  = 0,
    INFO_RESULT_OPEN_ERR = 1,

    INFO_RESULT_INFO_ERR  = 2,
    INFO_RESULT_MAGIC_ERR = 3,
    INFO_RESULT_READ_ERR  = 4,
};

struct infoThreadArgs {
    HWND   hDlg;
    size_t index;
};

static unsigned int __stdcall InfoThread(void *param) {

    struct infoThreadArgs *args = param;
    struct progGuiInfo *info = (void *)GetWindowLongPtr(args->hDlg, DWLP_USER);
    struct devInfo *    devinfo = &(info->devices[args->index]);
    WPARAM              result  = INFO_RESULT_OPEN_ERR;

    hid_device *dev =
        hid_open(devinfo->vid, devinfo->pid, devinfo->serialNumber);
    if (dev) {
        result  = INFO_RESULT_INFO_ERR;
        int res = hidprog_getinfo(dev, &(info->progInfo));
        if (!res) {
            result = INFO_RESULT_MAGIC_ERR;
            if (info->progInfo.magic == info->expected_magic) {
                result = INFO_RESULT_READ_ERR;

                res = hidprog_read(dev, info->progInfo.flash_base,
                                   (void *)(&info->devImageInfo),
                                   sizeof(struct image_header));
                if (!res) {
                    result = INFO_RESULT_SUCCESS;
                }
            }
        }
        hid_close(dev);
    }

    PostMessage(args->hDlg, UM_INFO_READY, result, 0);

    free(args);
    return 0;
}

static void onDeviceChanged(HWND hDlg) {
    struct progGuiInfo *info = (void *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (!info->hThread) {
        if (info->devices) {
            EnableWindow(GetDlgItem(hDlg, IDC_PROGRAM), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_REFRESH), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_DEVICE), FALSE);
            HWND hStatus = GetDlgItem(hDlg, IDC_STATUS_BAR);
            SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETPOS, 0, 0);
            SendMessage(hStatus, SB_SETTEXT, 0,
                        (LPARAM) _T("Getting Device Info"));

            struct infoThreadArgs *args = calloc(1, sizeof(*args));
            args->hDlg                  = hDlg;
            args->index = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DEVICE));

            info->hThread =
                (HANDLE)_beginthreadex(NULL, 0, InfoThread, args, 0, NULL);
            if ((LONG)info->hThread == -1L) {
                info->hThread = NULL;
                PostMessage(hDlg, UM_INFO_READY, INFO_RESULT_OPEN_ERR, 0);
                free(args);
            }
        } else {
            EnableWindow(GetDlgItem(hDlg, IDC_PROGRAM), FALSE);
            Static_SetText(GetDlgItem(hDlg, IDC_CURRENT_VERSION),
                           _T("<No Device>"));
        }
    } else {
        MessageBox(hDlg, _T("Another operation is already in progress"),
                   _T("Error"), MB_OK | MB_ICONERROR);
    }
}

static void onInfoReady(HWND hDlg, WPARAM wParam) {
    struct progGuiInfo *info = (void *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (info->hThread) {
        CloseHandle(info->hThread);
        info->hThread = 0;
    }

    Static_SetText(GetDlgItem(hDlg, IDC_CURRENT_VERSION), _T(""));
    HWND hStatus = GetDlgItem(hDlg, IDC_STATUS_BAR);
    switch (wParam) {
        case INFO_RESULT_SUCCESS:
            SendMessage(hStatus, SB_SETTEXT, 0,
                        (LPARAM) _T("Bootloader Magic OK"));
            if (isValidImageHeader(&info->devImageInfo)) {
                TCHAR buf[255];
                static_stprintf(buf, _T("%d.%d.%d"),
                                info->devImageInfo.version_major,
                                info->devImageInfo.version_minor,
                                info->devImageInfo.version_patch);
                Static_SetText(GetDlgItem(hDlg, IDC_CURRENT_VERSION), buf);
            } else {
                Static_SetText(GetDlgItem(hDlg, IDC_CURRENT_VERSION),
                               _T("<invalid>"));
                SendMessage(hStatus, SB_SETTEXT, 0,
                            (LPARAM) _T("Device header is invalid or blank"));
            }
            EnableWindow(GetDlgItem(hDlg, IDC_PROGRAM), TRUE);

            break;

        case INFO_RESULT_OPEN_ERR:
            SendMessage(hStatus, SB_SETTEXT, 0,
                        (LPARAM) _T("Error opening device"));
            break;

        case INFO_RESULT_INFO_ERR:
            SendMessage(hStatus, SB_SETTEXT, 0,
                        (LPARAM) _T("Error querying bootloader info"));
            break;
        case INFO_RESULT_MAGIC_ERR:
            SendMessage(hStatus, SB_SETTEXT, 0,
                        (LPARAM) _T("Bootloader Magic mismatch"));
            break;
        case INFO_RESULT_READ_ERR:
            SendMessage(hStatus, SB_SETTEXT, 0,
                        (LPARAM) _T("Error reading image header"));
            break;
    }

    EnableWindow(GetDlgItem(hDlg, IDC_REFRESH), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_DEVICE), TRUE);
}

enum programResult {
    PROGRAM_RESULT_SUCCESS  = 0,
    PROGRAM_RESULT_OPEN_ERR = 1,

    PROGRAM_RESULT_WRITE_ERR  = 2,
    PROGRAM_RESULT_VERIFY_ERR = 3,
};

struct programThreadArgs {
    HWND   hDlg;
    size_t index;
};

static unsigned int __stdcall ProgramThread(void *param) {

    struct programThreadArgs *args = param;
    struct progGuiInfo *info = (void *)GetWindowLongPtr(args->hDlg, DWLP_USER);
    struct devInfo *    devinfo   = &(info->devices[args->index]);
    WPARAM              result    = PROGRAM_RESULT_OPEN_ERR;
    HWND                hProgress = GetDlgItem(args->hDlg, IDC_PROGRESS);
    HWND                hStatus   = GetDlgItem(args->hDlg, IDC_STATUS_BAR);

    hid_device *dev =
        hid_open(devinfo->vid, devinfo->pid, devinfo->serialNumber);
    if (dev) {
        result  = PROGRAM_RESULT_WRITE_ERR;
        int res = hidprog_setaddr(dev, info->progInfo.flash_base);
        if (!res) {
            PostMessage(hStatus, SB_SETTEXT, 0, (LPARAM) _T("Programming"));
            for (size_t i = 0; i < info->firmware_len;) {
                int remain = info->firmware_len - i;
                if (remain > 1024)
                    remain = 1024;

                res = hidprog_program(dev, (uint32_t)-1, info->firmware + i,
                                      remain);
                if (res) {
                    break;
                }
                i += remain;

                PostMessage(hProgress, PBM_SETPOS,
                            (50 * i) / info->firmware_len, 0);
            }
            if (!res) {
                result  = PROGRAM_RESULT_VERIFY_ERR;
                int res = hidprog_setaddr(dev, info->progInfo.flash_base);
                PostMessage(hStatus, SB_SETTEXT, 0, (LPARAM) _T("Verifying"));
                if (!res) {
                    for (size_t i = 0; i < info->firmware_len;) {
                        int remain = info->firmware_len - i;
                        if (remain > 1024)
                            remain = 1024;

                        res = hidprog_verify(dev, (uint32_t)-1,
                                             info->firmware + i, remain);
                        if (res) {
                            break;
                        }
                        i += remain;
                        PostMessage(hProgress, PBM_SETPOS,
                                    50 + (50 * i) / info->firmware_len, 0);
                    }
                    if (!res) {
                        result = PostMessage(hProgress, PBM_SETPOS, 100, 0);
                        res    = hidprog_read(dev, info->progInfo.flash_base,
                                           (void *)(&info->devImageInfo),
                                           sizeof(struct image_header));
                        if (!res) {
                            result = PROGRAM_RESULT_SUCCESS;
                        }
                    }
                }
            }
        }
        hid_close(dev);
    }

    PostMessage(args->hDlg, UM_PROGRAM_DONE, result, 0);

    free(args);
    return 0;
}

static void onProgram(HWND hDlg) {
    struct progGuiInfo *info      = (void *)GetWindowLongPtr(hDlg, DWLP_USER);
    BOOL                do_update = FALSE;

    SendMessage(GetDlgItem(hDlg, IDC_STATUS_BAR), SB_SETTEXT, 0,
                (LPARAM) _T(""));

    if (versionIsNewer(&info->devImageInfo, &info->fwImageInfo)) {
        do_update = TRUE;

    } else {
        int res =
            MessageBox(hDlg,
                       _T("Update version is not higher than current ")
                       _T("version. Continue?"),
                       _T("Confirm Update"), MB_YESNO | MB_ICONEXCLAMATION);
        if (res == IDYES) {
            do_update = TRUE;
        }
    }

    if (do_update) {
        // Only allow one thread to talk to to the hardware at once
        if (!info->hThread) {
            // Double check that we found devices to talk to
            if (info->devices) {
                SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETRANGE32, 0,
                            100);
                SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETPOS, 0, 0);
                // SendMessage(GetDlgItem(hDlg, IDC_PROGRESS), PBM_SETSTATE,
                //			PBST_NORMAL, 0);
                EnableWindow(GetDlgItem(hDlg, IDC_PROGRAM), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_REFRESH), FALSE);
                EnableWindow(GetDlgItem(hDlg, IDC_DEVICE), FALSE);

                struct programThreadArgs *args = calloc(1, sizeof(*args));
                args->hDlg                     = hDlg;
                args->index = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_DEVICE));

                info->hThread = (HANDLE)_beginthreadex(NULL, 0, ProgramThread,
                                                       args, 0, NULL);
                if ((LONG)info->hThread == -1L) {
                    info->hThread = NULL;
                    PostMessage(hDlg, UM_PROGRAM_DONE, PROGRAM_RESULT_OPEN_ERR,
                                0);
                    free(args);
                }
            }
        } else {
            MessageBox(hDlg, _T("Another operation is already in progress"),
                       _T("Error"), MB_OK | MB_ICONERROR);
        }
    }
}

static void onHelp(HWND hDlg) {
    struct progGuiInfo *info = (void *)GetWindowLongPtr(hDlg, DWLP_USER);
    DialogBoxParam(info->instance, MAKEINTRESOURCE(IDD_HELP), hDlg,
                   HelpCallback, (LPARAM)info);
}

static void onProgramDone(HWND hDlg, WPARAM wParam) {
    struct progGuiInfo *info = (void *)GetWindowLongPtr(hDlg, DWLP_USER);

    if (info->hThread) {
        CloseHandle(info->hThread);
        info->hThread = 0;
    }

    Static_SetText(GetDlgItem(hDlg, IDC_CURRENT_VERSION), _T(""));
    HWND hStatus = GetDlgItem(hDlg, IDC_STATUS_BAR);
    switch (wParam) {
        case PROGRAM_RESULT_SUCCESS:
            SendMessage(hStatus, SB_SETTEXT, 0,
                        (LPARAM) _T("Programing Complete!"));
            if (isValidImageHeader(&info->devImageInfo)) {
                TCHAR buf[255];
                static_stprintf(buf, _T("%d.%d.%d"),
                                info->devImageInfo.version_major,
                                info->devImageInfo.version_minor,
                                info->devImageInfo.version_patch);
                Static_SetText(GetDlgItem(hDlg, IDC_CURRENT_VERSION), buf);
            } else {
                Static_SetText(GetDlgItem(hDlg, IDC_CURRENT_VERSION),
                               _T("<invalid>"));
                SendMessage(hStatus, SB_SETTEXT, 0,
                            (LPARAM) _T("Device header is invalid or blank"));
            }
            EnableWindow(GetDlgItem(hDlg, IDC_PROGRAM), TRUE);

            break;

        case PROGRAM_RESULT_OPEN_ERR:
            SendMessage(hStatus, SB_SETTEXT, 0,
                        (LPARAM) _T("Error opening device"));
            break;

        case PROGRAM_RESULT_WRITE_ERR:
            SendMessage(hStatus, SB_SETTEXT, 0,
                        (LPARAM) _T("Error programming device"));
            break;
        case PROGRAM_RESULT_VERIFY_ERR:
            SendMessage(hStatus, SB_SETTEXT, 0,
                        (LPARAM) _T("Error verifying device"));
            break;
    }

    EnableWindow(GetDlgItem(hDlg, IDC_REFRESH), TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_DEVICE), TRUE);
}

void onInitDialog(HWND hDlg, LPARAM dwInitParam) {

    SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)dwInitParam);
    struct progGuiInfo *info = (void *)GetWindowLongPtr(hDlg, DWLP_USER);

    EnableWindow(GetDlgItem(hDlg, IDC_PROGRAM), FALSE);

    // Bring our Window on top. We have to go through all *THREE* of these,
    // or Far Manager hides our window :(
    SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    SetWindowPos(hDlg, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    SetWindowPos(hDlg, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);

    PostMessage(hDlg, WM_COMMAND, IDC_REFRESH, 0);

    HWND hStatus = GetDlgItem(hDlg, IDC_STATUS_BAR);
    int  sb_size = -1;

    SendMessage(hStatus, SB_SETPARTS, 1, (LPARAM)&sb_size);

    if (isValidImageHeader(&info->fwImageInfo)) {
        TCHAR buf[255];
        static_stprintf(buf, _T("%d.%d.%d"), info->fwImageInfo.version_major,
                        info->fwImageInfo.version_minor,
                        info->fwImageInfo.version_patch);
        Static_SetText(GetDlgItem(hDlg, IDC_UPDATE_VERSION), buf);
    }
}

static INT_PTR CALLBACK MainCallback(HWND hDlg, UINT uMsg, WPARAM wParam,
                                     LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            onClose(hDlg);
            return TRUE;

        case WM_DESTROY:
            onDestroy();
            return TRUE;

        case WM_INITDIALOG:
            onInitDialog(hDlg, lParam);
            return TRUE;
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDEXIT:
                    onExit(hDlg);
                    return TRUE;

                case IDC_REFRESH:
                    onRefresh(hDlg);
                    return TRUE;

                case IDC_PROGRAM:
                    onProgram(hDlg);
                    return TRUE;

                case IDC_DEVICE:
                    if (HIWORD(wParam) == CBN_SELCHANGE) {
                        onDeviceChanged(hDlg);
                        return TRUE;
                    }
                    break;
                case IDHELP:
                    onHelp(hDlg);
            }
            break;
        case UM_INFO_READY:
            onInfoReady(hDlg, wParam);
            return TRUE;
        case UM_PROGRAM_DONE:
            onProgramDone(hDlg, wParam);
            return TRUE;
    }
    return FALSE;
}

#if defined(_MSC_VER) && (_MSC_VER >= 1600)
int WINAPI WinMain(_In_ HINSTANCE hInst, _In_opt_ HINSTANCE hPrevInstance,
                   _In_ LPSTR lpCmdLine, _In_ int nCmdShow)
#else
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, LPSTR lpCmdLine,
                   int nCmdShow)
#endif
{
    (void)hPrevInstance;
    (void)lpCmdLine;

    InitCommonControls();
    LoadLibrary(_T("Riched20.dll"));
    hid_init();

    struct progGuiInfo info = {0};

    info.instance = hInst;
    info.firmware = GetResource(hInst, MAKEINTRESOURCE(IDR_FIRMWARE),
                                MAKEINTRESOURCE(RT_RCDATA), &info.firmware_len);
    if (!info.firmware) {
        return -1;
    }
    memcpy(&info.fwImageInfo, info.firmware, sizeof(info.fwImageInfo));
    if (!isValidImageHeader(&info.fwImageInfo)) {
        return -1;
    }
    info.expected.vid          = info.fwImageInfo.vid;
    info.expected.pid          = info.fwImageInfo.pid;
    info.expected.serialNumber = NULL;
    info.expected_magic        = get_le32(info.fwImageInfo.bootloader_magic);

    HWND hDlg = CreateDialogParam(hInst, MAKEINTRESOURCE(IDD_UPDATE), 0,
                                  MainCallback, (LPARAM)&info);
    SetTitleBarIcon(hInst, hDlg);

    ShowWindow(hDlg, nCmdShow);

    BOOL ret;
    MSG  msg;

    while ((ret = GetMessage(&msg, 0, 0, 0)) != 0) {
        if (ret == -1)
            return -1;

        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return 0;
}
