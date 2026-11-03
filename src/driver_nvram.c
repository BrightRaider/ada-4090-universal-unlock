/* SPDX-License-Identifier: GPL-2.0-only */
#include "port.h"
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePath.h>

static EFI_GUID GlobalGuid = {0x8be4df61, 0x93ca, 0x11d2, {0xaa, 0x0d, 0, 0xe0, 0x98, 0x03, 0x2b, 0x8c}};
static EFI_GUID LoadedGuid = EFI_LOADED_IMAGE_PROTOCOL_GUID;
static EFI_GUID PathGuid = EFI_DEVICE_PATH_PROTOCOL_GUID;

#ifndef LOAD_OPTION_ACTIVE
#define LOAD_OPTION_ACTIVE 0x00000001U
#endif

static UINTN GetDevicePathSize(const EFI_DEVICE_PATH_PROTOCOL *dp) {
  if (!dp) return 0;
  const EFI_DEVICE_PATH_PROTOCOL *node = dp;
  while (node->Type != 0x7f || node->SubType != 0xff) {
    UINTN len = (UINTN)node->Length[0] | ((UINTN)node->Length[1] << 8);
    if (len < 4) return 0;
    node = (const EFI_DEVICE_PATH_PROTOCOL *)((const UINT8 *)node + len);
  }
  return ((const UINT8 *)node - (const UINT8 *)dp) + 4;
}

EFI_STATUS GuInstallDriverOption(EFI_HANDLE image, EFI_SYSTEM_TABLE *t) {
  if (!image || !t || !t->BootServices || !t->RuntimeServices) return EFI_INVALID_PARAMETER;
  EFI_BOOT_SERVICES *bs = t->BootServices;
  EFI_RUNTIME_SERVICES *rs = t->RuntimeServices;

  EFI_LOADED_IMAGE_PROTOCOL *loaded = NULL;
  EFI_STATUS s = bs->HandleProtocol(image, &LoadedGuid, (VOID **)&loaded);
  if (s != EFI_SUCCESS || !loaded || !loaded->DeviceHandle) return EFI_UNSUPPORTED;

  EFI_DEVICE_PATH_PROTOCOL *vol_path = NULL;
  s = bs->HandleProtocol(loaded->DeviceHandle, &PathGuid, (VOID **)&vol_path);
  if (s != EFI_SUCCESS || !vol_path) return EFI_UNSUPPORTED;

  UINTN vol_size = GetDevicePathSize(vol_path);
  if (vol_size < 4) return EFI_UNSUPPORTED;
  UINTN vol_nodes_size = vol_size - 4; // exclude end node

  const CHAR16 rel_path[] = L"\\EFI\\Drivers\\AdaUnlockDxe.efi";
  UINTN path_bytes = sizeof(rel_path);
  UINTN file_node_len = 4 + path_bytes;
  UINTN total_dp_len = vol_nodes_size + file_node_len + 4; // plus end node

  const CHAR16 desc[] = L"AdaUnlock";
  UINTN desc_bytes = sizeof(desc);

  UINTN load_opt_size = sizeof(UINT32) + sizeof(UINT16) + desc_bytes + total_dp_len;
  UINT8 *buf = NULL;
  s = bs->AllocatePool(EfiBootServicesData, load_opt_size, (VOID **)&buf);
  if (s != EFI_SUCCESS || !buf) return EFI_OUT_OF_RESOURCES;

  UINT8 *p = buf;
  // 1. Attributes
  *(UINT32 *)p = LOAD_OPTION_ACTIVE; p += sizeof(UINT32);
  // 2. FilePathListLength
  *(UINT16 *)p = (UINT16)total_dp_len; p += sizeof(UINT16);
  // 3. Description (null-terminated)
  for (UINTN i = 0; i < desc_bytes; i++) p[i] = ((const UINT8 *)desc)[i];
  p += desc_bytes;

  // 4. FilePathList: Volume Device Path (without end node)
  for (UINTN i = 0; i < vol_nodes_size; i++) p[i] = ((const UINT8 *)vol_path)[i];
  p += vol_nodes_size;

  // File Path Node: Type=4 (Media), SubType=4 (FilePath)
  p[0] = 0x04; p[1] = 0x04;
  p[2] = (UINT8)(file_node_len & 0xff);
  p[3] = (UINT8)((file_node_len >> 8) & 0xff);
  p += 4;
  for (UINTN i = 0; i < path_bytes; i++) p[i] = ((const UINT8 *)rel_path)[i];
  p += path_bytes;

  // End Node: Type=0x7f, SubType=0xff, Len=4
  p[0] = 0x7f; p[1] = 0xff; p[2] = 0x04; p[3] = 0x00;

  UINT32 attrs = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;

  // Write Driver0001
  s = rs->SetVariable(L"Driver0001", &GlobalGuid, attrs, load_opt_size, buf);
  bs->FreePool(buf);
  if (s != EFI_SUCCESS) return s;

  // Update DriverOrder: prepend 0x0001 if not already present
  UINT16 existing_order[64];
  UINTN order_size = sizeof(existing_order);
  UINT32 order_attrs = attrs;
  EFI_STATUS order_status = rs->GetVariable(L"DriverOrder", &GlobalGuid, &order_attrs, &order_size, existing_order);
  if (order_status == EFI_SUCCESS && order_size >= sizeof(UINT16)) {
    UINTN count = order_size / sizeof(UINT16);
    BOOLEAN found = FALSE;
    for (UINTN i = 0; i < count; i++) {
      if (existing_order[i] == 0x0001) { found = TRUE; break; }
    }
    if (!found) {
      if (count > 63) count = 63;
      UINT16 new_order[64];
      new_order[0] = 0x0001;
      for (UINTN i = 0; i < count; i++) new_order[i + 1] = existing_order[i];
      s = rs->SetVariable(L"DriverOrder", &GlobalGuid, attrs, (count + 1) * sizeof(UINT16), new_order);
    }
  } else {
    UINT16 driver_order[1] = { 0x0001 };
    s = rs->SetVariable(L"DriverOrder", &GlobalGuid, attrs, sizeof(driver_order), driver_order);
  }

  return s;
}

EFI_STATUS GuRemoveDriverOption(EFI_SYSTEM_TABLE *t) {
  if (!t || !t->RuntimeServices) return EFI_INVALID_PARAMETER;
  EFI_RUNTIME_SERVICES *rs = t->RuntimeServices;
  UINT32 attrs = EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS;
  (VOID)rs->SetVariable(L"Driver0001", &GlobalGuid, attrs, 0, NULL);

  UINT16 existing_order[64];
  UINTN order_size = sizeof(existing_order);
  UINT32 order_attrs = attrs;
  EFI_STATUS s = rs->GetVariable(L"DriverOrder", &GlobalGuid, &order_attrs, &order_size, existing_order);
  if (s == EFI_SUCCESS && order_size >= sizeof(UINT16)) {
    UINTN count = order_size / sizeof(UINT16);
    UINT16 new_order[64];
    UINTN new_count = 0;
    for (UINTN i = 0; i < count; i++) {
      if (existing_order[i] != 0x0001) {
        new_order[new_count++] = existing_order[i];
      }
    }
    if (new_count > 0) {
      (VOID)rs->SetVariable(L"DriverOrder", &GlobalGuid, attrs, new_count * sizeof(UINT16), new_order);
    } else {
      (VOID)rs->SetVariable(L"DriverOrder", &GlobalGuid, attrs, 0, NULL);
    }
  } else {
    (VOID)rs->SetVariable(L"DriverOrder", &GlobalGuid, attrs, 0, NULL);
  }
  return EFI_SUCCESS;
}
