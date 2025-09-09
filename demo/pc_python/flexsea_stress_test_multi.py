import ctypes
from ctypes import *
import time
import sys
import datetime
from utilities import Csv, create_directory
from dataclasses import dataclass
from matplotlib import pyplot as plt

# Add the FlexSEA path to this project
sys.path.append('../../flexsea_python')
from flexsea_python import FlexSEAPython, CommHardware
from flexsea_tools import *
# Note: with PyCharm you must add this folder and mark is as a Sources Folder to avoid an Unresolved Reference issue

# Platform specific shared library and serial port
pf = FlexSEAPython.identify_platform()
if pf == 'WIN':
    dll_filename = '../../projects/eclipse_pc/DynamicLib/libflexsea-v2.dll'
    com_port = 'COM9'
elif pf == 'MAC':
    dll_filename = '../../../flexsea-v2/projects/eclipse_pc/DynamicLib/libflexsea-v2.dylib'
    com_port = '/dev/tty.usbserial-ABCD'  # Default, can be over-ridden by CLI argument
else:
    # ToDo this is likely too generic
    dll_filename = '../../../flexsea-v2/projects/eclipse_pc/DynamicLib/libflexsea-v2.so'
    com_port = '/dev/ttyAMA0'  # Default, can be over-ridden by CLI argument

new_tx_delay_ms = 10  # 10 ms = 100 Hz

MIN_OVERHEAD = 4

# 2 peripherals, each with their own stress test command code (in other instances both will use the same cmd)
STRESS_TEST_CHANNELS = 2
FX_CMD_STRESS_TEST_PERIPH_1 = 2
FX_CMD_STRESS_TEST_PERIPH_2 = 5
RAMP_MAX = 1000
STRESS_TEST_CYCLES = 1000  # Per channel

# Variables used in TX and RX to analyze a loop back
start_time = 0
pc_packet_number = -1
pc_ramp_value = -1
dut_packet_number = [-1, -1]
dut_ramp_value = [-1, -1]
tx_timestamp = [0, 0]

csv_header = [['TX Packet Num', 'TX Ramp Value Num', 'TX ch1 Timestamp (ms)', 'RX ch1 Timestamp (ms)',
               'RX ch1 Packet Num', 'RX ch1 Ramp Value', 'TX ch2 Timestamp (ms)', 'RX ch2 Timestamp (ms)',
               'RX ch2 Packet Num', 'RX ch2 Ramp Value']]
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


stress_test_data_ch1 = []
stress_test_data_ch2 = []


# C-style data structure
# Note: this has to match the embedded code for the serial demo to work!
class FxStressTestStruct(Structure):
    _pack_ = 1
    _fields_ = [("packet_number", c_int32),
                ("ramp_value", c_int16),
                ("reset", c_uint8)]


# Custom command handler used by the stress test code - PC side, Stepper (ch1)
def fx_rx_cmd_handler_2(cmd_6bits, rw, buf):
    # print('PC RX cmd handler 2()')
    rx_data = FxStressTestStruct()
    ctypes.memmove(pointer(rx_data), buf[1:], sizeof(rx_data))
    global stress_test_data_ch1
    global pc_packet_number, pc_ramp_value, tx_timestamp, start_time

    rx_timestamp = round(time.time() * 1000) - start_time
    stress_test_data_ch1.append(StressTestData(tx_timestamp=tx_timestamp[0], rx_timestamp=rx_timestamp,
                                           tx_packet_num=pc_packet_number, rx_packet_num=rx_data.packet_number,
                                           tx_ramp_value=pc_ramp_value, rx_ramp_value=rx_data.ramp_value))


# Custom command handler used by the stress test code - PC side, Power (ch2)
def fx_rx_cmd_handler_5(cmd_6bits, rw, buf):
    # print('PC RX cmd handler 5()')
    rx_data = FxStressTestStruct()
    ctypes.memmove(pointer(rx_data), buf[1:], sizeof(rx_data))
    global stress_test_data_ch2
    global pc_packet_number, pc_ramp_value, tx_timestamp, start_time

    rx_timestamp = round(time.time() * 1000) - start_time
    stress_test_data_ch2.append(StressTestData(tx_timestamp=tx_timestamp[1], rx_timestamp=rx_timestamp,
                                           tx_packet_num=pc_packet_number, rx_packet_num=rx_data.packet_number,
                                           tx_ramp_value=pc_ramp_value, rx_ramp_value=rx_data.ramp_value))


