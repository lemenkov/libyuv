#if defined (__mips__)
#
#  Copyright (c) 2012 The LibYuv project authors. All Rights Reserved.
#
#  Use of this source code is governed by a BSD-style license
#  that can be found in the LICENSE file in the root of the source
#  tree. An additional intellectual property rights grant can be found
#  in the file PATENTS.  All contributing project authors may
#  be found in the AUTHORS file in the root of the source tree.
#
#if defined(__linux__) && defined(__ELF__)
.section .note.GNU-stack,"",%progbits
#endif
  .text

  .globl  memcpy_MIPS;
  .align  2;
  .type   memcpy_MIPS,@function;
  .ent    memcpy_MIPS,0;
memcpy_MIPS:
  .frame  $sp,0,$ra
  .set    noreorder
  .set    noat

  slti    $at,$a2,8
  bne     $at,$zero,last8
   move   $v0,$a0 # memcpy returns the dst pointer

# Test if the src and dst are word-aligned, or can be made word-aligned
  xor     $t8,$a1,$a0
  andi    $t8,$t8,0x3   # t8 is a0/a1 word-displacement

  bne     $t8,$zero,unaligned
  negu    $a3,$a0

  andi    $a3,$a3,0x3 # we need to copy a3 bytes to make a0/a1 aligned
  beq     $a3,$zero,chk16w  # when a3=0 then the dst (a0) is
   subu   $a2,$a2,$a3 # word-aligned now a2 is the remining bytes count

  lwr     $t8,0($a1)
  addu    $a1,$a1,$a3
  swr     $t8,0($a0)
  addu    $a0,$a0,$a3

# Now the dst/src are mutually word-aligned with word-aligned addresses
chk16w:
  andi    $t8,$a2,0x3f  # any whole 64-byte chunks?
                        # t8 is the byte count after 64-byte chunks
  beq     $a2,$t8,chk8w # if a2==t8, no 64-byte chunks
                        # There will be at most 1 32-byte chunk after it
   subu   $a3,$a2,$t8 # subtract from a2 the reminder
                      # Here a3 counts bytes in 16w chunks
  addu    $a3,$a0,$a3 # Now a3 is the final dst after 64-byte chunks
  addu    $t0,$a0,$a2 # t0 is the "past the end" address

# When in the loop we exercise "pref 30,x(a0)", the a0+x should not be past
# the "t0-32" address
# This means: for x=128 the last "safe" a0 address is "t0-160"
# Alternatively, for x=64 the last "safe" a0 address is "t0-96"
# In the current version we will use "pref 30,128(a0)", so "t0-160" is the limit
  subu    $t9,$t0,160 # t9 is the "last safe pref 30,128(a0)" address

  pref    0,0($a1)    # bring the first line of src, addr 0
  pref    0,32($a1) # bring the second line of src, addr 32
  pref    0,64($a1) # bring the third line of src, addr 64
  pref    30,32($a0)  # safe, as we have at least 64 bytes ahead
# In case the a0 > t9 don't use "pref 30" at all
  sgtu    $v1,$a0,$t9
  bgtz    $v1,loop16w # skip "pref 30,64(a0)" for too short arrays
   nop
# otherwise, start with using pref30
  pref    30,64($a0)
loop16w:
  pref    0,96($a1)
  lw      $t0,0($a1)
  bgtz    $v1,skip_pref30_96  # skip "pref 30,96(a0)"
   lw     $t1,4($a1)
  pref    30,96($a0)   # continue setting up the dest, addr 96
skip_pref30_96:
  lw      $t2,8($a1)
  lw      $t3,12($a1)
  lw      $t4,16($a1)
  lw      $t5,20($a1)
  lw      $t6,24($a1)
  lw      $t7,28($a1)
  pref    0,128($a1)    # bring the next lines of src, addr 128

  sw      $t0,0($a0)
  sw      $t1,4($a0)
  sw      $t2,8($a0)
  sw      $t3,12($a0)
  sw      $t4,16($a0)
  sw      $t5,20($a0)
  sw      $t6,24($a0)
  sw      $t7,28($a0)

  lw      $t0,32($a1)
  bgtz    $v1,skip_pref30_128 # skip "pref 30,128(a0)"
  lw      $t1,36($a1)
  pref    30,128($a0)   # continue setting up the dest, addr 128
