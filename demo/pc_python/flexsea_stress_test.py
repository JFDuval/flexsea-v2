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
import flexsea_python
from flexsea_python import FlexSEAPython, CommHardware
from flexsea_tools import *
# Note: with PyCharm you must add this folder and mark is as a Sources Folder to avoid an Unresolved Reference issue

# Platform specific shared library and serial port
pf = FlexSEAPython.identify_platform()
if pf == 'WIN':
    dll_filename = '../../projects/eclipse_pc/DynamicLib/libflexsea-v2.dll'
    com_port = 'COM6'
elif pf == 'MAC':
    dll_filename = '../../projects/eclipse_pc/DynamicLib/libflexsea-v2.dylib'
    com_port = '/dev/tty.usbserial-ABCD'  # Default, can be over-ridden by CLI argument
else:
    # ToDo this is likely too generic
    dll_filename = '../../projects/eclipse_pc/DynamicLib/libflexsea-v2.so'
    com_port = '/dev/ttyAMA0'  # Default, can be over-ridden by CLI argument

new_tx_delay_ms = 20  # 10 ms = 100 Hz

FX_CMD_STRESS_TEST_PERIPH_1 = 3
FX_CMD_STRESS_TEST_PERIPH_2 = 4
RAMP_MAX = 1000
STRESS_TEST_CYCLES = 2500

# Variables used in TX and RX to analyze a loop back
start_time = 0
pc_packet_number = -1
pc_ramp_value = -1
dut_packet_number = -1
dut_ramp_value = -1
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


# C-style data structure
# Note: this has to match the embedded code for the serial demo to work!
class FxStressTestStruct(Structure):
    _pack_ = 1
    _fields_ = [("packet_number", c_int32),
                ("ramp_value", c_int16),
                ("reset", c_uint8)]


# Custom command handler used by the stress test code - PC side
def fx_rx_cmd_handler_3_pc(cmd_6bits, rw, ack, buf):
    # print(f'PC RX cmd handler()')
    rx_data = FxStressTestStruct()
    ctypes.memmove(pointer(rx_data), buf[flexsea_python.CMD_OVERHEAD:], sizeof(rx_data))
    global stress_test_data
    global pc_packet_number, pc_ramp_value, tx_timestamp, start_time

    rx_timestamp = round(time.time() * 1000) - start_time
    stress_test_data.append(StressTestData(tx_timestamp=tx_timestamp, rx_timestamp=rx_timestamp,
                                           tx_packet_num=pc_packet_number, rx_packet_num=rx_data.packet_number,
                                           tx_ramp_value=pc_ramp_value, rx_ramp_value=rx_data.ramp_value))


# Custom command handler used by the stress test code - DUT side
def fx_rx_cmd_handler_3_dut(cmd_6bits, rw, ack, buf):
    # print(f'DUT RX cmd handler()')
    rx_data = FxStressTestStruct()
    ctypes.memmove(pointer(rx_data), buf[flexsea_python.CMD_OVERHEAD:], sizeof(rx_data))
    global dut_packet_number, dut_ramp_value
    dut_packet_number, dut_ramp_value = counter_and_ramp(dut_packet_number, dut_ramp_value)
    # Ready to TX


# We create and serialize the test payload
def gen_stress_test_payload(packet_number, ramp_value, reset=0):
    payload = FxStressTestStruct(packet_number=packet_number, ramp_value=ramp_value, reset=reset)
    payload_string = bytes(payload)
    return payload_string


def plot_results():
    plt.figure(figsize=(16, 10))
    # Clear old data, print latest buffer
    plt.clf()

    # Top plot: display according to time
    # plt.subplot(2, 1, 1)
    plot_x = [x.tx_packet_num for x in stress_test_data]
    plt.plot(plot_x, [x.tx_ramp_value for x in stress_test_data], label="TX Ramp values")
    plt.plot(plot_x, [x.rx_ramp_value for x in stress_test_data], label="RX Ramp values")
    plt.title('Ramp in and out')
    plt.xlabel('Cycle')
    plt.ylabel('Amplitude (ticks)')
    plt.legend()
    plt.draw()

    # # Bottom plot: display according to sample
    # plt.subplot(2, 1, 2)
    # plt.plot(dac, label="DAC")
    # plt.plot(adc, label="ADC")
    # plt.xlabel('Sample')
    # plt.ylabel('Amplitude (ticks)')
    # plt.legend()
    # plt.draw()

    # plt.savefig(f'logs/{file_timestamp}-Test-{test_num:03d}-cont.png')
    plt.show(block=True)  # Blocking until closed


