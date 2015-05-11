// MODULE: AP3Exception implementation
//;
// ===========================================================
// File Info:
//
// $Author: daellenj $
// $Date: 2014/08/23 21:47:46 $
// $Revision: 1.21 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 01/07/03 - R3 - Hugp
//
// Major changes for HP-UX/Linux implementation. Added this header.
//
// -----------------------------------------------------------

// @@DatatypeInclude
#include "D3Types.h"
#include <fstream>
#include <assert.h>
#include <stdio.h>
#include <stdarg.h>
// @@End
#include "Exception.h"
#include "D3Funcs.h"
#include "ODBCDatabase.h"
#include "OTLDatabase.h"
#include "XMLImporterExporter.h"
// @@Includes

namespace D3
{
	const char*	szD3_EXCEPTION_HANDLING_ERROR="Error occurred while reporting exception.";

	using namespace std;

	//============================================================================
	//
	// ExceptionContext implementation
	//

	ExceptionContextPtr	ExceptionContext::M_pDefaultContext = NULL;


	ExceptionContext::ExceptionContext(ExceptionSeverity eLogLevel)
		: m_eLogLevel(eLogLevel), m_postream(NULL), m_pfnServiceLog(NULL)
	{
		if (!M_pDefaultContext) M_pDefaultContext = this;
	}



	ExceptionContext::ExceptionContext(const std::string & strLogFileName, ExceptionSeverity eLogLevel)
		: m_strLogFileName(strLogFileName), m_eLogLevel(eLogLevel), m_postream(NULL), m_pfnServiceLog(NULL)
	{
		if (!M_pDefaultContext) M_pDefaultContext = this;
	}



	ExceptionContext::ExceptionContext(std::ostream* postream, ExceptionSeverity eLogLevel)
		: m_eLogLevel(eLogLevel), m_postream(postream), m_pfnServiceLog(NULL)
	{
		if (!M_pDefaultContext) M_pDefaultContext = this;
	}



	ExceptionContext::ExceptionContext(PFNSERVICELOG	pfnServiceLog, ExceptionSeverity eLogLevel)
		: m_eLogLevel(eLogLevel), m_postream(NULL), m_pfnServiceLog(pfnServiceLog)
	{
		if (!M_pDefaultContext) M_pDefaultContext = this;
	}



	ExceptionContext::~ExceptionContext()
	{
		if (this == M_pDefaultContext)
			M_pDefaultContext = NULL;
	}



