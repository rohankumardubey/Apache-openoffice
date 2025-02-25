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



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sc.hxx"

// System - Includes -----------------------------------------------------



// INCLUDE ---------------------------------------------------------------

#include "scitems.hxx"
#include <editeng/eeitem.hxx>

#include <svx/algitem.hxx>
#include <svtools/colorcfg.hxx>
#include <editeng/editview.hxx>
#include <editeng/editstat.hxx>
#include <editeng/escpitem.hxx>
#include <editeng/flditem.hxx>
#include <editeng/numitem.hxx>
#include <vcl/svapp.hxx>
#include <vcl/outdev.hxx>
#include <svl/inethist.hxx>
#include <unotools/syslocale.hxx>
#ifndef _SVSTDARR_USHORTS
#define _SVSTDARR_USHORTS
#include <svl/svstdarr.hxx>
#endif

#include "editutil.hxx"
#include "global.hxx"
#include "attrib.hxx"
#include "document.hxx"
#include "docpool.hxx"
#include "patattr.hxx"
#include "scmod.hxx"
#include "inputopt.hxx"
#include "compiler.hxx"

// STATIC DATA -----------------------------------------------------------

//	Delimiters zusaetzlich zu EditEngine-Default:

const sal_Char __FAR_DATA ScEditUtil::pCalcDelimiters[] = "=()+-*/^&<>";


//------------------------------------------------------------------------

String ScEditUtil::ModifyDelimiters( const String& rOld )
{
	String aRet = rOld;
	aRet.EraseAllChars( '_' );	// underscore is used in function argument names
	aRet.AppendAscii( RTL_CONSTASCII_STRINGPARAM( pCalcDelimiters ) );
    aRet.Append(ScCompiler::GetNativeSymbol(ocSep)); // argument separator is localized.
	return aRet;
}

static String lcl_GetDelimitedString( const EditEngine& rEngine, const sal_Char c )
{
	String aRet;
	sal_uInt32 nParCount = rEngine.GetParagraphCount();
	for (sal_uInt32 nPar=0; nPar<nParCount; nPar++)
	{
		if (nPar > 0)
			aRet += c;
		aRet += rEngine.GetText( nPar );
	}
	return aRet;
}

String ScEditUtil::GetSpaceDelimitedString( const EditEngine& rEngine )
{
    return lcl_GetDelimitedString(rEngine, ' ');
}

String ScEditUtil::GetMultilineString( const EditEngine& rEngine )
{
    return lcl_GetDelimitedString(rEngine, '\n');
}

//------------------------------------------------------------------------

