#ifndef _OTLParams_h_
#define _OTLParams_h_

// MODULE: OTLParams header
//
// NOTE:	This module implements OTL specific defines
//				Modules requiring OTL support should include
//				this file instead of including <otlv4.h> directly.
//;
// ===========================================================
// File Info:
//
// $Author: pete $
// $Date: 2007/05/28 04:11:01 $
// $Revision: 1.1.1.1 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 28/07/04 - R1 - Hugp
//
// Created
//
// -----------------------------------------------------------

#ifdef APAL_SUPPORT_OTL			// from apal_site_config.h

#define OTL_STL							// Turn on STL features
#ifndef OTL_ANSI_CPP
#define OTL_ANSI_CPP				// Turn on ANSI C++ typecasts
#endif

#define OTL_BIND_VAR_STRICT_TYPE_CHECKING_ON

#include <otlv4.h>					// include the OTL 4 header file

#endif /* APAL_SUPPORT_OTL */

#endif /* _OTLParams_h_ */
