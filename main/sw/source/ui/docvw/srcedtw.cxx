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
#include "precompiled_sw.hxx"


#include <hintids.hxx>
#ifndef _CMDID_H
#include <cmdid.h>
#endif


#include <svtools/textview.hxx>
#ifndef _SVX_SVXIDS_HRC
#include <svx/svxids.hrc>
#endif
#ifndef _SCRBAR_HXX //autogen
#include <vcl/scrbar.hxx>
#endif
#include <sfx2/dispatch.hxx>
#include <sfx2/app.hxx>
#include <svtools/htmltokn.h>
#include <svtools/txtattr.hxx>
#include <unotools/sourceviewconfig.hxx>
#include <svtools/colorcfg.hxx>
#include <editeng/flstitem.hxx>
#include <vcl/metric.hxx>
#include <svtools/ctrltool.hxx>
#include <tools/time.hxx>
#include <swmodule.hxx>
#ifndef _DOCSH_HXX
#include <docsh.hxx>
#endif
#ifndef _SRCVIEW_HXX
#include <srcview.hxx>
#endif
#ifndef _HELPID_H
#include <helpid.h>
#endif
#include <deque>



struct SwTextPortion
{
	sal_uInt32 nLine;
	sal_uInt16 nStart, nEnd;
    svtools::ColorConfigEntry eType;
};

#define MAX_SYNTAX_HIGHLIGHT 20
#define MAX_HIGHLIGHTTIME 200
#define SYNTAX_HIGHLIGHT_TIMEOUT 200

typedef std::deque<SwTextPortion> SwTextPortions;


static void lcl_Highlight(const String& rSource, SwTextPortions& aPortionList)
{
	const sal_Unicode cOpenBracket = '<';
	const sal_Unicode cCloseBracket= '>';
	const sal_Unicode cSlash		= '/';
	const sal_Unicode cExclamation = '!';
	const sal_Unicode cMinus		= '-';
	const sal_Unicode cSpace		= ' ';
	const sal_Unicode cTab			= 0x09;
	const sal_Unicode cLF          = 0x0a;
	const sal_Unicode cCR          = 0x0d;


	const sal_uInt16 nStrLen = rSource.Len();
	sal_uInt16 nInsert = 0;				// Number of inserted Portions
	sal_uInt16 nActPos = 0;				// Position, at the '<' was found
	sal_uInt16 nOffset = 0; 			// Offset of nActPos for '<'
	sal_uInt16 nPortStart = USHRT_MAX; 	// For the TextPortion
	sal_uInt16 nPortEnd  = 	0;  		//
	SwTextPortion aText;
	while(nActPos < nStrLen)
	{
        svtools::ColorConfigEntry eFoundType = svtools::HTMLUNKNOWN;
		if(rSource.GetChar(nActPos) == cOpenBracket && nActPos < nStrLen - 2 )
		{
			// 'leere' Portion einfuegen
			if(nPortEnd < nActPos - 1 )
			{
				aText.nLine = 0;
				// am Anfang nicht verschieben
				aText.nStart = nPortEnd;
				if(nInsert)
					aText.nStart += 1;
				aText.nEnd = nActPos - 1;
                aText.eType = svtools::HTMLUNKNOWN;
				aPortionList.push_back( aText );
                nInsert++;
			}
			sal_Unicode cFollowFirst = rSource.GetChar((xub_StrLen)(nActPos + 1));
			sal_Unicode cFollowNext = rSource.GetChar((xub_StrLen)(nActPos + 2));
			if(cExclamation == cFollowFirst)
			{
				// "<!" SGML oder Kommentar
				if(cMinus == cFollowNext &&
					nActPos < nStrLen - 3 && cMinus == rSource.GetChar((xub_StrLen)(nActPos + 3)))
				{
                    eFoundType = svtools::HTMLCOMMENT;
				}
				else
                    eFoundType = svtools::HTMLSGML;
				nPortStart = nActPos;
				nPortEnd = nActPos + 1;
			}
			else if(cSlash == cFollowFirst)
			{
				// "</" Slash ignorieren
				nPortStart = nActPos;
				nActPos++;
				nOffset++;
			}
            if(svtools::HTMLUNKNOWN == eFoundType)
			{
				//jetzt koennte hier ein keyword folgen
				sal_uInt16 nSrchPos = nActPos;
				while(++nSrchPos < nStrLen - 1)
				{
					sal_Unicode cNext = rSource.GetChar(nSrchPos);
					if( cNext == cSpace	||
						cNext == cTab 	||
						cNext == cLF 	||
						cNext == cCR)
						break;
					else if(cNext == cCloseBracket)
					{
						break;
					}
				}
				if(nSrchPos > nActPos + 1)
				{
					//irgend ein String wurde gefunden
					String sToken = rSource.Copy(nActPos + 1, nSrchPos - nActPos - 1 );
					sToken.ToUpperAscii();
					int nToken = ::GetHTMLToken(sToken);
					if(nToken)
					{
						//Token gefunden
                        eFoundType = svtools::HTMLKEYWORD;
						nPortEnd = nSrchPos;
						nPortStart = nActPos;
					}
					else
					{
						//was war das denn?
#if OSL_DEBUG_LEVEL > 1
                        DBG_ERROR("Token nicht erkannt!");
                        DBG_ERROR(ByteString(sToken, gsl_getSystemTextEncoding()).GetBuffer());
#endif
					}

				}
			}
			// jetzt muss noch '>' gesucht werden
            if(svtools::HTMLUNKNOWN != eFoundType)
			{
				sal_Bool bFound = sal_False;
				for(sal_uInt16 i = nPortEnd; i < nStrLen; i++)
					if(cCloseBracket == rSource.GetChar(i))
					{
						bFound = sal_True;
						nPortEnd = i;
						break;
					}
                if(!bFound && (eFoundType == svtools::HTMLCOMMENT))
				{
					// Kommentar ohne Ende in dieser Zeile
					bFound  = sal_True;
					nPortEnd = nStrLen - 1;
				}

                if(bFound ||(eFoundType == svtools::HTMLCOMMENT))
				{
                    SwTextPortion aTextPortion;
                    aTextPortion.nLine = 0;
                    aTextPortion.nStart = nPortStart + 1;
                    aTextPortion.nEnd = nPortEnd;
                    aTextPortion.eType = eFoundType;
                    aPortionList.push_back( aTextPortion ); 
                    nInsert++;
                    eFoundType = svtools::HTMLUNKNOWN;
				}

			}
		}
		nActPos++;
	}
	if(nInsert && nPortEnd < nActPos - 1)
	{
		aText.nLine = 0;
		aText.nStart = nPortEnd + 1;
		aText.nEnd = nActPos - 1;
        aText.eType = svtools::HTMLUNKNOWN;
		aPortionList.push_back( aText );
        nInsert++;
	}
}

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/


