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

query = '''
INSERT INTO compression_result (
  command, input_file, input_file_hash, input_file_size,
  output_file, output_file_hash, output_file_size,
  time_started, time_finished, duration, compression_ratio, mb_per_second, bits_per_byte
)
VALUES (
  '{}', '{}', '{}', {} :: BIGINT,
  '{}', '{}', {} :: BIGINT,
  to_timestamp({} / 1000), to_timestamp({} / 1000), {} :: DOUBLE PRECISION, {} :: DOUBLE PRECISION, {} :: DOUBLE PRECISION, {} :: DOUBLE PRECISION
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


the_queue = multiprocessing.Queue()

extensions = {
    'bin/paq8f': '.paq8f',
    'bin/paq8fthis': '.paq8f',
    'bin/paq8fthis2': '.paq8f',
    'bin/paq8fthis3': '.paq8f',
    'bin/paq8fthis4': '.paq8f',
    'bin/paq8fthis_fast': '.paq8f',
    'bin/paq8ja': '.paq8ja',
    'bin/paq8l': '.paq8l',
    'bin/paq8m': '.paq8m',
    'bin/paq8jb': '.paq8jb',
    'bin/paq8jbb': '.paq8jb',
    'bin/paq8jc': '.paq8jc',
    'bin/paq8jd': '.paq8jd',
    'bin/paq8n': '.paq8n',
    'bin/paq8o': '.paq8o',
    'bin/paq8o2': '.paq8o2',
    'bin/paq8o3': '.paq8o3',
    'bin/paq8o4': '.paq8o4',
    'bin/paq8o5': '.paq8o5',
    'bin/paq8o6': '.paq8o6',
    'bin/paq8o7': '.paq8o7',
    'bin/paq8o8': '.paq8o8',
    'bin/paq8o9': '.paq8o9',
    'bin/paq8o10t': '.paq8o10t',
    'bin/paq8p': '.paq8p',
    'bin/paq8p1': '.paq8p1',
    'bin/paq8p2': '.paq8p2',
    'bin/paq8kx_v1': '.paq8kx',
    'bin/paq8kx_v2': '.paq8kx',
    'bin/paq8kx_v3': '.paq8kx',
    'bin/paq8p3_fix2': '.paq8p3',
    'bin/paq8p3_fix3': '.paq8p3',
    'bin/paq8p3x': '.paq8p3x',
    'bin/paq8p3x2': '.paq8p3x2',
    'bin/paq8p3x2_v3': '.paq8p3x2',
    'bin/paq8p3x_v20': '.paq8p3x',
}


def evaluate_one_command_one_file(command, outfile, infile):
    if command.split()[0] in extensions:
        extension = extensions[command.split()[0]]
    elif 'paq8px' in command:
        extension = '.paq8px'
    elif 'paq8q' in command:
        extension = '.paq8q'
    else:
        extension = ''
    original_size = get_size(infile)
    original_hash = get_hash(infile)
    start_time = get_milli()
    p = subprocess.run(command, stdout=subprocess.PIPE, shell=True)
    end_time = get_milli()
    duration = (end_time - start_time) / 1000
    try:
        new_size = get_size(outfile + extension)
        new_hash = get_hash(outfile + extension)
        ratio = original_size / new_size
        bits_per_byte = 8 / ratio
        kb_per_sec = original_size / duration / 1024
        print(colorama.Fore.GREEN + command + colorama.Fore.RESET)
        print('Duration: {:.2f}s, Ratio: {:.2f}, kB/s: {:.2f}, bpb: {:.2f}, ({}/{})'.format(duration, ratio, kb_per_sec, bits_per_byte, new_size, original_size))
        os.remove(outfile + extension)
        cursor.execute(query.format(command, infile, original_hash, original_size, outfile, new_hash, new_size, start_time, end_time, duration, ratio, kb_per_sec, bits_per_byte))
    except Exception as e:
        print(e)
        print(colorama.Fore.RED + command + colorama.Fore.RESET)



testfiles = get_test_files()
executables = get_executables()


def worker_main(queue):
    while True:
        command, outfile, infile = queue.get(True)
        evaluate_one_command_one_file(command, outfile, infile)

the_pool = multiprocessing.Pool(6, worker_main,(the_queue,))


for executable in executables:
    p = subprocess.run(executable, input='\n', stdout=subprocess.PIPE, universal_newlines=True)
    min_level, max_level = get_levels(p.stdout)
    for testfile in testfiles:
        if min_level is None:
            outfile = ''.join(random.choices(string.ascii_uppercase + string.digits, k=10))
            the_queue.put(('{} out/{} {}'.format(executable, outfile, testfile), 'out/{}'.format(outfile), testfile))
        else:
            for level in range(min_level, max_level + 1):
                outfile = ''.join(random.choices(string.ascii_uppercase + string.digits, k=10))
                the_queue.put(('{} -{} out/{} {}'.format(executable, level, outfile, testfile), 'out/{}'.format(outfile), testfile))

while not the_queue.empty():
    time.sleep(1)
