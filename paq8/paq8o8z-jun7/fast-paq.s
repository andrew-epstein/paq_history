# GAS assembly language code for PAQ7 or newer (PAQ8o8z, in this case).
# (C) 2005-8, Matt Mahoney (w/ tweaks by rugxulo _AT_ gmail _DOT_ com)
# This is free software under GPL, http://www.gnu.org/licenses/gpl.txt
#
#   DJGPP / MinGW? / Linux? :     as fast-paq.s -o fast-paq.o
#
# http://gcc.gnu.org
#
# This code wont work in 64-bit mode.

.set CPUID_FLAG, 21
.set MMX_FLAG, 23
.set SSE2_FLAG, 26
.set OSFXSR, 9
#.set V86, 17

.intel_syntax noprefix

.section .text

.global _is_CPUID_supported

_is_CPUID_supported:   # gleaned from Phil Frisbie, Jr.s code
                       # http://glstation.free.fr/mine/djgpp/download/cpu.zip
  push ebx
  pushfd               # get extended flags
  pop eax
  mov ebx,eax          # save current flags
  xor eax,1 shl CPUID_FLAG
  push eax             # put new flags on stack
  popfd                # flags updated now in flags
  pushfd               # get extended flags
  pop eax
  xor eax,ebx          # if bit 21 r/w then supports CPUID
  mov eax,0
  jz .bye
  inc eax
.bye:
  pop ebx
  ret

.global  _get_cpu_flags

_get_cpu_flags:
  pushad                     # save all (better safe than sorry!)
  xor eax,eax
  inc eax
  cpuid
  xor eax,eax
  bt edx,MMX_FLAG            # bit 23
  adc eax,0                  # if Carry mov al, 1
  bt edx,SSE2_FLAG           # bit 26
  adc eax,0                  # if Carry, set eax = 2
  mov [esp+0x1C],eax         # replace soon-popped EAX on stack w/ return
  popad                      # restore all standard 386 registers
  ret

# Reset after MMX
#.global  _do_emms
#_do_emms:
#  emms                      # or "femms" for AMD (faster)
#  ret

# Vector product a*b of n signed words, returning signed dword scaled
# down by 8 bits. n is rounded up to a multiple of 8.

.global  _dot_product_mmx # (short* a, short* b, int n)

_dot_product_mmx:
  mov eax, [esp+4]         # a
  mov edx, [esp+8]         # b
  mov ecx, [esp+12]        # n
  add ecx, 7               # n rounding up
  and ecx, -8
  jz .done
  sub eax, 8
  sub edx, 8
  pxor mm0, mm0            # sum = 0
.loop:                     # each loop sums 4 products
  movq mm1, [eax+ecx*2]    # put halves of vector product in mm0
  pmaddwd mm1, [edx+ecx*2]
  movq mm2, [eax+ecx*2-8]
  pmaddwd mm2, [edx+ecx*2-8]
  psrad mm1, 8
  psrad mm2, 8
  paddd mm0, mm1
  paddd mm0, mm2
  sub ecx, 8
  ja .loop
  movq mm1, mm0            # add 2 halves of mm0 and return in eax
  psrlq mm1, 32
  paddd mm0, mm1
  movd eax, mm0
  emms
.done:
  ret

# Train n neural network weights w[n] on inputs t[n] and err.
# w[i] += t[i]*err*2+1 >> 17 bounded to +- 32K.
# n is rounded up to a multiple of 8.

.global  _train_mmx # (short* t, short* w, int n, int err)

_train_mmx:
  mov eax, [esp+16]       # err
  and eax, 0xffff         # put 4 copies of err in mm0
  movd mm0, eax
  movd mm1, eax
  psllq mm1, 16
  por mm0, mm1
  movq mm1, mm0
  psllq mm1, 32
  por mm0, mm1
  pcmpeqb mm1, mm1        # 4 copies of 1 in mm1
  psrlw mm1, 15
  mov eax, [esp+4]        # t
  mov edx, [esp+8]        # w
  mov ecx, [esp+12]       # n
  add ecx, 7              # n/8 rounding up
  and ecx, -8
  sub eax, 8
  sub edx, 8
  jz .done1
