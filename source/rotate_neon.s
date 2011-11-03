  .global ReverseLine_NEON
  .global ReverseLineUV_NEON
  .global TransposeWx8_NEON
  .global TransposeUVWx8_NEON
  .type ReverseLine_NEON, function
  .type ReverseLineUV_NEON, function
  .type TransposeWx8_NEON, function
  .type TransposeUVWx8_NEON, function

@ void ReverseLine_NEON (const uint8* src, uint8* dst, int width)
@ r0 const uint8* src
@ r1 uint8* dst
@ r2 width
ReverseLine_NEON:

  @ compute where to start writing destination
  add         r1, r2      @ dst + width

  @ work on segments that are multiples of 16
  lsrs        r3, r2, #4

  @ the output is written in two block.  8 bytes followed
  @ by another 8.  reading is done sequentially, from left to
  @ right.  writing is done from right to left in block sizes
  @ r1, the destination pointer is incremented after writing
  @ the first of the two blocks.  need to subtract that 8 off
  @ along with 16 to get the next location.
  mov         r3, #-24

  beq         Lline_residuals

  @ back of destination by the size of the register that is
  @ going to be reversed
  sub         r1, #16

  @ the loop needs to run on blocks of 16.  what will be left
  @ over is either a negative number, the residuals that need
  @ to be done, or 0.  if this isn't subtracted off here the
  @ loop will run one extra time.
  sub         r2, #16

Lsegments_of_16:
    vld1.8      {q0}, [r0]!               @ src += 16

    @ reverse the bytes in the 64 bit segments.  unable to reverse
    @ the bytes in the entire 128 bits in one go.
    vrev64.8    q0, q0

    @ because of the inability to reverse the entire 128 bits
    @ reverse the writing out of the two 64 bit segments.
    vst1.8      {d1}, [r1]!
    vst1.8      {d0}, [r1], r3            @ dst -= 16

    subs        r2, #16
    bge         Lsegments_of_16

  @ add 16 back to the counter.  if the result is 0 there is no
  @ residuals so return
  adds        r2, #16
  bxeq        lr

  add         r1, #16

Lline_residuals:

  mov         r3, #-3

  sub         r1, #2
  subs        r2, #2
  @ check for 16*n+1 scenarios where segments_of_2 should not
  @ be run, but there is something left over.
  blt         Lsegment_of_1

@ do this in neon registers as per
@ http://blogs.arm.com/software-enablement/196-coding-for-neon-part-2-dealing-with-leftovers/
Lsegments_of_2:
    vld2.8      {d0[0], d1[0]}, [r0]!     @ src += 2

    vst1.8      {d1[0]}, [r1]!
    vst1.8      {d0[0]}, [r1], r3         @ dst -= 2

    subs        r2, #2
    bge         Lsegments_of_2

  adds        r2, #2
  bxeq        lr

Lsegment_of_1:
  add         r1, #1
  vld1.8      {d0[0]}, [r0]
  vst1.8      {d0[0]}, [r1]

  bx          lr

