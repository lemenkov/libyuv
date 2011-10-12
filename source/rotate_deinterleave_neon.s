  .global RestoreRegisters_NEON
  .global ReverseLine_di_NEON
  .global SaveRegisters_NEON
  .global Transpose_di_wx8_NEON
  .type RestoreRegisters_NEON, function
  .type ReverseLine_di_NEON, function
  .type SaveRegisters_NEON, function
  .type Transpose_di_wx8_NEON, function

@ void SaveRegisters_NEON (unsigned long long store)
@ r0 unsigned long long store
SaveRegisters_NEON:
  vst1.i64    {d8, d9, d10, d11}, [r0]!
  vst1.i64    {d12, d13, d14, d15}, [r0]!
  bx          lr

@ void RestoreRegisters_NEON (unsigned long long store)
@ r0 unsigned long long store
RestoreRegisters_NEON:
  vld1.i64    {d8, d9, d10, d11}, [r0]!
  vld1.i64    {d12, d13, d14, d15}, [r0]!
  bx          lr


@ void ReverseLine_NEON (const uint8* src,
@                        uint8* dst_a,
@                        uint8* dst_b,
@                        int width)
@ r0 const uint8* src
@ r1 uint8* dst_a
@ r2 uint8* dst_b
@ r3 width
ReverseLine_di_NEON:

  @ compute where to start writing destination
  add         r1, r1, r3      @ dst_a + width
  add         r2, r2, r3      @ dst_b + width

  @ work on input segments that are multiples of 16, but
  @ width that has been passed is output segments, half
  @ the size of input.
  lsrs        r12, r3, #3

  beq         .line_residuals

  @ the output is written in to two blocks.
  mov         r12, #-8

  @ back of destination by the size of the register that is
  @ going to be reversed
  sub         r1, r1, #8
  sub         r2, r2, #8

  @ the loop needs to run on blocks of 16.  what will be left
  @ over is either a negative number, the residuals that need
  @ to be done, or 0.  if this isn't subtracted off here the
  @ loop will run one extra time.
  sub         r3, r3, #8

.segments_of_8:
    vld2.8      {d0, d1}, [r0]!         @ src += 16

    @ reverse the bytes in the 64 bit segments
    vrev64.8    q0, q0

    vst1.8      {d0}, [r1], r12         @ dst_a -= 8
    vst1.8      {d1}, [r2], r12         @ dst_b -= 8

    subs        r3, r3, #8
    bge         .segments_of_8

  @ add 16 back to the counter.  if the result is 0 there is no
  @ residuals so return
  adds        r3, r3, #8
  bxeq        lr

  add         r1, r1, #8
  add         r2, r2, #8

.line_residuals:

  mov         r12, #-1

  sub         r1, r1, #1
  sub         r2, r2, #1

@ do this in neon registers as per
@ http://blogs.arm.com/software-enablement/196-coding-for-neon-part-2-dealing-with-leftovers/
.segments_of_2:
    vld2.8      {d0[0], d1[0]}, [r0]!     @ src += 2

    vst1.8      {d0[0]}, [r1], r12        @ dst_a -= 1
    vst1.8      {d1[0]}, [r2], r12        @ dst_b -= 1

    subs        r3, r3, #1
    bgt         .segments_of_2

  bx          lr