Rectangle ScEditUtil::GetEditArea( const ScPatternAttr* pPattern, sal_Bool bForceToTop )
{
	// bForceToTop = always align to top, for editing
	// (sal_False for querying URLs etc.)

	if (!pPattern)
		pPattern = pDoc->GetPattern( nCol, nRow, nTab );

	Point aStartPos = aScrPos;

	sal_Bool bLayoutRTL = pDoc->IsLayoutRTL( nTab );
	long nLayoutSign = bLayoutRTL ? -1 : 1;

	const ScMergeAttr* pMerge = (const ScMergeAttr*)&pPattern->GetItem(ATTR_MERGE);
	long nCellX = (long) ( pDoc->GetColWidth(nCol,nTab) * nPPTX );
	if ( pMerge->GetColMerge() > 1 )
	{
		SCCOL nCountX = pMerge->GetColMerge();
		for (SCCOL i=1; i<nCountX; i++)
			nCellX += (long) ( pDoc->GetColWidth(nCol+i,nTab) * nPPTX );
	}
	long nCellY = (long) ( pDoc->GetRowHeight(nRow,nTab) * nPPTY );
	if ( pMerge->GetRowMerge() > 1 )
	{
		SCROW nCountY = pMerge->GetRowMerge();
        nCellY += (long) pDoc->GetScaledRowHeight( nRow+1, nRow+nCountY-1, nTab, nPPTY);
	}

	const SvxMarginItem* pMargin = (const SvxMarginItem*)&pPattern->GetItem(ATTR_MARGIN);
	sal_uInt16 nIndent = 0;
	if ( ((const SvxHorJustifyItem&)pPattern->GetItem(ATTR_HOR_JUSTIFY)).GetValue() ==
				SVX_HOR_JUSTIFY_LEFT )
		nIndent = ((const SfxUInt16Item&)pPattern->GetItem(ATTR_INDENT)).GetValue();
	long nPixDifX	= (long) ( ( pMargin->GetLeftMargin() + nIndent ) * nPPTX );
	aStartPos.X()	+= nPixDifX * nLayoutSign;
	nCellX			-= nPixDifX + (long) ( pMargin->GetRightMargin() * nPPTX );		// wegen Umbruch etc.

	//	vertikale Position auf die in der Tabelle anpassen

	long nPixDifY;
	long nTopMargin = (long) ( pMargin->GetTopMargin() * nPPTY );
	SvxCellVerJustify eJust = (SvxCellVerJustify) ((const SvxVerJustifyItem&)pPattern->
												GetItem(ATTR_VER_JUSTIFY)).GetValue();

	//	asian vertical is always edited top-aligned
    sal_Bool bAsianVertical = ((const SfxBoolItem&)pPattern->GetItem( ATTR_STACKED )).GetValue() &&
		((const SfxBoolItem&)pPattern->GetItem( ATTR_VERTICAL_ASIAN )).GetValue();

	if ( eJust == SVX_VER_JUSTIFY_TOP ||
			( bForceToTop && ( SC_MOD()->GetInputOptions().GetTextWysiwyg() || bAsianVertical ) ) )
		nPixDifY = nTopMargin;
	else
	{
		MapMode aMode = pDev->GetMapMode();
		pDev->SetMapMode( MAP_PIXEL );

		long nTextHeight = pDoc->GetNeededSize( nCol, nRow, nTab,
												pDev, nPPTX, nPPTY, aZoomX, aZoomY, sal_False );
		if (!nTextHeight)
		{									// leere Zelle
			Font aFont;
			// font color doesn't matter here
			pPattern->GetFont( aFont, SC_AUTOCOL_BLACK, pDev, &aZoomY );
			pDev->SetFont(aFont);
			nTextHeight = pDev->GetTextHeight() + nTopMargin +
							(long) ( pMargin->GetBottomMargin() * nPPTY );
		}

		pDev->SetMapMode(aMode);

		if ( nTextHeight > nCellY + nTopMargin || bForceToTop )
			nPixDifY = 0;							// zu gross -> oben anfangen
		else
		{
			if ( eJust == SVX_VER_JUSTIFY_CENTER )
				nPixDifY = nTopMargin + ( nCellY - nTextHeight ) / 2;
			else
				nPixDifY = nCellY - nTextHeight + nTopMargin;		// JUSTIFY_BOTTOM
		}
	}

	aStartPos.Y() += nPixDifY;
	nCellY		-= nPixDifY;

	if ( bLayoutRTL )
		aStartPos.X() -= nCellX - 2;	// excluding grid on both sides

														//	-1 -> Gitter nicht ueberschreiben
	return Rectangle( aStartPos, Size(nCellX-1,nCellY-1) );
}

//------------------------------------------------------------------------

