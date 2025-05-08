import ctypes
from ctypes import *
import serial
import time
import sys
import datetime
from utilities import Csv, create_directory
from dataclasses import dataclass


# Add the FlexSEA path to this project
sys.path.append('../../')
from flexsea_python import FlexSEAPython
from flexsea_tools import *
# Note: with PyCharm you must add this folder and mark is as a Sources Folder to avoid an Unresolved Reference issue

dll_filename = '../../projects/eclipse_pc/DynamicLib/libflexsea-v2.dll'
com_port = 'COM20'
serial_port = 0  # Holds the serial port object
new_tx_delay_ms = 40

FX_CMD_STRESS_TEST = 2
RAMP_MAX = 1000
STRESS_TEST_CYCLES = 10

# Variables used in TX and RX to analyze a loop back
start_time = 0
tx_packet_number = -1
tx_ramp_value = -1
tx_timestamp = 0

csv_header = [['TX Timestamp (ms)', 'RX Timestamp (ms)', 'TX Packet Num', 'RX Packet Num', 'TX Ramp Value Num',
               'RX Ramp Value']]
csv = ''


@dataclass
class StressTestData:
    """
    Class that will hold the content of the latest test
    """
    tx_timestamp: int
    rx_timestamp: int
    tx_packet_num: int
    rx_packet_num: int
    tx_ramp_value: int
    rx_ramp_value: int

    def __init__(self, tx_timestamp=0, rx_timestamp=0, tx_packet_num=0, rx_packet_num=0, tx_ramp_value=0,
                 rx_ramp_value=0):
        self.tx_timestamp = tx_timestamp
        self.rx_timestamp = rx_timestamp
        self.tx_packet_num = tx_packet_num
        self.rx_packet_num = rx_packet_num
        self.tx_ramp_value = tx_ramp_value
        self.rx_ramp_value = rx_ramp_value


stress_test_data = []


# Open serial port
def serial_open(com_port_name):
    global serial_port
    serial_port = serial.Serial(com_port_name, baudrate=115200)
    if serial_port:
        print(f'Successfully opened {com_port_name}.')
        return 0
    else:
        print(f'Could not open {com_port_name}!')
        return 1


# Write to a previously opened serial port
def serial_write(bytestream):
    if serial_port:
        serial_port.write(bytestream)
    else:
        print("No serial port object, can't write!")


# C-style data structure
# Note: this has to match the embedded code for the serial demo to work!
class FxStressTestStruct(Structure):
    _pack_ = 1
    _fields_ = [("packet_number", c_uint32),
                ("ramp_value", c_uint16)]


# Custom command handler used by the stress test code
def fx_rx_cmd_handler_2(cmd_6bits, rw, buf):
    rx_data = FxStressTestStruct()
    ctypes.memmove(pointer(rx_data), buf[1:], sizeof(rx_data))
    global stress_test_data
    global tx_packet_number, tx_ramp_value, tx_timestamp, start_time

    rx_timestamp = round(time.time() * 1000) - start_time
    stress_test_data.append(StressTestData(tx_timestamp=tx_timestamp, rx_timestamp=rx_timestamp,
                                           tx_packet_num=tx_packet_number, rx_packet_num=rx_data.packet_number,
                                           tx_ramp_value=tx_ramp_value, rx_ramp_value=rx_data.ramp_value))


# We create and serialize the test payload
def gen_stress_test_payload(packet_number, ramp_value):
    payload = FxStressTestStruct(packet_number=packet_number, ramp_value=ramp_value)
    payload_string = bytes(payload)
    return payload_string


