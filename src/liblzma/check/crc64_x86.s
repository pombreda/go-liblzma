/*
 * Speed-optimized CRC64 using slicing-by-four algorithm
 * Instruction set: i386
 * Optimized for:   i686
 *
 * This code has been put into the public domain by its authors:
 * Igor Pavlov <http://7-zip.org/>
 * Lasse Collin <lasse.collin@tukaani.org>
 *
 * This code needs lzma_crc64_table, which can be created using the
 * following C code:

uint64_t lzma_crc64_table[4][256];

void
init_table(void)
{
	static const uint64_t poly64 = UINT64_C(0xC96C5795D7870F42);

	for (size_t s = 0; s < 4; ++s) {
		for (size_t b = 0; b < 256; ++b) {
			uint64_t r = s == 0 ? b : lzma_crc64_table[s - 1][b];

			for (size_t i = 0; i < 8; ++i) {
				if (r & 1)
					r = (r >> 1) ^ poly64;
				else
					r >>= 1;
			}

			lzma_crc64_table[s][b] = r;
		}
	}
}

 * The prototype of the CRC64 function:
 * extern uint64_t lzma_crc64(const uint8_t *buf, size_t size, uint64_t crc);
 */

	.text
	.global	lzma_crc64
	.type	lzma_crc64, @function

	.align	16
lzma_crc64:
	/*
	 * Register usage:
	 * %eax crc LSB
	 * %edx crc MSB
	 * %esi buf
	 * %edi size or buf + size
	 * %ebx lzma_crc64_table
	 * %ebp Table index
	 * %ecx Temporary
	 */
	pushl	%ebx
	pushl	%esi
	pushl	%edi
	pushl	%ebp
	movl	0x14(%esp), %esi /* buf */
	movl	0x18(%esp), %edi /* size */
	movl	0x1C(%esp), %eax /* crc LSB */
	movl	0x20(%esp), %edx /* crc MSB */

	/*
	 * Store the address of lzma_crc64_table to %ebx. This is needed to
	 * get position-independent code (PIC).
	 */
	call	.L_PIC
.L_PIC:
	popl	%ebx
	addl	$_GLOBAL_OFFSET_TABLE_+[.-.L_PIC], %ebx
	movl	lzma_crc64_table@GOT(%ebx), %ebx

	/* Complement the initial value. */
	notl	%eax
	notl	%edx

.L_align:
	/*
	 * Check if there is enough input to use slicing-by-four.
	 * We need eight bytes, because the loop pre-reads four bytes.
	 */
	cmpl	$8, %edi
	jl	.L_rest

	/* Check if we have reached alignment of four bytes. */
	testl	$3, %esi
	jz	.L_slice

	/* Calculate CRC of the next input byte. */
	movzbl	(%esi), %ebp
	incl	%esi
	movzbl	%al, %ecx
	xorl	%ecx, %ebp
	shrdl	$8, %edx, %eax
	xorl	(%ebx, %ebp, 8), %eax
	shrl	$8, %edx
	xorl	4(%ebx, %ebp, 8), %edx
	decl	%edi
	jmp	.L_align

.L_slice:
	/*
	 * If we get here, there's at least eight bytes of aligned input
	 * available. Make %edi multiple of four bytes. Store the possible
	 * remainder over the "size" variable in the argument stack.
	 */
	movl	%edi, 0x18(%esp)
	andl	$-4, %edi
	subl	%edi, 0x18(%esp)

	/*
	 * Let %edi be buf + size - 4 while running the main loop. This way
	 * we can compare for equality to determine when exit the loop.
	 */
	addl	%esi, %edi
	subl	$4, %edi

	/* Read in the first four aligned bytes. */
	movl	(%esi), %ecx

.L_loop:
	xorl	%eax, %ecx
	movzbl	%cl, %ebp
	movl	0x1800(%ebx, %ebp, 8), %eax
	xorl	%edx, %eax
	movl	0x1804(%ebx, %ebp, 8), %edx
	movzbl	%ch, %ebp
	xorl	0x1000(%ebx, %ebp, 8), %eax
	xorl	0x1004(%ebx, %ebp, 8), %edx
	shrl	$16, %ecx
	movzbl	%cl, %ebp
	xorl	0x0800(%ebx, %ebp, 8), %eax
	xorl	0x0804(%ebx, %ebp, 8), %edx
	movzbl	%ch, %ebp
	addl	$4, %esi
	xorl	(%ebx, %ebp, 8), %eax
	xorl	4(%ebx, %ebp, 8), %edx

	/* Check for end of aligned input. */
	cmpl	%edi, %esi

	/*
	 * Copy the next input byte to %ecx. It is slightly faster to
	 * read it here than at the top of the loop.
	 */
	movl	(%esi), %ecx
	jl	.L_loop

	/*
	 * Process the remaining four bytes, which we have already
	 * copied to %ecx.
	 */
	xorl	%eax, %ecx
	movzbl	%cl, %ebp
	movl	0x1800(%ebx, %ebp, 8), %eax
	xorl	%edx, %eax
	movl	0x1804(%ebx, %ebp, 8), %edx
	movzbl	%ch, %ebp
	xorl	0x1000(%ebx, %ebp, 8), %eax
	xorl	0x1004(%ebx, %ebp, 8), %edx
	shrl	$16, %ecx
	movzbl	%cl, %ebp
	xorl	0x0800(%ebx, %ebp, 8), %eax
	xorl	0x0804(%ebx, %ebp, 8), %edx
	movzbl	%ch, %ebp
	addl	$4, %esi
	xorl	(%ebx, %ebp, 8), %eax
	xorl	4(%ebx, %ebp, 8), %edx

	/* Copy the number of remaining bytes to %edi. */
	movl	0x18(%esp), %edi

.L_rest:
	/* Check for end of input. */
	testl	%edi, %edi
	jz	.L_return

	/* Calculate CRC of the next input byte. */
	movzbl	(%esi), %ebp
	incl	%esi
	movzbl	%al, %ecx
	xorl	%ecx, %ebp
	shrdl	$8, %edx, %eax
	xorl	(%ebx, %ebp, 8), %eax
	shrl	$8, %edx
	xorl	4(%ebx, %ebp, 8), %edx
	decl	%edi
	jmp	.L_rest

.L_return:
	/* Complement the final value. */
	notl	%eax
	notl	%edx

	popl	%ebp
	popl	%edi
	popl	%esi
	popl	%ebx
	ret

	.size	lzma_crc32, .-lzma_crc32
