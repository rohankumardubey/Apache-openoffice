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



#ifndef _NBDTMGFACT_HXX
#include <svx/nbdtmgfact.hxx>
#endif
namespace svx { namespace sidebar {
NBOutlineTypeMgrFact::NBOutlineTypeMgrFact()
{
}

NBOTypeMgrBase* NBOutlineTypeMgrFact::CreateInstance(const NBOType aType)
{
	//NBOTypeMgrBase* pRet= 0;
	if ( aType == eNBOType::BULLETS )
	{
		return BulletsTypeMgr::GetInstance();
	}else if ( aType == eNBOType::GRAPHICBULLETS )
	{
		return GraphicBulletsTypeMgr::GetInstance();
	}else if ( aType == eNBOType::MIXBULLETS )
	{
		return MixBulletsTypeMgr::GetInstance();
	}else if ( aType == eNBOType::NUMBERING )
	{
		return NumberingTypeMgr::GetInstance();
	}else if ( aType == eNBOType::OUTLINE )
	{
		return OutlineTypeMgr::GetInstance();
	}

	return NULL;
}
}}

/* vim: set noet sw=4 ts=4: */