ScEditAttrTester::ScEditAttrTester( ScEditEngineDefaulter* pEng ) :
	pEngine( pEng ),
	pEditAttrs( NULL ),
	bNeedsObject( sal_False ),
	bNeedsCellAttr( sal_False )
{
	if ( pEngine->GetParagraphCount() > 1 )
	{
		bNeedsObject = sal_True;			//!	Zellatribute finden ?
	}
	else
	{
		const SfxPoolItem* pItem = NULL;
		pEditAttrs = new SfxItemSet( pEngine->GetAttribs(
										ESelection(0,0,0,pEngine->GetTextLen(0)), EditEngineAttribs_OnlyHard ) );
        const SfxItemSet& rEditDefaults = pEngine->GetDefaults();

		for (sal_uInt16 nId = EE_CHAR_START; nId <= EE_CHAR_END && !bNeedsObject; nId++)
		{
			SfxItemState eState = pEditAttrs->GetItemState( nId, sal_False, &pItem );
			if (eState == SFX_ITEM_DONTCARE)
				bNeedsObject = sal_True;
			else if (eState == SFX_ITEM_SET)
			{
				if ( nId == EE_CHAR_ESCAPEMENT || nId == EE_CHAR_PAIRKERNING ||
						nId == EE_CHAR_KERNING || nId == EE_CHAR_XMLATTRIBS )
				{
					//	Escapement and kerning are kept in EditEngine because there are no
					//	corresponding cell format items. User defined attributes are kept in
					//	EditEngine because "user attributes applied to all the text" is different
					//	from "user attributes applied to the cell".

					if ( *pItem != rEditDefaults.Get(nId) )
						bNeedsObject = sal_True;
				}
				else
					if (!bNeedsCellAttr)
						if ( *pItem != rEditDefaults.Get(nId) )
							bNeedsCellAttr = sal_True;
                //  rEditDefaults contains the defaults from the cell format
			}
		}

		//	Feldbefehle enthalten?

		SfxItemState eFieldState = pEditAttrs->GetItemState( EE_FEATURE_FIELD, sal_False );
		if ( eFieldState == SFX_ITEM_DONTCARE || eFieldState == SFX_ITEM_SET )
			bNeedsObject = sal_True;

		//	not converted characters?

		SfxItemState eConvState = pEditAttrs->GetItemState( EE_FEATURE_NOTCONV, sal_False );
		if ( eConvState == SFX_ITEM_DONTCARE || eConvState == SFX_ITEM_SET )
			bNeedsObject = sal_True;
	}
}

ScEditAttrTester::~ScEditAttrTester()
{
	delete pEditAttrs;
}


//------------------------------------------------------------------------

ScEnginePoolHelper::ScEnginePoolHelper( SfxItemPool* pEnginePoolP,
				sal_Bool bDeleteEnginePoolP )
			:
			pEnginePool( pEnginePoolP ),
			pDefaults( NULL ),
			bDeleteEnginePool( bDeleteEnginePoolP ),
			bDeleteDefaults( sal_False )
{
}


ScEnginePoolHelper::ScEnginePoolHelper( const ScEnginePoolHelper& rOrg )
			:
			pEnginePool( rOrg.bDeleteEnginePool ? rOrg.pEnginePool->Clone() : rOrg.pEnginePool ),
			pDefaults( NULL ),
			bDeleteEnginePool( rOrg.bDeleteEnginePool ),
			bDeleteDefaults( sal_False )
{
}


ScEnginePoolHelper::~ScEnginePoolHelper()
{
	if ( bDeleteDefaults )
		delete pDefaults;
	if ( bDeleteEnginePool )
		SfxItemPool::Free(pEnginePool);
}


//------------------------------------------------------------------------

ScEditEngineDefaulter::ScEditEngineDefaulter( SfxItemPool* pEnginePoolP,
				sal_Bool bDeleteEnginePoolP )
			:
			ScEnginePoolHelper( pEnginePoolP, bDeleteEnginePoolP ),
			EditEngine( pEnginePoolP )
{
	//	All EditEngines use ScGlobal::GetEditDefaultLanguage as DefaultLanguage.
	//	DefaultLanguage for InputHandler's EditEngine is updated later.

	SetDefaultLanguage( ScGlobal::GetEditDefaultLanguage() );
}


ScEditEngineDefaulter::ScEditEngineDefaulter( const ScEditEngineDefaulter& rOrg )
			:
			ScEnginePoolHelper( rOrg ),
			EditEngine( pEnginePool )
{
	SetDefaultLanguage( ScGlobal::GetEditDefaultLanguage() );
}


ScEditEngineDefaulter::~ScEditEngineDefaulter()
{
}


