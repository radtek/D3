// MODULE: D3Funcs Implementation
//;
// ===========================================================
// File Info:
//
// $Author: pete $
// $Date: 2014/10/01 03:51:23 $
// $Revision: 1.18 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 24/09/02 - R1 - Hugp (created module)
//
// This module now includes a libary of c-style functions used
// by various modules in the system
//
//
// 01/07/02 - R2 - Hugp
//
// Various enhancements and changes for UNIX compatibility
//
// -----------------------------------------------------------
//
// @@DatatypeInclude
#include "D3Types.h"
// @@End
// @@Includes
#include "D3Funcs.h"
#include "D3Date.h"
#include "Exception.h"
#include <time.h>
#include <limits.h>
#include <float.h>

#include <boost/thread/thread.hpp>		// for SleepThread()
#include <boost/thread/xtime.hpp>			// for SleepThread()

// Module uses XML DOM
//
#include <DOM/DOMBuilder.h>
#include <DOM/Attr.h>
#include <DOM/Text.h>
#include <DOM/Element.h>
#include <DOM/NamedNodeMap.h>
#include <DOM/NodeList.h>
#include <DOM/Document.h>
#include <DOM/DocumentType.h>
#include <DOM/Comment.h>
#include <DOM/DOMImplementation.h>
#include <DOMWriter.h>
#include <XMLUtils.h>
#include <XMLException.h>

// @@End

namespace D3
{
	// Returns the process handle of the current process
	//
	long GetCurrentProcessID()
	{
		long		lID;


#ifdef AP3_OS_TARGET_WIN32
		lID = (long) GetCurrentProcessId();
#else
		lID = (long) getpid();
#endif

		return lID;
	}



	// Returns the thread handle of the current thread
	//
	long GetCurrentThreadID()
	{
		long		lID;


#ifdef AP3_OS_TARGET_WIN32
		lID = (long) GetCurrentThreadId();
#else
		lID = (long) pthread_self();
#endif

		return lID;
	}

	std::string SystemDateTimeAsD3String()
	{
		char		tmpbuf[36];
		time_t	ltime;
		struct	tm *now;
		std::string	strDT;


		// Get the current system date/time
		_tzset();
		::time( &ltime );
		now = ::localtime( &ltime );

		sprintf(tmpbuf, "%04u%02u%02u%02u%02u%02u", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday, now->tm_hour, now->tm_min, now->tm_sec);

		strDT = tmpbuf;

		return strDT;
	}



	std::string SystemDateTimeAsStandardString()
	{
		char		tmpbuf[36];
		time_t	ltime;
		struct	tm *now;
		std::string	strDT;


		// Get the current system date/time
		_tzset();
		::time( &ltime );
		now = ::localtime( &ltime );

		sprintf(tmpbuf, "%02u/%02u/%04u %02u:%02u:%02u", now->tm_mday, now->tm_mon + 1, now->tm_year + 1900, now->tm_hour, now->tm_min, now->tm_sec);

		strDT = tmpbuf;

		return strDT;
	}



	std::string SystemDateAsStandardString()
	{
		char		tmpbuf[36];
		time_t	ltime;
		struct	tm *now;
		std::string	strDT;


		// Get the current system date/time
		_tzset();
		::time( &ltime );

		now = ::localtime( &ltime );

		sprintf(tmpbuf, "%02u/%02u/%04u", now->tm_mday, now->tm_mon + 1, now->tm_year + 1900);

		strDT = tmpbuf;

		return strDT;
	}



	std::string SystemTimeAsStandardString()
	{
		char		tmpbuf[36];
		time_t	ltime;
		struct	tm *now;
		std::string	strDT;


		// Get the current system date/time
		_tzset();
		::time( &ltime );
		now = ::localtime( &ltime );

		sprintf(tmpbuf, "%02u:%02u:%02u", now->tm_hour, now->tm_min, now->tm_sec);

		strDT = tmpbuf;

		return strDT;
	}



	std::string AllTrim(const std::string & strIn, char cTrim)
	{
		std::string	strTemp;

		strTemp = LeftTrim(strIn, cTrim);
		strTemp = RightTrim(strTemp, cTrim);

		return strTemp;
	}



	std::string LeftTrim(const std::string & strIn, char cTrim)
	{
		std::string::size_type iStrt = strIn.find_first_not_of(cTrim);

		if (iStrt != std::string::npos)
			if (iStrt != 0)
				return strIn.substr(iStrt, strIn.size());
			else
				return strIn;

		return std::string();
	}



	std::string RightTrim(const std::string & strIn, char cTrim)
	{
		std::string::size_type iEnd = strIn.find_last_not_of(cTrim);

		if (iEnd != std::string::npos)
			if (iEnd != strIn.size() - 1)
				return strIn.substr(0, iEnd + 1);
			else
				return strIn;

		return std::string();
	}



	std::string ToLower(const std::string & strIn)
	{
		std::string				strOut = strIn;
		unsigned int			iSize = strIn.size();

		for (unsigned int i = 0; i < iSize; i++)
			strOut[i] = tolower(strIn[i]);

		return strOut;
	}



	std::string ToUpper(const std::string & strIn)
	{
		std::string				strOut = strIn;
		unsigned int			iSize = strIn.size();

		for (unsigned int i = 0; i < iSize; i++)
			strOut[i] = toupper(strIn[i]);

		return strOut;
	}



	std::string ToSentence(const std::string & strIn)
	{
		std::string		strOut = strIn;

		for (unsigned int i = 0; i < strIn.size(); i++)
		{
			if(i == 0)
				strOut[i] = toupper(strIn[i]);
			else
				strOut[i] = tolower(strIn[i]);
		}

		return strOut;
	}



	std::string ToCaps(const std::string & strIn)
	{
		std::string				strOut = strIn;
		unsigned int			iSize = strIn.size();
		char							x;
		bool							bCapitalize = true;

		for (unsigned int i = 0; i < iSize; i++)
		{
			x = strOut[i];
			if ((x >= 'A' && x <= 'Z') || (x >= 'a' && x <= 'z'))
			{
				if (bCapitalize)
					x = toupper(x);
				else
					x = tolower(x);

				strOut[i] = x;
				bCapitalize = false;
			}
			else
				bCapitalize = true;
		}

		return strOut;
	}



