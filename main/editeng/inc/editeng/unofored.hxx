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



#ifndef _SVX_UNOFORED_HXX
#define _SVX_UNOFORED_HXX

#include <editeng/editeng.hxx>
#include <editeng/unoedsrc.hxx>
#include "editeng/editengdllapi.h"

//	SvxEditEngineForwarder - SvxTextForwarder fuer EditEngine

class EDITENG_DLLPUBLIC SvxEditEngineForwarder : public SvxTextForwarder
{
private:
	EditEngine&			rEditEngine;

public:
						SvxEditEngineForwarder( EditEngine& rEngine );
	virtual				~SvxEditEngineForwarder();

	virtual sal_uInt32		GetParagraphCount() const;
	virtual sal_uInt16		GetTextLen( sal_uInt32 nParagraph ) const;
	virtual String		GetText( const ESelection& rSel ) const;
	virtual SfxItemSet	GetAttribs( const ESelection& rSel, sal_Bool bOnlyHardAttrib = EditEngineAttribs_All ) const;
	virtual	SfxItemSet	GetParaAttribs( sal_uInt32 nPara ) const;
	virtual void		SetParaAttribs( sal_uInt32 nPara, const SfxItemSet& rSet );
    virtual void        RemoveAttribs( const ESelection& rSelection, sal_Bool bRemoveParaAttribs, sal_uInt16 nWhich );
	virtual void		GetPortions( sal_uInt32 nPara, SvUShorts& rList ) const;
	
	virtual sal_uInt16		GetItemState( const ESelection& rSel, sal_uInt16 nWhich ) const;
	virtual sal_uInt16		GetItemState( sal_uInt32 nPara, sal_uInt16 nWhich ) const;

	virtual void		QuickInsertText( const String& rText, const ESelection& rSel );
	virtual void		QuickInsertField( const SvxFieldItem& rFld, const ESelection& rSel );
	virtual void		QuickSetAttribs( const SfxItemSet& rSet, const ESelection& rSel );
	virtual void		QuickInsertLineBreak( const ESelection& rSel );

	virtual SfxItemPool* GetPool() const;

	virtual XubString    CalcFieldValue( const SvxFieldItem& rField, sal_uInt32 nPara, sal_uInt16 nPos, Color*& rpTxtColor, Color*& rpFldColor );
	virtual void 	     FieldClicked( const SvxFieldItem& rField, sal_uInt32 nPara, xub_StrLen nPos );
	virtual sal_Bool		 IsValid() const;

    virtual LanguageType 	GetLanguage( sal_uInt32 nPara, sal_uInt16 nIndex ) const;
    virtual sal_uInt16			GetFieldCount( sal_uInt32 nPara ) const;
    virtual EFieldInfo		GetFieldInfo( sal_uInt32 nPara, sal_uInt16 nField ) const;
    virtual EBulletInfo     GetBulletInfo( sal_uInt32 nPara ) const;
    virtual Rectangle		GetCharBounds( sal_uInt32 nPara, sal_uInt16 nIndex ) const;
    virtual Rectangle		GetParaBounds( sal_uInt32 nPara ) const;
    virtual MapMode			GetMapMode() const;
	virtual OutputDevice*	GetRefDevice() const;
    virtual sal_Bool		GetIndexAtPoint( const Point&, sal_uInt32& nPara, sal_uInt16& nIndex ) const;
    virtual sal_Bool		GetWordIndices( sal_uInt32 nPara, sal_uInt16 nIndex, sal_uInt16& nStart, sal_uInt16& nEnd ) const;
    virtual sal_Bool 		GetAttributeRun( sal_uInt16& nStartIndex, sal_uInt16& nEndIndex, sal_uInt32 nPara, sal_uInt16 nIndex, sal_Bool bInCell = sal_False ) const;
    virtual sal_uInt16			GetLineCount( sal_uInt32 nPara ) const;
    virtual sal_uInt16			GetLineLen( sal_uInt32 nPara, sal_uInt16 nLine ) const;
    virtual void            GetLineBoundaries( /*out*/sal_uInt16 &rStart, /*out*/sal_uInt16 &rEnd, sal_uInt32 nParagraph, sal_uInt16 nLine ) const;
    virtual sal_uInt16          GetLineNumberAtIndex( sal_uInt32 nPara, sal_uInt16 nIndex ) const;
    virtual sal_Bool		Delete( const ESelection& );
    virtual sal_Bool		InsertText( const String&, const ESelection& );
    virtual sal_Bool		QuickFormatDoc( sal_Bool bFull=sal_False );
    virtual sal_Int16		GetDepth( sal_uInt32 nPara ) const;
    virtual sal_Bool		SetDepth( sal_uInt32 nPara, sal_Int16 nNewDepth );

    virtual const SfxItemSet*   GetEmptyItemSetPtr();
 
    // implementation functions for XParagraphAppend and XTextPortionAppend
    virtual void        AppendParagraph();
    virtual xub_StrLen  AppendTextPortion( sal_uInt32 nPara, const String &rText, const SfxItemSet &rSet );
    //XTextCopy
    virtual void        CopyText(const SvxTextForwarder& rSource); 
};

#endif