void ScEditEngineDefaulter::SetDefaults( const SfxItemSet& rSet, sal_Bool bRememberCopy )
{
	if ( bRememberCopy )
	{
		if ( bDeleteDefaults )
			delete pDefaults;
		pDefaults = new SfxItemSet( rSet );
		bDeleteDefaults = sal_True;
	}
	const SfxItemSet& rNewSet = bRememberCopy ? *pDefaults : rSet;
	sal_Bool bUndo = IsUndoEnabled();
	EnableUndo( sal_False );
	sal_Bool bUpdateMode = GetUpdateMode();
	if ( bUpdateMode )
		SetUpdateMode( sal_False );
	sal_uInt32 nPara = GetParagraphCount();
	for ( sal_uInt32 j=0; j<nPara; j++ )
	{
		SetParaAttribs( j, rNewSet );
	}
	if ( bUpdateMode )
		SetUpdateMode( sal_True );
	if ( bUndo )
		EnableUndo( sal_True );
}


void ScEditEngineDefaulter::SetDefaults( SfxItemSet* pSet, sal_Bool bTakeOwnership )
{
	if ( bDeleteDefaults )
		delete pDefaults;
	pDefaults = pSet;
	bDeleteDefaults = bTakeOwnership;
	if ( pDefaults )
		SetDefaults( *pDefaults, sal_False );
}


void ScEditEngineDefaulter::SetDefaultItem( const SfxPoolItem& rItem )
{
	if ( !pDefaults )
	{
		pDefaults = new SfxItemSet( GetEmptyItemSet() );
		bDeleteDefaults = sal_True;
	}
	pDefaults->Put( rItem );
	SetDefaults( *pDefaults, sal_False );
}

const SfxItemSet& ScEditEngineDefaulter::GetDefaults()
{
    if ( !pDefaults )
    {
        pDefaults = new SfxItemSet( GetEmptyItemSet() );
        bDeleteDefaults = sal_True;
    }
    return *pDefaults;
}

void ScEditEngineDefaulter::SetText( const EditTextObject& rTextObject )
{
	sal_Bool bUpdateMode = GetUpdateMode();
	if ( bUpdateMode )
		SetUpdateMode( sal_False );
	EditEngine::SetText( rTextObject );
	if ( pDefaults )
		SetDefaults( *pDefaults, sal_False );
	if ( bUpdateMode )
		SetUpdateMode( sal_True );
}

void ScEditEngineDefaulter::SetTextNewDefaults( const EditTextObject& rTextObject,
			const SfxItemSet& rSet, sal_Bool bRememberCopy )
{
	sal_Bool bUpdateMode = GetUpdateMode();
	if ( bUpdateMode )
		SetUpdateMode( sal_False );
	EditEngine::SetText( rTextObject );
	SetDefaults( rSet, bRememberCopy );
	if ( bUpdateMode )
		SetUpdateMode( sal_True );
}

void ScEditEngineDefaulter::SetTextNewDefaults( const EditTextObject& rTextObject,
			SfxItemSet* pSet, sal_Bool bTakeOwnership )
{
	sal_Bool bUpdateMode = GetUpdateMode();
	if ( bUpdateMode )
		SetUpdateMode( sal_False );
	EditEngine::SetText( rTextObject );
	SetDefaults( pSet, bTakeOwnership );
	if ( bUpdateMode )
		SetUpdateMode( sal_True );
}


void ScEditEngineDefaulter::SetText( const String& rText )
{
	sal_Bool bUpdateMode = GetUpdateMode();
	if ( bUpdateMode )
		SetUpdateMode( sal_False );
	EditEngine::SetText( rText );
	if ( pDefaults )
		SetDefaults( *pDefaults, sal_False );
	if ( bUpdateMode )
		SetUpdateMode( sal_True );
}

void ScEditEngineDefaulter::SetTextNewDefaults( const String& rText,
			const SfxItemSet& rSet, sal_Bool bRememberCopy )
{
	sal_Bool bUpdateMode = GetUpdateMode();
	if ( bUpdateMode )
		SetUpdateMode( sal_False );
	EditEngine::SetText( rText );
	SetDefaults( rSet, bRememberCopy );
	if ( bUpdateMode )
		SetUpdateMode( sal_True );
}

