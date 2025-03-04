#include <libwasm.h>
#include <log.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
extern int errno;

static const uint32_t DUMP_MAGIC = 0x0BADF00D;
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

    l += 4;

    write(&DUMP_MAGIC, file);
    write(&DUMP_VERSION, file);
    write(&l, file);
    info("L = %d", l);
    write_string(name, file);
    write(&module->flags, file);
    write(&module->nglobals, file);
    write(&module->nfuncs, file);
    info("Flags = %lu nglobals = %lu nfuncs = %lu", module->flags, module->nglobals, module->nfuncs);

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

    fclose(file);
    
    return WASM_SUCCESS;
}

static uint8_t getU8(uint8_t* buf, uint32_t* offset) {
	*offset += 1;
	return buf[*offset - 1];
}

static uint16_t getU16(uint8_t* buf, uint32_t* offset) {
	*offset += 2;
	return (((uint16_t) buf[*offset - 1]) << 8 | buf[*offset - 2]);
}

static uint32_t getU32(uint8_t* buf, uint32_t* offset) {
	*offset += 4;
	return (((uint32_t) buf[*offset - 1]) << 24 | 
			((uint32_t) buf[*offset - 2]) << 16 |
			((uint32_t) buf[*offset - 3]) << 8  |
			((uint32_t) buf[*offset - 4]));
}

static uint64_t getU64(uint8_t* buf, uint32_t* offset) {
	*offset += 4;
	return (((uint64_t) buf[*offset - 1]) << 56 | 
			((uint64_t) buf[*offset - 2]) << 48 |
			((uint64_t) buf[*offset - 3]) << 40 |
			((uint64_t) buf[*offset - 4]) << 32 |
            ((uint64_t) buf[*offset - 5]) << 24 |
            ((uint64_t) buf[*offset - 6]) << 16 |
            ((uint64_t) buf[*offset - 7]) << 8  |
            ((uint64_t) buf[*offset - 8]));
}


int loadDump(struct WasmModule* module, const char* fname) {
	if (!module) {
		error("Module is null");
		return WASM_ARGUMENT_NULL;
	}
	
	FILE* file = fopen(fname, "r");
	if (!file) {
		error("File %s cannot be accessed", fname);
		return WASM_FILE_ACCESS_ERROR;
	}

	fseek(file, 0, SEEK_END);
	uint32_t size = ftell(file);
	fseek(file, 0, SEEK_SET);

	uint8_t* buf = malloc(size * sizeof(char));
	fread(buf, size, 1, file);
	fclose(file);
	uint32_t offset = 0;

	if (getU32(buf, &offset) != DUMP_MAGIC) {
		error("Not a dump file");
		return WASM_FILE_INVALID_MAGIC;
	}

	if (getU16(buf, &offset) != DUMP_VERSION) {
		error("Not the current dump file version");
		return WASM_FILE_INVALID_VERSION;
	}

    int len = getU32(buf, &offset);
    module->name = (char*) (buf + offset);
    offset += len;

    module->flags = getU32(buf, &offset);
    offset += 4;
    module->nglobals = getU32(buf, &offset);
    offset += 4;
    module->nfuncs = getU32(buf, &offset);
    info("Flags = %d globals = %d funcs = %d", module->flags, module->nglobals, module->nfuncs);
	return WASM_SUCCESS;
}