@ void TransposeWx8_NEON (const uint8* src, int src_stride,
@                         uint8* dst, int dst_stride,
@                         int w)
@ r0 const uint8* src
@ r1 int src_stride
@ r2 uint8* dst
@ r3 int dst_stride
@ stack int w
TransposeWx8_NEON:
  push        {r4,r8,r9,lr}

  ldr         r8, [sp, #16]        @ width

  @ loops are on blocks of 8.  loop will stop when
  @ counter gets to or below 0.  starting the counter
  @ at w-8 allow for this
  sub         r8, #8

@ handle 8x8 blocks.  this should be the majority of the plane
Lloop_8x8:
    mov         r9, r0

    vld1.8      {d0}, [r9], r1
    vld1.8      {d1}, [r9], r1
    vld1.8      {d2}, [r9], r1
    vld1.8      {d3}, [r9], r1
    vld1.8      {d4}, [r9], r1
    vld1.8      {d5}, [r9], r1
    vld1.8      {d6}, [r9], r1
    vld1.8      {d7}, [r9]

    vtrn.8      d1, d0
    vtrn.8      d3, d2
    vtrn.8      d5, d4
    vtrn.8      d7, d6

    vtrn.16     d1, d3
    vtrn.16     d0, d2
    vtrn.16     d5, d7
    vtrn.16     d4, d6

    vtrn.32     d1, d5
    vtrn.32     d0, d4
    vtrn.32     d3, d7
    vtrn.32     d2, d6

    vrev16.8    q0, q0
    vrev16.8    q1, q1
    vrev16.8    q2, q2
    vrev16.8    q3, q3

    mov         r9, r2

    vst1.8      {d1}, [r9], r3
    vst1.8      {d0}, [r9], r3
    vst1.8      {d3}, [r9], r3
    vst1.8      {d2}, [r9], r3
    vst1.8      {d5}, [r9], r3
    vst1.8      {d4}, [r9], r3
    vst1.8      {d7}, [r9], r3
    vst1.8      {d6}, [r9]

    add         r0, #8            @ src += 8
    add         r2, r3, lsl #3    @ dst += 8 * dst_stride
    subs        r8,  #8           @ w   -= 8
    bge         Lloop_8x8

  @ add 8 back to counter.  if the result is 0 there are
  @ no residuals.
  adds        r8, #8
  beq         Ldone

  @ some residual, so between 1 and 7 lines left to transpose
  cmp         r8, #2
  blt         Lblock_1x8

  cmp         r8, #4
  blt         Lblock_2x8

Lblock_4x8:
  mov         r9, r0
  vld1.32     {d0[0]}, [r9], r1
  vld1.32     {d0[1]}, [r9], r1
  vld1.32     {d1[0]}, [r9], r1
  vld1.32     {d1[1]}, [r9], r1
  vld1.32     {d2[0]}, [r9], r1
  vld1.32     {d2[1]}, [r9], r1
  vld1.32     {d3[0]}, [r9], r1
  vld1.32     {d3[1]}, [r9]

  mov         r9, r2

  adr         r12, vtbl_4x4_transpose
  vld1.8      {q3}, [r12]

  vtbl.8      d4, {d0, d1}, d6
  vtbl.8      d5, {d0, d1}, d7
  vtbl.8      d0, {d2, d3}, d6
  vtbl.8      d1, {d2, d3}, d7

  @ TODO: rework shuffle above to write
  @       out with 4 instead of 8 writes
  vst1.32     {d4[0]}, [r9], r3
  vst1.32     {d4[1]}, [r9], r3
  vst1.32     {d5[0]}, [r9], r3
  vst1.32     {d5[1]}, [r9]

  add         r9, r2, #4
  vst1.32     {d0[0]}, [r9], r3
  vst1.32     {d0[1]}, [r9], r3
  vst1.32     {d1[0]}, [r9], r3
  vst1.32     {d1[1]}, [r9]

  add         r0, #4            @ src += 4
  add         r2, r3, lsl #2    @ dst += 4 * dst_stride
  subs        r8,  #4           @ w   -= 4
  beq         Ldone

  @ some residual, check to see if it includes a 2x8 block,
  @ or less
  cmp         r8, #2
  blt         Lblock_1x8

Lblock_2x8:
  mov         r9, r0
  vld1.16     {d0[0]}, [r9], r1
  vld1.16     {d1[0]}, [r9], r1
  vld1.16     {d0[1]}, [r9], r1
  vld1.16     {d1[1]}, [r9], r1
  vld1.16     {d0[2]}, [r9], r1
  vld1.16     {d1[2]}, [r9], r1
  vld1.16     {d0[3]}, [r9], r1
  vld1.16     {d1[3]}, [r9]

  vtrn.8      d0, d1

  mov         r9, r2

  vst1.64     {d0}, [r9], r3
  vst1.64     {d1}, [r9]

  add         r0, #2            @ src += 2
  add         r2, r3, lsl #1    @ dst += 2 * dst_stride
  subs        r8,  #2           @ w   -= 2
  beq         Ldone

Lblock_1x8:
  vld1.8      {d0[0]}, [r0], r1
  vld1.8      {d0[1]}, [r0], r1
  vld1.8      {d0[2]}, [r0], r1
  vld1.8      {d0[3]}, [r0], r1
  vld1.8      {d0[4]}, [r0], r1
  vld1.8      {d0[5]}, [r0], r1
  vld1.8      {d0[6]}, [r0], r1
  vld1.8      {d0[7]}, [r0]

  vst1.64     {d0}, [r2]

Ldone:

  pop         {r4,r8,r9,pc}

vtbl_4x4_transpose:
  .byte  0,  4,  8, 12,  1,  5,  9, 13,  2,  6, 10, 14,  3,  7, 11, 15

@ void ReverseLineUV_NEON (const uint8* src,
@                          uint8* dst_a,
@                          uint8* dst_b,
@                          int width)
@ r0 const uint8* src
@ r1 uint8* dst_a
@ r2 uint8* dst_b
@ r3 width
ReverseLineUV_NEON:

  @ compute where to start writing destination
  add         r1, r1, r3      @ dst_a + width
  add         r2, r2, r3      @ dst_b + width

  @ work on input segments that are multiples of 16, but
  @ width that has been passed is output segments, half
  @ the size of input.
  lsrs        r12, r3, #3

  beq         Lline_residuals_di

  @ the output is written in to two blocks.
  mov         r12, #-8

  @ back of destination by the size of the register that is
  @ going to be reversed
  sub         r1, r1, #8
  sub         r2, r2, #8

  @ the loop needs to run on blocks of 8.  what will be left
  @ over is either a negative number, the residuals that need
  @ to be done, or 0.  if this isn't subtracted off here the
  @ loop will run one extra time.
  sub         r3, r3, #8

Lsegments_of_8_di:
    vld2.8      {d0, d1}, [r0]!         @ src += 16

    @ reverse the bytes in the 64 bit segments
    vrev64.8    q0, q0

    vst1.8      {d0}, [r1], r12         @ dst_a -= 8
    vst1.8      {d1}, [r2], r12         @ dst_b -= 8

    subs        r3, r3, #8
    bge         Lsegments_of_8_di

  @ add 8 back to the counter.  if the result is 0 there is no
  @ residuals so return
  adds        r3, r3, #8
  bxeq        lr

  add         r1, r1, #8
  add         r2, r2, #8

Lline_residuals_di:

  mov         r12, #-1

  sub         r1, r1, #1
  sub         r2, r2, #1

@ do this in neon registers as per
@ http://blogs.arm.com/software-enablement/196-coding-for-neon-part-2-dealing-with-leftovers/
Lsegments_of_1:
    vld2.8      {d0[0], d1[0]}, [r0]!     @ src += 2

    vst1.8      {d0[0]}, [r1], r12        @ dst_a -= 1
    vst1.8      {d1[0]}, [r2], r12        @ dst_b -= 1

    subs        r3, r3, #1
    bgt         Lsegments_of_1

  bx          lr

@ void TransposeUVWx8_NEON (const uint8* src, int src_stride,
@                           uint8* dst_a, int dst_stride_a,
@                           uint8* dst_b, int dst_stride_b,
@                           int width)
@ r0 const uint8* src
@ r1 int src_stride
@ r2 uint8* dst_a
@ r3 int dst_stride_a
@ stack uint8* dst_b
@ stack int dst_stride_b
@ stack int width
TransposeUVWx8_NEON:
  push        {r4-r9,lr}

  ldr         r4, [sp, #28]         @ dst_b
  ldr         r5, [sp, #32]         @ dst_stride_b
  ldr         r8, [sp, #36]         @ width
  @ loops are on blocks of 8.  loop will stop when
  @ counter gets to or below 0.  starting the counter
  @ at w-8 allow for this
  sub         r8, #8

@ handle 8x8 blocks.  this should be the majority of the plane
Lloop_8x8_di:
    mov         r9, r0

    vld2.8      {d0,  d1},  [r9], r1
    vld2.8      {d2,  d3},  [r9], r1
    vld2.8      {d4,  d5},  [r9], r1
    vld2.8      {d6,  d7},  [r9], r1
    vld2.8      {d16, d17}, [r9], r1
    vld2.8      {d18, d19}, [r9], r1
    vld2.8      {d20, d21}, [r9], r1
    vld2.8      {d22, d23}, [r9]

    vtrn.8      q1, q0
    vtrn.8      q3, q2
    vtrn.8      q9, q8
    vtrn.8      q11, q10

    vtrn.16     q1, q3
    vtrn.16     q0, q2
    vtrn.16     q9, q11
    vtrn.16     q8, q10

    vtrn.32     q1, q9
    vtrn.32     q0, q8
    vtrn.32     q3, q11
    vtrn.32     q2, q10

    vrev16.8    q0, q0
    vrev16.8    q1, q1
    vrev16.8    q2, q2
    vrev16.8    q3, q3
    vrev16.8    q8, q8
    vrev16.8    q9, q9
    vrev16.8    q10, q10
    vrev16.8    q11, q11

    mov         r9, r2

    vst1.8      {d2},  [r9], r3
    vst1.8      {d0},  [r9], r3
    vst1.8      {d6},  [r9], r3
    vst1.8      {d4},  [r9], r3
    vst1.8      {d18}, [r9], r3
    vst1.8      {d16}, [r9], r3
    vst1.8      {d22}, [r9], r3
    vst1.8      {d20}, [r9]

    mov         r9, r4

    vst1.8      {d3},  [r9], r5
    vst1.8      {d1},  [r9], r5
    vst1.8      {d7},  [r9], r5
    vst1.8      {d5},  [r9], r5
    vst1.8      {d19}, [r9], r5
    vst1.8      {d17}, [r9], r5
    vst1.8      {d23}, [r9], r5
    vst1.8      {d21}, [r9]

    add         r0, #8*2          @ src   += 8*2
    add         r2, r3, lsl #3    @ dst_a += 8 * dst_stride_a
    add         r4, r5, lsl #3    @ dst_b += 8 * dst_stride_b
    subs        r8,  #8           @ w     -= 8
    bge         Lloop_8x8_di

  @ add 8 back to counter.  if the result is 0 there are
  @ no residuals.
  adds        r8, #8
  beq         Ldone_di

  @ some residual, so between 1 and 7 lines left to transpose
  cmp         r8, #2
  blt         Lblock_1x8_di

  cmp         r8, #4
  blt         Lblock_2x8_di

@ TODO(frkoenig) : clean this up
Lblock_4x8_di:
  mov         r9, r0
  vld1.64     {d0}, [r9], r1
  vld1.64     {d1}, [r9], r1
  vld1.64     {d2}, [r9], r1
  vld1.64     {d3}, [r9], r1
  vld1.64     {d4}, [r9], r1
  vld1.64     {d5}, [r9], r1
  vld1.64     {d6}, [r9], r1
  vld1.64     {d7}, [r9]

  adr         r12, vtbl_4x4_transpose_di
  vld1.8      {q15}, [r12]

  vtrn.8      q0, q1
  vtrn.8      q2, q3

  vtbl.8      d16, {d0, d1}, d30
  vtbl.8      d17, {d0, d1}, d31
  vtbl.8      d18, {d2, d3}, d30
  vtbl.8      d19, {d2, d3}, d31
  vtbl.8      d20, {d4, d5}, d30
  vtbl.8      d21, {d4, d5}, d31
  vtbl.8      d22, {d6, d7}, d30
  vtbl.8      d23, {d6, d7}, d31

  mov         r9, r2

  vst1.32     {d16[0]},  [r9], r3
  vst1.32     {d16[1]},  [r9], r3
  vst1.32     {d17[0]},  [r9], r3
  vst1.32     {d17[1]},  [r9], r3

  add         r9, r2, #4
  vst1.32     {d20[0]}, [r9], r3
  vst1.32     {d20[1]}, [r9], r3
  vst1.32     {d21[0]}, [r9], r3
  vst1.32     {d21[1]}, [r9]

  mov         r9, r4

  vst1.32     {d18[0]}, [r9], r5
  vst1.32     {d18[1]}, [r9], r5
  vst1.32     {d19[0]}, [r9], r5
  vst1.32     {d19[1]}, [r9], r5

  add         r9, r4, #4
  vst1.32     {d22[0]},  [r9], r5
  vst1.32     {d22[1]},  [r9], r5
  vst1.32     {d23[0]},  [r9], r5
  vst1.32     {d23[1]},  [r9]

  add         r0, #4*2          @ src   += 4 * 2
  add         r2, r3, lsl #2    @ dst_a += 4 * dst_stride_a
  add         r4, r5, lsl #2    @ dst_b += 4 * dst_stride_b
  subs        r8,  #4           @ w     -= 4
  beq         Ldone_di

  @ some residual, check to see if it includes a 2x8 block,
  @ or less
  cmp         r8, #2
  blt         Lblock_1x8_di

Lblock_2x8_di:
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
  add         r2, r3, lsl #1    @ dst_a += 2 * dst_stride_a
  add         r4, r5, lsl #1    @ dst_a += 2 * dst_stride_a
  subs        r8,  #2           @ w     -= 2
  beq         Ldone_di

Lblock_1x8_di:
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

Ldone_di:
  pop         {r4-r9, pc}

vtbl_4x4_transpose_di:
  .byte  0,  8,  1,  9,  2, 10,  3, 11,  4, 12,  5, 13,  6, 14,  7, 15
