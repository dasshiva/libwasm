#include <libwasm.h>
int findSectionByHash(struct WasmModule* mod, const uint64_t hash) {
    if (!mod->sections) 
        return -1;

    for (int i = 0; i < mod->flags; i++) {
        if (mod->sections[i].hash == hash) 
            return i;
    }

    return -1;
}
