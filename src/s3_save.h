/* SPDX-License-Identifier: GPL-2.0-only */
#pragma once
#include <Uefi.h>

#define EFI_S3_SAVE_STATE_PROTOCOL_GUID \
  { 0xe73935aa, 0x3651, 0x4302, { 0xab, 0xdc, 0x4f, 0x30, 0xb0, 0xce, 0x53, 0x39 } }

#define EFI_BOOT_SCRIPT_SAVE_PROTOCOL_GUID \
  { 0x32b304f4, 0xeb48, 0x4f3f, { 0x88, 0x99, 0xad, 0x8d, 0x52, 0xf9, 0x82, 0xa1 } }

typedef enum {
  EfiBootScriptWidthUint8 = 0,
  EfiBootScriptWidthUint16 = 1,
  EfiBootScriptWidthUint32 = 2,
  EfiBootScriptWidthUint64 = 3,
  EfiBootScriptWidthMaximum
} EFI_BOOT_SCRIPT_WIDTH;

#define EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE 0x00
#define EFI_BOOT_SCRIPT_MEM_READ_WRITE_OPCODE 0x01
#define EFI_BOOT_SCRIPT_PCI_CONFIG_WRITE_OPCODE 0x02

typedef struct _EFI_S3_SAVE_STATE_PROTOCOL EFI_S3_SAVE_STATE_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_S3_SAVE_STATE_WRITE)(
  IN CONST EFI_S3_SAVE_STATE_PROTOCOL *This,
  IN UINTN OpCode,
  ...
);

struct _EFI_S3_SAVE_STATE_PROTOCOL {
  EFI_S3_SAVE_STATE_WRITE Write;
  VOID *InsertAt;
  VOID *Compare;
};
