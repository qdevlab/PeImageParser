## PeImageParser

A small PE (Portable Executable) format parser for binary analysis. It works with
a PE image already mapped into memory (not a raw file on disk). It takes a pointer
to the image base and walks the in-memory layout to extract headers, sections,
relocations, and imports.

The default test program parses its own process image (`GetModuleHandle(NULL)`) and
prints the results to the console.

### Usage

    PeImageParser.exe

The program loads its own image base and displays:
- DOS / NT headers (PE32 and PE32+)
- Section table (name, virtual address, raw size, etc.)
- Base relocations (type and RVA)
- Import table (module names, function names / ordinals, thunk addresses)

### How it works

1. `GetModuleHandle(NULL)` - get the base address of the current process image.
2. `ImageInfo::Parse()` - walk the PE structure:
   - `_ParseHeaders` - DOS signature, NT signature, Optional Header (32/64), section headers.
   - `_ParseRelocations` - enumerate `IMAGE_DIRECTORY_ENTRY_BASERELOC` blocks.
   - `_ParseImports` - enumerate `IMAGE_DIRECTORY_ENTRY_IMPORT` descriptors, ILT/IAT thunks.
3. `ImageInfo::Log()` - print everything to stdout.

`Rva2Offset` is available for converting between RVA and file offset when working with
on-disk images (currently the test uses a mapped image, so RVA == offset from base).

### Build

Open `PeImageParser.sln` in Visual Studio 2022 and build.
Configurations: Debug / Release, x86 / x64.

Build x86 to parse a PE32 image, x64 to parse a PE32+ image.

### Note

Research / educational tool.
