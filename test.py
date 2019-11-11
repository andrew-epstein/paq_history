#!/usr/bin/env python3

from collections import namedtuple
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

CompressionParams = namedtuple('CompressionParams',
                               'arg1_type, arg2_type, arg3_type, arg4_type, min_level, max_level, other_1, other_2')

query = '''
INSERT INTO compression_result (
  command, compression_program_id, input_file, input_file_hash,
  input_file_size, output_file, output_file_hash, output_file_size,
  time_started, time_finished, duration, compression_ratio, kb_per_second, bits_per_byte,
  decompression_command, decompression_duration, decompression_kb_per_second
)
VALUES (
  '{}', {}, '{}', '{}', {} :: BIGINT,
  '{}', '{}', {} :: BIGINT,
  to_timestamp({} / 1000), to_timestamp({} / 1000), {} :: DOUBLE PRECISION, {} :: DOUBLE PRECISION, {} :: DOUBLE PRECISION, {} :: DOUBLE PRECISION,
  '', 0, 0
);
'''

conn = psycopg2.connect(
    "dbname=epsteina user=epsteina host=localhost port=5432")
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
    matches = re.findall(r'-(\d)', help_text)
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
    p = subprocess.run('shasum {}'.format(filename),
                       stdout=subprocess.PIPE, shell=True)
    return p.stdout.split()[0].decode('ascii')


def get_milli():
    return time.time() * 1000


def make_outfilename():
    return 'out/' + ''.join(random.choices(string.ascii_uppercase + string.digits, k=10))


def evaluate_one_command_one_file(command, outfile, infile, program_id):
    original_size = get_size(infile)
    original_hash = get_hash(infile)
    start_time = get_milli()
    try:
        p = subprocess.run(command, stdout=subprocess.PIPE, shell=True, check=True)
    except subprocess.CalledProcessError as e:
        print(colorama.Fore.RED + '{} # exited with nonzero error code'.format(command) + colorama.Fore.RESET)
        return False
    end_time = get_milli()
    duration = (end_time - start_time) / 1000
    try:
        outfiles = glob.glob(outfile + '*')
        if len(outfiles) != 1:
            print(colorama.Fore.RED + command +
                  ' # BAD OUTFILE(S)' + colorama.Fore.RESET)
            return False
        outfile = outfiles[0]
        new_size = get_size(outfile)
        new_hash = get_hash(outfile)
        ratio = original_size / new_size
        bits_per_byte = 8 / ratio
        kb_per_sec = original_size / duration / 1024
        print(colorama.Fore.GREEN + command + colorama.Fore.RESET)
        print('Duration: {:.2f}s, Ratio: {:.2f}, kB/s: {:.2f}, bpb: {:.2f}, ({}/{})'.format(
            duration, ratio, kb_per_sec, bits_per_byte, new_size, original_size))
        os.remove(outfile)
        cursor.execute(
            query.format(
                command,
                program_id,
                infile,
                original_hash,
                original_size,
                outfile,
                new_hash,
                new_size,
                start_time,
                end_time,
                duration,
                ratio,
                kb_per_sec,
                bits_per_byte))
        return True
    except Exception as e:
        print(colorama.Fore.RED + command + colorama.Fore.RESET)
        print(e)
        return False


def insert_program(executable, program_hash):
    """ Create a row in `compression_program` for this executable
    if one doesn't yet exist.
    Returns the id of the row.
    """
    select_query = '''
    SELECT id
      FROM compression_program
      WHERE program_hash = '{}';
    '''
    insert_query = '''
    INSERT INTO compression_program
      (program_name, program_hash
    )
      VALUES ('{}', '{}')
    RETURNING id;
    '''
    conn = psycopg2.connect(
        "dbname=epsteina user=epsteina host=localhost port=5432")
    conn.set_session(readonly=False, autocommit=True)
    cursor = conn.cursor()

    cursor.execute(select_query.format(program_hash))
    if cursor.rowcount == 0:
        cursor.execute(insert_query.format(executable, program_hash))
        cursor.execute(select_query.format(program_hash))
        program_id = cursor.fetchone()[0]
        conn.close()
        return program_id
    else:
        program_id = cursor.fetchone()[0]
        conn.close()
        return program_id


