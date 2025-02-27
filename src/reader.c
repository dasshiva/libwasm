#include <libwasm.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdio.h>


#define  DEFAULT_MAX_MODULE_SIZE          (16 * 1024 * 1024)               // 16 MB default
#define  DEFAULT_MAX_BUILTIN_SECTION_SIZE ((DEFAULT_MAX_MODULE_SIZE) / 8)  // 2 MB default
#define  DEFAULT_MAX_CUSTOM_SECTION_SIZE  ((DEFAULT_MAX_MODULE_SIZE) / 16) // 1 MB default

static int validateArguments(struct WasmModuleReader* init, struct WasmConfig *config);

int createReader(struct WasmModuleReader *init, struct WasmConfig *config) {
    int status;

    status = validateArguments(init, config);
    if (status) 
        return status;

    init->config = config;
    
    struct stat st;
    if (stat(config->name, &st)) 
        return WASM_FILE_ACCESS_ERROR;

    if (st.st_size > config->maxModuleSize) 
        return WASM_MODULE_TOO_LARGE;

    init->_data = malloc(st.st_size);
    if (!init->_data) 
        return WASM_OUT_OF_MEMORY;

    FILE* file = fopen(config->name, "rb");
    if (!file) {
        status = WASM_FILE_ACCESS_ERROR;
        goto free_data;
    }

    if(fread(init->_data, 1, st.st_size, file) != st.st_size) {
        status = WASM_FILE_READ_ERROR;
        goto free_data;
    }

    fclose(file);

    init->offset = 0;
    init->size = st.st_size;

    init->thisModule = malloc(sizeof(struct WasmModule));
    if (!init->thisModule) {
        status = WASM_OUT_OF_MEMORY;
        goto free_data;
    }

    init->thisModule->name = config->name;
    init->thisModule->hash = hash(config->name);
    return WASM_SUCCESS;

free_data:
    free(init->_data);
    return status;
}

struct WasmModule* getModuleFromReader(struct WasmModuleReader* reader) {
    return reader->thisModule;
}

void destroyReader(struct WasmModuleReader *obj) {
    if (obj->_data) 
        free(obj->_data);
    
    if (obj->thisModule)
        free(obj->thisModule);
}

static int validateArguments(struct WasmModuleReader* init, struct WasmConfig *config) {
    // First some crucial checks
    if (!init) 
       return WASM_ARGUMENT_NULL;

    if (!config)
       return WASM_INVALID_ARG;

    if (!config->name) 
       return WASM_INVALID_MODULE_NAME;

    if (!config->maxModuleSize) 
       config->maxModuleSize = DEFAULT_MAX_MODULE_SIZE;

    if (!config->maxBuiltinSectionSize)
       config->maxBuiltinSectionSize = DEFAULT_MAX_MODULE_SIZE;

    if (!config->maxCustomSectionSize)
       config->maxCustomSectionSize = DEFAULT_MAX_CUSTOM_SECTION_SIZE;

   return WASM_SUCCESS;
}