#include "stdafx.h"
/*
-----------------------------------------------------------------------------
 Class: ScrollBar

 Desc: See header.

 Copyright (c) 2001-2002 by the person(s) listed below.  All rights reserved.
	Chris Danford
-----------------------------------------------------------------------------
*/

#include "ScrollBar.h"
#include "PrefsManager.h"
#include "GameConstantsAndTypes.h"


ScrollBar::ScrollBar()
{
	m_sprBackground.Load( THEME->GetPathTo("Graphics","select music scrollbar 2x2") );
	m_sprBackground.StopAnimating();
	m_sprBackground.SetState( 1 );
	this->AddSubActor( &m_sprBackground );

	m_sprScrollThumbPart1.Load( THEME->GetPathTo("Graphics","select music scrollbar 2x2") );
	m_sprScrollThumbPart1.StopAnimating();
	m_sprScrollThumbPart1.SetState( 3 );
	this->AddSubActor( &m_sprScrollThumbPart1 );

	m_sprScrollThumbPart2.Load( THEME->GetPathTo("Graphics","select music scrollbar 2x2") );
	m_sprScrollThumbPart2.StopAnimating();
	m_sprScrollThumbPart2.SetState( 3 );
	this->AddSubActor( &m_sprScrollThumbPart2 );

	m_sprTopButton.Load( THEME->GetPathTo("Graphics","select music scrollbar 2x2") );
	m_sprTopButton.StopAnimating();
	m_sprTopButton.SetState( 0 );
	this->AddSubActor( &m_sprTopButton );

	m_sprBottomButton.Load( THEME->GetPathTo("Graphics","select music scrollbar 2x2") );
	m_sprBottomButton.StopAnimating();
	m_sprBottomButton.SetState( 2 );
	this->AddSubActor( &m_sprBottomButton );

	SetBarHeight( 100 );
}

void ScrollBar::SetBarHeight( int iHeight )
{
	m_iBarHeight = iHeight;
	m_sprBackground.SetZoomY( m_iBarHeight/m_sprBackground.GetUnzoomedHeight() );
	m_sprTopButton.SetY( -m_iBarHeight/2.0f );
	m_sprBottomButton.SetY( +m_iBarHeight/2.0f );
	m_sprScrollThumbPart1.SetY( 0 );
	m_sprScrollThumbPart2.SetY( 0 );
}

void ScrollBar::SetPercentage( float fStartPercent, float fEndPercent )
{
	const int k_iBarContentHeight = m_iBarHeight - int( m_sprTopButton.GetUnzoomedHeight()/2 + m_sprBottomButton.GetUnzoomedHeight()/2 );

	// make sure the percent numbers are between 0 and 1
	fStartPercent	= fmodf( fStartPercent+1, 1 );
	fEndPercent		= fmodf( fEndPercent+1, 1 );

	int iPart1TopY, iPart1BottomY, iPart2TopY, iPart2BottomY;

	if( fStartPercent < fEndPercent )	// we only need to one 1 thumb part
	{
		iPart1TopY		= (int)SCALE( fStartPercent,0.0f, 1.0f, -k_iBarContentHeight/2.0f, +k_iBarContentHeight/2.0f ); 
		iPart1BottomY	= (int)SCALE( fEndPercent,  0.0f, 1.0f, -k_iBarContentHeight/2.0f, +k_iBarContentHeight/2.0f ); 
		iPart2TopY		= -1; 
		iPart2BottomY	= -1; 
	}
	else	// we need two thumb parts
	{
		iPart1TopY		= (int)SCALE( 0.0f,			0.0f, 1.0f, -k_iBarContentHeight/2.0f, +k_iBarContentHeight/2.0f ); 
		iPart1BottomY	= (int)SCALE( fEndPercent,	0.0f, 1.0f, -k_iBarContentHeight/2.0f, +k_iBarContentHeight/2.0f ); 
		iPart2TopY		= (int)SCALE( fStartPercent,0.0f, 1.0f, -k_iBarContentHeight/2.0f, +k_iBarContentHeight/2.0f ); 
		iPart2BottomY	= (int)SCALE( 1.0f,			0.0f, 1.0f, -k_iBarContentHeight/2.0f, +k_iBarContentHeight/2.0f ); 
	}

		
	m_sprScrollThumbPart1.StretchTo( CRect(
		(int)-m_sprScrollThumbPart1.GetUnzoomedWidth()/2,
		iPart1TopY,
		(int)+m_sprScrollThumbPart1.GetUnzoomedWidth()/2,
		iPart1BottomY
		) );

	if( iPart2TopY != -1 )
	{
		m_sprScrollThumbPart2.StretchTo( CRect(
			(int)-m_sprScrollThumbPart2.GetUnzoomedWidth()/2,
			iPart2TopY,
			(int)+m_sprScrollThumbPart2.GetUnzoomedWidth()/2,
			iPart2BottomY
			) );
	}
	else
	{
		m_sprScrollThumbPart2.SetZoomY( 0 );
	}
}