def get_params(executable):
    query = '''
    SELECT t.arg1_type, t.arg2_type, t.arg3_type, t.arg4_type, min_level, max_level, other_1, other_2
    FROM (SELECT * FROM compression_params) AS t
    WHERE regexp_match('{}', program_name_pattern) IS NOT NULL;
    '''

    cursor.execute(query.format(executable))
    try:
        result = cursor.fetchone()
        return CompressionParams(*result)
    except BaseException:
        return None


def substitute_arg(arg_type, testfile, outfile, level, other=None):
    if arg_type == 'LEVEL_HYPHEN':
        return '-{}'.format(level)
    elif arg_type == 'LEVEL':
        return str(level)
    elif arg_type == 'INFILE':
        return testfile
    elif arg_type == 'OUTFILE':
        return outfile
    elif arg_type == 'OTHER':
        return other
    else:
        return ''


def construct_command(executable, params, testfile, outfile, level):
    arg1_type = params.arg1_type
    arg2_type = params.arg2_type
    arg3_type = params.arg3_type
    arg4_type = params.arg4_type
    other1 = params.other_1
    other2 = params.other_2
    command = '{executable} {arg1} {arg2} {arg3} {arg4}'
    result = command.format(
        executable=executable,
        arg1=substitute_arg(arg1_type, testfile, outfile, level, other1),
        arg2=substitute_arg(arg2_type, testfile, outfile, level, other2),
        arg3=substitute_arg(arg3_type, testfile, outfile, level),
        arg4=substitute_arg(arg4_type, testfile, outfile, level)
    )
    return result


def worker_main(queue):
    while True:
        command, outfile, infile, program_id = queue.get(True)
        evaluate_one_command_one_file(command, outfile, infile, program_id)


testfiles = get_test_files()
executables = get_executables()
the_queue = multiprocessing.Queue()
the_pool = multiprocessing.Pool(12, worker_main, (the_queue,))


for executable in executables:
    program_hash = get_hash(executable)
    program_id = insert_program(executable, program_hash)
    compression_params = get_params(executable[4:])
    if not compression_params:
        print(colorama.Fore.RED + 'No compression params for executable: {}'.format(executable) + colorama.Fore.RESET)
        continue

    for testfile in testfiles:
        min_level = compression_params.min_level if compression_params.min_level else 0
        max_level = compression_params.max_level if compression_params.max_level else 0
        for level in range(min_level, max_level + 1):
            outfile = make_outfilename()
            command = construct_command(executable, compression_params, testfile, outfile, level)
            the_queue.put((command, outfile, testfile, program_id))


'''
for executable in executables:
    program_hash = get_hash(executable)
    program_id = insert_program(executable, program_hash)

    p = subprocess.run(executable, input='\n',
                       stdout=subprocess.PIPE, universal_newlines=True)
    min_level, max_level = get_levels(p.stdout)
    for testfile in testfiles:
        if 'fpaq3d' in executable:
            min_level, max_level = 0, 7
            for level in range(min_level, max_level + 1):
                outfile = make_outfilename()
                the_queue.put(('{} c {} {} out/{}'.format(executable, level,
                                                          testfile, outfile), 'out/{}'.format(outfile), testfile, program_id))
        elif 'fpaq' in executable:
            outfile = make_outfilename()
            the_queue.put(('{} c {} out/{}'.format(executable, testfile,
                                                   outfile), 'out/{}'.format(outfile), testfile,program_id))
        elif 'paq8pxd' in executable:
            continue
        else:
            if min_level is None:
                outfile = make_outfilename()
                the_queue.put(('{} out/{} {}'.format(executable, outfile,
                                                     testfile), 'out/{}'.format(outfile), testfile, program_id))
            else:
                for level in range(min_level, max_level + 1):
                    outfile = make_outfilename()
                    print('Adding to queue: {}, {}, {}'.format(executable, level, testfile))
                    the_queue.put(('{} -{} {} out/{}'.format(executable, level,
                                                             testfile, outfile), 'out/{}'.format(outfile), testfile, program_id))
'''

while not the_queue.empty():
    time.sleep(100)
