import    os
import    re


# Wrap a 1:1 mapping of symbol names to literals
class Enum(object):
    def __init__(self, lut):
        self.lut    = lut
        self.rlut   = dict((v, k) for (k, v) in lut.items())
    def __getattr__(self, name):
        return self.lut.get(name)
    def __getitem__(self, value):
        return self.rlut.get(value)


# Define names (e.g. reg_set.AL), provide numeric->text LUT (e.g. reg_set[3])
reg_set = Enum({'AL':0x00,'CL':0x01,'DL':0x02,'BL':0x03,
                'AH':0x04,'CH':0x05,'DH':0x06,'BH':0x07,
                'AX':0x10,'CX':0x11,'DX':0x12,'BX':0x13,
                'SP':0x14,'BP':0x15,'SI':0x16,'DI':0x17,
                'ES':0x30,'CS':0x31,'SS':0x32,'DS':0x33,})

# Define opcode maps
lines = file(os.path.join(os.path.split(__file__)[0], '8086_table.txt'), 'rb').readlines()
ops = filter(None, map(re.compile(r'([0-9A-F]{2})\s+(.+)').match, lines))
opcode_map = dict((int(m.group(1), 16), m.group(2).split()) for m in ops)
ops = filter(None, map(re.compile(r'(GRP\S+)/(\d)\s+(.+)').match, lines))
opcode_extension_map = dict(((m.group(1), int(m.group(2))), m.group(3).split()) for m in ops)
del lines; del ops


class Instruction(object):
    def __init__(self, addr, code, mnemonic, *args):
        self.addr    = addr
        self.code    = code
        self.mnemonic    = mnemonic
        self.args    = args

        for a in self.args: a.set_parent(self)
    def __str__(self):
        core = ' '.join(map(str, (self.addr, self.code, self.mnemonic)))
        if (self.args): return core + ' ' + ', '.join(map(str, self.args))
        else: return core

class Address(object):
    def __init__(self, segment, offset):
        self.segment = segment
        self.offset  = offset
    def __str__(self):
        return '%04X:%04X'%(self.segment, self.offset)

    def calc_relative_address(self, displacement):
        return Address(self.segment, (self.offset+displacement)&0xffff)

class MachineCode(object):
    def __init__(self, bytes):
        self.bytes = bytes
    def __str__(self):
        return '%-12s'%''.join('%02X'%byte for byte in self.bytes)
    def __len__(self):
        return len(self.bytes)

    def get_opcode(self):
        return self.bytes[0]

class Mnemonic(object):
    def __init__(self, name):
        self.name = name
    def __str__(self):
        return '%-6s'%self.name

class Argument(object):
    _parent = None
    def set_parent(self, parent):
        self._parent = parent

class Arg_Address(Argument, Address):
    pass

class Arg_Constant(Argument, int):
    pass

class Arg_Offset(Argument):
    def __init__(self, offset, offset_size):
        self.offset = offset

        # offset will be read from the machine code as an unsigned integer
        if (self.offset >= pow(2, offset_size-1)):
            self.offset -= pow(2, offset_size)
    def __str__(self):
        addr = self._parent.addr
        code = self._parent.code
        return str(addr.calc_relative_address(len(code)+self.offset))[-4:]

class Arg_Register(Argument):
    type_lut = {'b':0x00, 'v':0x10, 'w':0x10, 'S':0x30}
    def __init__(self, name=None, code=None, type=None):
        if (type): type = self.type_lut[type]
        if (name):
            reg = reg_set.__getattr__(name)
            code = reg&0xf; type = reg&0xf0
        self.code = code
        self.type = type
    def __str__(self):
        return reg_set[self.code+self.type]

    def set_type(self, type):
        self.type = self.type_lut[type]

class Arg_Integer(Argument):
    def __init__(self, value, value_size):
        self.value    = value
        self.value_size    = value_size
    def __str__(self):
        if (self._parent.code.get_opcode() == 0x83):
            # Special case: display as an 8-bit sign-extended value
            if (self.value >= 128): value = self.value - 256
            else: value = self.value
            return '%+03X'%value
        else:
            return '%0*X'%(self.value_size/4, self.value)

