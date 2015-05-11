#ifndef INC_EXCEPTION_H
#define INC_EXCEPTION_H

// MODULE: Exception header
//;
// ===========================================================
// File Info:
//
// $Author: pete $
// $Date: 2014/08/14 12:43:52 $
// $Revision: 1.12 $
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
// @@End
// @@Includes
// @@End

//  exception handler
//
/*
This exception handler works as follows:

When Exception are created, they are created within an ExceptionContext. The exception context
determines how exceptions are handled. The context has features to automatically log exceptions to
a file, what level of errors to log and also keeps track of the last exception that was thrown.

An ExceptionContext can also log simple informative messages directly without a need to create an
exception object.

There is one default context in the system which has relevance if an Exception object is created
without passing in an ExceptionContext.

Exceptions are typically thrown within a C++ try/catch block:

	try
	{
		.
		.
		.
		if (bError)
			throw Exception(	__FILE__,
													__LINE__,
													Exception_error,
													"Executing SQL statement '%s' returned SQL error '%s'",
													strSQLQuery.c_str(),
													strSQLError.c_str());
		.
		.
		.
	}
	catch (Exception e)
	{
		// can add futher messages to existing exception here
		//
		e.AddMessage("Unable to update product '%s'", strProductID.c_str());
		rethrow;
	}

In the above example, the throw expression in the try block creates an exception object
using the default context. The default context is created at startup and logs messages
(in mode all messages will be logged and in release mode only exceptions classified as
errors or fatal errors will be logged) to the DEFAULT_LOGFILENAME (see
project_parameter_projectname.h).

The catch handler adds additional information to the existing exception and rethrows the
exception. The outermost catch handler is typically an APP3COM interface method. The actual
exceptions are logged in reverse order at the time the exception object is being destroyed.

It is also possible to simply output informative messages and warnings directly to the
exception context without the need to create an Exception object as in the following
example:

	if (pPallet)
		ExceptionContext::GetDefaultExceptionContext()->WriteMessage(__FILE__,
																																		__LINE__,
																																		Exception_info,
																																		"Order %s, no pallet specified, using default pallet %s!",
																																		strOrderID.c_str(),
																																		pDefaultPallet->GetID().c_str());

In some cases, exceptions may have to be written to a specific log file. This can
be accomplished by creating a specific ExceptionContext:

e.g.:

	ExceptionContext		EC("c:\\temp\\mylogfile.log", Exception_info);

	try
	{
		.
		.
		.
		if (iError)
			throw Exception(	&EC,
													__FILE__,
													__LINE__,
													Exception_error,
													"Error %l occured!",
													iError);
		.
		.
		.
	}
	catch (Exception e)
	{
		// can add futher messages to existing exception here
		//
		e.AddMessage("Unable to update product '%s'", strProductID.c_str());
		e.LogError();
	}

Note: You must take care when using specific Exception contexts that the context is not
destroyed before the exception within the context is destroyed. For example, in the sample
code above, moving the line "ExceptionContext EC(...)" inside the try block would cause
a GPF because EC would be destroyed before the exception handler is invoked.

*/
#include <list>
#include <string>
#include <stdarg.h>
#include <boost/thread/thread.hpp>		// Use mutex to serialise output to context

#define D3_MAX_ERRMSG_SIZE		65536

namespace D3
{
	enum ExceptionSeverity
	{
		Exception_diagnostic,			// default
		Exception_info,						// default
		Exception_warning,
		Exception_error,
		Exception_fatal
	};

	#ifdef _DEBUG
		#define	Exception_defaultlevel Exception_diagnostic
	#else
		#define	Exception_defaultlevel Exception_info
	#endif

	typedef	void (*PFNSERVICELOG)(ExceptionSeverity, const char *);


	class ExceptionContext;
	class Exception;

	typedef ExceptionContext*	ExceptionContextPtr;
	typedef Exception*				ExceptionPtr;

	class ExceptionContext
	{
		friend class Exception;

