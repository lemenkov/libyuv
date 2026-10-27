# Gemini Project Context: libyuv Row Functions

This file provides context for the core row-processing architecture of
libyuv. Use these guidelines when refactoring, reviewing, or generating
code within the `row_*.cc` files.

## Architectural Overview

Libyuv uses a dispatch system where high-level conversion functions call
optimized "Row" functions. These functions are categorized by SIMD architecture
and compiler compatibility.

## Source File Map

### x86 Architectures (32-bit and 64-bit)

*   **row_gcc.cc**: **Master copy.** Contains inline assembly in GCC syntax for
    GCC and Clang. Supports AVX, and AVX512. AVX512 implementations are strictly
    for 64-bit targets.
*   **row_win.cc**: Derivative of `row_gcc.cc`. Contains C++ intrinsics
    specifically for Visual C++ (MSVC). Can be tested with Clang using
    `-DLIBYUV_ENABLE_ROWWIN`.
*   **Note**: Use either `row_gcc` or `row_win`, never both.

### Arm Architecture (32-bit)

*   **row_neon.cc**: 32-bit Arm. Written entirely in inline assembly for
    GCC/Clang.

### Arm Architecture (AArch64) (64-bit)

*   **row_neon64.cc**: 64-bit Arm (AArch64) Neon. Written entirely in inline
    assembly for GCC/Clang.
*   **row_sve.cc**: Armv9 Scalable Vector Extension (SVE2).
*   **row_sme.cc**: Armv9 Scalable Matrix Extension (SME) and Streaming SVE
    (SSVE).
*   **row_sve.h**: Streaming-compatible implementations shared across
    **row_sve.cc** and **row_sme.cc**.

### Other Architectures

*   **row_rvv.cc**: RISC-V Vector (RVV). Implemented using intrinsics. Optimized
    for SiFive X280.
*   **row_hvx.cc**: Qualcomm Hexagon Vector Extensions (HVX). Implemented using
    intrinsics.
*   **row_lsx.cc / row_lasx.cc**: Loongarch MIPS-like extensions.

### Utility and Fallbacks

*   **row_common.cc**: Portable C/C++ versions. This is the reference
    implementation.
*   **row_any.cc**: Handles "remainder" pixels for widths not multiples of SIMD
    register size. Used for x86, Neon, and MIPS. Not required for SVE2, SME,
    RVV, or HVX due to hardware-level masking.

## Coding Guidelines

1.  **Feature Macros**: Use the `HAS_` macros in `include/libyuv/row.h` to
    enable or disable specific instruction set extensions.
2.  **Inline asm clobbers (all architectures)**: Every register written by an
    inline asm block must be listed in its clobber list: vector registers,
    general purpose registers, mask/predicate registers, `"cc"` when flags
    are set (`subs`, `cmp`, `whilelt`, ...) and `"memory"` when the block
    stores. Declare them even if the ABI treats them as caller-saved: with
    LTO/LTCG the compiler may keep values live in any register across the
    asm, and other OS ABIs (e.g. Windows) differ in which registers are
    callee-saved. Never remove a clobber because it "is not needed by the
    ABI". Use real register names, not numbers (`"v22"`, not `"22"`). Order
    the list `"memory"` first (when stores occur), then `"cc"` (on
    architectures with flags), then registers.

### x86 Architectures (32-bit and 64-bit)

1.  **AVX512 Logic**: AVX512 row functions are strictly enabled for **64-bit x86
    only**.
2.  **MSVC Intrinsics (`row_win.cc`)**: When converting `row_gcc.cc` to
    `row_win.cc`, use `_mm512_permutex2var_epi8`, not `_mm512_permi2var_epi8`
    intrinsic.
3.  **Clobber names**: Always declare vector clobbers as `"xmmN"`, never
    `"ymmN"` or `"zmmN"`, including for AVX2 and AVX512 code. Clang for
    Windows must preserve xmm6-xmm15 and does not save them when they are
    declared as ymm or zmm.
4.  **MSan Intrinsics (`row_win.cc` / `scale_win.cc`)**: Clang builds with
    MemorySanitizer (`-fsanitize=memory`) define `LIBYUV_ENABLE_ROWWIN` and
    compile `row_win.cc` and `scale_win.cc` instead of inline assembly. Avoid
    AVX-512 intrinsics that LLVM's MSan pass does not instrument:
    *   **Down-converting masked stores**: Never use
        `_mm512_mask_cvtepi32_storeu_epi8`, `_mm512_mask_cvtepi64_storeu_epi8`,
        `_mm512_mask_cvtepi16_storeu_epi8`, or
        `_mm512_mask_cvtepi32_storeu_epi16`. They lower to target-specific
        `@llvm.x86.avx512.mask.pmov.*.mem.*` intrinsics that LLVM MSan does not
        model, leaving the destination shadow memory poisoned (uninitialized).
        Instead, convert to a narrower vector first and use a standard masked
        store (e.g. `_mm_mask_storeu_epi8(dst, mask, _mm512_cvtepi32_epi8(v))`),
        which lowers to `@llvm.masked.store` for MSan and still compiles to a
        single `vpmovdb` instruction at `-O2`.
    *   **Ternary logic (`_mm512_ternarylogic_epi32` / `epi64`) on partially
        uninitialized vectors**: LLVM MSan does not model `vpternlog` truth
        tables bitwise and flags `use-of-uninitialized-value` if any bit of any
        of the three inputs is uninitialized, even if masked off by `imm8`. For
        partial channel updates (e.g. writing alpha into `dst` in
        `ARGBCopyYToAlphaRow_AVX512BW`), use byte-masked stores
        (`_mm512_mask_storeu_epi8`) instead of reading `dst` and blending with
        `_mm512_ternarylogic_epi32`.

### AArch64

1. For SVE2/SME, aim to use full predicates (`ptrue`) and full vectors for the
   main loop body and use predication only to mask the tail loop as needed.
   Pointer updates should be done with `inc[bhwd]` or `add`, not by `incp`.

## Changelist (CL) Format Guidelines

When adding new code, remove trailing spaces.
When generating descriptions, follow the Chromium standard format. Wrap
commit message text at 72 characters

### Changelist (CL) Description Example

\[libyuv] Optimized ARGBToRGB24 for AVX2

Detailed technical explanation of the change. Describe the SIMD
optimization logic and any platform-specific constraints. Mention
if this impacts specific row files like row_gcc.cc.

Test: libyuv_unittest --gunit_filter=*ARGBToRGB24*
Bug: libyuv:12345, b/67890
