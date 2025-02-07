// PipingDrawing.cpp : Defines the initialization routines for the DLL.
//

#include "stdafx.h"
#include <assert.h>

#include <IsTools.h>
#include <util/FileTools.h>
#include <util/Path.h>
#include <util/SplitPath.h>
#include <SmartDrawAnnoEnv.h>

#include <SmartDrawPDSModuleImpl.h>
#include <SmartDrawAnno.h>

#include <NozzleChart.h>
#include <RevisionChart.h>
#include <TitleBlockData.h>

#include "PipingDrawing.h"
#include "PipingDrawingImpl.h"
#include "PipingAnnoTerritory.h"
#include "PipingInternalTerritory.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

//
//TODO: If this DLL is dynamically linked against the MFC DLLs,
//		any functions exported from this DLL which call into
//		MFC must have the AFX_MANAGE_STATE macro added at the
//		very beginning of the function.
//
//		For example:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// normal function body here
//		}
//
//		It is very important that this macro appear in each
//		function, prior to any calls into MFC.  This means that
//		it must appear as the first statement within the 
//		function, even before any object variable declarations
//		as their constructors may generate calls into the MFC
//		DLL.
//
//		Please see MFC Technical Notes 33 and 58 for additional
//		details.
//

// CPipingDrawingApp

BEGIN_MESSAGE_MAP(CPipingDrawingApp, CWinApp)
END_MESSAGE_MAP()


// CPipingDrawingApp construction

CPipingDrawingApp::CPipingDrawingApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_sDrawingName = _T("Piping");
	m_sHsrType = _T("Generate,Import");
}


// The one and only CPipingDrawingApp object

CPipingDrawingApp theApp;


// CPipingDrawingApp initialization

BOOL CPipingDrawingApp::InitInstance()
{
	//AFX_MANAGE_STATE(AfxGetStaticModuleState());
	CWinApp::InitInstance();

	return TRUE;
}

CPipingDrawingImpl::CPipingDrawingImpl(const CString& sOutputFormat) : m_sOutputFormat(sOutputFormat)
{
}

CPipingDrawingImpl::~CPipingDrawingImpl()
{
}

/******************************************************************************
    @author     humkyung
    @date       2012-03-09
    @class      CPipingDrawingImpl
    @function   Annotate
    @return     int
    @param      const STRING_T& sOutputFilePath
	@param		const STRING_T& sHsrFilePath
    @param      CaDraw_DraFile* pDraFile
    @brief
******************************************************************************/
int CPipingDrawingImpl::Annotate(const STRING_T& sOutputFilePath , const STRING_T& sHsrFilePath , CaDraw_DraFile* pDraFile , const double& dDrawingScale)
{
	assert(pDraFile && "pDraFile is NULL");

	if(pDraFile)
	{
#ifdef	NDEBUG
		//AfxGetApp()->GetMainWnd()->SendMessage(SD_LOG , WPARAM(_T("Parse hsr file")) );
#endif
		this->ParseHsrTextFile(sHsrFilePath);
#ifdef	NDEBUG
		//AfxGetApp()->GetMainWnd()->SendMessage(SD_LOG , WPARAM(_T("Annotate")) );
#endif
		pDraFile->CreateLogicView();
		list<CaDraw_LogicView*> oLogicViewList;
		pDraFile->GetLogicViewList(&oLogicViewList);
		for(list<CaDraw_LogicView*>::iterator itr = oLogicViewList.begin();itr != oLogicViewList.end();++itr)
		{
			CAnnoTerritory* pAnnoTerritory = new CAnnoTerritory(*itr);
			if(NULL != pAnnoTerritory)
			{
				pAnnoTerritory->Prepare();
				pAnnoTerritory->Annotate(*m_pHsrLineList);
				m_pAnnoTerritoryList->push_back(pAnnoTerritory);
			}
		}
#ifdef	NDEBUG
		//AfxGetApp()->GetMainWnd()->SendMessage(SD_LOG , WPARAM(_T("Write")) );
#endif
		this->Write(sOutputFilePath , dDrawingScale);

		/// clear
		for(list<CAnnoTerritory*>::iterator itr = m_pAnnoTerritoryList->begin();itr != m_pAnnoTerritoryList->end();++itr)
		{
			SAFE_DELETE(*itr);
		}
		m_pAnnoTerritoryList->clear();
		/// up to here

		return ERROR_SUCCESS;
	}

	return ERROR_INVALID_PARAMETER;
}

