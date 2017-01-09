/* Assemble this file with "gasp -a rsasm.s | as -o rsasm.o" */

/* Fast encoder for (255,223) Reed-Solomon code over GF(256) */

/* Called as */
/*    rse32(unsigned char data[223],unsigned char parity[32]); */

/* This code started as output of gcc -S, then was heavily hand */
/* massaged to exploit the Pentium dual pipeline */

/* Runs at about 30 megabits/sec on a 133 MHz Pentium */

/* Phil Karn, KA9Q July 5, 1997 */
	.comm	Gtab,8192,4

.globl rse32
	.type	 rse32,@function
rse32:
	pushl %ebp
	movl %esp,%ebp
	subl $32,%esp
	pushl %edi
	pushl %esi
	pushl %ebx
	movl 8(%ebp),%ebx	/* -> data */
	movl 12(%ebp),%esi	/* -> output parity bytes */

/* init parity bytes to zeroes */
	movl $7,%eax
.L75:	movl $0,-32(%ebp,%eax,4)
	decl %eax
	jns .L75

	movl $222,%edi		/* number of data bytes */
.L80:	xorl %eax,%eax
	movl -4(%ebp),%ecx	/* %ecx = (R31,R30,R29,R28) */
	movb (%edi,%ebx),%al	/* %al = data byte */
	roll $8,%ecx		/* %ecx = (R30,R29,R28,R31) */
	movl -8(%ebp),%edx	/* %edx = (R27,R26,R25,R24) */
	xorb %cl,%al		/* %al = R31 ^ data (feedback term) */

	shll $8,%edx		/* %edx = (R26,R25,R24,0) */
	movb -5(%ebp),%cl	/* %ecx = (R30,R29,R28,R27) */
	movb -9(%ebp),%dl	/* %edx = (R26,R25,R24,R23) */
	xorl Gtab+7168(,%eax,4),%ecx	/* %ecx = (R31',R30',R29',R28') */
	xorl Gtab+6144(,%eax,4),%edx	/* %edx = (R27',R26',R25',R24') */
	movl %ecx,-4(%ebp)
	movl %edx,-8(%ebp)

	movl -12(%ebp),%ecx	/* %ecx = (R23,R22,R21,R20) */
	movl -16(%ebp),%edx	/* %edx = (R19,R18,R17,R16) */
	shll $8,%ecx		/* %ecx = (R22,R21,R20,0) */
	shll $8,%edx		/* %edx = (R18,R17,R16,0) */
	movb -13(%ebp),%cl	/* %ecx = (R22,R21,R20,R19) */
	movb -17(%ebp),%dl	/* %edx = (R18,R17,R16,R15) */
	xorl Gtab+5120(,%eax,4),%ecx
	xorl Gtab+4096(,%eax,4),%edx
	movl %ecx,-12(%ebp)
	movl %edx,-16(%ebp)

	movl -20(%ebp),%ecx	/* %ecx = (R15,R14,R13,R12) */
	movl -24(%ebp),%edx	/* %edx = (R11,R10,R09,R08) */
	shll $8,%ecx		/* %ecx = (R14,R13,R12,0) */
	shll $8,%edx		/* %edx = (R10,R09,R08,0) */
	movb -21(%ebp),%cl	/* %ecx = (R14,R13,R12,R11) */
	movb -25(%ebp),%dl	/* %edx = (R10,R09,R08,R07) */
	xorl Gtab+3072(,%eax,4),%ecx
	xorl Gtab+2048(,%eax,4),%edx
	movl %ecx,-20(%ebp)
	movl %edx,-24(%ebp)

	movl -28(%ebp),%ecx	/* %ecx = (R07,R06,R05,R04) */
	movl -32(%ebp),%edx	/* %edx = (R03,R02,R01,R00) */
	shll $8,%ecx		/* %ecx = (R06,R05,R04,0) */
	shll $8,%edx		/* %edx = (R02,R01,R00,0) */
	movb -29(%ebp),%cl	/* %ecx = (R06,R05,R04,R03) */
	xorl Gtab(,%eax,4),%edx
	xorl Gtab+1024(,%eax,4),%ecx
	movl %edx,-32(%ebp)
	movl %ecx,-28(%ebp)

	decl %edi
	jns .L80

	/* Copy parity bytes to user buffer */
	movl $7,%edx
.L85:	movl -32(%ebp,%edx,4),%eax
	movl %eax,(%esi,%edx,4)
	decl %edx
	jns .L85

	xorl %eax,%eax		/* return 0 */
	leal -44(%ebp),%esp
	popl %ebx
	popl %esi
	popl %edi
	movl %ebp,%esp
	popl %ebp
	ret


/* This macro evaluates the input polynomial (which has 255 elements) */
/* at four consecutive values of alpha**n and stores the results */
/* The instructions are ordered to avoid address generation interlocks */
/* and to encourage parallel execution in the Pentium's two pipelines */

/* Input: %esi -> input buffer, 12(%ebp) -> output buffer */
/* Uses char Mtab[32][256], a multiplication lookup table */
/* trashes eax, ebx, ecx, edx, edi */

	.MACRO	DOSYN R
	xorl %eax,%eax
	xorl %ebx,%ebx
	xorl %ecx,%ecx
	xorl %edx,%edx

	movl $254,%edi
loop\@:	movb Mtab+256*(\R+0)(%eax),%al
	movb (%edi,%esi),%ah
	movb Mtab+256*(\R+1)(%ebx),%bl
	movb Mtab+256*(\R+2)(%ecx),%cl
	movb Mtab+256*(\R+3)(%edx),%dl
	xorb %ah,%al
	xorb %ah,%bl
	xorb %ah,%cl
	xorb %ah,%dl
	xorb %ah,%ah
	decl %edi
	jns loop\@
	movl 12(%ebp),%edi
	movb %al,\R(%edi)
	movb %bl,\R+1(%edi)
	movb %cl,\R+2(%edi)
	movb %dl,\R+3(%edi)
	.ENDM

	.comm	Mtab,8192,1
.text
.globl rssyndrome
	.type	 rssyndrome,@function
rssyndrome:
	pushl %ebp
	movl %esp,%ebp
	pushl %edi
	pushl %esi
	pushl %ebx
	movl 8(%ebp),%esi

	DOSYN 0
	DOSYN 4
	DOSYN 8
	DOSYN 12
	DOSYN 16
	DOSYN 20
	DOSYN 24
	DOSYN 28

	popl %ebx
	popl %esi
	popl %edi
	popl %ebp
	ret
	.END




