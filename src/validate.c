#include <libwasm.h>
#include <stdlib.h>

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

    module->functions = malloc(sizeof(Function) * function.flags);
    for (int i = 0; i < function.flags; i++) {
	    module->functions[i].signature = type.types[function.functions[i]];
	    module->functions[i].code = code.code[function.functions[i]];
    }

    // We don't need these anymore
    free(&module->sections[fnidx]);
    free(&module->sections[codeidx]);

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
