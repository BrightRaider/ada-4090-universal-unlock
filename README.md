# Universal GPU Unlock (One-Click UEFI Tool) - Ada & Ampere Edition

[![Architecture](https://img.shields.io/badge/Architecture-Ada%20Lovelace%20%26%20Ampere-green.svg)](#)
[![GPU](https://img.shields.io/badge/GPU-RTX%204090%20%7C%204080%20%7C%203090%20%7C%203080-blue.svg)](#)
[![Safety](https://img.shields.io/badge/Safety-100%25%20RAM--Only%20(Zero%20EEPROM%20Writes)-success.svg)](#)
[![OS](https://img.shields.io/badge/OS-Windows%2010%20%2F%2011%20%7C%20WSL2-orange.svg)](#)
[![Secure Boot](https://img.shields.io/badge/Secure%20Boot-Compatible%20(Custom%20DB)-blueviolet.svg)](#)

---

> [!IMPORTANT]
> # 🌟 Special Dedication & Huge Thanks to geo8 ([`geodnb`](https://github.com/geodnb))
> ### 🏆 The Project That Started It All: [**Ada Unlock Readiness Check (`AdaUnlockCheck`)**](https://github.com/geodnb/RTX40-series-tensor-unlock-readiness-checker)
> 
> **A massive, heartfelt personal thank you to [geo8 (`geodnb`)](https://github.com/geodnb)!**
> 
> My entire work on this project was sparked directly by his tool: [**`RTX40-series-tensor-unlock-readiness-checker`**](https://github.com/geodnb/RTX40-series-tensor-unlock-readiness-checker). Running `AdaUnlockCheck` on my own hardware gave me the critical register verification, diagnostic baseline, and the direct inspiration I needed to build this universal, 1-click dynamic UEFI pre-boot unlocker.
>
> Without his pioneering diagnostic framework and public research, this universal Windows release would simply not exist today. **Thank you, geo8! 🙌**

---


A universal, automated, one-click UEFI pre-boot tool that completely removes the hardware/firmware rate limiter on **NVIDIA GeForce RTX 40 Series (Ada Lovelace)** and **RTX 30 Series (Ampere)** graphics cards. 

The unlock takes place purely in volatile GPU RAM during pre-boot; **zero bits are written to the VBIOS EEPROM**. The unmodified official Windows NVIDIA driver (`nvlddmkm.sys`) loads immediately afterwards, recognizing the fully unlocked tensor cores across Windows, PyTorch, CUDA, WSL2, and Docker.

---

## 🚀 Supported Hardware

| Generation | Architecture | Supported Models | Device IDs |
| :--- | :--- | :--- | :--- |
| **RTX 40 Series** | **Ada Lovelace** | RTX 4090, RTX 4090D, RTX 4080, RTX 4080 SUPER, RTX 4070 Ti SUPER | `0x2684`, `0x2685`, `0x2704`, `0x2703`, `0x2705` |
| **RTX 30 Series** | **Ampere** | RTX 3090, RTX 3090 Ti, RTX 3080 Ti, RTX 3080 (10GB & 12GB), CMP 90HX | `0x2204`, `0x2208`, `0x2206`, `0x220a`, `0x2216`, `0x2207` |

- **Any AIB Vendor:** Works across Founders Edition, ASUS, MSI, Gigabyte, ZOTAC, PNY, Palit, Gainward, Inno3D, Galax, etc.
- **Any Motherboard & Platform:** AMD AM4/AM5 (X570, B550, B650, X670) and Intel LGA1700 (Z690, Z790, B760) with dynamic PCIe bus scanning.

---

## 📊 Live Verified Benchmarks

### NVIDIA GeForce RTX 4090 (Ada Lovelace AD102)
Measured on real hardware under Windows 10 (CUDA 13.0 / PyTorch 2.10) using `tools/benchmark_4090.py`:

| Compute Operation | Matrix Size ($N 	imes N$) | Latency per Iteration | Throughput (TFLOPS) | Status |
| :--- | :--- | :--- | :--- | :--- |
| **FP16 Tensor Cores** | $4096 	imes 4096$ | **0.670 ms** | **205.04 TFLOPS** | 🔥 Fully Unlocked (+40% vs Stock) |
| **FP16 Tensor Cores** | $8192 	imes 8192$ | **7.300 ms** | **150.62 TFLOPS** | 🔥 Fully Unlocked |
| **FP16 Tensor Cores** | $16384 	imes 16384$ | **51.077 ms** | **172.21 TFLOPS** | 🔥 Fully Unlocked |
| **BF16 Tensor Cores** | $4096 	imes 4096$ | **1.040 ms** | **132.16 TFLOPS** | 🔥 Fully Unlocked |
| **BF16 Tensor Cores** | $8192 	imes 8192$ | **6.332 ms** | **173.65 TFLOPS** | 🔥 Fully Unlocked |
| **BF16 Tensor Cores** | $16384 	imes 16384$ | **51.650 ms** | **170.30 TFLOPS** | 🔥 Fully Unlocked |
| **FP32 CUDA Baseline** | $4096 	imes 4096$ | **2.616 ms** | **52.54 TFLOPS** | ✅ Stock Parity |

### NVIDIA GeForce RTX 3090 (Ampere GA102)
Measured on real hardware under Windows:

| Compute Mode | Stock Throughput | Unlocked Throughput | Performance Delta |
| :--- | :--- | :--- | :--- |
| **FP16 Tensor (FP32 Accum)** | **69.31 TFLOPS** | **130.44 TFLOPS** | 🚀 **+88.2% Boost** |
| **BF16 Tensor** | Rate Limited | **130+ TFLOPS** | 🔥 Full Compute |
| **FP32 CUDA Baseline** | 27.51 TFLOPS | 27.76 TFLOPS | ✅ Parity Preserved |

---

## ⚡ Quickstart Guide (For End Users)

### Step 1: Create the USB Drive
1. Download the latest release `.zip` from the [Releases](https://github.com/BrightRaider/ada-4090-universal-unlock/releases) page and extract it.
2. Insert any USB flash drive (1 GB or larger).
3. Right-click **`Setup.bat`** and select **"Run as Administrator"** (or run `Setup-AdaUnlock.ps1`).
4. Follow the prompt to format and copy the necessary EFI bootloader, microcode assets, and certificates.

### Step 2: Boot & Unlock
1. Restart your PC and press your motherboard's boot-menu key (**F11** on MSI/ASRock, **F12** on Gigabyte, **F8** on ASUS).
2. Select the USB flash drive (`UEFI: USB...`).
3. Press **`U`** at the prompt (or wait for the automatic timeout).
4. The transaction executes in **~300 ms**:
   - Isolates the GPU BAR0 MMIO.
   - Boots FWSEC & SEC2 Falcon coprocessors with architecture-specific pinned firmware.
   - Rewrites rate-limiter selectors (`SS0 -> 0x88888888`, `SS1 -> 0x8`).
   - Executes PCIe Function Level Reset (FLR).
   - Seamlessly hands execution over to Windows Boot Manager (`\EFI\Microsoft\Bootootmgfw.efi`).
5. Windows boots up with full tensor performance!

---

## 🔒 Secure Boot Configuration

If your system has UEFI Secure Boot enabled:
1. During boot, enter BIOS Setup (**Del** key).
2. Navigate to **Security** $
ightarrow$ **Secure Boot** $
ightarrow$ **Key Management** (or *Custom Key Management*).
3. Select **"Append Key"** / **"Enroll Signature"** under the **Authorized Signatures (db)** menu.
4. Select your USB drive and choose **`AdaUnlock_SecureBoot.cer`**.
5. Save changes and reboot (**F10**).
*Secure Boot remains 100% active and protected against malware while allowing the unlock binary to run.*

---

## 🛠️ Repository Structure

```text
├── bin/
│   └── BOOTX64.EFI              <- Pre-compiled, Authenticode-signed universal UEFI binary
├── AD102-ASSETS/                <- Ada microcode payloads (4090, 4080, 4070TiS)
├── GA102-ASSETS/                <- Ampere microcode payloads (3090, 3090Ti, 3080, 3080Ti)
├── src/                         <- Full C source code for the universal UEFI application
│   ├── app.c                    <- Main UEFI entry point and GPU detection
│   ├── checks.c                 <- PCIe configuration space and capability parser
│   ├── profile.c                <- Architecture cold-state gate (Ada & Ampere)
│   ├── port.c                   <- Dynamic BAR0 and MMIO access routines
│   ├── msi_release.c            <- Dynamic UEFI GOP disconnect & exclusive isolation
│   ├── boot_io.c                <- Dynamic Windows ESP discovery & dual-architecture asset loader
│   ├── vendor/                  <- V67 booter and FWSEC microcode interface
│   └── Build-Experimental-MSVC.ps1 <- Automated MSVC build script
├── Setup.bat                    <- 1-Click Provisioner Launcher
└── Setup-AdaUnlock.ps1          <- Automated PowerShell deployment tool
```

---

## ⚠️ Safety & Reversibility Guarantee

- **Zero VBIOS Flashing:** The VBIOS EEPROM chip is **never** written to.
- **Volatile RAM-only:** All register modifications exist solely in GPU RAM and are completely erased on power-off or warm reboot.
- **Fail-Safe Containment:** If any checksum, architecture check, or MMIO verification fails during pre-boot, the tool aborts safely and boots unmodified stock Windows.
---

## 🙏 Acknowledgements & Credits

* 🌟 **[geo8 / geodnb](https://github.com/geodnb) (Special Dedication):** Creator of the original [**Ada Unlock Readiness Check (`RTX40-series-tensor-unlock-readiness-checker`)**](https://github.com/geodnb/RTX40-series-tensor-unlock-readiness-checker). His diagnostic suite and hardware register analysis were the direct catalyst and inspiration for this entire universal Windows project. Special personal thanks for sparking this breakthrough!
* **Zagitta & WildFlash1st:** For pioneering the Linux `gpu-unlock` research and `cmp90hx-unlock` rejoining methodologies.
* **TOTORONG & JustAResearcher:** For early proof-of-concept Windows UEFI implementations.
* **Igor Wallossek (Igor'sLAB):** For in-depth technical analysis, power verification, and community discussion.