		friend D3_API	std::string ReportDiagnosticX(const char * pszFileName, int iLineNo, const char * pFormat, ...);
		friend D3_API	std::string ReportInfoX(const char * pszFileName, int iLineNo, const char * pFormat, ...);
		friend D3_API	std::string ReportWarningX(const char * pszFileName, int iLineNo, const char * pFormat, ...);
		friend D3_API	std::string ReportErrorX(const char * pszFileName, int iLineNo, const char * pFormat, ...);
		friend D3_API	std::string ReportFatalX(const char * pszFileName, int iLineNo, const char * pFormat, ...);

		friend D3_API	std::string ReportDiagnostic(const char * pFormat, ...);
		friend D3_API	std::string ReportInfo(const char * pFormat, ...);
		friend D3_API	std::string ReportWarning(const char * pFormat, ...);
		friend D3_API	std::string ReportError(const char * pFormat, ...);
		friend D3_API	std::string ReportFatal(const char * pFormat, ...);

		friend D3_API	std::string GenericExceptionHandlerX(const char * pszFileName, int FileLine, const char * pFormat, ...);
		friend D3_API	std::string GenericExceptionHandler(const char * pFormat, ...);

		protected:
			static ExceptionContextPtr		M_pDefaultContext;

			std::string										m_strLogFileName;
			std::ostream*									m_postream;				// if this is a valid pointer, output is directed to this stream (use SetLogFile() method)
			PFNSERVICELOG									m_pfnServiceLog;
			ExceptionSeverity							m_eLogLevel;
			std::string										m_strLastError;
			boost::mutex									m_mtxExclusive;		// Blocks concurrent writing to log

		public:
			D3_API ExceptionContext(ExceptionSeverity eLogLevel = Exception_defaultlevel);
			D3_API ExceptionContext(const std::string & strLogFileName, ExceptionSeverity eLogLevel = Exception_defaultlevel);
			D3_API ExceptionContext(std::ostream* postream, ExceptionSeverity eLogLevel = Exception_defaultlevel);
			D3_API ExceptionContext(PFNSERVICELOG	pfnServiceLog, ExceptionSeverity eLogLevel = Exception_defaultlevel);
			D3_API ~ExceptionContext();

			D3_API static	ExceptionContextPtr	GetDefaultExceptionContext()					{ return M_pDefaultContext; }

			D3_API void									SetLogLevel(ExceptionSeverity eLogLevel)		{ m_eLogLevel = eLogLevel < Exception_fatal ? eLogLevel : Exception_error; };
			D3_API ExceptionSeverity		GetLogLevel()																{ return m_eLogLevel; };

			D3_API const std::string		GetLastError()															{ std::string strLastError(m_strLastError); m_strLastError=""; return strLastError; };
			D3_API void									SetLastError(const std::string& strError)		{ m_strLastError=strError; };
			D3_API const std::string &	GetLogFile()																{ return m_strLogFileName; };
			D3_API void									SetAsDefault()															{ M_pDefaultContext = this; };

			D3_API std::string					ReportDiagnosticX(const char * pszFileName, int iLineNo, const char * pFormat, ...);
			D3_API std::string					ReportInfoX(const char * pszFileName, int iLineNo, const char * pFormat, ...);
			D3_API std::string					ReportWarningX(const char * pszFileName, int iLineNo, const char * pFormat, ...);
			D3_API std::string					ReportErrorX(const char * pszFileName, int iLineNo, const char * pFormat, ...);
			D3_API std::string					ReportFatalX(const char * pszFileName, int iLineNo, const char * pFormat, ...);

			D3_API std::string					ReportDiagnostic(const char * pFormat, ...);
			D3_API std::string					ReportInfo(const char * pFormat, ...);
			D3_API std::string					ReportWarning(const char * pFormat, ...);
			D3_API std::string					ReportError(const char * pFormat, ...);
			D3_API std::string					ReportFatal(const char * pFormat, ...);

			D3_API std::string					WriteMessage(const char * pszFileName, int iLineNo, ExceptionSeverity eSeverity, const char * pFormat, ...);
			D3_API std::string					GenericExceptionHandler(const char * pszFileName, int FileLine, const char * pFormat, ...);

