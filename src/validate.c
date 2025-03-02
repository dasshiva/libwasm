#include <libwasm.h>
#include <stdio.h>

static int compactIntoFunction();
static int compactIntoMemory();
static int compactIntoTable();
static int compactIntoImport();
static int prepareValidatedModule();

int validateModule(struct WasmModule *module) {
    int typeidx = findSectionByHash(module, WASM_HASH_Type);
    int fnidx = findSectionByHash(module, WASM_HASH_Function);
    int codeidx = findSectionByHash(module, WASM_HASH_Code);

    // If types section is absent then neither code nor function sections
    // should be present as they will have nothing to be validated against
    if (typeidx == -1 && fnidx == -1 && codeidx == -1) { 
	    return WASM_SUCCESS;
    }

    else if (typeidx == -1) {
	    if (fnidx == -1 && codeidx == -1) {}
	    else 
		    return WASM_NO_TYPE;
    }

    // Now we know all our code, type and function sections are present
    // Now see if function and code sections are equal in length
    struct Section function = module->sections[fnidx];
    struct Section code = module->sections[codeidx];

    if (function.flags != code.flags)
	    return WASM_FUNCTION_CODE_MISMATCH;
    return WASM_SUCCESS;
}

static int compactIntoFunction() {
    return 1;
}

static int compactIntoMemory() {
    return 1;
}

static int compactIntoTable() {
    return 1;
}

static int compactIntoImport() {
    return 1;
}

static int prepareValidatedModule() {
    return 1;
}
