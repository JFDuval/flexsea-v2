import struct
from ctypes import *

# Data conversion tools


# Converts a byte into an unsigned int, uint8
def byte_to_uint8(data_byte):
    int_val = int(data_byte)
    return int_val


# Converts a byte into a signed int, int8
def byte_to_int8(data_byte):
    int_val = int(data_byte)
    if int_val > 127:
        int_val = int_val - 256
    return int_val


# Converts two bytes into an unsigned int, uint16
def bytes_to_uint16(data_byte):
    int_val = int.from_bytes(data_byte, 'little')
    return int_val


# Converts two bytes into a signed int, int16
def bytes_to_int16(data_byte):
    int_val = int.from_bytes(data_byte, 'little')
    if int_val > 32767:
        int_val = int_val - 65536
    return int_val


# Converts four bytes into an unsigned int, uint32
def bytes_to_uint32(data_byte):
    int_val = int.from_bytes(data_byte, 'little')
    return int_val


# Converts four bytes into a signed int, int32
def bytes_to_int32(data_byte):
    int_val = int.from_bytes(data_byte, 'little')
    if int_val > 2147483647:
        int_val = int_val - 4294967296
    return int_val


# Converts four bytes into a float
def bytes_to_float(data_byte):
    int_val = struct.unpack('f', data_byte)[0]
    return int_val
