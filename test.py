#!/usr/bin/env python3

import multiprocessing
import os
import re
import psycopg2
import subprocess
import time
import random
import string
import colorama
import glob

query = '''
INSERT INTO compression_result (
  command, input_file, input_file_hash, input_file_size,
  output_file, output_file_hash, output_file_size,
  time_started, time_finished, duration, compression_ratio, kb_per_second, bits_per_byte,
  decompression_command, decompression_duration, decompression_kb_per_second
)
VALUES (
  '{}', '{}', '{}', {} :: BIGINT,
  '{}', '{}', {} :: BIGINT,
  to_timestamp({} / 1000), to_timestamp({} / 1000), {} :: DOUBLE PRECISION, {} :: DOUBLE PRECISION, {} :: DOUBLE PRECISION, {} :: DOUBLE PRECISION,
  '', 0, 0
);
'''

conn = psycopg2.connect("dbname=epsteina user=epsteina host=localhost port=5432")
conn.set_session(readonly=False, autocommit=True)
cursor = conn.cursor()
colorama.init()


def get_test_files():
    parent = 'testfiles'
    testfiles = []
    for folder, _, files in os.walk(parent):
        for filename in files:
            testfiles.append(os.path.join(folder, filename))

    return testfiles


def get_executables():
    parent = 'bin'
    executables = []
    for folder, _, files in os.walk(parent):
        for filename in files:
            executables.append(os.path.join(folder, filename))

    return executables


def get_levels(help_text):
    matches = re.findall('-(\d)', help_text)
    if not matches:
        return None, None
    matches = [int(d) for d in matches]
    min_level, max_level = min(matches), max(matches)
    if min_level == max_level:
        return None, None
    return min_level, max_level


def get_size(filename):
    return os.path.getsize(filename)


def get_hash(filename):
    p = subprocess.run('shasum {}'.format(filename), stdout=subprocess.PIPE, shell=True)
    return p.stdout.split()[0].decode('ascii')


def get_milli():
    return time.time() * 1000


def make_outfilename():
    return ''.join(random.choices(string.ascii_uppercase + string.digits, k=10))


the_queue = multiprocessing.Queue()

def evaluate_one_command_one_file(command, outfile, infile):
    original_size = get_size(infile)
    original_hash = get_hash(infile)
    start_time = get_milli()
    p = subprocess.run(command, stdout=subprocess.PIPE, shell=True)
    end_time = get_milli()
    duration = (end_time - start_time) / 1000
    try:
        outfiles = glob.glob(outfile + '*')
        if len(outfiles) != 1:
            print(colorama.Fore.RED + command + ' # BAD OUTFILE(S)' + colorama.Fore.RESET)
            return False
        outfile = outfiles[0]
        new_size = get_size(outfile)
        new_hash = get_hash(outfile)
        ratio = original_size / new_size
        bits_per_byte = 8 / ratio
        kb_per_sec = original_size / duration / 1024
        print(colorama.Fore.GREEN + command + colorama.Fore.RESET)
        print('Duration: {:.2f}s, Ratio: {:.2f}, kB/s: {:.2f}, bpb: {:.2f}, ({}/{})'.format(duration, ratio, kb_per_sec, bits_per_byte, new_size, original_size))
        os.remove(outfile)
        cursor.execute(query.format(command, infile, original_hash, original_size, outfile, new_hash, new_size, start_time, end_time, duration, ratio, kb_per_sec, bits_per_byte))
        return True
    except Exception as e:
        print(colorama.Fore.RED + command + colorama.Fore.RESET)
        print(e)
        return False


def worker_main(queue):
    while True:
        command, outfile, infile = queue.get(True)
        evaluate_one_command_one_file(command, outfile, infile)


testfiles = get_test_files()
executables = get_executables()
the_pool = multiprocessing.Pool(6, worker_main, (the_queue,))

for executable in executables:
    p = subprocess.run(executable, input='\n', stdout=subprocess.PIPE, universal_newlines=True)
    min_level, max_level = get_levels(p.stdout)
    for testfile in testfiles:
        if 'fpaq3d' in executable:
            min_level, max_level = 0, 7
            for level in range(min_level, max_level + 1):
                outfile = make_outfilename()
                the_queue.put(('{} c {} {} out/{}'.format(executable, level, testfile, outfile), 'out/{}'.format(outfile), testfile))
        elif 'fpaq' in executable:
            outfile = make_outfilename()
            the_queue.put(('{} c {} out/{}'.format(executable, testfile, outfile), 'out/{}'.format(outfile), testfile))
        elif 'lpaq' in executable:
            min_level, max_level = 0, 9
            for level in range(min_level, max_level + 1):
                if 'lpaq1b' in executable or 'lpaq1c' in executable:
                    outfile = make_outfilename()
                    the_queue.put(('{} {} 12346wma {} out/{}'.format(executable, level, testfile, outfile), 'out/{}'.format(outfile), testfile))
                else:
                    outfile = make_outfilename()
                    the_queue.put(('{} {} {} out/{}'.format(executable, level, testfile, outfile), 'out/{}'.format(outfile), testfile))
        elif 'paq8pxd' in executable:
            continue
        else:
            if min_level is None:
                outfile = make_outfilename()
                the_queue.put(('{} out/{} {}'.format(executable, outfile, testfile), 'out/{}'.format(outfile), testfile))
            else:
                for level in range(min_level, max_level + 1):
                    outfile = make_outfilename()
                    the_queue.put(('{} -{} out/{} {}'.format(executable, level, outfile, testfile), 'out/{}'.format(outfile), testfile))

while not the_queue.empty():
    time.sleep(1)