/******************************************************************************
    @author     humkyung
    @date       2012-03-11
    @function   Write
    @return     int
    @param      const STRING_T& sOutputFilePath
    @brief
******************************************************************************/
int CPipingDrawingImpl::Write(const STRING_T& sOutputFilePath , const double& dDrawingScale)
{
	OFSTREAM_T ofile(sOutputFilePath.c_str());
	if(ofile.is_open())
	{
		ofile.precision( 5 );
		ofile.setf(ios_base:: fixed, ios_base:: floatfield);

		CaDraw_TagEnv& env = CaDraw_TagEnv::GetInstance();

		if(_T("AUTOCAD") == m_sOutputFormat) ofile << _T("<LINETYPE>|*,acad.lin") << std::endl;	/// 2015.04.04 added by humkyung
		env.m_extDimStyle.Write(ofile , dDrawingScale);
		env.m_intDimStyle.Write(ofile , dDrawingScale);
		
		for(list<CAnnoTerritory*>::iterator itr = m_pAnnoTerritoryList->begin();itr != m_pAnnoTerritoryList->end();++itr)
		{
			(*itr)->Write(ofile , dDrawingScale);
		}

		this->WriteBorderData(ofile , dDrawingScale);
		this->WriteNozzleChart(ofile, dDrawingScale);

		/// load title block data and write to annotation file
		CTitleBlockData oTitleBlockData;
		oTitleBlockData.Load(m_sIniFilePath.c_str() , m_sDraFileName.c_str());
		oTitleBlockData.Write(ofile,dDrawingScale);
		/// up to here

		/// load revision chart data and write to annotation file
		this->WriteRevisionChart(ofile , m_sDraFileName,dDrawingScale);
		/// up to here

		ofile.close();

		return ERROR_SUCCESS;
	}

	return ERROR_FILE_NOT_FOUND;
}

/******************************************************************************
    @author     humkyung
    @date       2012-03-09
    @class
    @function   GetModelFolderPathWith
    @return     STRING_T
    @param      CaDraw_Entity::MODEL_TYPE_T modelType
    @param      const                       STRING_T&
    @param      sPrjFilePath
    @brief
******************************************************************************/
STRING_T GetModelFolderPathWith(CaDraw_Entity::MODEL_TYPE_T modelType , const STRING_T& sPrjFilePath)
{
	TCHAR szBuf[MAX_PATH + 1]={'\0',};
	
	memset(szBuf , '\0' , sizeof(TCHAR)*(MAX_PATH + 1));
	switch(modelType)
	{
	case CaDraw_Entity::PIPE_MODEL_T:
		GetPrivateProfileString(_T("Folder") , _T("Pipe") , _T("") , szBuf , MAX_PATH , sPrjFilePath.c_str());
		break;
	case CaDraw_Entity::EQUIPMENT_MODEL_T:
		GetPrivateProfileString(_T("Folder") , _T("Equipment") , _T("") , szBuf , MAX_PATH , sPrjFilePath.c_str());
		break;
	case CaDraw_Entity::STRUCTURE_MODEL_T:
		GetPrivateProfileString(_T("Folder") , _T("Structure") , _T("") , szBuf , MAX_PATH , sPrjFilePath.c_str());
		break;
	case CaDraw_Entity::CABLE_T_MODEL_T:
		GetPrivateProfileString(_T("Folder") , _T("CableT") , _T("") , szBuf , MAX_PATH , sPrjFilePath.c_str());
		break;
	case CaDraw_Entity::MISC_MODEL_T:
		GetPrivateProfileString(_T("Folder") , _T("Misc") , _T("") , szBuf , MAX_PATH , sPrjFilePath.c_str());
		break;
	case CaDraw_Entity::DRAWING_MODEL_T:
		GetPrivateProfileString(_T("Folder") , _T("Drawing") , _T("") , szBuf , MAX_PATH , sPrjFilePath.c_str());
		break;
	};

	return szBuf;
}

