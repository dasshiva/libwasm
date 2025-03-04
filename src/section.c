#include "libwasm.h"
#include <log.h>
#include "precompiled-hashes.h"
#include <section.h>
#include <read_utils.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define  CHECK_IF_FILE_TRUNCATED(file) { \
	if (file.offset == UINT32_MAX) { \
		error("Truncated section"); \
		return WASM_TRUNCATED_SECTION; \
    } \
}

int internal_error(struct ParseSectionParams* arg) {
	error("Internal error: Parse function at invalid index called");
	return WASM_SUCCESS;
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

 /*
 * NOTE: __ALL__ uses of fetchXXX() __MUST__
 * be protected with a call to CHECK_IF_FILE_TRUNCATED
 * Otherwise attackers get a chance to break the parser by promising data to
 * be of one size and sneaking data of another size into the file 
 * causing out of bounds reads and other associated errors.
 *
 * Special care to be taken when using memcpy: 
 * Before calling memcpy use an if(...) to see if there's actually
 * as much data available that you are planning to copy. Do not at any 
 * point assume that CHECK_IF_FILE_TRUNCATED protects memcpy too.
 * It doesn't. 
 *
 * REMEMBER: All file data present as reader->_data is untrusted
 * and can contain maliciously generated data attempting to bypass
 * security checks. Keep this in mind when writing code to support
 * other types of sections. 
 * I cannot stress this point enough. DO NOT TRUST reader->_data BLINDLY.
 *
 * The current implementation parses code following the WebAssembly MVP
 * with some exceptions namely:
 * 1) the initialisation code for elements in global/data/element sections 
 *  cannot be longer than 15 bytes.
 * Deviation from spec: The spec does not enforce a limit for such code
 * 2) a single function may not take more than 255 parameters
 * Deviation from spec: The spec does not enforce a strict limit on method
 * parameters allowing them to be upto 2^32 - 1.
 */
#define CHECK_IF_VALID_VALTYPE(x) (((x) >= 0x7C) && ((x) <= 0x7F))

static int parseNameSection(struct WasmModuleReader reader, struct ParseSectionParams* params);

int parseCustomSection(struct ParseSectionParams* params) {
	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t nameSize = fetchU32(&reader);
	debug("Custom section name size = %d", nameSize);

	if (!nameSize) {
		error("Custom section cannot be empty");
		return WASM_EMPTY_NAME;
	}

	if (nameSize + reader.offset >= reader.size) {
		error("Custom section is truncated");
		return WASM_TRUNCATED_SECTION;
	}

	char* name = malloc(sizeof(char) * nameSize + 1);
	memcpy(name, (uint8_t*)reader._data + reader.offset, nameSize);
	skip(&reader, nameSize);
	name[nameSize] = '\0';

	debug("Found custom section \'%s\'", name);

	uint64_t hashName = hash(name);
	if (hashName == WASM_HASH_name) {
		params->section->name = "name";
		params->section->hash = WASM_HASH_name;
		params->section->flags = 0;
		parseNameSection(reader, params);
		return 0; // Ignore the return value
	}
	else {
		warn("Unsupported custom section \'%s\'", name);
		params->section->name = name;
		params->section->hash = hashName;
	}

	return WASM_SUCCESS;
}

static int parseNameSection(struct WasmModuleReader reader, struct ParseSectionParams* params) {
	// All subsections must occur only once and in order of increasing id
	// Thererfore the first section will be the module name and the next
	// one will be the function names
	// We do not care about the rest for now
	// Also we are not allowed to throw errors for invalid data in custom sections
	// So simply return if anything is not right
	debug("Parsing section '\'name\'");

	uint8_t id = fetchRawU8(&reader);
	if (!id) { // This is the module section
		params->section->names = malloc(sizeof(struct NameSectionName));
		fetchU32(&reader); // size of the module section immaterial to us as length of the string comes later

		uint32_t size = fetchU32(&reader);
		debug("Module name size = %d", size);

		if (!size) {
			warn("Module name is empty");
			return WASM_SUCCESS;
		}

		if (reader.offset + size >= reader.size) {
			warn("Name section is truncated");
			return WASM_SUCCESS;
		}

		params->section->names->moduleName = malloc(sizeof(char) * size + 1);
		memcpy(params->section->names->moduleName, (uint8_t*)reader._data + reader.offset, size);
		params->section->names->moduleName[size] = '\0';

		warn("Module name = %s", params->section->names->moduleName);
		skip(&reader, size);
		CHECK_IF_FILE_TRUNCATED(reader);
	} 
	else {
		warn("Expected subsection id 0 but got %d, bailing", id);
		goto ret; 
	}

	id = fetchRawU8(&reader);
	if (id == 1) {
		uint32_t secn_size = fetchU32(&reader);
		debug("Function subsection size = %u", secn_size);
		CHECK_IF_FILE_TRUNCATED(reader);

		uint32_t npairs = fetchU32(&reader);
		debug("Number of pairs in subsection = %u", npairs);
		CHECK_IF_FILE_TRUNCATED(reader);

		params->section->flags = npairs;
		params->section->names->indexes = malloc(sizeof(uint32_t) * npairs);
		params->section->names->functionNames = malloc(sizeof(char*) * npairs);
		for (uint32_t i = 0; i < npairs; i++) {
			params->section->names->indexes[i] = fetchU32(&reader);
			CHECK_IF_FILE_TRUNCATED(reader);

			uint32_t nameSize = fetchU32(&reader);
			CHECK_IF_FILE_TRUNCATED(reader);
			if (reader.offset + nameSize >= reader.size) {
				warn("Truncated name section");
				return WASM_SUCCESS;;
			}

			params->section->names->functionNames[i] = malloc(sizeof(char) * nameSize + 1);
			memcpy(params->section->names->functionNames[i], (uint8_t*)reader._data + reader.offset, nameSize);
			params->section->names->functionNames[i][nameSize] = '\0';
			skip(&reader, nameSize);
			CHECK_IF_FILE_TRUNCATED(reader);

			debug("Id[%u] = %s", i, params->section->names->functionNames[i]);
		}

	}
	else 
		warn("Expected subsection id 1 but got %d, bailing", id);

	warn("All other subsection id's are skipped");
	

	// Ignore all other subsections till we add support for them

ret:	
	return WASM_SUCCESS;
}

static int parseTypeSection(struct ParseSectionParams* params) {
	debug("Parsing type section");

	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);
	debug("Number of types = %u", size);

	params->section->name = "Type";
	params->section->hash = WASM_HASH_Type;
	params->section->flags = size;
	if (!size) {
		warn("Type section is present but empty");
		params->section->custom = NULL;
		return WASM_SUCCESS;
	}

	params->section->types = malloc(sizeof(struct TypeSectionType) * size);
	for (int i = 0; i < size; i++) {
		params->section->types[i].idx = i;
		uint8_t rd = fetchRawU8(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (rd != 0x60) {
			error("Function type does not start with 0x60");
			return WASM_INVALID_FUNCTYPE;
		}

		uint32_t plen = fetchU32(&reader);
		debug("Number of parameters = %u", plen);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (plen) {	
			if (plen > 255) {
				error("Method has too many parameters");
				return WASM_TOO_MANY_PARAMS;
			}
			
			params->section->types[i].paramsLen = plen;
			params->section->types[i].params = malloc(sizeof(uint8_t) * plen);
			for (int j = 0; j < plen; j++) {
				params->section->types[i].params[j] = fetchRawU8(&reader);
				CHECK_IF_FILE_TRUNCATED(reader);
				if (!CHECK_IF_VALID_VALTYPE(params->section->types[i].params[j])) {
					error("Value type of a function parameter is %u which is not valid", params->section->types[i].params[j]);
					return WASM_INVALID_TYPEVAL;
				}
			}
		}

		else {
			params->section->types[i].paramsLen = 0;
			params->section->types[i].params = NULL;
		}

		uint32_t retlen = fetchU32(&reader);
		debug("Number of returns = %u", retlen);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (retlen > 1) {
			error("Function with more than 1 return type are unsupported");
			return WASM_UNSUPPORTED_MULTIVALUE_FUNC;
		}

		if (retlen) {
			uint8_t rtype = fetchRawU8(&reader);
			CHECK_IF_FILE_TRUNCATED(reader);
			if (!(CHECK_IF_VALID_VALTYPE(rtype))) {
				error("Return type %u which is not a valid valtype", rtype);
                		return WASM_INVALID_TYPEVAL;
			}
			
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

	if (reader.offset + 1 != reader.size) {
		error("Type section has stray bytes");
		return WASM_TRAILING_BYTES;
	}

	return WASM_SUCCESS;
}

static int parseImportSection(struct ParseSectionParams* params) {
	debug("Parsing Import section");

	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);
	debug("Number of imports = %u", size);

	params->section->name = "Import";
	params->section->hash = WASM_HASH_Import;
	params->section->flags = size;

	if (!size) {
		warn("Import section present but empty");
		params->section->flags = 0;
		params->section->custom = NULL;
		return WASM_SUCCESS;
	}

	params->section->imports = malloc(sizeof(struct ImportSectionImport) * size);

	for (int i = 0; i < size; i++) {
		uint32_t modlen = fetchU32(&reader) + 1; // space for null
        	if (!modlen) {
			error("Import module name is empty");
            		return WASM_EMPTY_NAME;
		}

        	CHECK_IF_FILE_TRUNCATED(reader);

		if (reader.offset + modlen - 1>= reader.size) {
			error("Import section is truncated");
			return WASM_TRUNCATED_SECTION;
		}

        	params->section->imports[i].module = malloc(sizeof(const char*) * modlen);
        	memcpy(params->section->imports[i].module, (uint8_t*)reader._data + reader.offset, modlen);
        	params->section->imports[i].module[modlen - 1] = '\0';
		params->section->imports[i].hashModule = hash(params->section->imports[i].module);
        	skip(&reader, modlen - 1);
		CHECK_IF_FILE_TRUNCATED(reader);

		uint32_t namelen = fetchU32(&reader) + 1; // space for null
		if (!(namelen - 1)) {
			error("Importing entity's name is empty");
			return WASM_EMPTY_NAME;
		}

        	CHECK_IF_FILE_TRUNCATED(reader);

		if (reader.offset + namelen - 1 >= reader.size) {
			error("Import section is truncated");
			return WASM_TRUNCATED_SECTION;
		}

		params->section->imports[i].name = malloc(sizeof(const char*) * namelen);
		memcpy(params->section->imports[i].name, (uint8_t*)reader._data + reader.offset, namelen - 1);
		params->section->imports[i].name[namelen - 1] = '\0';
		params->section->imports[i].hashName = hash(params->section->imports[i].name);

        	skip(&reader, namelen - 1);
		CHECK_IF_FILE_TRUNCATED(reader);

		params->section->imports[i].type = fetchRawU8(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (params->section->imports[i].type >= WASM_MAXTYPE) {
			error("Expected valid import type but got %u", params->section->imports[i].type);
			return WASM_INVALID_IMPORT_TYPE;
		}

		params->section->imports[i].index = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);

		debug("Import[%d] %s.%s valtype = %u index = %u", i, params->section->imports[i].module, params->section->imports[i].name, params->section->imports[i].type, params->section->imports[i].name); 
	}


	if (reader.offset + 1 != reader.size) {
		error("Import Section has unclaimed bytes");
		return WASM_TRAILING_BYTES;
	}

	return WASM_SUCCESS;
}

static int parseFunctionSection(struct ParseSectionParams* params) {
	debug("Parsing function section");
	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	debug("Number of entries in function section = %u", size);
	CHECK_IF_FILE_TRUNCATED(reader);

	if (!size) {
		warn("Function section present but empty");
		return WASM_SUCCESS;
	}
	
	params->section->name = "Function";
	params->section->hash = WASM_HASH_Function;
	params->section->flags = size;
	params->section->functions = malloc(sizeof(uint32_t) * size);

	for (int i = 0; i < size; i++) {
		params->section->functions[i] = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		debug("Function[%d] = %u", i, params->section->functions[i]);
	}


	if (reader.offset + 1 != reader.size) {
		debug("Function Section has stray bytes");
		return WASM_TRAILING_BYTES;
	}

	return WASM_SUCCESS;
}

static int parseTableSection(struct ParseSectionParams* params) {
	debug("Parsing table section");

	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);
	debug("Number of tables in table section = %u", size);

	if (size != 1) {
		error("Multiple tables in one module is unsupported");
		return WASM_TOO_MANY_TABLES;
	}

	uint8_t ty = fetchRawU8(&reader);;

	if (ty != 0x70) {
		error("Tables can only have function refs (0x70), but got %u", ty);
		return WASM_INVALID_TABLE_ELEMENT_TYPE;
	}

	CHECK_IF_FILE_TRUNCATED(reader);

	params->section->flags = 1;
	params->section->name = "Table";
	params->section->hash = WASM_HASH_Table;
	params->section->table = malloc(sizeof(struct TableSectionTable));

	uint8_t limtype = fetchRawU8(&reader);
	if (limtype > 1) {
		error("Invalid limit type %u", limtype);
		return WASM_INVALID_LIMIT_TYPE;
	}

	debug("Limit type = %s", (limtype) ? "min-max" : "min");
	CHECK_IF_FILE_TRUNCATED(reader);

	if (limtype) {
		params->section->table->min = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		debug("Table min size = %u", params->section->table->min);
		params->section->table->max = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		debug("Table max size = %u", params->section->table->max);
	}
	else {
		params->section->table->min = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		debug("Table min size = %u", params->section->table->min);

		params->section->table->max = UINT32_MAX;
	}

	if (reader.offset + 1 != reader.size) {
		error("Table Section has stray bytes");
		return WASM_TRAILING_BYTES;
	}

	return WASM_SUCCESS;
}

static int parseMemorySection(struct ParseSectionParams* params) {
	debug("Parsing memory section");

	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	debug("Number of memories = %u");
	CHECK_IF_FILE_TRUNCATED(reader);

	if (size != 1) {
		debug("Multi-memory is not supported");
		return WASM_TOO_MANY_MEMORIES;
	}

	params->section->flags = 1;
	params->section->name = "Memory";
	params->section->hash = WASM_HASH_Memory;
	params->section->memory = malloc(sizeof(struct TableSectionTable));

	uint8_t limtype = fetchRawU8(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);

	if (limtype > 1) {
		error("Invalid limit type %u", limtype);
		return WASM_INVALID_LIMIT_TYPE;
	}

	debug("Limit type = %s", (limtype) ? "min-max" : "min");

	if (limtype) {
		params->section->memory->min = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		debug("Memory min size = %u", params->section->memory->min);
		params->section->memory->max = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		debug("Memory max size = %u", params->section->memory->min);
	}
	else {
		params->section->memory->min = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		debug("Memory min size = %u", params->section->memory->min);
		params->section->memory->max = UINT32_MAX;
	}

	if (reader.offset + 1 != reader.size) {
		debug("Memory section has stray bytes");
		return WASM_TRAILING_BYTES;
	}

	return WASM_SUCCESS;
}

static int parseExportSection(struct ParseSectionParams* params) {
	debug("Parsing export section");

	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);	
	CHECK_IF_FILE_TRUNCATED(reader);
	debug("Number of exports = %u", size);

	params->section->name = "Export";
	params->section->hash = WASM_HASH_Export;
	params->section->flags = size;

	if (!size) {
		params->section->flags = 0;
		params->section->custom = NULL;
		return WASM_SUCCESS;
	}

	params->section->exports = malloc(sizeof(struct ExportSectionExport) * size);

	for (int i = 0; i < size; i++) {
		uint32_t namelen = fetchU32(&reader) + 1; // space for null
            	if (!namelen) { 
			debug("Export name is empty");
			return WASM_EMPTY_NAME;
		}

	        CHECK_IF_FILE_TRUNCATED(reader);

		if (reader.offset + namelen - 1 >= reader.size) {
			debug("Truncated section");
			return WASM_TRUNCATED_SECTION;
		}

		params->section->exports[i].name = malloc(sizeof(const char*) * namelen);
		memcpy(params->section->exports[i].name, (uint8_t*)reader._data + reader.offset, namelen);
		params->section->exports[i].name[namelen - 1] = '\0';
		params->section->exports[i].hashName = hash(params->section->exports[i].name);
        	skip(&reader, namelen - 1);
		CHECK_IF_FILE_TRUNCATED(reader);

		params->section->exports[i].type = fetchRawU8(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (params->section->exports[i].type >= WASM_MAXTYPE) {
			error("Export type is not valid must be between 0 and 3 but is %u", params->section->exports[i].type);
			return WASM_INVALID_IMPORT_TYPE;
		}

		params->section->exports[i].index = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);

		debug("Export[%d] %s 0x%x", i, params->section->exports[i].name, params->section->exports[i].index);
	}

	if (reader.offset + 1 != reader.size) {
		error("Export section has stray bytes");
		return WASM_TRAILING_BYTES;
	}

	return WASM_SUCCESS;
}

