// PreTEEFormat.cpp: implementation of the CTeeElement class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <math\ITKMath.h>
#include <graphics\geometry.h>
#include "TeeElement.h"
#include "SmartHSR.h"

IMPLEMENTS_HSR_ELEMENT(CTeeElement , CHSRElement , _T("TEE"));
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
/**	
	@brief
*/
CTeeElement::CTeeElement()
{
	m_nSegments = 16;
	m_bAdjusted = false;
}

/**	
	@brief
*/
CTeeElement::~CTeeElement()
{

}

#ifdef VER_03
/**	\brief	The CTeeElement::ParseLine function

	\param	pPreFormatScanner	a parameter of type CHSRScanner*

	\return	bool	
*/
bool CTeeElement::ParseLine(CHSRScanner* pPreFormatScanner){
	assert(pPreFormatScanner && "pPreFormatScanner is NULL");
	bool bRet=false;

	if(pPreFormatScanner){
		if(10 == pPreFormatScanner->m_nFactor){
			m_nPIPEs = 2;
			bRet = ParseBWLine(pPreFormatScanner);
		}else if(22 == pPreFormatScanner->m_nFactor){
			m_nPIPEs = 5;
			ParseSWLine(pPreFormatScanner);

			bRet = true;
		}
	}

	return bRet;
}
#else
/**	
	@brief	The CTeeElement::ParseLine function
	@author	humkyung
	@param	pPreFormatScanner	a parameter of type CHSRScanner*
	@return	bool	
*/
bool CTeeElement::ParseLine(CHSRScanner* pHSRScanner)
{
	assert(pHSRScanner && "pHSRScanner is NULL");
	bool bRet=false;

	if(pHSRScanner)
	{
		if( (15 == pHSRScanner->m_nFactor) || (16 == pHSRScanner->m_nFactor) )
		{
			m_nPIPEs = 2;
			bRet = ParseBWLine(pHSRScanner);
		}
		else if(pHSRScanner->m_nFactor >= 36)
		{
			m_nPIPEs = 5;
			bRet = ParseSWLine(pHSRScanner);
		}
	}

	return bRet;
}
#endif

//	parameter	:
//	description	:
//	remarks		:
//	returns		:
bool CTeeElement::IsInVolume(const CIsVolume& volume)
{
	for(int i=0;i < m_nPIPEs;i++)
	{
		if(	(m_pt[i][0].x >= volume.minx()) && (m_pt[i][0].x <= volume.maxx()) &&
			(m_pt[i][0].y >= volume.miny()) && (m_pt[i][0].y <= volume.maxy()) &&
			(m_pt[i][0].z >= volume.minz()) && (m_pt[i][0].z <= volume.maxz())) return true;
		
		if(	(m_pt[i][1].x >= volume.minx()) && (m_pt[i][1].x <= volume.maxx()) &&
			(m_pt[i][1].y >= volume.miny()) && (m_pt[i][1].y <= volume.maxy()) &&
			(m_pt[i][1].z >= volume.minz()) && (m_pt[i][1].z <= volume.maxz())) return true;
	}
	
	return false;
}

/**	
	@brief	The CTeeElement::CreateFace function

	@param	pVIEWFormat	a parameter of type CPreViewFormat*
	@param	nID	a parameter of type long&

	@return	PFACE	
*/
CHSRFace* CTeeElement::CreateFace(CHSRViewFormat* pVIEWFormat,long& nID)
{
	assert(pVIEWFormat && "pVIEWFormat is NULL");
	CHSRFace* pRet=NULL;

	if(pVIEWFormat && m_bAdjusted)
	{
		if(2 == m_nPIPEs)
			pRet = CreateBWFace(pVIEWFormat,nID);
		else if(5 == m_nPIPEs)
			pRet = CreateSWFace(pVIEWFormat,nID);
	}

	return pRet;
}

