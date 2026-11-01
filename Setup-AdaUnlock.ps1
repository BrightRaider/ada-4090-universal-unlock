<#
.SYNOPSIS
    Universal RTX 40 & RTX 30 Series Tensor Unlock Setup Tool (v1.4)
.DESCRIPTION
    Automatisches 1-Klick Setup-Tool zur Erstellung eines bootfähigen
    UEFI-Sticks mit dem universellen Ada & Ampere Rate-Limiter Unlock.
#>

param(
    [switch]$NonInteractive
)

# 1. Administrator-Rechte anfordern
$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if (-not $isAdmin) {
    Write-Host "[INFO] Starte Setup mit Administrator-Rechten neu..." -ForegroundColor Cyan
    Start-Process powershell.exe -Verb RunAs -ArgumentList "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
    exit
}

Clear-Host
Write-Host "======================================================================" -ForegroundColor Magenta
Write-Host "   RTX 40 & 30 SERIES UNIVERSAL UNLOCK - 1-KLICK SETUP (v1.4)          " -ForegroundColor Yellow
Write-Host "   Unterstützt: RTX 4090/D, 4080/S, 4070TiS | RTX 3090/Ti, 3080/Ti   " -ForegroundColor Cyan
Write-Host "======================================================================" -ForegroundColor Magenta
Write-Host ""

# 2. Hardware-Erkennung (GPU)
Write-Host "[1/4] Suche nach kompatibler NVIDIA GeForce GPU (Ada / Ampere)..." -ForegroundColor Cyan
$gpus = Get-CimInstance Win32_VideoController | Where-Object {
    $_.Name -like "*4090*" -or $_.Name -like "*4080*" -or $_.Name -like "*4070 Ti SUPER*" -or
    $_.Name -like "*3090*" -or $_.Name -like "*3080*" -or
    $_.PNPDeviceID -like "*VEN_10DE&DEV_2684*" -or $_.PNPDeviceID -like "*VEN_10DE&DEV_2685*" -or
    $_.PNPDeviceID -like "*VEN_10DE&DEV_2704*" -or $_.PNPDeviceID -like "*VEN_10DE&DEV_2703*" -or
    $_.PNPDeviceID -like "*VEN_10DE&DEV_2705*" -or
    $_.PNPDeviceID -like "*VEN_10DE&DEV_2204*" -or $_.PNPDeviceID -like "*VEN_10DE&DEV_2208*" -or
    $_.PNPDeviceID -like "*VEN_10DE&DEV_2206*" -or $_.PNPDeviceID -like "*VEN_10DE&DEV_220A*" -or
    $_.PNPDeviceID -like "*VEN_10DE&DEV_2216*"
}

if ($gpus) {
    foreach ($gpu in $gpus) {
        Write-Host "  [OK] Gefundene GPU: $($gpu.Name) ($($gpu.PNPDeviceID))" -ForegroundColor Green
    }
} else {
    Write-Host "  [WARNUNG] Keine Ziel-GPU im aktuellen System gefunden (Setup kann trotzdem fortgesetzt werden)." -ForegroundColor Yellow
}

Write-Host ""
Write-Host "[2/4] Suche nach USB-Laufwerken..." -ForegroundColor Cyan

# 3. USB-Laufwerk auswählen
$usbDrives = Get-CimInstance Win32_Volume | Where-Object { $_.DriveType -eq 2 -and $_.DriveLetter }

if (-not $usbDrives) {
    Write-Host "  [FEHLER] Kein Wechseldatenträger (USB-Stick) gefunden!" -ForegroundColor Red
    Write-Host "  Bitte stecke einen USB-Stick an und drücke eine Taste zum erneuten Suchen..." -ForegroundColor Yellow
    [void][System.Console]::ReadKey($true)
    $usbDrives = Get-CimInstance Win32_Volume | Where-Object { $_.DriveType -eq 2 -and $_.DriveLetter }
    if (-not $usbDrives) {
        Write-Host "  [ABBRUCH] Kein USB-Stick erkannt." -ForegroundColor Red
        Pause
        exit
    }
}