class Arg_Dereference(Argument):
    q_opcodes        = set([    0x80, 0x81, 0x82, 0x83, 0xC6, 0xC7,
                    0xD0, 0xD1, 0xD2, 0xD3, 0xF6, 0xF7, 0xFE, 0xFF])
    nq_mnemonics        = set(['PUSH', 'CALL', 'JMP'])
    q_far_mnemonics    = set(['CALL', 'JMP'])
    q_lut            = {'b':'BYTE PTR', 'w':'WORD PTR', 'v':'WORD PTR', 'p':'FAR'}
    
    def __init__(self, type=None, base=None, index=None, disp=None, disp_size=None):
        self.type    = type
        self.base    = base
        self.index    = index
        self.disp    = disp
        self.disp_size    = disp_size
    def __str__(self):
        # Build up the dereference, one piece at a time
        rv = []; n_terms = 0
        # Qualifier, if applicable
        if (self._parent.code.get_opcode() in self.q_opcodes):
            if ((not str(self._parent.mnemonic).strip() in self.nq_mnemonics) or
                ((str(self._parent.mnemonic).strip() in self.q_far_mnemonics) and (self.type == 'p'))):
                rv.append(self.q_lut[self.type] + ' ')
        # Bracket
        rv.append('[')
        # Optional base and index terms
        if (self.base != None):
            rv.append(reg_set[self.base]); 
            n_terms += 1
        if (self.index != None):
            if (n_terms): rv.append('+')
            rv.append(reg_set[self.index]); 
            n_terms += 1
        # Displacement
        if (self.disp != None):
            if (self.disp_size == 8):
                # 8-bit displacement; always preceeded by at least one term
                disp = self.disp
                if (disp > 128): disp -= 256
                rv.append('%+03X'%disp); 
                n_terms += 1
            else:
                # 16-bit displacement
                if (n_terms): rv.append('+')
                rv.append('%04X'%self.disp)
                n_terms += 1
        # Bracket
        rv.append(']')
        # Concatenate and return
        return ''.join(rv)

    def set_type(self, type):
        self.type    = type


