.syntax unified

.code 32

.global start
.section .text.start
start:
	mov r0, r4
	mov r1, r5
	add sp, #0x200
	blx loader
