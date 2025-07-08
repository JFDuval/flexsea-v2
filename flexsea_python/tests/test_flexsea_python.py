import sys
import unittest

# Add the FlexSEA path to this project
sys.path.append('../flexsea_python')
from flexsea_python import FlexSEAPython
from flexsea_tools import *
# Note: with PyCharm you must add this folder and mark is as a Sources Folder to avoid an Unresolved Reference issue

# Platform specific shared library and serial port
pf = FlexSEAPython.identify_platform()
if pf == 'WIN':
    dll_filename = '../../projects/eclipse_pc/DynamicLib/libflexsea-v2.dll'
    com_port = 'COM3'
elif pf == 'MAC':
    dll_filename = '../../../flexsea-v2/projects/eclipse_pc/DynamicLib/libflexsea-v2.dylib'
    com_port = '/dev/tty.usbserial-ABCD'  # Default, can be over-ridden by CLI argument
else:
    # ToDo this is likely too generic
    dll_filename = '../../../flexsea-v2/projects/eclipse_pc/DynamicLib/libflexsea-v2.so'
    com_port = '/dev/ttyAMA0'  # Default, can be over-ridden by CLI argument


class TestFlexSEAPython(unittest.TestCase):

    def test_init(self):
        """Can we create a FlexSEA object?"""
        self.fx = FlexSEAPython(dll_filename, com_port_name=com_port)
        self.assertIsNotNone(self.fx)

    def test_write_to_circular_buffer(self):
        """Can we write to our circular buffer?"""
        self.fx = FlexSEAPython(dll_filename, com_port_name=com_port)
        # Write one byte
        self.cb_len = self.fx.get_circular_buffer_length()
        self.assertEqual(self.cb_len, 0)
        bs1 = '1'
        bslen1 = len(bs1)
        self.fx.write_to_circular_buffer(bs1, bslen1)
        self.cb_len = self.fx.get_circular_buffer_length()
        self.assertEqual(self.cb_len, bslen1)
        # Write string
        bs2 = '23456'
        bslen2 = len(bs2)
        self.fx.write_to_circular_buffer(bs2, bslen2)
        self.cb_len = self.fx.get_circular_buffer_length()
        # Do we get the total length?
        self.assertEqual(self.cb_len, bslen1 + bslen2)

    def test_write_to_circular_buffer_overflow(self):
        """What if we write more bytes than it can hold?"""
        self.fx = FlexSEAPython(dll_filename, com_port_name=com_port)
        self.max_len = self.fx.get_max_cb_length()

        # Fill the buffer
        for i in range(self.max_len):
            # Write one byte
            bs1 = 'e'
            bslen1 = len(bs1)
            self.retval = self.fx.write_to_circular_buffer(bs1, bslen1)
            self.assertEqual(self.retval, 0)

        # Confirm that it's full
        self.cb_len = self.fx.get_circular_buffer_length()
        self.assertEqual(self.cb_len, self.max_len)

        # Write one byte, make sure it throws an overflow error
        bs1 = 'e'
        bslen1 = len(bs1)
        self.retval = self.fx.write_to_circular_buffer(bs1, bslen1)
        self.assertEqual(self.retval, 1)

        # Confirm that it's (still) full
        self.cb_len = self.fx.get_circular_buffer_length()
        self.assertEqual(self.cb_len, self.max_len)

    def test_reinit_circular_buffer_midway(self):
        """Can we re-initialize a partially full buffer?"""
        self.fx = FlexSEAPython(dll_filename, com_port_name=com_port)
        self.max_len = self.fx.get_max_cb_length()

        # Fill the buffer halfway
        for i in range(int(self.max_len / 2)):
            # Write one byte
            bs1 = 'e'
            bslen1 = len(bs1)
            self.retval = self.fx.write_to_circular_buffer(bs1, bslen1)
            self.assertEqual(self.retval, 0)

        # Confirm that it's half full
        self.cb_len = self.fx.get_circular_buffer_length()
        self.assertEqual(self.cb_len, int(self.max_len / 2))

        # Re-init
        self.fx.reinit_circular_buffer()

        # Confirm that it's empty
        self.cb_len = self.fx.get_circular_buffer_length()
        self.assertEqual(self.cb_len, 0)

    def test_reinit_circular_buffer_full(self):
        """Can we re-initialize a completely full buffer?"""
        self.fx = FlexSEAPython(dll_filename, com_port_name=com_port)
        self.max_len = self.fx.get_max_cb_length()

        # Fill the buffer
        for i in range(self.max_len):
            # Write one byte
            bs1 = 'e'
            bslen1 = len(bs1)
            self.retval = self.fx.write_to_circular_buffer(bs1, bslen1)
            self.assertEqual(self.retval, 0)

        # Confirm that it's full
        self.cb_len = self.fx.get_circular_buffer_length()
        self.assertEqual(self.cb_len, self.max_len)

        # Re-init
        self.fx.reinit_circular_buffer()

        # Confirm that it's empty
        self.cb_len = self.fx.get_circular_buffer_length()
        self.assertEqual(self.cb_len, 0)

    def test_reinit_circular_buffer_overflow(self):
        """Can we re-initialize a completely full buffer, one that overflowed?"""
        self.fx = FlexSEAPython(dll_filename, com_port_name=com_port)
        self.max_len = self.fx.get_max_cb_length()

        # Overfill the buffer
        for i in range(self.max_len + 10):
            # Write one byte
            bs1 = 'e'
            bslen1 = len(bs1)
            self.retval = self.fx.write_to_circular_buffer(bs1, bslen1)

        # Confirm that it's full
        self.cb_len = self.fx.get_circular_buffer_length()
        self.assertEqual(self.cb_len, self.max_len)

        # Re-init
        self.fx.reinit_circular_buffer()

        # Confirm that it's empty
        self.cb_len = self.fx.get_circular_buffer_length()
        self.assertEqual(self.cb_len, 0)


if __name__ == '__main__':
    unittest.main()