	std::string Escape(const std::string & strIn)
	{
		std::string				strOut;
		unsigned int			iSize = strIn.size();

		for (unsigned int i = 0; i < iSize; i++)
		{
			register unsigned char c = (unsigned char) strIn[i];

			switch (c)
			{
			case '\f':
				strOut += "\\f";
				break;
			case '\n':
				strOut += "\\n";
				break;
			case '\r':
				strOut += "\\r";
				break;
			case '\t':
				strOut += "\\t";
				break;
			default:
				strOut += c;
			}
		}

		return strOut;
	}



	std::string Unescape(const std::string & strIn)
	{
		std::string				strOut;
		register bool			bEscape = false;
		unsigned int			iSize = strIn.size();

		for (unsigned int i = 0; i < iSize; i++)
		{
			register unsigned char c = (unsigned char) strIn[i];

			if (bEscape)
			{
				switch (c)
				{
				case 'f':
					strOut += '\f';
					break;
				case 'n':
					strOut += '\n';
					break;
				case 'r':
					strOut += '\r';
					break;
				case 't':
					strOut += '\t';
					break;
				case 'b':
					strOut += ' ';
					break;
				default:
					strOut += c;
				}
				bEscape = false;
			}
			else if (c == '\\')
				bEscape = true;
			else
				strOut += c;
		}

		return strOut;
	}



	std::string & ReplaceSubstring(std::string & strIn, const std::string & strFindString, const std::string & strReplaceWith, bool bAll)
	{
		std::string::size_type	pos = 0;

		do
		{
			// Locate the substring to replace.
			pos = strIn.find(strFindString, pos);

			if (pos == string::npos)
				break;

			// Make the replacement. 
			strIn.replace(pos, strFindString.size(), strReplaceWith);

			// Advance index forward so the next iteration doesn't pick it up as well.
			pos += strReplaceWith.size();
		} while (bAll);

		return strIn;
	}



	std::string XMLEncode(const std::string & strIn)
	{
		std::string				strOut;


		for (register std::string::size_type i = 0; i < strIn.size(); i++)
		{
			register unsigned char c = (unsigned char) strIn[i];

			if (c == '\'')
				strOut += "&apos;";
			else if (c == '"')
				strOut += "&quot;";
			else if (c == '&')
				strOut += "&amp;";
			else if (c == '<')
				strOut += "&lt;";
			else if (c == '>')
				strOut += "&gt;";
			else if (c > 0x7F)
			{
				// am i really supposed to do this?
				char buff[10];
				sprintf(buff, "&#x%02x;", c);
				strOut += buff;
			}
			else if (c == '\t' || c == '\r' || c == '\n' || c >= 0x20)
				strOut += c;
		}

		return strOut;
	}



	std::string JSONEncode(const std::string & strIn)
	{
		std::string				strOut;
		char							buffer[20];


		for (register std::string::size_type i = 0; i < strIn.size(); i++)
		{
			register unsigned char c = (unsigned char) strIn[i];

			switch (c)
			{
				case '"':
					strOut += "\\\"";
					break;

				case '\\':
					strOut += "\\u005C";
					break;

				case '/':
					strOut += "\\/";
					break;

				case '\b':
					strOut += "\\b";
					break;

				case '\f':
					strOut += "\\f";
					break;

				case '\n':
					strOut += "\\n";
					break;

				case '\r':
					strOut += "\\r";
					break;

				case '\t':
					strOut += "\\t";
					break;

				default:
					if (c < 0x1F)
					{
						sprintf(buffer, "\\u%04x", c);
						strOut += buffer;
					}
					else if (c > 0xBF)
					{
						strOut += (char) 0xC3;
						strOut += (char) (c - 0x40);
					}
					else if (c > 0x7F)
					{
						strOut += (char) 0xC2;
						strOut += c;
					}
					else
					{
						strOut += c;
					}
			}
		}


		return strOut;
	}



	std::string IcifyIdentifier(const std::string& strIdentifier)
	{
		string						strNew;
		string::size_type	idx;
		char							x;
		bool							bCapitalize = false;


		if (strIdentifier.find('_') == string::npos)
			return strIdentifier;

		for ( idx = 0; idx < strIdentifier.size(); idx++)
		{
			x = strIdentifier[idx];

			if (bCapitalize)
			{
				if (x >= 'a' && x <= 'z')
					x -= ('a' - 'A');

				bCapitalize = false;
			}

			switch (x)
			{
				case ' ':
					continue;			// skip spaces

				case '_':
					bCapitalize = true;
					continue;			// Skip underlines

				default:
					strNew += x;
			}
		}

		return strNew;
	}




	std::string Pluralize(const std::string & strNoun)
	{
		char					cLastChar;
		char					c2ndLastChar;
		bool					b2ndLastIsVowel = false;
		std::string		strPluralized;


		if (strNoun.empty())
			return strPluralized;

		if (strNoun.size() > 1)
		{
			c2ndLastChar = tolower(strNoun[strNoun.size() - 2]);

			switch (c2ndLastChar)
			{
				case 'a':
				case 'e':
				case 'i':
				case 'o':
				case 'u':
					b2ndLastIsVowel = true;
			}

			cLastChar = tolower(strNoun[strNoun.size() - 1]);

			switch (cLastChar)
			{
				case 'h':
				{
					// If the last letter of the word is an 'h' and the next to last letter is either a 'c' or an 's',
					// then the plural of the word adds an 'es' to the word. (dish => dishes)
					//
					if (c2ndLastChar == 'c' || c2ndLastChar == 's')
						strPluralized = strNoun + "es";
					else
						strPluralized = strNoun + "s";

					break;
				}

				case 's':
				{
					// If the last letter of the word is an 's', then the plural of the word adds an 'es' to the word.
					// (class => classes)
					//
					strPluralized = strNoun + "es";
					break;
				}

				case 'y':
				{
					// If the last letter of the word is a 'y', then if the next to last letter of the word is a vowel,
					// then the plural adds an 's' to the word.      (day => days)
					//
					// If the last letter of the word is a 'y', and it is not preceded by a vowel, then the plural
					// replaces the 'y' withan 'ies'.  (dandy => dandies)
					//
					if (b2ndLastIsVowel)
						strPluralized = strNoun + "s";
					else
						strPluralized = strNoun.substr(0, strNoun.size() - 1) + "ies";

					break;
				}

				case 'z':
				{
					// If the last letter of the word is a 'z' that is preceded by a vowel, then the plural adds a 'zes'
					// to the word. (whiz => whizzes)
					//

					// If the last letter of the word is a 'z' that is not preceded by a vowel, then the plural adds an
					// 'es' to the word.  (buzz => buzzes)
					//
					if (b2ndLastIsVowel)
						strPluralized = strNoun + "zes";
					else
						strPluralized = strNoun + "es";

					break;
				}

				default:
				{
					// Most all other conditions add an 's' to the word to form the plural of the word.
					//
					strPluralized = strNoun + "s";
				}
			}
		}
		else
		{
			strPluralized = strNoun + "s";
		}


		return strPluralized;
	}



