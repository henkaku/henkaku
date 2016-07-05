.syntax unified

.code 32

.global start
.section .text.start
start:
	mov r0, r4
	blx loader
