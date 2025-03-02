#include "precompiled-hashes.h"
#include <libwasm.h>
#include <stdio.h>
#include <stdlib.h>

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
    struct Section type = module->sections[typeidx];

    if (function.flags != code.flags)
	    return WASM_FUNCTION_CODE_MISMATCH;

    // Now we know our code and function sections match up so all code
    // bodies are owned by a function
    // Now see the if type indices used by the function section 
    // are valid indices or not
    uint32_t tysize = type.flags;
    for (uint32_t i = 0; i < function.flags; i++) {
	    if (function.functions[i] >= tysize)
		    return WASM_INVALID_TYPE_INDEX;
    }

    // We now have the full trio needed to represent a function:
    // the function's type and its code and local variables
    // To make it easier to access the function as a whole entity,
    // we will group all the units of a function into one structure

    // We also need to include the imports as functions
    int impidx = findSectionByHash(module, WASM_HASH_Import);
    int imported = 0;
    if (impidx != -1) 
        imported = module->sections[impidx].flags;

    module->functions = malloc(sizeof(Function) * (function.flags + imported));
    for (int i = 0; i < imported; i++) {
        module->functions[i].signature = &module->sections[typeidx].types[i];
        module->functions[i].code = NULL;
        module->functions[i].hash = 0;
        module->functions[i].name = NULL;
    }

    for (int i = imported; i < function.flags + imported; i++) {
	    module->functions[i].signature = &module->sections[typeidx].types[i];
	    module->functions[i].code = &module->sections[codeidx].code[i];
        module->functions[i].hash = 0;
        module->functions[i].name = NULL;
    }

    module->functions->nfuncs = function.flags + imported;

    int nameidx = findSectionByHash(module, WASM_HASH_name);
    if (nameidx != -1) {
        struct Section n = module->sections[nameidx];
        if (n.names->moduleName) {
            module->name = n.names->moduleName;
            module->hash = hash(n.names->moduleName);
        }

        int valid = 1;
        if (n.names->indexes && n.names->functionNames) {
            for (uint32_t i = 0; i < n.flags; i++) {
                if (n.names->indexes[i] >= (function.flags + imported)) {
                    valid = 0;
                    break;
                }
            }
        }

        if (valid) {
            for (int i = 0; i < (function.flags + imported); i++) {
                module->functions[i].name = n.names->functionNames[i];
            }
            
        }
        else {
            // Do nothing as custom sections' contents cannot invalidate
            // module content
        }

    }

    return WASM_SUCCESS;
}