skip_pref30_128:
  lw      $t2,40($a1)
  lw      $t3,44($a1)
  lw      $t4,48($a1)
  lw      $t5,52($a1)
  lw      $t6,56($a1)
  lw      $t7,60($a1)
  pref    0, 160($a1)    # bring the next lines of src, addr 160

  sw      $t0,32($a0)
  sw      $t1,36($a0)
  sw      $t2,40($a0)
  sw      $t3,44($a0)
  sw      $t4,48($a0)
  sw      $t5,52($a0)
  sw      $t6,56($a0)
  sw      $t7,60($a0)

  addiu   $a0,$a0,64  # adding 64 to dest
  sgtu    $v1,$a0,$t9
  bne     $a0,$a3,loop16w
   addiu  $a1,$a1,64  # adding 64 to src
  move    $a2,$t8

# Here we have src and dest word-aligned but less than 64-bytes to go

chk8w:
  pref 0, 0x0($a1)
  andi    $t8,$a2,0x1f  # is there a 32-byte chunk?
                        # the t8 is the reminder count past 32-bytes
  beq     $a2,$t8,chk1w # when a2=t8, no 32-byte chunk
   nop

  lw      $t0,0($a1)
  lw      $t1,4($a1)
  lw      $t2,8($a1)
  lw      $t3,12($a1)
  lw      $t4,16($a1)
  lw      $t5,20($a1)
  lw      $t6,24($a1)
  lw      $t7,28($a1)
  addiu   $a1,$a1,32

  sw      $t0,0($a0)
  sw      $t1,4($a0)
  sw      $t2,8($a0)
  sw      $t3,12($a0)
  sw      $t4,16($a0)
  sw      $t5,20($a0)
  sw      $t6,24($a0)
  sw      $t7,28($a0)
  addiu   $a0,$a0,32

chk1w:
  andi    $a2,$t8,0x3 # now a2 is the reminder past 1w chunks
  beq     $a2,$t8,last8
   subu   $a3,$t8,$a2 # a3 is count of bytes in 1w chunks
  addu    $a3,$a0,$a3 # now a3 is the dst address past the 1w chunks

# copying in words (4-byte chunks)
wordCopy_loop:
  lw      $t3,0($a1)  # the first t3 may be equal t0 ... optimize?
  addiu   $a1,$a1,4
  addiu   $a0,$a0,4
  bne     $a0,$a3,wordCopy_loop
   sw     $t3,-4($a0)

# For the last (<8) bytes
last8:
  blez    $a2,leave
   addu   $a3,$a0,$a2 # a3 is the last dst address
last8loop:
  lb      $v1,0($a1)
  addiu   $a1,$a1,1
  addiu   $a0,$a0,1
  bne     $a0,$a3,last8loop
   sb     $v1,-1($a0)

leave:
  j       $ra
   nop

#
# UNALIGNED case
#

unaligned:
  # got here with a3="negu a0"
  andi    $a3,$a3,0x3 # test if the a0 is word aligned
  beqz    $a3,ua_chk16w
   subu   $a2,$a2,$a3 # bytes left after initial a3 bytes

  lwr     $v1,0($a1)
  lwl     $v1,3($a1)
  addu    $a1,$a1,$a3 # a3 may be here 1, 2 or 3
  swr     $v1,0($a0)
  addu    $a0,$a0,$a3 # below the dst will be word aligned (NOTE1)

ua_chk16w:
  andi    $t8,$a2,0x3f  # any whole 64-byte chunks?
                        # t8 is the byte count after 64-byte chunks
  beq     $a2,$t8,ua_chk8w  # if a2==t8, no 64-byte chunks
  # There will be at most 1 32-byte chunk after it
  subu    $a3,$a2,$t8 # subtract from a2 the reminder
                      # Here a3 counts bytes in 16w chunks
  addu    $a3,$a0,$a3 # Now a3 is the final dst after 64-byte chunks
  addu    $t0,$a0,$a2 # t0 is the "past the end" address
  subu    $t9,$t0,160 # t9 is the "last safe pref 30,128(a0)" address
  pref    0,0($a1)    # bring the first line of src, addr 0
  pref    0,32($a1) # bring the second line of src, addr 32
  pref    0,64($a1) # bring the third line of src, addr 64
  pref    30,32($a0)  # safe, as we have at least 64 bytes ahead
# In case the a0 > t9 don't use "pref 30" at all
  sgtu    $v1,$a0,$t9
  bgtz    $v1,ua_loop16w  # skip "pref 30,64(a0)" for too short arrays
   nop
# otherwise, start with using pref30
  pref    30,64($a0)
ua_loop16w:
  pref    0,96($a1)
  lwr     $t0,0($a1)
  lwl     $t0,3($a1)
  lwr     $t1,4($a1)
  bgtz    $v1,ua_skip_pref30_96
   lwl    $t1,7($a1)
  pref    30,96($a0)   # continue setting up the dest, addr 96
