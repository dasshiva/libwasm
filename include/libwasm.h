#ifndef __LIBWASM_H__
#define __LUBWASM_H__

struct WasmModule;
struct WasmModuleConfig;

struct WasmModuleReader {
	struct WasmModule* thisModule;
	struct WasmConfig* config;
	uint32_t           size;
	uint32_t           offset;
};

struct WasmModuleWriter {
	struct WasmModule* thisModule;
	struct WasmConfig* config;
	uint32_t           size;
        uint32_t           offset;
};

struct Section {
	const char*    name;
	const uint64_t hash;
	union {
		void* custom;
	};
};

struct WasmConfig {
	uint32_t maxCustomSectionSize;
	uint32_t maxModuleSize;
	uint32_t maxBuiltinSectionSize;
	uint32_t data;
};

struct WasmModule {
	const char*       name;
	const uint64_t    hash;
	uint64_t          flags;
	struct   Section* sections;
};
#endif
