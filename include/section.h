#ifndef __SECTION_H__
#define __SECTION_H__

#include "libwasm.h"

enum {
	WASM_CUSTOM_SECTION = 0,
	WASM_TYPE_SECTION,
	WASM_IMPORT_SECTION,
	WASM_FUNCTION_SECTION,
	WASM_TABLE_SECTION,
	WASM_MEMORY_SECTION,
	WASM_GLOBAL_SECTION,
	WASM_EXPORT_SECTION,
	WASM_START_SECTION,
	WASM_ELEMENT_SECTION,
	WASM_CODE_SECTION,
	WASM_DATA_SECTION,
	WASM_MAX_SECTION
};

#define CHECK_ERROR_CODE(ptr) (ptr)
enum {
	WASM_TRUNCATED_SECTION = WASM_MAX_ERROR + 1,
	WASM_MAX_SECTION_ERROR
};


struct ParseSectionParams {
	uint8_t*        data;
	uint32_t        offset;
	uint32_t        size;
	struct Section* section;
};

void* parseSection(void* section);
typedef void* (*parseFnList)(void*);
extern parseFnList parseSectionList[];
#endif