/**	@brief	The CTeeElement::CreateBWFace function
	@author	�����

	@param	pVIEWFormat	a parameter of type CPreViewFormat*
	@param	nID	a parameter of type long&

	@remarks\n
	;2003.03.13 - 

	@return	PFACE	
*/
CHSRFace* CTeeElement::CreateBWFace(CHSRViewFormat* pVIEWFormat,long& nID)
{
	assert(pVIEWFormat && "pVIEWFormat is NULL");
	CHSRFace* pRet=NULL;

	CHSRApp* pHSRApp=CHSRApp::GetInstance();
	if(pVIEWFormat && pHSRApp)
	{
		CHSRFace* pTop[2]   ={NULL,};
		CHSRFace* pSide[2]  ={NULL,};
		CHSRFace* pBottom[2]={NULL,};

		PIPELINEMODE mode=pHSRApp->GetEnv()->GetPipeLineMode();
		double nCriticalRadius = pHSRApp->GetEnv()->GetCriticalRadius();

		static POINT_T ptEDGE[36];
		POINT_T pt[2]={0.,};
		double  nRadius[2]={0.,};
		for(int i=0;i < m_nPIPEs;i++)
		{
			pTop[i] = pSide[i] = pBottom[i] = NULL;
			
			pt[0]  = pVIEWFormat->MODEL2VIEW(m_pt[i][0]);
			pt[1]  = pVIEWFormat->MODEL2VIEW(m_pt[i][1]);
			nRadius[0] = pVIEWFormat->MODEL2VIEW(m_nRadius[i][0]);
			nRadius[1] = pVIEWFormat->MODEL2VIEW(m_nRadius[i][1]);
			bool bSingleLine=((SINGLE_LINE == mode) || ((BOTH == mode) && nRadius[0] <= nCriticalRadius)) ? true : false;

			VECTOR_T vecAxis={0.},vecZ={0.,0.,1.};
			vecAxis.dx=pt[1].x - pt[0].x;
			vecAxis.dy=pt[1].y - pt[0].y;
			vecAxis.dz=pt[1].z - pt[0].z;
			if(!CMath::NormalizeVector(vecAxis)) return NULL;
			
			double nDot=CMath::DotProduct(vecAxis,vecZ);
			
			 /// VERTICAL
			if(fabs(fabs(nDot) - 1) < 0.001)
			{
				POINT_T ptOrigin=(pt[0].z > pt[1].z) ? pt[0] : pt[1];
				ptOrigin.z = (pt[0].z + pt[1].z)*0.5;
				if((1 == i) && bSingleLine)
				{
					if(pt[0].z > pt[1].z)
					{
						/// 2'nd pipe's top face is intersection point of tee.
						if(pTop[i] = pHSRApp->new_hsr_face()){
							POINT_T ptLine[2]={0.,};
							ptLine[0] = pVIEWFormat->MODEL2VIEW(m_pt[0][0]);
							ptLine[1] = pVIEWFormat->MODEL2VIEW(m_pt[0][1]);
							
							LINE_T line={0};
							line.ptStart = ptLine[0];
							line.ptEnd   = ptLine[1];
							PHSR_VERTEX pSymbol=CreateTeeSymbol(line,nRadius[0]);
							if(pSymbol){
								int nCount=0;
								for(PHSR_VERTEX ptr=pSymbol;ptr;ptr = ptr->next,nCount++){
									ptr->pt->x+= ptOrigin.x;
									ptr->pt->y+= ptOrigin.y;
									ptr->pt->z = ptOrigin.z;
								}
								if(NULL != pTop[i]->pHead) pHSRApp->free_vertex(pTop[i]->pHead);
								pTop[i]->pHead   = pSymbol;
								pTop[i]->nCount  = nCount;
								pTop[i]->CalcPlaneEquation();
								pTop[i]->type    = TEE;
								pTop[i]->ele     = SYMBOL;
								pTop[i]->radius  = nRadius[0];
								pTop[i]->ptCenter= ptOrigin;
							}else{
								if(NULL != pTop[i]) pHSRApp->delete_hsr_face(pTop[i]);
								pTop[i] = NULL;
							}
						}
					}else{ /// downward
						if(pTop[i]=CHSRFace::CreateCircleFace(ptOrigin,nRadius[0],vecAxis,m_nSegments)){
							pTop[i]->id  = nID++;
							pTop[i]->type= TEE;
							pTop[i]->ele = SYMBOL;
							pTop[i]->ptCenter = ptOrigin;
							pTop[i]->radius   = nRadius[0];
							pTop[i]->SetColor(*m_pstrColor);
						}
					}
				}else{
					if(!bSingleLine){
						if(pTop[i]=CHSRFace::CreateCircleFace(ptOrigin,nRadius[0],vecAxis,m_nSegments)){
							pTop[i]->id  = nID++;
							pTop[i]->type= TEE;
							pTop[i]->ele = (pTop[i]->IsPerpendicularToViewPoint()) ? SECTION : HSR_CIRCLE_SHAPE;
							pTop[i]->ptCenter = ptOrigin;
							pTop[i]->radius   = nRadius[0];
							pTop[i]->SetColor(*m_pstrColor);
						}
					}
				}
			// HORIZONTAL
			}
			else if(fabs(nDot) < 0.001)
			{
				VECTOR_T vecCross={0.,};
				CMath::GetCrossProduct(vecCross,vecAxis,vecZ);
				if(!CMath::NormalizeVector(vecCross)) return NULL;
				
				ptEDGE[0].x = pt[0].x + vecCross.dx*nRadius[0];
				ptEDGE[0].y = pt[0].y + vecCross.dy*nRadius[0];
				ptEDGE[0].z = (pt[0].z + nRadius[0]) + vecCross.dz*nRadius[0];
				ptEDGE[1].x = pt[0].x - vecCross.dx*nRadius[0];
				ptEDGE[1].y = pt[0].y - vecCross.dy*nRadius[0];
				ptEDGE[1].z = (pt[0].z + nRadius[0]) - vecCross.dz*nRadius[0];
				ptEDGE[2].x = pt[1].x - vecCross.dx*nRadius[1];
				ptEDGE[2].y = pt[1].y - vecCross.dy*nRadius[1];
				ptEDGE[2].z = (pt[1].z + nRadius[1]) - vecCross.dz*nRadius[1];
				ptEDGE[3].x = pt[1].x + vecCross.dx*nRadius[1];
				ptEDGE[3].y = pt[1].y + vecCross.dy*nRadius[1];
				ptEDGE[3].z = (pt[1].z + nRadius[1]) + vecCross.dz*nRadius[1];
				ptEDGE[4]   = ptEDGE[0];
				
				if(pSide[i]=CHSRFace::CreateFace(5,ptEDGE))
				{
					pSide[i]->id    = nID++;
					pSide[i]->type  = TEE;
					pSide[i]->ele   = HSR_RECTANGLE_SHAPE;
					pSide[i]->radius= nRadius[0];
					pSide[i]->SetColor(*m_pstrColor);

					POINT_T ptCenter[3]={0.,};
					ptCenter[0].x = (ptEDGE[0].x + ptEDGE[1].x)*0.5;
					ptCenter[0].y = (ptEDGE[0].y + ptEDGE[1].y)*0.5;
					ptCenter[0].z = (ptEDGE[0].z + ptEDGE[1].z)*0.5;
					ptCenter[2].x = (ptEDGE[2].x + ptEDGE[3].x)*0.5;
					ptCenter[2].y = (ptEDGE[2].y + ptEDGE[3].y)*0.5;
					ptCenter[2].z = (ptEDGE[2].z + ptEDGE[3].z)*0.5;
					ptCenter[1].x = (ptCenter[0].x + ptCenter[2].x)*0.5;
					ptCenter[1].y = (ptCenter[0].y + ptCenter[2].y)*0.5;
					ptCenter[1].z = (ptCenter[0].z + ptCenter[2].z)*0.5;
					pSide[i]->pCenterline = CHSRFace::CreateLine(3,ptCenter);

					PIPELINEMODE mode=pHSRApp->GetEnv()->GetPipeLineMode();
					double nCriticalRadius = pHSRApp->GetEnv()->GetCriticalRadius();
					if((SINGLE_LINE == mode) || ((BOTH == mode) && pSide[i]->radius <= nCriticalRadius))
					{
						if(0 == i)
						{
							pSide[i]->mark[0] = true;
							pSide[i]->mark[1] = true;
						}
						else
						{
							pSide[i]->mark[1] = true;
						}
					}
					else
					{
						//--> 2002-11-18
						if(pSide[0] && pSide[1])
						{
							pSide[0]->SplitFaceLine(pSide[1],true);
							pSide[1]->SplitFaceLine(pSide[0],true);
							for(PHSR_VERTEX ptr = pSide[1]->pHead;ptr && ptr->next;ptr = ptr->next)
								if(pSide[0]->IsHiddenLine(ptr,ptr->next))
									ptr->render = INVALID;
							for(PHSR_VERTEX ptr = pSide[0]->pHead;ptr && ptr->next;ptr = ptr->next)
								if(pSide[1]->IsHiddenLine(ptr,ptr->next))
									ptr->render = INVALID;
						}
					}
				}
			/// SLOPE
			}
			else
			{
				VECTOR_T vecCross={0};
				CMath::GetCrossProduct(vecCross,vecAxis,vecZ);
				if(!CMath::NormalizeVector(vecCross)) return NULL;
				
				ptEDGE[0].x = pt[0].x + vecCross.dx*nRadius[0];
				ptEDGE[0].y = pt[0].y + vecCross.dy*nRadius[0];
				ptEDGE[0].z = (pt[0].z + nRadius[0]) + vecCross.dz*nRadius[0];
				ptEDGE[1].x = pt[0].x - vecCross.dx*nRadius[0];
				ptEDGE[1].y = pt[0].y - vecCross.dy*nRadius[0];
				ptEDGE[1].z = (pt[0].z + nRadius[0]) - vecCross.dz*nRadius[0];
				ptEDGE[2].x = pt[1].x - vecCross.dx*nRadius[1];
				ptEDGE[2].y = pt[1].y - vecCross.dy*nRadius[1];
				ptEDGE[2].z = (pt[1].z + nRadius[1]) - vecCross.dz*nRadius[1];
				ptEDGE[3].x = pt[1].x + vecCross.dx*nRadius[1];
				ptEDGE[3].y = pt[1].y + vecCross.dy*nRadius[1];
				ptEDGE[3].z = (pt[1].z + nRadius[1]) + vecCross.dz*nRadius[1];
				ptEDGE[4]   = ptEDGE[0];
				
				if(pSide[i]=CHSRFace::CreateFace(5,ptEDGE))
				{
					pSide[i]->id     = nID++;
					pSide[i]->type   = TEE;
					pSide[i]->ele    = HSR_RECTANGLE_SHAPE;
					pSide[i]->radius = nRadius[0];
					pSide[i]->SetColor(*m_pstrColor);

					POINT_T ptCenter[3]={0.,};
					ptCenter[0].x = (ptEDGE[0].x + ptEDGE[1].x)*0.5;
					ptCenter[0].y = (ptEDGE[0].y + ptEDGE[1].y)*0.5;
					ptCenter[0].z = (ptEDGE[0].z + ptEDGE[1].z)*0.5;
					ptCenter[2].x = (ptEDGE[2].x + ptEDGE[3].x)*0.5;
					ptCenter[2].y = (ptEDGE[2].y + ptEDGE[3].y)*0.5;
					ptCenter[2].z = (ptEDGE[2].z + ptEDGE[3].z)*0.5;
					ptCenter[1].x = (ptCenter[0].x + ptCenter[2].x)*0.5;
					ptCenter[1].y = (ptCenter[0].y + ptCenter[2].y)*0.5;
					ptCenter[1].z = (ptCenter[0].z + ptCenter[2].z)*0.5;
					pSide[i]->pCenterline = CHSRFace::CreateLine(3,ptCenter);

					if((SINGLE_LINE == mode) || ((BOTH == mode) && pSide[i]->radius <= nCriticalRadius))
					{
						if(0 == i)
						{
							pSide[i]->mark[0] = true;
							pSide[i]->mark[1] = true;
						}
						else
						{
							pSide[i]->mark[1] = true;
						}
					}
					else
					{
						//--> 2002-11-18
						if(pSide[0] && pSide[1])
						{
							pSide[0]->SplitFaceLine(pSide[1],true);
							pSide[1]->SplitFaceLine(pSide[0],true);
							for(PHSR_VERTEX ptr = pSide[1]->pHead;ptr && ptr->next;ptr = ptr->next)
								if(pSide[0]->IsHiddenLine(ptr,ptr->next))
									ptr->render = INVALID;
							for(PHSR_VERTEX ptr = pSide[0]->pHead;ptr && ptr->next;ptr = ptr->next)
								if(pSide[1]->IsHiddenLine(ptr,ptr->next))
									ptr->render = INVALID;
						}
					}
				}
				/*
				if(pTop[i]=CHSRFace::CreateCircleFace(pt[0],nRadius[0],vecAxis,m_nSegments))
				{
					pTop[i]->type    = TEE;
					pTop[i]->id      = nID++;
					pTop[i]->ele     = HSR_CIRCLE_SHAPE;
					pTop[i]->ptCenter= pt[0];
					pTop[i]->radius  = nRadius[0];
					
					if((1 == i) && bSingleLine)
					{
						pTop[i]->ele = SYMBOL;
					}
				}

				if(pBottom[i]=CHSRFace::CreateCircleFace(pt[1],nRadius[0],vecAxis,m_nSegments))
				{
					pBottom[i]->type    = TEE;
					pBottom[i]->id      = nID++;
					pBottom[i]->ele     = HSR_CIRCLE_SHAPE;
					pBottom[i]->ptCenter= pt[1];
					pBottom[i]->radius  = nRadius[0];

					if((1 == i) && bSingleLine)
					{
						pBottom[i]->ele = SYMBOL;
					}
				}
				*/
			}
		}
		
		for(int i = 0;i < m_nPIPEs;i++)
		{
			pHSRApp->m_clsPreProcessor.Run(NULL,TEE,pTop[i],pSide[i],pBottom[i]);
		}
	}

	return pRet;
}

