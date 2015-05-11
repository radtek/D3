#ifndef INC_D3FUNCS_H
#define INC_D3FUNCS_H

// MODULE: D3Funcs Header
//;
// ===========================================================
// File Info:
//
// $Author: pete $
// $Date: 2014/10/01 03:51:23 $
// $Revision: 1.14 $
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
#include "D3Types.h"

class D3Date;


namespace D3
{
	//! Return's the OS independent thread ID if the calling thread
	D3_API	long GetCurrentProcessID();
	//! Return's the OS independent thread ID if the calling thread
	D3_API	long GetCurrentThreadID();

	// System Data/Time to formated Date/Time strings
	//
	D3_API	std::string SystemDateTimeAsD3String();

	D3_API	std::string SystemDateTimeAsStandardString();
	D3_API	std::string SystemDateAsStandardString();
	D3_API	std::string SystemTimeAsStandardString();

	// Some std::string helpers
	//
	D3_API	std::string AllTrim(const std::string & strIn, char cTrim = ' ');
	D3_API	std::string LeftTrim(const std::string & strIn, char cTrim = ' ');
	D3_API	std::string RightTrim(const std::string & strIn, char cTrim = ' ');

	D3_API	std::string ToLower(const std::string & strIn);
	D3_API	std::string ToUpper(const std::string & strIn);
	D3_API	std::string ToCaps(const std::string & strIn);

	D3_API	std::string Escape(const std::string & strIn);
	D3_API	std::string Unescape(const std::string & strIn);
	D3_API	std::string ToSentence(const std::string & strIn);

	//! Replaces strFindString with strReplaceWith in strIn with strReplaceWith.
	/*!
			Notes:
				- if bAll is true, all occurences of strFindString will be replaced with strReplaceWith, otherwise only the first one will be replaced
				- returns a reference to strIn
	*/
	D3_API	std::string & ReplaceSubstring(std::string & strIn, const std::string & strFindString, const std::string & strReplaceWith, bool bAll = true);

	//! Return a copy of strInput where all occurences of strSrch are replaced with strRepl
	D3_API	std::string ReplaceAll(const std::string & strInput, const std::string & strSrch, const std::string & strRepl);

	// Returns strIn as an XML Encoded string
	D3_API	std::string XMLEncode(const std::string & strIn);

	// Returns strIn as an JSON Encoded string
	D3_API	std::string JSONEncode(const std::string & strIn);

	//! Expects an identifier and converts it to something that suits ICE
	D3_API	std::string IcifyIdentifier(const std::string& strIdentifier);

	// Return the noun passed in in its pluralized form
	//
	D3_API	std::string Pluralize(const std::string & strNoun);

	// These helpers decode std::string fileds which are used to store TMS
	// extended column Names and Values
	//
	D3_API	void DelimitedNamesToList(const std::string & strInput, StringList & listString, char cTerminator = ';');
	D3_API	void DelimitedValuesToList(const std::string & strInput, StringList & listString, char cTerminator = ';');

	D3_API	int GetRandomNumber(unsigned int iMin, unsigned int iMax);
	D3_API	void SleepThread(int iSecs);

	D3_API	char * ltoa(long lIn, char * szBuffer, int);
	D3_API	char * itoa(int iIn, char * szBuffer, int);

	//! This function parses a string value and builds a new string that contains two single quotes for each single quote that is encountered in the original string
	D3_API	std::string AsSQLQueryString(const std::string & strValue);

	//! This function returns HostName
	D3_API	std::string GetSystemHostName();
	D3_API	bool IsValidHost(const std::string & HostList);

