#pragma once
#include "runtime_base.h"
#ifdef __SWITCH__


/* Interface: module */


#include <elf.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <switch.h>


struct SsrnxLinker;

typedef enum SsrnxModuleState {
    SSRNX_MODULE_EMPTY = 0,
    SSRNX_MODULE_LOADED,
    SSRNX_MODULE_LAYOUT_READY,
    SSRNX_MODULE_RESERVED,
    SSRNX_MODULE_DYNAMIC_PARSED,
    SSRNX_MODULE_RELOCATING,
    SSRNX_MODULE_RELOCATED,
    SSRNX_MODULE_MAPPED,
    SSRNX_MODULE_CONSTRUCTED,
    SSRNX_MODULE_FINALIZED,
    SSRNX_MODULE_FAILED
} SsrnxModuleState;

typedef struct SsrnxRelocStats {
    size_t relative;
    size_t glob_dat;
    size_t jump_slot;
    size_t abs64;
    size_t relr;
    size_t tls_dtpmod;
    size_t tls_dtprel;
    size_t tls_tprel;
    size_t tlsdesc;
    size_t irelative;
    size_t ifunc;
    size_t weak_zero;
    size_t unresolved;
    size_t unsupported;
} SsrnxRelocStats;

typedef struct SsrnxTlsInfo {
    bool present;
    uint64_t vaddr;
    size_t file_size;
    size_t mem_size;
    size_t alignment;
    size_t module_id;
    ptrdiff_t static_offset;
    bool registered;
} SsrnxTlsInfo;

typedef struct SsrnxDynamicInfo {
    /*
     * Keep ELF virtual addresses as the canonical dynamic symbol/string-table
     * locations. Readers select the writable backing view before mapping and
     * the runtime alias after Horizon code-memory mapping.
     */
    uint64_t strtab_vaddr;
    size_t strsz;

    uint64_t symtab_vaddr;
    size_t syment;
    size_t symbol_count;

    Elf64_Rela *rela;
    size_t rela_count;

    /* Android packed RELA (APS2) is decoded at dynamic-parse time into owned
     * Elf64_Rela entries so the normal relocation engine remains canonical. */
    Elf64_Rela *android_rela;
    size_t android_rela_count;

    Elf64_Rela *jmprel;
    size_t jmprel_count;

    Elf64_Xword *relr;
    size_t relr_count;
    bool relr_is_android;

    uint64_t init_vaddr;
    uint64_t init_array_vaddr;
    size_t init_array_count;
    uint64_t fini_vaddr;
    uint64_t fini_array_vaddr;
    size_t fini_array_count;

    uint64_t hash_vaddr;
    uint64_t gnu_hash_vaddr;

    uint64_t versym_vaddr;
    uint64_t verneed_vaddr;
    size_t verneed_count;
    uint64_t verdef_vaddr;
    size_t verdef_count;

    uint32_t *needed_name_offsets;
    size_t needed_count;
    uint32_t soname_offset;

    /* Dynamic-linker policy bits. Android/Bionic uses DT_FLAGS_1/DF_1_GLOBAL
     * to build the process-wide global lookup group, and DT_SYMBOLIC or
     * DT_FLAGS/DF_SYMBOLIC to request local-first binding for definitions in
     * this DSO. Keep the raw flag words so the linker can reproduce that
     * policy without title-specific exceptions. */
    uint64_t flags;
    uint64_t flags_1;
    bool symbolic;
} SsrnxDynamicInfo;

typedef struct SsrnxModule {
    char path[0x200];

    void *file_image;
    size_t file_size;

    Elf64_Ehdr *ehdr;
    Elf64_Phdr *phdrs;
    Elf64_Shdr *shdrs;

    uint64_t min_vaddr;
    uint64_t max_vaddr;
    size_t image_size;

    void *backing;
    void *runtime_base;
    uintptr_t load_bias;
    struct SsrnxVmRange *reservation;
    bool mapped;
    bool post_map_relocations_applied;

    struct SsrnxLinker *linker;

    SsrnxDynamicInfo dynamic;
    SsrnxTlsInfo tls;
    SsrnxRelocStats reloc_stats;
    SsrnxModuleState state;
} SsrnxModule;

