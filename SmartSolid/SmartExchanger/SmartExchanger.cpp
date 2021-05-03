// SmartExchanger.cpp : Defines the entry point for the DLL application.
//

#include "stdafx.h"
#include <DGNLib.h>

#include "SmartExchanger.h"
#include "verinfo.h"
#include "VersionNo.h"

#include <list>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
using namespace std;

HANDLE hInst=NULL;
BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	hInst = hModule;
	return TRUE;
}

double g_nUOR=1.;


#ifndef SMARTEXCHANGER_EXPORTS
	#define DLL_EXPORT __declspec(dllexport)
#endif
#ifndef SMARTEXCHANGER_EXPORTS
	#define DLL_IMPORT __declspec(dllimport)
#endif

string GetFileSize(LPCTSTR lpFilePath)
{
	string res;

	FILE* fp = fopen(lpFilePath , "rb");
	if(NULL != fp)
	{
		fseek(fp , 0 , SEEK_END);
		const long lSize = ftell(fp);
		fclose(fp);

		ostringstream oss;
		oss << lSize;
		res = oss.str();
	}
	//// CreateFile�� GENERIC_READ �ɼ��� ����Ͽ� ReadMe.txt������ ����.
	//HANDLE h_file = CreateFile(lpFilePath , GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	//if(h_file != INVALID_HANDLE_VALUE)
	//{
	//	FILETIME create_time, access_time, write_time;

	//	// ������ ���Ͽ��� ������ ����, �ֱ� ��� �׸��� �ֱ� ���ŵ� �ð��� ��´�.
	//	GetFileTime(h_file, &create_time, &access_time, &write_time);

	//	SYSTEMTIME write_system_time, write_local_time;
	//	// FILETIME ����ü������ SYSTEMTIME ����ü ������ ��ȯ�Ѵ�. FILETIME ����ü�� ����ִ� ������
	//	// �츮�� ���������� �̿��Ϸ��� ����� ���������� ������ ����ϱ� ���� SYSTEMTIME ����ü
	//	// �������� ��ȯ�ؼ� ����Ѵ�.
	//	FileTimeToSystemTime(&write_time, &write_system_time);

	//	// FILETIME ����ü���� SYSTEMTIME ����ü�� ��ȯ�Ǹ� �ð������� UTC(Universal Time Coordinated) ������ 
	//	// �ð��̱� ������ Ž���⳪ ��Ÿ ���α׷����� ���� �ð��� �ٸ���. ���� ��Ȯ�� �ð������� ��� ���ؼ�
	//	// UTC ������ �ð��� �����ð����� ��ȯ�ؾ� �Ѵ�. �Ʒ��� �Լ��� �ش� �۾��� �ϴ� �Լ��̴�.
	//	SystemTimeToTzSpecificLocalTime(NULL, &write_system_time, &write_local_time);

	//	// write_local_time �� ����ϸ� �ȴ�..
	//	ostringstream oss;
	//	oss << write_local_time.wYear << "-" << write_local_time.wMonth << "-" << write_local_time.wDay << " ";
	//	oss << write_local_time.wHour << ":" << write_local_time.wMinute;
	//	res = oss.str();

	//	CloseHandle(h_file);
	//}

	return res;
}

/**	
	@brief	convert .dgn to .nsq

	@author	humkyung

	@param	pExportFilePath	a parameter of type const char*
	@param	pImportFilePath	a parameter of type const char*
	@param	pIniFilePath ȯ�� ���� ���� ���

	@return	void	
*/
extern "C" DLL_EXPORT
void __stdcall Dgn2Sxf(const TCHAR* pExportFilePath , const int& iModelType , const TCHAR* pImportFilePath , const TCHAR* pIniFilePath , const double& dOffsetX , const double& dOffsetY , const double& dOffsetZ )
{
	assert(pExportFilePath && "pExportFilePath is NULL");
	assert(pImportFilePath && "pImportFilePath is NULL");
	assert(pIniFilePath && "pIniFilePath is NULL");
	static const string rApp("HLR_OPTIONS");

	if(pExportFilePath && pImportFilePath && pIniFilePath)
	{
		int ver[4]={0,};
		sscanf(STRFILEVER , _T("%d,%d,%d,%d") , &ver[0] , &ver[1] , &ver[2], &ver[3]);
		ostringstream oss;
		oss << ver[0] << "," << ver[1] << "," << ver[2];
		string sVer = oss.str();

		string sLastWriteTime = GetFileSize(pImportFilePath);

		/// check version and date
		ifstream ifile(pExportFilePath);
		if(ifile.is_open())
		{
			string sLastVer , sLastDate;
			getline(ifile , sLastVer);
			getline(ifile , sLastDate);
			ifile.close();

			if((string("#" + sVer) == sLastVer) && (string("#" + sLastWriteTime) == sLastDate)) return;
		}
		/// up to here

		CDGNLib* pDGNLib=CDGNLib::GetInstance();
		if(pDGNLib && pDGNLib->Load(pImportFilePath))
		{
			CDGNDoc* pDoc=pDGNLib->GetDoc();
			if(pDoc)
			{
				PSQPRIMITIVE pHead=pDoc->GetHead();

				TCHAR szBuf[MAX_PATH + 1] = {'\0',};
				int nSkipLevel = 0;
				if(GetPrivateProfileString(rApp.c_str() , _T("InsulationLevel") , _T("") , szBuf , MAX_PATH , pIniFilePath))
				{
					nSkipLevel = atoi(szBuf);
				}
				if(GetPrivateProfileString(rApp.c_str() , _T("InsulationDisplay") , _T("") , szBuf , MAX_PATH , pIniFilePath))
				{
					if(string("Remove") != szBuf) nSkipLevel = 0;
				}

				SxfExportFile(pExportFilePath , iModelType , pHead , nSkipLevel , sVer , sLastWriteTime , dOffsetX , dOffsetY , dOffsetZ);
			}
		}
	}
}