static int parseStartSection(struct ParseSectionParams* params) {
	debug("Parsing start section");
	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t fn = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);
	debug("Start function index = %u", fn);

	params->section->start = fn;
	params->section->flags = 0;
	params->section->name = "Start";
	params->section->hash = WASM_HASH_Start;

	if (reader.offset + 1 != reader.size) { 
		error("Start section has stray bytes");
		return WASM_TRAILING_BYTES;
	}

	return WASM_SUCCESS;
}

static const uint8_t maxInitExprSize = 15; // Largest size of 'init' for Global/data/element sections

static int parseGlobalSection(struct ParseSectionParams* params) {
	debug("Parsing global section");
	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);
	debug("Number of globals = %u", size);

	params->section->name = "Global";
	params->section->hash = WASM_HASH_Global;
	params->section->flags = size;

	if (!size) {
		debug("Globals section present but empty");
		params->section->custom = NULL;
		return WASM_SUCCESS;
	} 

	params->section->globals = malloc(sizeof(struct GlobalSectionGlobal) * size);

	for (int i = 0; i < size; i++) {
		params->section->globals[i].valtype = fetchRawU8(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (!CHECK_IF_VALID_VALTYPE(params->section->globals[i].valtype)) {
			error("Invalid global type %u", params->section->globals[i].valtype);
			return WASM_INVALID_TYPEVAL;
		}

		params->section->globals[i].mut = fetchRawU8(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (params->section->globals[i].mut > 1) {
			error("Global mutability flag is %u which is invalid in this context", params->section->globals[i].mut);
			return WASM_INVALID_GLOBAL_MUTABILITY;
		}

		uint32_t offset = reader.offset;
		int initSize = 1;
		while (1) {
			if (initSize > maxInitExprSize) {
				error("Init expression size = %u > 15", initSize);
				return WASM_INIT_TOO_LONG;
			}

			if (fetchRawU8(&reader) == 0x0B) 
				break;

			CHECK_IF_FILE_TRUNCATED(reader);
			initSize++;
		}

		reader.offset = offset;
		params->section->globals[i].expr = malloc(sizeof(uint8_t) * initSize);
		memcpy(params->section->globals[i].expr, (uint8_t*)reader._data + offset, initSize);
		skip(&reader, initSize);
		CHECK_IF_FILE_TRUNCATED(reader);

		debug("Globals[%u] : Value Type = %d Mutable = %d Initsize = %d", i, params->section->globals[i].valtype, params->section->globals[i].mut, initSize);
	}


	if (reader.offset + 1 != reader.size) { 
		error("Global section has stray bytes");
		return WASM_TRAILING_BYTES;
	}

	return WASM_SUCCESS;
}

static int parseDataSection(struct ParseSectionParams* params) {
	debug("Parsing Data section");

	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);	
	CHECK_IF_FILE_TRUNCATED(reader);
	debug("Number of data entries in data section = %u", size);

	params->section->name = "Data";
	params->section->hash = WASM_HASH_Data;
	params->section->flags = size;

	if (!size) {
		warn("Data section present but empty");
		params->section->custom = NULL;
		return WASM_SUCCESS;;
	}

	params->section->data = malloc(sizeof(struct DataSectionData) * size);
	for (int i = 0; i < size; i++) {
		uint32_t memidx = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		if (memidx != 0) {
			error("Invalid memory index = %u", memidx);
			return WASM_INVALID_MEMORY_INDEX;
		}
		

		uint32_t off = reader.offset;
		uint8_t exprSize = 1;
		while (1) {
			if (exprSize >= maxInitExprSize) {
				error("exprSize %u >= 15", exprSize);
				return WASM_INIT_TOO_LONG;
			}

			if (fetchRawU8(&reader) == 0xB)
				break;
			CHECK_IF_FILE_TRUNCATED(reader);
			exprSize += 1;
		}

		reader.offset = off;
		params->section->data[i].expr = malloc(sizeof(uint8_t) * exprSize);
		memcpy(params->section->data[i].expr, (uint8_t*)reader._data + reader.offset, exprSize);
		skip(&reader, exprSize);
		CHECK_IF_FILE_TRUNCATED(reader);

		uint32_t dataSize = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		params->section->data[i].len = dataSize;
		params->section->data[i].bytes = malloc(sizeof(uint8_t) * dataSize);
		if (reader.offset + dataSize >= reader.size) {
			error("Truncated data section");
			return WASM_TRUNCATED_SECTION;
		}

		memcpy(params->section->data[i].bytes, (uint8_t*)reader._data + reader.offset, dataSize);
		skip(&reader, dataSize);
		CHECK_IF_FILE_TRUNCATED(reader);

		debug("Data[%u]: ExprSize = %u DataSize = %u", i, exprSize, dataSize);

	}

	if (reader.offset + 1 != reader.size) {
		error("Data section has stray bytes");
		return WASM_TRAILING_BYTES;
	}

	return WASM_SUCCESS;
}

