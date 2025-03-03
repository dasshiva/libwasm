#ifndef __LIBWASM_H__
#define __LIBWASM_H__

#include <stdint.h>
#include "precompiled-hashes.h"

struct WasmModule;
struct WasmConfig;

struct WasmModuleReader {
	struct WasmModule* thisModule;
	struct WasmConfig* config;
	uint32_t           size;
	uint32_t           offset;
	void*              _data;
};

struct WasmModuleWriter {
	struct WasmModule* thisModule;
	struct WasmConfig* config;
	uint32_t           size;
    	uint32_t           offset;
	void*              _data;
};


struct WasmConfig {
	uint32_t    maxCustomSectionSize;
	uint32_t    maxModuleSize;
	uint32_t    maxBuiltinSectionSize;
	uint32_t    flags;
	const char* name;
};

struct Section;
struct Function;
struct GlobalSectionGlobal;
struct Table;
struct Memory;

struct WasmModule {
	const char*                   name;
	uint64_t                      hash;
	uint64_t                      flags;
	uint64_t                      nglobals;
	struct   Section*             sections;
	struct   Function*            functions;
	struct   GlobalSectionGlobal* globals;
	struct   Table*               tables;
	struct   Memory*              memories;
};

typedef struct WasmModuleReader Reader;
typedef struct WasmModuleWriter Writer;
typedef struct WasmModule       Module;
typedef struct WasmConfig       Config;
typedef struct Section          Section;

#define NULL_STRING_HASH 0xBADBADBADUL
uint64_t hash(const char* s);

// WasmModuleReader functions
int    createReader(struct WasmModuleReader* init, struct WasmConfig* config);
int    parseModule(struct WasmModuleReader* reader);
struct WasmModule* getModuleFromReader(struct WasmModuleReader* init);

// NOTE: Do not free a struct WasmModule* by yourself
// Always use destroyReader() explicitly to free these resources
void   destroyReader(struct WasmModuleReader* obj);

// WasmModuleWriter functions
int    createWriter(struct WasmModuleWriter* init, struct WasmConfig* config);
struct WasmModule* getModuleFromWriter(struct WasmModuleWriter* init);

// NOTE: Do not free a struct WasmModule* by yourself
// Always use destroyWriter() explicitly to free these resources
void   destroyWriter(struct WasmModuleWriter* obj);

#define WASM_MAGIC   0x6D736100
#define WASM_VERSION 0x00000001

// codes returned by parseModule
enum {
	WASM_SUCCESS = 0,
	WASM_OUT_OF_MEMORY,
	WASM_FILE_ACCESS_ERROR,
	WASM_FILE_INVALID_MAGIC,
	WASM_FILE_INVALID_VERSION,
	WASM_TRUNCATED_FILE,
	WASM_ARGUMENT_NULL,
	WASM_INVALID_ARG,
	WASM_INVALID_MODULE_NAME,
	WASM_MODULE_TOO_LARGE,
	WASM_FILE_READ_ERROR,
	WASM_INVALID_SECTION_ID,
	WASM_SECTION_TOO_LARGE,
	WASM_CUSTOM_SECTION_TOO_LARGE,
	WASM_INVALID_TYPEVAL,
	WASM_TRAILING_BYTES,
	WASM_UNSUPPORTED_MULTIVALUE_FUNC,
	WASM_TOO_MANY_PARAMS,
	WASM_INVALID_FUNCTYPE,
	WASM_EMPTY_NAME,
	WASM_INVALID_IMPORT_TYPE,
	WASM_TOO_MANY_TABLES,
	WASM_INVALID_TABLE_ELEMENT_TYPE,
	WASM_TOO_MANY_MEMORIES,
	WASM_INVALID_GLOBAL_MUTABILITY,
	WASM_INIT_TOO_LONG,
	WASM_INVALID_EXPR,
	WASM_INVALID_MEMORY_INDEX,
	WASM_INVALID_TABLE_INDEX,
        WASM_TRUNCATED_SECTION,
	WASM_MAX_ERROR,
};

// Codes returned by validateModule
enum {
	WASM_NO_TYPE = WASM_MAX_ERROR + 1,
	WASM_FUNCTION_CODE_MISMATCH,
	WASM_INVALID_TYPE_INDEX,
	WASM_MAX_VALIDATION_ERROR
};

const char* errString(int err);

struct TypeSectionType {
	uint8_t* params;
	uint32_t idx;
	uint8_t  ret;
	uint8_t  paramsLen;
};

typedef struct TypeSectionType Type;

// values for ImportSectionImport.type
enum {
	WASM_TYPEIDX,
	WASM_TABLETYPE,
	WASM_MEMTYPE,
	WASM_GLOBALTYPE,
	WASM_MAXTYPE
};

struct ImportSectionImport {
	char*       name;
	uint64_t    hashName;
	char*       module;
	uint64_t    hashModule;
	uint32_t    index;
	uint8_t     type;
};

typedef struct ImportSectionImport Import;

struct ExportSectionExport {
	char*       name;
	uint64_t    hashName;
	uint32_t    index;
	uint8_t     type;
};

typedef struct ExportSectionExport Export;
struct TableSectionTable {
	uint32_t min;
	uint32_t max;
};


struct GlobalSectionGlobal {
	uint8_t* expr;
	uint8_t  mut;
	uint8_t  valtype;
};

typedef struct GlobalSectionGlobal Global;

struct CodeSectionCode {
	uint32_t codeSize;
	uint32_t localSize;
	uint8_t* locals;
	uint8_t* expr;
};

typedef struct CodeSectionCode Code;

struct DataSectionData {
	uint8_t* expr;
	uint8_t* bytes;
	uint32_t len;
};

typedef struct DataSectionData Data;

struct ElementSectionElement {
	uint8_t*   expr;
	uint32_t*  funcidx;
	uint32_t   len;
};

typedef struct ElementSectionElement Element;

typedef struct Function {
	char*    name;
	uint64_t hash;
	uint64_t nfuncs;
	struct TypeSectionType* signature;
	struct CodeSectionCode* code;
} Function;

typedef struct Memory {
	struct TableSectionTable*     memory;
	struct DataSectionData*       init;
} Memory;

typedef struct Table {
	struct TableSectionTable*     table;
	struct ElementSectionElement* init;
} Table;

struct NameSectionName {
	char*     moduleName;
	uint32_t* indexes;
	char**    functionNames;
};

typedef struct NameSectionName Name;

struct Section {
	const char*    name;
	uint64_t       hash;
	union {
		struct TypeSectionType* types;
		struct ImportSectionImport* imports;
		uint32_t* functions;
		struct TableSectionTable* table;
		struct TableSectionTable* memory; // Table and memory sections are functionally almost the same
		struct ExportSectionExport* exports;
		uint64_t  start; 
		struct GlobalSectionGlobal* globals;
		struct CodeSectionCode*  code;
		struct DataSectionData*  data;
		struct ElementSectionElement* element;
		struct NameSectionName*  names;
		void*  custom;  // Unknown custom section
	};
	uint32_t       flags;

};

int validateModule(struct WasmModule* module);
int findSectionByHash(struct WasmModule* mod, const uint64_t hash);
#endif