ua_skip_pref30_96:
  lwr     $t2,8($a1)
  lwl     $t2,11($a1)
  lwr     $t3,12($a1)
  lwl     $t3,15($a1)
  lwr     $t4,16($a1)
  lwl     $t4,19($a1)
  lwr     $t5,20($a1)
  lwl     $t5,23($a1)
  lwr     $t6,24($a1)
  lwl     $t6,27($a1)
  lwr     $t7,28($a1)
  lwl     $t7,31($a1)
  pref    0,128($a1)    # bring the next lines of src, addr 128

  sw      $t0,0($a0)
  sw      $t1,4($a0)
  sw      $t2,8($a0)
  sw      $t3,12($a0)
  sw      $t4,16($a0)
  sw      $t5,20($a0)
  sw      $t6,24($a0)
  sw      $t7,28($a0)

  lwr     $t0,32($a1)
  lwl     $t0,35($a1)
  lwr     $t1,36($a1)
  bgtz    $v1,ua_skip_pref30_128
   lwl    $t1,39($a1)
  pref    30,128($a0)   # continue setting up the dest, addr 128
ua_skip_pref30_128:
  lwr     $t2,40($a1)
  lwl     $t2,43($a1)
  lwr     $t3,44($a1)
  lwl     $t3,47($a1)
  lwr     $t4,48($a1)
  lwl     $t4,51($a1)
  lwr     $t5,52($a1)
  lwl     $t5,55($a1)
  lwr     $t6,56($a1)
  lwl     $t6,59($a1)
  lwr     $t7,60($a1)
  lwl     $t7,63($a1)
  pref    0, 160($a1)    # bring the next lines of src, addr 160

  sw      $t0,32($a0)
  sw      $t1,36($a0)
  sw      $t2,40($a0)
  sw      $t3,44($a0)
  sw      $t4,48($a0)
  sw      $t5,52($a0)
  sw      $t6,56($a0)
  sw      $t7,60($a0)

  addiu   $a0,$a0,64  # adding 64 to dest
  sgtu    $v1,$a0,$t9
  bne     $a0,$a3,ua_loop16w
   addiu  $a1,$a1,64  # adding 64 to src
  move    $a2,$t8

# Here we have src and dest word-aligned but less than 64-bytes to go

ua_chk8w:
  pref    0, 0x0($a1)
  andi    $t8,$a2,0x1f  # is there a 32-byte chunk?
                        # the t8 is the reminder count
  beq     $a2,$t8,ua_chk1w  # when a2=t8, no 32-byte chunk

   lwr    $t0,0($a1)
  lwl     $t0,3($a1)
  lwr     $t1,4($a1)
  lwl     $t1,7($a1)
  lwr     $t2,8($a1)
  lwl     $t2,11($a1)
  lwr     $t3,12($a1)
  lwl     $t3,15($a1)
  lwr     $t4,16($a1)
  lwl     $t4,19($a1)
  lwr     $t5,20($a1)
  lwl     $t5,23($a1)
  lwr     $t6,24($a1)
  lwl     $t6,27($a1)
  lwr     $t7,28($a1)
  lwl     $t7,31($a1)
  addiu   $a1,$a1,32

  sw      $t0,0($a0)
  sw      $t1,4($a0)
  sw      $t2,8($a0)
  sw      $t3,12($a0)
  sw      $t4,16($a0)
  sw      $t5,20($a0)
  sw      $t6,24($a0)
  sw      $t7,28($a0)
  addiu   $a0,$a0,32

ua_chk1w:
  andi    $a2,$t8,0x3 # now a2 is the reminder past 1w chunks
  beq     $a2,$t8,ua_smallCopy
  subu    $a3,$t8,$a2 # a3 is count of bytes in 1w chunks
  addu    $a3,$a0,$a3 # now a3 is the dst address past the 1w chunks

# copying in words (4-byte chunks)
ua_wordCopy_loop:
  lwr     $v1,0($a1)
  lwl     $v1,3($a1)
  addiu   $a1,$a1,4
  addiu   $a0,$a0,4   # note: dst=a0 is word aligned here, see NOTE1
  bne     $a0,$a3,ua_wordCopy_loop
   sw     $v1,-4($a0)

# Now less than 4 bytes (value in a2) left to copy
ua_smallCopy:
  beqz    $a2,leave
   addu   $a3,$a0,$a2 # a3 is the last dst address
ua_smallCopy_loop:
  lb      $v1,0($a1)
  addiu   $a1,$a1,1
  addiu   $a0,$a0,1
  bne     $a0,$a3,ua_smallCopy_loop
   sb     $v1,-1($a0)

  j       $ra
   nop

  .set    at
  .set    reorder
  .end    memcpy_MIPS;
  .size   memcpy_MIPS,.-memcpy_MIPS

#endif // if defined (__mips__)
