# Skyscraper 3.22.0-dev-checksumdat for Windows (64-bit)

Prebuilt Windows build of this fork, with the `checksumDat=` option (see [FORK.md](../../FORK.md)).
Upstream does not support Windows; neither does this build. Use at your own risk.

| | |
|---|---|
| Version | `3.22.0-dev-checksumdat` |
| Source | This repository, branch `master` (code identical to `feat_checksum_dat` at `733c8ef`, plus `VERSION.ini`) |
| Toolchain | Qt 6.8.2, LLVM-MinGW 17.0.6, x86_64 |
| Tested on | Windows 11 Pro (build 26200) |
| `Skyscraper.exe` SHA-256 | `2E777DE10EBD24EF5037468AAF4B128D35564156196F1000793A7250D1A85F39` |

## Install

1. Copy this whole folder anywhere (the DLLs and the subfolders must stay next to
   `Skyscraper.exe`).
2. Skyscraper reads its configuration from `%USERPROFILE%\.skyscraper`. Create it from the files in
   this repository with the upstream helper, run from the repository root:

   ```powershell
   .\win32\deploy_mandatory_files.ps1 . "$env:USERPROFILE\.skyscraper"
   ```

   Files you have already customized (`artwork.xml`, `peas.json`…) are kept; the new versions are
   saved with a `.dist` suffix.
3. Edit `%USERPROFILE%\.skyscraper\config.ini`. To use precalculated checksums, add for example:

   ```ini
   [ps2]
   checksumDat="D:/dats/ps2_checksums.dat"
   ```

   The option is documented in [docs/CONFIGINI.md](../../docs/CONFIGINI.md#checksumdat).

## Run

```powershell
.\Skyscraper.exe -p ps2 -s screenscraper -i D:/roms/ps2
```

No Qt installation and no changes to `PATH` are needed. Windows Terminal and PowerShell show the
colored output. When calling it from a script, redirect standard input, or it may wait for input.

## Contents

| File | What it is |
|---|---|
| `Skyscraper.exe` | Skyscraper, built from this repository |
| `Qt6Core.dll`, `Qt6Gui.dll`, `Qt6Network.dll`, `Qt6Sql.dll`, `Qt6Svg.dll`, `Qt6Xml.dll` | Qt 6.8.2 libraries, dynamically linked |
| `generic\`, `iconengines\`, `imageformats\`, `networkinformation\`, `platforms\`, `sqldrivers\`, `tls\` | Qt plugins (`tls\` provides HTTPS through Windows Schannel) |
| `libc++.dll`, `libunwind.dll` | LLVM C++ runtime |
| `licenses\` | License texts |

## Licenses

| Component | License | Text |
|---|---|---|
| Skyscraper | GPL-3.0-or-later | `licenses\Skyscraper-GPL-3.0.txt` |
| Qt 6.8.2 | LGPL-3.0 (open-source edition) | `licenses\Qt-LGPL-3.0.txt`, together with the GPL-3.0 text above |
| libc++, libunwind | Apache-2.0 WITH LLVM-exception | `licenses\LLVM-libc++-libunwind-Apache-2.0-with-LLVM-exception.txt` |
| MinGW-w64 runtime (static parts) | MinGW-w64 runtime license (permissive) | `licenses\MinGW-w64-runtime.txt` |

- **Skyscraper source:** this repository, at the commit that contains this build.
- **Qt source:** the unmodified Qt 6.8.2 sources are at
  <https://download.qt.io/archive/qt/6.8/6.8.2/single/>. Qt is used unmodified and dynamically
  linked, so you can replace the Qt DLLs with your own build of the same Qt version.
- Qt itself includes third-party code (for example zlib, libpng, libjpeg, HarfBuzz, FreeType, PCRE2,
  SQLite) under permissive licenses. The full list and texts are in the Qt sources and at
  <https://doc.qt.io/qt-6.8/licenses-used-in-qt.html>.
