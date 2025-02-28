#include "read_utils.h"
#include <libwasm.h>
#include <section.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdio.h>

#define  CHECK_IF_FILE_TRUNCATED(file) { \
    if (file->offset == UINT32_MAX) { \
        return WASM_TRUNCATED_FILE; \
    } \
}
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

struct section_offset {
    uint32_t lo;
    uint32_t size;
    uint8_t  type;
};

int parseModule(struct WasmModuleReader *reader) {
    uint32_t magic = fetchRawU32(reader);
    if (magic != WASM_MAGIC) 
        return WASM_FILE_INVALID_MAGIC;
    CHECK_IF_FILE_TRUNCATED(reader);

    uint32_t version = fetchRawU32(reader);
    if(version != WASM_VERSION)
        return WASM_FILE_INVALID_VERSION;
    CHECK_IF_FILE_TRUNCATED(reader);

    if (reader->offset == reader->size)  // To deal with empty files
            return WASM_SUCCESS;

    uint16_t nsecs = 0;

     // the offset from where sections start
    uint16_t section_start_offset = reader->offset;

    // First, a validation pass to check module structural integrity
    // This makes sure that all sections are really how long they are
    // and that there are no truncated sections
    // Also tells us the number of sections we have in the file, allowing us to 
    // prepare other data structures

    while (1) {
        int id = fetchRawU8(reader);
	    if (id >= WASM_MAX_SECTION)
		    return WASM_INVALID_SECTION_ID;
        CHECK_IF_FILE_TRUNCATED(reader);

        uint32_t section = fetchU32(reader);
        CHECK_IF_FILE_TRUNCATED(reader);

        if (id && section > reader->config->maxBuiltinSectionSize) 
            return WASM_SECTION_TOO_LARGE;

        if (!id && section > reader->config->maxCustomSectionSize)
            return WASM_CUSTOM_SECTION_TOO_LARGE;

        nsecs++;
        if (reader->offset + section == reader->size)
            break;

        skip(reader, section);
    }

    reader->thisModule->flags |= nsecs;
    reader->thisModule->sections = malloc(sizeof(Section) * nsecs);
    struct section_offset* section_offsets = malloc(sizeof(struct section_offset) * nsecs);

    reader->offset = section_start_offset;

    // Now record the section offsets and their sizes
    // This allows us to parse each section parallely yielding better performance
    for(int i = 0; i < nsecs; i++) {
        uint8_t id = fetchRawU8(reader);
        uint32_t sec_length = fetchU32(reader);
        section_offsets[i].lo = reader->offset;
        section_offsets[i].size = sec_length;
        section_offsets[i].type = id;
        skip(reader, sec_length);
    }

    /*for (int i = 0; i < nsecs; i++) {
        printf("Start = 0x%x Size = 0x%x Type = %d\n", section_offsets[i].lo, section_offsets[i].size, section_offsets[i].type);
    } */

    /*
    pthread_t p1, p2, p3; // Process sections 3 at once
    int workloads = nsecs / 3;
    int extras = nsecs % 3;
    if (workloads) {
        for (int i = 0; i < workloads * 3; i += 3) {
            void* r1, *r2, *r3;
	        struct ParseSectionParams param1 = {
		        .data = reader->_data,
		        .offset = section_offsets[i].lo,
		        .size = section_offsets[i].size,
		        .section = &reader->thisModule->sections[i]
	        };

	        struct ParseSectionParams param2 = {
		        .data = reader->_data,
		        .offset = section_offsets[i + 1].lo,
		        .size = section_offsets[i + 1].size,
                .section = &reader->thisModule->sections[i + 1]
	        };

	        struct ParseSectionParams param3 = {
		        .data = reader->_data,
		        .offset = section_offsets[i + 2].lo,
		        .size = section_offsets[i + 2].size,
		        .section = &reader->thisModule->sections[i + 2]
	        };
            pthread_create(&p1, NULL, parseSectionList[section_offsets[i].type], &param1);
            pthread_create(&p2, NULL, parseSectionList[section_offsets[i + 1].type], &param2);
            pthread_create(&p3, NULL, parseSectionList[section_offsets[i + 2].type], &param3);

            pthread_join(p1, &r1);
            pthread_join(p2, &r2);
            pthread_join(p3, &r3);

            if (r1)
                return ((long)r1);

            if (r2)
                return ((long)r2);

            if (r3)
                return ((long)r3);
        }
    }

    if (extras) {
        for (int i = workloads * 3; i < nsecs; i++) {
	    struct ParseSectionParams param = {
		    .data = reader->_data,
                    .offset = section_offsets[i].lo,
                    .size = section_offsets[i].size,
                    .section = &reader->thisModule->sections[i]
	    };
            parseSectionList[section_offsets[i].type](&param);
        }
    }
    */
    

    /* 
    * To benchmark the scalar implementation with the parallel one
    * comment from 'pthread p1, p2 ...' upto the end of the 'if (extras) ..' block
    * and use a benchmarking tool like hyperfine or time */
    
    /*for (int i = 0; i < nsecs; i++) {
        struct ParseSectionParams param = {
            .data = reader->_data,
            .offset = section_offsets[i].lo,
            .size = section_offsets[i].size,
            .section = &reader->thisModule->sections[i]
        };
        parseSectionList[section_offsets[i].type](&param);
    }*/
    
    reader->offset = section_start_offset;

    return WASM_SUCCESS;
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
