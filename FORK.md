# Fork rsporteman/skyscraper

Personal fork of [Gemba/skyscraper](https://github.com/Gemba/skyscraper). It adds one feature, the
`checksumDat=` option: Skyscraper takes the CRC32, MD5 and SHA1 that ScreenScraper needs from a
Logiqx XML DAT instead of reading every game file. A ready-to-run Windows build is included in
[`dist/win64/`](dist/win64/README.md).

**Fork version:** `3.22.0-dev-checksumdat`, based on upstream `master` at `c253bfe` (3.22.0-dev).

| Branch | Contents |
|---|---|
| `master` | Upstream `master` + the checksumDat change + this file, the evidence images in `docs/fork/` and the Windows build in `dist/win64/` |
| `feat_checksum_dat` | Only the change, as a single commit with no fork files. This is the branch intended for a pull request upstream (not opened yet) |

## The problem

To identify a game, ScreenScraper takes the CRC32, MD5 and SHA1 of the game file. Upstream
Skyscraper always reads the whole file to compute them. With large images (multi-GB CHD or ISO) that
is slow, and on a slow disk it is very slow:

| Setup (same PC, ~1.5 GB PS2 CHD) | Time spent on checksums only |
|---|---|
| Skyscraper in WSL reading the Windows drive (`/mnt/g`) | ~2.5 min per game (1 KB reads, ~8 MB/s) |
| Native Windows Skyscraper, local disk | ~10 s per game |
| With `checksumDat` | nothing is read |

For a 574-game PS2 collection the checksums alone added up to about 20 hours under WSL.

## The change

New `config.ini` option, valid in `[main]` and `[<PLATFORM>]`:

```ini
[ps2]
checksumDat="G:/ROMs/_organizacao/skyscraper/ps2_checksums.dat"
```

The value is the absolute path of a Logiqx XML DAT, the format used by No-Intro, Redump, ClrMamePro
and RomVault:

```xml
<datafile>
  <game name="Madden NFL 09 (USA)">
    <rom name="Madden NFL 09 (USA).chd" size="1424568312" crc="C767C447"
         md5="F43B1105125344F2968847E3FFCA4959" sha1="4A7EA73C7A26E8DFA2BDCAD117E75616BAE26664"/>
  </game>
</datafile>
```

Rules:

- A file matches a `<rom/>` when the **file name** (directory stripped) and the **size in bytes** are
  equal. The DAT checksums are then used and the file is not read.
- Entries without a valid `size`, `crc`, `md5` or `sha1` are skipped, and their count is reported.
- Same name and size with different checksums is ambiguous: Skyscraper computes them as before.
- No DAT entry, or a different size (the file changed): computed as before.
- Only the `screenscraper` module uses it. Other modules ignore it with a notice.

Changed files:

| File | Change |
|---|---|
| `src/checksumdat.h/.cpp` | New `ChecksumDat` class: parses the DAT (`QXmlStreamReader`) and looks up by name and size |
| `src/screenscraper.cpp/.h` | `getSearchNames()` uses the DAT; the original code moved unchanged to `computeChecksums()` |
| `src/settings.cpp/.h` | `checksumDat` option, validated like `scummIni` (absolute path, file exists) |
| `src/skyscraper.cpp/.h` | `loadChecksumDat()` loads the DAT before scraping |
| `docs/CONFIGINI.md`, `config.ini.example` | Option documentation |
| `test/checksumdat/` (new), `test/getsearchnames/`, `test/settings/` | Tests |
| `VERSION.ini` (`master` only) | Fork version string |

## Tests

| Suite | New cases |
|---|---|
| `test/checksumdat` | hit by name and size, uppercase hex; different size, unknown name and different case do not match; name with a directory (`/` and `\`); same name with different sizes; identical duplicates (including case-only) and ambiguous ones; 6 kinds of incomplete entry; invalid XML; reload clears the previous data; load from file and missing file |
| `test/getsearchnames` | the ScreenScraper query uses the DAT checksums (deliberately fake values, proving the file was not read); 0-byte file; size mismatch and empty DAT fall back to computing |
| `test/settings` | `checksumDat` in `[main]`, overridden by `[<PLATFORM>]`; missing file and relative path raise an error |

![Unit tests](docs/fork/checksumdat_unit_tests.png)

On Windows, `test/pathtools`, `test/retroarch` and `test/settings` have 6 failures that **already
exist on upstream `master`**: those tests expect `/` and get `\`. Verified by running upstream
`master` without the change. Upstream CI runs the tests on Linux.

## Validation with real games

Same game scraped twice with `--refresh` (full ScreenScraper query both times):

![A/B with and without checksumDat](docs/fork/checksumdat_ab_windows.png)

- The query sent to ScreenScraper is **identical** in both runs (same CRC, MD5, SHA1, name and
  size), and the game was matched at 100% both times.
- Without the DAT, `Skyscraper.exe` read 1,360 MB (the whole CHD); with the DAT, 1.2 MB.
- The time difference is small here because the file was already in the Windows disk cache; the
  gain shows on a cold disk, a network share or WSL (table above).

The packaged build in `dist/win64/` was also checked with `PATH` reduced to `C:\Windows`: it starts,
reports `3.22.0-dev-checksumdat`, loads the DAT, uses the DAT checksums for a 1.7 GB CHD and matches
the game at 100% over HTTPS.

## Building on Windows

Same as upstream `win32/README.md`, but with [aqtinstall](https://github.com/miurahr/aqtinstall)
instead of the Qt online installer (no Qt account needed):

```powershell
python -m pip install --user aqtinstall
python -m aqt install-qt windows desktop 6.8.2 win64_llvm_mingw --outputdir C:\Qt
python -m aqt install-tool windows desktop tools_llvm_mingw1706 --outputdir C:\Qt
$env:PATH = "C:\Qt\6.8.2\llvm-mingw_64\bin;C:\Qt\Tools\llvm-mingw1706_64\bin;$env:PATH"
mkdir build-dist; cd build-dist
qmake ..\skyscraper.pro
mingw32-make -j8 release
cd release
windeployqt6 --no-translations Skyscraper.exe
```

Notes:

- The post-build step that copies the Qt licenses from `C:\Qt\Licenses` fails with aqtinstall,
  because that folder only comes with the official installer. `Skyscraper.exe` is built anyway.
- Do not pass `--release` to `windeployqt6`: with this Qt it stops with "Unable to find the platform
  plugin".
- `windeployqt6` does not copy the LLVM runtime. Copy `libc++.dll` and `libunwind.dll` from
  `C:\Qt\6.8.2\llvm-mingw_64\bin`.
- `opengl32sw.dll`, `D3Dcompiler_47.dll` and the ODBC, PostgreSQL and Mimer SQL drivers are not
  needed by a command-line tool and are left out of `dist/win64/`.
- Tests build with `qmake test_<name>.pro -after "VERSION=3.22.0.999"`, because the Windows resource
  compiler rejects a version like `3.22.0-dev`.
- When running `Skyscraper.exe` from a script, redirect standard input; otherwise it may wait.

## Creating the DAT for your files

If your files are unmodified verified dumps, the group's DAT (Redump, No-Intro) already works. If
you converted them to CHD or compressed them, you need a DAT with the checksums of the **current**
files: any ROM manager can create one, or record the checksums when converting. This fork's author
used a PowerShell script that reads each file once, in 4 MB blocks, and computes the three checksums
in a single pass.

## License

Skyscraper is free software under the **GNU General Public License v3.0**, and so is this fork
([LICENSE](LICENSE)). The new files (`src/checksumdat.*`, `test/checksumdat/*`) carry the same
GPL-3.0-or-later header as the rest of the project. Copyright of the original code stays with Lars
Muldjord, Gemba and the other contributors.

The Windows build in `dist/win64/` also contains third-party libraries, under their own licenses.
Their notices are in [`dist/win64/licenses/`](dist/win64/licenses) and summarized in
[`dist/win64/README.md`](dist/win64/README.md#licenses).
