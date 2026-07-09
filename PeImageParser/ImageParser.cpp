// ################################################################################################### //

#include "ImageParser.h"

// ################################################################################################### //

		// Constructor
		ImageInfo::ImageInfo(
			const void* imageData
		)
			: _imageData(imageData)
		{
			//
		}

		// Destructor
		ImageInfo::~ImageInfo() {
			_Reset();
		}

		// Reset
		void ImageInfo::_Reset() {

			peBits = 0;
			peSectionAlignment = 0;
			peHeadersSize = 0;

			ZERO_STRUCT(peDosHeader);
			ZERO_STRUCT(peNtHeaders32);
			ZERO_STRUCT(peNtHeaders64);

			arraySectionHeaders.Clear();
			arrayRelocations.Clear();
			arrayImportModules.Clear();

		}

		// Parse
		bool ImageInfo::Parse()
		{

			// Clean up
			_Reset();

			// Check
			if (!_imageData) {
				FAIL_ARGS();
				return FALSE;
			}

			// Loop
			bool parseResult = FALSE;
			do {

				// Headers
				if (!_ParseHeaders()) {
					break;
				}

				// Relocs
				if (!_ParseRelocations()) {
					break;
				}

				// Imports
				if (!_ParseImports()) {
					break;
				}

				// Set result
				parseResult = TRUE;

			} while (FALSE);

			// Check parse result
			if (!parseResult) {
				_Reset();
			}

			// Return
			return parseResult;
		}

		bool ImageInfo::Rva2Offset(
			uint32		rvaOrOffset,
			bool		isRva,
			uint32*		out_rvaOrOffset,
			void**		out_rvaOrOffsetPointer
		){

			// Init
			if (out_rvaOrOffset) {
				*out_rvaOrOffset = 0;
			}
			if (out_rvaOrOffsetPointer) {
				*out_rvaOrOffsetPointer = NULL;
			}

			// Check sections
			if (arraySectionHeaders.Count() < 1) {
				FAIL("No information about sections");
				return FALSE;
			}

			// First section
			IMAGE_SECTION_HEADER* firstSection = arraySectionHeaders.At(0);
			if (!firstSection) {
				UNEXPECTED("First section is null");
				return FALSE;
			}

			// Is rva => need to find offset
			if (isRva) {

				// Check RVA less than first section RVA
				if (rvaOrOffset < firstSection->VirtualAddress) {

					// Check RVA less then first section PointerToRawData
					if (rvaOrOffset < firstSection->PointerToRawData) {
						if (out_rvaOrOffset) {
							*out_rvaOrOffset = rvaOrOffset;
						}
						if (out_rvaOrOffsetPointer) {
							*out_rvaOrOffsetPointer = (byte*)_imageData + rvaOrOffset;
						}
						return TRUE;
					}
					// Unexpected behaviour
					else {
						UNEXPECTED("Rva is less first VA and more then PointerToRawData");
						return FALSE;
					}
				}
				else {
					// belong to section
				}
			}
			else {
				// Check offset less than first section offset
				if (rvaOrOffset < firstSection->PointerToRawData) {
					if (out_rvaOrOffset) {
						*out_rvaOrOffset = rvaOrOffset;
					}
					if (out_rvaOrOffsetPointer) {
						*out_rvaOrOffsetPointer = (byte*)_imageData + rvaOrOffset;
					}
					return TRUE;
				}
				else {
					// belong to section
				}
			}

			// Section alignment
			if (!peSectionAlignment) {
				UNEXPECTED("Invalid section alignment");
				return FALSE;
			}

			// Enum all sections
			for (uint32 i = 0; i < arraySectionHeaders.Count(); i++) {

				// VirtualSize can be 0 => use SizeOfRawData

				// Get section header
				IMAGE_SECTION_HEADER* sectionHeader = arraySectionHeaders.At(i);
				if (!sectionHeader->VirtualAddress || !sectionHeader->PointerToRawData) {
					UNEXPECTED("Invalid section header values (VirtualAddress || PointerToRawData)");
					return FALSE;
				}

				// Is RVA
				if (isRva) {

					uint32 sectionVirtualSize = ALIGN_UP(sectionHeader->Misc.VirtualSize, peSectionAlignment);

					if (
						BEETWEEN_IE(
							rvaOrOffset,
							sectionHeader->VirtualAddress,
							sectionHeader->VirtualAddress + sectionVirtualSize
						)
						) {

						// Result
						uint32 resultValue = sectionHeader->PointerToRawData + (rvaOrOffset - sectionHeader->VirtualAddress);
						if (out_rvaOrOffset) {
							*out_rvaOrOffset = resultValue;
						}
						if (out_rvaOrOffsetPointer) {
							*out_rvaOrOffsetPointer = (byte*)_imageData + resultValue;
						}
						return TRUE;
					}
				}

				// Is offset
				else {

					if (
						BEETWEEN_IE(
							rvaOrOffset,
							sectionHeader->PointerToRawData,
							sectionHeader->PointerToRawData + sectionHeader->SizeOfRawData
						)
						) {

						// Result
						uint32 resultValue = sectionHeader->VirtualAddress + (rvaOrOffset - sectionHeader->PointerToRawData);
						if (out_rvaOrOffset) {
							*out_rvaOrOffset = resultValue;
						}
						if (out_rvaOrOffsetPointer) {
							*out_rvaOrOffsetPointer = (byte*)_imageData + resultValue;
						}
						return TRUE;
					}
				}
			}

			// Return
			return FALSE;

		}

		// Parse headers
		bool ImageInfo::_ParseHeaders() {

			// Trace
			TRACE();

			// Clear sections list
			arraySectionHeaders.Clear();

			// Check
			if (!_imageData) {
				FAIL("Invalid arguments");
				return false;
			}

			// Cursor
			byte* peDataCursor = (byte*)_imageData;

			// Dos header
			IMAGE_DOS_HEADER* pDosHeader = (IMAGE_DOS_HEADER*)peDataCursor;
			if (pDosHeader->e_magic != IMAGE_DOS_SIGNATURE) {
				FAIL("Invalid DOS signature");
				return FALSE;
			}
			COPY_STRUCT_BY_PTR(&peDosHeader, pDosHeader);

			// Nt header
			IMAGE_NT_HEADERS* pNtHeaders = (IMAGE_NT_HEADERS*)(peDataCursor + pDosHeader->e_lfanew);
			if (pNtHeaders->Signature != IMAGE_NT_SIGNATURE) {
				FAIL("Invalid NT signature");
				return FALSE;
			}

			// Headers and values
			if (pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
				peBits = 32;
				memcpy(&peNtHeaders32, pNtHeaders, sizeof(IMAGE_NT_HEADERS32));

				peSectionAlignment = peNtHeaders32.OptionalHeader.SectionAlignment;
				peHeadersSize = peNtHeaders32.OptionalHeader.SizeOfHeaders;

			}
			else if (pNtHeaders->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
				peBits = 64;
				memcpy(&peNtHeaders64, pNtHeaders, sizeof(IMAGE_NT_HEADERS64));

				peSectionAlignment = peNtHeaders64.OptionalHeader.SectionAlignment;
				peHeadersSize = peNtHeaders64.OptionalHeader.SizeOfHeaders;

			}
			else {
				FAIL("Invalid Optional NT header magic");
				return FALSE;
			}

			// Sections
			IMAGE_SECTION_HEADER* peSectionHeader =
				(IMAGE_SECTION_HEADER*)(
					(byte*)pNtHeaders +
					sizeof(decltype(IMAGE_NT_HEADERS::Signature)) +
					sizeof(decltype(IMAGE_NT_HEADERS::FileHeader)) +
					pNtHeaders->FileHeader.SizeOfOptionalHeader
				);

			for (ULONG i = 0; i < pNtHeaders->FileHeader.NumberOfSections; i++) {

				// Log
				INFO("\tSection %d",i);
				INFO("\t\tName             : %.8hs",  peSectionHeader->Name);
				INFO("\t\tVirtualSize      : 0x%08X", peSectionHeader->Misc.VirtualSize);
				INFO("\t\tVirtualAddress   : 0x%08X", peSectionHeader->VirtualAddress);
				INFO("\t\tSizeOfRawData    : 0x%08X", peSectionHeader->SizeOfRawData);
				INFO("\t\tPointerToRawData : 0x%08X", peSectionHeader->PointerToRawData);

				// Append
				arraySectionHeaders.PushBack(*peSectionHeader);

				// Section info
				peSections.Append(SectionInfo(_imageData, peSectionHeader));

				// Next
				peSectionHeader++;
			}

			// Return
			return TRUE;

		}

		// Parse relocs
		bool ImageInfo::_ParseRelocations() {

			// Trace
			TRACE();

			// Clear relocations list
			arrayRelocations.Clear();

			// Check
			if (!_imageData) {
				FAIL("Invalid arguments");
				return false;
			}

			// Get reloc directory pointer
			IMAGE_DATA_DIRECTORY* relocationDirectory =
				peBits == 32 ?
				&peNtHeaders32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC] :
				&peNtHeaders64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];

			// Check directory exists
			bool directoryExists = relocationDirectory->Size && relocationDirectory->VirtualAddress;
			if (!directoryExists) {
				INFO("Relocation directory not exists");
				return TRUE;
			}

			// Get relocation directory entry
			byte* relocationEntryPointer = NULL;
			uint32 relocationDirectorySize = relocationDirectory->Size;

			relocationEntryPointer = (byte*)_imageData + relocationDirectory->VirtualAddress;

			// Enum pe relocations
			bool resultFlag = TRUE;
			uint32 relocationDataProcessed = 0;

			while (TRUE) {

				// Check processed size
				if (relocationDataProcessed >= relocationDirectorySize) {
					break;
				}

				// Line
				IMAGE_BASE_RELOCATION* relocationLine = (IMAGE_BASE_RELOCATION*)relocationEntryPointer;
				uint32 relocationLineVirtualBase = relocationLine->VirtualAddress;
				uint32 relocationLineSize = relocationLine->SizeOfBlock;
				if (relocationLineSize < sizeof(IMAGE_BASE_RELOCATION)) {
					UNEXPECTED("Relocation line unexpected size : %d", relocationLineSize);
					resultFlag = FALSE;
					break;
				}
				uint32 relocationLineItemsCount = (relocationLineSize - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
				WORD* relocationLineItemsPointer = (WORD*)((byte*)relocationLine + sizeof(IMAGE_BASE_RELOCATION));

				// Enum items
				for (uint32 i = 0; i < relocationLineItemsCount; i++) {

					// IMAGE_REL_BASED_ABSOLUTE              0
					// IMAGE_REL_BASED_HIGH                  1
					// IMAGE_REL_BASED_LOW                   2
					// IMAGE_REL_BASED_HIGHLOW               3
					// IMAGE_REL_BASED_HIGHADJ               4
					// IMAGE_REL_BASED_MACHINE_SPECIFIC_5    5
					// IMAGE_REL_BASED_RESERVED              6
					// IMAGE_REL_BASED_MACHINE_SPECIFIC_7    7
					// IMAGE_REL_BASED_MACHINE_SPECIFIC_8    8
					// IMAGE_REL_BASED_MACHINE_SPECIFIC_9    9
					// IMAGE_REL_BASED_DIR64                 10

					// Item
					WORD relocationItemValue = relocationLineItemsPointer[i];
					WORD relocationItemType = relocationItemValue >> 12;
					WORD relocationItemOffset = relocationItemValue & 0x0FFF;
					uint32 relocationItemVirtualOffset = relocationLineVirtualBase + relocationItemOffset;

					// Packed item
					RelocationItemInfo relocationInfo = { 0 };
					relocationInfo.type = relocationItemType;
					relocationInfo.rva = relocationItemVirtualOffset;

					// Append
					arrayRelocations.PushBack(relocationInfo);
					peRelocations.Append(relocationInfo);

				}

				// Next
				relocationDataProcessed += relocationLineSize;
				relocationEntryPointer += relocationLineSize;
			}

			// Check flag && Clear import modules if need
			if (!resultFlag) {
				FAIL("Error occured while parsing relocations");
				arrayRelocations.Clear();
			}

			// Return
			return resultFlag;
		}

		// Parse imports
		bool ImageInfo::_ParseImports() {

			// Trace
			TRACE();

			// Clear relocations list
			arrayImportModules.Clear();

			// Check
			if (!_imageData) {
				FAIL("Invalid arguments");
				return false;
			}

			// Get import directory pointer
			IMAGE_DATA_DIRECTORY* importDirectory =
				peBits == 32 ?
				&peNtHeaders32.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] :
				&peNtHeaders64.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];

			// Check directory exists
			bool directoryExists = importDirectory->Size && importDirectory->VirtualAddress;
			if (!directoryExists) {
				INFO("Import directory not exists");
				return TRUE;
			}

			// Get import directory entry
			byte* importEntryPointer = (byte*)_imageData + importDirectory->VirtualAddress;

			// Enum
			bool resultFlag = FALSE;
			IMAGE_IMPORT_DESCRIPTOR* importDescriptorIndex = (IMAGE_IMPORT_DESCRIPTOR*)importEntryPointer;
			while (TRUE) {

				// Check last descriptor
				if (
					!importDescriptorIndex->Characteristics ||
					!importDescriptorIndex->FirstThunk ||
					!importDescriptorIndex->Name
					) {
					resultFlag = TRUE;
					break;
				}

				// Module name
				char* importModuleName = (char*)_imageData + importDescriptorIndex->Name;

				// Module name check
				if (!strlen(importModuleName)) {
					UNEXPECTED("Module name is empty string");
					break;
				}

				// Check thunk name & address
				void* thunkName = NULL;
				void* thunkAddress = NULL;
				if (!importDescriptorIndex->FirstThunk && !importDescriptorIndex->OriginalFirstThunk) {
					UNEXPECTED("!FirstThunk && !OriginalFirstThunk");
					break;
				}

				// Get OriginalFirstThunk RVA if exists
				if (importDescriptorIndex->OriginalFirstThunk) {

					thunkName = (byte*)_imageData + importDescriptorIndex->OriginalFirstThunk;
				}

				// Get FirstThunk RVA if exists
				if (importDescriptorIndex->FirstThunk) {
					thunkAddress = (byte*)_imageData + importDescriptorIndex->FirstThunk;
				}

				// Delphi PE building
				{
					if (!thunkName && !thunkAddress) {
						UNEXPECTED("!thunkName && !thunkAddress");
						break;
					}

					if (!thunkName) {
						thunkName = thunkAddress;
					}

					if (!thunkAddress) {
						thunkAddress = thunkName;
					}
				}

				// Module
				ImportModuleInfo importModule(importModuleName);

				// Enum module result
				bool descriptorResult = FALSE;
				uint32 descriptorThunksCount = 0;

				// Enum
				{

					if (peBits == 32) {

						IMAGE_THUNK_DATA32* thunkItemName_ILT_32 = (IMAGE_THUNK_DATA32*)thunkName;
						IMAGE_THUNK_DATA32* thunkItemAddress_IAT_32 = (IMAGE_THUNK_DATA32*)thunkAddress;

						while (true) {

							// Check last thunk
							if (
								!thunkItemName_ILT_32->u1.AddressOfData &&
								!thunkItemAddress_IAT_32->u1.AddressOfData
							) {
								descriptorResult = true;
								break;
							}

							void* location = &thunkItemAddress_IAT_32->u1.Function;
							void* address = (void*)thunkItemAddress_IAT_32->u1.Function;

							bool ordinalFlag = FLAG(thunkItemName_ILT_32->u1.Ordinal, IMAGE_ORDINAL_FLAG32);
							if (ordinalFlag) {

								// Ordinal
								WORD ordinal = (WORD)(IMAGE_ORDINAL32(thunkItemName_ILT_32->u1.Ordinal));

								// Debug
								LOG("[%04d] " HEX " : " HEX " %hs.%d",descriptorThunksCount, location, address, importModuleName, ordinal);

								// Append to module
								importModule.Append(
									ImportItemInfo(
										FALSE,
										(char*)ordinal,
										0,
										0,
										location,
										address
									)
								);

							}
							else {

								// Name
								IMAGE_IMPORT_BY_NAME* name = (PIMAGE_IMPORT_BY_NAME)((byte*)_imageData + thunkItemName_ILT_32->u1.AddressOfData);

								// Debug
								LOG("[%04d] " HEX " : " HEX " %hs.%hs",descriptorThunksCount, location, address, importModuleName, name->Name);

								// Append to module
								importModule.Append(
									ImportItemInfo(
										TRUE,
										name->Name,
										0,
										0,
										location,
										address
									)
								);

							}

							// Next
							thunkItemName_ILT_32++;
							thunkItemAddress_IAT_32++;
							descriptorThunksCount++;

						}

					}
					else {

						IMAGE_THUNK_DATA64* thunkItemName_ILT_64 = (IMAGE_THUNK_DATA64*)thunkName;
						IMAGE_THUNK_DATA64* thunkItemAddress_IAT_64 = (IMAGE_THUNK_DATA64*)thunkAddress;

						while (true) {

							// Check last thunk
							if (
								!thunkItemName_ILT_64->u1.AddressOfData &&
								!thunkItemAddress_IAT_64->u1.AddressOfData
								) {
								descriptorResult = true;
								break;
							}

							void* location = &thunkItemAddress_IAT_64->u1.Function;
							void* address = (void*)thunkItemAddress_IAT_64->u1.Function;

							bool ordinalFlag = FLAG(thunkItemName_ILT_64->u1.Ordinal, IMAGE_ORDINAL_FLAG64);
							if (ordinalFlag) {

								// Ordinal
								WORD ordinal = (WORD)(IMAGE_ORDINAL64(thunkItemName_ILT_64->u1.Ordinal));

								// Debug
								LOG("[%04d] " HEX " : " HEX " %hs.%d",descriptorThunksCount, location, address, importModuleName, ordinal);

								// Append to module
								importModule.Append(
									ImportItemInfo(
										FALSE,
										(char*)ordinal,
										0,
										0,
										location,
										address
									)
								);

							}
							else {

								// Name
								IMAGE_IMPORT_BY_NAME* name = (PIMAGE_IMPORT_BY_NAME)((byte*)_imageData + thunkItemName_ILT_64->u1.AddressOfData);

								// Debug
								LOG("[%04d] " HEX " : " HEX " %hs.%hs",descriptorThunksCount, location, address, importModuleName, name->Name);

								// Append to module
								importModule.Append(
									ImportItemInfo(
										TRUE,
										name->Name,
										0,
										0,
										location,
										address
									)
								);

							}

							// Next
							thunkItemName_ILT_64++;
							thunkItemAddress_IAT_64++;
							descriptorThunksCount++;

						}

					}

				}

				// Check descriptor flag
				if (!descriptorResult) {
					resultFlag = FALSE;
					break;
				}

				// Append import module
				arrayImportModules.PushBack(importModule);
				peImports.Append(importModule);

				// Next import descriptor
				importDescriptorIndex++;


			}// while (enum descriptors)

			 // Check flag && Clear import modules if need
			if (!resultFlag) {
				FAIL("Error occured while parsing imports");
				arrayImportModules.Clear();
			}

			// Return
			return resultFlag;

		}

		// Log sections
		void ImageInfo::_LogSections() {
			for (UINT i = 0; i < arraySectionHeaders.Count();i++) {

				auto peSectionHeader = arraySectionHeaders.At(i);
				if (!peSectionHeader) {
					UNEXPECTED("Invalid pointer, exit");
					return;
				}

				LOG("\tSection %d",i);
				LOG("\t\tName             : %.8hs",  peSectionHeader->Name);
				LOG("\t\tVirtualSize      : 0x%08X", peSectionHeader->Misc.VirtualSize);
				LOG("\t\tVirtualAddress   : 0x%08X", peSectionHeader->VirtualAddress);
				LOG("\t\tSizeOfRawData    : 0x%08X", peSectionHeader->SizeOfRawData);
				LOG("\t\tPointerToRawData : 0x%08X", peSectionHeader->PointerToRawData);
			}
		}

		// Log imports
		void ImageInfo::_LogImports() {

			// Check
			if (arrayImportModules.Count() < 1) {
				INFO("Imports is empty");
			}

			// Enum modules
			for (size_t i = 0; i < arrayImportModules.Count(); i++) {

				// Get module info
				auto moduleInfo = arrayImportModules.GetElementPtrByIndex(i);
				if (!moduleInfo) {
					UNEXPECTED("Module info is null");
					return;
				}

				// Log
				LOG("Module : %hs",moduleInfo->name);

				// Enum module items
				for (size_t j = 0; j < moduleInfo->arrayItems.Count(); j++) {

					auto itemInfo = moduleInfo->arrayItems.At(j);
					if (!itemInfo) {
						UNEXPECTED("Item info is null");
						return;
					}

					if (itemInfo->byName) {
						LOG(
							"\tLoc : " HEX ": Addr : " HEX "; RVA : %08X; Offset : %08X; Name: %hs",
							itemInfo->location,
							itemInfo->address,
							itemInfo->rva,
							itemInfo->offset,
							itemInfo->nameOrOrdinal
						);
					}
					else {
						LOG(
							"\tLoc : " HEX "; Addr : " HEX "; RVA : %08X; Offset : %08X; Ord : %d",
							itemInfo->location,
							itemInfo->address,
							itemInfo->rva,
							itemInfo->offset,
							itemInfo->nameOrOrdinal
						);

					}

				}

			}

		}

		// Log headers
		void ImageInfo::_LogHeaders() {


		}

		// Log
		void ImageInfo::Log() {

			CENTERED("%ls",L"PE");
			LOG("PE bits : %d", peBits);

			// Headers
			CENTERED("%ls",L"HEADERS");
			_LogHeaders();

			// Sections
			CENTERED("%ls",L"SECTIONS");
			_LogSections();

			// Relocations
			CENTERED("%ls",L"RELOCATIONS");
			LOG("Count : %zu",arrayRelocations.Count());

			// Imports
			CENTERED("%ls",L"IMPORTS");
			_LogImports();

			// Return
			return;
		}


// ################################################################################################### //