SwSrcEditWindow::SwSrcEditWindow( Window* pParent, SwSrcView* pParentView ) :
	Window( pParent, WB_BORDER|WB_CLIPCHILDREN ),

    pTextEngine(0),

    pOutWin(0),
    pHScrollbar(0),
	pVScrollbar(0),

    pSrcView(pParentView),
    pSourceViewConfig(new utl::SourceViewConfig),

	nCurTextWidth(0),
    nStartLine(USHRT_MAX),
    eSourceEncoding(gsl_getSystemTextEncoding()),
	bDoSyntaxHighlight(sal_True),
    bHighlighting(sal_False)
{
	SetHelpId(HID_SOURCE_EDITWIN);
	CreateTextEngine();
    pSourceViewConfig->AddListener(this);
}
/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/
 SwSrcEditWindow::~SwSrcEditWindow()
{
    pSourceViewConfig->RemoveListener(this);
    delete pSourceViewConfig;
    aSyntaxIdleTimer.Stop();
	if ( pTextEngine )
	{
		EndListening( *pTextEngine );
		pTextEngine->RemoveView( pTextView );

		delete pHScrollbar;
		delete pVScrollbar;

		delete pTextView;
		delete pTextEngine;
	}
	delete pOutWin;
}

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/

void SwSrcEditWindow::DataChanged( const DataChangedEvent& rDCEvt )
{
	Window::DataChanged( rDCEvt );

	switch ( rDCEvt.GetType() )
	{
	case DATACHANGED_SETTINGS:
		// ScrollBars neu anordnen bzw. Resize ausloesen, da sich
		// ScrollBar-Groesse geaendert haben kann. Dazu muss dann im
		// Resize-Handler aber auch die Groesse der ScrollBars aus
		// den Settings abgefragt werden.
		if( rDCEvt.GetFlags() & SETTINGS_STYLE )
			Resize();
		break;
	}
}

void  SwSrcEditWindow::Resize()
{
	// ScrollBars, etc. passiert in Adjust...
	if ( pTextView )
	{
		long nVisY = pTextView->GetStartDocPos().Y();
		pTextView->ShowCursor();
		Size aOutSz( GetOutputSizePixel() );
		long nMaxVisAreaStart = pTextView->GetTextEngine()->GetTextHeight() - aOutSz.Height();
		if ( nMaxVisAreaStart < 0 )
			nMaxVisAreaStart = 0;
		if ( pTextView->GetStartDocPos().Y() > nMaxVisAreaStart )
		{
			Point aStartDocPos( pTextView->GetStartDocPos() );
			aStartDocPos.Y() = nMaxVisAreaStart;
			pTextView->SetStartDocPos( aStartDocPos );
			pTextView->ShowCursor();
		}
        long nScrollStd = GetSettings().GetStyleSettings().GetScrollBarSize();
		Size aScrollSz(aOutSz.Width() - nScrollStd, nScrollStd );
		Point aScrollPos(0, aOutSz.Height() - nScrollStd);

		pHScrollbar->SetPosSizePixel( aScrollPos, aScrollSz);

		aScrollSz.Width() = aScrollSz.Height();
		aScrollSz.Height() = aOutSz.Height();
		aScrollPos = Point(aOutSz.Width() - nScrollStd, 0);

		pVScrollbar->SetPosSizePixel( aScrollPos, aScrollSz);
		aOutSz.Width() 	-= nScrollStd;
		aOutSz.Height() 	-= nScrollStd;
		pOutWin->SetOutputSizePixel(aOutSz);
        InitScrollBars();

        // Zeile im ersten Resize setzen
		if(USHRT_MAX != nStartLine)
		{
			if(nStartLine < pTextEngine->GetParagraphCount())
			{
				TextSelection aSel(TextPaM( nStartLine, 0 ), TextPaM( nStartLine, 0x0 ));
				pTextView->SetSelection(aSel);
				pTextView->ShowCursor();
			}
			nStartLine = USHRT_MAX;
		}

		if ( nVisY != pTextView->GetStartDocPos().Y() )
			Invalidate();
	}

}

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/

