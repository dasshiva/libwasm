#include "libwasm.h"
#include <section.h>
#include <read_utils.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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

#define CHECK_IF_VALID_VALTYPE(x) (((x) >= 0x7C) && ((x) <= 0x7F))

void* parseSection(void* arg) {
	struct ParseSectionParams* params = arg;
	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;
	return NULL;
}

void* parseTypeSection(void* arg) {
	struct ParseSectionParams* params = arg;

	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);

	params->section->name = "Type";
	params->section->hash = hash("Type");
	if (!size) {
		params->section->flags = 0;
		params->section->custom = NULL;
		return NULL;
	}

	params->section->types = malloc(sizeof(struct TypeSectionType) * size);
	params->section->flags = size;

	for (int i = 0; i < size; i++) {
		uint8_t rd = fetchRawU8(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (rd != 0x60)
			return ((void*) WASM_INVALID_FUNCTYPE);

		uint32_t plen = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (plen) {	
			if (plen > 255)
				return ((void*) WASM_TOO_MANY_PARAMS);
			
			params->section->types[i].paramsLen = plen;
			params->section->types[i].params = malloc(sizeof(uint8_t) * plen);
			for (int j = 0; j < plen; j++) {
				params->section->types[i].params[j] = fetchRawU8(&reader);
				CHECK_IF_FILE_TRUNCATED(reader);
				if (!CHECK_IF_VALID_VALTYPE(params->section->types[i].params[j])) {
					return ((void*) WASM_INVALID_TYPEVAL);
				}
			}
		}
		else {
			params->section->types[i].paramsLen = 0;
			params->section->types[i].params = NULL;
		}

		uint32_t retlen = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (retlen > 1) 
			return ((void*) WASM_UNSUPPORTED_MULTIVALUE_FUNC);

		if (retlen) {
			uint8_t rtype = fetchRawU8(&reader);
			CHECK_IF_FILE_TRUNCATED(reader);
			if (!(CHECK_IF_VALID_VALTYPE(rtype))) 
                return ((void*) WASM_INVALID_TYPEVAL);
			
			params->section->types[i].ret = rtype;
		}
		else {
			params->section->types[i].ret = 0;
		}

	}

	/*for (int i = 0; i < size; i++) {
		struct TypeSectionType ty = params->section->types[i];
		if (ty.ret) {
			printf("Returns = 0x%x\n", ty.ret);
		}

		if (ty.params) {
			printf("Paramters = ");
			for (int j = 0; j < ty.paramsLen; j++) {
				printf("0x%x ", ty.params[j]);
			}
			printf("\n");
		}
	}*/

	if (reader.offset + 1 != reader.size) 
		return ((void*) WASM_TRAILING_BYTES);

	return NULL;
}

void* parseImportSection(void* arg) {
	struct ParseSectionParams* params = arg;
	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);

	params->section->name = "Import";
	params->section->hash = hash("Import");
	if (!size) {
		params->section->flags = 0;
		params->section->custom = NULL;
		return NULL;
	}

	params->section->imports = malloc(sizeof(struct ImportSectionImport) * size);
	params->section->flags = size;

	for (int i = 0; i < size; i++) {
		uint32_t modlen = fetchU32(&reader) + 1; // space for null
                if (!modlen)
                        return ((void*) WASM_EMPTY_NAME);
                CHECK_IF_FILE_TRUNCATED(reader);

		if (reader.offset + modlen - 1>= reader.size) 
			return ((void*) WASM_TRUNCATED_SECTION);

                params->section->imports[i].module = malloc(sizeof(const char*) * modlen);
                memcpy(params->section->imports[i].module, (uint8_t*)reader._data + reader.offset, modlen);
                params->section->imports[i].module[modlen - 1] = '\0';
                params->section->imports[i].hashModule = hash(params->section->imports[i].module);
                skip(&reader, modlen - 1);
		CHECK_IF_FILE_TRUNCATED(reader);

		uint32_t namelen = fetchU32(&reader) + 1; // space for null
                if (!namelen)                                                                       return ((void*) WASM_EMPTY_NAME);
                CHECK_IF_FILE_TRUNCATED(reader);

		if (reader.offset + namelen - 1 >= reader.size)
			return ((void*) WASM_TRUNCATED_SECTION);

		params->section->imports[i].name = malloc(sizeof(const char*) * namelen);
		memcpy(params->section->imports[i].name, (uint8_t*)reader._data + reader.offset, namelen);
		params->section->imports[i].name[namelen - 1] = '\0';
		params->section->imports[i].hashName = hash(params->section->imports[i].name);
                skip(&reader, namelen - 1);
		CHECK_IF_FILE_TRUNCATED(reader);

		params->section->imports[i].type = fetchRawU8(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (params->section->imports[i].type >= WASM_MAXTYPE)
			return ((void*) WASM_INVALID_IMPORT_TYPE);

		params->section->imports[i].index = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
	}

	/* for (int i = 0; i < size; i++) {
		printf("%s.", params->section->imports[i].module);
		printf("%s ", params->section->imports[i].name);
		printf("Type = %d Index =%d\n", params->section->imports[i].type, params->section->imports[i].index);
	} */

	if (reader.offset + 1 != reader.size)                         
		return ((void*) WASM_TRAILING_BYTES);

	return NULL;
}