@ void Transpose_di_wx8_NEON (const uint8* src, int src_pitch,
@                             uint8* dst_a, int dst_pitch_a,
@                             uint8* dst_b, int dst_pitch_b,
@                             int width)
@ r0 const uint8* src
@ r1 int src_pitch
@ r2 uint8* dst_a
@ r3 int dst_pitch_a
@ stack uint8* dst_b
@ stack int dst_pitch_b
@ stack int width
Transpose_di_wx8_NEON:
  push        {r4-r9,lr}

  ldr         r4, [sp, #28]         @ dst_b
  ldr         r5, [sp, #32]         @ dst_pitch_b
  ldr         r7, [sp, #36]         @ width
  @ loops are on blocks of 8.  loop will stop when
  @ counter gets to or below 0.  starting the counter
  @ at w-8 allow for this
  sub         r8, #8

@ handle 8x8 blocks.  this should be the majority of the plane
.loop_8x8:
    mov         r9, r0

    vld2.8      {d0,  d1},  [r9], r1
    vld2.8      {d2,  d3},  [r9], r1
    vld2.8      {d4,  d5},  [r9], r1
    vld2.8      {d6,  d7},  [r9], r1
    vld2.8      {d8,  d9},  [r9], r1
    vld2.8      {d10, d11}, [r9], r1
    vld2.8      {d12, d13}, [r9], r1
    vld2.8      {d14, d15}, [r9]

    vtrn.8      q1, q0
    vtrn.8      q3, q2
    vtrn.8      q5, q4
    vtrn.8      q7, q6

    vtrn.16     q1, q3
    vtrn.16     q0, q2
    vtrn.16     q5, q7
    vtrn.16     q4, q6

    vtrn.32     q1, q5
    vtrn.32     q0, q4
    vtrn.32     q3, q7
    vtrn.32     q2, q6

    vrev16.8    q0, q0
    vrev16.8    q1, q1
    vrev16.8    q2, q2
    vrev16.8    q3, q3
    vrev16.8    q4, q4
    vrev16.8    q5, q5
    vrev16.8    q6, q6
    vrev16.8    q7, q7

    mov         r9, r2

    vst1.8      {d2},  [r9], r3
    vst1.8      {d0},  [r9], r3
    vst1.8      {d6},  [r9], r3
    vst1.8      {d4},  [r9], r3
    vst1.8      {d10}, [r9], r3
    vst1.8      {d8},  [r9], r3
    vst1.8      {d14}, [r9], r3
    vst1.8      {d12}, [r9]

    mov         r9, r4

    vst1.8      {d3},  [r9], r5
    vst1.8      {d1},  [r9], r5
    vst1.8      {d7},  [r9], r5
    vst1.8      {d5},  [r9], r5
    vst1.8      {d11}, [r9], r5
    vst1.8      {d9},  [r9], r5
    vst1.8      {d15}, [r9], r5
    vst1.8      {d13}, [r9]

    add         r0, #8*2          @ src   += 8*2
    add         r2, r3, lsl #3    @ dst_a += 8 * dst_pitch_a
    add         r4, r5, lsl #3    @ dst_b += 8 * dst_pitch_b
    subs        r8,  #8           @ w     -= 8
    bge         .loop_8x8

  @ add 8 back to counter.  if the result is 0 there are
  @ no residuals.
  adds        r8, #8
  beq         .done

  @ some residual, so between 1 and 7 lines left to transpose
  cmp         r8, #2
  blt         .block_1x8

  cmp         r8, #4
  blt         .block_2x8

@ TODO(frkoenig) : clean this up
.block_4x8:
  mov         r9, r0
  vld1.64     {d0}, [r9], r1
  vld1.64     {d1}, [r9], r1
  vld1.64     {d2}, [r9], r1
  vld1.64     {d3}, [r9], r1
  vld1.64     {d4}, [r9], r1
  vld1.64     {d5}, [r9], r1
  vld1.64     {d6}, [r9], r1
  vld1.64     {d7}, [r9]

  adr         r12, vtbl_4x4_transpose
  vld1.8      {q7}, [r12]

  vtrn.8      q0, q1
  vtrn.8      q2, q3

  vtbl.8      d8,  {d0, d1}, d14
  vtbl.8      d9,  {d0, d1}, d15
  vtbl.8      d10, {d2, d3}, d14
  vtbl.8      d11, {d2, d3}, d15
  vtbl.8      d12, {d4, d5}, d14
  vtbl.8      d13, {d4, d5}, d15
  vtbl.8      d0,  {d6, d7}, d14
  vtbl.8      d1,  {d6, d7}, d15

  mov         r9, r2

  vst1.32     {d8[0]},  [r9], r3
  vst1.32     {d8[1]},  [r9], r3
  vst1.32     {d9[0]},  [r9], r3
  vst1.32     {d9[1]},  [r9], r3

  add         r9, r2, #4
  vst1.32     {d12[0]}, [r9], r3
  vst1.32     {d12[1]}, [r9], r3
  vst1.32     {d13[0]}, [r9], r3
  vst1.32     {d13[1]}, [r9]

  mov         r9, r4

  vst1.32     {d10[0]}, [r9], r5
  vst1.32     {d10[1]}, [r9], r5
  vst1.32     {d11[0]}, [r9], r5
  vst1.32     {d11[1]}, [r9], r5

  add         r9, r4, #4
  vst1.32     {d0[0]},  [r9], r5
  vst1.32     {d0[1]},  [r9], r5
  vst1.32     {d1[0]},  [r9], r5
  vst1.32     {d1[1]},  [r9]

  add         r0, #4*2          @ src   += 4 * 2
  add         r2, r3, lsl #2    @ dst_a += 4 * dst_pitch_a
  add         r4, r5, lsl #2    @ dst_b += 4 * dst_pitch_b
  subs        r8,  #4           @ w     -= 4
  beq         .done

  @ some residual, check to see if it includes a 2x8 block,
  @ or less
  cmp         r8, #2
  blt         .block_1x8

.block_2x8:
  mov         r9, r0
  vld2.16     {d0[0], d2[0]}, [r9], r1
  vld2.16     {d1[0], d3[0]}, [r9], r1
  vld2.16     {d0[1], d2[1]}, [r9], r1
  vld2.16     {d1[1], d3[1]}, [r9], r1
  vld2.16     {d0[2], d2[2]}, [r9], r1
  vld2.16     {d1[2], d3[2]}, [r9], r1
  vld2.16     {d0[3], d2[3]}, [r9], r1
  vld2.16     {d1[3], d3[3]}, [r9]

  vtrn.8      d0, d1
  vtrn.8      d2, d3

  mov         r9, r2

  vst1.64     {d0}, [r9], r3
  vst1.64     {d2}, [r9]

  mov         r9, r4

  vst1.64     {d1}, [r9], r5
  vst1.64     {d3}, [r9]

  add         r0, #2*2          @ src   += 2 * 2
  add         r2, r3, lsl #1    @ dst_a += 2 * dst_pitch_a
  add         r4, r5, lsl #1    @ dst_a += 2 * dst_pitch_a
  subs        r8,  #2           @ w     -= 2
  beq         .done

.block_1x8:
  vld2.8      {d0[0], d1[0]}, [r0], r1
  vld2.8      {d0[1], d1[1]}, [r0], r1
  vld2.8      {d0[2], d1[2]}, [r0], r1
  vld2.8      {d0[3], d1[3]}, [r0], r1
  vld2.8      {d0[4], d1[4]}, [r0], r1
  vld2.8      {d0[5], d1[5]}, [r0], r1
  vld2.8      {d0[6], d1[6]}, [r0], r1
  vld2.8      {d0[7], d1[7]}, [r0]

  vst1.64     {d0}, [r2]
  vst1.64     {d1}, [r4]

.done:
  pop         {r4-r9, pc}

vtbl_4x4_transpose:
  .byte  0,  8,  1,  9,  2, 10,  3, 11,  4, 12,  5, 13,  6, 14,  7, 15