$targetDrive = $null
if ($usbDrives.Count -eq 1) {
    $targetDrive = $usbDrives[0]
    Write-Host "  [INFO] Automatisch gewähltes USB-Laufwerk: $($targetDrive.DriveLetter) ($($targetDrive.Label), $([math]::Round($targetDrive.Capacity / 1GB, 2)) GB)" -ForegroundColor Green
} else {
    Write-Host "  Mehrere USB-Laufwerke erkannt. Bitte wählen:" -ForegroundColor Yellow
    for ($i = 0; $i -lt $usbDrives.Count; $i++) {
        Write-Host "    [$($i+1)] $($usbDrives[$i].DriveLetter) ($($usbDrives[$i].Label), $([math]::Round($usbDrives[$i].Capacity / 1GB, 2)) GB)"
    }
    $choice = Read-Host "  Nummer eingeben (1-$($usbDrives.Count))"
    $targetDrive = $usbDrives[[int]$choice - 1]
}

$driveLetter = $targetDrive.DriveLetter
Write-Host ""
Write-Host "  ZIEL-LAUFWERK: $driveLetter" -ForegroundColor Yellow
Write-Host "  ACHTUNG: Es wird empfohlen, den Stick in FAT32 zu verwenden." -ForegroundColor Gray
$formatAnswer = Read-Host "  Möchtest du das Laufwerk $driveLetter jetzt schnell formatieren? (j/N)"

if ($formatAnswer -match "^[jJyY]") {
    Write-Host "  Formatiere $driveLetter mit FAT32..." -ForegroundColor Cyan
    Format-Volume -DriveLetter $driveLetter.TrimEnd(':') -FileSystem FAT32 -NewFileSystemLabel "GPU_UNLOCK" -Confirm:$false | Out-Null
    Write-Host "  [OK] Formatierung abgeschlossen." -ForegroundColor Green
}

# 4. Dateien kopieren
Write-Host ""
Write-Host "[3/4] Kopiere Universal Booter & Firmware-Assets (Ada & Ampere)..." -ForegroundColor Cyan

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$efiSrc = Join-Path $scriptDir "bin\BOOTX64.EFI"
$ad102Src = Join-Path $scriptDir "AD102-ASSETS"
$ga102Src = Join-Path $scriptDir "GA102-ASSETS"
$certSrc = Join-Path $scriptDir "AdaUnlock_SecureBoot.cer"

$efiDstDir = Join-Path $driveLetter "EFI\BOOT"
New-Item -ItemType Directory -Path $efiDstDir -Force | Out-Null
Copy-Item $efiSrc (Join-Path $efiDstDir "BOOTX64.EFI") -Force
Write-Host "  [OK] UEFI Bootloader kopiert nach $driveLetter\EFI\BOOT\BOOTX64.EFI" -ForegroundColor Green

if (Test-Path $ad102Src) {
    $adDst = Join-Path $driveLetter "AD102-ASSETS"
    New-Item -ItemType Directory -Path $adDst -Force | Out-Null
    Copy-Item (Join-Path $ad102Src "*") $adDst -Force
    Write-Host "  [OK] Ada (AD102/AD103) Assets kopiert nach $driveLetter\AD102-ASSETS\" -ForegroundColor Green
}

if (Test-Path $ga102Src) {
    $gaDst = Join-Path $driveLetter "GA102-ASSETS"
    New-Item -ItemType Directory -Path $gaDst -Force | Out-Null
    Copy-Item (Join-Path $ga102Src "*") $gaDst -Force
    $gspDst = Join-Path $gaDst "gsp_ga10x.bin"
    if (-not (Test-Path $gspDst)) {
        $gspSrc = Join-Path $ad102Src "gsp_ga10x.bin"
        if (Test-Path $gspSrc) {
            Copy-Item $gspSrc $gspDst -Force
        }
    }
    Write-Host "  [OK] Ampere (GA102) Assets kopiert nach $driveLetter\GA102-ASSETS\" -ForegroundColor Green
}

if (Test-Path $certSrc) {
    Copy-Item $certSrc (Join-Path $driveLetter "AdaUnlock_SecureBoot.cer") -Force
    Write-Host "  [OK] Secure Boot Zertifikat kopiert nach $driveLetter\AdaUnlock_SecureBoot.cer" -ForegroundColor Green
}

# 5. Integritätsprüfung
Write-Host ""
Write-Host "[4/4] Verifiziere SHA-256 Prüfsummen..." -ForegroundColor Cyan