void TextViewOutWin::DataChanged( const DataChangedEvent& rDCEvt )
{
	Window::DataChanged( rDCEvt );

	switch( rDCEvt.GetType() )
	{
	case DATACHANGED_SETTINGS:
		// den Settings abgefragt werden.
		if( rDCEvt.GetFlags() & SETTINGS_STYLE )
		{
			const Color &rCol = GetSettings().GetStyleSettings().GetWindowColor();
			SetBackground( rCol );
			Font aFont( pTextView->GetTextEngine()->GetFont() );
			aFont.SetFillColor( rCol );
			pTextView->GetTextEngine()->SetFont( aFont );
		}
		break;
	}
}

void  TextViewOutWin::MouseMove( const MouseEvent &rEvt )
{
	if ( pTextView )
		pTextView->MouseMove( rEvt );
}

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/


void  TextViewOutWin::MouseButtonUp( const MouseEvent &rEvt )
{
	if ( pTextView )
	{
		pTextView->MouseButtonUp( rEvt );
		SfxBindings& rBindings = ((SwSrcEditWindow*)GetParent())->GetSrcView()->GetViewFrame()->GetBindings();
		rBindings.Invalidate( SID_TABLE_CELL );
		rBindings.Invalidate( SID_CUT );
		rBindings.Invalidate( SID_COPY );
	}
}

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/


void  TextViewOutWin::MouseButtonDown( const MouseEvent &rEvt )
{
	GrabFocus();
	if ( pTextView )
		pTextView->MouseButtonDown( rEvt );
}

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/


void  TextViewOutWin::Command( const CommandEvent& rCEvt )
{
	switch(rCEvt.GetCommand())
	{
		case COMMAND_CONTEXTMENU:
			((SwSrcEditWindow*)GetParent())->GetSrcView()->GetViewFrame()->
				GetDispatcher()->ExecutePopup();
		break;
		case COMMAND_WHEEL:
		case COMMAND_STARTAUTOSCROLL:
		case COMMAND_AUTOSCROLL:
		{
			const CommandWheelData* pWData = rCEvt.GetWheelData();
			if( !pWData || COMMAND_WHEEL_ZOOM != pWData->GetMode() )
			{
				((SwSrcEditWindow*)GetParent())->HandleWheelCommand( rCEvt );
			}
		}
		break;

		default:
			if ( pTextView )
			pTextView->Command( rCEvt );
		else
			Window::Command(rCEvt);
	}
}


/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/


void  TextViewOutWin::KeyInput( const KeyEvent& rKEvt )
{
	sal_Bool bDone = sal_False;
	SwSrcEditWindow* pSrcEditWin = (SwSrcEditWindow*)GetParent();
	sal_Bool bChange = !pSrcEditWin->IsReadonly() || !TextEngine::DoesKeyChangeText( rKEvt );
	if(bChange)
		bDone = pTextView->KeyInput( rKEvt );

	SfxBindings& rBindings = ((SwSrcEditWindow*)GetParent())->GetSrcView()->GetViewFrame()->GetBindings();
	if ( !bDone )
	{
		if ( !SfxViewShell::Current()->KeyInput( rKEvt ) )
			Window::KeyInput( rKEvt );
	}
	else
	{
		rBindings.Invalidate( SID_TABLE_CELL );
		if ( rKEvt.GetKeyCode().GetGroup() == KEYGROUP_CURSOR )
			rBindings.Update( SID_BASICIDE_STAT_POS );
		if (pSrcEditWin->GetTextEngine()->IsModified() )
		{
			rBindings.Invalidate( SID_SAVEDOC );
			rBindings.Invalidate( SID_DOC_MODIFIED );
		}
		if( rKEvt.GetKeyCode().GetCode() == KEY_INSERT )
			rBindings.Invalidate( SID_ATTR_INSERT );
	}

	rBindings.Invalidate( SID_CUT );
	rBindings.Invalidate( SID_COPY );

	SwDocShell* pDocShell = pSrcEditWin->GetSrcView()->GetDocShell();
	if(pSrcEditWin->GetTextEngine()->IsModified())
	{
		pDocShell->SetModified();
	}
}

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/


void  TextViewOutWin::Paint( const Rectangle& rRect )
{
	pTextView->Paint( rRect );
}

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/


