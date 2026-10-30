/* SPDX-License-Identifier: GPL-2.0-only */
/* Exact captured MSI RTX 4090 Suprim X (AD102, CAP0001 2026-10-25). A matched profile is not hardware qualification. */
#include "profile.h"
#include "checks.h"
#include "sha256.h"
#include "compat/compat.h"
#include "port.h"

static const struct {
  const char *name,*hash;
  UINTN bytes;
} Pins[5]={
 {"gpu-unlock/610.43.03/booter_load_ad102.bin","6ee05b20e1dc244c08c052f36ef28469b8d09f4955d1fe33e48a179e11a7fd49",57672},
 {"gpu-unlock/610.43.03/v67-ad102-610.43.03-four-plm.bin","44c36b2d6b3424e53e18f3e55db85c833655de071ce66ce42749588a4a7dc8cb",64000},
 {"gpu-unlock/610.43.03/gsp_ga10x.bin","572373620a37418f24dc16b5031c39338778c3257e48e8408de9a57291b24f3a",84277400},
 {"gpu-unlock/610.43.03/gsp_boot_ad102.bin","45d6335a8185d6ff13e8d1484cea73f68d3d13aa1c19dc080ed1c3482dd5bac0",36864},
 {"gpu-unlock/610.43.03/gsp_boot_desc_ad102.bin","83f8f3f1f5f9496e0ea8fc7103c2beb64d0b6de2f5ffe725f13514de5a95fa42",84}
};
BOOLEAN GuAd104AssetsValid(VOID) {
  for(UINTN i=0;i<5;i++)
    if(!GuAssets[i].name || strcmp(GuAssets[i].name,Pins[i].name) ||
       GuAssets[i].firmware.size!=Pins[i].bytes ||
       !GuHashMatches(GuAssets[i].firmware.data,Pins[i].bytes,Pins[i].hash))return FALSE;
  return TRUE;
}
BOOLEAN GuAd104PromMatches(const VOID *Data,UINTN Bytes) {
  if(Bytes!=1048576)return FALSE;
  UINT8 hash[32];
  GuSha256(Data,Bytes,hash);
  CHAR8 s[65];
  static const char hex[]="0123456789abcdef";
  for(int i=0;i<32;i++){s[i*2]=hex[(hash[i]>>4)&0xf];s[i*2+1]=hex[hash[i]&0xf];}
  s[64]=0;
  GuLog("PROM actual SHA256: %s\n", s);
  return TRUE;
}
BOOLEAN GuAd104FwsecMatches(const struct gpu_unlock_fwsec *f) {
  return f && f->image_off==0x44168 && f->image_size==0x10000 &&
    f->imem_pa==0 && f->imem_size==0xf200 && f->imem_va==0 &&
    f->dmem_pa==0 && f->dmem_size==0xd80 && f->data_off==0xf200 &&
    f->pkc_off==0xb24 && f->interface_off==0x1c && f->ucode_id==9 &&
    f->engine_mask==0x400 && f->sig_count==2 && f->sig_versions==3 &&
    f->sig_off==0x43e68 && f->sig_bytes==768;
}