/******************************************************************************
    @author     humkyung
    @date       2012-03-09
    @class
    @function   GetDrawingName
    @return     LPCTSTR
    @brief
******************************************************************************/
extern "C" __declspec(dllexport) 
LPCTSTR __stdcall GetDrawingName()
{
	return (theApp.m_sDrawingName.operator LPCTSTR());
}

extern "C" __declspec(dllexport) 
LPCTSTR __stdcall GetDialogList()
{
	return (theApp.m_sDrawingName.operator LPCTSTR());
}

extern "C" __declspec(dllexport) 
HWND __stdcall CreatePropertyPage(LPCTSTR pName , LPCTSTR pIniFilePath , HWND hParent)
{
	return NULL;
}

extern "C" __declspec(dllexport) 
int __stdcall SaveSetting()
{
	return ERROR_SUCCESS;
}

extern "C" __declspec(dllexport) 
void __stdcall ClosePropertyPage(LPCTSTR pName)
{
}

/******************************************************************************
    @author     humkyung
    @date       2012-03-09
    @function   GetHsrType
    @return     LPCTSTR
    @brief
******************************************************************************/
extern "C" __declspec(dllexport) 
LPCTSTR __stdcall GetHsrType()
{
	return (theApp.m_sHsrType.operator LPCTSTR());
}

