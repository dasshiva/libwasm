#include <section.h>
#include <read_utils.h>
#include <stdlib.h>
#include <stdio.h>

#define  CHECK_IF_FILE_TRUNCATED(file) { \
	if (file.offset == UINT32_MAX) { \
		return ((void*) WASM_TRUNCATED_SECTION); \
    } \
}

void* internal_error(void* arg) {
	printf("Parse function at invalid index called");
	return NULL;
}

/*
 * HACK: All the fetchXXX() methods only care about the 
 * _data, offset and size fields of WasmModuleReader
 * So we initialise a dummy WasmModuleReader using the values
 * in arg and use the fetchXXX() methods without rewriting existing
 * functionality
 */

/*
 * All parseXXXSection() methods must begin their parsing sequence
 * by first creating the WasmModuleReader and then using it to
 * read more bytes through the fetchXXX() methods
 */

void* parseSection(void* arg) {
	struct ParseSectionParams* params = arg;
	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size;
	return NULL;
}

void* parseTypeSection(void* arg) {
	struct ParseSectionParams* params = arg;

	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size;

	uint32_t size = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);

	params->section->name = "Type";
	params->section->hash = hash("Type");
	return NULL;
}

parseFnList parseSectionList[] = {
	[WASM_CUSTOM_SECTION] = &parseSection,
	[WASM_TYPE_SECTION] = &parseTypeSection,
	[WASM_IMPORT_SECTION] = &parseSection,
	[WASM_FUNCTION_SECTION] = &parseSection,
	[WASM_TABLE_SECTION] = &parseSection,
	[WASM_MEMORY_SECTION] = &parseSection,
	[WASM_GLOBAL_SECTION] = &parseSection,
	[WASM_EXPORT_SECTION] = &parseSection,
	[WASM_START_SECTION] = &parseSection,
	[WASM_ELEMENT_SECTION] = &parseSection,
	[WASM_CODE_SECTION] = &parseSection,
	[WASM_DATA_SECTION] = &parseSection,
	[WASM_MAX_SECTION] = &internal_error
};
