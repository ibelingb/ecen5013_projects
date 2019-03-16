#!/usr/bin/env python3

#*****************************************************************************
# @author Joshua Malburg
# joshua.malburg@colorado.edu
# Advanced Embedded Software Development
# ECEN5013-002 - Rick Heidebrecht
# @date March 15, 2019
#*****************************************************************************
#
# @file log_parser.py
# @brief parse binary log file from projet #1; use python3
#
#*****************************************************************************

from enum import IntEnum
import struct

DEBUG = False

FRAME_START_BYTE = b'<'
FRAME_STOP_BYTE = b'<'

FIELD_SZ_MAX_4BYTES = 4
FIELD_SZ_MAX_EVENT_ID = 12
FIELD_SZ_MAX_FILENAME = 2
FIELD_SZ_MAX_LINENUM = 12
FIELD_SZ_MAX_TIME = 12
FIELD_SZ_MAX_PAYLOAD = 12
FIELD_SZ_MAX_PAYLOAD = 0
FIELD_SZ_MAX_CHECKSUM = 12


class parseState_e(IntEnum):
    FIND_START_FRAME_BYTE = 0
    PARSE_EVENT_ID = 1
    PARSE_FILENAME = 2
    PARSE_LINENUM = 3
    PARSE_TIME = 4
    PARSE_PAYLOAD_LEN = 5
    PARSE_PAYLOAD = 6
    PARSE_CHECKSUM = 7
    PARSE_STOP_FRAME_BYTE = 8
    PARSE_DONE = 9
    PARSE_END = 10


class logMsg_e(IntEnum):
    NONE = 0
    SYSTEM_ID = 1
    SYSTEM_VERSION = 2
    LOGGER_INITIALIZED = 3
    GPIO_INITIALIZED = 4
    SYSTEM_INITIALIZED = 5
    SYSTEM_HALTED = 6
    INFO = 7
    WARNING = 8
    ERROR = 9
    PROFILING_STARTED = 10
    PROFILING_RESULT = 11
    COMPLETED = 12
    DATA_RECEIVED = 13
    DATA_ANALYSIS_STARTED = 14
    DATA_ALPHA_COUNT = 15
    DATA_NUM_COUNT = 16
    DATA_PUNC_COUNT = 17
    DATA_MISC_COUNT = 18
    DATA_ANALYSIS_COMPLETED = 19
    HEARTBEAT = 20
    CORE_DUMP = 21
    THREAD_STATUS = 22
    LIGHT_SENSOR_EVENT = 23
    TEMP_SENSOR_EVENT = 24
    REMOTE_HANDLING_EVENT = 25
    MAIN_EVENT = 26
    END = 27


class funcType_e(IntEnum):
    MY_MEMMOVE = 0
    STDLIB_MEMMOVE = 1
    DMA_1BYTE_MEMMOVE = 2
    DMA_4BYTE_MEMMOVE = 3
    MY_MEMSET = 15
    STDLIB_MEMSET = 16
    DMA_1BYTE_MEMSET = 17
    DMA_4BYTE_MEMSET = 18
    END = 19


class ProfileData:
    funcType = funcType_e.END
    txSize = 0
    txCycles = 0


class LogItem:
    eventId = logMsg_e.NONE
    filename = "TBD"
    lineNum = 0
    time = 0
    payloadLength = 0
    payload = "TBD"
    checksum = 0


def clear_log_item(my_log_item):
    my_log_item.eventId = logMsg_e.NONE
    my_log_item.filename = "TBD"
    my_log_item.lineNum = 0
    my_log_item.time = 0
    my_log_item.payloadLength = 0
    my_log_item.payload = "TBD"
    my_log_item.checksum = 0


def read_string(file):
    buffer = ""
    while True:
        char = file.read(1)
        if char == b'' or char == b'\0':
            return buffer
        else:
            buffer += char.decode('ASCII')

def print_state(state):
    print("state is ", end="")
    print(state)


def verify_checksum(item):

    chksum = item.eventId + item.lineNum \
        + item.payloadLength + item.time

    for char in item.filename:
        chksum += ord(char)

    for i in range(0, item.payloadLength):
        chksum += ord(item.payload([i]))

    return chksum == item.checksum


def print_source(item):
    print("[", item.eventId,
          "]: ", end="")


def print_time(item):
    print(item.time, " cycles", end="")


def print_file_location(item):
    print("["
          , item.filename, ": ", item.lineNum, "]", end="")


def print_message(item):
    if item.payloadLength != 0:
        print(" - ", item.payload)
    print("")