# We create and serialize the test payload
def gen_stress_test_payload(packet_number, ramp_value, reset=0):
    payload = FxStressTestStruct(packet_number=packet_number, ramp_value=ramp_value, reset=reset)
    payload_string = bytes(payload)
    return payload_string


def plot_results():
    plt.figure(figsize=(16, 10))
    # Clear old data, print latest buffer
    plt.clf()

    # Top plot: display according to time, ch1
    plt.subplot(2, 1, 1)
    plot_x = [x.tx_packet_num for x in stress_test_data_ch1]
    plt.plot(plot_x, [x.tx_ramp_value for x in stress_test_data_ch1], label="TX Ramp values")
    plt.plot(plot_x, [x.rx_ramp_value for x in stress_test_data_ch1], label="RX Ramp values")
    plt.title('Ramp in and out - ch1 (Stepper)')
    plt.xlabel('Cycle')
    plt.ylabel('Amplitude (ticks)')
    plt.legend()
    # plt.draw()

    # Bottom plot: display according to time, ch2
    plt.subplot(2, 1, 2)
    plot_x = [x.tx_packet_num for x in stress_test_data_ch2]
    plt.plot(plot_x, [x.tx_ramp_value for x in stress_test_data_ch2], label="TX Ramp values")
    plt.plot(plot_x, [x.rx_ramp_value for x in stress_test_data_ch2], label="RX Ramp values")
    plt.title('Ramp in and out - ch2 (Power)')
    plt.xlabel('Cycle')
    plt.ylabel('Amplitude (ticks)')
    plt.legend()
    plt.draw()

    # Save as PNG
    plt.savefig(f'logs/{file_timestamp}.png')
    plt.show(block=True)  # Blocking until closed


def save_csv_results():
    i = 0
    for cycle in stress_test_data_ch1:
        # We add a line to the CSV file:
        csv_line = [[stress_test_data_ch1[i].tx_packet_num, stress_test_data_ch1[i].tx_ramp_value, 
                    stress_test_data_ch1[i].tx_timestamp, stress_test_data_ch1[i].rx_timestamp,
                    stress_test_data_ch1[i].rx_packet_num, stress_test_data_ch1[i].rx_ramp_value, 
                    stress_test_data_ch2[i].tx_timestamp, stress_test_data_ch2[i].rx_timestamp,
                    stress_test_data_ch2[i].rx_packet_num, stress_test_data_ch2[i].rx_ramp_value]]
        csv.write(csv_line)
        i = i + 1


# Increment and ceil counter and ramp
def counter_and_ramp(cnt, rmp):
    cnt = cnt + 1
    rmp = rmp + 1
    if rmp > RAMP_MAX:
        rmp = 0
    return cnt, rmp