void* parseFunctionSection(void* arg) {
	struct ParseSectionParams* params = arg;
	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);

	if (!size)
		return NULL;
	
	params->section->name = "Function";
	params->section->hash = hash("Function");
	params->section->flags = size;
	params->section->functions = malloc(sizeof(uint32_t) * size);

	for (int i = 0; i < size; i++) {
		params->section->functions[i] = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
	}

	/*
	for (int i = 0; i < size; i++) {
		printf("%d\n", params->section->functions[i]);
	} */

	if (reader.offset + 1 != reader.size)                         
		return ((void*) WASM_TRAILING_BYTES);

	return NULL;
}

void* parseTableSection(void* arg) {
	struct ParseSectionParams* params = arg;
	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);

	if (size != 1)
		return ((void*) WASM_TOO_MANY_TABLES);

	if (fetchU32(&reader) != 0x70)
		return ((void*) WASM_INVALID_TABLE_ELEMENT_TYPE);

	CHECK_IF_FILE_TRUNCATED(reader);

	params->section->flags = 1;
	params->section->name = "Table";
	params->section->hash = hash("Table");
	params->section->table = malloc(sizeof(struct Table));

	uint8_t limtype = fetchRawU8(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);

	if (limtype) {
		params->section->table->min = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		params->section->table->max = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
	}
	else {
		params->section->table->min = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		params->section->table->max = UINT32_MAX;
	}

	if (reader.offset + 1 != reader.size)                         
		return ((void*) WASM_TRAILING_BYTES);

	return NULL;
}

void* parseMemorySection(void* arg) {
	struct ParseSectionParams* params = arg;
	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);

	if (size != 1)
		return ((void*) WASM_TOO_MANY_MEMORIES);

	CHECK_IF_FILE_TRUNCATED(reader);

	params->section->flags = 1;
	params->section->name = "Memory";
	params->section->hash = hash("Memory");
	params->section->memory = malloc(sizeof(struct Table));

	uint8_t limtype = fetchRawU8(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);

	if (limtype) {
		params->section->memory->min = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		params->section->memory->max = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
	}
	else {
		params->section->memory->min = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		params->section->memory->max = UINT32_MAX;
	}

	if (reader.offset + 1 != reader.size)                         
		return ((void*) WASM_TRAILING_BYTES);

	return NULL;
}

void* parseExportSection(void* arg) {
	struct ParseSectionParams* params = arg;
	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);

	params->section->name = "Export";
	params->section->hash = hash("Export");
	if (!size) {
		params->section->flags = 0;
		params->section->custom = NULL;
		return NULL;
	}

	params->section->exports = malloc(sizeof(struct ExportSectionExport) * size);
	params->section->flags = size;

	for (int i = 0; i < size; i++) {
		uint32_t namelen = fetchU32(&reader) + 1; // space for null
            if (!namelen)                                                                       
				return ((void*) WASM_EMPTY_NAME);
        CHECK_IF_FILE_TRUNCATED(reader);

		if (reader.offset + namelen - 1 >= reader.size)
			return ((void*) WASM_TRUNCATED_SECTION);

		params->section->exports[i].name = malloc(sizeof(const char*) * namelen);
		memcpy(params->section->exports[i].name, (uint8_t*)reader._data + reader.offset, namelen);
		params->section->exports[i].name[namelen - 1] = '\0';
		params->section->exports[i].hashName = hash(params->section->exports[i].name);
        skip(&reader, namelen - 1);
		CHECK_IF_FILE_TRUNCATED(reader);

		params->section->exports[i].type = fetchRawU8(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (params->section->exports[i].type >= WASM_MAXTYPE)
			return ((void*) WASM_INVALID_IMPORT_TYPE);

		params->section->exports[i].index = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
	}

	/*
		for (int i = 0; i < size; i++) {
			printf("%s ", params->section->exports[i].name);
			printf("Type = %d Index =%d\n", params->section->exports[i].type, params->section->exports[i].index);
		}
	*/
	if (reader.offset + 1 != reader.size)                         
		return ((void*) WASM_TRAILING_BYTES);

	return NULL;
}

void* parseStartSection(void* arg) {
	struct ParseSectionParams* params = arg;
	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t fn = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);

	params->section->start = fn;
	params->section->flags = 0;
	params->section->name = "Start";
	params->section->hash = hash("Start");

	printf("Start function  = %ld\n", params->section->start);
	return NULL;
}

parseFnList parseSectionList[] = {
	[WASM_CUSTOM_SECTION] = &parseSection,
	[WASM_TYPE_SECTION] = &parseTypeSection,
	[WASM_IMPORT_SECTION] = &parseImportSection,
	[WASM_FUNCTION_SECTION] = &parseFunctionSection,
	[WASM_TABLE_SECTION] = &parseTableSection,
	[WASM_MEMORY_SECTION] = &parseMemorySection,
	[WASM_GLOBAL_SECTION] = &parseSection,
	[WASM_EXPORT_SECTION] = &parseExportSection,
	[WASM_START_SECTION] = &parseStartSection,
	[WASM_ELEMENT_SECTION] = &parseSection,
	[WASM_CODE_SECTION] = &parseSection,
	[WASM_DATA_SECTION] = &parseSection,
	[WASM_MAX_SECTION] = &internal_error
};
