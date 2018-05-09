	.section	__TEXT,__text,regular,pure_instructions
	.macosx_version_min 10, 11
	.globl	_dot_product ##
	.p2align	4, 0x90
_dot_product:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	vpxor	%xmm0, %xmm0, %xmm0
	cmpl	$8, %edx
	jl	LBB0_8
	movslq	%edx, %rax
	movq	%rax, %rcx
	notq	%rcx
	cmpq	$-17, %rcx
	movq	$-16, %rdx
	cmovgq	%rcx, %rdx
	leaq	(%rdx,%rax), %rcx
	addq	$8, %rcx
	movl	%ecx, %edx
	shrl	$3, %edx
	addl	$1, %edx
	andq	$7, %rdx
	je	LBB0_2
	negq	%rdx
	vpxor	%xmm0, %xmm0, %xmm0
	.p2align	4, 0x90
LBB0_4:
	vmovdqa	-16(%rdi,%rax,2), %xmm1
	vpmaddwd	-16(%rsi,%rax,2), %xmm1, %xmm1
	leaq	-8(%rax), %rax
	vpsrad	$8, %xmm1, %xmm1
	vpaddd	%xmm0, %xmm1, %xmm0
	addq	$1, %rdx
	jne	LBB0_4
	cmpq	$56, %rcx
	jae	LBB0_6
	jmp	LBB0_8
LBB0_2:
	vpxor	%xmm0, %xmm0, %xmm0
	cmpq	$56, %rcx
	jb	LBB0_8
LBB0_6:
	addq	$64, %rax
	.p2align	4, 0x90
LBB0_7:
	vmovdqa	-144(%rdi,%rax,2), %xmm1
	vpmaddwd	-144(%rsi,%rax,2), %xmm1, %xmm1
	vpsrad	$8, %xmm1, %xmm1
	vpaddd	%xmm0, %xmm1, %xmm0
	vmovdqa	-160(%rdi,%rax,2), %xmm1
	vpmaddwd	-160(%rsi,%rax,2), %xmm1, %xmm1
	vpsrad	$8, %xmm1, %xmm1
	vmovdqa	-176(%rdi,%rax,2), %xmm2
	vpmaddwd	-176(%rsi,%rax,2), %xmm2, %xmm2
	vpsrad	$8, %xmm2, %xmm2
	vpaddd	%xmm2, %xmm1, %xmm1
	vpaddd	%xmm1, %xmm0, %xmm0
	vmovdqa	-192(%rdi,%rax,2), %xmm1
	vpmaddwd	-192(%rsi,%rax,2), %xmm1, %xmm1
	vpsrad	$8, %xmm1, %xmm1
	vmovdqa	-256(%rdi,%rax,2), %xmm2
	vmovdqa	-240(%rdi,%rax,2), %xmm3
	vmovdqa	-224(%rdi,%rax,2), %xmm4
	vmovdqa	-208(%rdi,%rax,2), %xmm5
	vpmaddwd	-208(%rsi,%rax,2), %xmm5, %xmm5
	vpsrad	$8, %xmm5, %xmm5
	vpaddd	%xmm5, %xmm1, %xmm1
	vpmaddwd	-224(%rsi,%rax,2), %xmm4, %xmm4
	vpsrad	$8, %xmm4, %xmm4
	vpaddd	%xmm4, %xmm1, %xmm1
	vpaddd	%xmm1, %xmm0, %xmm0
	vpmaddwd	-240(%rsi,%rax,2), %xmm3, %xmm1
	vpsrad	$8, %xmm1, %xmm1
	vpmaddwd	-256(%rsi,%rax,2), %xmm2, %xmm2
	vpsrad	$8, %xmm2, %xmm2
	vpaddd	%xmm2, %xmm1, %xmm1
	vpaddd	%xmm1, %xmm0, %xmm0
	addq	$-64, %rax
	cmpq	$71, %rax
	jg	LBB0_7
LBB0_8:
	vpsrldq	$8, %xmm0, %xmm1
	vpaddd	%xmm1, %xmm0, %xmm0
	vpshufd	$229, %xmm0, %xmm1
	vpaddd	%xmm1, %xmm0, %xmm0
	vmovd	%xmm0, %eax
	popq	%rbp
	retq
	.cfi_endproc
	.section	__TEXT,__literal16,16byte_literals
	.p2align	4