void ssrnx_module_init(SsrnxModule *module);
void ssrnx_module_unload(SsrnxModule *module);

void *ssrnx_module_runtime_address(const SsrnxModule *module, uint64_t elf_vaddr);
void *ssrnx_module_backing_address(const SsrnxModule *module, uint64_t elf_vaddr);
bool ssrnx_module_backing_contains(const SsrnxModule *module, uint64_t elf_vaddr, size_t size);
bool ssrnx_module_runtime_contains(const SsrnxModule *module, uintptr_t runtime_address, size_t size);


/* Interface: dynamic */


#include <stdbool.h>



SsrnxResult ssrnx_dynamic_parse(SsrnxModule *module);
SsrnxResult ssrnx_dynamic_run_constructors(SsrnxModule *module);
SsrnxResult ssrnx_dynamic_run_destructors(SsrnxModule *module);

/* Enable verbose AArch64 constructor tracing around real-APK bring-up.
 * Kept off for the retained Stage 1-12 fixtures so diagnostics do not alter
 * their normal output or timing. */
void ssrnx_dynamic_set_constructor_trace(bool enabled);

SsrnxResult ssrnx_dynamic_get_needed(const SsrnxModule *module, size_t index, const char **out_name);
SsrnxResult ssrnx_dynamic_get_soname(const SsrnxModule *module, const char **out_name);


/* Interface: symbols */


#include <elf.h>
#include <stdint.h>



typedef struct SsrnxSymbolVersion {
    const char *name;
    const char *provider; /* DT_VERNEED vn_file for imported versions. */
    uint32_t hash;
    bool hidden;
} SsrnxSymbolVersion;

typedef struct SsrnxResolvedSymbol {
    uintptr_t address;
    const char *name;
    SsrnxSymbolVersion version;
    const SsrnxModule *provider;
    unsigned type;
    bool weak_zero;
} SsrnxResolvedSymbol;

SsrnxResult ssrnx_symbol_get(const SsrnxModule *module, size_t symbol_index,
                             const Elf64_Sym **out_symbol, const char **out_name);
SsrnxResult ssrnx_symbol_resolve(SsrnxModule *module, size_t symbol_index,
                                 SsrnxResolvedSymbol *out);
/* Re-resolve a relocation symbol after mapping without recording a second
 * compatibility bind or relocation failure statistic. Used by the deferred
 * IFUNC pass, which revisits already-accounted relocations. */
SsrnxResult ssrnx_symbol_resolve_post_map(SsrnxModule *module, size_t symbol_index,
                                          SsrnxResolvedSymbol *out);
SsrnxResult ssrnx_symbol_probe(const SsrnxModule *module, size_t symbol_index,
                               const char **out_name, bool *out_weak_zero);

SsrnxResult ssrnx_symbol_version(const SsrnxModule *module, size_t symbol_index,
                                   SsrnxSymbolVersion *out_version);
SsrnxResult ssrnx_symbol_find_defined_versioned_ex(const SsrnxModule *module, const char *name,
                                                   const SsrnxSymbolVersion *required_version,
                                                   uintptr_t *out_address, unsigned *out_type);
SsrnxResult ssrnx_symbol_find_defined_versioned(const SsrnxModule *module, const char *name,
                                                const SsrnxSymbolVersion *required_version,
                                                uintptr_t *out_address);
SsrnxResult ssrnx_symbol_find_tls_defined_versioned(const SsrnxModule *module, const char *name,
                                                    const SsrnxSymbolVersion *required_version,
                                                    size_t *out_offset);
SsrnxResult ssrnx_symbol_resolve_tls(const SsrnxModule *requester, size_t symbol_index,
                                     const SsrnxModule **out_provider, size_t *out_offset,
                                     SsrnxSymbolVersion *out_version);
SsrnxResult ssrnx_symbol_find_defined(const SsrnxModule *module, const char *name,
                                      uintptr_t *out_address);


/* Interface: linker */