/**	
	@brief	The CTeeElement::CreateSWFace function
	@author	humkyung
	@param	pVIEWFormat	a parameter of type CPreViewFormat*
	@param	nID	a parameter of type long&
	@return	PFACE	
*/
CHSRFace* CTeeElement::CreateSWFace(CHSRViewFormat* pVIEWFormat,long& nID)
{
	assert(pVIEWFormat && "pVIEWFormat is NULL");
	CHSRFace* pRet=NULL;

	CHSRApp* pHSRApp=CHSRApp::GetInstance();
	if(pVIEWFormat && pHSRApp){
		static CHSRFace* pTop[MAX_TEE_PARTS] ={NULL,};
		static CHSRFace* pSide[MAX_TEE_PARTS] ={NULL,};
		static CHSRFace* pBottom[MAX_TEE_PARTS]={NULL,};
		static double nRadius[MAX_TEE_PARTS][2]={0.,};
		static POINT_T ptEDGE[36];

		PIPELINEMODE mode=pHSRApp->GetEnv()->GetPipeLineMode();
		double nCriticalRadius = pHSRApp->GetEnv()->GetCriticalRadius();

		POINT_T pt[2]={0.,};
		memset(pTop,0,sizeof(PFACE)*MAX_TEE_PARTS);
		memset(pSide,0,sizeof(PFACE)*MAX_TEE_PARTS);
		memset(pBottom,0,sizeof(PFACE)*MAX_TEE_PARTS);
		memset(nRadius,0,sizeof(double)*MAX_TEE_PARTS);
		memset(ptEDGE,0,sizeof(POINT_T)*36);
		for(int i=0;i < m_nPIPEs;i++){
			pTop[i] = pSide[i] = pBottom[i] = NULL;
			
			pt[0] = pVIEWFormat->MODEL2VIEW(m_pt[i][0]);
			pt[1] = pVIEWFormat->MODEL2VIEW(m_pt[i][1]);
			nRadius[i][0]= pVIEWFormat->MODEL2VIEW(m_nRadius[i][0]);
			nRadius[i][1]= pVIEWFormat->MODEL2VIEW(m_nRadius[i][1]);
			bool bSingleLine=((SINGLE_LINE == mode) || ((BOTH == mode) && nRadius[0][0] <= nCriticalRadius)) ? true : false;
			
			VECTOR_T vecAxis={0.},vecZ={0.,0.,1.};
			vecAxis.dx=pt[1].x - pt[0].x;
			vecAxis.dy=pt[1].y - pt[0].y;
			vecAxis.dz=pt[1].z - pt[0].z;
			if(!CMath::NormalizeVector(vecAxis)) return NULL;
			
			double nDot=CMath::DotProduct(vecAxis,vecZ);
			/// VERTICAL.
			if(fabs(fabs(nDot) - 1) < 0.001){
				POINT_T ptOrigin=(pt[0].z > pt[1].z) ? pt[0] : pt[1];
				if((1 == i) && bSingleLine){
					if(pt[0].z > pt[1].z){
						// 2'nd pipe's top face is intersection point of tee.
						if(pTop[i] = pHSRApp->new_hsr_face()){
							POINT_T ptLine[2]={0.,};
							ptLine[0] = pVIEWFormat->MODEL2VIEW(m_pt[0][0]);
							ptLine[1] = pVIEWFormat->MODEL2VIEW(m_pt[0][1]);
							
							LINE_T line={0,};
							line.ptStart = ptLine[0];
							line.ptEnd   = ptLine[1];
							PHSR_VERTEX pSymbol=CreateTeeSymbol(line,nRadius[i][0]);
							if(pSymbol){
								int nCount=0;
								for(PHSR_VERTEX ptr=pSymbol;ptr;ptr = ptr->next,nCount++){
									ptr->pt->x+= ptOrigin.x;
									ptr->pt->y+= ptOrigin.y;
									ptr->pt->z = ptOrigin.z;
								}
								pTop[i]->ele     = SYMBOL;
								pTop[i]->pHead   = pSymbol;
								pTop[i]->nCount  = nCount;
								pTop[i]->CalcPlaneEquation();
								pTop[i]->id      = nID++;
								pTop[i]->type    = TEEX;
								pTop[i]->radius  = nRadius[0][0];
								pTop[i]->ptCenter= ptOrigin;
							}else{
								pHSRApp->delete_hsr_face(pTop[i]);
								pTop[i] = NULL;
							}
						}
					}else{ /// downward
						if(pTop[i]=CHSRFace::CreateCircleFace(ptOrigin,nRadius[i][0],vecAxis,m_nSegments)){
							pTop[i]->id  = nID++;
							pTop[i]->type= TEEX;
							pTop[i]->ele = SYMBOL;
							pTop[i]->ptCenter = ptOrigin;
							pTop[i]->radius = nRadius[0][0];
							pTop[i]->SetColor(*m_pstrColor);
						}
					}
				}else{
					if(!bSingleLine){
						if(pTop[i]=CHSRFace::CreateCircleFace(ptOrigin,nRadius[i][0],vecAxis,m_nSegments)){
							pTop[i]->id  = nID++;
							pTop[i]->type= TEEX;
							pTop[i]->ele = (pTop[i]->IsPerpendicularToViewPoint()) ? SECTION : HSR_CIRCLE_SHAPE;
							pTop[i]->ptCenter = ptOrigin;
							pTop[i]->radius   = nRadius[0][0];
							pTop[i]->SetColor(*m_pstrColor);
						}
					}
				}
			/// HORIZONTAL.
			}else if(fabs(nDot) < 0.001){
				VECTOR_T vecCross={0};
				CMath::GetCrossProduct(vecCross,vecAxis,vecZ);
				if(!CMath::NormalizeVector(vecCross)) return NULL;
				
				ptEDGE[0].x = pt[0].x + vecCross.dx*nRadius[i][0];
				ptEDGE[0].y = pt[0].y + vecCross.dy*nRadius[i][0];
				ptEDGE[0].z = (pt[0].z + nRadius[i][0]) + vecCross.dz*nRadius[i][0];
				ptEDGE[1].x = pt[0].x - vecCross.dx*nRadius[i][0];
				ptEDGE[1].y = pt[0].y - vecCross.dy*nRadius[i][0];
				ptEDGE[1].z = (pt[0].z + nRadius[i][0]) - vecCross.dz*nRadius[i][0];
				ptEDGE[2].x = pt[1].x - vecCross.dx*nRadius[i][1];
				ptEDGE[2].y = pt[1].y - vecCross.dy*nRadius[i][1];
				ptEDGE[2].z = (pt[1].z + nRadius[i][1]) - vecCross.dz*nRadius[i][1];
				ptEDGE[3].x = pt[1].x + vecCross.dx*nRadius[i][1];
				ptEDGE[3].y = pt[1].y + vecCross.dy*nRadius[i][1];
				ptEDGE[3].z = (pt[1].z + nRadius[i][1]) + vecCross.dz*nRadius[i][1];
				ptEDGE[4] = ptEDGE[0];
				
				if(pSide[i]=CHSRFace::CreateFace(5,ptEDGE)){
					pSide[i]->id    = nID++;
					pSide[i]->type  = TEEX;
					pSide[i]->ele   = HSR_RECTANGLE_SHAPE;
					pSide[i]->radius= nRadius[0][0];
					pSide[i]->SetColor(*m_pstrColor);

					POINT_T ptCenter[3]={0.,};
					ptCenter[0].x = (ptEDGE[0].x + ptEDGE[1].x)*0.5;
					ptCenter[0].y = (ptEDGE[0].y + ptEDGE[1].y)*0.5;
					ptCenter[0].z = (ptEDGE[0].z + ptEDGE[1].z)*0.5;
					ptCenter[2].x = (ptEDGE[2].x + ptEDGE[3].x)*0.5;
					ptCenter[2].y = (ptEDGE[2].y + ptEDGE[3].y)*0.5;
					ptCenter[2].z = (ptEDGE[2].z + ptEDGE[3].z)*0.5;
					ptCenter[1].x = (ptCenter[0].x + ptCenter[2].x)*0.5;
					ptCenter[1].y = (ptCenter[0].y + ptCenter[2].y)*0.5;
					ptCenter[1].z = (ptCenter[0].z + ptCenter[2].z)*0.5;
					pSide[i]->pCenterline = CHSRFace::CreateLine(3,ptCenter);

					/// 2004-02-23
					if(pSide[0] && pSide[1]){
						pSide[0]->SplitFaceLine(pSide[1],true);
						pSide[1]->SplitFaceLine(pSide[0],true);
						for(PHSR_VERTEX ptr = pSide[1]->pHead;ptr && ptr->next;ptr = ptr->next)
							if(pSide[0]->IsHiddenLine(ptr,ptr->next))
								ptr->render = INVALID;
							for(PHSR_VERTEX ptr = pSide[0]->pHead;ptr && ptr->next;ptr = ptr->next)
								if(pSide[1]->IsHiddenLine(ptr,ptr->next))
									ptr->render = INVALID;
					}
				}
			/// SLOPE.
			}
			else
			{
				VECTOR_T vecCross={0};
				CMath::GetCrossProduct(vecCross,vecAxis,vecZ);
				if(!CMath::NormalizeVector(vecCross)) return NULL;
				
				ptEDGE[0].x = pt[0].x + vecCross.dx*nRadius[i][0];
				ptEDGE[0].y = pt[0].y + vecCross.dy*nRadius[i][0];
				ptEDGE[0].z = (pt[0].z + nRadius[i][0]) + vecCross.dz*nRadius[i][0];
				ptEDGE[1].x = pt[0].x - vecCross.dx*nRadius[i][0];
				ptEDGE[1].y = pt[0].y - vecCross.dy*nRadius[i][0];
				ptEDGE[1].z = (pt[0].z + nRadius[i][0]) - vecCross.dz*nRadius[i][0];
				ptEDGE[2].x = pt[1].x - vecCross.dx*nRadius[i][1];
				ptEDGE[2].y = pt[1].y - vecCross.dy*nRadius[i][1];
				ptEDGE[2].z = (pt[1].z + nRadius[i][1]) - vecCross.dz*nRadius[i][1];
				ptEDGE[3].x = pt[1].x + vecCross.dx*nRadius[i][1];
				ptEDGE[3].y = pt[1].y + vecCross.dy*nRadius[i][1];
				ptEDGE[3].z = (pt[1].z + nRadius[i][1]) + vecCross.dz*nRadius[i][1];
				ptEDGE[4] = ptEDGE[0];
				
				if(pSide[i]=CHSRFace::CreateFace(5,ptEDGE)){
					pSide[i]->id    = nID++;
					pSide[i]->type  = TEEX;
					pSide[i]->ele   = HSR_RECTANGLE_SHAPE;
					pSide[i]->radius= nRadius[0][0];
					pSide[i]->SetColor(*m_pstrColor);

					POINT_T ptCenter[3]={0.,};
					ptCenter[0].x = (ptEDGE[0].x + ptEDGE[1].x)*0.5;
					ptCenter[0].y = (ptEDGE[0].y + ptEDGE[1].y)*0.5;
					ptCenter[0].z = (ptEDGE[0].z + ptEDGE[1].z)*0.5;
					ptCenter[2].x = (ptEDGE[2].x + ptEDGE[3].x)*0.5;
					ptCenter[2].y = (ptEDGE[2].y + ptEDGE[3].y)*0.5;
					ptCenter[2].z = (ptEDGE[2].z + ptEDGE[3].z)*0.5;
					ptCenter[1].x = (ptCenter[0].x + ptCenter[2].x)*0.5;
					ptCenter[1].y = (ptCenter[0].y + ptCenter[2].y)*0.5;
					ptCenter[1].z = (ptCenter[0].z + ptCenter[2].z)*0.5;
					pSide[i]->pCenterline = CHSRFace::CreateLine(3,ptCenter);

					/// 2004-02-23
					if(pSide[0] && pSide[1]){
						pSide[0]->SplitFaceLine(pSide[1],true);
						pSide[1]->SplitFaceLine(pSide[0],true);
						for(PHSR_VERTEX ptr = pSide[1]->pHead;ptr && ptr->next;ptr = ptr->next)
							if(pSide[0]->IsHiddenLine(ptr,ptr->next))
								ptr->render = INVALID;
							for(PHSR_VERTEX ptr = pSide[0]->pHead;ptr && ptr->next;ptr = ptr->next)
								if(pSide[1]->IsHiddenLine(ptr,ptr->next))
									ptr->render = INVALID;
					}
				}

				if(pTop[i]=CHSRFace::CreateCircleFace(pt[0],nRadius[i][0],vecAxis,m_nSegments)){
					pTop[i]->type    = TEEX;
					pTop[i]->id      = nID++;
					pTop[i]->ele     = HSR_CIRCLE_SHAPE;
					pTop[i]->ptCenter= pt[0];
					pTop[i]->radius  = nRadius[0][0];
					pTop[i]->SetColor(*m_pstrColor);

					if((1 == i) && bSingleLine){
						pTop[i]->ele = SYMBOL;
					}
				}

				if(pBottom[i]=CHSRFace::CreateCircleFace(pt[1],nRadius[i][0],vecAxis,m_nSegments)){
					pBottom[i]->type    = TEEX;
					pBottom[i]->id      = nID++;
					pBottom[i]->ele     = HSR_CIRCLE_SHAPE;
					pBottom[i]->ptCenter= pt[1];
					pBottom[i]->radius  = nRadius[0][0];
					pBottom[i]->SetColor(*m_pstrColor);

					if((1 == i) && bSingleLine){
						pBottom[i]->ele = SYMBOL;
					}
				}
			}
		}

		for(int i = 2;i < m_nPIPEs;i++)
		{
			if(pSide[i] && pSide[i]->pCenterline && pSide[i]->pCenterline->next)
			{
				pSide[i]->mark[0] = true;
				pSide[i]->mark[1] = true;
				for(int j=0;j < 2;j++)
				{
					if((j != i) && pSide[j] && pSide[j]->pCenterline && pSide[j]->pCenterline->next){
						double dx=pSide[j]->pCenterline->pt->x - pSide[i]->pCenterline->pt->x;
						double dy=pSide[j]->pCenterline->pt->y - pSide[i]->pCenterline->pt->y;
						double dz=pSide[j]->pCenterline->pt->z - pSide[i]->pCenterline->pt->z;
						if((fabs(dx) < 0.01) && (fabs(dy) < 0.01) && (fabs(dz) < 0.01)) pSide[i]->mark[0] = false;
						
						dx=pSide[j]->pCenterline->next->pt->x - pSide[i]->pCenterline->pt->x;
						dy=pSide[j]->pCenterline->next->pt->y - pSide[i]->pCenterline->pt->y;
						dz=pSide[j]->pCenterline->next->pt->z - pSide[i]->pCenterline->pt->z;
						if((fabs(dx) < 0.01) && (fabs(dy) < 0.01) && (fabs(dz) < 0.01)) pSide[i]->mark[0] = false;

						dx=pSide[j]->pCenterline->pt->x - pSide[i]->pCenterline->next->pt->x;
						dy=pSide[j]->pCenterline->pt->y - pSide[i]->pCenterline->next->pt->y;
						dz=pSide[j]->pCenterline->pt->z - pSide[i]->pCenterline->next->pt->z;
						if((fabs(dx) < 0.01) && (fabs(dy) < 0.01) && (fabs(dz) < 0.01)) pSide[i]->mark[1] = false;

						dx=pSide[j]->pCenterline->next->pt->x - pSide[i]->pCenterline->next->pt->x;
						dy=pSide[j]->pCenterline->next->pt->y - pSide[i]->pCenterline->next->pt->y;
						dz=pSide[j]->pCenterline->next->pt->z - pSide[i]->pCenterline->next->pt->z;
						if((fabs(dx) < 0.01) && (fabs(dy) < 0.01) && (fabs(dz) < 0.01)) pSide[i]->mark[1] = false;
					}
				}
			}
		}

		/// append TEEX faces
		for(int i = 0;i < m_nPIPEs;i++)
			pHSRApp->m_clsPreProcessor.Run(NULL,TEEX,pTop[i],pSide[i],pBottom[i]);
	}

	return pRet;
}

