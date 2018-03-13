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
from authproxy import AuthServiceProxy
from itertools import islice
def readCSV(ad,csvpath):
    with open(csvpath,'rb') as f:
        data=csv.reader(f)
        for x in islice(data,1,None): #row index from 2
            ad.append(x)
    #f.close()
def Cmd(cmd,to,amt):
	if cmd == "sendtoaddress":
		try:
			famt=float(amt)
			logger.debug(to+"\t"+str(famt))
			input('Continue...')
			logger.debug(access.sendtoaddress(to,famt))
			return 0
		except:
			logger.error( "\n---An error occurred---\n")
			return 1
	else:
		return 2
if __name__=='__main__':
	# ===== BEGIN LOG SETTINGS =====
	logger = logging.getLogger("autotransfer.py")
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


	if rpcpass == "":
		access = AuthServiceProxy("http://127.0.0.1:9442")
	else:
		access = AuthServiceProxy("http://"+rpcuser+":"+rpcpass+"@127.0.0.1:9442")
	cmd = "sendtoaddress"
	csvpath=sys.argv[1]

	data=[]
	readCSV(data,csvpath)
	for i in data:
		if Cmd(cmd,i[0],i[1])!=0 :
			logger.error("run error")
			exit()
	logger.debug("successed")

	
		

