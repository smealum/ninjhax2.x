
sns_code/sns_code.bin:     file format binary


Disassembly of section .data:

0011d300 <.data>:
  11d300:	eb000002 	bl	0x11d310
  11d304:	e1a00000 	nop			; (mov r0, r0)
  11d308:	e1a00000 	nop			; (mov r0, r0)
  11d30c:	e1a00000 	nop			; (mov r0, r0)
  11d310:	e92d4010 	push	{r4, lr}
  11d314:	e24dd030 	sub	sp, sp, #48	; 0x30
  11d318:	e3a03001 	mov	r3, #1
  11d31c:	e28d0004 	add	r0, sp, #4
  11d320:	e28d1008 	add	r1, sp, #8
  11d324:	e59f2078 	ldr	r2, [pc, #120]	; 0x11d3a4
  11d328:	eb00002a 	bl	0x11d3d8
  11d32c:	e3a04001 	mov	r4, #1
  11d330:	e3500000 	cmp	r0, #0
  11d334:	159f306c 	ldrne	r3, [pc, #108]	; 0x11d3a8
  11d338:	15030541 	strne	r0, [r3, #-1345]	; 0x541
  11d33c:	e3a03000 	mov	r3, #0
  11d340:	e58d300c 	str	r3, [sp, #12]
  11d344:	e59d3004 	ldr	r3, [sp, #4]
  11d348:	e58d3010 	str	r3, [sp, #16]
  11d34c:	e59d300c 	ldr	r3, [sp, #12]
  11d350:	e28d2030 	add	r2, sp, #48	; 0x30
  11d354:	e28d000c 	add	r0, sp, #12
  11d358:	e0823103 	add	r3, r2, r3, lsl #2
  11d35c:	e28d1010 	add	r1, sp, #16
  11d360:	e1a02004 	mov	r2, r4
  11d364:	e5133020 	ldr	r3, [r3, #-32]
  11d368:	eb000022 	bl	0x11d3f8
  11d36c:	e3500000 	cmp	r0, #0
  11d370:	1afffff5 	bne	0x11d34c
  11d374:	e59d300c 	ldr	r3, [sp, #12]
  11d378:	e3530000 	cmp	r3, #0
  11d37c:	1a000006 	bne	0x11d39c
  11d380:	e28d3010 	add	r3, sp, #16
  11d384:	e0830104 	add	r0, r3, r4, lsl #2
  11d388:	e59d1010 	ldr	r1, [sp, #16]
  11d38c:	eb00001f 	bl	0x11d410
  11d390:	e58d400c 	str	r4, [sp, #12]
  11d394:	e2844001 	add	r4, r4, #1
  11d398:	eaffffeb 	b	0x11d34c
  11d39c:	eb000023 	bl	0x11d430
  11d3a0:	eaffffe9 	b	0x11d34c
  11d3a4:	0011d5c0 	andseq	sp, r1, r0, asr #11
  11d3a8:	deadbfff 	mcrle	15, 5, fp, cr13, cr15, {7}
  11d3ac:	00000000 	andeq	r0, r0, r0
  11d3b0:	e92d0030 	push	{r4, r5}
  11d3b4:	e59d4008 	ldr	r4, [sp, #8]
  11d3b8:	e59d500c 	ldr	r5, [sp, #12]
  11d3bc:	ef000070 	svc	0x00000070
  11d3c0:	e8bd0030 	pop	{r4, r5}
  11d3c4:	e12fff1e 	bx	lr
  11d3c8:	ef000071 	svc	0x00000071
  11d3cc:	e12fff1e 	bx	lr
  11d3d0:	ef000072 	svc	0x00000072
  11d3d4:	e12fff1e 	bx	lr
  11d3d8:	e92d0003 	push	{r0, r1}
  11d3dc:	ef000047 	svc	0x00000047
  11d3e0:	e59d3000 	ldr	r3, [sp]
  11d3e4:	e5831000 	str	r1, [r3]
  11d3e8:	e59d3004 	ldr	r3, [sp, #4]
  11d3ec:	e5832000 	str	r2, [r3]
  11d3f0:	e28dd008 	add	sp, sp, #8
  11d3f4:	e12fff1e 	bx	lr
  11d3f8:	e52d0004 	push	{r0}		; (str r0, [sp, #-4]!)
  11d3fc:	ef00004f 	svc	0x0000004f
  11d400:	e59d2000 	ldr	r2, [sp]
  11d404:	e5821000 	str	r1, [r2]
  11d408:	e28dd004 	add	sp, sp, #4
  11d40c:	e12fff1e 	bx	lr
  11d410:	e52d0004 	push	{r0}		; (str r0, [sp, #-4]!)
  11d414:	ef00004a 	svc	0x0000004a
  11d418:	e59d2000 	ldr	r2, [sp]
  11d41c:	e5821000 	str	r1, [r2]
  11d420:	e28dd004 	add	sp, sp, #4
  11d424:	e12fff1e 	bx	lr
  11d428:	e1a00000 	nop			; (mov r0, r0)
  11d42c:	e1a00000 	nop			; (mov r0, r0)
  11d430:	ee1d0f70 	mrc	15, 0, r0, cr13, cr0, {3}
  11d434:	e2800080 	add	r0, r0, #128	; 0x80
  11d438:	e12fff1e 	bx	lr
  11d43c:	e92d0011 	push	{r0, r4}
  11d440:	e59d0008 	ldr	r0, [sp, #8]
  11d444:	e59d400c 	ldr	r4, [sp, #12]
  11d448:	ef000001 	svc	0x00000001
  11d44c:	e49d2004 	pop	{r2}		; (ldr r2, [sp], #4)
  11d450:	e5821000 	str	r1, [r2]
  11d454:	e49d4004 	pop	{r4}		; (ldr r4, [sp], #4)
  11d458:	e12fff1e 	bx	lr
  11d45c:	ef000003 	svc	0x00000003
  11d460:	e12fff1e 	bx	lr
  11d464:	e92d0011 	push	{r0, r4}
  11d468:	e59d0008 	ldr	r0, [sp, #8]
  11d46c:	e59d400c 	ldr	r4, [sp, #12]
  11d470:	ef000008 	svc	0x00000008
  11d474:	e49d2004 	pop	{r2}		; (ldr r2, [sp], #4)
  11d478:	e5821000 	str	r1, [r2]
  11d47c:	e49d4004 	pop	{r4}		; (ldr r4, [sp], #4)
  11d480:	e12fff1e 	bx	lr
  11d484:	ef000009 	svc	0x00000009
  11d488:	e12fff1e 	bx	lr
  11d48c:	ef00000a 	svc	0x0000000a
  11d490:	e12fff1e 	bx	lr
  11d494:	e52d0004 	push	{r0}		; (str r0, [sp, #-4]!)
  11d498:	ef000013 	svc	0x00000013
  11d49c:	e49d3004 	pop	{r3}		; (ldr r3, [sp], #4)
  11d4a0:	e5831000 	str	r1, [r3]
  11d4a4:	e12fff1e 	bx	lr
  11d4a8:	ef000014 	svc	0x00000014
  11d4ac:	e12fff1e 	bx	lr
  11d4b0:	e52d0004 	push	{r0}		; (str r0, [sp, #-4]!)
  11d4b4:	ef000016 	svc	0x00000016
  11d4b8:	e49d2004 	pop	{r2}		; (ldr r2, [sp], #4)
  11d4bc:	e5821000 	str	r1, [r2]
  11d4c0:	e12fff1e 	bx	lr
  11d4c4:	e52d0004 	push	{r0}		; (str r0, [sp, #-4]!)
  11d4c8:	ef000017 	svc	0x00000017
  11d4cc:	e49d2004 	pop	{r2}		; (ldr r2, [sp], #4)
  11d4d0:	e5821000 	str	r1, [r2]
  11d4d4:	e12fff1e 	bx	lr
  11d4d8:	ef000018 	svc	0x00000018
  11d4dc:	e12fff1e 	bx	lr
  11d4e0:	ef000019 	svc	0x00000019
  11d4e4:	e12fff1e 	bx	lr
  11d4e8:	e52d0004 	push	{r0}		; (str r0, [sp, #-4]!)
  11d4ec:	e59d0004 	ldr	r0, [sp, #4]
  11d4f0:	ef00001e 	svc	0x0000001e
  11d4f4:	e49d2004 	pop	{r2}		; (ldr r2, [sp], #4)
  11d4f8:	e5821000 	str	r1, [r2]
  11d4fc:	e12fff1e 	bx	lr
  11d500:	ef00001f 	svc	0x0000001f
  11d504:	e12fff1e 	bx	lr
  11d508:	ef000020 	svc	0x00000020
  11d50c:	e12fff1e 	bx	lr
  11d510:	ef000022 	svc	0x00000022
  11d514:	e12fff1e 	bx	lr
  11d518:	ef000023 	svc	0x00000023
  11d51c:	e12fff1e 	bx	lr
  11d520:	ef000024 	svc	0x00000024
  11d524:	e12fff1e 	bx	lr
  11d528:	e52d5004 	push	{r5}		; (str r5, [sp, #-4]!)
  11d52c:	e1a05000 	mov	r5, r0
  11d530:	e59d0004 	ldr	r0, [sp, #4]
  11d534:	e59d4008 	ldr	r4, [sp, #8]
  11d538:	ef000025 	svc	0x00000025
  11d53c:	e5851000 	str	r1, [r5]
  11d540:	e49d5004 	pop	{r5}		; (ldr r5, [sp], #4)
  11d544:	e12fff1e 	bx	lr
  11d548:	ef000028 	svc	0x00000028
  11d54c:	e12fff1e 	bx	lr
  11d550:	e92d0011 	push	{r0, r4}
  11d554:	ef00002a 	svc	0x0000002a
  11d558:	e49d4004 	pop	{r4}		; (ldr r4, [sp], #4)
  11d55c:	e5841000 	str	r1, [r4]
  11d560:	e5842004 	str	r2, [r4, #4]
  11d564:	e49d4004 	pop	{r4}		; (ldr r4, [sp], #4)
  11d568:	e12fff1e 	bx	lr
  11d56c:	e92d0011 	push	{r0, r4}
  11d570:	ef00002b 	svc	0x0000002b
  11d574:	e49d4004 	pop	{r4}		; (ldr r4, [sp], #4)
  11d578:	e5841000 	str	r1, [r4]
  11d57c:	e5842004 	str	r2, [r4, #4]
  11d580:	e49d4004 	pop	{r4}		; (ldr r4, [sp], #4)
  11d584:	e12fff1e 	bx	lr
  11d588:	e52d0004 	push	{r0}		; (str r0, [sp, #-4]!)
  11d58c:	ef00002d 	svc	0x0000002d
  11d590:	e49d3004 	pop	{r3}		; (ldr r3, [sp], #4)
  11d594:	e5831000 	str	r1, [r3]
  11d598:	e12fff1e 	bx	lr
  11d59c:	ef000032 	svc	0x00000032
  11d5a0:	e12fff1e 	bx	lr
  11d5a4:	e52d0004 	push	{r0}		; (str r0, [sp, #-4]!)
  11d5a8:	ef000035 	svc	0x00000035
  11d5ac:	e49d3004 	pop	{r3}		; (ldr r3, [sp], #4)
  11d5b0:	e5831000 	str	r1, [r3]
  11d5b4:	e12fff1e 	bx	lr
  11d5b8:	e1a00000 	nop			; (mov r0, r0)
  11d5bc:	e1a00000 	nop			; (mov r0, r0)
  11d5c0:	533a6268 	teqpl	sl, #-2147483642	; 0x80000006
  11d5c4:	49434550 	stmdbmi	r3, {r4, r6, r8, sl, lr}^
  11d5c8:	Address 0x0011d5c8 is out of bounds.