#define	TEE_EQUAL(a,b) (fabs((a) - (b)) < 0.01)

/**	\brief	The CTeeElement::Adjust function


	\remarks\n
	;2003.03.13 - modified

	\return	bool	
*/
bool CTeeElement::Adjust(){
	bool bRet=false;

	if(2 == m_nPIPEs){
		POINT_T ptMid[2]={0.,};
		ptMid[0].x = (m_pt[0][0].x + m_pt[0][1].x)*0.5;
		ptMid[0].y = (m_pt[0][0].y + m_pt[0][1].y)*0.5;
		ptMid[0].z = (m_pt[0][0].z + m_pt[0][1].z)*0.5;
		ptMid[1].x = (m_pt[1][0].x + m_pt[1][1].x)*0.5;
		ptMid[1].y = (m_pt[1][0].y + m_pt[1][1].y)*0.5;
		ptMid[1].z = (m_pt[1][0].z + m_pt[1][1].z)*0.5;
		
		double len[2]={0,};
		double dx=0.,dy=0.,dz=0.;
		dx = m_pt[1][0].x - ptMid[0].x;
		dy = m_pt[1][0].y - ptMid[0].y;
		dz = m_pt[1][0].z - ptMid[0].z;
		len[0] = dx*dx + dy*dy + dz*dz;
		dx = m_pt[1][1].x - ptMid[0].x;
		dy = m_pt[1][1].y - ptMid[0].y;
		dz = m_pt[1][1].z - ptMid[0].z;
		len[0] = (len[0] > (dx*dx + dy*dy + dz*dz)) ? dx*dx + dy*dy + dz*dz : len[0];
		
		dx = m_pt[0][0].x - ptMid[1].x;
		dy = m_pt[0][0].y - ptMid[1].y;
		dz = m_pt[0][0].z - ptMid[1].z;
		len[1] = dx*dx + dy*dy + dz*dz;
		dx = m_pt[0][1].x - ptMid[1].x;
		dy = m_pt[0][1].y - ptMid[1].y;
		dz = m_pt[0][1].z - ptMid[1].z;
		len[1] = (len[1] > (dx*dx + dy*dy + dz*dz)) ? dx*dx + dy*dy + dz*dz : len[1];
		
		if(len[0] > len[1]){
			POINT_T tmp=m_pt[1][1];
			m_pt[1][1] = m_pt[0][1];	
			m_pt[0][1] = tmp;
			tmp=m_pt[1][0];
			m_pt[1][0] = m_pt[0][0];
			m_pt[0][0] = tmp;
			
			double nRadius= m_nRadius[0][0];
			m_nRadius[0][0]  = m_nRadius[1][0];
			m_nRadius[1][0]  = nRadius;
			nRadius= m_nRadius[0][1];
			m_nRadius[0][1]  = m_nRadius[1][1];
			m_nRadius[1][1]  = nRadius;
		}
		
		ptMid[0].x = (m_pt[0][0].x + m_pt[0][1].x)*0.5;
		ptMid[0].y = (m_pt[0][0].y + m_pt[0][1].y)*0.5;
		ptMid[0].z = (m_pt[0][0].z + m_pt[0][1].z)*0.5;
		
		dx = m_pt[1][0].x - ptMid[0].x;
		dy = m_pt[1][0].y - ptMid[0].y;
		dz = m_pt[1][0].z - ptMid[0].z;
		len[0] = dx*dx + dy*dy + dz*dz;
		dx = m_pt[1][1].x - ptMid[0].x;
		dy = m_pt[1][1].y - ptMid[0].y;
		dz = m_pt[1][1].z - ptMid[0].z;
		len[1] = dx*dx + dy*dy + dz*dz;
		
		if(len[1] < len[0]){
			POINT_T tmp=m_pt[1][1];
			m_pt[1][1] = m_pt[1][0];	
			m_pt[1][0] = tmp;
		}

		ptMid[0].x = (m_pt[0][0].x + m_pt[0][1].x)*0.5;
		ptMid[0].y = (m_pt[0][0].y + m_pt[0][1].y)*0.5;
		ptMid[0].z = (m_pt[0][0].z + m_pt[0][1].z)*0.5;
		//if(	(TEE_EQUAL(ptMid[0].x,m_pt[1][0].x) && TEE_EQUAL(ptMid[0].y,m_pt[1][0].y) && TEE_EQUAL(ptMid[0].z,m_pt[1][0].z)) || 
		//	(TEE_EQUAL(ptMid[0].x,m_pt[1][1].x) && TEE_EQUAL(ptMid[0].y,m_pt[1][1].y) && TEE_EQUAL(ptMid[0].z,m_pt[1][1].z))){
		//	bRet = true;
		//	}
		bRet = true;
	}else if(5 == m_nPIPEs){
		POINT_T ptMid={0.};
		for(int i = 0;i < m_nPIPEs;i++){
			ptMid.x = (m_pt[i][0].x + m_pt[i][1].x)*0.5;
			ptMid.y = (m_pt[i][0].y + m_pt[i][1].y)*0.5;
			ptMid.z = (m_pt[i][0].z + m_pt[i][1].z)*0.5;
			for(int j = 0;j < m_nPIPEs;j++){
				if(i != j){
					if(TEE_EQUAL(ptMid.x,m_pt[j][0].x) && TEE_EQUAL(ptMid.y,m_pt[j][0].y) && TEE_EQUAL(ptMid.z,m_pt[j][0].z)){
						POINT_T tmp[5][2]={0,};
						double  nRadius[5][2]={0.,};

						tmp[0][0] = m_pt[0][0];tmp[0][1] = m_pt[0][1];
						nRadius[0][0] = m_nRadius[0][0];nRadius[0][1] = m_nRadius[0][1];
						tmp[1][0] = m_pt[1][0];tmp[1][1] = m_pt[1][1];
						nRadius[1][0] = m_nRadius[1][0];nRadius[1][1] = m_nRadius[1][1];
						tmp[2][0] = m_pt[2][0];tmp[2][1] = m_pt[2][1];
						nRadius[2][0] = m_nRadius[2][0];nRadius[2][1] = m_nRadius[2][1];
						tmp[3][0] = m_pt[3][0];tmp[3][1] = m_pt[3][1];
						nRadius[3][0] = m_nRadius[3][0];nRadius[3][1] = m_nRadius[3][1];
						tmp[4][0] = m_pt[4][0];tmp[4][1] = m_pt[4][1];
						nRadius[4][0] = m_nRadius[4][0];nRadius[4][1] = m_nRadius[4][1];
						
						m_pt[0][0] = tmp[i][0];m_pt[0][1] = tmp[i][1];
						m_nRadius[0][0] = nRadius[i][0];m_nRadius[0][1] = nRadius[i][1];
						m_pt[1][0] = tmp[j][0];m_pt[1][1] = tmp[j][1];
						m_nRadius[1][0] = nRadius[j][0];m_nRadius[1][1] = nRadius[j][1];

						int n=2;
						for(int k=0;k < m_nPIPEs;k++){
							if((k != i) && (k != j)){
								m_pt[n][0]  = tmp[k][0];
								m_pt[n][1]  = tmp[k][1];
								m_nRadius[n][0] = nRadius[k][0];
								m_nRadius[n][1] = nRadius[k][1];
								n++;
							}
						}
						
						return true;
					}else if(TEE_EQUAL(ptMid.x,m_pt[j][1].x) && TEE_EQUAL(ptMid.y,m_pt[j][1].y) && TEE_EQUAL(ptMid.z,m_pt[j][1].z)){
						POINT_T tmp[5][2]={0,};
						double nRadius[5][2]={0.,};

						tmp[0][0] = m_pt[j][0];
						m_pt[j][0] = m_pt[j][1];
						m_pt[j][1] = tmp[0][0];

						tmp[0][0] = m_pt[0][0];tmp[0][1] = m_pt[0][1];
						nRadius[0][0] = m_nRadius[0][0];nRadius[0][1] = m_nRadius[0][1];
						tmp[1][0] = m_pt[1][0];tmp[1][1] = m_pt[1][1];
						nRadius[1][0] = m_nRadius[1][0];nRadius[1][1] = m_nRadius[1][1];
						tmp[2][0] = m_pt[2][0];tmp[2][1] = m_pt[2][1];
						nRadius[2][0] = m_nRadius[2][0];nRadius[2][1] = m_nRadius[2][1];
						tmp[3][0] = m_pt[3][0];tmp[3][1] = m_pt[3][1];
						nRadius[3][0] = m_nRadius[3][0];nRadius[3][1] = m_nRadius[3][1];
						tmp[4][0] = m_pt[4][0];tmp[4][1] = m_pt[4][1];
						nRadius[4][0] = m_nRadius[4][0];nRadius[4][1] = m_nRadius[4][1];
						
						m_pt[0][0] = tmp[i][0];m_pt[0][1] = tmp[i][1];
						m_nRadius[0][0] = nRadius[i][0];m_nRadius[0][1] = nRadius[i][1];
						m_pt[1][0] = tmp[j][0];m_pt[1][1] = tmp[j][1];
						m_nRadius[1][0] = nRadius[j][0];m_nRadius[1][1] = nRadius[j][1];

						int n=2;
						for(int k=0;k < m_nPIPEs;k++){
							if((k != i) && (k != j)){
								m_pt[n][0]  = tmp[k][0];
								m_pt[n][1]  = tmp[k][1];
								m_nRadius[n][0]= nRadius[k][0];
								m_nRadius[n][1]= nRadius[k][1];
								n++;
							}
						}

						return true;
					}
				}
			}
		}
		bRet = true;
	}

	return bRet;
}