		protected:
			std::string						VGenericExceptionHandler(const char * pszFileName, int FileLine, const char * pFormat, va_list args);
			std::string						VWriteMessage(const char * pszFileName, int iLineNo, ExceptionSeverity eSeverity, const char * pFormat, va_list args);
			std::string						LogError(ExceptionPtr pE);
			std::string						WriteToLog(const std::string & strError, ExceptionSeverity eSeverity);
			std::string						MessageTerminator();

	};



	class Exception
	{
		friend class ExceptionContext;

		protected:
			typedef	std::list<std::string>	StringList;

			ExceptionContextPtr	m_pEC;						// Exception context
			StringList					m_listMessages;		// List of strings describing the error
			ExceptionSeverity		m_eSeverity;			// ExceptionSeverity (see enum above)
			const char *				m_pszFileName;	  // Name of the file throwing exception
			int									m_iLineNo;				// Line number inside module
			std::string					m_strDT;					// date/time created


			// Not for outsiders
			explicit Exception(ExceptionContextPtr pEC, const char * pszFileName, int iLineNo, ExceptionSeverity eSeverity, const char * pFormat, va_list vArgs);
			explicit Exception(const char * pszFileName, int iLineNo, ExceptionSeverity eSeverity, const char * pFormat, va_list vArgs);

		public:
			D3_API	explicit Exception(ExceptionContextPtr pEC, const char * pszFileName, int iLineNo, ExceptionSeverity eSeverity, const char * pFormat, ...);
			D3_API	explicit Exception(const char * pszFileName, int iLineNo, ExceptionSeverity eSeverity, const char * pFormat, ...);
			D3_API	explicit Exception(const char * pFormat, ...);

			D3_API	void										AddMessage(const char * pFormat, ...);

			D3_API	ExceptionSeverity				GetSeverity()																		{ return m_eSeverity; }
			D3_API	void										SetSeverity(ExceptionSeverity eSeverity)				{ m_eSeverity = eSeverity; }

			D3_API	bool										IsDiagnostic(){ return m_eSeverity == Exception_diagnostic; };
			D3_API	bool										IsInfo()			{ return m_eSeverity == Exception_info; };
			D3_API	bool										IsWarning()		{ return m_eSeverity == Exception_warning; };
			D3_API	bool										IsError()			{ return m_eSeverity == Exception_error; };
			D3_API	bool										IsFatal()			{ return m_eSeverity == Exception_fatal; };

			D3_API	std::string							AsString() const;

			D3_API	std::string							LogError();

			// deprecated (only for backwards compatibility)
			D3_API	static std::string			GenericExceptionHandler(const char * pszFileName, int iLineNo, ExceptionSeverity Severity = Exception_error, ExceptionContextPtr pExceptionContext = ExceptionContext::GetDefaultExceptionContext())
			{
				return pExceptionContext->GenericExceptionHandler(pszFileName, iLineNo, "");
			}

		protected:
			void										VAddMessage(const char * pFormat, va_list vArgs);
	};

	#define DFLT_EC_CTX					ExceptionContext::GetDefaultExceptionContext()

	D3_API	std::string ReportDiagnosticX(const char * pszFileName, int iLineNo, const char * pFormat, ...);
	D3_API	std::string ReportInfoX(const char * pszFileName, int iLineNo, const char * pFormat, ...);
	D3_API	std::string ReportWarningX(const char * pszFileName, int iLineNo, const char * pFormat, ...);
	D3_API	std::string ReportErrorX(const char * pszFileName, int iLineNo, const char * pFormat, ...);
	D3_API	std::string ReportFatalX(const char * pszFileName, int iLineNo, const char * pFormat, ...);

	D3_API	std::string ReportDiagnostic(const char * pFormat, ...);
	D3_API	std::string ReportInfo(const char * pFormat, ...);
	D3_API	std::string ReportWarning(const char * pFormat, ...);
	D3_API	std::string ReportError(const char * pFormat, ...);
	D3_API	std::string ReportFatal(const char * pFormat, ...);

	D3_API	std::string GenericExceptionHandlerX(const char * pszFileName, int FileLine, const char * pFormat, ...);
	D3_API	std::string GenericExceptionHandler(const char * pFormat, ...);

	//! Simply returns a string describing the error
	D3_API	std::string ShortGenericExceptionHandler();

} // end namespace D3

#endif /* INC_EXCEPTION_H */
