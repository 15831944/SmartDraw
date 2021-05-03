#! -*- Encoding: Latin-1 -*-

import sys
import os
import locale
import math
import re
import win32com.client
import traceback
import pythoncom

#from pythoncom import CoInitialize,CoUninitialize
from math import *

locale.setlocale(locale.LC_ALL, "")

#try:
#	reloadall()
#except:
#	traceback.print_exc(file=open('c:\\test.at','a'))

# brief		float�� ����ȯ�� �Ѵ�. �� ��ȯ�ÿ� ������ �߻��ϸ� 0�� �����Ѵ�.
# author	BHK
def SAFE_FLOAT(arg):
	try:
		res = float(arg)
		return res
	except ValueError:
		return 0

	return 0

# brief		AutoCAD�� �ε��� �� ���ڷ� �Ѿ�� ������ �����Ѵ�.
# author	BHK
# param		argv[0] = dwg file path
# date		2009-08-13 09:21:42 
def PyOpenDwgFile(argv):
	try:
		if (type(argv) == type([])) and (3 == len(argv)):
			acad = win32com.client.Dispatch('AutoCAD.Application')
			acad.Visible = True

			# Arx�� �ε��մϴ�.
			#for i in range(len(argv) - 1):
			#	acad.LoadArx(argv[i + 1])
			if 'Border Setting' == argv[2]:
				docs = acad.Documents
				bFound = False
				for i in range(docs.Count):
					doc = docs.Item(i)
#					pJgenApp.AppendLogToOutputWnd(str(doc.Path + '\\' + doc.Name))
					if argv[0] == (doc.Path + '\\' + doc.Name):
						bFound = True
						break
				#���ϰ�ο� �ѱ��� �����Ҽ� �����Ƿ� euc_kr�� ���ڵ��Ѵ�.
				if False == bFound:
#					pJgenApp.AppendLogToOutputWnd('doc not found')
					docs.Open(unicode(argv[0] , 'euc_kr'), True)
#					pJgenApp.AppendLogToOutputWnd('doc not found and new doc open')
					doc = docs.Item(0)
					doc.SendCommand('-units 2 4 1 2 0 N ')
					doc.SendCommand('-vports si ')
					acad.ZoomExtents()	# zoom extents
			elif len(argv[0]) != 0:
				docs = acad.Documents
				# ���� �ִ� ��� DWG�� �ݴ´�.
				# date : 2009-09-02 10:07:13 
				for i in range(docs.Count):
					# close document by descent.
					at = docs.Count - 1
					doc = docs.Item(at)
					if True == doc.ReadOnly:
						doc.Close(False)
					else:
						doc.Close(False)
					
					#if (argv[0] == doc.FullName):
					#	doc.Activate	# activate ��Ų��.
					#	return 'SUCCESS'
				#���ϰ�ο� �ѱ��� �����Ҽ� �����Ƿ� euc_kr�� ���ڵ��Ѵ�.
				if 'True' == argv[1]:
					docs.Open(unicode(argv[0] , 'euc_kr'), True)
				else:
					docs.Open(unicode(argv[0] , 'euc_kr'))
				
				doc = docs.Item(0)
				#doc.SendCommand('-units 2 4 1 2 0 N ')
				#doc.SendCommand('-vports si ')
				#acad.ZoomExtents()	# zoom extents

			acad.WindowState = 3	# maximize acad window.

			return 'SUCCESS'
	except:
		print str(traceback.format_exc())
#		return pJgenApp.AppendLogToOutputWnd(str(traceback.format_exc()))

	return 'FAIL'

#brief		arx�� �ε��մϴ�.
#author		BHK 
#param		
#date		2009-08-14 19:21:37 
def PyLoadArx(argv):
	try:
		if (type(argv) == type([])) and (len(argv) >= 1):
			acad = win32com.client.Dispatch('AutoCAD.Application')

			# Arx�� �ε��մϴ�.
			for i in range(len(argv)):
				acad.LoadArx(argv[i])

			return 'SUCCESS'
	except:
		print str(traceback.format_exc())
#		return pJgenApp.AppendLogToOutputWnd(str(traceback.format_exc()))

	return 'FAIL'

#brief		AutoCAD�� command�� ������.
#author		BHK 
#param		argv[0] = command string
#date		2009-08-14 18:55:49 
def PySendCommand(argv):
	try:
		if(type(argv) == type([])) and (1 == len(argv)):
			pythoncom.CoInitialize()
			acad = win32com.client.dynamic.Dispatch('AutoCAD.Application')
			docs = acad.Documents
			if(docs.Count == 0):
				acad.Quit()
				acad = win32com.client.dynamic.Dispatch('AutoCAD.Application')
	
			doc = acad.ActiveDocument
			doc.SendCommand(argv[0] + '\n')
			pythoncom.CoUninitialize()

			return 'SUCCESS'
	except:
		print str(traceback.format_exc())
#		return pJgenApp.AppendLogToOutputWnd(str(traceback.format_exc()))

	return 'FAIL'

# brief		min,max point�� pick�Ͽ� �� ���� ���Ѵ�.
# author	BHK
# param		
# date		2009-08-13 15:04:43
def PyGetMinMaxPoints(argv):
	try:
		acad = win32com.client.Dispatch('AutoCAD.Application')
		shell = win32com.client.Dispatch("WScript.Shell")
		shell.AppActivate(acad.Caption)
		doc = acad.ActiveDocument
		util = doc.Utility
		
		# Nearest & Endpoint
		doc.SetVariable('osmode', 513)

		pt1 = util.GetPoint()
		pt2 = util.GetPoint()

		res = []
		res.append(pt1[0])
		res.append(pt1[1])
		res.append(pt2[0])
		res.append(pt2[1])
		return res
	except:
		print str(traceback.format_exc())
#		return pJgenApp.AppendLogToOutputWnd(str(traceback.format_exc()))

#brief		�Ÿ��� ���Ѵ�.
#author		BHK 
#param		
#date		2009-08-14 10:02:52 
def PyGetDistance(argv):
	try:
		acad = win32com.client.Dispatch('AutoCAD.Application')
		shell = win32com.client.Dispatch("WScript.Shell")
		shell.AppActivate(acad.Caption)
		doc = acad.ActiveDocument
		util = doc.Utility
		distance = util.GetDistance()

		res = []
		res.append(distance)
		return res
	except:
		print str(traceback.format_exc())
#		return pJgenApp.AppendLogToOutputWnd(str(traceback.format_exc()))	

def selftest():
    print "CADInteractive"

if __name__ == "__main__":
    selftest()