static int parseCodeSection(struct ParseSectionParams* params) {
	debug("Parsing code section");
	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);
	debug("Number of code bodies in code section = %u", size);

	params->section->name = "Code";
	params->section->hash = WASM_HASH_Code;
	params->section->flags = size;

	if (!size) {
		warn("Code section present but empty");
		params->section->custom = NULL;
		return WASM_SUCCESS;
	}

	params->section->code = malloc(sizeof(struct CodeSectionCode) * size);

	for (int i = 0; i < size; i++) {
		uint32_t codeSize = fetchU32(&reader); // codesize includes the size of locals and function code
		uint32_t copySize = codeSize; // Keep a copy of codeSize later used for skipping to the next section

		CHECK_IF_FILE_TRUNCATED(reader);
		uint32_t poff = reader.offset;
		uint32_t paramtypes = fetchU32(&reader);
		if (paramtypes) {
			uint32_t off = reader.offset;
			uint32_t paramslen = 0;
			for (int j = 0; j < paramtypes; j++) {
				paramslen += fetchU32(&reader);
				CHECK_IF_FILE_TRUNCATED(reader);
				fetchRawU8(&reader);
				CHECK_IF_FILE_TRUNCATED(reader);
			}

			params->section->code[i].localSize = paramslen;
			reader.offset = off;
			params->section->code[i].locals = malloc(sizeof(uint8_t) * paramslen);
			uint32_t cur = 0;
			for (int j = 0; j < paramtypes; j++) {
				uint32_t n = fetchU32(&reader);
				CHECK_IF_FILE_TRUNCATED(reader);
				uint8_t type = fetchRawU8(&reader);
				CHECK_IF_FILE_TRUNCATED(reader);

				for (int k = 0; k < n; k++, cur++) {
					params->section->code[i].locals[k] = type;
				}
			}

			codeSize -= (reader.offset - poff);
		}

		else {
			params->section->code[i].localSize = 0;
			params->section->code[i].locals = NULL;
			codeSize -= 1; // 0 is always 1 byte long 
		}

		params->section->code[i].codeSize = codeSize;
		params->section->code[i].expr = malloc(sizeof(uint8_t) * codeSize);
		if (reader.offset + codeSize >= reader.size) {
			error("Code section truncated");
			return WASM_TRUNCATED_SECTION;
		}

		memcpy(params->section->code[i].expr, (uint8_t*)reader._data + reader.offset, codeSize);

		if (params->section->code[i].expr[codeSize - 1] != 0xB) {
			error("Code body ends with 0x%x instead of 0xB", params->section->code[i].expr[codeSize - 1]);
			return WASM_INVALID_EXPR;
		}
		

		reader.offset = poff;
		skip(&reader, copySize);
		CHECK_IF_FILE_TRUNCATED(reader);

		debug("Code[%u]: codeSize = %d localsSize = %d\n", i, params->section->code[i].codeSize, params->section->code[i].localSize);
	}

	if (reader.offset + 1 != reader.size) {
		error("Code section has stray bytes");
		return WASM_TRAILING_BYTES;
	}

	return WASM_SUCCESS;
}