void ScEditEngineDefaulter::SetTextNewDefaults( const String& rText,
			SfxItemSet* pSet, sal_Bool bTakeOwnership )
{
	sal_Bool bUpdateMode = GetUpdateMode();
	if ( bUpdateMode )
		SetUpdateMode( sal_False );
	EditEngine::SetText( rText );
	SetDefaults( pSet, bTakeOwnership );
	if ( bUpdateMode )
		SetUpdateMode( sal_True );
}

void ScEditEngineDefaulter::RepeatDefaults()
{
    if ( pDefaults )
    {
        sal_uInt32 nPara = GetParagraphCount();
        for ( sal_uInt32 j=0; j<nPara; j++ )
            SetParaAttribs( j, *pDefaults );
    }
}

void ScEditEngineDefaulter::RemoveParaAttribs()
{
	SfxItemSet* pCharItems = NULL;
	sal_Bool bUpdateMode = GetUpdateMode();
	if ( bUpdateMode )
		SetUpdateMode( sal_False );
	sal_uInt32 nParCount = GetParagraphCount();
	for (sal_uInt32 nPar=0; nPar<nParCount; nPar++)
	{
		const SfxItemSet& rParaAttribs = GetParaAttribs( nPar );
		sal_uInt16 nWhich;
		for (nWhich = EE_CHAR_START; nWhich <= EE_CHAR_END; nWhich ++)
		{
			const SfxPoolItem* pParaItem;
			if ( rParaAttribs.GetItemState( nWhich, sal_False, &pParaItem ) == SFX_ITEM_SET )
			{
				//	if defaults are set, use only items that are different from default
				if ( !pDefaults || *pParaItem != pDefaults->Get(nWhich) )
				{
					if (!pCharItems)
						pCharItems = new SfxItemSet( GetEmptyItemSet() );
					pCharItems->Put( *pParaItem );
				}
			}
		}

		if ( pCharItems )
		{
			SvUShorts aPortions;
			GetPortions( nPar, aPortions );

			//	loop through the portions of the paragraph, and set only those items
			//	that are not overridden by existing character attributes

			sal_uInt16 nPCount = aPortions.Count();
			sal_uInt16 nStart = 0;
			for ( sal_uInt16 nPos=0; nPos<nPCount; nPos++ )
			{
				sal_uInt16 nEnd = aPortions.GetObject( nPos );
				ESelection aSel( nPar, nStart, nPar, nEnd );
				SfxItemSet aOldCharAttrs = GetAttribs( aSel );
				SfxItemSet aNewCharAttrs = *pCharItems;
				for (nWhich = EE_CHAR_START; nWhich <= EE_CHAR_END; nWhich ++)
				{
					//	Clear those items that are different from existing character attributes.
					//	Where no character attributes are set, GetAttribs returns the paragraph attributes.
					const SfxPoolItem* pItem;
					if ( aNewCharAttrs.GetItemState( nWhich, sal_False, &pItem ) == SFX_ITEM_SET &&
						 *pItem != aOldCharAttrs.Get(nWhich) )
					{
						aNewCharAttrs.ClearItem(nWhich);
					}
				}
				if ( aNewCharAttrs.Count() )
					QuickSetAttribs( aNewCharAttrs, aSel );

				nStart = nEnd;
			}

			DELETEZ( pCharItems );
		}

		if ( rParaAttribs.Count() )
		{
			//	clear all paragraph attributes (including defaults),
			//	so they are not contained in resulting EditTextObjects

			SetParaAttribs( nPar, SfxItemSet( *rParaAttribs.GetPool(), rParaAttribs.GetRanges() ) );
		}
	}
	if ( bUpdateMode )
		SetUpdateMode( sal_True );
}

//------------------------------------------------------------------------

ScTabEditEngine::ScTabEditEngine( ScDocument* pDoc )
		: ScEditEngineDefaulter( pDoc->GetEnginePool() )
{
	SetEditTextObjectPool( pDoc->GetEditPool() );
	Init((const ScPatternAttr&)pDoc->GetPool()->GetDefaultItem(ATTR_PATTERN));
}

ScTabEditEngine::ScTabEditEngine( const ScPatternAttr& rPattern,
            SfxItemPool* pEnginePoolP, SfxItemPool* pTextObjectPool )
        : ScEditEngineDefaulter( pEnginePoolP )
{
	if ( pTextObjectPool )
		SetEditTextObjectPool( pTextObjectPool );
	Init( rPattern );
}