	D3_API void Convert(std::string & outVal, char* inVal);
	D3_API void Convert(std::string & outVal, char inVal);
	D3_API void Convert(std::string & outVal, bool inVal);
	D3_API void Convert(std::string & outVal, short inVal);
	D3_API void Convert(std::string & outVal, unsigned short inVal);
	D3_API void Convert(std::string & outVal, int inVal);
	D3_API void Convert(std::string & outVal, unsigned int inVal);
	D3_API void Convert(std::string & outVal, long inVal);
	D3_API void Convert(std::string & outVal, unsigned long inVal);
	D3_API void Convert(std::string & outVal, float inVal);
	D3_API void Convert(std::string & outVal, double inVal);
	D3_API void Convert(std::string & outVal, const D3Date & inVal);

	D3_API void Convert(char* outVal, const std::string & inVal,								unsigned int outSize);
	D3_API void Convert(char* outVal, char inVal,															unsigned int outSize);
	D3_API void Convert(char* outVal, bool inVal,															unsigned int outSize);
	D3_API void Convert(char* outVal, short inVal,															unsigned int outSize);
	D3_API void Convert(char* outVal, unsigned short inVal,										unsigned int outSize);
	D3_API void Convert(char* outVal, int inVal,																unsigned int outSize);
	D3_API void Convert(char* outVal, unsigned int inVal,											unsigned int outSize);
	D3_API void Convert(char* outVal, long inVal,															unsigned int outSize);
	D3_API void Convert(char* outVal, unsigned long inVal,											unsigned int outSize);
	D3_API void Convert(char* outVal, float inVal,															unsigned int outSize);
	D3_API void Convert(char* outVal, double inVal,														unsigned int outSize);
	D3_API void Convert(char* outVal, const D3Date & inVal,	unsigned int outSize);


	D3_API void Convert(char & outVal, const std::string & inVal);
	D3_API void Convert(char & outVal, char* inVal);
	D3_API void Convert(char & outVal, bool inVal);
	D3_API void Convert(char & outVal, short inVal);
	D3_API void Convert(char & outVal, unsigned short inVal);
	D3_API void Convert(char & outVal, int inVal);
	D3_API void Convert(char & outVal, unsigned int inVal);
	D3_API void Convert(char & outVal, long inVal);
	D3_API void Convert(char & outVal, unsigned long inVal);
	D3_API void Convert(char & outVal, float inVal);
	D3_API void Convert(char & outVal, double inVal);
	D3_API void Convert(char & outVal, const D3Date & inVal);

	D3_API void Convert(bool & outVal, const std::string & inVal);
	D3_API void Convert(bool & outVal, char* inVal);
	D3_API void Convert(bool & outVal, char inVal);
	D3_API void Convert(bool & outVal, short inVal);
	D3_API void Convert(bool & outVal, unsigned short inVal);
	D3_API void Convert(bool & outVal, int inVal);
	D3_API void Convert(bool & outVal, unsigned int inVal);
	D3_API void Convert(bool & outVal, long inVal);
	D3_API void Convert(bool & outVal, unsigned long inVal);
	D3_API void Convert(bool & outVal, float inVal);
	D3_API void Convert(bool & outVal, double inVal);
	D3_API void Convert(bool & outVal, const D3Date & inVal);

	D3_API void Convert(short & outVal, const std::string & inVal);
	D3_API void Convert(short & outVal, char* inVal);
	D3_API void Convert(short & outVal, char inVal);
	D3_API void Convert(short & outVal, bool inVal);
	D3_API void Convert(short & outVal, unsigned short inVal);
	D3_API void Convert(short & outVal, int inVal);
	D3_API void Convert(short & outVal, unsigned int inVal);
	D3_API void Convert(short & outVal, long inVal);
	D3_API void Convert(short & outVal, unsigned long inVal);
	D3_API void Convert(short & outVal, float inVal);
	D3_API void Convert(short & outVal, double inVal);
	D3_API void Convert(short & outVal, const D3Date & inVal);