void SwSrcEditWindow::CreateTextEngine()
{
	const Color &rCol = GetSettings().GetStyleSettings().GetWindowColor();
	pOutWin = new TextViewOutWin(this, 0);
	pOutWin->SetBackground(Wallpaper(rCol));
	pOutWin->SetPointer(Pointer(POINTER_TEXT));
	pOutWin->Show();

	//Scrollbars anlegen
	pHScrollbar = new ScrollBar(this, WB_3DLOOK |WB_HSCROLL|WB_DRAG);
        pHScrollbar->EnableRTL( false ); // #107300# --- RTL --- no mirroring for scrollbars
	pHScrollbar->SetScrollHdl(LINK(this, SwSrcEditWindow, ScrollHdl));
	pHScrollbar->Show();

	pVScrollbar = new ScrollBar(this, WB_3DLOOK |WB_VSCROLL|WB_DRAG);
        pVScrollbar->EnableRTL( false ); // #107300# --- RTL --- no mirroring for scrollbars
	pVScrollbar->SetScrollHdl(LINK(this, SwSrcEditWindow, ScrollHdl));
	pHScrollbar->EnableDrag();
	pVScrollbar->Show();

	pTextEngine = new ExtTextEngine;
	pTextView = new ExtTextView( pTextEngine, pOutWin );
	pTextView->SetAutoIndentMode(sal_True);
	pOutWin->SetTextView(pTextView);

	pTextEngine->SetUpdateMode( sal_False );
	pTextEngine->InsertView( pTextView );

	Font aFont;
	aFont.SetTransparent( sal_False );
	aFont.SetFillColor( rCol );
	SetPointFont( aFont );
	aFont = GetFont();
	aFont.SetFillColor( rCol );
	pOutWin->SetFont( aFont );
	pTextEngine->SetFont( aFont );

    aSyntaxIdleTimer.SetTimeout( SYNTAX_HIGHLIGHT_TIMEOUT );
	aSyntaxIdleTimer.SetTimeoutHdl( LINK( this, SwSrcEditWindow, SyntaxTimerHdl ) );

	pTextEngine->EnableUndo( sal_True );
	pTextEngine->SetUpdateMode( sal_True );

	pTextView->ShowCursor( sal_True, sal_True );
	InitScrollBars();
	StartListening( *pTextEngine );

	SfxBindings& rBind = GetSrcView()->GetViewFrame()->GetBindings();
	rBind.Invalidate( SID_TABLE_CELL );
//	rBind.Invalidate( SID_ATTR_CHAR_FONTHEIGHT );
}

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/


void SwSrcEditWindow::SetScrollBarRanges()
{
	// Extra-Methode, nicht InitScrollBars, da auch fuer TextEngine-Events.

	pHScrollbar->SetRange( Range( 0, nCurTextWidth-1 ) );
	pVScrollbar->SetRange( Range(0, pTextEngine->GetTextHeight()-1) );
}

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/


void SwSrcEditWindow::InitScrollBars()
{
	SetScrollBarRanges();

    Size aOutSz( pOutWin->GetOutputSizePixel() );
    pVScrollbar->SetVisibleSize( aOutSz.Height() );
	pVScrollbar->SetPageSize(  aOutSz.Height() * 8 / 10 );
	pVScrollbar->SetLineSize( pOutWin->GetTextHeight() );
	pVScrollbar->SetThumbPos( pTextView->GetStartDocPos().Y() );
	pHScrollbar->SetVisibleSize( aOutSz.Width() );
	pHScrollbar->SetPageSize( aOutSz.Width() * 8 / 10 );
	pHScrollbar->SetLineSize( pOutWin->GetTextWidth( 'x' ) );
	pHScrollbar->SetThumbPos( pTextView->GetStartDocPos().X() );

}

/*--------------------------------------------------------------------
	Beschreibung:
 --------------------------------------------------------------------*/


IMPL_LINK(SwSrcEditWindow, ScrollHdl, ScrollBar*, pScroll)
{
	if(pScroll == pVScrollbar)
	{
		long nDiff = pTextView->GetStartDocPos().Y() - pScroll->GetThumbPos();
		GetTextView()->Scroll( 0, nDiff );
		pTextView->ShowCursor( sal_False, sal_True );
		pScroll->SetThumbPos( pTextView->GetStartDocPos().Y() );
	}
	else
	{
		long nDiff = pTextView->GetStartDocPos().X() - pScroll->GetThumbPos();
		GetTextView()->Scroll( nDiff, 0 );
		pTextView->ShowCursor( sal_False, sal_True );
		pScroll->SetThumbPos( pTextView->GetStartDocPos().X() );
	}
	GetSrcView()->GetViewFrame()->GetBindings().Invalidate( SID_TABLE_CELL );
	return 0;
}

/*-----------------15.01.97 09.22-------------------

--------------------------------------------------*/

