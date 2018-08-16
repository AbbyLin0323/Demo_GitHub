# -*- coding: utf-8 -*-
__author__ = 'RogerRao'
# L0_Schedule_c   76 host send cmd : Type = 0x27 PMPort = 0x0 Rsvd = 0x0 C = 0x1 Command = 0xec FeatureLo = 0x0 LBALo = 0x0 Device = 0x0 LBAHi = 0x0 FeatureHi = 0x0 Count = 0x0 ICC = 0x0 Control = 0x0 Auxiliary = 0x0
# index time1 time2 0 read/write lba_start sector_count
import sys
import re
import os
# 对于pattern文件来讲，目前只需要提取 read和write 两种
def decode_trim_cmd(cmd_line, index):
    print "enter decode_trim_cmd stub!"
    # return str(index) + "\t0.0\t0.0\t0\tTrim\tLBA_start\tLBA_end\n"
    return "none"

# 对于win7 这类型的现代操作系统，基本上只会发NCQ cmd，不会发 pio 和 dma 的sata cmd

def decode_ncq_cmd(cmd_line, index):
    # print "enter decode_ncq_cmd stub!\n"
    LBALo = int(cmd_line[18], 16)
    LBAHi = int(cmd_line[22], 16)
    FeatureHi = int(cmd_line[24], 16)
    FeatureLo = int(cmd_line[16], 16)
    SectorCount = (FeatureHi << 8 ) + FeatureLo
    LBA = (LBAHi << 24) + LBALo
    if cmd_line[14] == "0x60":
        OPS = "Read"
    elif cmd_line[14] == "0x61":
            OPS = "Write"
    else:
        OPS = "Invalid"
        return "INVALID NCQ CMD"

    # print "%d\t%f\t%f\t%d\t%s\t%d\t%d\n" % (index, 0.0, 0.0, 0, OPS, LBA, SectorCount)
    OneCmdLine = str(index) + "\t" + str(0.0) + "\t" + str(0.0) + "\t" + str(0) \
                 + "\t" + OPS + "\t" + str(LBA) + "\t" + str(SectorCount) + "\n"
    return OneCmdLine

def decode_pio_cmd(cmd_line, index):
    print "enter decode_pio_cmd stub!"
    # return str(index) + "\t0.000000\t0.0000000\tWrite\tLBAStart\tSectorCount\n"
    return "none"

def decode_dma_cmd(cmd_line, index):
    print "enter decode_dma_cmd stub!"
    # return str(index) + "\t0.000000\t0.0000000\tWrite\tLBAStart\tSectorCount\n"
    return "none"

# 如果以后有新的cmd需要被支持，可以往这个字典里面添加
cmd_decode_dict = {'0x60': decode_ncq_cmd, '0x61':decode_ncq_cmd, '0x6': decode_trim_cmd}

# 负责查看字典里是否有cmd对应的解析函数
def dispatch_decode_function(cmd, cmd_line, index):
    # print "cmd=" + cmd
    if "N/A" != cmd_decode_dict.get(cmd,"N/A"):
        return cmd_decode_dict[cmd](cmd_line, index)
    return "none"

def convert_host_cmd_rec(argv):
    index = 0;
    r = re.compile('[ \t=:]+')
    fileName = "TraceLogReport_MCU_0_2015_06_01_16_28_22.txt"
    print "Openning Trace Log file %s" % fileName
    if not os.path.exists(fileName):
        print "!!! Attention: " + fileName + " Does Not Exist! Please Check File Name."
        return

    process_file = open(fileName)
    output_fileName = "converted_" + fileName
    converted_file = open(output_fileName, 'w')
    if converted_file is None:
        print "ERR: Open Output File Error!!!"
    for line in process_file:
        if line.find("host send cmd")!=-1:
            #print line
            split_line = r.split(line)
            #print split_line
            # Get Command , LBAHi, LBALo, SectorCount
            Command = split_line[14]
            OneCmdLine = dispatch_decode_function(Command, split_line, index)
            if OneCmdLine != "none":
                converted_file.write(OneCmdLine)
                index += 1

    process_file.close()
    converted_file.close()
    print "Pattern File Already Save to %s" % output_fileName
    print "Bye."
    
if __name__ == '__main__':
    print "====== Convert HOST_CMD_REC Trace Log into Pattern File Format ======"
    print "Firstly, You should Get a Trace Log File by Reading from Device via TraceLog tool"
    print "Secondly, Specify the Trace Log fileName in this Script"
    print "======================================================================"
    convert_host_cmd_rec(sys.argv)