class Disassembler(object):
    def __init__(self):
        self.index    = 0
        self.bytes    = ''
        self.first_addy    = Address(0, 0)
        self.modrm    = None

    def read_integer(self, num_bytes):
        rv = sum(ord(self.bytes[self.index+i])*pow(2, i*8) for i in range(num_bytes))
        self.index += num_bytes; return rv

    def read_modrm(self):
        if (self.modrm): return

        modrm = ord(self.bytes[self.index]); self.index += 1
        mod = modrm>>6; reg=(modrm>>3)&7; rm=modrm&7

        if (mod == 0):
            if (rm == 6):
                # Special case
                self.modrm = {'rm':Arg_Dereference(disp=self.read_integer(2), disp_size=16), 'reg':reg}
                return
            disp = None; disp_size = None
        elif (mod == 1):
            disp = self.read_integer(1); disp_size = 8
        elif (mod == 2):
            disp = self.read_integer(2); disp_size = 16
        elif (mod == 3):
            self.modrm = {'rm':Arg_Register(code=rm), 'reg':reg}
            return

        if (rm == 0):
            base = reg_set.BX; index = reg_set.SI
        elif (rm == 1):
            base = reg_set.BX; index = reg_set.DI
        elif (rm == 2):
            base = reg_set.BP; index = reg_set.SI
        elif (rm == 3):
            base = reg_set.BP; index = reg_set.DI
        elif (rm == 4):
            base = None; index = reg_set.SI
        elif (rm == 5):
            base = None; index = reg_set.DI
        elif (rm == 6):
            base = reg_set.BP; index = None
        elif (rm == 7):
            base = reg_set.BX; index = None

        self.modrm = {'rm':Arg_Dereference(base=base, index=index, disp=disp, disp_size=disp_size), 'reg':reg}

    def read_opcode(self):
        opcode = ord(self.bytes[self.index]); self.index += 1
        mnemonic = opcode_map[opcode][0]; arguments = opcode_map[opcode][1:]

        # Handle group opcodes, and opcode extensions
        if (mnemonic[:3] == 'GRP'):
            self.read_modrm()
            extension = opcode_extension_map[(mnemonic, self.modrm['reg'])]
            if (extension[0] == '--'):
                # Special case - illegal extension, use the '???' pseudo-mnemonic
                mnemonic = '???'
            else:
                mnemonic = extension[0]
            # Override the primary opcode's argument descriptors iff the extension's set is non-empty
            if (extension[1:]): arguments = extension[1:]

        return mnemonic, arguments

    size_lut = {'b':8, 'v':16, 'w':16}
    def make_argument(self, desc):
        if (desc[0] == 'e'):
            # Register name; has a deeper meaning for post-8086 processors
            return Arg_Register(name=desc[1:])
        elif (reg_set.__getattr__(desc) != None):
            # Register name
            return Arg_Register(name=desc)
        elif (desc in '13'):
            # Constant
            return Arg_Constant(int(desc))
        elif (desc == 'M'):
            # pseudo-dereference
            self.read_modrm()
            rm = self.modrm['rm']; rm.set_type('v')
            return rm
        elif (desc[0] == 'A'):
            offset = self.read_integer(2)
            return Arg_Address(self.read_integer(2), offset)
        elif (desc[0] in 'EM'):
            self.read_modrm()
            rm = self.modrm['rm']; rm.set_type(desc[1])
            return rm
        elif (desc[0] == 'G'):
            self.read_modrm()
            return Arg_Register(code=self.modrm['reg'], type=desc[1])
        elif (desc[0] == 'I'):
            if (desc[1] == '0'):
                # Very special case
                value = self.read_integer(1)
                if (value == 0xa): return
                return Arg_Integer(value, 8)
            else:
                size = self.size_lut[desc[1]]
                return Arg_Integer(self.read_integer(size/8), size)
        elif (desc[0] == 'J'):
            size = self.size_lut[desc[1]]
            return Arg_Offset(self.read_integer(size/8), size)
        elif (desc[0] == 'O'):
            return Arg_Dereference(type=desc[1], disp=self.read_integer(2), disp_size=16)
        elif (desc[0] == 'S'):
            self.read_modrm()
            return Arg_Register(code=self.modrm['reg'], type='S')
        else:
            raise "Unknown argument description %s" % desc

    def read_instruction(self):
        self.modrm = None; start = self.index
        mnemonic, arguments = self.read_opcode()

        address = self.first_addy.calc_relative_address(start)
        if (mnemonic == '--'):
            # Illegal opcode, use the 'DB' pseudo-instruction
            mnemonic = Mnemonic('DB')
            arguments = [Arg_Integer(ord(self.bytes[start]), 8)]
        else:
            mnemonic = Mnemonic(mnemonic)
            arguments = filter(None, map(self.make_argument, arguments))

        code = MachineCode(map(ord, self.bytes[start:self.index]))
        return Instruction(address, code, mnemonic, *arguments)

    def disassemble(self, bytes, segment=0, offset=0, trap=0, quiet=0):
        self.index    = 0
        self.bytes    = bytes
        self.first_addy    = Address(segment, offset)

        # If we're not trapping exceptions, just iterate over the instructions
        if (not trap):
            while (self.index < len(self.bytes)): yield(self.read_instruction())
            return

        # If we are trapping, optionally print an error message pointing to the problem
        while (self.index < len(self.bytes)):
            try:
                start = self.index
                yield(self.read_instruction())
            except:
                if (not quiet): print 'Fault in instruction at %d' % start
                break
                
               
f = open("test\dude", "rb")
bin = f.read()
inst = Disassembler().disassemble(bin)

for x in inst:
    print(x)
