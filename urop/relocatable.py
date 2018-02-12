from util import isint


class Relocatable():

    data = 1
    SceWebKit = 2
    SceLibKernel = 3
    SceLibc = 4
    SceLibHttp = 5
    SceNet = 6
    SceAppMgr = 7

    def __init__(self, tag, imm):
        self.tag = tag
        self.imm = imm

    def __add__(self, x):
        if not isint(x):
            raise RuntimeError("cannot __add__ a {}".format(x))
        return Relocatable(self.tag, self.imm + x)

    def __sub__(self, x):
        if not isint(x):
            raise RuntimeError("cannot __sub__ a {}".format(x))
        return Relocatable(self.tag, self.imm - x)

    def __repr__(self):
        return "Relocable<tag={}, imm=0x{:x}>".format(self.tag, self.imm)

data_base = Relocatable(Relocatable.data, 0)
SceWebKit_base = Relocatable(Relocatable.SceWebKit, 0)
SceLibKernel_base = Relocatable(Relocatable.SceLibKernel, 0)
SceLibc_base = Relocatable(Relocatable.SceLibc, 0)
SceLibHttp_base = Relocatable(Relocatable.SceLibHttp, 0)
SceNet_base = Relocatable(Relocatable.SceNet, 0)
SceAppMgr_base = Relocatable(Relocatable.SceAppMgr, 0)
