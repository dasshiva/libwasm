#include <libwasm.h>
#include <stdio.h>

static int compactIntoFunction();
static int compactIntoMemory();
static int compactIntoTable();
static int compactIntoImport();
static int prepareValidatedModule();

int validateModule(struct WasmModule *module) {
    int tyidx = findSectionByHash(module, WASM_HASH_Type);
    int fnidx = findSectionByHash(module, WASM_HASH_Function);
    return 1;
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