	// ReplaceAll returns a copy of strInput with all occurrences of strSearch changed to strReplace
	//
	std::string ReplaceAll(const std::string & strInput, const std::string & strSrch, const std::string & strRepl)
	{
		std::string								strOutput;
		std::string::size_type		iCurPos, iPrvPos = 0;


		iCurPos = strInput.find(strSrch);

		if (iCurPos == std::string::npos)
			return strInput;

		while (true)
		{
			strOutput += strInput.substr(iPrvPos, iCurPos - iPrvPos);
			strOutput += strRepl;

			iPrvPos = iCurPos + strSrch.size();

			iCurPos = strInput.find(strSrch, iPrvPos);

			if (iCurPos == std::string::npos)
			{
				strOutput += strInput.substr(iPrvPos, strInput.size() - iPrvPos);
				break;
			}
		}

		return strOutput;
	}



	// This function parses a character delimited list of Names stored in a std::string and
	// puplates a list of std::string passed in with the names found. The list of strings passed
	// in is cleared before the function commences any work. If the input std::string contains
	// empty names, the function names such names UNKNOWN suffixed with an number starting
	// with 1 which is incremeted as each empty name is found. Any leading or trailing blanks

	// of names in the input std::string will be removed.
	//
	// Usage:
	//
	// std::string			strIn = "  Name1;;Name2  ;  ;  Name4 ;;Name6;";
	// StringList	listStrings;
	//
	// DelimitedNamesToList(strIn, listStrings, ';');
	//
	// In this example, after the call to DelimitedNamesToList, the listStrings collection
	// will contain the following strings (and in exactly this order):
	//
	// {"Name1", "UNKNOWN1", "Name2", "UNKNOWN2", "Name4", "UNKNOWN3", "Name6"}
	//
	// Note: The delimiter after the last name in the input std::string is optional
	//
	void DelimitedNamesToList(const std::string & strInput, StringList & listString, char cTerminator)
	{
		std::string	strPart;
		int			i1=0, i2=0, idxFld=1;
		char		buffer[20];


		listString.clear();

		i2 = strInput.find(cTerminator);

		while(i1 < (int) strInput.size())
		{
			// Get the filed name and remove leading and trailing blanks
			//
			strPart = strInput.substr(i1, i2 > -1 ? i2 - i1 : strInput.size());
			strPart = AllTrim(strPart);

			// If we have a nameless field, give it a dummy name
			//
			if (strPart.empty() && i2 > -1)
			{
				strPart = "UNKNOWN";
				sprintf(buffer, "%i", idxFld);
				strPart += buffer;
				idxFld++;
			}

			if (!strPart.empty())
				listString.push_back(strPart);

			if (i2 == -1)
				break;

			i1 = i2 + 1;
			i2 = strInput.find(cTerminator, i1);
		}
	}

	// This function parses a character delimited list of values stored in a std::string and
	// populates a list of strings passed in with the values found. The list of strings passed
	// in is cleared before the function commences any work.
	//
	// Usage:
	//
	// std::string			strIn = "  Value1;;Value2  ;  ;  Value4 ;;Value6;";
	// StringList	listStrings;
	//
	// DelimitedValuesToList(strIn, listStrings, ';');
	//
	// In this example, after the call to DelimitedValuesToList, the listStrings collection
	// will contain the following strings (and in exactly this order):
	//
	// {"  Value1", "", "Value2", "  ", "  Value4 ", "", "Value6"}
	//
	// Note: The delimiter after the last name in the input std::string is optional
	//
	void DelimitedValuesToList(const std::string & strInput, StringList & listString, char cTerminator)
	{
		std::string		strPart;
		int						i1=0, i2=0;


		listString.clear();

		i2 = strInput.find(cTerminator);

		while(i1 < (int) strInput.size())
		{
			// Get the fields value and store it in the list
			//
			strPart = strInput.substr(i1, i2 > -1 ? i2 - i1 : strInput.size());
			listString.push_back(strPart);

			if (i2 == -1)
				break;

			i1 = i2 + 1;
			i2 = strInput.find(cTerminator, i1);
		}
	}

	int GetRandomNumber(unsigned int iMin, unsigned int iMax)
	{
		int iRand;
		srand((unsigned) ::time(NULL));
		iRand = rand();

		if (iMax < iMin)
			return iRand;

		if (iMax > 0)
			iRand %= iMax - iMin;

		iRand += iMin;

		return iRand;
	}

	void SleepThread(int iSecs)
	{
		boost::xtime xt;

		boost::xtime_get(&xt, boost::TIME_UTC_);
		xt.sec += iSecs;
		boost::thread::sleep(xt);
	}

	char * ltoa(long lIn, char * szBuffer, int)
	{
		sprintf(szBuffer, "%i", lIn);
		return szBuffer;
	}

	char * itoa(int iIn, char * szBuffer, int)
	{
		sprintf(szBuffer, "%i", iIn);
		return szBuffer;
	}

	std::string AsSQLQueryString(const std::string & strValue)
	{
		string				strTemp = "'";
		const char*		pszChar;

		for (pszChar = strValue.c_str(); *pszChar; pszChar++)
		{
			if (*pszChar == '\'')
				strTemp += "''";
			else
				strTemp += *pszChar;
		}

		strTemp += '\'';

		return strTemp;
	}


	std::string GetSystemHostName()
	{
		char										HostName[33];

		if(::gethostname(&HostName[0], 32))
		{
			// must have returned 0 on success
			string Error = "GetHostName() - WSA Error ";
			#ifdef AP3_OS_TARGET_WIN32
			Error += ::WSAGetLastError();
			#endif
		}

		return HostName;
	}