#ifdef VER_03
/**	\brief	The CTeeElement::ParseBWLine function

	\param	pPreFormatScanner	a parameter of type CHSRScanner*

	\return	bool	
*/
bool CTeeElement::ParseBWLine(CHSRScanner* pPreFormatScanner){
	assert(pPreFormatScanner && "pPreFormatScanner is NULL");
	bool bRet=false;

	if(pPreFormatScanner){
		char* pVal=NULL;

		if(pVal = pPreFormatScanner->Val("layer")) m_strLayer = pVal;
		if(pVal = pPreFormatScanner->Val("top radius1"))
			m_nRadius[0][0] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("top pt1")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[0][0].x),&(m_pt[0][0].y),&(m_pt[0][0].z));
		}
		if(pVal = pPreFormatScanner->Val("bottom radius1"))
			m_nRadius[0][1] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("bottom pt1")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[0][1].x),&(m_pt[0][1].y),&(m_pt[0][1].z));
		}
		
		if(pVal = pPreFormatScanner->Val("top radius2"))
			m_nRadius[1][0] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("top pt2")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[1][0].x),&(m_pt[1][0].y),&(m_pt[1][0].z));
		}
		if(pVal = pPreFormatScanner->Val("bottom radius2"))
			m_nRadius[1][1] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("bottom pt2")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[1][1].x),&(m_pt[1][1].y),&(m_pt[1][1].z));
		}
		m_bAdjusted = Adjust();

		bRet = true;
	}

	return bRet;
}

