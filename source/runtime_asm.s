
/* native_call_a64 */
/* Generic AArch64 mixed-register call trampoline used by the DEX JNI bridge.
 *
 * uint64_t ssrnx_dex_call_native_a64(void *fn,
 *                                     const uint64_t gpr[8],
 *                                     const uint64_t fpr[8],
 *                                     uint64_t *out_fp_bits,
 *                                     const uint64_t *stack_words,
 *                                     size_t stack_count);
 *
 * The caller pre-classifies arguments according to AAPCS64.  Integer/object
 * arguments are loaded into x0-x7 and floating-point arguments into d0-d7;
 * arguments beyond those registers are passed in stack_words, one 8-byte slot
 * each in argument order, and copied to the outgoing argument area at sp
 * (kept 16-byte aligned).  out_fp_bits receives d0 after the call so
 * float/double JNI returns can be reconstructed without disturbing the
 * ordinary x0 return value. */
    .text
    .align  4
    .global ssrnx_dex_call_native_a64
    .type   ssrnx_dex_call_native_a64, %function
ssrnx_dex_call_native_a64:
    stp     x29, x30, [sp, #-48]!
    stp     x19, x20, [sp, #16]
    stp     x21, x22, [sp, #32]
    mov     x29, sp

    mov     x16, x0          /* target */
    mov     x17, x1          /* GPR array */
    mov     x20, x2          /* FPR array */
    mov     x19, x3          /* FP return storage */
    mov     x21, x4          /* stack argument words */
    mov     x22, x5          /* stack argument count */

    /* Outgoing stack arguments: round the slot count up to an even number so
     * sp stays 16-byte aligned, then copy the words to [sp, #8*i]. */
    add     x9, x22, #1
    and     x9, x9, #~1
    lsl     x9, x9, #3
    sub     sp, sp, x9
    mov     x10, #0
1:
    cmp     x10, x22
    b.hs    2f
    ldr     x11, [x21, x10, lsl #3]
    str     x11, [sp, x10, lsl #3]
    add     x10, x10, #1
    b       1b
2:
    ldr     d0, [x20, #0]
    ldr     d1, [x20, #8]
    ldr     d2, [x20, #16]
    ldr     d3, [x20, #24]
    ldr     d4, [x20, #32]
    ldr     d5, [x20, #40]
    ldr     d6, [x20, #48]
    ldr     d7, [x20, #56]

    ldr     x0, [x17, #0]
    ldr     x1, [x17, #8]
    ldr     x2, [x17, #16]
    ldr     x3, [x17, #24]
    ldr     x4, [x17, #32]
    ldr     x5, [x17, #40]
    ldr     x6, [x17, #48]
    ldr     x7, [x17, #56]

    blr     x16
    mov     sp, x29
    str     d0, [x19]

    ldp     x21, x22, [sp, #32]
    ldp     x19, x20, [sp, #16]
    ldp     x29, x30, [sp], #48
    ret
    .size   ssrnx_dex_call_native_a64, .-ssrnx_dex_call_native_a64



/* exception_entry */
/* Resumable CPU exception entry (linked with --wrap=__libnx_exception_entry).
 *
 * The kernel enters here with x0 = exception type and x1 = the user exception
 * frame (ThreadExceptionFrameA64: x0-x8, lr, sp, pc, pstate, afsr0, afsr1,
 * esr, far). x9-x29 and the FP/SIMD registers are still live; sp points into
 * the process-local exception region, with room for a few words only.
 * svcReturnFromException(0) reloads x0-x8, lr, sp, pc and pstate from the
 * frame and resumes the faulting instruction. The kernel lets only one thread
 * at a time run between exception entry and that return, so the resolver runs
 * on __nx_exception_stack, which libnx's own entry takes only after it.
 *
 * Permission and translation faults (instruction or data abort, FSC 0b0011xx
 * or 0b0001xx) are offered to ssrnx_wx_fault_resolve first; it emulates
 * Android RWX pages on Horizon by switching pages between RW and RX (an RW ->
 * RX switch remaps the page, and a thread touching it meanwhile takes a
 * translation fault that is resumed once the switch is done). It is compiled
 * general-regs-only, so only the AAPCS64 caller-saved general registers need
 * saving here; x18 is saved too because Android code may keep it reserved.
 * Translated Linux syscalls use BRK #0xf100 to resume on a normal guest stack;
 * BRK #0xf101 at the exact host completion stub restores the original PC/SP
 * and pstate after the syscall. No syscall executes under the exception gate.
 * Every other exception
 * goes to libnx's entry with the entry sp (SSRNX's crash report). The
 * pre-check uses only x2/x3, which libnx reads back from the frame. */
    .section .text.__wrap___libnx_exception_entry, "ax", %progbits
    .align  4
    .global __wrap___libnx_exception_entry
    .type   __wrap___libnx_exception_entry, %function
__wrap___libnx_exception_entry:
    cbz     x1, 2f
    ldr     w2, [x1, #108]          /* frame->esr */
    lsr     w3, w2, #26             /* exception class */
    cmp     w3, #0x3c               /* AArch64 BRK */
    b.eq    5f
    cmp     w3, #0x20               /* instruction abort, lower EL */
    b.eq    1f
    cmp     w3, #0x24               /* data abort, lower EL */
    b.ne    2f
    b       1f
5:  and     w2, w2, #0xffff
    mov     w3, #0xf100             /* translated Linux SVC */
    cmp     w2, w3
    b.eq    4f
    mov     w3, #0xf101             /* syscall bridge completion */
    cmp     w2, w3
    b.ne    2f
    ldr     x2, [x1, #88]           /* frame->pc */
    adrp    x3, :got:ssrnx_inline_syscall_complete
    ldr     x3, [x3, #:got_lo12:ssrnx_inline_syscall_complete]
    cmp     x2, x3
    b.ne    2f                      /* only our exact host stub may complete */
    ldr     x2, [x1, #80]           /* frame->sp: resume header */
    ldr     x3, [x2]
    str     x3, [x1, #88]           /* resume after guest SVC */
    ldr     w3, [x2, #8]
    str     w3, [x1, #96]           /* original pstate */
    add     x2, x2, #16
    str     x2, [x1, #80]           /* original guest sp */
    mov     w0, wzr
    bl      svcReturnFromException
    b       3f
1:  and     w3, w2, #0x3c
    cmp     w3, #0x0c               /* permission fault, any level */
    b.eq    4f
    cmp     w3, #0x04               /* translation fault, any level */
    b.ne    2f
4:  adrp    x2, :got:__nx_exception_stack
    ldr     x2, [x2, #:got_lo12:__nx_exception_stack]
    adrp    x3, :got:__nx_exception_stack_size
    ldr     x3, [x3, #:got_lo12:__nx_exception_stack_size]
    ldr     x3, [x3]
    cbz     x2, 2f
    cbz     x3, 2f
    add     x2, x2, x3
    and     x2, x2, #0xfffffffffffffff0
    mov     x3, sp
    mov     sp, x2
    sub     sp, sp, #112
    stp     x0, x1, [sp, #0]
    stp     x9, x10, [sp, #16]
    stp     x11, x12, [sp, #32]
    stp     x13, x14, [sp, #48]
    stp     x15, x16, [sp, #64]
    stp     x17, x18, [sp, #80]
    str     x3, [sp, #96]
    mov     x0, x1
    ldr     w2, [x1, #108]
    lsr     w2, w2, #26
    cmp     w2, #0x3c
    b.ne    6f
    bl      ssrnx_inline_syscall_route
    b       7f
6:
    bl      ssrnx_wx_fault_resolve
7:
    mov     w8, w0
    ldp     x0, x1, [sp, #0]
    ldp     x9, x10, [sp, #16]
    ldp     x11, x12, [sp, #32]
    ldp     x13, x14, [sp, #48]
    ldp     x15, x16, [sp, #64]
    ldp     x17, x18, [sp, #80]
    ldr     x3, [sp, #96]
    mov     sp, x3
    cbz     w8, 2f
    mov     w0, wzr
    bl      svcReturnFromException
3:  b       3b

2:  b       __real___libnx_exception_entry
    .size   __wrap___libnx_exception_entry, . - __wrap___libnx_exception_entry



/* inline_syscall_entry */
/* Preserve Linux's register contract, including all SIMD registers and FP
 * state. A second private trap returns to the original PC without borrowing
 * a guest register for the indirect branch. x0 alone contains the result. */
    .text
    .align 4
    .global ssrnx_inline_syscall_entry
    .type ssrnx_inline_syscall_entry, %function
ssrnx_inline_syscall_entry:
    sub sp, sp, #800
    stp x1, x2, [sp, #0]
    stp x3, x4, [sp, #16]
    stp x5, x6, [sp, #32]
    stp x7, x8, [sp, #48]
    stp x9, x10, [sp, #64]
    stp x11, x12, [sp, #80]
    stp x13, x14, [sp, #96]
    stp x15, x16, [sp, #112]
    stp x17, x18, [sp, #128]
    stp x19, x20, [sp, #144]
    stp x21, x22, [sp, #160]
    stp x23, x24, [sp, #176]
    stp x25, x26, [sp, #192]
    stp x27, x28, [sp, #208]
    stp x29, x30, [sp, #224]
    stp q0, q1, [sp, #240]
    stp q2, q3, [sp, #272]
    stp q4, q5, [sp, #304]
    stp q6, q7, [sp, #336]
    stp q8, q9, [sp, #368]
    stp q10, q11, [sp, #400]
    stp q12, q13, [sp, #432]
    stp q14, q15, [sp, #464]
    stp q16, q17, [sp, #496]
    stp q18, q19, [sp, #528]
    stp q20, q21, [sp, #560]
    stp q22, q23, [sp, #592]
    stp q24, q25, [sp, #624]
    stp q26, q27, [sp, #656]
    stp q28, q29, [sp, #688]
    stp q30, q31, [sp, #720]
    mrs x9, fpcr
    mrs x10, fpsr
    str x9, [sp, #752]
    str x10, [sp, #760]
    mov x6, x5
    mov x5, x4
    mov x4, x3
    mov x3, x2
    mov x2, x1
    mov x1, x0
    mov x0, x8
    ldr x7, [sp, #800]
    sub x7, x7, #4
    bl ssrnx_bionic_raw_syscall
    ldr x9, [sp, #752]
    ldr x10, [sp, #760]
    msr fpcr, x9
    msr fpsr, x10
    ldp q0, q1, [sp, #240]
    ldp q2, q3, [sp, #272]
    ldp q4, q5, [sp, #304]
    ldp q6, q7, [sp, #336]
    ldp q8, q9, [sp, #368]
    ldp q10, q11, [sp, #400]
    ldp q12, q13, [sp, #432]
    ldp q14, q15, [sp, #464]
    ldp q16, q17, [sp, #496]
    ldp q18, q19, [sp, #528]
    ldp q20, q21, [sp, #560]
    ldp q22, q23, [sp, #592]
    ldp q24, q25, [sp, #624]
    ldp q26, q27, [sp, #656]
    ldp q28, q29, [sp, #688]
    ldp q30, q31, [sp, #720]
    ldp x1, x2, [sp, #0]
    ldp x3, x4, [sp, #16]
    ldp x5, x6, [sp, #32]
    ldp x7, x8, [sp, #48]
    ldp x9, x10, [sp, #64]
    ldp x11, x12, [sp, #80]
    ldp x13, x14, [sp, #96]
    ldp x15, x16, [sp, #112]
    ldp x17, x18, [sp, #128]
    ldp x19, x20, [sp, #144]
    ldp x21, x22, [sp, #160]
    ldp x23, x24, [sp, #176]
    ldp x25, x26, [sp, #192]
    ldp x27, x28, [sp, #208]
    ldp x29, x30, [sp, #224]
    add sp, sp, #800
    .global ssrnx_inline_syscall_complete
ssrnx_inline_syscall_complete:
    brk #0xf101
    .size ssrnx_inline_syscall_entry, . - ssrnx_inline_syscall_entry



/* bionic_setjmp */
/* SSRNX Android/Bionic arm64 non-local-jump bridge.
 *
 * The guest buffer is opaque to applications, but it is always at least
 * 32 x 64-bit words on arm64.  Keep the legacy Bionic-compatible core/fp
 * slots and use one reserved slot at byte 176 for x18.  Modern Android uses
 * x18 as a platform register (including shadow-call-stack configurations),
 * while devkitA64/Horizon code is compiled under a different platform ABI.
 * SSRNX must therefore preserve x18 explicitly whenever this bridge calls
 * back into host C and across non-local jumps.
 *
 * SSRNX's guest signal mask is runtime state rather than Horizon state.
 */
.text
.align 2

.global ssrnx_guest__setjmp
.type ssrnx_guest__setjmp, %function
ssrnx_guest__setjmp:
    mov w1, #0
    b ssrnx_guest_sigsetjmp

.global ssrnx_guest_setjmp
.type ssrnx_guest_setjmp, %function
ssrnx_guest_setjmp:
    mov w1, #1
    b ssrnx_guest_sigsetjmp

.global ssrnx_guest_sigsetjmp
.type ssrnx_guest_sigsetjmp, %function
ssrnx_guest_sigsetjmp:
    movz w9, #0x7200
    movk w9, #0x5365, lsl #16
    cbz w1, 1f
    // x18 is Android platform state, but host C is a foreign platform ABI.
    // Preserve it explicitly across the helper call.
    stp x0, x30, [sp, #-32]!
    str x18, [sp, #16]
    bl ssrnx_guest_signal_mask_get32
    mov w1, w0
    ldr x18, [sp, #16]
    ldp x0, x30, [sp], #32
    str w1, [x0, #4]
    movz w9, #0x7201
    movk w9, #0x5365, lsl #16
1:
    str w9, [x0, #0]
    mov x10, sp
    stp x30, x10, [x0, #8]
    stp x28, x29, [x0, #24]
    stp x26, x27, [x0, #40]
    stp x24, x25, [x0, #56]
    stp x22, x23, [x0, #72]
    stp x20, x21, [x0, #88]
    str x19, [x0, #104]
    stp d14, d15, [x0, #112]
    stp d12, d13, [x0, #128]
    stp d10, d11, [x0, #144]
    stp d8, d9, [x0, #160]
    // Reserved jmp_buf space: preserve Android's platform register too.
    str x18, [x0, #176]
    mov w0, wzr
    ret

.global ssrnx_guest_longjmp
.type ssrnx_guest_longjmp, %function
ssrnx_guest_longjmp:
    b ssrnx_guest_siglongjmp

.global ssrnx_guest_siglongjmp
.type ssrnx_guest_siglongjmp, %function
ssrnx_guest_siglongjmp:
    ldr w10, [x0, #0]
    movz w9, #0x7200
    movk w9, #0x5365, lsl #16
    cmp w10, w9
    b.eq 2f
    movz w9, #0x7201
    movk w9, #0x5365, lsl #16
    cmp w10, w9
    b.ne 9f
    // Do not let the host helper ABI destroy Android's x18 while the guest
    // context is suspended.  The final guest x18 is restored from env below.
    stp x0, x1, [sp, #-32]!
    str x18, [sp, #16]
    ldr w0, [x0, #4]
    bl ssrnx_guest_signal_mask_set32
    ldr x18, [sp, #16]
    ldp x0, x1, [sp], #32
2:
    ldp x30, x10, [x0, #8]
    mov sp, x10
    ldp x28, x29, [x0, #24]
    ldp x26, x27, [x0, #40]
    ldp x24, x25, [x0, #56]
    ldp x22, x23, [x0, #72]
    ldp x20, x21, [x0, #88]
    ldr x19, [x0, #104]
    ldp d14, d15, [x0, #112]
    ldp d12, d13, [x0, #128]
    ldp d10, d11, [x0, #144]
    ldp d8, d9, [x0, #160]
    ldr x18, [x0, #176]
    tst x30, #3
    b.ne 9f
    mov x10, sp
    tst x10, #15
    b.ne 9f
    cmp w1, wzr
    csinc w0, w1, wzr, ne
    ret
9:
    brk #0
    b 9b



/* tlsdesc_a64 */
    .text
    .align 2
    .global ssrnx_tlsdesc_static_resolver
    .type ssrnx_tlsdesc_static_resolver, %function
/* AArch64 TLSDESC resolver ABI:
 *   x0 = descriptor address
 *   descriptor[1] = TP-relative byte offset
 *   x0 = result offset
 * Preserve every register except x0/x1/x30 and flags. This resolver only
 * touches x0, which is stricter than the ABI requires. */
ssrnx_tlsdesc_static_resolver:
    ldr x0, [x0, #8]
    ret
    .size ssrnx_tlsdesc_static_resolver, .-ssrnx_tlsdesc_static_resolver