	bool IsValidHost(const std::string & HostList)
	{
		std::string::size_type	iPos, iPos2;
		std::string						HostName;

		try
		{
			if(HostList.empty())return false;

			iPos = iPos2 = 0;

			while(true)
			{
				iPos = HostList.find(";" , iPos);

				HostName = HostList.substr(iPos2, iPos - iPos2);

				if(HostName == GetSystemHostName())
					return true;

				if(iPos != std::string::npos)
					iPos++;
				iPos2 = iPos;

				if(iPos >= HostList.length())
					return false;
			}
		}
		catch(...)
		{
			return false;
		}
	}


	// ============================================================================
	// Basic type conversion (used primarily by Value and sub classes)
	//

	// To std::string conversions
	//
	void Convert(std::string & outVal, char* inVal)
	{

		outVal = inVal;
	}

	void Convert(std::string & outVal, char inVal)
	{
		char		szBuff[32];


		sprintf(szBuff, "%i", inVal);
		outVal = szBuff;
	}

	void Convert(std::string & outVal, bool inVal)
	{
		outVal = inVal ? "1" : "0";
	}

	void Convert(std::string & outVal, short inVal)
	{
		char		szBuff[32];


		sprintf(szBuff, "%i", inVal);
		outVal = szBuff;
	}

	void Convert(std::string & outVal, unsigned short inVal)
	{
		char		szBuff[32];

		sprintf(szBuff, "%u", inVal);
		outVal = szBuff;
	}

	void Convert(std::string & outVal, int inVal)
	{
		char		szBuff[32];

		sprintf(szBuff, "%i", inVal);
		outVal = szBuff;
	}

	void Convert(std::string & outVal, unsigned int inVal)
	{
		char		szBuff[32];

		sprintf(szBuff, "%u", inVal);
		outVal = szBuff;
	}

	void Convert(std::string & outVal, long inVal)
	{
		char		szBuff[32];

		sprintf(szBuff, "%i", inVal);
		outVal = szBuff;
	}

	void Convert(std::string & outVal, unsigned long inVal)
	{
		char		szBuff[32];

		sprintf(szBuff, "%u", inVal);
		outVal = szBuff;
	}

	void Convert(std::string & outVal, float inVal)
	{
		char		szBuff[64];

		sprintf(szBuff, "%f", inVal);
		outVal = szBuff;
	}

	void Convert(std::string & outVal, double inVal)
	{
		char		szBuff[64];

		sprintf(szBuff, "%f", inVal);
		outVal = szBuff;
	}

	void Convert(std::string & outVal, const D3Date & inVal)
	{
		outVal = inVal.AsISOString();
	}



	// To char* conversions
	//
	void Convert(char* outVal, const std::string & inVal, unsigned int outSize)
	{
		if (outVal == NULL)
			throw Exception(__FILE__, __LINE__, Exception_error, "Convert(char*, std::string, unsigned int) output argument is NULL.");

		strncpy(outVal, inVal.c_str(), outSize);
	}

	void Convert(char* outVal, char inVal, unsigned int outSize)
	{
		char * p = outVal;


		if (outVal == NULL)
			throw Exception(__FILE__, __LINE__, Exception_error, "Convert(char*, char, unsigned int) output argument is NULL.");

		if (outSize > 0)
			*p++ = inVal;

		if (outSize > 1)
			*p = '\0';
	}

	void Convert(char* outVal, bool inVal, unsigned int outSize)
	{
		char * p = outVal;

		if (outVal == NULL)
			throw Exception(__FILE__, __LINE__, Exception_error, "Convert(char*, bool, unsigned int) output argument is NULL.");

		if (outSize > 0)
			*p++ = inVal ? '1' : '0';

		if (outSize > 1)
			*p = '\0';
	}

	void Convert(char* outVal, short inVal, unsigned int outSize)
	{
		char szBuff[32];

		if (outVal == NULL)
			throw Exception(__FILE__, __LINE__, Exception_error, "Convert(char*, bool, unsigned int) output argument is NULL.");

		sprintf(szBuff, "%i", inVal);
		strncpy(outVal, szBuff, outSize);
	}

	void Convert(char* outVal, unsigned short inVal, unsigned int outSize)
	{
		char szBuff[32];

		if (outVal == NULL)
			throw Exception(__FILE__, __LINE__, Exception_error, "Convert(char*, bool, unsigned int) output argument is NULL.");

		sprintf(szBuff, "%u", inVal);
		strncpy(outVal, szBuff, outSize);
	}

	void Convert(char* outVal, int inVal, unsigned int outSize)
	{
		char szBuff[32];

		if (outVal == NULL)
			throw Exception(__FILE__, __LINE__, Exception_error, "Convert(char*, bool, unsigned int) output argument is NULL.");

		sprintf(szBuff, "%i", inVal);
		strncpy(outVal, szBuff, outSize);
	}

	void Convert(char* outVal, unsigned int inVal, unsigned int outSize)

	{
		char szBuff[32];

		if (outVal == NULL)
			throw Exception(__FILE__, __LINE__, Exception_error, "Convert(char*, bool, unsigned int) output argument is NULL.");

		sprintf(szBuff, "%u", inVal);
		strncpy(outVal, szBuff, outSize);
	}

	void Convert(char* outVal, long inVal, unsigned int outSize)
	{
		char szBuff[32];

		if (outVal == NULL)
			throw Exception(__FILE__, __LINE__, Exception_error, "Convert(char*, bool, unsigned int) output argument is NULL.");

		sprintf(szBuff, "%i", inVal);
		strncpy(outVal, szBuff, outSize);
	}

	void Convert(char* outVal, unsigned long inVal, unsigned int outSize)
	{
		char szBuff[32];

		if (outVal == NULL)
			throw Exception(__FILE__, __LINE__, Exception_error, "Convert(char*, bool, unsigned int) output argument is NULL.");

		sprintf(szBuff, "%u", inVal);
		strncpy(outVal, szBuff, outSize);
	}

	void Convert(char* outVal, float inVal, unsigned int outSize)
	{
		char szBuff[64];

		if (outVal == NULL)
			throw Exception(__FILE__, __LINE__, Exception_error, "Convert(char*, bool, unsigned int) output argument is NULL.");

		sprintf(szBuff, "%f", inVal);
		strncpy(outVal, szBuff, outSize);
	}

