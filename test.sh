#!/usr/bin/env bash

TOO_FEW_ARGS_ERR="Too few arguments!"
TOO_MANY_ARGS_ERR="Too many arguments! (Did you forget to quote?)"

RED='\033[0;31m'
YELLOW='\033[0;33m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# Echos a usage string
# $1: Usage string to be printed
function usage() {
	echo -e "Usage:\n\t$1"
}

#USAGE="${0} <input_filename>"
#if [ "$#" -lt 1 ]; then
	#echo "$TOO_FEW_ARGS_ERR"
	#usage "$USAGE"
	#exit 1
#elif [ "$#" -gt 1 ]; then
	#echo "$TOO_MANY_ARGS_ERR"
	#usage "$USAGE"
	#exit 2
#fi

# Returns the current Unix timestamp in milliseconds
function milli() {
	# TODO: use `date` if available, fall back to `gdate` for OSX
	gdate +%s%3N
}

# Returns the size of a file in bytes
# $1: The file to get the size of
function size() {
	(
		du --apparent-size --block-size=1 "$1" 2>/dev/null ||
		gdu --apparent-size --block-size=1 "$1" 2>/dev/null ||
		find "$1" -printf "%s" 2>/dev/null ||
		gfind "$1" -printf "%s" 2>/dev/null ||
		stat --printf="%s" "$1" 2>/dev/null ||
		stat -f%z "$1" 2>/dev/null ||
		wc -c <"$1" 2>/dev/null
	) | awk '{print $1}'
}

# Returns the SHA-1 hash of a file
# $1: The file to get the hash of
function digest() {
	shasum "$1" | cut -d ' ' -f 1
}

# Evaluates one command's compression, echos some statistics, and saves results to "result.csv"
# $1: Input filename
# $2: Output filename
# $3: Command
function evaluate_one_file_one_command() {
	USAGE="${FUNCNAME[0]} <input_filename> <output_filename> <command>"
	if [ "$#" -lt 3 ]; then
		echo "$TOO_FEW_ARGS_ERR"
		usage "$USAGE"
		return 1
	elif [ "$#" -gt 3 ]; then
		echo "$TOO_MANY_ARGS_ERR"
		usage "$USAGE"
		return 2
	fi

	ORIGINAL_SIZE=$(size $1)
	ORIGINAL_HASH=$(digest $1)

	START=$(milli)
	eval $3
	END=$(milli)
	NEW_SIZE=$(size $2)
	NEW_HASH=$(digest $2)

	DURATION=$(echo "scale=3; ($END - $START) / 1000" | bc -l)
	RATIO=$(echo "scale=5; $ORIGINAL_SIZE / $NEW_SIZE" | bc -l)
	MB_PER_SECOND=$(echo "scale=5; $ORIGINAL_SIZE / $DURATION / 1024 / 1024" | bc -l)
	BITS_PER_BYTE=$(echo "scale=5; 8 / $RATIO" | bc -l)
	echo -e "Command was: $GREEN $3 $NC"
	echo -e "Duration: ${RED}${DURATION}${NC}s, Ratio: ${RED}${RATIO}${NC}, MB/s: ${RED}${MB_PER_SECOND}${NC}, bpb: ${RED}${BITS_PER_BYTE}${NC}"
	echo "$3,$DURATION,$RATIO,$MB_PER_SECOND,$BITS_PER_BYTE" >> result.csv
	psql --port='5432' --host='localhost' -c "INSERT INTO compression_result (command, input_file, input_file_hash, input_file_size, output_file, output_file_hash, output_file_size, time_started, time_finished, duration, compression_ratio, mb_per_second, bits_per_byte) VALUES ('$3', '$1', '$ORIGINAL_HASH', $ORIGINAL_SIZE :: BIGINT, '$2', '$NEW_HASH', $NEW_SIZE :: BIGINT, now(), now(), $DURATION :: DOUBLE PRECISION, $RATIO :: DOUBLE PRECISION, $MB_PER_SECOND :: DOUBLE PRECISION, $BITS_PER_BYTE :: DOUBLE PRECISION);"
	rm $2
}

function evaluate_one_file_all_commands() {
	#for i in $(seq 1 5); do
		#evaluate_one_file_one_command "$1" "${1}.paq${i}" "./bin/paq${i} ${1}.paq${i} ${1}"
	#done

	#for i in $(seq 0 9); do
		#evaluate_one_file_one_command "$1" "${1}${i}.paq6" "./bin/paq6 -${i} ${1}${i}.paq6 ${1}"
	#done

	#for i in $(seq 1 5); do
		#evaluate_one_file_one_command "$1" "${1}${i}.paq7" "./bin/paq7 -${i} ${1}${i}.paq7 ${1}"
	#done

	for i in $(seq 0 8); do
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8f" "./bin/paq8f -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8f" "./bin/paq8fthis -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8f" "./bin/paq8fthis2 -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8f" "./bin/paq8fthis3 -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8f" "./bin/paq8fthis4 -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8f" "./bin/paq8fthis_fast -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8g" "./bin/paq8g -${i} ${1}${i}.paq8g ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8h" "./bin/paq8h -${i} ${1}${i}.paq8h ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8hp2" "./bin/paq8hp2 -${i} ${1}${i}.paq8hp2 ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8hp3" "./bin/paq8hp3 -${i} ${1}${i}.paq8hp3 ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8hp4" "./bin/paq8hp4 -${i} ${1}${i}.paq8hp4 ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8hp5" "./bin/paq8hp5 -${i} ${1}${i}.paq8hp5 ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8hp6" "./bin/paq8hp6 -${i} ${1}${i}.paq8hp6 ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8hp7" "./bin/paq8hp7 -${i} ${1}${i}.paq8hp7 ${1}"
		# TODO: Strangely low compression ratio
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8hp8" "./bin/paq8hp8 -${i} ${1}${i}.paq8hp8 ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8hp9" "./bin/paq8hp9 -${i} ${1}${i}.paq8hp9 ${1}"
		# TODO: Segfault
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8hp10" "./bin/paq8hp10 -${i} ${1}${i}.paq8hp10 ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8hp11" "./bin/paq8hp11 -${i} ${1}${i}.paq8hp11 ${1}"
		evaluate_one_file_one_command "$1" "${1}${i}.paq8hp12" "./bin/paq8hp12 -${i} ${1}${i}.paq8hp12 ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8i" "./bin/paq8i -${i} ${1}${i}.paq8i ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8ja" "./bin/paq8ja -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8jb" "./bin/paq8jb -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8jc" "./bin/paq8jc -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8jd" "./bin/paq8jd -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8l" "./bin/paq8l -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8m" "./bin/paq8m -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8n" "./bin/paq8n -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8o" "./bin/paq8o -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8o3" "./bin/paq8o3 -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8o4" "./bin/paq8o4 -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8o5" "./bin/paq8o5 -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8o6" "./bin/paq8o6 -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8o7" "./bin/paq8o7 -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8o8" "./bin/paq8o8 -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8o9" "./bin/paq8o9 -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8o10t" "./bin/paq8o10t -${i} ${1}${i} ${1}"
		#evaluate_one_file_one_command "$1" "${1}${i}.paq8p" "./bin/paq8p -${i} ${1}${i} ${1}"
	done
}

while IFS= read -r -d '' D
do
	evaluate_one_file_all_commands "$D"
done < <(find testfiles -mindepth 1 -maxdepth 1 -type f -print0)
