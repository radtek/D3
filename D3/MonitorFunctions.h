#ifndef _MonitorFunctions_h_
#define _MonitorFunctions_h_


#include <string>

#include <boost/thread/thread.hpp>
#include <boost/thread/recursive_mutex.hpp>

#include <Exception.h>
// #include "SystemFuncs.h"



namespace wofuncs
{

#ifdef _DEBUG

	//!	The MonitoredFunction class is available for debugging purpose only
	/*!	The purpose of this class is to print a message in the log whenever a function
			is entered and exited. You use objects of this class as follows:

			\code
			wofuncs::MonitorFunction mf(format, arg1, arg2, ..., argn);
			\endcode

			In _DEBUG mode, the class will print something similar to sprintf(), except
			that the output is not a buffer but the global exception context. Best practise
			is to make the object is the very first object created on the stack when the
			function you wish to monitor is entered.

			When an object of this class is created it will log the desired message with the
			additional text " - Enter Function" and when the object is destroyed it will re-
			print the same message appended with " - Exit Function".

			\note The fully composed message may not exceed 4000 bytes.
	*/
	class MonitoredFunction
	{
		protected:
			std::string		m_strMsg;					//!< The formatted message

		public:
			//!	The ctor is the only method you'll use from this class
			/*! The ctor is used similar to sprintf() except that you omit the output buffer.
					The composed message will appear in the global exception context appended with
					" - Enter Function"
			*/
			MonitoredFunction(const char * szFmt, ...)
			{
				char			szMsg[4096];
				va_list		vArgs;


				szMsg[0] = '\0';
				va_start(vArgs, szFmt);
				vsprintf(szMsg, szFmt, vArgs);
				va_end(vArgs);

				m_strMsg = szMsg;
				m_strMsg += "%s";

				D3::ReportDiagnostic(m_strMsg.c_str(), " - Enter Function");
			}



			~MonitoredFunction()
			{
				D3::ReportDiagnostic(m_strMsg.c_str(), " - Exit Function");
			}

	};

#else

	class MonitoredFunction
	{
		public:
			MonitoredFunction(const char * szFmt, ...) {}
	};

#endif



#ifndef _DEBUG
	#ifdef _TRACE_LOCKING
		#undef _TRACE_LOCKING
	#endif
#endif


#ifdef _TRACE_CONCURRENCY

	//!	The MonitoredLocker class helps debug deadlocks.
	/*!	The purpose of this class is to enable you to serialise access to functions
			through a boost::recursive_mutex. The ctor of this class takes a const char*
			and a boost::::recursive_mutex&. If you wish to serialise access to a function,
			you simply create a MonitoredLocker on the stack as follows:

			\code
			static boost::recursive_mutex  mtx;

			// Only one thread can access this function at any one time
			//
			void Activity::PerformWork()
			{
				wofuncs::MonitoredLocker		ml(mtx, "Activity::PerformWork(): Activity %s.", Name().c_str());

				// your code here
			}
			\endcode

			If this module is compiled using the _DEBUG directive in combination with the
			_TRACE_CONCURRENCY directive, the MonitoredLocker class in the code example above will
			print the following messages to the standard ExceptionContext:

			Activity::PerformWork(): Activity xxxx - Thread nnnn requests lock.
			Activity::PerformWork(): Activity xxxx - Thread nnnn acquired lock.
			Activity::PerformWork(): Activity xxxx - Thread nnnn releases lock.

			\Note This class ads very limited overhead to the locking of mutext objects if
			either _DEBUG or _DBGFREEZE is missing. If both directive are specified, the
			overhead is far more significant.

	*/
	class MonitoredLocker
	{
		protected:
			boost::recursive_mutex::scoped_lock*	m_pSL;						//!< The actual lock
			std::string														m_strMsg;					//!< The formatted message
			boost::recursive_mutex*								m_pMtx;

		public:
			//!	The ctor is the only method you'll use from this class
			/*! The ctor is used similar to sprintf() except that you omit the output buffer.
					The composed message will appear in the global exception context appended with
					" - Thread n requests lock" before the lock is actually obtained and the same appended
					with " - Thread n acquires lock!" after the lock has been obtained.
			*/
			MonitoredLocker(boost::recursive_mutex & mtx, const char * szFmt, ...)
				: m_pSL(NULL), m_pMtx(&mtx)
			{
				char			szMsg[4096];
				va_list		vArgs;


				szMsg[0] = '\0';
				va_start(vArgs, szFmt);
				vsprintf(szMsg, szFmt, vArgs);
				va_end(vArgs);

				m_strMsg = szMsg;
				m_strMsg += " - Thread %u %s lock %x.";

				D3::ReportDiagnostic(m_strMsg.c_str(), D3::GetCurrentThreadID(), "requests", m_pMtx);
				m_pSL = new boost::recursive_mutex::scoped_lock(mtx);
				D3::ReportDiagnostic(m_strMsg.c_str(), D3::GetCurrentThreadID(), "acquired", m_pMtx);
			}



			~MonitoredLocker()
			{
				D3::ReportDiagnostic(m_strMsg.c_str(), D3::GetCurrentThreadID(), "releases", m_pMtx);
				delete m_pSL;
			}

	};

#else

	class MonitoredLocker
	{
		protected:
			boost::recursive_mutex::scoped_lock	m_SL;

		public:
			MonitoredLocker(boost::recursive_mutex & mtx, const char * szFmt, ...) : m_SL(mtx) {}
	};

#endif

} // namespace wouncs

#endif  /* _MonitorFunctions_h_ */

