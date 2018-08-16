#*******************************************************************************
# Copyright (C), 2015 VIA Technologies, Inc. All Rights Reserved.              *
#                                                                              *
# This PROPRIETARY SOFTWARE is the property of VIA Technologies, Inc.          *
# and may contain trade secrets and/or other confidential information of VIA   *
# Technologies,Inc.                                                            *
# This file shall not be disclosed to any third party, in whole or in part,    *
# without prior written consent of VIA.                                        *
#                                                                              *
# THIS PROPRIETARY SOFTWARE AND ANY RELATED DOCUMENTATION ARE PROVIDED AS IS,  *
# WITH ALL FAULTS, AND WITHOUT WARRANTY OF ANY KIND EITHER EXPRESS OR IMPLIED, *
# AND VIA TECHNOLOGIES, INC. DISCLAIMS ALL EXPRESS OR IMPLIED WARRANTIES OF    *
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR        *
# NON-INFRINGEMENT.                                                            *
#*******************************************************************************
#Filename    : ptl.py
#Version     : Ver 0.1
#Author      : Haven
#Date        : 2015.04.20
#Description : this python script is help for analysing performance trace log
#Usage       : ptl.py MCU_x "filter string"
#              eg ptl.py MCU_0 "Ahci task start"
#Modify      :
#20150420    Haven create file
#********************************************************************************

import sys
import glob
import os

def get_input_filename(mcux):
	filelist = glob.glob("*.txt")
	filter = 'TraceLogReport_'+mcux
	
	for filename in filelist:
		if filter in filename:
			input_file_name = filename
			break

	if 'input_file_name' in dir():
		print "input file is " + input_file_name
		return input_file_name
	else:
		print "can not find *"+mcux+"*.txt, please check your input param and exist log files"
		exit()

def select_target_log(input_file_name, select_str):
	output_file_name = select_str.replace(' ','_') + '.txt'
	input_file = open(input_file_name, 'r')
	output_file = open(output_file_name, 'w')
	list = input_file.readlines()

	abs_count = 1
	for line in list:
		if select_str in line:
			output_file.write(str(abs_count) + ' abs_count ' + line)
			abs_count = 1
		else:
			abs_count += 1

	input_file.close()
	output_file.close()

def sort_cycle_count(select_str):
	input_file_name = select_str.replace(' ','_') + '.txt'
	output_file_name = select_str.replace(' ','_') + '_sorted' + '.txt'
	input_file = open(input_file_name, 'r')
	output_file = open(output_file_name, 'w')

	list = input_file.readlines()
	list = [x.split('ulDiffCycles = ') for x in list]
	list = [x[1] for x in list]
	list = [x.strip() for x in list]
	list = [int(x,16) for x in list]
	list.sort(reverse=True)
	list = [str(x) + '\n' for x in list]

	output_file.writelines(list)

	input_file.close()
	output_file.close()
	#print "result saved to " + output_file_name

def sort_abs_cycle_count(select_str):
	input_file_name = select_str.replace(' ','_') + '.txt'
	output_file_name = select_str.replace(' ','_') + '_abs_sorted' + '.txt'
	input_file = open(input_file_name, 'r')
	output_file = open(output_file_name, 'w')

	list = input_file.readlines()
	list = [x.split('ulDiffCycles = ') for x in list]
	list = [x[0] for x in list]
	list = [x.split('ulCurrentTime = ') for x in list]
	list = [[x[0].split('abs_count')[0],x[1]] for x in list]
	list = [[int(x[0],10),int(x[1],16)] for x in list]
	
	list = calc_abs_cycle_count(list)
	
	list.sort(reverse=True)
	list = [str(x) + '\n' for x in list]

	output_file.writelines(list)

	input_file.close()
	output_file.close()

def calc_abs_cycle_count(list):
	abs_list = []
	last_time = list[0][1]
	for curr_list in list:
		curr_time = curr_list[1]
		abs_count = curr_list[0]
		abs_list.append(curr_time-last_time-abs_count*170)
		last_time = curr_time
	return abs_list
	
def calc_static_cycle_count(select_str):
	range_tuple=('0-49', '50-99', '100-149', '150-199', '200-249', '250-299', '300-399', '400-499', '500-599', '600-699', '700-799', '800-899', '900-999', 
	'1000-1499', '1500-1999', '2000-2999', '3000-3999', '4000-4999', '5000-5999', '6000-6999', '7000-7999', '8000-8999', '9000-9999', 
	'10000-14999', '15000-19999', '20000-25000', '25001-30000', '30001')

	input_filename = select_str.replace(' ','_') + '_sorted' + '.txt'
	
	fn = open(input_filename,'r')
	input_list = fn.readlines()
	input_list = [int(x.strip()) for x in input_list]
	fn.close()
	
	range_list = []
	range_inter= []
	range_num=len(range_tuple)
	
	for n in range(range_num):
		range_list.append([range_tuple[n],0])
	
	range_inter = [x.split('-') for x in range_tuple]
	range_inter.pop()
	range_inter = [[int(x[0]),int(x[1])] for x in range_inter]
		
	for cycle_count in input_list:
		if cycle_count >= int(range_tuple[-1]):
			range_list[-1][1] += 1
		else:
			for n in range(range_num-1):
				low = range_inter[n][0]
				high = range_inter[n][1]
				if cycle_count >= low and cycle_count <= high:
					range_list[n][1] += 1
					break

	range_list = [str(x[0]) +'\t'+ str(x[1]) + '\n' for x in range_list]
	
	output_file_name = sys.argv[1]+'_'+select_str.replace('_abs','').replace(' ','_') + '_statistical' + '.txt'
	
	if '_abs' in select_str:
		fn = open(output_file_name,'a')
	else:
		fn = open(output_file_name,'w')
		
	fn.writelines('\n'+select_str+'\n\n')
	fn.writelines(range_list)
	fn.writelines('total records :' + str(len(input_list)) + '\n\n')
	fn.close()

def clear_files(select_str):
	file_list = []
	file_list.append(select_str.replace(' ','_') + '.txt')
	file_list.append(select_str.replace(' ','_') + '_sorted' + '.txt')
	file_list.append(select_str.replace(' ','_') + '_abs_sorted' + '.txt')
	
	for fn in file_list:
		if os.path.exists(fn):
			os.remove(fn)

def main():
	mcux = sys.argv[1]
	select_str = sys.argv[2]
	
	#preparing input_filename
	input_filename = get_input_filename(mcux)
	
	#first step, select trace log strings for target trace location by the input filter string
	select_target_log(input_filename, select_str)

	#second step, filter cycle count and sorted them.
	sort_cycle_count(select_str)
	sort_abs_cycle_count(select_str)
	
	#3rd step, calc_static_cycle_count
	calc_static_cycle_count(select_str)
	calc_static_cycle_count(select_str+'_abs')
	
	#4th step, clear temp files
	clear_files(select_str)

main()	