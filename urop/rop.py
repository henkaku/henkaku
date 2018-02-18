from util import isint, u32
from relocatable import Relocatable, data_base


class Caller:

    def __init__(self, rop):
        self.rop = rop

    def __getattr__(self, name):
        if not hasattr(self.rop.functions, name):
            raise RuntimeError("function does not exist: {}".format(name))

        func = getattr(self.rop.functions, name)

        def f(*args, **kwargs):
            return self.rop.call(func, *args, **kwargs)
        return f


Ret = object()


class Load:

    def __init__(self, addr):
        self.addr = addr


class Rop:

    class EmptyData:
        pass

    def __init__(self):
        self.caller = Caller(self)
        self.data = Rop.EmptyData()
        self._data_binary = b""
        self.alignment = 4
        self.compiled = False
        self.rop = []
        self.assume_null_init = False

    def call(self, func, *args, **kwargs):
        """ Generic call function that will do argument matching and execute a proper call* handler """
        # Generate argument signature
        sig = ""
        for arg in args:
            if isint(arg) or isinstance(arg, Relocatable):
                sig += "v"
            elif arg is Ret:
                sig += "r"
            elif isinstance(arg, str):
                # It will be allocated in compile() and essentially same as int
                sig += "v"
            elif isinstance(arg, Load):
                sig += "l"
            else:
                raise RuntimeError("unsupported function argument: {}".format(arg))
        for match, f in self._call_funcs.items():
            if match.startswith(sig):
                return f(self, func, *args, **kwargs)
        raise RuntimeError("didn't match sig: {} for args: {}".format(sig, args))

    def _alloc(self, data):
        # Insert "data" into what will become the data section
        remainder = len(data) % self.alignment
        if remainder != 0:
            data += b"\x00" * (self.alignment - remainder)

        offset = len(self._data_binary)
        self._data_binary += data
        return data_base + offset

    def pre_alloc_var(self, value):
        """
        Same as pre_alloc_data, but it's not pushed into self.data
        so especially useful for temporary-temporary storage, e.g. see RopThread
        """

        if isint(value):
            ptr = self._alloc(b"\x00" * value)
        elif isinstance(value, str):
            # make sure to null-terminate strings!
            ptr = self._alloc(value.encode("utf-8") + b"\x00")
        elif isinstance(value, bytes):
            ptr = self._alloc(value)
        else:
            raise RuntimeError("tried to allocate unknown type: {}".format(type(value)))

        return ptr

    def pre_alloc_data(self, **kwargs):
        """
        pre_ here means that no matter when you call it, the data's actually
        going to be allocated the first thing in the returned ropchain
        """

        for key, value in kwargs.items():
            ptr = self.pre_alloc_var(value)

            setattr(self.data, key, ptr)

    def do_write_data(self, data_binary):
        # This is kinda hacky, write32 will append to self.rop, so make it empty for now
        old_rop = self.rop
        self.rop = []
        for word in range(0, len(data_binary) // 4):
            data = data_binary[word*4:(word+1)*4]

            num = u32(data)
            if num == 0 and self.assume_null_init:
                continue
            addr = data_base + 4 * word
            self.write32(num, addr)

        # Now, append the old rop back
        # our rop is position-independent (is it?) so should be good
        self.rop += old_rop

    def _write_data_section(self):
        """
        Writes contents of data section into the data section (i.e. starting at data_base)
        This is the first thing that should be done
        Writes 1 word (4 bytes) at a time, is allowed to do clever optimizations.
        """

        remainder = len(self._data_binary) % 4
        if remainder != 0:
            self._data_binary += b"\x00" * (4 - remainder)

        self.do_write_data(self._data_binary)

    def compile(self):
        """
        Compiles rop chain.
        Doesn't return anything, access compiled_rop/compiled_relocs
        """

        if self.compiled:
            raise RuntimeError("you can't call compile() multiple times!")
        self.compiled = True

        # Go through the ropchain, find immediate strings and allocate them into the data section
        for x in range(len(self.rop)):
            if isinstance(self.rop[x], str):
                # make sure to null-terminate here as well
                self.rop[x] = self._alloc(self.rop[x].encode("utf-8") + b"\x00")

        self._write_data_section()

        self.compiled_rop = []
        self.compiled_relocs = []

        for item in self.rop:
            word = None
            reloc = None

            # really, you can put either int or Relocatable inside Load[]
            # whether it's Load or whatever's inside only matters during call type resolution
            # in call()
            if isinstance(item, Load):
                item = item.addr

            if isint(item):
                word = item
                reloc = 0
            elif isinstance(item, Relocatable):
                word = item.imm
                reloc = item.tag

            if word is None or reloc is None:
                print("for ropchain={}".format(self.rop))
                raise RuntimeError("compilation failed at item={}".format(item))

            assert(word is not None)
            assert(reloc is not None)

            if word < 0:
                word &= 0xFFFFFFFF
            
            self.compiled_rop.append(word)
            self.compiled_relocs.append(reloc)