$adHashes = @{
    "booter_load_ad102.bin" = "6ee05b20e1dc244c08c052f36ef28469b8d09f4955d1fe33e48a179e11a7fd49"
    "v67-ad102-610.43.03-four-plm.bin" = "44c36b2d6b3424e53e18f3e55db85c833655de071ce66ce42749588a4a7dc8cb"
    "gsp_ga10x.bin" = "572373620a37418f24dc16b5031c39338778c3257e48e8408de9a57291b24f3a"
    "gsp_boot_ad102.bin" = "45d6335a8185d6ff13e8d1484cea73f68d3d13aa1c19dc080ed1c3482dd5bac0"
    "gsp_boot_desc_ad102.bin" = "83f8f3f1f5f9496e0ea8fc7103c2beb64d0b6de2f5ffe725f13514de5a95fa42"
}

$gaHashes = @{
    "booter_load_ga102.bin" = "056cbc064a888f80717726c54d7f64fcd68044d123870c95a74f37f4975dcaa4"
    "v67-ga102-610.43.03.bin" = "2a0aa1f86d0b6afff5214d641d87d46dc98ef06ed908ff05d0ddbad5ffb7e197"
    "gsp_ga10x.bin" = "572373620a37418f24dc16b5031c39338778c3257e48e8408de9a57291b24f3a"
    "gsp_boot_ga102.bin" = "3df651acb174fd5f5561a1996965b192f50c5d763168446ed0d4ead19d895768"
    "gsp_boot_desc_ga102.bin" = "cc5a3fc1f4c47f2fe107b1b68f0176d8a448e6dec84b575e5560ccef5262bf15"
}

$allValid = $true
if (Test-Path (Join-Path $driveLetter "AD102-ASSETS")) {
    Write-Host "  Prüfe AD102 Assets:" -ForegroundColor Gray
    foreach ($name in $adHashes.Keys) {
        $filePath = Join-Path $driveLetter "AD102-ASSETS\$name"
        if (Test-Path $filePath) {
            $actualHash = (Get-FileHash -Path $filePath -Algorithm SHA256).Hash.ToLower()
            if ($actualHash -eq $adHashes[$name]) {
                Write-Host "    [OK] ${name}: bitgenau verifiziert." -ForegroundColor Green
            } else {
                Write-Host "    [FEHLER] ${name}: Hash-Fehler ($actualHash)" -ForegroundColor Red
                $allValid = $false
            }
        }
    }
}

if (Test-Path (Join-Path $driveLetter "GA102-ASSETS")) {
    Write-Host "  Prüfe GA102 Assets:" -ForegroundColor Gray
    foreach ($name in $gaHashes.Keys) {
        $filePath = Join-Path $driveLetter "GA102-ASSETS\$name"
        if (Test-Path $filePath) {
            $actualHash = (Get-FileHash -Path $filePath -Algorithm SHA256).Hash.ToLower()
            if ($actualHash -eq $gaHashes[$name]) {
                Write-Host "    [OK] ${name}: bitgenau verifiziert." -ForegroundColor Green
            } else {
                Write-Host "    [FEHLER] ${name}: Hash-Fehler ($actualHash)" -ForegroundColor Red
                $allValid = $false
            }
        }
    }
}

Write-Host ""
Write-Host "======================================================================" -ForegroundColor Magenta
if ($allValid) {
    Write-Host "             SETUP ERFOLGREICH ABGESCHLOSSEN!                         " -ForegroundColor Green
} else {
    Write-Host "       SETUP MIT WARNUNGEN/FEHLERN ABGESCHLOSSEN                      " -ForegroundColor Red
}
Write-Host "======================================================================" -ForegroundColor Magenta
Write-Host ""
Write-Host "ANLEITUNG ZUM STARTEN:" -ForegroundColor Yellow
Write-Host "1. PC neu starten und beim Hochfahren das Boot-Menü öffnen (meist F11 oder F12)."
Write-Host "2. Den USB-Stick auswählen ('UEFI: USB...')."
Write-Host "3. Falls Secure Boot aktiv ist und meckert:"
Write-Host "   - BIOS -> Security / Secure Boot -> Key Management -> 'Enroll Signature' (oder 'Append db')."
Write-Host "   - Wähle die Datei 'AdaUnlock_SecureBoot.cer' vom USB-Stick aus und speichere mit F10."
Write-Host "4. Der Unlock läuft automatisch ab (in 0,3s) und Windows startet mit voller Rechenleistung!"
Write-Host ""
Write-Host "Drücke eine beliebige Taste zum Beenden..."
[void][System.Console]::ReadKey($true)
