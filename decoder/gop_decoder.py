"""
High-level GOP (Generic Operation Packet) decoder for WINC1500 communication.
"""
import struct
import spi_decoder

# ... (GOP constants are the same)
# Host Interface (HIF) Group IDs from winc_wifi.h
GID_MAIN = 0
GID_WIFI = 1
GID_IP = 2
GID_HIF = 3

GROUP_IDS = {
    GID_MAIN: "GID_MAIN",
    GID_WIFI: "GID_WIFI",
    GID_IP: "GID_IP",
    GID_HIF: "GID_HIF",
}

# Host Interface operations from winc_wifi.h
def GIDOP(gid, op):
    return (gid << 8) | op

GOP_CONN_REQ_OLD = GIDOP(GID_WIFI, 40)
GOP_STATE_CHANGE = GIDOP(GID_WIFI, 44)
GOP_DHCP_CONF = GIDOP(GID_WIFI, 50)
GOP_CONN_REQ_NEW = GIDOP(GID_WIFI, 59)
GOP_BIND = GIDOP(GID_IP, 65)
GOP_LISTEN = GIDOP(GID_IP, 66)
GOP_ACCEPT = GIDOP(GID_IP, 67)
GOP_SEND = GIDOP(GID_IP, 69)
GOP_RECV = GIDOP(GID_IP, 70)
GOP_SENDTO = GIDOP(GID_IP, 71)
GOP_RECVFROM = GIDOP(GID_IP, 72)
GOP_CLOSE = GIDOP(GID_IP, 73)

REQ_DATA = 0x80

OPERATIONS = {
    GOP_CONN_REQ_OLD: "GOP_CONN_REQ_OLD",
    GOP_STATE_CHANGE: "GOP_STATE_CHANGE",
    GOP_DHCP_CONF: "GOP_DHCP_CONF",
    GOP_CONN_REQ_NEW: "GOP_CONN_REQ_NEW",
    GOP_BIND: "GOP_BIND",
    GOP_LISTEN: "GOP_LISTEN",
    GOP_ACCEPT: "GOP_ACCEPT",
    GOP_SEND: "GOP_SEND",
    GOP_RECV: "GOP_RECV",
    GOP_SENDTO: "GOP_SENDTO",
    GOP_RECVFROM: "GOP_RECVFROM",
    GOP_CLOSE: "GOP_CLOSE",
}

def parse_bind_cmd(payload):
    payload_bytes = bytes(payload)
    family, port, ip, sock, _, session = struct.unpack(">HHIBBH", payload_bytes)
    ip_str = f"{(ip >> 24) & 0xff}.{(ip >> 16) & 0xff}.{(ip >> 8) & 0xff}.{ip & 0xff}"
    print(f"    - BIND_CMD: family={family}, port={port}, ip={ip_str}, sock={sock}, session={session}")

def parse_state_change(payload):
    state = "Connected" if payload[0] == 1 else "Disconnected"
    print(f"    - STATE_CHANGE: {state}")

def parse_dhcp_conf(payload):
    # This is a simplification. The C struct is more complex.
    ip = payload[0:4]
    ip_str = f"{ip[3]}.{ip[2]}.{ip[1]}.{ip[0]}"
    print(f"    - DHCP_CONF: IP Address: {ip_str}")


GOP_PARSERS = {
    GOP_BIND: parse_bind_cmd,
    GOP_STATE_CHANGE: parse_state_change,
    GOP_DHCP_CONF: parse_dhcp_conf,
}


def find_gop_start(data):
    """Finds the start of a GOP in a data stream by looking for a valid GID."""
    for i, byte in enumerate(data):
        if byte in GROUP_IDS:
            if i + 1 < len(data):
                gop = GIDOP(byte, data[i+1] & 0x7f)
                if gop in OPERATIONS:
                    return i
    return -1


def decode_gop(data, direction):
    """Decodes a single GOP from a data payload."""
    start = find_gop_start(data)
    if start == -1: return

    data = data[start:]
    gid = data[0]
    op = data[1]
    gop = GIDOP(gid, op & 0x7f)
    gop_str = OPERATIONS.get(gop, f"Unknown GOP (0x{gop:04x})")
    is_req = " (REQ_DATA)" if (op & REQ_DATA) else ""

    print(f"  {direction} GOP -> @{start} {gop_str}{is_req}")

    payload = data[8:] # Skip HIF header
    if gop in GOP_PARSERS:
        GOP_PARSERS[gop](payload)


def decode_full_stream(mosi, miso):
    mosi_pos = 0
    miso_pos = 0

    while mosi_pos < len(mosi):
        cmd = mosi[mosi_pos]

        if cmd == spi_decoder.CMD_SINGLE_READ:
            if mosi_pos + 11 > len(mosi): break
            tx = spi_decoder.SingleRead(mosi_pos, mosi[mosi_pos : mosi_pos + 11])
            miso_pos = tx.find_and_parse_response(miso, miso_pos)
            print(tx)
            mosi_pos += 11

        elif cmd == spi_decoder.CMD_SINGLE_WRITE:
            if mosi_pos + 11 > len(mosi): break
            tx = spi_decoder.SingleWrite(mosi_pos, mosi[mosi_pos : mosi_pos + 11])
            miso_pos = tx.find_and_parse_response(miso, miso_pos)
            print(tx)
            mosi_pos += 11

        elif cmd == spi_decoder.CMD_WRITE_DATA:
            if mosi_pos + 7 > len(mosi): break
            addr = spi_decoder.u24_to_int_be(mosi[mosi_pos+1:mosi_pos+4])
            count = spi_decoder.u24_to_int_be(mosi[mosi_pos+4:mosi_pos+7])
            print(f"[{mosi_pos}] CMD_WRITE_DATA\n  MOSI -> Addr: 0x{addr:04x}, Count: {count}")

            data_start = mosi_pos + 10
            if data_start + count > len(mosi): break

            payload = mosi[data_start : data_start + count]
            decode_gop(payload, "MOSI")
            mosi_pos = data_start + count

        elif cmd == spi_decoder.CMD_READ_DATA:
            if mosi_pos + 7 > len(mosi): break
            addr = spi_decoder.u24_to_int_be(mosi[mosi_pos+1:mosi_pos+4])
            count = spi_decoder.u24_to_int_be(mosi[mosi_pos+4:mosi_pos+7])
            print(f"[{mosi_pos}] CMD_READ_DATA\n  MOSI -> Addr: 0x{addr:04x}, Count: {count}")

            # The GOP response is in the MISO stream. We can't know the exact
            # length, so we'll just look for a GOP in the upcoming bytes.
            search_area = miso[miso_pos : miso_pos + count + 100] # Heuristic search window
            decode_gop(search_area, "MISO")
            mosi_pos += 7

        else:
            mosi_pos += 1