	void Convert(char* outVal, double inVal, unsigned int outSize)
	{
		char szBuff[64];

		if (outVal == NULL)
			throw Exception(__FILE__, __LINE__, Exception_error, "Convert(char*, bool, unsigned int) output argument is NULL.");

		sprintf(szBuff, "%f", inVal);
		strncpy(outVal, szBuff, outSize);
	}

	void Convert(char* outVal, const D3Date & inVal, unsigned int outSize)
	{
		if (outVal == NULL)
			throw Exception(__FILE__, __LINE__, Exception_error, "Convert(char*, bool, unsigned int) output argument is NULL.");

		strncpy(outVal, inVal.AsISOString().c_str(), outSize);
	}




	// To char conversions
	//
	void Convert(char & outVal, const std::string & inVal)
	{
		long l = atol(inVal.c_str());

		if (l < CHAR_MIN || l > CHAR_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(char, std::string) loss of precision.");

		outVal = (char) l;
	}

	void Convert(char & outVal, char* inVal)
	{
		long l = atol(inVal);

		if (l < CHAR_MIN || l > CHAR_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(char, char*) loss of precision.");

		outVal = (char) l;
	}

	void Convert(char & outVal, bool inVal)
	{
		if (inVal)
			outVal = 1;
		else
			outVal = 0;
	}

	void Convert(char & outVal, short inVal)
	{
		if (inVal < CHAR_MIN || inVal > CHAR_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(char, short) loss of precision.");

		outVal = (char) inVal;
	}

	void Convert(char & outVal, unsigned short inVal)
	{
		if (inVal > CHAR_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(char, unsigned short) loss of precision.");

		outVal = (char) inVal;
	}

	void Convert(char & outVal, int inVal)
	{
		if (inVal < CHAR_MIN || inVal > CHAR_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(char, int) loss of precision.");

		outVal = (char) inVal;
	}

	void Convert(char & outVal, unsigned int inVal)
	{
		if (inVal > CHAR_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(char, unsigned int) loss of precision.");

		outVal = (char) inVal;
	}

	void Convert(char & outVal, long inVal)
	{
		if (inVal < CHAR_MIN || inVal > CHAR_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(char, long) loss of precision.");

		outVal = (char) inVal;
	}

	void Convert(char & outVal, unsigned long inVal)
	{
		if (inVal > CHAR_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(char, unsigned long) loss of precision.");

		outVal = (char) inVal;
	}

	void Convert(char & outVal, float inVal)
	{
		if (inVal < CHAR_MIN || inVal > CHAR_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(char, float) loss of precision.");

		outVal = (char) inVal;
	}

	void Convert(char & outVal, double inVal)
	{
		if (inVal < CHAR_MIN || inVal > CHAR_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(char, double) loss of precision.");

		outVal = (char) inVal;
	}

	void Convert(char & outVal, const D3Date & inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}



	void Convert(bool & outVal, const std::string & inVal)
	{
		std::string		strTemp = ToLower(inVal);

		if (strTemp == "yes"			||
				strTemp == "true"			||
				strTemp == "1"				||
				strTemp == "t"				||
				strTemp == "y")
		{
			outVal = true;
			return;
		}

		if (strTemp.empty()				||
				strTemp == "no"				||
				strTemp == "false"		||
				strTemp == "0"				||
				strTemp == "f"				||
				strTemp == "n")
		{
			outVal = false;
			return;
		}

		ReportWarningX(__FILE__, __LINE__, "Convert(bool, std::string) failed to interpret string as boolean, default to false.");

		outVal = false;
	}

	void Convert(bool & outVal, char* inVal)
	{
		Convert(outVal, std::string(inVal));
	}

	void Convert(bool & outVal, char inVal)
	{
		if (inVal != 0)
			outVal = true;
		else
			outVal = false;
	}

	void Convert(bool & outVal, short inVal)
	{
		if (inVal != 0)
			outVal = true;
		else
			outVal = false;
	}

	void Convert(bool & outVal, unsigned short inVal)
	{
		if (inVal != 0)
			outVal = true;
		else
			outVal = false;
	}

	void Convert(bool & outVal, int inVal)
	{
		if (inVal != 0)
			outVal = true;
		else
			outVal = false;
	}

	void Convert(bool & outVal, unsigned int inVal)
	{
		if (inVal != 0)
			outVal = true;
		else
			outVal = false;
	}

	void Convert(bool & outVal, long inVal)
	{
		if (inVal != 0)
			outVal = true;
		else
			outVal = false;
	}

	void Convert(bool & outVal, unsigned long inVal)
	{
		if (inVal != 0)
			outVal = true;
		else
			outVal = false;
	}

	void Convert(bool & outVal, float inVal)
	{
		if (inVal != 0)
			outVal = true;
		else
			outVal = false;
	}

	void Convert(bool & outVal, double inVal)
	{
		if (inVal != 0)
			outVal = true;
		else
			outVal = false;
	}

	void Convert(bool & outVal, const D3Date & inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);

	}





	// To short conversions
	//
	void Convert(short & outVal, const std::string & inVal)
	{
		long l = atol(inVal.c_str());

		if (l < SHRT_MIN || l > SHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(short, std::string) loss of precision.");

		outVal = (short) l;
	}

	void Convert(short & outVal, char* inVal)
	{
		long l = atol(inVal);

		if (l < SHRT_MIN || l > SHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(short, char*) loss of precision.");

		outVal = (short) l;
	}

	void Convert(short & outVal, char inVal)
	{
		outVal = inVal;
	}

	void Convert(short & outVal, bool inVal)
	{
		outVal = inVal ? 1 : 0;
	}

	void Convert(short & outVal, unsigned short inVal)
	{
		if (inVal > SHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(short, unsigned short) loss of precision.");

		outVal = (short) inVal;
	}

	void Convert(short & outVal, int inVal)
	{
		if (inVal < SHRT_MIN || inVal > SHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(short, int) loss of precision.");

		outVal = (short) inVal;
	}

	void Convert(short & outVal, unsigned int inVal)
	{
		if (inVal > SHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(short, unsigned int) loss of precision.");

		outVal = (short) inVal;
	}

	void Convert(short & outVal, long inVal)
	{
		if (inVal < SHRT_MIN || inVal > SHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(short, long) loss of precision.");

		outVal = (short) inVal;
	}

	void Convert(short & outVal, unsigned long inVal)
	{
		if (inVal > SHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(short, unsigned long) loss of precision.");

		outVal = (short) inVal;
	}

	void Convert(short & outVal, float inVal)
	{
		if (inVal < SHRT_MIN || inVal > SHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(short, float) loss of precision.");

		outVal = (short) inVal;
	}

	void Convert(short & outVal, double inVal)
	{
		if (inVal < SHRT_MIN || inVal > SHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(short, double) loss of precision.");

		outVal = (short) inVal;
	}

	void Convert(short & outVal, const D3Date & inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}





	// To unsigned short conversions
	//
	void Convert(unsigned short & outVal, const std::string & inVal)
	{
		long l = atol(inVal.c_str());

		if (l < 0 || l > USHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned short, std::string) loss of precision.");

		outVal = (unsigned short) l;
	}

	void Convert(unsigned short & outVal, char* inVal)
	{
		long l = atol(inVal);

		if (l < 0 || l > USHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned short, char*) loss of precision.");

		outVal = (unsigned short) l;
	}

	void Convert(unsigned short & outVal, char inVal)
	{
		outVal = inVal;
	}

	void Convert(unsigned short & outVal, bool inVal)
	{
		outVal = inVal ? 1 : 0;
	}

	void Convert(unsigned short & outVal, short inVal)
	{
		if (inVal < 0)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned short, short) loss of precision.");

		outVal = (unsigned short) inVal;
	}

	void Convert(unsigned short & outVal, int inVal)
	{
		if (inVal < 0 || inVal > USHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned short, int) loss of precision.");

		outVal = (unsigned short) inVal;
	}

	void Convert(unsigned short & outVal, unsigned int inVal)
	{
		if (inVal > USHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned short, unsigned int) loss of precision.");

		outVal = (unsigned short) inVal;
	}

	void Convert(unsigned short & outVal, long inVal)
	{
		if (inVal < 0 || inVal > USHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned short, long) loss of precision.");

		outVal = (unsigned short) inVal;
	}

	void Convert(unsigned short & outVal, unsigned long inVal)
	{
		if (inVal > USHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned short, unsigned long) loss of precision.");

		outVal = (unsigned short) inVal;
	}

	void Convert(unsigned short & outVal, float inVal)
	{
		if (inVal < 0 || inVal > USHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned short, float) loss of precision.");

		outVal = (short) inVal;
	}

	void Convert(unsigned short & outVal, double inVal)
	{
		if (inVal < 0 || inVal > USHRT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned short, double) loss of precision.");

		outVal = (short) inVal;
	}

	void Convert(unsigned short & outVal, const D3Date & inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}





	// To int conversions
	//
	void Convert(int & outVal, const std::string & inVal)
	{
		long l = atol(inVal.c_str());

		if (l < INT_MIN || l > INT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(int, std::string) loss of precision.");

		outVal = (int) l;
	}

	void Convert(int & outVal, char* inVal)
	{
		long l = atol(inVal);

		if (l < INT_MIN || l > INT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(int, char*) loss of precision.");

		outVal = (int) l;
	}

	void Convert(int & outVal, char inVal)
	{
		outVal = inVal;
	}

	void Convert(int & outVal, bool inVal)
	{
		outVal = inVal ? 1 : 0;

	}

	void Convert(int & outVal, short inVal)
	{
		outVal = inVal;
	}

	void Convert(int & outVal, unsigned short inVal)
	{
		outVal = (int) inVal;
	}

	void Convert(int & outVal, unsigned int inVal)
	{
		if (inVal > INT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(int, unsigned int) loss of precision.");

		outVal = (int) inVal;
	}

	void Convert(int & outVal, long inVal)
	{
		if (inVal > INT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(int, long) loss of precision.");

		outVal = (int) inVal;
	}

	void Convert(int & outVal, unsigned long inVal)
	{
		if (inVal > INT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(int, unsigned long) loss of precision.");

		outVal = (int) inVal;
	}

	void Convert(int & outVal, float inVal)
	{
		if (inVal > INT_MAX || inVal < INT_MIN)
			ReportWarningX(__FILE__, __LINE__, "Convert(int, float) loss of precision.");

		outVal = (int) inVal;
	}

	void Convert(int & outVal, double inVal)
	{
		if (inVal > INT_MAX || inVal < INT_MIN)
			ReportWarningX(__FILE__, __LINE__, "Convert(int, float) loss of precision.");

		outVal = (int) inVal;
	}

	void Convert(int & outVal, const D3Date & inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}





	// To unsigned int conversions
	//
	void Convert(unsigned int & outVal, const std::string & inVal)
	{
		char*	p;
		outVal = strtoul(inVal.c_str(), &p, 10);
	}

	void Convert(unsigned int & outVal, char* inVal)
	{
		char*	p;
		outVal = strtoul(inVal, &p, 10);
	}

	void Convert(unsigned int & outVal, char inVal)
	{
		outVal = inVal;
	}

	void Convert(unsigned int & outVal, bool inVal)
	{
		outVal = inVal ? 1 : 0;
	}

	void Convert(unsigned int & outVal, short inVal)
	{
		if (inVal < 0)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned int, short) loss of precision.");

		outVal = (unsigned int) inVal;
	}

	void Convert(unsigned int & outVal, unsigned short inVal)
	{
		outVal = (unsigned int) inVal;
	}

	void Convert(unsigned int & outVal, int inVal)
	{
		if (inVal < 0)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned int, int) loss of precision.");

		outVal = (unsigned int) inVal;
	}

	void Convert(unsigned int & outVal, long inVal)
	{
		if (inVal < 0)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned int, long) loss of precision.");

		outVal = (unsigned int) inVal;
	}

	void Convert(unsigned int & outVal, unsigned long inVal)
	{
		if (inVal > UINT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned int, unsigned long) loss of precision.");

		outVal = (unsigned int) inVal;
	}

	void Convert(unsigned int & outVal, float inVal)
	{
		if (inVal < 0 || inVal > UINT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned int, float) loss of precision.");

		outVal = (unsigned int) inVal;
	}

	void Convert(unsigned int & outVal, double inVal)
	{
		if (inVal < 0 || inVal > UINT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned int, double) loss of precision.");

		outVal = (unsigned int) inVal;
	}

	void Convert(unsigned int & outVal, const D3Date & inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}





	// To long conversions
	//
	void Convert(long & outVal, const std::string & inVal)
	{
		outVal = atol(inVal.c_str());
	}

	void Convert(long & outVal, char* inVal)
	{
		outVal = atol(inVal);
	}

	void Convert(long & outVal, char inVal)
	{
		outVal = inVal;
	}

	void Convert(long & outVal, bool inVal)
	{
		outVal = inVal ? 1 : 0;
	}

	void Convert(long & outVal, short inVal)
	{
		outVal = inVal;
	}

	void Convert(long & outVal, unsigned short inVal)
	{
		outVal = inVal;
	}

	void Convert(long & outVal, int inVal)
	{
		outVal = inVal;
	}

	void Convert(long & outVal, unsigned int inVal)
	{
		if (inVal > LONG_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(long, unsigned int) loss of precision.");

		outVal = (long) inVal;
	}

	void Convert(long & outVal, unsigned long inVal)
	{
		if (inVal > LONG_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(long, unsigned long) loss of precision.");

		outVal = (long) inVal;
	}

	void Convert(long & outVal, float inVal)
	{
		if (inVal > LONG_MAX || inVal < LONG_MIN)
			ReportWarningX(__FILE__, __LINE__, "Convert(long, float) loss of precision.");

		outVal = (long) inVal;
	}

	void Convert(long & outVal, double inVal)
	{
		if (inVal > LONG_MAX || inVal < LONG_MIN)
			ReportWarningX(__FILE__, __LINE__, "Convert(long, double) loss of precision.");

		outVal = (long) inVal;
	}

	void Convert(long & outVal, const D3Date & inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}





	// To unsigned long conversions
	//
	void Convert(unsigned long & outVal, const std::string & inVal)
	{
		char* p;
		outVal = strtoul(inVal.c_str(), &p, 10);
	}

	void Convert(unsigned long & outVal, char* inVal)
	{
		char* p;
		outVal = strtoul(inVal, &p, 10);
	}

	void Convert(unsigned long & outVal, char inVal)
	{
		outVal = inVal;
	}

	void Convert(unsigned long & outVal, bool inVal)
	{
		outVal = inVal ? 1 : 0;
	}

	void Convert(unsigned long & outVal, short inVal)
	{
		if (inVal < 0)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned long, short) loss of precision.");

		outVal = (unsigned long) inVal;
	}

	void Convert(unsigned long & outVal, unsigned short inVal)
	{
		outVal = inVal;
	}

	void Convert(unsigned long & outVal, int inVal)
	{
		if (inVal < 0)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned long, int) loss of precision.");

		outVal = (unsigned long) inVal;
	}

	void Convert(unsigned long & outVal, unsigned int inVal)
	{
		outVal = inVal;
	}

	void Convert(unsigned long & outVal, long inVal)
	{
		if (inVal < 0)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned long, long) loss of precision.");

		outVal = (unsigned long) inVal;
	}

	void Convert(unsigned long & outVal, float inVal)
	{
		if (inVal < 0 || inVal > ULONG_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned long, float) loss of precision.");

		outVal = (unsigned long) inVal;
	}

	void Convert(unsigned long & outVal, double inVal)
	{
		if (inVal < 0 || inVal > ULONG_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(unsigned long, double) loss of precision.");

		outVal = (unsigned long) inVal;
	}


	void Convert(unsigned long & outVal, const D3Date & inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}





	// To float conversions
	//
	void Convert(float & outVal, const std::string & inVal)
	{
		outVal = (float) atof(inVal.c_str());
	}

	void Convert(float & outVal, char* inVal)
	{
		outVal = (float) atof(inVal);
	}

	void Convert(float & outVal, char inVal)
	{
		outVal = (float) inVal;
	}

	void Convert(float & outVal, bool inVal)
	{
		outVal = (float) (inVal ? 1.0 : 0.0);
	}

	void Convert(float & outVal, short inVal)
	{
		outVal = (float) inVal;
	}

	void Convert(float & outVal, unsigned short inVal)
	{
		outVal = (float) inVal;
	}

	void Convert(float & outVal, int inVal)
	{
		outVal = (float) inVal;
	}

	void Convert(float & outVal, unsigned int inVal)
	{
		outVal = (float) inVal;
	}

	void Convert(float & outVal, long inVal)
	{
		outVal = (float) inVal;
	}

	void Convert(float & outVal, unsigned long inVal)
	{
		outVal = (float) inVal;
	}

	void Convert(float & outVal, double inVal)
	{
		if (inVal < FLT_MIN || inVal > FLT_MAX)
			ReportWarningX(__FILE__, __LINE__, "Convert(float, double) loss of precision.");

		outVal = (float) inVal;
	}


	void Convert(float & outVal, const D3Date & inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}





	// To double conversions
	//
	void Convert(double & outVal, const std::string & inVal)
	{
		outVal = atof(inVal.c_str());
	}

	void Convert(double & outVal, char* inVal)
	{
		outVal = atof(inVal);
	}

	void Convert(double & outVal, char inVal)
	{
		outVal = inVal;
	}

	void Convert(double & outVal, bool inVal)
	{
		outVal = inVal ? 1.0 : 0.0;
	}

	void Convert(double & outVal, short inVal)
	{

		outVal = inVal;
	}

	void Convert(double & outVal, unsigned short inVal)
	{
		outVal = inVal;
	}

	void Convert(double & outVal, int inVal)
	{
		outVal = inVal;
	}

	void Convert(double & outVal, unsigned int inVal)
	{
		outVal = inVal;

	}

	void Convert(double & outVal, long inVal)
	{
		outVal = inVal;
	}

	void Convert(double & outVal, unsigned long inVal)
	{
		outVal = inVal;
	}

	void Convert(double & outVal, float inVal)
	{
		outVal = inVal;
	}

	void Convert(double & outVal, const D3Date & inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}




	// To D3Date conversions
	//
	void Convert(D3Date & outVal, const std::string & inVal)
	{
		outVal = inVal;
	}


	void Convert(D3Date & outVal, char* inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}

	void Convert(D3Date & outVal, char inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}

	void Convert(D3Date & outVal, bool inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}

	void Convert(D3Date & outVal, short inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}

	void Convert(D3Date & outVal, unsigned short inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}

	void Convert(D3Date & outVal, int inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}

	void Convert(D3Date & outVal, unsigned int inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}

	void Convert(D3Date & outVal, long inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}

	void Convert(D3Date & outVal, unsigned long inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}

	void Convert(D3Date & outVal, float inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}

	void Convert(D3Date & outVal, double inVal)
	{
		// !!! ILLEGAL CONVERSION !!!
		assert(false);
	}





	CSL::XML::Document*	GetDocument(CSL::XML::Node* pCurrentNode)
	{
		using namespace CSL::XML;

		Node* pNode = pCurrentNode;

		while (pNode)
		{
			if (pNode->getNodeType() == Node::DOCUMENT_NODE)
				return (CSL::XML::Document*) pNode;

			pNode = pNode->getParentNode();
		}

		return NULL;
	}



	CSL::XML::Node* AddChild(CSL::XML::Node* pParent, const std::string & strName)
	{
		using namespace CSL::XML;

		Document*		pDoc;
		Element*		pChild;


		if (!pParent)
			return NULL;

		pDoc = GetDocument(pParent);

		pChild = pDoc->createElement(strName);

		if (pChild)
			pParent->appendChild(pChild)->release();

		return pChild;
	}



	CSL::XML::Node* AddChild(CSL::XML::Node* pParent, const std::string & strName, const std::string & strValue)
	{
		using namespace CSL::XML;

		Document*		pDoc;
		Element*		pChild;


		if (!pParent)
			return NULL;

		pDoc = GetDocument(pParent);

		pChild = pDoc->createElement(strName);

		if (pChild)
		{
			pParent->appendChild(pChild)->release();
			pChild->appendChild(pDoc->createTextNode(strValue))->release();
		}

		return pChild;
	}



	CSL::XML::Node*	GetNamedChild(CSL::XML::Node* pParent, const std::string & strName)
	{
		using namespace CSL::XML;

		Node*		pNode;

		if (!pParent)
			return NULL;

		pNode = pParent->getFirstChild();

		while (pNode)
		{
			if (pNode->getNodeName() == strName)
				return pNode;

			pNode = pNode->getNextSibling();
		}

		return NULL;
	}



	std::string	GetNodeValue(CSL::XML::Node* pNode)
	{
		using namespace CSL::XML;

		Node*		pChild;

		if (pNode)
		{
			if (pNode->getNodeType() == Node::TEXT_NODE)
				return pNode->getNodeValue();

			pChild = pNode->getFirstChild();

			while (pChild)
			{
				if (pChild->getNodeType() == Node::TEXT_NODE)
					return pChild->getNodeValue();

				pChild = pChild->getNextSibling();
			}
		}

		return "";
	}



	std::string GetNodeAttributeValue	(CSL::XML::Node* pNode, const std::string & strName)
	{
		using namespace CSL::XML;

		std::string		strAttributeValue;


		std::string		strAttributeName;
		strAttributeName = pNode->getNodeName();

		if (pNode && pNode->hasAttributes())
		{
			Node*					pAttrib;
			NamedNodeMap*	pMapAttributes = pNode->getAttributes();

			if (pMapAttributes)
			{
				pAttrib = pMapAttributes->getNamedItem(strName);

				if (pAttrib)
					strAttributeValue = pAttrib->getNodeValue();

				pMapAttributes->release();
			}
		}

		return strAttributeValue;
	}



	std::string	GetNamedChildValue(CSL::XML::Node* pParent, const std::string & strName)
	{
		using namespace CSL::XML;

		return GetNodeValue(GetNamedChild(pParent, strName));
	}



	// A litle helper to make it a bit easier to fetch elements from an XML DOM
	CSL::XML::Node* GetElement(CSL::XML::Node* pNode, const std::string & sDelimitedTags, char cDelimiter)
	{
		StringListItr					itrKeys;
		StringList						listKeys;
		CSL::XML::Node*				pParent;
		CSL::XML::Node*				pChild = NULL;


		if (!pNode)
			throw Exception(__FILE__, __LINE__, Exception_error, "GetElement() called without a node parameter");

		// Break the search key into a list of element names
		//
		DelimitedNamesToList(sDelimitedTags, listKeys, cDelimiter);

		// Search level by level
		//
		pParent = pNode;


		for ( itrKeys  = listKeys.begin();
					itrKeys != listKeys.end();
					itrKeys++)
		{
			pChild	= pParent->getFirstChild();

			while(pChild)
			{
				if (pChild->getNodeName() == *itrKeys)
					break;

				pChild = pChild->getNextSibling();
			}

			if (!pChild)
				break;

			pParent = pChild;
		}

		// Let's find the text node
		if (!pChild)
			throw Exception(__FILE__, __LINE__, Exception_error, "GetElement(): Failed to locate key: ", sDelimitedTags.c_str());


		return pChild;
	}




	// A litle helper to make it a bit easier to fetch immediate child elements from an XML DOM
	unsigned int GetElementsChildren(CSL::XML::Node* pNode, const std::string & strChildrenName, NodePtrList & listChildren)
	{
		CSL::XML::Node*		pChild;
		unsigned int			uChildCount = 0;


		if (!pNode)
			throw Exception(__FILE__, __LINE__, Exception_error, "GetElementsChildren() called without a node parameter");

		pChild = pNode->getFirstChild();

		while (pChild)
		{
			if (pChild->getNodeName() == strChildrenName)
			{
				uChildCount++;
				listChildren.push_back(pChild);
			}

			pChild = pChild->getNextSibling();
		}


		return uChildCount;
	}




	std::string charToHex(const unsigned char * pIn, unsigned int l)
	{
		char*	hex = "0123456789abcdef";
		std::string	strOut;
		unsigned char	msb, lsb;

		for (unsigned int i = 0; i < l; i++)
		{
			msb = *pIn >> 4;
			lsb = *pIn & 0x0f;
			strOut += hex[msb];
			strOut += hex[lsb];
			pIn++;
		}

		return strOut;
	}

} /* end namespace apal3 */