def save_csv_results():
    for cycle in stress_test_data:
        # We add a line to the CSV file:
        csv_line = [[cycle.tx_timestamp, cycle.rx_timestamp, cycle.tx_packet_num, cycle.rx_packet_num,
                     cycle.tx_ramp_value, cycle.rx_ramp_value]]
        csv.write(csv_line)


# Increment and ceil counter and ramp
def counter_and_ramp(cnt, rmp):
    cnt = cnt + 1
    rmp = rmp + 1
    if rmp > RAMP_MAX:
        rmp = 0
    return cnt, rmp


# Loopback demo: we create a bytestream, shuffle it around, then decode it
# No serial port required, no interaction with any other system: pure software loopback
def flexsea_stress_test_local_loopback():

    print('Stress Test Code - Python project with FlexSEA v2.0 DLL')
    print('Local Loopback - No external interaction\n')

    # Initialize FlexSEA comm
    fx_pc = FlexSEAPython(dll_filename)
    fx_dut = FlexSEAPython(dll_filename)

    # Prepare for reception:
    fx_pc.register_cmd_handler(FX_CMD_STRESS_TEST_PERIPH_1, fx_rx_cmd_handler_3_pc)
    fx_dut.register_cmd_handler(FX_CMD_STRESS_TEST_PERIPH_1, fx_rx_cmd_handler_3_dut)

    global pc_packet_number, pc_ramp_value, tx_timestamp, start_time, stress_test_data
    pc_packet_number = -1
    pc_ramp_value = -1
    start_time = round(time.time() * 1000)
    stress_test_data = [] # Start with empty structure

    for i in range(STRESS_TEST_CYCLES):

        # PC generates bytestream:
        pc_packet_number, pc_ramp_value = counter_and_ramp(pc_packet_number, pc_ramp_value)
        ret_val, bytestream, bytestream_len = fx_pc.create_bytestream_from_cmd(cmd=FX_CMD_STRESS_TEST_PERIPH_1,
                                                                               rw="CmdReadWrite", ack="Nack",
                                                                               payload_string=gen_stress_test_payload(
                                                                                 pc_packet_number, pc_ramp_value))

        # Feed bytestream to DUT's circular buffer.
        # This is simulating the Device receiving bytes over a serial interface
        tx_timestamp = round(time.time() * 1000) - start_time
        fx_dut.write_to_circular_buffer(bytestream, bytestream_len)

        # At this point our encoded command is in the DUT's circular buffer. Can we decode it?
        send_reply, cmd_reply, new_data = fx_dut.receive()

        # Do we have a reply to send?
        # (this replicates fx_transmit())
        if send_reply:
            if cmd_reply == FX_CMD_STRESS_TEST_PERIPH_1:
                ret_val, bytestream, bytestream_len = fx_dut.create_bytestream_from_cmd(cmd=FX_CMD_STRESS_TEST_PERIPH_1,
                                                                                        rw="CmdWrite", ack="Nack",
                                                                                        payload_string=
                                                                                        gen_stress_test_payload(
                                                                                          pc_packet_number,
                                                                                          pc_ramp_value))
            # Feed bytestream to PC's circular buffer.
            # This is simulating the PC receiving bytes over a serial interface
            fx_pc.write_to_circular_buffer(bytestream, bytestream_len)

        # Final step, PC reception
        fx_pc.receive()

    print(f'Packets sent: {pc_packet_number + 1}')
    print(f'Packets received: {len(stress_test_data)}')

    plot_results()

    print('Done stressing.')