	D3_API void Convert(unsigned short & outVal, const std::string & inVal);
	D3_API void Convert(unsigned short & outVal, char* inVal);
	D3_API void Convert(unsigned short & outVal, char inVal);
	D3_API void Convert(unsigned short & outVal, bool inVal);
	D3_API void Convert(unsigned short & outVal, short inVal);
	D3_API void Convert(unsigned short & outVal, int inVal);
	D3_API void Convert(unsigned short & outVal, unsigned int inVal);
	D3_API void Convert(unsigned short & outVal, long inVal);
	D3_API void Convert(unsigned short & outVal, unsigned long inVal);
	D3_API void Convert(unsigned short & outVal, float inVal);
	D3_API void Convert(unsigned short & outVal, double inVal);
	D3_API void Convert(unsigned short & outVal, const D3Date & inVal);

	D3_API void Convert(int & outVal, const std::string & inVal);
	D3_API void Convert(int & outVal, char* inVal);
	D3_API void Convert(int & outVal, char inVal);
	D3_API void Convert(int & outVal, bool inVal);
	D3_API void Convert(int & outVal, short inVal);
	D3_API void Convert(int & outVal, unsigned short inVal);
	D3_API void Convert(int & outVal, unsigned int inVal);
	D3_API void Convert(int & outVal, long inVal);
	D3_API void Convert(int & outVal, unsigned long inVal);
	D3_API void Convert(int & outVal, float inVal);
	D3_API void Convert(int & outVal, double inVal);
	D3_API void Convert(int & outVal, const D3Date & inVal);

	D3_API void Convert(unsigned int & outVal, const std::string & inVal);
	D3_API void Convert(unsigned int & outVal, char* inVal);
	D3_API void Convert(unsigned int & outVal, char inVal);
	D3_API void Convert(unsigned int & outVal, bool inVal);
	D3_API void Convert(unsigned int & outVal, short inVal);
	D3_API void Convert(unsigned int & outVal, unsigned short inVal);
	D3_API void Convert(unsigned int & outVal, int inVal);
	D3_API void Convert(unsigned int & outVal, long inVal);
	D3_API void Convert(unsigned int & outVal, unsigned long inVal);
	D3_API void Convert(unsigned int & outVal, float inVal);
	D3_API void Convert(unsigned int & outVal, double inVal);
	D3_API void Convert(unsigned int & outVal, const D3Date & inVal);

	D3_API void Convert(long & outVal, const std::string & inVal);
	D3_API void Convert(long & outVal, char* inVal);
	D3_API void Convert(long & outVal, char inVal);
	D3_API void Convert(long & outVal, bool inVal);
	D3_API void Convert(long & outVal, short inVal);
	D3_API void Convert(long & outVal, unsigned short inVal);
	D3_API void Convert(long & outVal, int inVal);
	D3_API void Convert(long & outVal, unsigned int inVal);
	D3_API void Convert(long & outVal, unsigned long inVal);
	D3_API void Convert(long & outVal, float inVal);
	D3_API void Convert(long & outVal, double inVal);
	D3_API void Convert(long & outVal, const D3Date & inVal);

	D3_API void Convert(unsigned long & outVal, const std::string & inVal);
	D3_API void Convert(unsigned long & outVal, char* inVal);
	D3_API void Convert(unsigned long & outVal, char inVal);
	D3_API void Convert(unsigned long & outVal, bool inVal);
	D3_API void Convert(unsigned long & outVal, short inVal);
	D3_API void Convert(unsigned long & outVal, unsigned short inVal);
	D3_API void Convert(unsigned long & outVal, int inVal);
	D3_API void Convert(unsigned long & outVal, unsigned int inVal);
	D3_API void Convert(unsigned long & outVal, long inVal);
	D3_API void Convert(unsigned long & outVal, float inVal);
	D3_API void Convert(unsigned long & outVal, double inVal);
	D3_API void Convert(unsigned long & outVal, const D3Date & inVal);

