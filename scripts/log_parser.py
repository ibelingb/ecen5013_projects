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
    PARSE_SOURCEID = 7
    PARSE_CHECKSUM = 8
    PARSE_STOP_FRAME_BYTE = 9
    PARSE_DONE = 10
    PARSE_END = 11


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
    REMOTE_CMD_EVENT = 26
    LOG_EVENT = 27
    MAIN_EVENT = 28
    END = 29


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
    sourceId = 0
    checksum = 0

class LogEvent_e(IntEnum):
    LOG_EVENT_STARTED = 0
    LOG_EVENT_FILE_OPEN = 1
    LOG_EVENT_WRITE_LOGFILE_ERROR = 2
    LOG_EVENT_SHMEM_ERROR = 3
    LOG_EVENT_OPEN_LOGFILE_ERROR  = 4
    LOG_EVENT_EXITING = 5
    LOG_EVENT_END = 6


class MainEvent_e(IntEnum):
    STARTED_THREADS = 0
    LIGHT_THREAD_UNRESPONSIVE = 1
    TEMP_THREAD_UNRESPONSIVE = 2
    REMOTE_THREAD_UNRESPONSIVE = 3
    LOGGING_THREAD_UNRESPONSIVE = 4
    RESTART_THREAD = 5
    LOG_QUEUE_FULL = 6
    ISSUING_EXIT_CMD = 7
    END = 8

class RemoteEvent_e(IntEnum):
    STARTED = 0
    CNCT_ACCEPTED = 1
    CNCT_LOST = 2
    CMD_RECV = 3
    INVALID_RECV = 4
    LOG_QUEUE_ERROR = 5
    STATUS_QUEUE_ERROR = 6
    REMOTE_SHMEM_ERROR = 7
    REMOTE_SERVER_SOCKET_ERROR = 8
    REMOTE_CLIENT_SOCKET_ERROR = 9
    REMOTE_BIST_COMPLETE = 10
    REMOTE_INIT_SUCCESS = 11
    REMOTE_INIT_ERROR = 12
    EXITING = 13
    END = 14

class RemoteCmd_e(IntEnum):
    TEMPCMD_GETTEMP = 1
    TEMPCMD_GETLOWTHRES = 2
    TEMPCMD_GETHIGHTHRES = 3
    TEMPCMD_GETCONFIG = 4
    TEMPCMD_GETRESOLUTION = 5
    TEMPCMD_GETFAULT = 6
    TEMPCMD_GETEXTMODE = 7
    TEMPCMD_GETSHUTDOWNMODE = 8
    TEMPCMD_GETALERT = 9
    TEMPCMD_GETCONVRATE = 10
    TEMPCMD_GETOVERTEMPSTATE = 11
    LIGHTCMD_GETLUXDATA = 12
    LIGHTCMD_GETPOWERCTRL = 13
    LIGHTCMD_GETDEVPARTNO = 14
    LIGHTCMD_GETDEVREVNO = 15
    LIGHTCMD_GETTIMINGGAIN = 16
    LIGHTCMD_GETTIMINGINTEGRATION = 17
    LIGHTCMD_GETINTSELECT = 18
    LIGHTCMD_GETINTPERSIST = 19
    LIGHTCMD_GETLOWTHRES = 20
    LIGHTCMD_GETHIGHTHRES = 21
    LIGHTCMD_GETLIGHTSTATE = 22

class LightEvent_e(IntEnum):
    STARTED = 0
    DAY = 1
    NIGHT = 2
    SENSOR_INIT_ERROR = 3
    SENSOR_READ_ERROR = 4
    STATUS_QUEUE_ERROR = 5
    LOG_QUEUE_ERROR = 6
    SHMEM_ERROR = 7
    I2C_ERROR = 8
    SENSOR_BIST_COMPLETE = 9
    SENSOR_INIT_SUCCESS = 10
    EXITING = 11
    END = 12

class TempEvent_e(IntEnum):
    STARTED = 0
    OVERTEMP = 1
    OVERTEMP_RELEQUISHED = 2
    SENSOR_INIT_ERROR = 3
    SENSOR_READ_ERROR = 4
    STATUS_QUEUE_ERROR = 5
    LOG_QUEUE_ERROR = 6
    SHMEM_ERROR = 7
    I2C_ERROR = 8
    SENSOR_BIST_COMPLETE = 9
    SENSOR_INIT_SUCCESS = 10
    EXITING = 11
    END = 12


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
    print("[", item.eventId.name,
          "]: ", end="")

def print_tid(item):
    print(", tid:", item.sourceId,
          "]", end="")

def print_time(item):
    print("[", item.time, "us]", end="")


def print_file_location(item):
    print("["
          , item.filename.split('/')[-1], ":", item.lineNum, end="")

def print_message(item):
    if item.payloadLength != 0:
        if item.eventId == logMsg_e.LIGHT_SENSOR_EVENT:
            print(LightEvent_e(int(item.payload, 16)).name, end="")
        elif item.eventId == logMsg_e.TEMP_SENSOR_EVENT:
            print(TempEvent_e(int(item.payload, 16)).name, end="")
        elif item.eventId == logMsg_e.REMOTE_HANDLING_EVENT:
            print(RemoteEvent_e(int(item.payload, 16)).name, end="")
        elif item.eventId == logMsg_e.REMOTE_CMD_EVENT:
            print(RemoteCmd_e(int(item.payload, 16)).name, end="")
        elif item.eventId == logMsg_e.LOG_EVENT:
            print(LogEvent_e(int(item.payload, 16)).name, end="")
        elif item.eventId == logMsg_e.MAIN_EVENT:
            print(MainEvent_e(int(item.payload, 16)).name, end="")
        else:
            print(":", item.payload)
    print("")


def print_log_item(item):

#    if not verify_checksum(item):
#        print("ERROR - checksum invalid")
#    else:
        print_file_location(item)
        print_tid(item)
        print_time(item)
        print_source(item)
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

        if myStr == b'':
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

        parseState = parseState_e.PARSE_SOURCEID
    elif parseState == parseState_e.PARSE_SOURCEID:

        myStr = read_string(log_file)

        if DEBUG:
            print_state(parseState)
            print(myStr)

        if myStr == '[]':
            parseState = parseState_e.PARSE_DONE

        myLogItem.sourceId = int(myStr, 16)
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