LCPI1_0:
	.short	1
	.short	1
	.short	1
	.short	1
	.short	1
	.short	1
	.short	1
	.short	1
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_train
	.p2align	4, 0x90
_train:
	.cfi_startproc
	pushq	%rbp
	.cfi_def_cfa_offset 16
	.cfi_offset %rbp, -16
	movq	%rsp, %rbp
	.cfi_def_cfa_register %rbp
	cmpl	$8, %edx
	jl	LBB1_7
	vmovd	%ecx, %xmm0
	vpbroadcastw	%xmm0, %xmm0
	movslq	%edx, %rax
	movq	%rax, %rcx
	notq	%rcx
	cmpq	$-17, %rcx
	movq	$-16, %rdx
	cmovgq	%rcx, %rdx
	leaq	(%rdx,%rax), %rcx
	addq	$8, %rcx
	movl	%ecx, %edx
	shrl	$3, %edx
	addl	$1, %edx
	andq	$3, %rdx
	je	LBB1_4
	negq	%rdx
	vmovdqa	LCPI1_0(%rip), %xmm1
	.p2align	4, 0x90
LBB1_3:
	vmovdqa	-16(%rdi,%rax,2), %xmm2
	vpaddsw	%xmm2, %xmm2, %xmm2
	vpmulhw	%xmm0, %xmm2, %xmm2
	vpaddsw	%xmm1, %xmm2, %xmm2
	vpsraw	$1, %xmm2, %xmm2
	vpaddsw	-16(%rsi,%rax,2), %xmm2, %xmm2
	vmovdqa	%xmm2, -16(%rsi,%rax,2)
	leaq	-8(%rax), %rax
	addq	$1, %rdx
	jne	LBB1_3
LBB1_4:
	cmpq	$24, %rcx
	jb	LBB1_7
	addq	$32, %rax
	vmovdqa	LCPI1_0(%rip), %xmm1
	.p2align	4, 0x90
LBB1_6:
	vmovdqa	-80(%rdi,%rax,2), %xmm2
	vpaddsw	%xmm2, %xmm2, %xmm2
	vpmulhw	%xmm0, %xmm2, %xmm2
	vpaddsw	%xmm1, %xmm2, %xmm2
	vpsraw	$1, %xmm2, %xmm2
	vpaddsw	-80(%rsi,%rax,2), %xmm2, %xmm2
	vmovdqa	%xmm2, -80(%rsi,%rax,2)
	vmovdqa	-96(%rdi,%rax,2), %xmm2
	vpaddsw	%xmm2, %xmm2, %xmm2
	vpmulhw	%xmm0, %xmm2, %xmm2
	vpaddsw	%xmm1, %xmm2, %xmm2
	vpsraw	$1, %xmm2, %xmm2
	vpaddsw	-96(%rsi,%rax,2), %xmm2, %xmm2
	vmovdqa	%xmm2, -96(%rsi,%rax,2)
	vmovdqa	-112(%rdi,%rax,2), %xmm2
	vpaddsw	%xmm2, %xmm2, %xmm2
	vpmulhw	%xmm0, %xmm2, %xmm2
	vpaddsw	%xmm1, %xmm2, %xmm2
	vpsraw	$1, %xmm2, %xmm2
	vpaddsw	-112(%rsi,%rax,2), %xmm2, %xmm2
	vmovdqa	%xmm2, -112(%rsi,%rax,2)
	vmovdqa	-128(%rdi,%rax,2), %xmm2
	vpaddsw	%xmm2, %xmm2, %xmm2
	vpmulhw	%xmm0, %xmm2, %xmm2
	vpaddsw	%xmm1, %xmm2, %xmm2
	vpsraw	$1, %xmm2, %xmm2
	vpaddsw	-128(%rsi,%rax,2), %xmm2, %xmm2
	vmovdqa	%xmm2, -128(%rsi,%rax,2)
	addq	$-32, %rax
	cmpq	$39, %rax
	jg	LBB1_6
LBB1_7:
	popq	%rbp
	retq
	.cfi_endproc
.subsections_via_symbols
