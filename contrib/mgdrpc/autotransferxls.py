#!/user/bin python
#coding:utf-8
import commands    
import os    
import time
import csv
import sys
import string
import getpass
import logging
from itertools import islice
from authproxy import AuthServiceProxy

import xlrd
def open_excel(file= ''):
    try:
        data = xlrd.open_workbook(file)
        return data
    except Exception as e:
        print(str(e))

def readExcel(file,by_name='sheet'):#edit table name
	print '****** this is',file,' sheetName:',by_name,'******'
	data = open_excel(file)
	table = data.sheet_by_name(by_name) #get the table
	nrows = table.nrows  # get the total rows
	colnames = table.row_values(0)  # get the 0 row data,for example ['name', 'pwd', 'call']
	lists = []
	for rownum in range(1, nrows): #from the 1 line
	    row = table.row_values(rownum)
	    if row:
	        lists.append(row)
	return lists
    
def Cmd(cmd,opt,to,amt):
	if ( cmd == "sendtoaddress" and to!='' and amt!='' ):
		try:
			famt=float(amt)
			if( opt!='' and opt== '-s'):
				logger.debug(to+"\t"+str(famt))
			else:
				logger.debug('[test] '+to+"\t"+str(famt))
			#raw_input('Continue...')
			if( opt!='' and opt== '-s'):
				logger.debug(access.sendtoaddress(to,famt))
			return 0
		except:
			logger.error( "\n---An error occurred---\n")
			return 1
	else:
		return 2
if __name__=='__main__':
	# ===== BEGIN LOG SETTINGS =====
	logger = logging.getLogger("autotransferxls.py")
	formatter = logging.Formatter('%(asctime)s %(levelname)-8s: %(message)s')
	file_handler = logging.FileHandler("debug.log")
	file_handler.setFormatter(formatter)
	console_handler = logging.StreamHandler(sys.stdout)
	logger.addHandler(file_handler)
	logger.addHandler(console_handler)
	logger.setLevel(logging.DEBUG)
	# ===== BEGIN USER SETTINGS =====
	# if you do not set these you will be prompted for a password for every command
	rpcuser = "user"
	rpcpass = "pwd"
	# ====== END USER SETTINGS ======
	test=''
	if( len(sys.argv)==4 ):
		test=sys.argv[3]
	if( test!='' and test =='-t' ):
		if rpcpass == "":
			access = AuthServiceProxy("http://127.0.0.1:19442")
		else:
			access = AuthServiceProxy("http://"+rpcuser+":"+rpcpass+"@127.0.0.1:19442")
	else:
		if rpcpass == "":
			access = AuthServiceProxy("http://127.0.0.1:9442")
		else:
			access = AuthServiceProxy("http://"+rpcuser+":"+rpcpass+"@127.0.0.1:9442")
	# ==============================================================
	cmd = "sendtoaddress"
	csvpath=sys.argv[1]

	opt=''
	if( len(sys.argv)>=3 ):
		opt=sys.argv[2]

	#tableName=csvpath[:csvpath.find('.')] # get before of the  last '.' string
	cmd="sendtoaddress"
	data=[]
	data = readExcel(csvpath)
	for i in data:
		if Cmd(cmd,opt,i[0],i[1])!=0 :
			logger.error("run error")
			exit()
	logger.debug("successed")