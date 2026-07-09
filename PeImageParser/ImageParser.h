// ################################################################################################### //

#pragma once

// ################################################################################################### //

#include "Framework.h"

// ################################################################################################### //

struct RelocationItemInfo {
	WORD type;
	uint32 rva;
};

struct ImportItemInfo {
	bool byName;
	const char* nameOrOrdinal;
	uint32 rva;
	uint32 offset;
	void* location;
	void* address;

	ImportItemInfo(
		bool _byName,
		const char* _name,
		uint32 _rva,
		uint32 _offset,
		void* _location,
		void* _address
	) : byName(_byName), nameOrOrdinal(_name), rva(_rva), offset(_offset),
		location(_location), address(_address) {}
};

struct ImportModuleInfo {
	const char* name;
	DynamicArrayT<ImportItemInfo> arrayItems;

	ImportModuleInfo() : name(NULL) {}
	ImportModuleInfo(const char* _name) : name(_name) {}
	void Append(const ImportItemInfo& item) { arrayItems.PushBack(item); }
};

struct SectionInfo {
	SectionInfo(const void*, IMAGE_SECTION_HEADER*) {}
};

struct ImportsInfo {
	void Append(const ImportModuleInfo&) {}
};

struct RelocationsInfo {
	void Append(const RelocationItemInfo&) {}
};

struct SectionsInfo {
	void Append(const SectionInfo&) {}
};

// ################################################################################################### //

class ImageInfo {

private:

	const void* _imageData = nullptr;

private:

	void _Reset();
	bool _ParseHeaders();
	bool _ParseRelocations();
	bool _ParseImports();

public:

	uint32						peBits = 0;
	uint32						peSectionAlignment = 0;
	uint32						peHeadersSize = 0;

	IMAGE_DOS_HEADER			peDosHeader = { 0 };
	IMAGE_NT_HEADERS32			peNtHeaders32 = { 0 };
	IMAGE_NT_HEADERS64			peNtHeaders64 = { 0 };

	ImportsInfo					peImports;
	RelocationsInfo				peRelocations;
	SectionsInfo				peSections;

	DynamicArrayT<ImportModuleInfo>		arrayImportModules;
	DynamicArrayT<RelocationItemInfo>	arrayRelocations;
	DynamicArrayT<IMAGE_SECTION_HEADER>	arraySectionHeaders;

public:

	ImageInfo(const void* imageData);
	~ImageInfo();
	ImageInfo(ImageInfo&) = delete;

	bool Parse();

	bool Rva2Offset(
		uint32		rvaOrOffset,
		bool		isRva,
		uint32*		out_rvaOrOffset,
		void**		out_rvaOrOffsetPointer
	);

	void Log();

private:

	void _LogHeaders();
	void _LogSections();
	void _LogImports();

};

// ################################################################################################### //