/**	\brief	The CTeeElement::ParseSWLine function

	\param	pPreFormatScanner	a parameter of type CHSRScanner*

	\return	bool	
*/
bool CTeeElement::ParseSWLine(CHSRScanner* pPreFormatScanner){
	assert(pPreFormatScanner && "pPreFormatScanner is NULL");
	bool bRet=false;

	if(pPreFormatScanner){
		int nIndex=1;
		char* pVal=NULL;

		if(pVal = pPreFormatScanner->Val("layer")) m_strLayer = pVal;
		
		if(pVal = pPreFormatScanner->Val("top radius1"))
			m_nRadius[0][0] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("top pt1")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[0][0].x),&(m_pt[0][0].y),&(m_pt[0][0].z));
		}
		if(pVal = pPreFormatScanner->Val("bottom radius1"))
			m_nRadius[0][1] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("bottom pt1")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[0][1].x),&(m_pt[0][1].y),&(m_pt[0][1].z));
		}
		
		if(pVal = pPreFormatScanner->Val("top radius2"))
			m_nRadius[1][0] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("top pt2")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[1][0].x),&(m_pt[1][0].y),&(m_pt[1][0].z));
		}
		if(pVal = pPreFormatScanner->Val("bottom radius2"))
			m_nRadius[1][1] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("bottom pt2")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[1][1].x),&(m_pt[1][1].y),&(m_pt[1][1].z));
		}

		if(pVal = pPreFormatScanner->Val("top radius3"))
			m_nRadius[2][0] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("top pt3")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[2][0].x),&(m_pt[2][0].y),&(m_pt[2][0].z));
		}
		if(pVal = pPreFormatScanner->Val("bottom radius3"))
			m_nRadius[2][1] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("bottom pt3")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[2][1].x),&(m_pt[2][1].y),&(m_pt[2][1].z));
		}

		if(pVal = pPreFormatScanner->Val("top radius4"))
			m_nRadius[3][0] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("top pt4")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[3][0].x),&(m_pt[3][0].y),&(m_pt[3][0].z));
		}
		if(pVal = pPreFormatScanner->Val("bottom radius4"))
			m_nRadius[3][1] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("bottom pt4")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[3][1].x),&(m_pt[3][1].y),&(m_pt[3][1].z));
		}
		
		if(pVal = pPreFormatScanner->Val("top radius5"))
			m_nRadius[4][0] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("top pt5")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[4][0].x),&(m_pt[4][0].y),&(m_pt[4][0].z));
		}
		if(pVal = pPreFormatScanner->Val("bottom radius5"))
			m_nRadius[4][1] = ATOF_T(pVal);
		if(pVal = pPreFormatScanner->Val("bottom pt5")){
			sscanf(pVal,"%lf,%lf,%lf",&(m_pt[4][1].x),&(m_pt[4][1].y),&(m_pt[4][1].z));
		}

		m_bAdjusted = Adjust();

		bRet = true;
	}

	return bRet;
}
#else
/**	
	@brief	The CTeeElement::ParseBWLine function
	@author	humkyung
	@param	pPreFormatScanner	a parameter of type CHSRScanner*
	@return	bool	
*/
bool CTeeElement::ParseBWLine(CHSRScanner* pScanner)
{
	assert(pScanner && "pScanner is NULL");
	bool bRet=false;
	
	if(pScanner)
	{
		int nIndex=1;
		for(int i=0;i < m_nPIPEs;i++)
		{
			m_nRadius[i][0] = m_nRadius[i][1] = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
			
			m_pt[i][0].x = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
			m_pt[i][0].y = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
			m_pt[i][0].z = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
			m_pt[i][1].x = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
			m_pt[i][1].y = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
			m_pt[i][1].z = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
		}
		if(pScanner->m_nFactor > nIndex)
		{
			SetColorString((*pScanner->m_aryFactor)[nIndex]);
		}

		m_bAdjusted = Adjust();

		bRet = true;
	}

	return bRet;
}