/******************************************************************************
    @brief		init report
	@author     humkyung
    @date       2016-02-25
    @return     int
******************************************************************************/
extern "C" __declspec(dllexport) 
int __stdcall InitReport(LPCTSTR pSqliteFilePath)
{
	soci::session oSession(*soci::factory_sqlite3() , pSqliteFilePath);
	{
		STRING_T sSql = _T("create table if not exists REPORTS("
			"Name varchar(256) not null)");
		oSession << sSql;
		oSession << _T("delete from REPORTS");
		{
			oSession << _T("insert into REPORTS values('Pipe')");
			oSession << _T("insert into REPORTS values('Equipment')");
			oSession << _T("insert into REPORTS values('Nozzle')");
			oSession << _T("insert into REPORTS values('Instrument')");
			oSession << _T("insert into REPORTS values('Specialty')");
			oSession << _T("insert into REPORTS values('Support')");
			oSession << _T("insert into REPORTS values('Valve')");
		}

		///create Drawing table if not exits
		oSession << _T("drop table if exists Drawing");
		sSql = _T("create table if not exists Drawing("
		"Guid varchar(36) not null,"
		"Name varchar(256) not null,"
		"RevNo int,"
		"RevStr varchar(8))");
		oSession << sSql;
		///up to here

		///create View table if not exits
		oSession << _T("drop table if exists View");
		sSql = _T("create table if not exists View("
		"Guid varchar(36) not null,"
		"DocGuid varchar(36) not null,"
		"Name varchar(16) not null,"
		"MintX real,MinY real,MinZ real,MaxX real,MaxY real,MaxZ real,"
		"X real,Y real,Z real,Width real,Height real,"
		"Scale real,Rotate real,Dir varchar(8),"
		"Match1 varchar(32),Match2 varchar(32),Match3 varchar(32),"
		"Match4 varchar(32),Match5 varchar(32),Match6 varchar(32),"
		"Info varchar(32))");
		oSession << sSql;

		///create tables if not exits
		oSession << _T("drop table if exists Pipe");
		sSql = _T("create table if not exists Pipe("
		"ViewGuid varchar(36) not null,"
		"Id varchar(64) not null,"
		"Label varchar(64),"
		"Spec varchar(64) not null,"
		"Size real)");
		oSession << sSql;
		
		oSession << _T("drop table if exists Equipment");
		sSql = _T("create table if not exists Equipment("
		"ViewGuid varchar(36) not null,"
		"Name varchar(64) not null,"
		"Name2 varchar(64) not null,"
		"Desc varchar(64) not null,"
		"OriginX real,OriginY real,OriginZ real)");
		oSession << sSql;
	
		oSession << _T("drop table if exists Structure");
		sSql = _T("create table if not exists Structure("
		"ViewGuid varchar(36) not null,"
		"Name1 varchar(64) not null,"
		"Name2 varchar(64) not null,"
		"OriginX real,OriginY real,OriginZ real)");
		oSession << sSql;
	
		oSession << _T("drop table if exists Nozzle");
		sSql = _T("create table if not exists Nozzle("
		"ViewGuid varchar(36) not null,"
		"Name varchar(64) not null,"
		"EqpName varchar(64) not null,"
		"Rating varchar(64) not null,"
		"Code varchar(64) not null,"
		"Type varchar(64) not null,"
		"Orientation varchar(64) not null,"
		"Projection varchar(64) not null,"
		"Size varchar(16) not null)");
		oSession << sSql;
	
		oSession << _T("drop table if exists Valve");
		sSql = _T("create table if not exists Valve("
		"ViewGuid varchar(36) not null,"
		"Name varchar(64) not null,"
		"Remark varchar(64) not null)");
		oSession << sSql;
	
		oSession << _T("drop table if exists Specialty");
		sSql = _T("create table if not exists Specialty("
		"ViewGuid varchar(36) not null,"
		"Name varchar(64) not null)");
		oSession << sSql;
		
		oSession << _T("drop table if exists Instrument");	
		sSql = _T("create table if not exists Instrument("
		"ViewGuid varchar(36) not null,"
		"Name varchar(64) not null)");
		oSession << sSql;
	
		oSession << _T("drop table if exists Support");	
		sSql = _T("create table if not exists Support("
		"ViewGuid varchar(36) not null,"
		"Name varchar(64) not null,"
		"LineNo varchar(64) not null,"
		"ShopName varchar(64),"
		"FieldName varchar(64),"
		"CommodityCode varchar(64))");
		oSession << sSql;
		///up to here
	}

	return ERROR_SUCCESS;
}

/******************************************************************************
    @brief		report for a drawing
	@author     humkyung
    @date       2016-02-25
    @return     int
******************************************************************************/
extern "C" __declspec(dllexport) 
int __stdcall Report(CaDraw_DraFile* pDraFile , const CIsPoint3d& ptModelOffset , LPCTSTR pPrjFolderPath , LPCTSTR pPrjName , LPCTSTR pSettingFileName)
{
	assert((pDraFile && pPrjFolderPath && pPrjName && pSettingFileName) && _T("parameter is null"));
	if((NULL != pDraFile) && (NULL != pPrjFolderPath) && (NULL != pPrjName) && (NULL != pSettingFileName))
	{
		try
		{
			const CString sIniFilePath = (pPrjFolderPath + CString(_T("\\Setting\\")) + pSettingFileName + CString(_T(".ini")));
			const CString sPrjFilePath = (pPrjFolderPath + CString(_T("\\Setting\\")) + pPrjName + CString(_T(".prj")));
			const CString sDatabaseFilePath = (pPrjFolderPath + CString(_T("\\Database\\")) + pPrjName + CString(_T(".mdb")));
			const CString sReportFilePath = (pPrjFolderPath + CString(_T("\\Database\\")) + pPrjName + CString(_T(".db3")));

			list<CaDraw_View*> oViewList;
			pDraFile->GetViewList(&oViewList);
			for(list<CaDraw_View*>::const_iterator itr = oViewList.begin();itr != oViewList.end();++itr)
			{
				list<CaDraw_View::MODEL_T>* pModelList = (*itr)->GetModelList();
				for(list<CaDraw_View::MODEL_T>::const_iterator jtr = pModelList->begin();jtr != pModelList->end();++jtr)
				{
					CaDraw_Entity::MODEL_TYPE_T eModelType = (jtr->modelType);
					STRING_T sModelFolderPath = GetModelFolderPathWith(eModelType , sPrjFilePath.operator LPCTSTR());
					if(!sModelFolderPath.empty() && ('\\' != sModelFolderPath[sModelFolderPath.length() - 1])) sModelFolderPath += _T("\\");
					const STRING_T sModelFilePath = sModelFolderPath + jtr->rModelName.c_str();

					CSmartDrawPDSModuleImpl oCadModule;
					oCadModule.ExtractEntities(*itr , eModelType , sModelFilePath , sIniFilePath.operator LPCTSTR() , sDatabaseFilePath.operator LPCTSTR() , ptModelOffset);
				}
			}
			pDraFile->Report(sReportFilePath.operator LPCTSTR());
			return ERROR_SUCCESS;
		}
		catch(...)
		{
		}
	}

	return ERROR_BAD_ENVIRONMENT;
}

