# Changelog - Universal Multi-Gen GPU Unlock

All notable changes, architectural improvements, and device expansions for Universal GPU Unlock are documented here.

For packaged binaries and downloads, visit [GitHub Releases](https://github.com/BrightRaider/universal-gpu-unlock/releases).

---

## [v1.7.3] - 2026-11-06
### Dynamic Multi-Drive Windows ESP Discovery (Resolves #8)
- **Multi-Drive Auto-Targeting:** The bootloader no longer halts when multiple drives contain an EFI System Partition (`matches > 1`). Instead, it automatically targets and boots the primary Windows OS medium (`\EFI\Microsoft\Boot\bootmgfw.efi`), eliminating the need to disconnect secondary drives or repartition disks.
- **Broadened Device Path Parser:** Relaxed rigid media geometry constraints in `Inspect()` to natively support complex storage configurations, PCIe switches, NVMe namespace controllers, and MBR/GPT hybrid schemes.
- **Multi-Path Fallback:** Added dual-mode path resolution supporting relative and root-qualified bootloader paths (`\EFI\Microsoft\Boot\bootmgfw.efi`, `EFI\Microsoft\Boot\bootmgfw.efi`, and `\EFI\Boot\bootx64.efi`).
- **Logging Engine:** Added full `%p` pointer formatting support to `src/format.c`, eliminating false `Gu.fault` triggers during diagnostic logging.

---

## [v1.7.2] - 2026-11-06
### Pre-Boot Progress Monitor & S3 Sleep Protection Tool
- **Real-Time 4-Stage Pre-Boot Monitor:** Replaced the silent pre-boot console with a clean 4-stage progress dashboard in UEFI displaying discovery, microcode, GOP handover, and hardware unlock status before handing control to Windows.
- **S3 Protection Helper:** Added `Toggle-Sleep-Button.bat` to the root directory—a non-invasive 1-click tool to hide or restore the "Sleep" button from the Windows Start menu to prevent accidental volatile RAM state resets.
- **Telemetry Refinement:** Authenticode-signed fresh binaries in `bin/` (`BOOTX64.EFI` and `AdaUnlockDxe.efi`).

---

## [v1.7.1] - 2026-11-06
### Multi-dGPU Secondary Slot SEC2 Timeout Scaling (Resolves #6)
- **SEC2 Booter Timeout Scaling:** Scaled SEC2 booter halt deadline from 5.0s to 20.0s (`GU_BOOT_TIMEOUT_US = 20000000u`), resolving `error=-110` (`ETIMEDOUT`) halts on complex multi-dGPU platforms (e.g. Intel Arc primary + NVIDIA RTX secondary on AM5 X670E).
- **Device-ID Expansion:**
  - GA104: RTX 3070 Ti (`0x2482`), RTX 3070 (`0x2484`), RTX 3060 Ti (`0x2487`, `0x2488`, `0x2489`).
  - GA106: RTX 3060 12GB (`0x2503`, `0x2504`, `0x2507`, `0x2508`, `0x2520`, `0x2560`).
  - AD103: RTX 4080 SUPER revision (`0x2702`).

---

## [v1.7.0] - 2026-11-05
### Turing (SM75) Architecture Support
- **Full TU102 / TU104 / TU106 Support:** Added native architectural payload validation, FWSEC geometry extraction, and pre-boot rate-limiter bypass for RTX 2080 Ti, TITAN RTX, RTX 2080 SUPER, RTX 2070 SUPER, RTX 2060 SUPER, and CMP 50HX.
- **Universal Multi-Gen Staging:** Unified Ada Lovelace, Ampere, and Turing execution paths into a single dynamic pre-boot dispatcher.

---

## [v1.6.3] - 2026-11-04
### Automated Multi-Arch Payload Dispatch
- **Dynamic Microcode Selection:** Automated asset staging selecting between `AD102-ASSETS` and `GA102-ASSETS` based on runtime PCI Device ID.
- **Zero VBIOS Touch:** Strict enforcement of volatile RAM-only register overrides.

---

## [v1.6.2] - 2026-11-03
### Dual-Architecture Calibration & Stability
- Added Ampere SM86 matrix baseline benchmarks (`tools/benchmark_4090.py`).
- Refined PCIe bus scanning logic for multi-GPU motherboards.

---

## [v1.6.1] - 2026-11-02
### Authenticode Secure Boot Hardening
- Integrated custom Authenticode certificate enrollment (`AdaUnlock_SecureBoot.cer`).
- Supported seamless execution with Secure Boot enabled across major motherboard vendors.

---

## [v1.6] - 2026-11-01
### Early DXE Driver Architecture & S3 State Persistence
- Introduced `AdaUnlockDxe.efi` and `driver_nvram.c` for persistent boot script registration.
- Added NVRAM `Driver0001` auto-boot infrastructure.

---

## [v1.5] - 2026-10-31
### Internationalization & Production AI Benchmarks
- Full English localization across all scripts (`Setup.bat`, `Setup-AdaUnlock.ps1`).
- Added FLUX.1-dev and MiniMax-H3 Video DiT production benchmark suites.

---

## [v1.4] - 2026-10-30
### Device-ID Initialization Fix
- Resolved regression where `Gu.device_id` was cleared during early port init, fixing `stage=shared-assets status=-19`.

---

## [v1.3] - 2026-10-29
### Multi-Display & GOP Isolation
- Replaced single-child constraints with dynamic multi-output enumeration (supporting up to 8 displays).
- Relaxed root bridge protocol assertions across ASUS ROG, Gigabyte Aorus, and ASRock motherboards.
