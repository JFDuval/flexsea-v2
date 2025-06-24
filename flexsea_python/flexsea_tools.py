"""Data conversion and manipulation tools"""

import struct
import platform


# Converts a byte into an unsigned int, uint8
def byte_to_uint8(data_byte):
    int_val = int(data_byte)
    return int_val


# Converts an uint8 to a byte
def uint8_to_byte(data_uint8):
    return data_uint8.to_bytes(length=1, byteorder='big')


# Converts a byte into a signed int, int8
def byte_to_int8(data_byte):
    int_val = int(data_byte)
    if int_val > 127:
        int_val = int_val - 256
    return int_val


# Converts an int8 to a byte
def int8_to_byte(data_int8):
    return data_int8.to_bytes(length=1, byteorder='big', signed=True)


# Converts two bytes into an unsigned int, uint16
def bytes_to_uint16(data_bytes):
    int_val = int.from_bytes(data_bytes, 'little')
    return int_val


# Converts an uint16 to bytes
def uint16_to_bytes(data_uint16):
    return data_uint16.to_bytes(length=2, byteorder='little')


# Converts two bytes into a signed int, int16
def bytes_to_int16(data_bytes):
    int_val = int.from_bytes(data_bytes, 'little')
    if int_val > 32767:
        int_val = int_val - 65536
    return int_val


# Converts an int16 to bytes
def int16_to_bytes(data_int16):
    return data_int16.to_bytes(length=2, byteorder='little', signed=True)


# Converts four bytes into an unsigned int, uint32
def bytes_to_uint32(data_bytes):
    int_val = int.from_bytes(data_bytes, 'little')
    return int_val


# Converts an uint32 to bytes
def uint32_to_bytes(data_uint32):
    return data_uint32.to_bytes(length=4, byteorder='little')


# Converts four bytes into a signed int, int32
def bytes_to_int32(data_bytes):
    int_val = int.from_bytes(data_bytes, 'little')
    if int_val > 2147483647:
        int_val = int_val - 4294967296
    return int_val


# Converts an int32 to bytes
def int32_to_bytes(data_int32):
    return data_int32.to_bytes(length=4, byteorder='little', signed=True)


# Converts four bytes into a float
def bytes_to_float(data_bytes):
    int_val = struct.unpack('f', data_bytes)[0]
    return int_val


# Converts a float to bytes
def float_to_bytes(data_float):
    return bytearray(struct.pack("f", data_float))


# Compare the content of two ctype structures, field by field
# Return True if identical, False if any difference is detected
def identical_ctype_structs(struct_a, struct_b):
    if type(struct_a) != type(struct_b):
        return False
    for field_name, _ in struct_a._fields_:
        att_a = getattr(struct_a, field_name)
        att_b = getattr(struct_b, field_name)
        if att_a != att_b:
            return False
    return True

# What computer hardware is this code running on?
def identify_platform():
    pf_string = platform.platform()  # This returns a long string with OS and CPU info
    pf = 'Unknown'
    if pf_string.find('rpi') != -1:  # Ex: "Linux-6.12.25+rpt-rpi-2712-aarch64-with-glibc2.36"
        pf = "RPI"
    elif pf_string.find('Windows') != -1:  # Ex.: "Windows-11-10.0.26100-SP0"
        pf = "WIN"
    elif pf_string.find('MacOS') != -1:  # Ex.: "macOS-15.3.2-arm64-arm-64bit"
        pf = "MAC"
    else:
        pf = "LINUX"
        # By elimination, this is a Linux desktop... but it could be BSD or anything else!

    return pf