/******************************************************************************
    @brief
	@author     humkyung
    @date       2016-02-11
    @function   GenerateReport
    @return     int
******************************************************************************/
//extern "C" __declspec(dllexport) 
//int __stdcall GenerateReport(LPCTSTR pOutputFilePath , LPCTSTR pDraFilePath , LPCTSTR pPrjName , LPCTSTR pPrjFolderPath , const double& dDrawingScale , const double dModelOffset[3])
//{
//	assert(pOutputFilePath && "pOutputFilePath is NULL");
//	assert(pDraFilePath && "pDraFilePath is NULL");
//	assert(pPrjName && "pPrjName is NULL");
//	assert(pPrjFolderPath && "pPrjFolderPath is NULL");
//
//	if(pOutputFilePath && pDraFilePath && pPrjName && pPrjFolderPath)
//	{
//		TCHAR szBuf[MAX_PATH + 1]={'\0',};
//
//		try
//		{
//			const STRING_T sPrjFilePath(pPrjFolderPath + STRING_T(_T("\\Setting\\")) + pPrjName + STRING_T(_T(".prj")));
//			
//			STRING_T sDatabaseFilePath , sIniFilePath;
//			GetPrivateProfileString(_T("Database") , _T("Type") , _T("Access") , szBuf , MAX_PATH , sPrjFilePath.c_str());
//			if(STRING_T(_T("Access")) == STRING_T(szBuf))
//			{
//				if(0 == GetPrivateProfileString(_T("Database") , _T("Access File") , _T("") , szBuf , MAX_PATH , sPrjFilePath.c_str()))
//				{
//					return ERROR_BAD_ENVIRONMENT;
//				}
//				sDatabaseFilePath.assign(szBuf);
//			}
//			
//			auto_ptr<CaDraw_DraFile> pDraFile(new CaDraw_DraFile(pDraFilePath));
//			if(ERROR_SUCCESS == pDraFile->Parse())
//			{
//				sIniFilePath = (pPrjFolderPath + STRING_T(_T("\\Setting\\")) + pDraFile->GetSettingFileName() + STRING_T(_T(".ini")));
//
//				CaDraw_TagEnv& env = CaDraw_TagEnv::GetInstance();
//				env.Parse(sIniFilePath);
//
//				list<CaDraw_View*> oViewList;
//				pDraFile->GetViewList(&oViewList);
//				for(list<CaDraw_View*>::iterator itr = oViewList.begin();itr != oViewList.end();++itr)
//				{
//					list<CaDraw_View::MODEL_T>* pModelList = (*itr)->GetModelList();
//					for(list<CaDraw_View::MODEL_T>::iterator jtr = pModelList->begin();jtr != pModelList->end();++jtr)
//					{
//						CaDraw_Entity::MODEL_TYPE_T modelType = (jtr->modelType);
//						STRING_T sModelFolderPath = GetModelFolderPathWith(modelType , sPrjFilePath);
//						if(!sModelFolderPath.empty() && ('\\' != sModelFolderPath[sModelFolderPath.length() - 1])) sModelFolderPath += _T("\\");
//						STRING_T rModelFilePath = sModelFolderPath + jtr->rModelName.c_str();
//
//						CSmartDrawPDSModuleImpl oCadModule;
//						CIsPoint3d ptModelOffset(dModelOffset[0] , dModelOffset[1] , dModelOffset[2]);
//						oCadModule.ExtractEntities(*itr , modelType , rModelFilePath , sIniFilePath , sDatabaseFilePath , ptModelOffset);
//					}
//				}
//
//				const STRING_T sReportFilePath = pPrjFolderPath + _T("\\Database\\") + sPrjName + _T(".db");
//				pDraFile->GenerateReport(sReportFilePath);
//			}
//		}
//		return ERROR_SUCCESS;
//	}
//
//	return ERROR_BAD_ENVIRONMENT;
//}