/**	
	@brief	The CTeeElement::ParseSWLine function
	@author	humkyung
	@param	pPreFormatScanner	a parameter of type CHSRScanner*
	@return	bool	
*/
bool CTeeElement::ParseSWLine(CHSRScanner* pScanner)
{
	assert(pScanner && "pScanner is NULL");
	bool bRet=false;

	if(pScanner)
	{
		int nIndex=1;
		for(int i=0;i < m_nPIPEs;++i)
		{
			m_nRadius[i][0] = m_nRadius[i][1] = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
			
			m_pt[i][0].x = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
			m_pt[i][0].y = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
			m_pt[i][0].z = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
			m_pt[i][1].x = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
			m_pt[i][1].y = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
			m_pt[i][1].z = ATOF_T((*pScanner->m_aryFactor)[nIndex++].c_str());
		}
		if(pScanner->m_nFactor > nIndex)
		{
			SetColorString((*pScanner->m_aryFactor)[nIndex]);
		}

		m_bAdjusted = Adjust();

		bRet = true;
	}

	return bRet;
}
#endif

/**	\brief	The CTeeElement::CreateTeeSymbol function\n
	create symbol of tee which is vertical.

	\param	line	a parameter of type const LINE_T &
	\param	nRadius	a parameter of type const double

	\remarks\n
	;2002.05.23 - check routine which tee input distance is larger than radius.\n
	- if tee input distance is larger than radius, no create tee symbol\n
	- change 'nDist*0.5/(2*radius)' -> 'nDist*0.5/radius'
	;2007.03.23 - nDist is nRadius when nRadius * 2 < nDist

	\return	PVERTEX	
*/
PHSR_VERTEX CTeeElement::CreateTeeSymbol(const LINE_T &line, const double nRadius){
	assert((nRadius > 0) && "nRadius is less than 0");
	static POINT_T ptCurve[24] ={0,};
	static bool    bVisible[24]={0,};
	CHSRApp* pHSRApp=CHSRApp::GetInstance();
	double nDist =pHSRApp->GetEnv()->GetTeeDistA();
	if(nRadius * 2 < nDist) nDist = nRadius;

	assert((nDist > 0) && "nDist is less than 0");
	PHSR_VERTEX pVertices = NULL;
	if((nRadius > 0) && (nDist > 0)){
		POINT_T pt={0};

		pt.x = line.ptEnd.x - line.ptStart.x;
		pt.y = line.ptEnd.y - line.ptStart.y;
		double alpha=CGeometry::GetRotatedAngleInXYPlane(pt);
		double c=cos(alpha),s=sin(alpha);

		double sval=(nDist*0.5)/nRadius;
		if(sval <= 1.){
			int nIndex=0;
			double nTheta=asin(sval);
			double nStep=PI/8.;
			for(double nRad=0.;nRad < nTheta;nRad += nStep,nIndex++){
				ptCurve[nIndex].x = nRadius*cos(nRad);
				ptCurve[nIndex].y = nRadius*sin(nRad);

				bVisible[nIndex] = false;

				double x=ptCurve[nIndex].x*c - ptCurve[nIndex].y*s;
				double y=ptCurve[nIndex].x*s + ptCurve[nIndex].y*c;
				ptCurve[nIndex].x = x;
				ptCurve[nIndex].y = y;
			}
			ptCurve[nIndex].x = nRadius*cos(nTheta);
			ptCurve[nIndex].y = nRadius*sin(nTheta);
			bVisible[nIndex] = false;
			double x=ptCurve[nIndex].x*c - ptCurve[nIndex].y*s;
			double y=ptCurve[nIndex].x*s + ptCurve[nIndex].y*c;
			ptCurve[nIndex].x = x;
			ptCurve[nIndex].y = y;
			nIndex++;
			for(double nRad=nTheta;nRad < PI-nTheta;nRad+=nStep,nIndex++)
			{
				ptCurve[nIndex].x = nRadius*cos(nRad);
				ptCurve[nIndex].y = nRadius*sin(nRad);

				bVisible[nIndex] = true;

				double x=ptCurve[nIndex].x*c - ptCurve[nIndex].y*s;
				double y=ptCurve[nIndex].x*s + ptCurve[nIndex].y*c;
				ptCurve[nIndex].x = x;
				ptCurve[nIndex].y = y;
			}
			for(double nRad = PI-nTheta;nRad < PI+nTheta;nRad+=nStep,nIndex++)
			{
				ptCurve[nIndex].x = nRadius*cos(nRad);
				ptCurve[nIndex].y = nRadius*sin(nRad);

				bVisible[nIndex] = false;

				double x=ptCurve[nIndex].x*c - ptCurve[nIndex].y*s;
				double y=ptCurve[nIndex].x*s + ptCurve[nIndex].y*c;
				ptCurve[nIndex].x = x;
				ptCurve[nIndex].y = y;
			}
			ptCurve[nIndex].x = nRadius*cos(PI+nTheta);
			ptCurve[nIndex].y = nRadius*sin(PI+nTheta);
			bVisible[nIndex] = false;
			x=ptCurve[nIndex].x*c - ptCurve[nIndex].y*s;
			y=ptCurve[nIndex].x*s + ptCurve[nIndex].y*c;
			ptCurve[nIndex].x = x;
			ptCurve[nIndex].y = y;
			nIndex++;
			for(double nRad = PI+nTheta;nRad < 2*PI-nTheta;nRad+=nStep,nIndex++)
			{
				ptCurve[nIndex].x = nRadius*cos(nRad);
				ptCurve[nIndex].y = nRadius*sin(nRad);

				bVisible[nIndex] = true;

				double x=ptCurve[nIndex].x*c - ptCurve[nIndex].y*s;
				double y=ptCurve[nIndex].x*s + ptCurve[nIndex].y*c;
				ptCurve[nIndex].x = x;
				ptCurve[nIndex].y = y;
			}
			ptCurve[nIndex].x = nRadius*cos(2*PI-nTheta);
			ptCurve[nIndex].y = nRadius*sin(2*PI-nTheta);
			bVisible[nIndex]  = true;
			x=ptCurve[nIndex].x*c - ptCurve[nIndex].y*s;
			y=ptCurve[nIndex].x*s + ptCurve[nIndex].y*c;
			ptCurve[nIndex].x = x;
			ptCurve[nIndex].y = y;
			nIndex++;
			for(double nRad = 2*PI-nTheta;nRad < 2*PI;nRad+=nStep,nIndex++)
			{
				ptCurve[nIndex].x = nRadius*cos(nRad);
				ptCurve[nIndex].y = nRadius*sin(nRad);

				bVisible[nIndex] = false;

				double x=ptCurve[nIndex].x*c - ptCurve[nIndex].y*s;
				double y=ptCurve[nIndex].x*s + ptCurve[nIndex].y*c;
				ptCurve[nIndex].x = x;
				ptCurve[nIndex].y = y;
			}
			bVisible[nIndex]  = false;
			ptCurve[nIndex].x = ptCurve[0].x;
			ptCurve[nIndex].y = ptCurve[0].y;
			nIndex++;

			CHSRApp* pHSRApp = CHSRApp::GetInstance();
			for(int i = 0;i < nIndex;i++){
				PHSR_VERTEX pvt = pHSRApp->alloc_vertex();
				if(pvt){
					pvt->render = bVisible[i] ? SHOW : HIDE;
					PPOINT_T ppt = pHSRApp->alloc_point();
					if(ppt){
						ppt->x = ptCurve[i].x;
						ppt->y = ptCurve[i].y;
						ppt->z = 0.;
						pvt->pt= ppt;

						pVertices = ListPush(pVertices,pvt);
					}else	pHSRApp->free_vertex(pvt);
				}
			}
		}
	}
	
	return pVertices;
}