void ScTabEditEngine::Init( const ScPatternAttr& rPattern )
{
	SetRefMapMode(MAP_100TH_MM);
	SfxItemSet* pEditDefaults = new SfxItemSet( GetEmptyItemSet() );
	rPattern.FillEditItemSet( pEditDefaults );
	SetDefaults( pEditDefaults );
	// wir haben keine StyleSheets fuer Text
	SetControlWord( GetControlWord() & ~EE_CNTRL_RTFSTYLESHEETS );
}

//------------------------------------------------------------------------
//		Feldbefehle fuer Kopf- und Fusszeilen
//------------------------------------------------------------------------

//
//		Zahlen aus \sw\source\core\doc\numbers.cxx
//

String lcl_GetCharStr( sal_Int32 nNo )
{
	DBG_ASSERT( nNo, "0 ist eine ungueltige Nummer !!" );
	String aStr;

    const sal_Int32 coDiff = 'Z' - 'A' +1;
    sal_Int32 nCalc;

	do {
		nCalc = nNo % coDiff;
		if( !nCalc )
			nCalc = coDiff;
		aStr.Insert( (sal_Unicode)('a' - 1 + nCalc ), 0 );
        nNo = sal::static_int_cast<sal_Int32>( nNo - nCalc );
		if( nNo )
			nNo /= coDiff;
	} while( nNo );
	return aStr;
}

String lcl_GetNumStr( sal_Int32 nNo, SvxNumType eType )
{
	String aTmpStr( '0' );
	if( nNo )
	{
		switch( eType )
		{
		case SVX_CHARS_UPPER_LETTER:
		case SVX_CHARS_LOWER_LETTER:
			aTmpStr = lcl_GetCharStr( nNo );
			break;

		case SVX_ROMAN_UPPER:
		case SVX_ROMAN_LOWER:
            if( nNo < 4000 )
                aTmpStr = SvxNumberFormat::CreateRomanString( nNo, ( eType == SVX_ROMAN_UPPER ) );
            else
                aTmpStr.Erase();
			break;

		case SVX_NUMBER_NONE:
			aTmpStr.Erase();
			break;

//		CHAR_SPECIAL:
//			????

//		case ARABIC:	ist jetzt default
		default:
			aTmpStr = String::CreateFromInt32( nNo );
			break;
		}

        if( SVX_CHARS_UPPER_LETTER == eType )
			aTmpStr.ToUpperAscii();
	}
	return aTmpStr;
}

ScHeaderFieldData::ScHeaderFieldData()
{
	nPageNo = nTotalPages = 0;
	eNumType = SVX_ARABIC;
}

ScHeaderEditEngine::ScHeaderEditEngine( SfxItemPool* pEnginePoolP, sal_Bool bDeleteEnginePoolP )
        : ScEditEngineDefaulter( pEnginePoolP, bDeleteEnginePoolP )
{
}

String __EXPORT ScHeaderEditEngine::CalcFieldValue( const SvxFieldItem& rField,
                                    sal_uInt32 /* nPara */, sal_uInt16 /* nPos */,
                                    Color*& /* rTxtColor */, Color*& /* rFldColor */ )
{
	String aRet;
	const SvxFieldData*	pFieldData = rField.GetField();
	if ( pFieldData )
	{
		TypeId aType = pFieldData->Type();
		if (aType == TYPE(SvxPageField))
            aRet = lcl_GetNumStr( aData.nPageNo,aData.eNumType );
		else if (aType == TYPE(SvxPagesField))
            aRet = lcl_GetNumStr( aData.nTotalPages,aData.eNumType );
		else if (aType == TYPE(SvxTimeField))
            aRet = ScGlobal::pLocaleData->getTime(aData.aTime);
		else if (aType == TYPE(SvxFileField))
			aRet = aData.aTitle;
		else if (aType == TYPE(SvxExtFileField))
		{
			switch ( ((const SvxExtFileField*)pFieldData)->GetFormat() )
			{
				case SVXFILEFORMAT_FULLPATH :
					aRet = aData.aLongDocName;
				break;
				default:
					aRet = aData.aShortDocName;
			}
		}
		else if (aType == TYPE(SvxTableField))
			aRet = aData.aTabName;
		else if (aType == TYPE(SvxDateField))
            aRet = ScGlobal::pLocaleData->getDate(aData.aDate);
		else
		{
			//DBG_ERROR("unbekannter Feldbefehl");
			aRet = '?';
		}
	}
	else
	{
		DBG_ERROR("FieldData ist 0");
		aRet = '?';
	}

	return aRet;
}

