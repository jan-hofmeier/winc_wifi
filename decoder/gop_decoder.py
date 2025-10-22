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

def parse_conn_req_old(payload):
    payload_bytes = bytes(payload)
    psk = payload_bytes[0:65].partition(b'\0')[0].decode('ascii', errors='ignore')
    typ = payload_bytes[65]
    # Note: C struct packing may cause offsets to vary. This is a best guess.
    chan = struct.unpack('>H', payload_bytes[68:70])[0]
    ssid = payload_bytes[70:103].partition(b'\0')[0].decode('ascii', errors='ignore')
    nosave = payload_bytes[103]

    auth_type = "AUTH_PSK" if typ == 2 else "AUTH_OPEN" if typ == 1 else f"Unknown ({typ})"
    print(f"    - OLD_CONN_HDR:")
    print(f"      - SSID: '{ssid}'")
    print(f"      - PSK: '{(psk)}'")
    print(f"      - Auth Type: {auth_type}")
    print(f"      - Channel: {chan}")
    print(f"      - No Save: {nosave}")

def parse_conn_req_new(payload):
    payload_bytes = bytes(payload)
    cred_size, flags, chan, ssid_len = struct.unpack_from('>HBBB', payload_bytes)
    ssid_bytes = payload_bytes[5:5+ssid_len]
    ssid = ssid_bytes.decode('ascii', errors='ignore')
    auth = payload_bytes[44]
    auth_type = "AUTH_PSK" if auth == 2 else "AUTH_OPEN" if auth == 1 else f"Unknown ({auth})"

    print(f"    - CONN_HDR:")
    print(f"      - SSID: '{ssid}' (len={ssid_len})")
    print(f"      - Auth Type: {auth_type}")
    print(f"      - Channel: {chan}")
    print(f"      - Flags: 0x{flags:02x}")
    print(f"      - Credential Size: {cred_size}")

    if cred_size > 48: # PSK data follows
        psk_data = payload_bytes[48:]
        psk_len = psk_data[0]
        psk = psk_data[1:1+psk_len].decode('ascii', errors='ignore')
        print(f"    - PSK_DATA:")
        print(f"      - PSK: '{psk}' (len={psk_len})")

def parse_send_cmd(payload):
    # SEND_CMD is 4 bytes, followed by data
    sock, _, dlen = struct.unpack_from('>BBH', bytes(payload))
    data = payload[4:4+dlen]
    print(f"    - SEND_CMD: sock={sock}, dlen={dlen}")
    print(f"      - Data: {bytes(data[:16]).hex()}...")


