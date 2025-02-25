/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



#ifndef _SVDOUTL_HXX
#define _SVDOUTL_HXX

#include <editeng/outliner.hxx>
#include "svx/svxdllapi.h"
#include <svx/svdobj.hxx>

class SdrTextObj;
class SdrPaintInfoRec;
class SdrPage;

class SVX_DLLPUBLIC SdrOutliner : public Outliner
{
protected:
	SdrObjectWeakRef mpTextObj;
	const SdrPage* mpVisualizedPage;

public:
    SdrOutliner( SfxItemPool* pItemPool, sal_uInt16 nMode );
    virtual ~SdrOutliner() ;

	void  SetTextObj( const SdrTextObj* pObj );
	void SetTextObjNoInit( const SdrTextObj* pObj );
	const SdrTextObj* GetTextObj() const;

	void setVisualizedPage(const SdrPage* pPage) { if(pPage != mpVisualizedPage) mpVisualizedPage = pPage; }
	const SdrPage* getVisualizedPage() const { return mpVisualizedPage; }

	virtual String  CalcFieldValue(const SvxFieldItem& rField, sal_uInt32 nPara, sal_uInt16 nPos, Color*& rpTxtColor, Color*& rpFldColor);
};

#endif //_SVDOUTL_HXX