# Dual channel RS-485 stress test
# We alternate between channels at every step
def flexsea_stress_test_serial_multi():

    print('Stress test code - Python project with FlexSEA v2.0 DLL')
    ttrun = STRESS_TEST_CYCLES * STRESS_TEST_CHANNELS * \
            new_tx_delay_ms / 1000
    print(f'This test will take approximately {ttrun:0.2f} s to run.\n')

    # Initialize FlexSEA comm
    fx = []
    fx.append(FlexSEAPython(dll_filename, com_port_name=com_port, channel=0))
    fx.append(FlexSEAPython(dll_filename, open_new_port=False, com_port_name=None, channel=1,
                            existing_port=fx[0].get_serial_port()))

    # Prepare for reception:
    fx[0].register_cmd_handler(FX_CMD_STRESS_TEST_PERIPH_1, fx_rx_cmd_handler_2)
    fx[1].register_cmd_handler(FX_CMD_STRESS_TEST_PERIPH_2, fx_rx_cmd_handler_5)

    comm_hw = CommHardware()

    comm_hw.use_channel(0)  # RS-485 transceiver 0, "ch1" on the silk
    if fx[0].serial.valid_port():
        fx[0].who_am_i()
        comm_hw.use_channel(1)  # RS-485 transceiver 1, "ch2" on the silk
        if fx[1].serial.valid_port():
            fx[1].who_am_i()

    global pc_packet_number, pc_ramp_value, tx_timestamp, start_time, stress_test_data_ch1, stress_test_data_ch2
    pc_packet_number = -1
    pc_ramp_value = -1
    start_time = round(time.time() * 1000)
    stress_test_data_ch1 = []  # Start with empty structure
    stress_test_data_ch2 = []  # Start with empty structure

    # Reset embedded counters
    p_string = gen_stress_test_payload(0, 0, reset=1)
    comm_hw.use_channel(0)
    ret_val, bytestream, bytestream_len = fx[0].create_bytestream_from_cmd(cmd=FX_CMD_STRESS_TEST_PERIPH_1,
                                                                           rw="CmdWrite", payload_string=p_string)
    fx[0].serial.write(bytestream, bytestream_len)
    time.sleep(0.01)

    comm_hw.use_channel(1)
    ret_val, bytestream, bytestream_len = fx[1].create_bytestream_from_cmd(cmd=FX_CMD_STRESS_TEST_PERIPH_2,
                                                                           rw="CmdWrite", payload_string=p_string)
    fx[1].serial.write(bytestream, bytestream_len)
    time.sleep(0.01)

    for i in range(STRESS_TEST_CYCLES):

        print(f'Cycle #{i}')

        # PC generates bytestreams:
        p_string = gen_stress_test_payload(pc_packet_number, pc_ramp_value)
        pc_packet_number, pc_ramp_value = counter_and_ramp(pc_packet_number, pc_ramp_value)
        ret_val, bytestream_ch1, bytestream_len_ch1 = fx[0].create_bytestream_from_cmd(cmd=FX_CMD_STRESS_TEST_PERIPH_1,
                                                                                       rw="CmdReadWrite",
                                                                                       payload_string=p_string)
        ret_val, bytestream_ch2, bytestream_len_ch2 = fx[1].create_bytestream_from_cmd(cmd=FX_CMD_STRESS_TEST_PERIPH_2,
                                                                                       rw="CmdReadWrite",
                                                                                       payload_string=p_string)

        # Send a packet, listen for a reply
        comm_hw.use_channel(0)
        current_time = round(time.time() * 1000)
        tx_timestamp[0] = current_time - start_time
        fx[0].rw_one_packet(bytestream_ch1, bytestream_len_ch1, start_time, callback=None,
                          comm_wait=new_tx_delay_ms)
        # time.sleep(0.1)

        # Send a packet, listen for a reply
        comm_hw.use_channel(1)
        current_time = round(time.time() * 1000)
        tx_timestamp[1] = current_time - start_time
        fx[1].rw_one_packet(bytestream_ch2, bytestream_len_ch2, start_time, callback=None,
                          comm_wait=new_tx_delay_ms)
        # time.sleep(0.1)

    # Grab last bytes:
    time.sleep(0.1)
    comm_hw.use_channel(0)
    fx[0].receive()
    time.sleep(0.1)
    comm_hw.use_channel(1)
    fx[1].receive()

    # Print summary:
    print(f'Packets sent: {pc_packet_number + 1}')
    print(f'Packets received ch1: {len(stress_test_data_ch1)} (last decoded packet count is '
          f'{stress_test_data_ch1[-1].rx_packet_num})')
    print(f'Packets received ch2: {len(stress_test_data_ch2)} (last decoded packet count is '
          f'{stress_test_data_ch2[-1].rx_packet_num})')
    end_time = round(time.time() * 1000)
    test_time_s = (end_time - start_time) / 1000
    print(f'Test time: {test_time_s:0.2f} s')

    plot_results()
    save_csv_results()


if __name__ == "__main__":

    file_timestamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    create_directory('logs')
    csv = Csv(f'logs/stress-test-{file_timestamp}.csv', csv_header)

    flexsea_stress_test_serial_multi()
