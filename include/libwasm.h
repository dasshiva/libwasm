#ifndef __LIBWASM_H__
#define __LIBWASM_H__

#include <stdint.h>

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

struct Section {
	const char*    name;
	const uint64_t hash;
	union {
		void* custom;
	};
};

struct WasmConfig {
	uint32_t    maxCustomSectionSize;
	uint32_t    maxModuleSize;
	uint32_t    maxBuiltinSectionSize;
	uint32_t    flags;
	const char* name;
};

struct WasmModule {
	const char*       name;
	uint64_t          hash;
	uint64_t          flags;
	struct   Section* sections;
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

#define WASM_MAGIC   0x0061736D
#define WASM_VERSION 0x01000000

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
	WASM_MAX_ERROR,
};

const char* errString(int err);
#endif
