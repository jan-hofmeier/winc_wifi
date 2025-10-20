"""
Low-level SPI decoder for WINC1500 communication.
"""

# ... (rest of the constants are the same)
CMD_DMA_WRITE = 0xc1
CMD_DMA_READ = 0xc2
CMD_INTERNAL_WRITE = 0xc3
CMD_INTERNAL_READ = 0xc4
CMD_TERMINATE = 0xc5
CMD_REPEAT = 0xc6
CMD_WRITE_DATA = 0xc7
CMD_READ_DATA = 0xc8
CMD_SINGLE_WRITE = 0xc9
CMD_SINGLE_READ = 0xca
CMD_RESET = 0xcf

COMMANDS = {
    CMD_DMA_WRITE: "CMD_DMA_WRITE",
    CMD_DMA_READ: "CMD_DMA_READ",
    CMD_INTERNAL_WRITE: "CMD_INTERNAL_WRITE",
    CMD_INTERNAL_READ: "CMD_INTERNAL_READ",
    CMD_TERMINATE: "CMD_TERMINATE",
    CMD_REPEAT: "CMD_REPEAT",
    CMD_WRITE_DATA: "CMD_WRITE_DATA",
    CMD_READ_DATA: "CMD_READ_DATA",
    CMD_SINGLE_WRITE: "CMD_SINGLE_WRITE",
    CMD_SINGLE_READ: "CMD_SINGLE_READ",
    CMD_RESET: "CMD_RESET",
}

REGISTERS = {
    0x1000: "CHIPID_REG",
    0x1014: "EFUSE_REG",
    0x106c: "RCV_CTRL_REG3",
    0x1070: "RCV_CTRL_REG0",
    0x1078: "RCV_CTRL_REG2",
    0x1084: "RCV_CTRL_REG1",
    0x1088: "RCV_CTRL_REG5",
    0x108c: "NMI_STATE_REG",
    0x13f4: "REVID_REG",
    0x1408: "PIN_MUX_REG0",
    0x14a0: "NMI_GP_REG1",
    0x1a00: "NMI_EN_REG",
    0xE824: "SPI_CFG_REG",
    0x207bc: "HOST_WAIT_REG",
    0xc0008: "NMI_GP_REG2",
    0xc000c: "BOOTROM_REG",
    0x150400: "RCV_CTRL_REG4",
}


def u24_to_int_be(data):
    return (data[0] << 16) | (data[1] << 8) | data[2]

def u32_to_int_be(data):
    return (data[0] << 24) | (data[1] << 16) | (data[2] << 8) | data[3]

def u32_to_int_le(data):
    return data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24)

class SpiTransaction:
    def __init__(self, pos, mosi, miso):
        self.pos = pos
        self.mosi = mosi
        self.miso = miso
        self.command = mosi[0]
        self.command_str = COMMANDS.get(self.command, f"UNKNOWN (0x{self.command:02x})")

    def __str__(self):
        return f"[{self.pos}] {self.command_str}"

class SingleRead(SpiTransaction):
    def __init__(self, pos, mosi, miso):
        super().__init__(pos, mosi, miso)
        addr_bytes = mosi[1:4]
        self.addr = u24_to_int_be(addr_bytes)
        self.addr_str = REGISTERS.get(self.addr, f"0x{self.addr:04x}")

        # Response parsing
        self.response_cmd = miso[4]
        self.response_zero = miso[5]
        self.response_status = miso[6]

        if self.response_cmd == self.command and self.response_zero == 0 and (self.response_status & 0xf0) == 0xf0:
            val_bytes = miso[7:11]
            self.value = u32_to_int_le(val_bytes)
            self.response_valid = True
        else:
            self.value = None
            self.response_valid = False

    def __str__(self):
        s = super().__str__() + f"\n  MOSI -> Addr: {self.addr_str}"
        if self.response_valid:
            s += f"\n  MISO <- Value: 0x{self.value:08x}"
        else:
            s += f"\n  MISO <- Invalid response!"
        return s

class SingleWrite(SpiTransaction):
    def __init__(self, pos, mosi, miso):
        super().__init__(pos, mosi, miso)
        addr_bytes = mosi[1:4]
        self.addr = u24_to_int_be(addr_bytes)
        self.addr_str = REGISTERS.get(self.addr, f"0x{self.addr:04x}")
        val_bytes = mosi[4:8]
        self.value = u32_to_int_be(val_bytes)

        # Response parsing
        self.response_cmd = miso[8]
        self.response_zero = miso[9]

        if self.response_cmd == self.command and self.response_zero == 0:
            self.response_valid = True
        else:
            self.response_valid = False

    def __str__(self):
        s = super().__str__() + f"\n  MOSI -> Addr: {self.addr_str}, Value: 0x{self.value:08x}"
        if self.response_valid:
            s += "\n  MISO <- ACK"
        else:
            s += "\n  MISO <- Invalid response!"
        return s