IMPL_LINK( SwSrcEditWindow, SyntaxTimerHdl, Timer *, pTimer )
{
    Time aSyntaxCheckStart;
    DBG_ASSERT( pTextView, "Noch keine View, aber Syntax-Highlight ?!" );
	// pTextEngine->SetUpdateMode( sal_False );

	bHighlighting = sal_True;
	sal_uInt32 nLine;
	sal_uInt16 nCount  = 0;
	// zuerst wird der Bereich um dem Cursor bearbeitet
	TextSelection aSel = pTextView->GetSelection();
    sal_uInt32 nCur = aSel.GetStart().GetPara();
	if(nCur > 40)
		nCur -= 40;
	else
		nCur = 0;
	if(aSyntaxLineTable.Count())
		for(sal_uInt16 i = 0; i < 80 && nCount < 40; i++, nCur++)
		{
			void * p = aSyntaxLineTable.Get(nCur);
			if(p)
			{
				DoSyntaxHighlight( nCur );
				aSyntaxLineTable.Remove( nCur );
				nCount++;
                if(!aSyntaxLineTable.Count())
                    break;
                if((Time().GetTime() - aSyntaxCheckStart.GetTime()) > MAX_HIGHLIGHTTIME )
                {
                    pTimer->SetTimeout( 2 * SYNTAX_HIGHLIGHT_TIMEOUT );
                    break;
                }
            }
		}

	// wenn dann noch etwas frei ist, wird von Beginn an weitergearbeitet
	void* p = aSyntaxLineTable.First();
	while ( p && nCount < MAX_SYNTAX_HIGHLIGHT)
	{
		nLine = (sal_uInt32)aSyntaxLineTable.GetCurKey();
		DoSyntaxHighlight( nLine );
		p = aSyntaxLineTable.Next();
        aSyntaxLineTable.Remove(nLine);
		nCount ++;
        if(Time().GetTime() - aSyntaxCheckStart.GetTime() > MAX_HIGHLIGHTTIME)
        {
            pTimer->SetTimeout( 2 * SYNTAX_HIGHLIGHT_TIMEOUT );
            break;
        }
	}
	// os: #43050# hier wird ein TextView-Problem umpopelt:
	// waehrend des Highlightings funktionierte das Scrolling nicht
	/* MT: Shouldn't be a oproblem any more, using IdeFormatter in Insert/RemoveAttrib now.

    	TextView* pTmp = pTextEngine->GetActiveView();
    	pTextEngine->SetActiveView(0);
    	// pTextEngine->SetUpdateMode( sal_True );
    	pTextEngine->SetActiveView(pTmp);
    	pTextView->ShowCursor(sal_False, sal_False);
    */

	if(aSyntaxLineTable.Count() && !pTimer->IsActive())
		pTimer->Start();
	// SyntaxTimerHdl wird gerufen, wenn Text-Aenderung
	// => gute Gelegenheit, Textbreite zu ermitteln!
	long nPrevTextWidth = nCurTextWidth;
	nCurTextWidth = pTextEngine->CalcTextWidth() + 25;	// kleine Toleranz
	if ( nCurTextWidth != nPrevTextWidth )
		SetScrollBarRanges();
	bHighlighting = sal_False;

    return 0;
}
/*-----------------15.01.97 10.01-------------------

--------------------------------------------------*/

void SwSrcEditWindow::DoSyntaxHighlight( sal_uInt32 nPara )
{
	// Durch das DelayedSyntaxHighlight kann es passieren,
	// dass die Zeile nicht mehr existiert!
	if ( nPara < pTextEngine->GetParagraphCount() )
	{
		sal_Bool bTempModified = IsModified();
		pTextEngine->RemoveAttribs( nPara, (sal_Bool)sal_True );
		String aSource( pTextEngine->GetText( nPara ) );
		pTextEngine->SetUpdateMode( sal_False );
		ImpDoHighlight( aSource, nPara );
		// os: #43050# hier wird ein TextView-Problem umpopelt:
		// waehrend des Highlightings funktionierte das Scrolling nicht
		TextView* pTmp = pTextEngine->GetActiveView();
		pTmp->SetAutoScroll(sal_False);
		pTextEngine->SetActiveView(0);
		pTextEngine->SetUpdateMode( sal_True );
		pTextEngine->SetActiveView(pTmp);
		// Bug 72887 show the cursor
		pTmp->SetAutoScroll(sal_True);
		pTmp->ShowCursor( sal_False/*pTmp->IsAutoScroll()*/ );

		if(!bTempModified)
			ClearModifyFlag();
	}
}

/*-----------------15.01.97 09.49-------------------

--------------------------------------------------*/

void SwSrcEditWindow::DoDelayedSyntaxHighlight( sal_uInt32 nPara )
{
	if ( !bHighlighting && bDoSyntaxHighlight )
	{
		aSyntaxLineTable.Insert( nPara, (void*)(sal_uInt16)1 );
		aSyntaxIdleTimer.Start();
	}
}

/*-----------------15.01.97 11.32-------------------

--------------------------------------------------*/

