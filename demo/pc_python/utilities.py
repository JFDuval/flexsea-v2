import csv
import os


class Csv:
    """
    Save data to a CSV file
    """
    def __init__(self, fn, header):
        self.file = ''
        self.writer = ''
        self.header = header
        # Open file, write header
        self.open(fn)
        self.write(self.header)

    def open(self, fn):
        self.file = open(fn, 'w', newline='')
        self.writer = csv.writer(self.file, quoting=csv.QUOTE_NONNUMERIC, delimiter=',')

    def write(self, new_line):
        self.writer.writerows(new_line)

    def close(self):
        self.file.close()


def create_directory(new_folder):
    """
    If it doesn't exist, create a folder
    """
    if not os.path.isdir(new_folder):
        os.makedirs(new_folder)