	D3_API void Convert(float & outVal, const std::string & inVal);
	D3_API void Convert(float & outVal, char* inVal);
	D3_API void Convert(float & outVal, char inVal);
	D3_API void Convert(float & outVal, bool inVal);
	D3_API void Convert(float & outVal, short inVal);
	D3_API void Convert(float & outVal, unsigned short inVal);
	D3_API void Convert(float & outVal, int inVal);
	D3_API void Convert(float & outVal, unsigned int inVal);
	D3_API void Convert(float & outVal, long inVal);
	D3_API void Convert(float & outVal, unsigned long inVal);
	D3_API void Convert(float & outVal, double inVal);
	D3_API void Convert(float & outVal, const D3Date & inVal);

	D3_API void Convert(double & outVal, const std::string & inVal);
	D3_API void Convert(double & outVal, char* inVal);
	D3_API void Convert(double & outVal, char inVal);
	D3_API void Convert(double & outVal, bool inVal);
	D3_API void Convert(double & outVal, short inVal);
	D3_API void Convert(double & outVal, unsigned short inVal);
	D3_API void Convert(double & outVal, int inVal);
	D3_API void Convert(double & outVal, unsigned int inVal);
	D3_API void Convert(double & outVal, long inVal);
	D3_API void Convert(double & outVal, unsigned long inVal);
	D3_API void Convert(double & outVal, float inVal);
	D3_API void Convert(double & outVal, const D3Date & inVal);

	D3_API void Convert(D3Date & outVal, const std::string & inVal);
	D3_API void Convert(D3Date & outVal, char* inVal);
	D3_API void Convert(D3Date & outVal, char inVal);
	D3_API void Convert(D3Date & outVal, bool inVal);
	D3_API void Convert(D3Date & outVal, short inVal);
	D3_API void Convert(D3Date & outVal, unsigned short inVal);
	D3_API void Convert(D3Date & outVal, int inVal);
	D3_API void Convert(D3Date & outVal, unsigned int inVal);
	D3_API void Convert(D3Date & outVal, long inVal);
	D3_API void Convert(D3Date & outVal, unsigned long inVal);
	D3_API void Convert(D3Date & outVal, float inVal);
	D3_API void Convert(D3Date & outVal, double inVal);


	typedef CSL::XML::Node*				NodePtr;
	typedef std::list<NodePtr>		NodePtrList;
	typedef NodePtrList::iterator	NodePtrListItr;

	// Some helpers to make life working with XML DOM a bit easier
	//
	D3_API CSL::XML::Document*		GetDocument						(CSL::XML::Node*);
	D3_API CSL::XML::Node*				AddChild							(CSL::XML::Node* pParent, const std::string & strName);
	D3_API CSL::XML::Node*				AddChild							(CSL::XML::Node* pParent, const std::string & strName, const std::string & strValue);
	D3_API CSL::XML::Node*				GetNamedChild					(CSL::XML::Node* pParent, const std::string & strName);
	D3_API std::string						GetNodeValue					(CSL::XML::Node* pNode);
	D3_API std::string						GetNodeAttributeValue	(CSL::XML::Node* pNode, const std::string & strName);
	D3_API std::string						GetNamedChildValue		(CSL::XML::Node* pParent, const std::string & strName);
	D3_API CSL::XML::Node*				GetElement						(CSL::XML::Node* pNode, const std::string & sDelimitedTags, char cDelimiter = '/');
	D3_API unsigned int						GetElementsChildren		(CSL::XML::Node* pNode, const std::string & strChildrenName, NodePtrList & listChildren);


	//! This fuction returns a new string containing l bytes from pIn in hex representation (does not return leading 0x)
	D3_API std::string charToHex(const unsigned char * pIn, unsigned int l);

	//! This fuction returns a new string containing all bytes from the input string in hex representation (does not return leading 0x)
	D3_API inline std::string strToHex(const std::string& strIn) { return charToHex((const unsigned char *) strIn.c_str(), strIn.size()); }
} /* end namespace apal3 */

#endif /* INC_D3FUNCS_H */