void SwSrcEditWindow::ImpDoHighlight( const String& rSource, sal_uInt32 nLineOff )
{
	SwTextPortions aPortionList;
	lcl_Highlight(rSource, aPortionList);

	size_t nCount = aPortionList.size();
	if ( !nCount )
		return;

	SwTextPortion& rLast = aPortionList[nCount-1];
	if ( rLast.nStart > rLast.nEnd ) 	// Nur bis Bug von MD behoeben
	{
		nCount--;
		aPortionList.pop_back();
		if ( !nCount )
			return;
	}

	// Evtl. Optimieren:
	// Wenn haufig gleiche Farbe, dazwischen Blank ohne Farbe,
	// ggf. zusammenfassen, oder zumindest das Blank,
	// damit weniger Attribute
	sal_Bool bOptimizeHighlight = sal_True; // war in der BasicIDE static
	if ( bOptimizeHighlight )
	{
		// Es muessen nur die Blanks und Tabs mit attributiert werden.
		// Wenn zwei gleiche Attribute hintereinander eingestellt werden,
		// optimiert das die TextEngine.
		sal_uInt16 nLastEnd = 0;

#ifdef DBG_UTIL
        sal_uInt32 nLine = aPortionList[0].nLine;
#endif
		for ( size_t i = 0; i < nCount; i++ )
		{
			SwTextPortion& r = aPortionList[i];
			DBG_ASSERT( r.nLine == nLine, "doch mehrere Zeilen ?" );
			if ( r.nStart > r.nEnd ) 	// Nur bis Bug von MD behoeben
				continue;

			if ( r.nStart > nLastEnd )
			{
				// Kann ich mich drauf verlassen, dass alle ausser
				// Blank und Tab gehighlightet wird ?!
				r.nStart = nLastEnd;
			}
			nLastEnd = r.nEnd+1;
			if ( ( i == (nCount-1) ) && ( r.nEnd < rSource.Len() ) )
				r.nEnd = rSource.Len();
		}
	}

	for ( size_t i = 0; i < aPortionList.size(); i++ )
	{
		SwTextPortion& r = aPortionList[i];
		if ( r.nStart > r.nEnd ) 	// Nur bis Bug von MD behoeben
			continue;
        if(r.eType !=  svtools::HTMLSGML    &&
            r.eType != svtools::HTMLCOMMENT &&
            r.eType != svtools::HTMLKEYWORD &&
            r.eType != svtools::HTMLUNKNOWN)
                r.eType = svtools::HTMLUNKNOWN;
        Color aColor((ColorData)SW_MOD()->GetColorConfig().GetColorValue((svtools::ColorConfigEntry)r.eType).nColor);
        sal_uInt32 nLine = nLineOff+r.nLine; //
        pTextEngine->SetAttrib( TextAttribFontColor( aColor ), nLine, r.nStart, r.nEnd+1, sal_True );
	}
}

/*-----------------30.06.97 09:12-------------------

--------------------------------------------------*/

void SwSrcEditWindow::Notify( SfxBroadcaster& /*rBC*/, const SfxHint& rHint )
{
	if ( rHint.ISA( TextHint ) )
	{
		const TextHint& rTextHint = (const TextHint&)rHint;
		if( rTextHint.GetId() == TEXT_HINT_VIEWSCROLLED )
		{
			pHScrollbar->SetThumbPos( pTextView->GetStartDocPos().X() );
			pVScrollbar->SetThumbPos( pTextView->GetStartDocPos().Y() );
		}
		else if( rTextHint.GetId() == TEXT_HINT_TEXTHEIGHTCHANGED )
		{
			if ( (long)pTextEngine->GetTextHeight() < pOutWin->GetOutputSizePixel().Height() )
				pTextView->Scroll( 0, pTextView->GetStartDocPos().Y() );
			pVScrollbar->SetThumbPos( pTextView->GetStartDocPos().Y() );
			SetScrollBarRanges();
		}
		else if( ( rTextHint.GetId() == TEXT_HINT_PARAINSERTED ) ||
		         ( rTextHint.GetId() == TEXT_HINT_PARACONTENTCHANGED ) )
		{
            DoDelayedSyntaxHighlight( rTextHint.GetValue() );
		}
	}
}

void SwSrcEditWindow::ConfigurationChanged( utl::ConfigurationBroadcaster* pBrdCst, sal_uInt32 )
{
    if( pBrdCst == pSourceViewConfig)
        SetFont();
}

/*-----------------30.06.97 13:22-------------------

--------------------------------------------------*/

void    SwSrcEditWindow::Invalidate(sal_uInt16 )
{
	pOutWin->Invalidate();
	Window::Invalidate();

}

void SwSrcEditWindow::Command( const CommandEvent& rCEvt )
{
	switch(rCEvt.GetCommand())
	{
		case COMMAND_WHEEL:
		case COMMAND_STARTAUTOSCROLL:
		case COMMAND_AUTOSCROLL:
		{
			const CommandWheelData* pWData = rCEvt.GetWheelData();
			if( !pWData || COMMAND_WHEEL_ZOOM != pWData->GetMode() )
				HandleScrollCommand( rCEvt, pHScrollbar, pVScrollbar );
		}
		break;
		default:
			Window::Command(rCEvt);
	}
}

void SwSrcEditWindow::HandleWheelCommand( const CommandEvent& rCEvt )
{
	pTextView->Command(rCEvt);
	HandleScrollCommand( rCEvt, pHScrollbar, pVScrollbar );
}

void SwSrcEditWindow::GetFocus()
{
	pOutWin->GrabFocus();
}

