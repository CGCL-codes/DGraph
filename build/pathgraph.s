.section .text
.type PG_asm_get,@function
.globl PG_asm_get
PG_asm_get:
	movq (%rdi), %r8
	cmpq %r8, %rsi
	ja decompress
	movb $0, %al
	ret
decompress:
    movq $0, %rax
	movq $0, %rcx
loop:	
	sal $7, %rax
	movb (%r8), %cl
	movq %rcx, %r9
	and $0x7f, %r9
	or %r9, %rax
	addq $1, %r8 
#andb $0x80, %cl
	cmpb  $0x80, %cl
	jae loop
	movq %r8, (%rdi)
	addl %eax, (%rdx)
	movb $1, %al
	ret