extern "C" __declspec(dllexport) 
int __stdcall Annotate(LPCTSTR pOutputFilePath , LPCTSTR pDraFilePath , LPCTSTR pPrjName , LPCTSTR pPrjFolderPath , const double& dDrawingScale , const double dModelOffset[3])
{
	assert(pOutputFilePath && "pOutputFilePath is NULL");
	assert(pDraFilePath && "pDraFilePath is NULL");
	assert(pPrjName && "pPrjName is NULL");
	assert(pPrjFolderPath && "pPrjFolderPath is NULL");

	if(pOutputFilePath && pDraFilePath && pPrjName && pPrjFolderPath)
	{
		TCHAR szBuf[MAX_PATH + 1]={'\0',};

		try
		{
			STRING_T sPrjFilePath(pPrjFolderPath + STRING_T(_T("\\Setting\\")) + pPrjName + STRING_T(_T(".prj")));
			
			STRING_T sDatabaseFilePath , sIniFilePath , sRefFilePath;
			/// get database file path when type is 'Access' - 2014.08.02 added by humkyung
			GetPrivateProfileString(_T("Database") , _T("Type") , _T("Access") , szBuf , MAX_PATH , sPrjFilePath.c_str());
			if(STRING_T(_T("Access")) == STRING_T(szBuf))
			{
			/// up to here
				if(0 == GetPrivateProfileString(_T("Database") , _T("Access File") , _T("") , szBuf , MAX_PATH , sPrjFilePath.c_str()))
				{
					return ERROR_BAD_ENVIRONMENT;
				}
				sDatabaseFilePath.assign(szBuf);
				///get ref file path:#18(http://atools.co.kr:9002/redmine/issues/18) - 2016.03.12 added by humkyung
				STRING_T::size_type at = sDatabaseFilePath.rfind('.');
				sRefFilePath = sDatabaseFilePath.substr(0,at)+_T(".db3");
				///up to here
			}
			
			auto_ptr<CaDraw_DraFile> pDraFile(new CaDraw_DraFile(pDraFilePath));
			if(ERROR_SUCCESS == pDraFile->Parse())
			{
				sIniFilePath = (pPrjFolderPath + STRING_T(_T("\\Setting\\")) + pDraFile->GetSettingFileName() + STRING_T(_T(".ini")));

				CaDraw_TagEnv& env = CaDraw_TagEnv::GetInstance();
				env.Parse(sIniFilePath,sRefFilePath);

				list<CaDraw_View*> oViewList;
				pDraFile->GetViewList(&oViewList);
				for(list<CaDraw_View*>::iterator itr = oViewList.begin();itr != oViewList.end();++itr)
				{
					list<CaDraw_View::MODEL_T>* pModelList = (*itr)->GetModelList();
					for(list<CaDraw_View::MODEL_T>::iterator jtr = pModelList->begin();jtr != pModelList->end();++jtr)
					{
						CaDraw_Entity::MODEL_TYPE_T modelType = (jtr->modelType);
						STRING_T sModelFolderPath = GetModelFolderPathWith(modelType , sPrjFilePath);
						if(!sModelFolderPath.empty() && ('\\' != sModelFolderPath[sModelFolderPath.length() - 1])) sModelFolderPath += _T("\\");
						STRING_T rModelFilePath = sModelFolderPath + jtr->rModelName.c_str();

						CSmartDrawPDSModuleImpl oCadModule;
						CIsPoint3d ptModelOffset(dModelOffset[0] , dModelOffset[1] , dModelOffset[2]);
						oCadModule.ExtractEntities(*itr , modelType , rModelFilePath , sIniFilePath , sDatabaseFilePath , ptModelOffset);
					}
				}

				const STRING_T sFileNameWithoutExt = pDraFile->GetFileNameWithoutExt();
				////////////////////////////////////////////////////////////////////////////////////////////////////
				/// hsr 파일은 SmartHSR을 통해서 생성됨.
				STRING_T sHsrFilePath = pPrjFolderPath + STRING_T(_T("\\Working\\")) + sFileNameWithoutExt + _T(".hsr");

				////////////////////////////////////////////////////////////////////////////////////////////////////
				/// run annotation
				const STRING_T sAnnoFilePath = pPrjFolderPath + STRING_T(_T("\\Working\\")) + sFileNameWithoutExt + _T(".anno");
				STRING_T sDrawingModelFolderPath = GetModelFolderPathWith(CaDraw_Entity::DRAWING_MODEL_T , sPrjFilePath);

				/// get output format - 2015.04.04 added by humkyung
				CString sOutputFormat;
				if(GetPrivateProfileString(_T("GENERATION_OPTIONS") , _T("Output Format") , NULL , szBuf , MAX_PATH , sIniFilePath.c_str()))
				{
					sOutputFormat = szBuf;
					sOutputFormat.MakeUpper();
				}
				/// up to here
				{
					auto_ptr<CPipingDrawingImpl> oDrawing( new CPipingDrawingImpl(sOutputFormat) );
					{
						oDrawing->m_sIniFilePath = sIniFilePath;
						CSplitPath path(pDraFilePath);
						oDrawing->m_sDraFileName = path.GetFileName() + path.GetExtension();
					}
					oDrawing->Annotate(sAnnoFilePath.c_str() , sHsrFilePath.c_str() , pDraFile.get() , dDrawingScale);
				}

				/// write hsr + anno to output file
				/// copy border file to output file
				TCHAR szBuf[MAX_PATH + 1]={'\0',};
				if(GetPrivateProfileString(_T("GENERAL") , _T("Border File") , _T("") , szBuf , MAX_PATH , sIniFilePath.c_str()))
				{
					if(FALSE == ::CopyFile(pPrjFolderPath + CString(_T("\\Border\\")) + szBuf , pOutputFilePath , FALSE))
					{
						AfxMessageBox(_T("Fail to copy border file to output file") , MB_OK);
					}
				}
				else
				{
					/// use seed file
					if(_T("MSTN") == sOutputFormat)
					{
						::CopyFile(CFileTools::GetCommonAppDataPath() + _T("\\SmartDraw_PDS\\Seed\\seed3d.dgn") , pOutputFilePath , FALSE);
					}
					else
					{
						::CopyFile(CFileTools::GetCommonAppDataPath() + _T("\\SmartDraw_PDS\\Seed\\seed.dwg") , pOutputFilePath , FALSE);
					}
				}
				/// up to here
			}

			return ERROR_SUCCESS;
		}
		/// 2012.12.24 added by humkyung
		catch(const exception& ex)
		{
			AfxGetApp()->GetMainWnd()->SendMessage(SD_MESSAGE , WPARAM(ex.what()) , 0);
		}
		/// up to here
	}

	return ERROR_INVALID_PARAMETER;
}