# Loopback demo: we create a bytestream, shuffle it around, then decode it
# No serial port required, no interaction with any other system: pure software loopback
def flexsea_stress_test_local_loopback():

    print('Stress Test Code - Python project with FlexSEA v2.0 DLL')
    print('Local Loopback - No external interaction\n')

    # Initialize FlexSEA comm
    fx = FlexSEAPython(dll_filename)

    # Prepare for reception:
    fx.register_cmd_handler(FX_CMD_STRESS_TEST, fx_rx_cmd_handler_2)

    global tx_packet_number, tx_ramp_value, tx_timestamp, start_time
    tx_packet_number = -1
    tx_ramp_value = -1
    start_time = round(time.time() * 1000)

    for i in range(STRESS_TEST_CYCLES):

        # Generate bytestream:
        tx_packet_number = tx_packet_number + 1
        tx_ramp_value = tx_ramp_value + 1
        if tx_ramp_value > RAMP_MAX:
            tx_ramp_value = 0
        ret_val, bytestream, bytestream_len = fx.create_bytestream_from_cmd(cmd=FX_CMD_STRESS_TEST, rw="CmdReadWrite",
                                                                            payload_string=gen_stress_test_payload(
                                                                                tx_packet_number, tx_ramp_value))
        if ret_val:
            print("We did not successfully create a bytestream. Quit.")
            exit()

        # Feed bytestream to circular buffer
        tx_timestamp = round(time.time() * 1000) - start_time
        fx.write_to_circular_buffer(bytestream, bytestream_len)

        # At this point our encoded command is in the circular buffer. Can we decode it?
        ret_val, cmd_6bits_out, rw_out, buf, buf_len = fx.get_cmd_handler_from_bytestream()
        if ret_val:
            print("We did not successfully get a command handler. Quit.")
            exit()

        # Call handler:
        fx.call_cmd_handler(cmd_6bits_out, rw_out, buf)
        rx_time = round(time.time() * 1000) - start_time

    print(f'Packets sent: {tx_packet_number + 1}')
    print(f'Packets received: {len(stress_test_data)}')
    print('Done stressing.')


# Serial demo: we create and send commands to a serial peripheral
# (typically an STM32). Our peripheral will send a reply.
# This code will run until you stop it.
def flexsea_stress_test_serial():

    print('Demo code - Python project with FlexSEA v2.0 DLL')
    print('Serial TX - Connect a Nucleo first!\n')

    # Initialize FlexSEA comm
    fx = FlexSEAPython(dll_filename)

    # Configure serial port
    ret_val = serial_open(com_port)
    if ret_val:
        print(f"Problem opening the {com_port} serial port - quit.")
        exit()

    # Prepare for reception:
    fx.register_cmd_handler(FX_CMD_STRESS_TEST, fx_rx_cmd_handler_2)

    # Generate bytestream from text string (payload):
    ret_val, bytestream, bytestream_len = fx.create_bytestream_from_cmd(cmd=1, rw="CmdReadWrite",
                                                                        payload_string=gen_stress_test_payload())

    if not ret_val:
        print("We successfully created a bytestream.")
    else:
        print("We did not successfully create a bytestream. Quit.")
        exit()

    # Flush any old RX bytes
    serial_port.reset_input_buffer()

    # Send bytestream to serial port
    serial_write(bytestream)
    send_new_tx_cmd = False
    send_new_tx_cmd_timestamp = round(time.time() * 1000)

    # Send a packet, and wait for a reception
    try:
        while True:

            # Send a new packet after a brief delay
            current_time = round(time.time() * 1000)
            if send_new_tx_cmd and current_time > send_new_tx_cmd_timestamp + new_tx_delay_ms:
                # Send bytestream to serial port
                serial_write(bytestream)
                send_new_tx_cmd = False

            # If we got out of sync we kick it off here
            if current_time > send_new_tx_cmd_timestamp + 3 * new_tx_delay_ms:
                print('We got out of sync... figure out why.')
                # Send bytestream to serial port
                serial_write(bytestream)

            # Feed any received bytes into the circular buffer
            bytes_to_read = serial_port.in_waiting
            if bytes_to_read > 0:
                print(f'Bytes to read: {bytes_to_read}.')
                for i in range(bytes_to_read):
                    new_rx_byte = serial_port.read(1)
                    ret_val = fx.write_to_circular_buffer(new_rx_byte[0], 1)
                    if ret_val:
                        print("circ_buf_write_byte() problem!")
                        exit()

            # At this point received commands are in the circular buffer. Can we decode them?
            ret_val, cmd_6bits_out, rw_out, buf, buf_len = fx.get_cmd_handler_from_bytestream()
            if not ret_val:
                print("We successfully got a command handler from a bytestream.")
                print(f'Command handler: {cmd_6bits_out}')
                print(f'R/W type: {rw_out}')
                # Call handler:
                fx.call_cmd_handler(cmd_6bits_out, rw_out, buf)
                send_new_tx_cmd = True
                send_new_tx_cmd_timestamp = round(time.time() * 1000)

            time.sleep(0.01)

    except KeyboardInterrupt:
        print('Interrupted! End or demo code.')


if __name__ == "__main__":

    file_timestamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    create_directory('logs')
    csv = Csv(f'logs/stress-test-{file_timestamp}.csv', csv_header)

    # Available demos, select one or more:
    flexsea_stress_test_local_loopback()
    # print('\n=-=-=-=-=\n')
    # flexsea_stress_test_serial()