static int parseElementSection(struct ParseSectionParams* params) {  
	debug("Parsing element section");

	struct WasmModuleReader reader;
	reader._data = params->data;
	reader.offset = params->offset;
	reader.size = params->size + params->offset + 1;

	uint32_t size = fetchU32(&reader);
	CHECK_IF_FILE_TRUNCATED(reader);
	debug("Number of initialisers in element section = %u", size);

	params->section->name = "Element";
	params->section->hash = WASM_HASH_Element;
	params->section->flags = size;

	if (!size) {
		warn("Element section present but empty");
		params->section->custom = NULL;
		return WASM_SUCCESS;
	}

	params->section->element = malloc(sizeof(struct ElementSectionElement) * size);
	for (int i = 0; i < size; i++) {
		uint32_t tabidx = fetchU32(&reader);

		if (tabidx != 0) {
			error("Invalid table index = %u", tabidx);
			return WASM_INVALID_TABLE_INDEX;
		}
		CHECK_IF_FILE_TRUNCATED(reader);

		uint32_t off = reader.offset;
		uint8_t exprSize = 1;
		while (1) {
			if (exprSize >= maxInitExprSize) {
				error("exprSize %u >= 15", exprSize);
				return WASM_INIT_TOO_LONG;
			}

			if (fetchRawU8(&reader) == 0xB)
				break;
			CHECK_IF_FILE_TRUNCATED(reader);
			
			exprSize += 1;
		}

		reader.offset = off;
		params->section->element[i].expr = malloc(sizeof(uint8_t) * exprSize);
		memcpy(params->section->element[i].expr, (uint8_t*)reader._data + reader.offset, exprSize);
		skip(&reader, exprSize);
		CHECK_IF_FILE_TRUNCATED(reader);

		uint32_t dataSize = fetchU32(&reader);
		CHECK_IF_FILE_TRUNCATED(reader);
		params->section->element[i].len = dataSize;
		params->section->element[i].funcidx = malloc(sizeof(uint32_t) * dataSize); // allocate more than needed
		for (int i = 0; i < dataSize; i++) {
			params->section->element[i].funcidx[i] = fetchU32(&reader);
			CHECK_IF_FILE_TRUNCATED(reader);
		}

		debug("Element [%u]: exprSize = %u dataSize = %u", i, exprSize, dataSize);
	}

	if (reader.offset + 1 != reader.size) {
		error("Element section has stray bytes");
		return WASM_TRAILING_BYTES;
	}

	return WASM_SUCCESS;
}

const parseFnList parseSectionList[] = {
	[WASM_CUSTOM_SECTION] = &parseCustomSection,
	[WASM_TYPE_SECTION] = &parseTypeSection,
	[WASM_IMPORT_SECTION] = &parseImportSection,
	[WASM_FUNCTION_SECTION] = &parseFunctionSection,
	[WASM_TABLE_SECTION] = &parseTableSection,
	[WASM_MEMORY_SECTION] = &parseMemorySection,
	[WASM_GLOBAL_SECTION] = &parseGlobalSection,
	[WASM_EXPORT_SECTION] = &parseExportSection,
	[WASM_START_SECTION] = &parseStartSection,
	[WASM_ELEMENT_SECTION] = &parseElementSection,
	[WASM_CODE_SECTION] = &parseCodeSection,
	[WASM_DATA_SECTION] = &parseDataSection,
	[WASM_MAX_SECTION] = &internal_error
};
