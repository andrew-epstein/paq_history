DROP TABLE IF EXISTS compression_program CASCADE;
DROP TABLE IF EXISTS compression_result;
DROP INDEX IF EXISTS compression_result_input_file_hash_compression_ratio_idx;
DROP TABLE IF EXISTS compression_params;
DROP TABLE IF EXISTS decompression_params;
DROP TYPE IF EXISTS ARG_TYPE;

CREATE TABLE IF NOT EXISTS compression_program
(
    id           BIGSERIAL PRIMARY KEY,
    program_name TEXT NOT NULL,
    program_hash TEXT NOT NULL,
    UNIQUE (program_hash)
);

CREATE TABLE IF NOT EXISTS compression_result
(
    id                          BIGSERIAL PRIMARY KEY,
    command                     TEXT                        NOT NULL,
    compression_program_id      BIGINT REFERENCES compression_program (id),
    input_file                  TEXT                        NOT NULL,
    input_file_hash             TEXT                        NOT NULL,
    input_file_size             BIGINT                      NOT NULL,
    output_file                 TEXT                        NOT NULL,
    output_file_hash            TEXT                        NOT NULL,
    output_file_size            BIGINT                      NOT NULL,
    time_started                TIMESTAMP WITHOUT TIME ZONE NOT NULL,
    time_finished               TIMESTAMP WITHOUT TIME ZONE NOT NULL,
    duration                    DOUBLE PRECISION            NOT NULL,
    compression_ratio           DOUBLE PRECISION            NOT NULL,
    kb_per_second               DOUBLE PRECISION            NOT NULL,
    bits_per_byte               DOUBLE PRECISION            NOT NULL,
    decompression_command       TEXT                        NOT NULL,
    decompression_duration      DOUBLE PRECISION            NOT NULL,
    decompression_kb_per_second DOUBLE PRECISION            NOT NULL,
    level                       BIGINT                      NOT NULL,
    CONSTRAINT valid_duration_ms CHECK (duration >= 0),
    CONSTRAINT valid_kb_per_second CHECK (kb_per_second >= 0),
    CONSTRAINT valid_compression_ratio CHECK (compression_ratio >= 0),
    CONSTRAINT valid_decompression_duration CHECK (decompression_duration >= 0),
    CONSTRAINT valid_decompression_kb_per_second CHECK (decompression_kb_per_second >= 0)
);

CREATE INDEX CONCURRENTLY IF NOT EXISTS compression_result_input_file_hash_compression_ratio_idx
    ON compression_result (input_file_hash, compression_ratio);

CREATE TYPE ARG_TYPE AS ENUM (
    'LEVEL', 'LEVEL_HYPHEN', 'INFILE', 'OUTFILE', 'OTHER'
    );

CREATE TABLE IF NOT EXISTS compression_params
(
    id                      BIGSERIAL PRIMARY KEY,
    program_name_pattern    TEXT     NOT NULL,
    arg1_type               ARG_TYPE NOT NULL,
    arg2_type               ARG_TYPE,
    arg3_type               ARG_TYPE,
    arg4_type               ARG_TYPE,
    other_1                 TEXT,
    other_2                 TEXT,
    other_3                 TEXT,
    other_4                 TEXT,
    min_level               INT,
    max_level               INT,
    can_control_output_file BOOLEAN  NOT NULL DEFAULT TRUE,
    UNIQUE (program_name_pattern)
);

CREATE TABLE IF NOT EXISTS decompression_params
(
    id                      BIGSERIAL PRIMARY KEY,
    program_name_pattern    TEXT     NOT NULL,
    arg1_type               ARG_TYPE NOT NULL,
    arg2_type               ARG_TYPE,
    arg3_type               ARG_TYPE,
    arg4_type               ARG_TYPE,
    other_1                 TEXT,
    other_2                 TEXT,
    other_3                 TEXT,
    other_4                 TEXT,
    can_control_output_file BOOLEAN  NOT NULL DEFAULT TRUE,
    UNIQUE (program_name_pattern)
);


INSERT INTO compression_params (program_name_pattern, arg1_type, arg2_type, arg3_type, arg4_type, min_level, max_level,
                                other_1, other_2, can_control_output_file)
VALUES ('^paq[1-5]', 'OUTFILE', 'INFILE', NULL, NULL, NULL, NULL, NULL, NULL, TRUE),
       ('^paq6', 'LEVEL_HYPHEN', 'OUTFILE', 'INFILE', NULL, 0, 9, NULL, NULL, TRUE),
       ('^paq7', 'LEVEL_HYPHEN', 'OUTFILE', 'INFILE', NULL, 1, 5, NULL, NULL, TRUE),
       ('^lpaq[1-9][mva]{0,1}2{0,1}$', 'LEVEL', 'INFILE', 'OUTFILE', NULL, 0, 9, NULL, NULL, TRUE),
       ('^lpaq1b$|^lpaq1c$', 'LEVEL', 'OTHER', 'INFILE', 'OUTFILE', 0, 9, NULL, '12346wma', TRUE),
       ('^fp8_v[1-6]$', 'LEVEL_HYPHEN', 'OUTFILE', 'INFILE', NULL, 0, 8, NULL, NULL, TRUE),
       ('^PAQAR', 'LEVEL_HYPHEN', 'OUTFILE', 'INFILE', NULL, 0, 9, NULL, NULL, TRUE),
       ('^fpaq3d$', 'OTHER', 'LEVEL', 'INFILE', 'OUTFILE', 0, 7, 'c', NULL, TRUE),
       ('^fpaq(0[2bfrsx]{0,1}[1-6]{0,1}[ab]{0,1}|[1-3a-c][bc]{0,1})$', 'OTHER', 'INFILE', 'OUTFILE', NULL, NULL, NULL,
        'c', NULL, TRUE),
       ('^paq8', 'LEVEL_HYPHEN', 'OUTFILE', 'INFILE', NULL, 0, 8, NULL, NULL, TRUE);

INSERT INTO decompression_params (program_name_pattern, arg1_type, arg2_type, arg3_type, arg4_type, other_1, other_2,
                                  can_control_output_file)
VALUES ('^paq[1-5]', 'OUTFILE', NULL, NULL, NULL, NULL, NULL, TRUE),
       ('^paq[6-7]', 'OUTFILE', NULL, NULL, NULL, NULL, NULL, TRUE),
       ('^lpaq[1-9][mva]{0,1}2{0,1}$', 'OTHER', 'INFILE', 'OUTFILE', NULL, 'd', NULL, TRUE),
       ('^lpaq1b$|^lpaq1c$', 'OTHER', 'OTHER', 'INFILE', 'OUTFILE', 'd', '12346wma', TRUE),
       ('^fp8_v[1-6]$', 'OUTFILE', NULL, NULL, NULL, NULL, NULL, TRUE),
       ('^PAQAR', 'OUTFILE', NULL, NULL, NULL, NULL, NULL, TRUE),
       ('^fpaq3d$', 'OTHER', 'INFILE', 'OUTFILE', NULL, 'd', NULL, TRUE),
       ('^fpaq(0[2bfrsx]{0,1}[1-6]{0,1}[ab]{0,1}|[1-3a-c][bc]{0,1})$', 'OTHER', 'INFILE', 'OUTFILE', NULL, 'd', NULL,
        TRUE),
       ('^paq8', 'OUTFILE', NULL, NULL, NULL, NULL, NULL, FALSE);

