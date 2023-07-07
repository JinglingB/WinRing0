//-----------------------------------------------------------------------------
//     Author : hiyohiyo
//       Mail : hiyohiyo@crystalmark.info
//        Web : http://openlibsys.org/
//    License : The modified BSD license
//
//                          Copyright 2007-2020 OpenLibSys.org. All rights reserved.
//-----------------------------------------------------------------------------

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols

// COlsSampleApp:
// See OlsSample.cpp for the implementation of this class
//

class COlsSampleApp : public CWinApp
{
public:
	COlsSampleApp();
#if defined(_MT) && defined(_DLL)
	HMODULE m_hOpenLibSys;
#endif
// Overrides
	public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern COlsSampleApp theApp;