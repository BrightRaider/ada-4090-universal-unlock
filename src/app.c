/* SPDX-License-Identifier: GPL-2.0-only */

#include "port.h"

#include "checks.h"

#include "profile.h"

#include "msi_release.h"

#include "boot_io.h"

#include "compat/compat.h"

#include "vendor/unlock_610.h"

#include "s3_save.h"

#include "driver_nvram.h"

static EFI_GUID PciGuid=EFI_PCI_IO_PROTOCOL_GUID;

static EFI_GUID GlobalGuid={0x8be4df61,0x93ca,0x11d2,{0xaa,0x0d,0,0xe0,0x98,0x03,0x2b,0x8c}};

static EFI_STATUS Discover(EFI_HANDLE image,EFI_SYSTEM_TABLE *t) {

  EFI_HANDLE *h=NULL,target=NULL;EFI_PCI_IO_PROTOCOL *io=NULL;UINTN count=0,found=0;

  EFI_STATUS s=t->BootServices->LocateHandleBuffer(ByProtocol,&PciGuid,NULL,&count,&h);

  if(s==EFI_SUCCESS&&(!h||!count||count>1024))s=EFI_UNSUPPORTED;

  UINT16 detected_dev_id = 0;

  for(UINTN i=0;s==EFI_SUCCESS&&i<count;i++) {

    EFI_PCI_IO_PROTOCOL *p=NULL;UINT32 id=0;

    s=t->BootServices->HandleProtocol(h[i],&PciGuid,(VOID **)&p);

    if(s!=EFI_SUCCESS||!p||!p->Pci.Read){s=EFI_UNSUPPORTED;break;}

    s=p->Pci.Read(p,EfiPciIoWidthUint32,0,1,&id);

    if(s==EFI_SUCCESS&&GuIsSupportedPciId(id)){found++;target=h[i];io=p;detected_dev_id=(UINT16)(id>>16);}

  }

  if(h)t->BootServices->FreePool(h);

  if(s!=EFI_SUCCESS||found!=1||!io)return EFI_UNSUPPORTED;

  GuInit(t->BootServices,io);Gu.image=image;Gu.handle=target;

  Gu.device_id=detected_dev_id;Gu.is_ampere=GuIsAmpereDeviceId(detected_dev_id);Gu.is_turing=GuIsTuringDeviceId(detected_dev_id);

  UINT32 c[64];UINTN seg,bus,dev,fun;GU_CAPS caps;

  if(!io->GetLocation)return EFI_UNSUPPORTED;

  s=io->GetLocation(io,&seg,&bus,&dev,&fun);

  if(s==EFI_SUCCESS)s=io->Pci.Read(io,EfiPciIoWidthUint32,0,64,c);

  if(s!=EFI_SUCCESS||!GuCheckConfig(c,seg,bus,dev,fun,&caps)||((c[2]>>16)!=0x0300U&&(c[2]>>16)!=0x0302U))return EFI_UNSUPPORTED;

  Gu.bar_base=(c[4]&~0xfU);

  Gu.bar_size=0x1000000;

  Gu.pm_cap=caps.pm;Gu.pcie_cap=caps.pcie;Gu.pcie_version=caps.pcie_version;Gu.msi_cap=caps.msi;Gu.msix_cap=caps.msix;

  return EFI_SUCCESS;

}

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE image,EFI_SYSTEM_TABLE *t) {

  if(!image||!t||!t->BootServices||!t->RuntimeServices||!t->RuntimeServices->GetVariable)return EFI_INVALID_PARAMETER;

  UINT8 secure=1;UINTN n=1;UINT32 attrs=0;

  EFI_STATUS s=t->RuntimeServices->GetVariable(L"SecureBoot",&GlobalGuid,&attrs,&n,&secure);

  /* Authenticode signature verified by UEFI firmware against custom DB key. Allow SecureBoot. */

  (void)secure;

  s=GuBootOpen(image,t);if(s!=EFI_SUCCESS)return s;

  const char *stage="discover";CHAR16 choice=L'S';

  s=Discover(image,t);if(s!=EFI_SUCCESS)goto failed;

  if (Gu.is_ampere) {

    GuLog("Ampere GPU detected: Device ID 0x%04x\n", (unsigned)Gu.device_id);

    GuLog("GA102 AMPERE UNIVERSAL UNLOCK v1.6.3: Volatile RAM-only override; no VBIOS flash\n");

  } else {

    GuLog("Ada Lovelace GPU detected: Device ID 0x%04x\n", (unsigned)Gu.device_id);

    GuLog("AD10X ADA UNIVERSAL UNLOCK v1.6.3: Volatile RAM-only override; no VBIOS flash\n");

  }

  stage="assets";s=GuBootLoadAssets();if(s!=EFI_SUCCESS||Gu.fault)goto failed;

  stage="read-only-cold-profile";s=GuAd104Prepare();if(s!=EFI_SUCCESS||Gu.fault)goto failed;

  stage="stage-windows-image";s=GuBootLoadWindows();if(s!=EFI_SUCCESS||Gu.fault)goto failed;

  stage="manual-choice";s=GuBootChoose(&choice);if(s!=EFI_SUCCESS||Gu.fault)goto failed;

  if (choice == L'I') {

    GuLog("Operator I: Requesting DXE Driver NVRAM installation\n");

    s = GuInstallDriverOption(image, t);

    if (s == EFI_SUCCESS) {

      GuLog("DXE Driver successfully installed to Driver0001 / DriverOrder\n");

      GuBootSay(L"\r\n\r\n[OK] AdaUnlock DXE Driver installed to UEFI NVRAM (Driver0001)!\r\n"

                L"Driver will execute automatically during early boot on every startup.\r\n"

                L"Rebooting system in 3 seconds...\r\n");

    } else {

      GuLog("ERROR: Failed to install DXE Driver: %#llx\n", (unsigned long long)s);

      GuBootSay(L"\r\n\r\n[ERROR] Failed to install DXE Driver to NVRAM!\r\n"

                L"Rebooting in 5 seconds...\r\n");

      if (t->BootServices->Stall) t->BootServices->Stall(2000000);

    }

    (VOID)GuBootClose();

    if (t->BootServices->Stall) t->BootServices->Stall(3000000);

    if (t->RuntimeServices && t->RuntimeServices->ResetSystem) {

      t->RuntimeServices->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);

    }

    return s;

  }

  if (choice == L'R') {

    GuLog("Operator R: Requesting DXE Driver NVRAM removal\n");

    s = GuRemoveDriverOption(t);

    GuLog("DXE Driver removed from Driver0001 / DriverOrder (status=%#llx)\n", (unsigned long long)s);

    GuBootSay(L"\r\n\r\n[OK] AdaUnlock DXE Driver removed from UEFI NVRAM.\r\n"

              L"Rebooting system in 3 seconds...\r\n");

    (VOID)GuBootClose();

    if (t->BootServices->Stall) t->BootServices->Stall(3000000);

    if (t->RuntimeServices && t->RuntimeServices->ResetSystem) {

      t->RuntimeServices->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);

    }

    return s;

  }

  BOOLEAN unlock = (choice == L'U' || choice == L'P');

  BOOLEAN enable_s3 = (choice == L'P');

  if(!unlock) {

    GuLog("Operator skipped: no GPU transaction, no driver release; stock Windows\n");

    if(Gu.fault||GuBootClose()!=EFI_SUCCESS)return EFI_DEVICE_ERROR;

    return GuBootWindows();

  }

  stage="MSI-preflight";s=GuMsiPreflight(t,image,Gu.handle);if(s!=EFI_SUCCESS||Gu.fault)goto failed;

  GuBootSay(L"\r\nChecks passed. Releasing display; watch for Windows.\r\n");

  GuLog("Operator U: one physical attempt; no retries; display callbacks disabled\n");

  if(Gu.fault){s=EFI_DEVICE_ERROR;goto failed;}

  GuBootSilence();

  stage="MSI-release";s=GuMsiRelease();if(s!=EFI_SUCCESS||Gu.fault)goto failed;

  if(!GuMsiReleased()||!GuAd104ColdEntry()){s=EFI_NOT_READY;goto failed;}

  Gu.display_check=GuMsiReleased;Gu.display_ok=TRUE;Gu.armed=TRUE;

  UINT64 mask=0xffffffffffffULL;

  struct pci_dev dev={.vendor=0x10de,.device=Gu.device_id,.dev={.dma_mask=&mask,.coherent_dma_mask=0xfffffffffULL},

    .current_state=PCI_D0,.pm_cap=Gu.pm_cap,.refs=1};

  struct gpu_unlock_610_shared shared={0};

  stage="shared-assets";int result=gpu_unlock_610_load_shared(&dev,&shared);

  if(!result) {stage="firmware-transaction";result=gpu_unlock_610_unlock(&dev,&shared);}

  gpu_unlock_610_release_shared(&shared);

  GuLog("Transaction result=%d safe=%u retained=%u started=%u quiesced=%u flrs=%u DMA_live=%u\n",

    result,GuSafeToReturn(),Gu.retained,Gu.started,Gu.quiesced,Gu.flrs,Gu.dma_count);

  if(result||Gu.fault||!GuSafeToReturn()){s=EFI_DEVICE_ERROR;goto failed;}

  stage="release-exclusive-claim";s=GuMsiFinish();if(s!=EFI_SUCCESS||Gu.fault)goto failed;

  /* The final pre-Windows check runs after the log closes: this line is not proof of a Windows start. */

  GuLog("TRANSACTION OK: hardware readbacks passed, DMA released; final pre-Windows check follows unlogged; benchmark still required\n");

  /* Probe and register S3 Boot Script for standby persistence if operator chose [P] */

  if (enable_s3) {

    static EFI_GUID S3SaveStateGuid = EFI_S3_SAVE_STATE_PROTOCOL_GUID;

    EFI_S3_SAVE_STATE_PROTOCOL *s3 = NULL;

    EFI_STATUS s3_status = t->BootServices->LocateProtocol(&S3SaveStateGuid, NULL, (VOID **)&s3);

    GuLog("S3-Persistence (requested): LocateProtocol status=%#llx proto=%p\n", (unsigned long long)s3_status, s3);

    if (s3_status == EFI_SUCCESS && s3 && s3->Write) {

      UINT32 val_ss0 = 0x88888888;

      UINT32 val_ss1 = 0x00000008;

      UINT64 addr_ss0 = (UINT64)Gu.bar_base + 0x0082381cULL;

      UINT64 addr_ss1 = (UINT64)Gu.bar_base + 0x00823820ULL;

      EFI_STATUS w0 = s3->Write(s3, EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE, EfiBootScriptWidthUint32, addr_ss0, (UINTN)1, &val_ss0);

      EFI_STATUS w1 = s3->Write(s3, EFI_BOOT_SCRIPT_MEM_WRITE_OPCODE, EfiBootScriptWidthUint32, addr_ss1, (UINTN)1, &val_ss1);

      GuLog("S3-Persistence: registered SS0 write status=%#llx, SS1 write status=%#llx\n",

            (unsigned long long)w0, (unsigned long long)w1);

    } else {

      GuLog("S3-Persistence: S3SaveState protocol not available in DXE phase\n");

    }

  } else {

    GuLog("S3-Persistence: skipped by operator (standard mode U)\n");

  }

  stage="close-USB-log";s=GuBootClose();if(s!=EFI_SUCCESS||Gu.fault)goto failed;

  stage="final-Windows-verification";if(!GuMsiReadyForWindows()){s=EFI_NOT_READY;goto failed;}

  stage="start-Windows";s=GuBootWindows();

  /* A returned Windows image is not a successful handoff. */

  GuBootHalt();return s==EFI_SUCCESS?EFI_ABORTED:s;

failed:

  if(s==EFI_SUCCESS)s=EFI_DEVICE_ERROR;

  GuLog("STOP: stage=%s status=%#llx mutated=%u retained=%u fault=%u\n",stage,(unsigned long long)s,GuMsiMutated(),Gu.retained,Gu.fault);

  if(GuMsiMutated()||Gu.started||Gu.retained) {

    /* Fault-independent containment, no freeing of unverified live mappings. */

    if(Gu.started||Gu.master_attributes_pending)(VOID)GuClearMaster();

    (VOID)GuBootClose();GuBootHalt();return EFI_ABORTED;

  }

  GuBootSay(L"\r\nPreflight refused. No driver release or GPU transaction. Log saved to USB.\r\nReturning to Boot Menu in 3 seconds...\r\n");



  (VOID)GuBootClose();



  if(t&&t->BootServices&&t->BootServices->Stall)t->BootServices->Stall(3000000);



  return s;

}

