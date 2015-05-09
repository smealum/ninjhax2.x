
sns_code/sns_code.bin:     file format binary


Disassembly of section .data:

00100000 <.data>:
  100000:	eb000002 	bl	0x100010
  100004:	e1a00000 	nop			; (mov r0, r0)
  100008:	e1a00000 	nop			; (mov r0, r0)
  10000c:	e1a00000 	nop			; (mov r0, r0)
  100010:	e59f300c 	ldr	r3, [pc, #12]	; 0x100024
  100014:	e59f200c 	ldr	r2, [pc, #12]	; 0x100028
  100018:	e3a00000 	mov	r0, #0
  10001c:	e5832afe 	str	r2, [r3, #2814]	; 0xafe
  100020:	e12fff1e 	bx	lr
  100024:	c0dec000 	sbcsgt	ip, lr, r0
  100028:	deadbabe 	mcrle	10, 5, fp, cr13, cr14, {5}
  10002c:	00000000 	andeq	r0, r0, r0
  100030:	e92d0030 	push	{r4, r5}
  100034:	e59d4008 	ldr	r4, [sp, #8]
  100038:	e59d500c 	ldr	r5, [sp, #12]
  10003c:	ef000070 	svc	0x00000070
  100040:	e8bd0030 	pop	{r4, r5}
  100044:	e12fff1e 	bx	lr
  100048:	ef000071 	svc	0x00000071
  10004c:	e12fff1e 	bx	lr
  100050:	ef000072 	svc	0x00000072
  100054:	e12fff1e 	bx	lr
  100058:	e52d0004 	push	{r0}		; (str r0, [sp, #-4]!)
  10005c:	ef00004f 	svc	0x0000004f
  100060:	e59d2000 	ldr	r2, [sp]
  100064:	e5821000 	str	r1, [r2]
  100068:	e28dd004 	add	sp, sp, #4
  10006c:	e12fff1e 	bx	lr
  100070:	e52d0004 	push	{r0}		; (str r0, [sp, #-4]!)
  100074:	ef00004a 	svc	0x0000004a
  100078:	e59d2000 	ldr	r2, [sp]
  10007c:	e5821000 	str	r1, [r2]
  100080:	e28dd004 	add	sp, sp, #4
  100084:	e12fff1e 	bx	lr
  100088:	e1a00000 	nop			; (mov r0, r0)
  10008c:	e1a00000 	nop			; (mov r0, r0)