/*void SwSrcEditWindow::LoseFocus()
{
	Window::LoseFocus();
//	pOutWin->LoseFocus();
//	rView.LostFocus();
} */
/* -----------------------------29.08.2002 13:21------------------------------

 ---------------------------------------------------------------------------*/
sal_Bool  lcl_GetLanguagesForEncoding(rtl_TextEncoding eEnc, LanguageType aLanguages[])
{
    switch(eEnc)
    {
        case RTL_TEXTENCODING_UTF7             :
        case RTL_TEXTENCODING_UTF8             :
            // don#t fill - all LANGUAGE_SYSTEM means unicode font has to be used
        break;


        case RTL_TEXTENCODING_ISO_8859_3:
        case RTL_TEXTENCODING_ISO_8859_1  :
        case RTL_TEXTENCODING_MS_1252     :
        case RTL_TEXTENCODING_APPLE_ROMAN :
        case RTL_TEXTENCODING_IBM_850     :
        case RTL_TEXTENCODING_ISO_8859_14 :
        case RTL_TEXTENCODING_ISO_8859_15 :
            //fill with western languages
            aLanguages[0] = LANGUAGE_GERMAN;
            aLanguages[1] = LANGUAGE_FRENCH;
            aLanguages[2] = LANGUAGE_ITALIAN;
            aLanguages[3] = LANGUAGE_SPANISH;
        break;

        case RTL_TEXTENCODING_IBM_865     :
            //scandinavian
            aLanguages[0] = LANGUAGE_FINNISH;
            aLanguages[1] = LANGUAGE_NORWEGIAN;
            aLanguages[2] = LANGUAGE_SWEDISH;
            aLanguages[3] = LANGUAGE_DANISH;
        break;

        case RTL_TEXTENCODING_ISO_8859_10      :
        case RTL_TEXTENCODING_ISO_8859_13      :
        case RTL_TEXTENCODING_ISO_8859_2  :
        case RTL_TEXTENCODING_IBM_852     :
        case RTL_TEXTENCODING_MS_1250     :
        case RTL_TEXTENCODING_APPLE_CENTEURO   :
            aLanguages[0] = LANGUAGE_POLISH;
            aLanguages[1] = LANGUAGE_CZECH;
            aLanguages[2] = LANGUAGE_HUNGARIAN;
            aLanguages[3] = LANGUAGE_SLOVAK;
        break;

        case RTL_TEXTENCODING_ISO_8859_4  :
        case RTL_TEXTENCODING_IBM_775     :
        case RTL_TEXTENCODING_MS_1257          :
            aLanguages[0] = LANGUAGE_LATVIAN   ;
            aLanguages[1] = LANGUAGE_LITHUANIAN;
            aLanguages[2] = LANGUAGE_ESTONIAN  ;
        break;

        case RTL_TEXTENCODING_IBM_863       : aLanguages[0] = LANGUAGE_FRENCH_CANADIAN; break;
        case RTL_TEXTENCODING_APPLE_FARSI   : aLanguages[0] = LANGUAGE_FARSI; break;
        case RTL_TEXTENCODING_APPLE_ROMANIAN:aLanguages[0] = LANGUAGE_ROMANIAN; break;

        case RTL_TEXTENCODING_IBM_861     :
        case RTL_TEXTENCODING_APPLE_ICELAND    :
            aLanguages[0] = LANGUAGE_ICELANDIC;
        break;

        case RTL_TEXTENCODING_APPLE_CROATIAN:aLanguages[0] = LANGUAGE_CROATIAN; break;

        case RTL_TEXTENCODING_IBM_437     :
        case RTL_TEXTENCODING_ASCII_US    : aLanguages[0] = LANGUAGE_ENGLISH; break;

        case RTL_TEXTENCODING_IBM_862     :
        case RTL_TEXTENCODING_MS_1255     :
        case RTL_TEXTENCODING_APPLE_HEBREW     :
        case RTL_TEXTENCODING_ISO_8859_8  :
            aLanguages[0] = LANGUAGE_HEBREW;
        break;

        case RTL_TEXTENCODING_IBM_857     :
        case RTL_TEXTENCODING_MS_1254     :
        case RTL_TEXTENCODING_APPLE_TURKISH:
        case RTL_TEXTENCODING_ISO_8859_9  :
            aLanguages[0] = LANGUAGE_TURKISH;
        break;

        case RTL_TEXTENCODING_IBM_860     :
            aLanguages[0] = LANGUAGE_PORTUGUESE;
        break;

        case RTL_TEXTENCODING_IBM_869     :
        case RTL_TEXTENCODING_MS_1253     :
        case RTL_TEXTENCODING_APPLE_GREEK :
        case RTL_TEXTENCODING_ISO_8859_7  :
        case RTL_TEXTENCODING_IBM_737     :
            aLanguages[0] = LANGUAGE_GREEK;
        break;

        case RTL_TEXTENCODING_KOI8_R      :
        case RTL_TEXTENCODING_ISO_8859_5  :
        case RTL_TEXTENCODING_IBM_855     :
        case RTL_TEXTENCODING_MS_1251     :
        case RTL_TEXTENCODING_IBM_866     :
        case RTL_TEXTENCODING_APPLE_CYRILLIC   :
            aLanguages[0] = LANGUAGE_RUSSIAN;
        break;

        case RTL_TEXTENCODING_APPLE_UKRAINIAN:
        case RTL_TEXTENCODING_KOI8_U:
            aLanguages[0] = LANGUAGE_UKRAINIAN;
            break;

        case RTL_TEXTENCODING_IBM_864     :
        case RTL_TEXTENCODING_MS_1256          :
        case RTL_TEXTENCODING_ISO_8859_6  :
        case RTL_TEXTENCODING_APPLE_ARABIC :
            aLanguages[0] = LANGUAGE_ARABIC_SAUDI_ARABIA;
         break;

        case RTL_TEXTENCODING_APPLE_CHINTRAD   :
        case RTL_TEXTENCODING_MS_950           :
        case RTL_TEXTENCODING_GBT_12345        :
        case RTL_TEXTENCODING_BIG5             :
        case RTL_TEXTENCODING_EUC_TW           :
        case RTL_TEXTENCODING_BIG5_HKSCS       :
            aLanguages[0] = LANGUAGE_CHINESE_TRADITIONAL;
        break;

        case RTL_TEXTENCODING_EUC_JP           :
        case RTL_TEXTENCODING_ISO_2022_JP      :
        case RTL_TEXTENCODING_JIS_X_0201       :
        case RTL_TEXTENCODING_JIS_X_0208       :
        case RTL_TEXTENCODING_JIS_X_0212       :
        case RTL_TEXTENCODING_APPLE_JAPANESE   :
        case RTL_TEXTENCODING_MS_932           :
        case RTL_TEXTENCODING_SHIFT_JIS        :
            aLanguages[0] = LANGUAGE_JAPANESE;
        break;

        case RTL_TEXTENCODING_GB_2312          :
        case RTL_TEXTENCODING_MS_936           :
        case RTL_TEXTENCODING_GBK              :
        case RTL_TEXTENCODING_GB_18030         :
        case RTL_TEXTENCODING_APPLE_CHINSIMP   :
        case RTL_TEXTENCODING_EUC_CN           :
        case RTL_TEXTENCODING_ISO_2022_CN      :
            aLanguages[0] = LANGUAGE_CHINESE_SIMPLIFIED;
        break;

        case RTL_TEXTENCODING_APPLE_KOREAN     :
        case RTL_TEXTENCODING_MS_949           :
        case RTL_TEXTENCODING_EUC_KR           :
        case RTL_TEXTENCODING_ISO_2022_KR      :
        case RTL_TEXTENCODING_MS_1361          :
            aLanguages[0] = LANGUAGE_KOREAN;
        break;

        case RTL_TEXTENCODING_APPLE_THAI       :
        case RTL_TEXTENCODING_MS_874      :
        case RTL_TEXTENCODING_TIS_620          :
            aLanguages[0] = LANGUAGE_THAI;
        break;
//        case RTL_TEXTENCODING_SYMBOL      :
//        case RTL_TEXTENCODING_DONTKNOW:        :
        default: aLanguages[0] = Application::GetSettings().GetUILanguage();
    }
    return aLanguages[0] != LANGUAGE_SYSTEM;
}
void SwSrcEditWindow::SetFont()
{
    String sFontName = pSourceViewConfig->GetFontName();
    if(!sFontName.Len())
    {
        LanguageType aLanguages[5] =
        {
            LANGUAGE_SYSTEM, LANGUAGE_SYSTEM, LANGUAGE_SYSTEM, LANGUAGE_SYSTEM, LANGUAGE_SYSTEM
        };
        Font aFont;
        if(lcl_GetLanguagesForEncoding(eSourceEncoding, aLanguages))
        {
            //TODO: check for multiple languages
            aFont = OutputDevice::GetDefaultFont(DEFAULTFONT_FIXED, aLanguages[0], 0, this);
        }
        else
            aFont = OutputDevice::GetDefaultFont(DEFAULTFONT_SANS_UNICODE,
                        Application::GetSettings().GetLanguage(), 0, this);
        sFontName = aFont.GetName();
    }
    const SvxFontListItem* pFontListItem =
        (const SvxFontListItem* )pSrcView->GetDocShell()->GetItem( SID_ATTR_CHAR_FONTLIST );
	const FontList*	 pList = pFontListItem->GetFontList();
    FontInfo aInfo = pList->Get(sFontName,WEIGHT_NORMAL, ITALIC_NONE);

    const Font& rFont = GetTextEngine()->GetFont();
	Font aFont(aInfo);
    Size aSize(rFont.GetSize());
    //font height is stored in point and set in twip
    aSize.Height() = pSourceViewConfig->GetFontHeight() * 20;
    aFont.SetSize(pOutWin->LogicToPixel(aSize, MAP_TWIP));
    GetTextEngine()->SetFont( aFont );
    pOutWin->SetFont(aFont);
}
/* -----------------------------29.08.2002 13:47------------------------------

 ---------------------------------------------------------------------------*/
void SwSrcEditWindow::SetTextEncoding(rtl_TextEncoding eEncoding)
{
    eSourceEncoding = eEncoding;
    SetFont();
}