//------------------------------------------------------------------------
//
//							Feld-Daten
//
//------------------------------------------------------------------------

ScFieldEditEngine::ScFieldEditEngine( SfxItemPool* pEnginePoolP,
            SfxItemPool* pTextObjectPool, sal_Bool bDeleteEnginePoolP )
		:
        ScEditEngineDefaulter( pEnginePoolP, bDeleteEnginePoolP ),
		bExecuteURL( sal_True )
{
	if ( pTextObjectPool )
		SetEditTextObjectPool( pTextObjectPool );
	//	EE_CNTRL_URLSFXEXECUTE nicht, weil die Edit-Engine den ViewFrame nicht kennt
	// wir haben keine StyleSheets fuer Text
	SetControlWord( (GetControlWord() | EE_CNTRL_MARKFIELDS) & ~EE_CNTRL_RTFSTYLESHEETS );
}

String __EXPORT ScFieldEditEngine::CalcFieldValue( const SvxFieldItem& rField,
                                    sal_uInt32 /* nPara */, sal_uInt16 /* nPos */,
                                    Color*& rTxtColor, Color*& /* rFldColor */ )
{
	String aRet;
	const SvxFieldData*	pFieldData = rField.GetField();

	if ( pFieldData )
	{
		TypeId aType = pFieldData->Type();

		if (aType == TYPE(SvxURLField))
		{
			String aURL = ((const SvxURLField*)pFieldData)->GetURL();

			switch ( ((const SvxURLField*)pFieldData)->GetFormat() )
			{
				case SVXURLFORMAT_APPDEFAULT: //!!! einstellbar an App???
				case SVXURLFORMAT_REPR:
					aRet = ((const SvxURLField*)pFieldData)->GetRepresentation();
					break;

				case SVXURLFORMAT_URL:
					aRet = aURL;
					break;
			}

            svtools::ColorConfigEntry eEntry =
                INetURLHistory::GetOrCreate()->QueryUrl( aURL ) ? svtools::LINKSVISITED : svtools::LINKS;
			rTxtColor = new Color( SC_MOD()->GetColorConfig().GetColorValue(eEntry).nColor );
		}
		else
		{
			//DBG_ERROR("unbekannter Feldbefehl");
			aRet = '?';
		}
	}

	if (!aRet.Len()) 		// leer ist baeh
		aRet = ' ';			// Space ist Default der Editengine

	return aRet;
}

void __EXPORT ScFieldEditEngine::FieldClicked( const SvxFieldItem& rField, sal_uInt32, sal_uInt16 )
{
	const SvxFieldData* pFld = rField.GetField();

	if ( pFld && pFld->ISA( SvxURLField ) && bExecuteURL )
	{
		const SvxURLField* pURLField = (const SvxURLField*) pFld;
		ScGlobal::OpenURL( pURLField->GetURL(), pURLField->GetTargetFrame() );
	}
}

//------------------------------------------------------------------------

ScNoteEditEngine::ScNoteEditEngine( SfxItemPool* pEnginePoolP,
            SfxItemPool* pTextObjectPool, sal_Bool bDeleteEnginePoolP ) :
    ScEditEngineDefaulter( pEnginePoolP, bDeleteEnginePoolP )
{
	if ( pTextObjectPool )
		SetEditTextObjectPool( pTextObjectPool );
	SetControlWord( (GetControlWord() | EE_CNTRL_MARKFIELDS) & ~EE_CNTRL_RTFSTYLESHEETS );
}