def print_log_item(item):

#    if not verify_checksum(item):
#        print("ERROR - checksum invalid")
#    else:

        print_source(item)
        print_time(item)
        print_file_location(item)
        print_message(item)


print("Parsing log file...\n.\n.\n.")

log_file = open("log.bin", "rb")

parseState = parseState_e.FIND_START_FRAME_BYTE
myStr = ""
myLogItem = LogItem()
while parseState != parseState_e.PARSE_DONE:

    if parseState == parseState_e.FIND_START_FRAME_BYTE:

        myStr = log_file.read(1)

        if DEBUG:
            print_state(parseState)
            print(myStr)

        if myStr == b'':
            parseState = parseState_e.PARSE_DONE
        if myStr == FRAME_START_BYTE:
            parseState = parseState_e.PARSE_EVENT_ID

    elif parseState == parseState_e.PARSE_EVENT_ID:
        myStr = read_string(log_file)

        if DEBUG:
            print_state(parseState)
            print(myStr)

        if myStr == '[]':
            parseState = parseState_e.PARSE_DONE

        myLogItem.eventId = logMsg_e(int(myStr, 16))
        parseState = parseState_e.PARSE_FILENAME

    elif parseState == parseState_e.PARSE_FILENAME:

        myStr = read_string(log_file)

        if DEBUG:
            print_state(parseState)
            print(myStr)

        if myStr == '[]':
            parseState = parseState_e.PARSE_DONE

        myLogItem.filename = myStr
        parseState = parseState_e.PARSE_LINENUM

    elif parseState == parseState_e.PARSE_LINENUM:

        myStr = read_string(log_file)

        if DEBUG:
            print_state(parseState)
            print(myStr)

        if myStr == '[]':
            parseState = parseState_e.PARSE_DONE

        myLogItem.lineNum = int(myStr, 16)
        parseState = parseState_e.PARSE_TIME

    elif parseState == parseState_e.PARSE_TIME:

        myStr = read_string(log_file)

        if DEBUG:
            print_state(parseState)
            print(myStr)

        if myStr == '[]':
            parseState = parseState_e.PARSE_DONE

        myLogItem.time = int(myStr, 16)
        parseState = parseState_e.PARSE_PAYLOAD_LEN

    elif parseState == parseState_e.PARSE_PAYLOAD_LEN:

        myStr = read_string(log_file)

        if DEBUG:
            print_state(parseState)
            print(myStr)

        if myStr == '[]':
            parseState = parseState_e.PARSE_DONE

        myLogItem.payloadLength = int(myStr, 16)
        parseState = parseState_e.PARSE_PAYLOAD

    elif parseState == parseState_e.PARSE_PAYLOAD:

        if myLogItem.payloadLength != 0:
            if myLogItem.eventId == logMsg_e.PROFILING_RESULT:
                myProfileData = ProfileData()

                myProfileData.funcType = struct.unpack("=l",  log_file.read(4))
                myProfileData.txSize = struct.unpack("=l",  log_file.read(4))
                myProfileData.txCycles = struct.unpack("=q",  log_file.read(8))

                payLoadStr = " "
                payLoadStr += str(funcType_e(myProfileData.funcType[0]))
                payLoadStr += " transferred "
                payLoadStr += str(myProfileData.txSize[0])
                payLoadStr += " bytes in "
                payLoadStr += str(myProfileData.txCycles[0])
                payLoadStr += " cycles"
                myLogItem.payload = payLoadStr

            else:
                myStr = read_string(log_file)

                if DEBUG:
                    print_state(parseState)
                    print(myStr)

                if myStr == '[]':
                    parseState = parseState_e.PARSE_DONE

                myLogItem.payload = myStr

        parseState = parseState_e.PARSE_CHECKSUM

    elif parseState == parseState_e.PARSE_CHECKSUM:

        myStr = read_string(log_file)

        if DEBUG:
            print_state(parseState)
            print(myStr)

        if myStr == '[]':
            parseState = parseState_e.PARSE_DONE

        myLogItem.checksum = int(myStr, 16)
        parseState = parseState_e.PARSE_STOP_FRAME_BYTE

    elif parseState == parseState_e.PARSE_STOP_FRAME_BYTE:

        if DEBUG:
            print_state(parseState)

        print_log_item(myLogItem)
        parseState = parseState_e.FIND_START_FRAME_BYTE
        clear_log_item(myLogItem)

    else: # parseState == parseState_e.PARSE_STOP_FRAME_DONE

        if DEBUG:
            print_state(parseState)

        print("Parsing complete")

log_file.close()