# Serial demo: we create and send commands to a serial peripheral
# (typically an STM32). Our peripheral will send a reply.
def flexsea_stress_test_serial():

    print('Stress test code - Python project with FlexSEA v2.0 DLL')
    print(f'This test will take approximately {STRESS_TEST_CYCLES * new_tx_delay_ms / 1000:0.2f} s to run.\n')

    # Initialize FlexSEA comm
    fx = FlexSEAPython(dll_filename, com_port_name=com_port)
    fx.who_am_i()
    if fx.get_pf() == 'RPI':
        comm_hw = CommHardware()
        comm_hw.use_channel(0)  # RS-485 transceiver 0, "ch1" on the silk

    # Prepare for reception:
    fx.register_cmd_handler(FX_CMD_STRESS_TEST_PERIPH_1, fx_rx_cmd_handler_3_pc)

    global pc_packet_number, pc_ramp_value, tx_timestamp, start_time, stress_test_data
    pc_packet_number = -1
    pc_ramp_value = -1
    start_time = round(time.time() * 1000)
    bytes_received = 0
    stress_test_data = []  # Start with empty structure

    # Reset embedded counters
    ret_val, bytestream, bytestream_len = fx.create_bytestream_from_cmd(cmd=FX_CMD_STRESS_TEST_PERIPH_1,
                                                                        rw="CmdWrite", ack="Nack",
                                                                        payload_string=gen_stress_test_payload(
                                                                            0, 0, reset=1))
    fx.serial.write(bytestream, bytestream_len)
    time.sleep(0.01)

    for i in range(STRESS_TEST_CYCLES):

        # PC generates bytestream:
        pc_packet_number, pc_ramp_value = counter_and_ramp(pc_packet_number, pc_ramp_value)
        ret_val, bytestream, bytestream_len = fx.create_bytestream_from_cmd(cmd=FX_CMD_STRESS_TEST_PERIPH_1,
                                                                            rw="CmdReadWrite", ack="Nack",
                                                                            payload_string=gen_stress_test_payload(
                                                                              pc_packet_number, pc_ramp_value))

        # Send bytestream to serial port
        fx.serial.write(bytestream, bytestream_len)
        current_time = round(time.time() * 1000)
        tx_timestamp = current_time - start_time
        send_new_tx_cmd_timestamp = current_time + new_tx_delay_ms

        # Send a packet at periodic intervals, listen for a reply
        try:
            while current_time < send_new_tx_cmd_timestamp:

                current_time = round(time.time() * 1000)

                # Feed any received bytes into the circular buffer
                bytes_to_read = fx.serial.bytes_available()
                if bytes_to_read > 0:
                    for i in range(bytes_to_read):
                        new_rx_byte = fx.serial.read_byte()
                        ret_val = fx.write_to_circular_buffer(new_rx_byte[0], 1)
                        bytes_received = bytes_received + 1
                        if ret_val:
                            print("circ_buf_write_byte() problem!")
                            exit()

                # Final step, PC reception
                fx.receive()

        except KeyboardInterrupt:
            print('Interrupted! End of stress test code.')

    # Grab last bytes:
    print(f'Bytes received during TX: {bytes_received}')
    current_time = round(time.time() * 1000)
    final_time = current_time + 1500
    while current_time < final_time:

        current_time = round(time.time() * 1000)

        # Feed any received bytes into the circular buffer
        bytes_to_read = fx.serial.bytes_available()
        if bytes_to_read > 0:
            for i in range(bytes_to_read):
                new_rx_byte = fx.serial.read_byte()
                ret_val = fx.write_to_circular_buffer(new_rx_byte[0], 1)
                bytes_received = bytes_received + 1
                if ret_val:
                    print("circ_buf_write_byte() problem!")
                    exit()

        # Final step, PC reception
        fx.receive()

    print(f'Packets sent: {pc_packet_number + 1}')
    print(f'Packets received: {len(stress_test_data)} (or {stress_test_data[-1].rx_packet_num})')
    end_time = round(time.time() * 1000)
    test_time_s = (end_time - start_time) / 1000
    print(f'Test time: {test_time_s:0.2f} s')
    print(f'Bytes received: {bytes_received} ({bytes_received/len(stress_test_data)} / packet)')

    plot_results()
    save_csv_results()


if __name__ == "__main__":

    file_timestamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    create_directory('logs')
    csv = Csv(f'logs/stress-test-{file_timestamp}.csv', csv_header)

    # Available demos, select one or more:
    flexsea_stress_test_local_loopback()
    print('\n=-=-=-=-=\n')
    flexsea_stress_test_serial()