#include <stdbool.h>
#include <stddef.h>




typedef struct SsrnxLinkerNode SsrnxLinkerNode;

typedef struct SsrnxLinker {
    SsrnxLinkerNode **nodes;
    size_t node_count;
    size_t node_capacity;
    size_t compat_bind_count;
    char library_dir[0x200];
} SsrnxLinker;

void ssrnx_linker_init(SsrnxLinker *linker, const char *library_dir);
void ssrnx_linker_unload(SsrnxLinker *linker);

/* Load one root module plus its complete DT_NEEDED graph. */
SsrnxResult ssrnx_linker_load(SsrnxLinker *linker, const char *path, SsrnxModule **out_root);

/* Stage 14 Java Activity bootstrap may know the complete native root set before
 * Java executes the corresponding System.loadLibrary() calls. Discover the
 * union graph and reserve guest addresses only. Relocation, mapping,
 * .init/.init_array and JNI_OnLoad remain tied to the actual load operation so
 * unrelated RTLD_LOCAL roots cannot leak into one another's symbol scope. */
SsrnxResult ssrnx_linker_prepare_roots(SsrnxLinker *linker,
                                       const char *const *paths,
                                       size_t path_count,
                                       SsrnxModule **out_primary_root);

/* Run one already-mapped module's dependency constructors exactly once. Safe to
 * call repeatedly; dependencies are constructed before the requested module.
 * A constructor failure is latched: later calls return it without re-running. */
SsrnxResult ssrnx_linker_construct_module(SsrnxLinker *linker, const SsrnxModule *module);

/* True once the module's DT_INIT/DT_INIT_ARRAY have completed, i.e. once the
 * module is "loaded" in the Android-observable dlopen() sense. A module that
 * prepare_roots() mapped with deferred constructors reports false. */
bool ssrnx_linker_module_constructed(const SsrnxLinker *linker, const SsrnxModule *module);

/* Eager convenience path retained for NativeActivity and host tests: prepare the
 * complete graph, then construct every requested root in dependency order. */
SsrnxResult ssrnx_linker_load_roots(SsrnxLinker *linker,
                                    const char *const *paths,
                                    size_t path_count,
                                    SsrnxModule **out_primary_root);

/* Android System.loadLibrary() may request another root after the initial Java
 * bootstrap graph is already live. Add one lib*.so root to the existing graph
 * without relocating/reconstructing modules that are already loaded. */
SsrnxResult ssrnx_linker_load_library(SsrnxLinker *linker,
                                      const char *soname,
                                      SsrnxModule **out_module,
                                      bool *out_newly_loaded);

/* Resolve one undefined guest symbol against the loaded dependency scope. */
SsrnxResult ssrnx_linker_resolve_import_versioned(const SsrnxModule *requester,
                                                  const char *name,
                                                  const SsrnxSymbolVersion *version,
                                                  uintptr_t *out_address,
                                                  const SsrnxModule **out_provider);
SsrnxResult ssrnx_linker_resolve_import(const SsrnxModule *requester,
                                        const char *name,
                                        uintptr_t *out_address,
                                        const SsrnxModule **out_provider);

/* Side-effect-free import lookup used by relocation preflight. Unlike the
 * normal resolver it does not increment compatibility bind counters or emit
 * per-bind success logs. */
SsrnxResult ssrnx_linker_resolve_tls_import_versioned(const SsrnxModule *requester,
                                                      const char *name,
                                                      const SsrnxSymbolVersion *version,
                                                      const SsrnxModule **out_provider,
                                                      size_t *out_offset);

SsrnxResult ssrnx_linker_probe_import_versioned(const SsrnxModule *requester,
                                                const char *name,
                                                const SsrnxSymbolVersion *version,
                                                uintptr_t *out_address,
                                                const SsrnxModule **out_provider);
SsrnxResult ssrnx_linker_probe_import(const SsrnxModule *requester,
                                      const char *name,
                                      uintptr_t *out_address,
                                      const SsrnxModule **out_provider);

/* dlsym(handle, name) searches the handle's local dependency group, not just
 * the root object. Version is NULL for ordinary dlsym and non-NULL for future
 * dlvsym-style lookups. */