GOP_PARSERS = {
    GOP_BIND: parse_bind_cmd,
    GOP_STATE_CHANGE: parse_state_change,
    GOP_DHCP_CONF: parse_dhcp_conf,
    GOP_CONN_REQ_OLD: parse_conn_req_old,
    GOP_CONN_REQ_NEW: parse_conn_req_new,
    GOP_SEND: parse_send_cmd,
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


def decode_full_stream(mosi, miso, verbose=False):
    mosi_pos = 0
    miso_pos = 0

    while mosi_pos < len(mosi):
        cmd = mosi[mosi_pos]

        if cmd == spi_decoder.CMD_SINGLE_READ or cmd == spi_decoder.CMD_INTERNAL_READ:
            if mosi_pos + 11 > len(mosi): break
            tx = spi_decoder.SingleRead(mosi_pos, mosi[mosi_pos : mosi_pos + 11])
            miso_pos = tx.find_and_parse_response(miso, miso_pos)
            if verbose:
                print(tx)
            mosi_pos += 11

        elif cmd == spi_decoder.CMD_SINGLE_WRITE or cmd == spi_decoder.CMD_INTERNAL_WRITE:
            if mosi_pos + 10 > len(mosi): break
            tx = spi_decoder.SingleWrite(mosi_pos, mosi[mosi_pos : mosi_pos + 11])
            miso_pos = tx.find_and_parse_response(miso, miso_pos)
            if verbose:
                print(tx)
            mosi_pos += 9

        elif cmd == spi_decoder.CMD_WRITE_DATA:
            if mosi_pos + 7 > len(mosi): break
            addr = spi_decoder.u24_to_int_be(mosi[mosi_pos+1:mosi_pos+4])
            count = spi_decoder.u24_to_int_be(mosi[mosi_pos+4:mosi_pos+7])

            data_start = mosi_pos + 10
            if data_start + count > len(mosi): break

            payload = mosi[data_start : data_start + count]

            is_gop_header = False
            if count == 8 and len(payload) > 1:
                gid = payload[0]
                op = payload[1]
                gop = GIDOP(gid, op & 0x7f)
                if gid in GROUP_IDS and gop in OPERATIONS:
                    is_gop_header = True

            if is_gop_header:
                if verbose:
                    print(f"[{mosi_pos}] CMD_WRITE_DATA (GOP Header)\n  MOSI -> Addr: 0x{addr:04x}, Count: {count}")
                hif_len = payload[2] | (payload[3] << 8)
                payload_len_expected = hif_len - 8

                full_payload = list(payload)

                temp_mosi_pos = data_start + count
                payload_len_collected = 0

                while payload_len_collected < payload_len_expected and temp_mosi_pos < len(mosi):
                    start_of_cmd = temp_mosi_pos
                    while start_of_cmd < len(mosi) and mosi[start_of_cmd] == 0:
                        start_of_cmd += 1

                    if start_of_cmd >= len(mosi): break

                    next_cmd = mosi[start_of_cmd]
                    if next_cmd == spi_decoder.CMD_WRITE_DATA:
                        if start_of_cmd + 7 > len(mosi): break
                        next_addr = spi_decoder.u24_to_int_be(mosi[start_of_cmd+1:start_of_cmd+4])
                        next_count = spi_decoder.u24_to_int_be(mosi[start_of_cmd+4:start_of_cmd+7])
                        if verbose:
                            print(f"[{start_of_cmd}] CMD_WRITE_DATA (GOP Payload)\n  MOSI -> Addr: 0x{next_addr:04x}, Count: {next_count}")

                        next_data_start = start_of_cmd + 10
                        if next_data_start + next_count > len(mosi): break

                        data_chunk = mosi[next_data_start : next_data_start + next_count]
                        full_payload.extend(data_chunk)
                        payload_len_collected += next_count

                        temp_mosi_pos = next_data_start + next_count
                    else:
                        break

                decode_gop(full_payload, "MOSI")
                mosi_pos = temp_mosi_pos

            else:
                if verbose:
                    print(f"[{mosi_pos}] CMD_WRITE_DATA\n  MOSI -> Addr: 0x{addr:04x}, Count: {count}")
                decode_gop(payload, "MOSI")
                mosi_pos = data_start + count

        elif cmd == spi_decoder.CMD_READ_DATA:
            if mosi_pos + 7 > len(mosi): break
            addr = spi_decoder.u24_to_int_be(mosi[mosi_pos+1:mosi_pos+4])
            count = spi_decoder.u24_to_int_be(mosi[mosi_pos+4:mosi_pos+7])
            if verbose:
                print(f"[{mosi_pos}] CMD_READ_DATA\n  MOSI -> Addr: 0x{addr:04x}, Count: {count}")

            search_area = miso[miso_pos : miso_pos + count + 100]
            decode_gop(search_area, "MISO")
            mosi_pos += 7

        elif cmd == spi_decoder.CMD_RESET:
            if verbose:
                print(f"[{mosi_pos}] CMD_RESET")
            mosi_pos += 1

        elif cmd == 0:
            mosi_pos += 1

        else:
            print(f"Unknown CMD 0x{cmd:x} at {mosi_pos}. miso_pos: {miso_pos}")
            exit(-1)