	std::string ReportDiagnosticX(const char * pszFileName, int iLineNo, const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (!DFLT_EC_CTX || DFLT_EC_CTX->GetLogLevel() > Exception_diagnostic)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		DFLT_EC_CTX->VWriteMessage(pszFileName, iLineNo, Exception_diagnostic, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ReportDiagnostic(const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (!DFLT_EC_CTX || DFLT_EC_CTX->GetLogLevel() > Exception_diagnostic)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = DFLT_EC_CTX->VWriteMessage(0, 0, Exception_diagnostic, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ExceptionContext::ReportDiagnosticX(const char * pszFileName, int iLineNo, const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (GetLogLevel() > Exception_diagnostic)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = VWriteMessage(pszFileName, iLineNo, Exception_diagnostic, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ExceptionContext::ReportDiagnostic(const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (GetLogLevel() > Exception_diagnostic)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = VWriteMessage(0, 0, Exception_diagnostic, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ReportInfoX(const char * pszFileName, int iLineNo, const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (!DFLT_EC_CTX || DFLT_EC_CTX->GetLogLevel() > Exception_info)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = DFLT_EC_CTX->VWriteMessage(pszFileName, iLineNo, Exception_info, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ReportInfo(const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (!DFLT_EC_CTX || DFLT_EC_CTX->GetLogLevel() > Exception_info)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = DFLT_EC_CTX->VWriteMessage(0, 0, Exception_info, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ExceptionContext::ReportInfoX(const char * pszFileName, int iLineNo, const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (GetLogLevel() > Exception_info)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = VWriteMessage(pszFileName, iLineNo, Exception_info, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ExceptionContext::ReportInfo(const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (GetLogLevel() > Exception_info)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = VWriteMessage(0, 0, Exception_info, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ReportWarningX(const char * pszFileName, int iLineNo, const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (!DFLT_EC_CTX || DFLT_EC_CTX->GetLogLevel() > Exception_warning)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = DFLT_EC_CTX->VWriteMessage(pszFileName, iLineNo, Exception_warning, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ReportWarning(const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (!DFLT_EC_CTX || DFLT_EC_CTX->GetLogLevel() > Exception_warning)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = DFLT_EC_CTX->VWriteMessage(0, 0, Exception_warning, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ExceptionContext::ReportWarningX(const char * pszFileName, int iLineNo, const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (GetLogLevel() > Exception_warning)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = VWriteMessage(pszFileName, iLineNo, Exception_warning, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ExceptionContext::ReportWarning(const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (GetLogLevel() > Exception_warning)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = VWriteMessage(0, 0, Exception_warning, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ReportErrorX(const char * pszFileName, int iLineNo, const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (!DFLT_EC_CTX || DFLT_EC_CTX->GetLogLevel() > Exception_error)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = DFLT_EC_CTX->VWriteMessage(pszFileName, iLineNo, Exception_error, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ReportError(const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (!DFLT_EC_CTX || DFLT_EC_CTX->GetLogLevel() > Exception_error)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = DFLT_EC_CTX->VWriteMessage(0, 0, Exception_error, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ExceptionContext::ReportErrorX(const char * pszFileName, int iLineNo, const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (GetLogLevel() > Exception_error)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = VWriteMessage(pszFileName, iLineNo, Exception_error, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ExceptionContext::ReportError(const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (GetLogLevel() > Exception_error)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = VWriteMessage(0, 0, Exception_error, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ReportFatalX(const char * pszFileName, int iLineNo, const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (!DFLT_EC_CTX || DFLT_EC_CTX->GetLogLevel() > Exception_fatal)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = DFLT_EC_CTX->VWriteMessage(pszFileName, iLineNo, Exception_fatal, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ReportFatal(const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (!DFLT_EC_CTX || DFLT_EC_CTX->GetLogLevel() > Exception_fatal)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = DFLT_EC_CTX->VWriteMessage(0, 0, Exception_fatal, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ExceptionContext::ReportFatalX(const char * pszFileName, int iLineNo, const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (GetLogLevel() > Exception_fatal)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = VWriteMessage(pszFileName, iLineNo, Exception_fatal, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ExceptionContext::ReportFatal(const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (GetLogLevel() > Exception_fatal)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = VWriteMessage(0, 0, Exception_fatal, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ExceptionContext::WriteMessage(const char * pszFileName, int iLineNo, ExceptionSeverity eSeverity, const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		try
		{
			if (eSeverity < m_eLogLevel)
				return strResult;

			va_list vArgs;
			va_start(vArgs, pFormat);
			strResult = VWriteMessage(pszFileName, iLineNo, eSeverity, pFormat, vArgs);
			va_end(vArgs);
		}
		catch (...) {}

		return strResult;
	}



	std::string GenericExceptionHandlerX(const char * pszFileName, int iLineNo, const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (!DFLT_EC_CTX || DFLT_EC_CTX->GetLogLevel() > Exception_error)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = DFLT_EC_CTX->VGenericExceptionHandler(pszFileName, iLineNo, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string GenericExceptionHandler(const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		if (!DFLT_EC_CTX || DFLT_EC_CTX->GetLogLevel() > Exception_error)
			return strResult;

		va_list	vArgs;
		va_start(vArgs, pFormat);
		strResult = DFLT_EC_CTX->VGenericExceptionHandler(0, 0, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ShortGenericExceptionHandler()
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		try
		{
			throw;
		}
		catch(const D3::Exception& exD3)
		{
			strResult = exD3.AsString();
		}
#ifdef APAL_SUPPORT_ODBC
		catch(const odbc::SQLException& exODBC)
		{
			strResult = exODBC.what();
		}
#endif /* APAL_SUPPORT_ODBC */
#ifdef APAL_SUPPORT_OTL
		catch(const otl_exception& exOTL)
		{
			strResult = (const char*) exOTL.msg;
		}
#endif /* APAL_SUPPORT_OTL */
		catch(const std::bad_alloc& exBA)
		{
			strResult = exBA.what();
		}
		catch(const CSAXException& exSAX)
		{
			strResult = exSAX.what();
		}
		catch(const std::exception& exSTD)
		{
			strResult = exSTD.what();
		}
		catch(const std::string& exStr)
		{
			strResult = exStr;
		}
		catch(const char* psz)
		{
			strResult = psz;
		}
		catch(...)
		{
			strResult = "Unknown exception";
		}

		return strResult;
	}



	std::string ExceptionContext::GenericExceptionHandler(const char * pszFileName, int iLineNo, const char * pFormat, ...)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;

		va_list vArgs;
		va_start(vArgs, pFormat);
		strResult = VGenericExceptionHandler(pszFileName, iLineNo, pFormat, vArgs);
		va_end(vArgs);

		return strResult;
	}



	std::string ExceptionContext::VGenericExceptionHandler(const char * pszFileName, int FileLine, const char * pFormat, va_list vArgs)
	{
		std::string strResult = szD3_EXCEPTION_HANDLING_ERROR;
		char	xtramsg[D3_MAX_ERRMSG_SIZE];


		xtramsg[0] = '\0';
		vsnprintf(xtramsg, D3_MAX_ERRMSG_SIZE, pFormat, vArgs);

		if (!xtramsg[0])
			strncpy(xtramsg, "GenericExceptionHandler(): Error caught.", D3_MAX_ERRMSG_SIZE);

		try
		{
			throw;
		}
		catch(const D3::Exception& exD3)
		{
			strResult = WriteMessage(pszFileName, FileLine, Exception_error, "%s D3 exception: %s", xtramsg, exD3.AsString().c_str());
		}
#ifdef APAL_SUPPORT_ODBC
		catch(const odbc::SQLException& exODBC)
		{
			strResult = WriteMessage(pszFileName, FileLine, Exception_error, "%s odbc++ exception: %s", xtramsg, exODBC.what());
		}
#endif /* APAL_SUPPORT_ODBC */
#ifdef APAL_SUPPORT_OTL
		catch(const otl_exception& exOTL)
		{
			strResult = WriteMessage(pszFileName, FileLine, Exception_error, "%s otl exception: %s", xtramsg, exOTL.msg);
		}
#endif /* APAL_SUPPORT_OTL */
		catch(const std::bad_alloc& exBA)
		{
			strResult = WriteMessage(pszFileName, FileLine, Exception_error, "%s std::bad_alloc exception: %s", xtramsg, exBA.what());
		}
		catch(const CSAXException& exSAX)
		{
			strResult = WriteMessage(pszFileName, FileLine, Exception_error, "%s CSAXException: %s", xtramsg, exSAX.what());
		}
		catch(const std::exception& exSTD)
		{
			strResult = WriteMessage(pszFileName, FileLine, Exception_error, "%s std::exception: %s", xtramsg, exSTD.what());
		}
		catch(const std::string& exStr)
		{
			strResult = WriteMessage(pszFileName, FileLine, Exception_error, "%s %s", xtramsg, exStr.c_str());
		}
		catch(const char* psz)
		{
			strResult = WriteMessage(pszFileName, FileLine, Exception_error, "%s %s", xtramsg, psz);
		}
		catch(...)
		{
			strResult = WriteMessage(pszFileName, FileLine, Exception_error, "%s Unknown exception", xtramsg);
		}

		return strResult;
	}



	std::string ExceptionContext::VWriteMessage(const char * pszFileName, int iLineNo, ExceptionSeverity eSeverity, const char * pFormat, va_list vArgs)
	{
		if (eSeverity < m_eLogLevel)
			return "";

		Exception				e(this, pszFileName, iLineNo, eSeverity, pFormat, vArgs);

		return LogError(&e);
	}



	std::string ExceptionContext::LogError(ExceptionPtr pE)
	{
		if (pE->GetSeverity() < m_eLogLevel)
			return "";

		std::string strOutput = pE->AsString();

		return WriteToLog(strOutput, pE->GetSeverity());
	}



	std::string ExceptionContext::MessageTerminator()
	{
		std::string	strTerminator;

		if (!m_strLogFileName.empty())
			strTerminator = "\n";

		return strTerminator;
	}



	std::string ExceptionContext::WriteToLog(const std::string & strError, ExceptionSeverity eSeverity)
	{
		boost::mutex::scoped_lock		lk(m_mtxExclusive);

		if (eSeverity >= m_eLogLevel)
		{
			if (m_postream)
			{
				(*m_postream) << strError.c_str() << endl;
			}
			else if (m_pfnServiceLog)
			{
				// We may need to substitute "%" with "%%"
				if (strError.find('%'))
				{
					(m_pfnServiceLog)(eSeverity, ReplaceAll(strError, "%", "%%").c_str());
				}
				else
				{
					(m_pfnServiceLog)(eSeverity, strError.c_str());
				}
			}
			else if (!m_strLogFileName.empty())
			{
				if (!strError.empty())
				{
					FILE*		f;

					f = fopen(m_strLogFileName.c_str(), "a+");

					if (f)
					{
						fwrite(strError.c_str(), 1, strError.size(), f);
						fwrite("\n", 1, 1, f);
						fclose(f);
					}
					else
					{
						std::cout << "ERROR: ExceptionContext::WriteToLog(): Cannot open/create log file. " << m_strLogFileName << std::endl;
						std::cout << strError << std::endl;
					}
				}
			}
			else
			{
				std::cout << strError << std::endl;
			}

			m_strLastError = strError;
		}

		// Make sure messages are also logged to the global log
		//
		if (M_pDefaultContext && this != M_pDefaultContext)
			M_pDefaultContext->WriteToLog(strError, eSeverity);

		return m_strLastError;
	}



	//===========================================================================
	//
	// Exception implementation
	//

	Exception::Exception(ExceptionContextPtr pEC, const char * pszFileName, int iLineNo, ExceptionSeverity eSeverity, const char * pFormat, ...)
	 : m_pEC(pEC), m_eSeverity(eSeverity), m_pszFileName(pszFileName), m_iLineNo(iLineNo)
	{
		char		szBuff[40];

#ifdef AP3_OS_TARGET_WIN32
		_snprintf(szBuff,40," [% 5u] ", GetCurrentThreadID());
#else
		snprintf(szBuff,40," [% 5u] ", GetCurrentThreadID());
#endif

		m_strDT  = SystemDateTimeAsStandardString();
		m_strDT += szBuff;

		if (!m_pEC)
			m_pEC = ExceptionContext::GetDefaultExceptionContext();

		va_list vArgs;
		va_start(vArgs, pFormat);
		VAddMessage(pFormat, vArgs);
		va_end(vArgs);
	}



	Exception::Exception(ExceptionContextPtr pEC, const char * pszFileName, int iLineNo, ExceptionSeverity eSeverity, const char * pFormat, va_list vArgs)
	 : m_pEC(pEC), m_eSeverity(eSeverity), m_pszFileName(pszFileName), m_iLineNo(iLineNo)
	{
		char		szBuff[40];

#ifdef AP3_OS_TARGET_WIN32
		_snprintf(szBuff,40," [% 5u] ", GetCurrentThreadID());
#else
		snprintf(szBuff,40," [% 5u] ", GetCurrentThreadID());
#endif

		m_strDT  = SystemDateTimeAsStandardString();
		m_strDT += szBuff;

		if (!m_pEC)
			m_pEC = ExceptionContext::GetDefaultExceptionContext();

		VAddMessage(pFormat, vArgs);
	}



	Exception::Exception(const char * pszFileName, int iLineNo, ExceptionSeverity eSeverity, const char * pFormat, ...)
	 : m_pEC(ExceptionContext::GetDefaultExceptionContext()), m_eSeverity(eSeverity), m_pszFileName(pszFileName), m_iLineNo(iLineNo)
	{
		char		szBuff[40];

#ifdef AP3_OS_TARGET_WIN32
		_snprintf(szBuff,40," [% 5u] ", GetCurrentThreadID());
#else
		snprintf(szBuff,40," [% 5u] ", GetCurrentThreadID());
#endif

		m_strDT  = SystemDateTimeAsStandardString();
		m_strDT += szBuff;

		va_list vArgs;
		va_start(vArgs, pFormat);
		VAddMessage(pFormat, vArgs);
		va_end(vArgs);
	}



	Exception::Exception(const char * pszFileName, int iLineNo, ExceptionSeverity eSeverity, const char * pFormat, va_list vArgs)
	 : m_pEC(ExceptionContext::GetDefaultExceptionContext()), m_eSeverity(eSeverity), m_pszFileName(pszFileName), m_iLineNo(iLineNo)
	{
		char		szBuff[40];

#ifdef AP3_OS_TARGET_WIN32
		_snprintf(szBuff,40," [% 5u] ", GetCurrentThreadID());
#else
		snprintf(szBuff,40," [% 5u] ", GetCurrentThreadID());
#endif

		m_strDT  = SystemDateTimeAsStandardString();
		m_strDT += szBuff;

		VAddMessage(pFormat, vArgs);
	}



	Exception::Exception(const char * pFormat, ...)
	 : m_pEC(ExceptionContext::GetDefaultExceptionContext()), m_eSeverity(Exception_error), m_pszFileName(NULL), m_iLineNo(0)
	{
		char		szBuff[40];

#ifdef AP3_OS_TARGET_WIN32
		_snprintf(szBuff,40," [% 5u] ", GetCurrentThreadID());
#else
		snprintf(szBuff,40," [% 5u] ", GetCurrentThreadID());
#endif

		m_strDT  = SystemDateTimeAsStandardString();
		m_strDT += szBuff;

		va_list vArgs;
		va_start(vArgs, pFormat);
		VAddMessage(pFormat, vArgs);
		va_end(vArgs);
	}



	void Exception::AddMessage(const char * pFormat, ...)
	{
		va_list vArgs;
		va_start(vArgs, pFormat);
		VAddMessage(pFormat, vArgs);
		va_end(vArgs);
	}



	void Exception::VAddMessage(const char * pFormat, va_list vArgs)
	{
		char	buffer[D3_MAX_ERRMSG_SIZE];
		buffer[0] = '\0';
		vsnprintf(buffer, D3_MAX_ERRMSG_SIZE, pFormat, vArgs);
		m_listMessages.push_front(buffer);
	}



	std::string Exception::AsString() const
	{
		StringList::const_iterator	itrMessages;
		std::string									strPrefix;
		std::string									strLocation;
		std::string									strOutput;
		std::string									strMessage;
		char												buffer[32];


		strPrefix = m_strDT;

		switch (m_eSeverity)
		{
			case Exception_diagnostic:
				strPrefix += "Diagnostic.: ";
				break;

			case Exception_info:
				strPrefix += "Info.......: ";
				break;

			case Exception_warning:
				strPrefix += "Warning....: ";
				break;

			case Exception_error:
				strPrefix += "Error......: ";
				break;

			case Exception_fatal:
				strPrefix += "Fatal Error: ";
				break;

			default:
				strPrefix += "Unknown....: ";
		}

		if (m_pszFileName && m_iLineNo > 0)
		{
			strLocation  = "[";
			strLocation += m_pszFileName;
			strLocation += "/";
			ltoa(m_iLineNo, buffer, 10);
			strLocation += buffer;
			strLocation += "] ";
		}

		itrMessages = m_listMessages.begin();

		while (itrMessages != m_listMessages.end())
		{
			strMessage = *itrMessages;
			itrMessages++;

			if (!strOutput.empty())
				strOutput += '\n';

			strOutput += strPrefix;

			if (itrMessages == m_listMessages.end())
				strOutput += strLocation;

			strOutput += strMessage;

			strPrefix += "  >>";
		}

		return strOutput;
	}



	std::string	Exception::LogError()
	{
		if (m_pEC)
			return m_pEC->LogError(this);

		assert(ExceptionContext::GetDefaultExceptionContext());
		return ExceptionContext::GetDefaultExceptionContext()->LogError(this);
	}


} /* end namespace apal3 */