.loop1:                   # each iteration adjusts 8 weights
  movq mm2, [edx+ecx*2]   # w[i]
  movq mm3, [eax+ecx*2]   # t[i]
  movq mm4, [edx+ecx*2-8] # w[i]
  movq mm5, [eax+ecx*2-8] # t[i]
  paddsw mm3, mm3
  paddsw mm5, mm5
  pmulhw mm3, mm0
  pmulhw mm5, mm0
  paddsw mm3, mm1
  paddsw mm5, mm1
  psraw mm3, 1
  psraw mm5, 1
  paddsw mm2, mm3
  paddsw mm4, mm5
  movq [edx+ecx*2], mm2
  movq [edx+ecx*2-8], mm4
  sub ecx, 8
  ja .loop1
.done1:
  emms
  ret

# ---------------------------------------------------------------------------

# This should work on a Pentium 4 or AMD64 (or higher) in 32-bit mode,
# but it isnt too much faster than the MMX version.
# (What about the latest Core 2 Duos / Quads? Test results, anyone ??)

.global  _enable_sse2

_enable_sse2:
#  mov ax,cs
#  test al,3                 # not ring 0 (???), then exit
#  jnz .bye1
  mov eax,cr4
  bt eax,OSFXSR             # if already set, no need to enable
  jc .bye1
  or eax,1 shl OSFXSR       # bit 9 must be set
  mov cr4,eax
  mov eax,cr0
  and al,3                  # bit 2 (EM) must be off, bit 1 (MP) must be on
  or al,2
  mov cr0,eax
.bye1:
  ret

.global  _dot_product_sse2     # (short* a, short* b, int n)

xmm_save:
  .rept 512
  .byte 0
  .endr

_dot_product_sse2:
  cmp dword ptr [_fxsave_it],0
  jz .nosave2
  fxsave [xmm_save]
.nosave2:
  mov eax, [esp+4]          # a
  mov edx, [esp+8]          # b
  mov ecx, [esp+12]         # n
  add ecx, 7                # n rounding up
  and ecx, -8
  jz .done2
  sub eax, 16
  sub edx, 16
  pxor xmm0, xmm0           # sum = 0
.loop2:                     # each loop sums 4 products
  movdqa xmm1, [eax+ecx*2]  # put parital sums of vector product in xmm0
  pmaddwd xmm1, [edx+ecx*2]
  psrad xmm1, 8
  paddd xmm0, xmm1
  sub ecx, 8
  ja .loop2
  movdqa xmm1, xmm0         # add 4 parts of xmm0 and return in eax
  psrldq xmm1, 8
  paddd xmm0, xmm1
  movdqa xmm1, xmm0
  psrldq xmm1, 4
  paddd xmm0, xmm1
  movd eax, xmm0
.done2:
  cmp dword ptr [_fxsave_it],0
  jz .ret
  fxrstor [xmm_save]
.ret:
  ret

# Train n neural network weights w[n] on inputs t[n] and err.
# w[i] += t[i]*err*2+1 >> 17 bounded to +- 32K.
# n is rounded up to a multiple of 8.

# Train for SSE2
# Use this code to get some performance...

.global  _train_sse2 # (short* t, short* w, int n, int err)

_train_sse2:
  cmp dword ptr [_fxsave_it],0
  jz .nosave1
  fxsave [xmm_save]
.nosave1:
  mov eax, [esp+4]         # t
  mov edx, [esp+8]         # w
  mov ecx, [esp+12]        # n
  add ecx, 7               # n/8 rounding up
  and ecx, -8
  jz .done3
  sub eax, 16
  sub edx, 16
  movd xmm0, [esp+16]
  pshuflw xmm0,xmm0,0
  punpcklqdq xmm0,xmm0
.loop3:                    # each iteration adjusts 8 weights
  movdqa xmm3, [eax+ecx*2] # t[i]
  movdqa xmm2, [edx+ecx*2] # w[i]
  paddsw xmm3, xmm3        # t[i]*2
  pmulhw xmm3, xmm0        # t[i]*err*2 >> 16
  paddsw xmm3, [_mask]     # (t[i]*err*2 >> 16)+1
  psraw xmm3, 1            # (t[i]*err*2 >> 16)+1 >> 1
  paddsw xmm2, xmm3        # w[i] + xmm3
  movdqa [edx+ecx*2], xmm2
  sub ecx, 8
  ja .loop3
.done3:
  cmp dword ptr [_fxsave_it],0
  jz .ret1
  fxrstor [xmm_save]
.ret1:
  ret

.rept 3
.long 0                    # stupid 16-byte SSE2 alignment (annoying)!
.endr

_mask:
  .long 0x10001,0x10001,0x10001,0x10001      # 8 copies of 1 in xmm1

