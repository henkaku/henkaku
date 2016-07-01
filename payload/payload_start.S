.syntax unified

.code 32

.section .text.start
start:
	add sp, #0x1000
	mov r0, r4
	blx payload
