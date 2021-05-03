#pragma once

#include <DgnLineString.h>
#include <DgnCellHeader.h>

#include <util/ado/ADODB.h>
#include <aDraw_View.h>
#include <aDraw_Pipe.h>
#include "aDraw_PipeParser.h"

class CaDraw_SupportParser : public CaDraw_PipeParser
{
public:
	CaDraw_SupportParser(const CaDraw_Entity::MODEL_TYPE_T& eModelType , const STRING_T& rModelPath , const double& uor_per_subunit);
	~CaDraw_SupportParser(void);
	int Parse(CaDraw_View* pDraView , CDgnCellHeader* pDgnCell, CADODB& adoDB , const int& nPartitionNo , const CIsPoint3d& ptModelOffset);
public:
	long m_occur[2];
};
