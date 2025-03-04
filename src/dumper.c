#include <libwasm.h>
#include <log.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

extern int errno;

static const uint32_t DUMP_MAGIC = 0xBADF00D;
static const uint16_t DUMP_VERSION = 0x0001;
static const char* UNNAMED_MODULE = "<UNNAMED>";
static const char* UNNAMED_FUNC = "<UNNAMED-FUNCTION>";
static const char* DUMP_EXT = ".wd";

/*
#define write(ptr, file) \
    if (!fwrite(ptr, sizeof(*ptr), 1, file)) { \
        error("Writing value to dumping file failed"); \
        perror("Reason: "); \
        return WASM_FILE_ACCESS_ERROR; \
    }

// Also write the nul terminator along with name
#define write_string(ptr, file){  \
    int len = strlen(ptr); \
    if (!fwrite(ptr, sizeof(char), (len + 1), file)) {\
        error("Writing string to dumping file failed"); \
        perror("Reason: "); \
        return WASM_FILE_ACCESS_ERROR; \
    }}

// Only for writing uint8_t buffers
#define write_buf(ptr, n, file) \
    if (!fwrite(ptr, n, 1, file)) { \
        error("Writing buffer to dumping file failed"); \
        perror("Reason: "); \
        return WASM_FILE_ACCESS_ERROR; \
    } 
*/

// These are unsafe macros that don't check errors
// The above macors are safe but for some reason fwrite return a wrong number of
// bytes written despite writing everything
// FIXME: Unsafe workaround
#define write(ptr, file) \
    fwrite(ptr, sizeof(*ptr), 1, file);

#define write_string(ptr, file){  \
    int len = strlen(ptr); \
    fwrite(ptr, sizeof(char), (len + 1), file);\
}

#define write_buf(ptr, n, file) \
    fwrite(ptr, n, 1, file)

int dumpModule(struct WasmModule* module) {
    const char* _name;

    if (module->name) 
        _name = module->name;
    else
        _name = UNNAMED_MODULE;
    
    int l = strlen(_name);
    char* name = malloc(l + 4);
    memset(name, '\0', l);
    memcpy(name, _name, l);
    strcat(name, DUMP_EXT);
    info("Dumping file = %s", name);

    FILE* file = fopen(name, "w");
    if (!file) {
        error("Failed to open dumping file for writing");
        return WASM_FILE_ACCESS_ERROR;
    }

    write(&DUMP_MAGIC, file);
    write(&DUMP_VERSION, file);
    write(&l, file);
    write_string(name, file);
    write(&module->flags, file);
    write(&module->nglobals, file);
    write(&module->nfuncs, file);

    for (int i = 0; i < module->nfuncs; i++) {
        if (module->functions[i].name) {
            write_string(module->functions[i].name, file);
        }
        else 
            write_string(UNNAMED_FUNC, file);

        write(&module->functions[i].hash, file);
        write(&module->functions[i].signature->ret, file);
        write(&module->functions[i].signature->paramsLen, file);
        write_buf(module->functions[i].signature->params, module->functions[i].signature->paramsLen, file);
        write(&module->functions[i].signature->idx, file);

        if (module->functions[i].code) {
            write(&module->functions[i].code->localSize, file);
            write_buf(&module->functions[i].code->locals, module->functions[i].code->localSize, file);
            write(&module->functions[i].code->codeSize, file);
            write_buf(&module->functions[i].code->expr, module->functions[i].code->codeSize, file);
        }
        else {
            int l = 0;
            write(&l, file); // 0 localsize
            write(&l, file); // 0 codesize
        }
    }

    write(&module->nglobals, file);
    for (int i = 0; i < module->nglobals; i++) {
        write(&module->globals[i].valtype, file);
        write(&module->globals[i].mut, file);
        write(&module->globals[i].exprSize, file);
        write_buf(module->globals[i].expr, module->globals[i].exprSize, file);
    }

    if (module->memories) {
        int t = 1;
        write(&t, file);
        write(&module->memories->memory->min, file);
        write(&module->memories->memory->max, file);
        write(&module->memories->nData, file);
        for (int i = 0; i < module->memories->nData; i++) {
            write(&module->memories->init[i].exprSize, file);
            write_buf(module->memories->init[i].expr, module->memories->init[i].exprSize, file);
            write(&module->memories->init[i].len, file);
            write_buf(module->memories->init[i].bytes, module->memories->init[i].len, file);
        }
    }
    else {
        int t = 0;
        write(&t, file);
    }

    if (module->tables) {
        int t = 1;
        write(&t, file);
        write(&module->tables->table->min, file);
        write(&module->tables->table->max, file);
        write(&module->tables->nElement, file);
        for (int i = 0; i < module->tables->nElement; i++) {
            write(&module->tables->init[i].exprSize, file);
            write_buf(module->tables->init[i].expr, module->tables->init[i].exprSize, file);
            write(&module->tables->init[i].len, file);
            // Multiply by 4 as size of each funcidx is 4
            write_buf(module->tables->init[i].funcidx, module->tables->init[i].len * 4, file);
        }
    }
    else {
        int t = 0;
        write(&t, file);
    }
    
    return WASM_SUCCESS;
}