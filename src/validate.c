#include <libwasm.h>
#include <stdio.h>
int validateModule(struct WasmModule *module) {
    int tyidx = findSectionByHash(module, WASM_HASH_Type);
    int fnidx = findSectionByHash(module, WASM_HASH_Function);
    return 1;
}