SsrnxResult ssrnx_linker_find_in_handle(const SsrnxLinker *linker,
                                        const SsrnxModule *module,
                                        const char *name,
                                        const SsrnxSymbolVersion *version,
                                        uintptr_t *out_address,
                                        const SsrnxModule **out_provider);
SsrnxResult ssrnx_linker_find_global(const SsrnxLinker *linker,
                                     const char *name,
                                     const SsrnxSymbolVersion *version,
                                     uintptr_t *out_address,
                                     const SsrnxModule **out_provider);

/* Stable dlopen handle lifetime. Java/System.loadLibrary roots are pinned;
 * native dlopen references are counted and dlclose runs eligible destructors. */
SsrnxResult ssrnx_linker_acquire_module(SsrnxLinker *linker, const SsrnxModule *module);
SsrnxResult ssrnx_linker_release_module(SsrnxLinker *linker, const SsrnxModule *module);
SsrnxResult ssrnx_linker_pin_module(SsrnxLinker *linker, const SsrnxModule *module);
bool ssrnx_linker_module_handle_live(const SsrnxLinker *linker, const SsrnxModule *module);

/* Resolve only against Bionic's process-wide guest global group. Used when a
 * relocation references a default-visibility symbol defined by the requester:
 * such a definition is preemptible unless the DSO requested symbolic binding
 * or the symbol is STV_PROTECTED. */
SsrnxResult ssrnx_linker_probe_global_versioned(const SsrnxModule *requester,
                                                const char *name,
                                                const SsrnxSymbolVersion *version,
                                                uintptr_t *out_address,
                                                const SsrnxModule **out_provider);
SsrnxResult ssrnx_linker_probe_global(const SsrnxModule *requester,
                                      const char *name,
                                      uintptr_t *out_address,
                                      const SsrnxModule **out_provider);

/* dlopen(..., RTLD_GLOBAL) promotes an already-loaded local root so its symbols
 * are visible to later loads. DF_1_GLOBAL modules are promoted automatically. */
SsrnxResult ssrnx_linker_promote_global(SsrnxLinker *linker,
                                        const SsrnxModule *module,
                                        const char *reason);

size_t ssrnx_linker_module_count(const SsrnxLinker *linker);
size_t ssrnx_linker_compat_bind_count(const SsrnxLinker *linker);
const SsrnxModule *ssrnx_linker_module_at(const SsrnxLinker *linker, size_t index);


/* Interface: loader */



/* Generic single-module loader retained for Stage 1/2 regression tests. */
SsrnxResult ssrnx_loader_load(const char *path, SsrnxModule *module);

/* Stage 1 regression path: succeeds only when the guest requires no relocations. */
SsrnxResult ssrnx_loader_load_stage1(const char *path, SsrnxModule *module);

SsrnxResult ssrnx_loader_find_export(const SsrnxModule *module, const char *name, void **out_address);

/* Stage 3 linker building blocks. These do not run constructors implicitly. */
SsrnxResult ssrnx_loader_prepare(const char *path, SsrnxModule *module);
/* Header-only classification for speculative Java native-library candidates. */
SsrnxResult ssrnx_loader_probe_shared_object(const char *path, bool *out_shared);
SsrnxResult ssrnx_loader_relocate(SsrnxModule *module);
SsrnxResult ssrnx_loader_patch_system_registers(SsrnxModule *module);
SsrnxResult ssrnx_loader_map(SsrnxModule *module);
SsrnxResult ssrnx_loader_resolve_ifuncs(SsrnxModule *module);
SsrnxResult ssrnx_loader_construct(SsrnxModule *module);


/* Interface: relocations */



SsrnxResult ssrnx_apply_relr(SsrnxModule *module);
SsrnxResult ssrnx_preflight_relocations(SsrnxModule *module, size_t *out_unique_missing);
SsrnxResult ssrnx_apply_relocations(SsrnxModule *module);
SsrnxResult ssrnx_apply_post_map_relocations(SsrnxModule *module);


#endif
