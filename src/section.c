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

#define CHECK_IF_VALID_VALTYPE(x) ((x) <= 0x7F && (x) >= 0x7C)

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
	params->section->types = malloc(sizeof(struct TypeSectionType) * size);
	printf("Here");
	for (int i = 0; i < size; i++) {
		uint8_t rd = fetchRawU8(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (rd != 0x60)
			return ((void*) WASM_INVALID_FUNCTYPE);

		uint32_t retlen = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (retlen != 1) 
			return ((void*) WASM_UNSUPPORTED_MULTIVALUE_FUNC);

		uint8_t rtype = fetchRawU8(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (CHECK_IF_VALID_VALTYPE(rtype))
                        return ((void*) WASM_INVALID_TYPEVAL);

		uint32_t plen = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (plen > 255)
			return ((void*) WASM_TOO_MANY_PARAMS);

		params->section->types[i].ret = rtype;
		params->section->types[i].paramsLen = plen;
		params->section->types[i].params = malloc(sizeof(uint8_t) * plen);
		for (int j = 0; j < plen; j++) {
			params->section->types[i].params[j] = fetchRawU8(&reader);
			CHECK_IF_FILE_TRUNCATED(reader);
			if (CHECK_IF_VALID_VALTYPE(params->section->types[i].params[j]))
                        	return ((void*) WASM_INVALID_TYPEVAL);
		}

	}

	if (reader.offset != reader.size) 
		return ((void*) WASM_TRAILING_BYTES);

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