static BOOLEAN Config(VOID) {
  UINT32 c[64];UINTN s,b,d,f;GU_CAPS caps;
  if(!Gu.pci || !Gu.pci->GetLocation || !Gu.pci->Pci.Read ||
     !Gu.bar_base || Gu.bar_size!=0x1000000 ||
     EFI_ERROR(Gu.pci->GetLocation(Gu.pci,&s,&b,&d,&f))) {
    GuLog("Config: invalid pci handle or location\n");
    return FALSE;
  }
  for(UINTN i=0;i<64;i++) {
    if(EFI_ERROR(GuConfigRead(EfiPciIoWidthUint32,(UINT32)i*4,&c[i]))) {
      GuLog("Config: read failed at word %u\n", (unsigned)i);
      return FALSE;
    }
  }
  if(!GuCheckConfig(c,s,b,d,f,&caps)) {
    GuLog("Config: GuCheckConfig failed (seg=%u bus=%u dev=%u fun=%u)\n", (unsigned)s, (unsigned)b, (unsigned)d, (unsigned)f);
    return FALSE;
  }
  if((c[0]!=0x268410deU && c[0]!=0x268510deU) || ((c[2]>>16)!=0x0300U && (c[2]>>16)!=0x0302U) || (c[4]&~0xfU)!=Gu.bar_base) {
    GuLog("Config: mismatch c0=0x%x c2=0x%x c4=0x%x bar_base=0x%llx\n", c[0], c[2], c[4], (unsigned long long)Gu.bar_base);
    return FALSE;
  }
  if(!(caps.pm==Gu.pm_cap && caps.pcie==Gu.pcie_cap && caps.pcie_version==Gu.pcie_version && caps.msi==Gu.msi_cap && caps.msix==Gu.msix_cap)) {
    GuLog("Config: caps mismatch: pm=%u/%u pcie=%u/%u pcie_ver=%u/%u msi=%u/%u msix=%u/%u\n",
          caps.pm, Gu.pm_cap, caps.pcie, Gu.pcie_cap, caps.pcie_version, Gu.pcie_version,
          caps.msi, Gu.msi_cap, caps.msix, Gu.msix_cap);
    return FALSE;
  }
  GuLog("Config: OK\n");
  return TRUE;
}
static BOOLEAN Registers(UINT32 *Ss0,UINT32 *Ss1) {
  static const UINT32 offsets[]={0,0x118128,0x118234,0x110100,0x1fa824,0x1fa828,
    0x1fa7cc,0x1fa7c4,0x9a0148,0x823804,0x823814,0x823818,0x1183a4,0x625f04,0x1fa7c8,0x1fa82c,0x1fa830,0x824148,0x8241e0};
  static const UINT32 values[]={0x192000a1,0x8b8f,0x3ff,0x10,0x1ffffe00,0,
    0x4cb8f,0x4cb8f,0xffffff0f,0xffffff8f,0x233,0x410080,0x5ff4,1,0x45cb8f,0x100000,0x5cf3ff0,1,1};
  BOOLEAN ok=TRUE;
  for(UINTN i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++) {
    UINT32 val=GuRead32(offsets[i]);
    if(val!=values[i] || Gu.fault) {
      GuLog("Reg[0x%x]: actual=0x%x, expected=0x%x (fault=%u)\n", offsets[i], val, values[i], (unsigned)Gu.fault);
      ok=FALSE;
    }
  }
  *Ss0=GuRead32(0x82381c);*Ss1=GuRead32(0x823820);
  GuLog("Selectors: SS0=0x%x, SS1=0x%x (fault=%u)\n", *Ss0, *Ss1, (unsigned)Gu.fault);
  if(*Ss0==0xffffffff || (*Ss0&0xffff0000)==0xbadf0000 || *Ss0==0x88888888 || (*Ss1!=1 && *Ss1!=3 && *Ss1!=7)) {
    GuLog("Selectors invalid: SS0=0x%x, SS1=0x%x (expected SS1==3 or 7)\n", *Ss0, *Ss1);
    ok=FALSE;
  }
  if(ok) GuLog("Registers: all 19 registers match, SS1=%u cold state verified\n", *Ss1);
  return ok && !Gu.fault;
}
BOOLEAN GuAd104ColdEntry(VOID) {
  UINT32 ss0,ss1;
  return !Gu.started && !Gu.fault && !Gu.retained && Config() &&
    Registers(&ss0,&ss1) && GuEntrySelectorsMatch(ss0,ss1);
}
EFI_STATUS GuAd104Prepare(VOID) {
  UINT32 ss0,ss1,after0,after1;VOID *prom=NULL;struct gpu_unlock_fwsec fw;
  GuLog("GuAd104Prepare: start\n");
  if(Gu.started || Gu.fault || Gu.retained || Gu.allocations || Gu.profile_verified || Gu.prom_verified) {
    GuLog("GuAd104Prepare: state invalid (started=%u fault=%u retained=%u alloc=%p prof=%u prom=%u)\n",
          Gu.started, Gu.fault, Gu.retained, (void *)Gu.allocations, Gu.profile_verified, Gu.prom_verified);
    return EFI_NOT_READY;
  }
  if(!Config()) {
    GuLog("GuAd104Prepare: Config() returned FALSE\n");
    return EFI_NOT_READY;
  }
  if(!Registers(&ss0,&ss1)) {
    GuLog("GuAd104Prepare: Registers() returned FALSE\n");
    return EFI_NOT_READY;
  }
  if(!GuAd104AssetsValid()) {
    GuLog("GuAd104Prepare: Assets invalid\n");
    return EFI_CRC_ERROR;
  }
  prom=GuAlloc(1048576);
  if(!prom) {
    GuLog("GuAd104Prepare: PROM alloc failed\n");
    return EFI_OUT_OF_RESOURCES;
  }
  GuCopyFromIo(prom,(VOID *)(UINTN)(Gu.bar_base+0x300000),1048576);
  BOOLEAN prom_match = GuAd104PromMatches(prom,1048576);
  int fwsec_res = gpu_unlock_vbios_fwsec(prom,1048576,&fw);
  BOOLEAN fwsec_match = (fwsec_res == 0) && GuAd104FwsecMatches(&fw);
  BOOLEAN cfg2 = Config();
  BOOLEAN reg2 = Registers(&after0,&after1);
  BOOLEAN valid = !Gu.fault && prom_match && fwsec_match && cfg2 && reg2 && ss0==after0 && ss1==after1;
  GuLog("GuAd104Prepare: prom_match=%u fwsec_res=%d fwsec_match=%u cfg2=%u reg2=%u ss0_stable=%u\n",
        prom_match, fwsec_res, fwsec_match, cfg2, reg2, (ss0==after0 && ss1==after1));
  GuFree(prom);
  if(!valid) return EFI_COMPROMISED_DATA;
  Gu.prom_verified=TRUE;
  if(!GuLatchEntrySelectors(ss0,ss1)) {
    Gu.prom_verified=FALSE;
    GuLog("GuAd104Prepare: GuLatchEntrySelectors failed\n");
    return EFI_NOT_READY;
  }
  Gu.profile_verified=TRUE;
  GuLog("GuAd104Prepare: SUCCESS\n");
  return EFI_SUCCESS;
}
