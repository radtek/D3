#ifndef INC_D3TYPES_H
#define INC_D3TYPES_H

// ===========================================================
// File Info:
//
// $Author: pete $
// $Date: 2012/09/27 01:39:03 $
// $Revision: 1.18 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 01/09/03 - R1 - Hugp
//
// Created file for D3 stuff
//
// -----------------------------------------------------------

// The version of the meta dictionary
#define APAL_METADICTIONARY_VERSION_MAJOR			6
#define APAL_METADICTIONARY_VERSION_MINOR			7
#define APAL_METADICTIONARY_VERSION_REVISION	4


#ifdef _MSC_VER
#define AP3_OS_TARGET_WIN32

#ifndef _CRT_SECURE_NO_DEPRECATE
#define _CRT_SECURE_NO_DEPRECATE
#endif

// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <WinSock.h>

#ifdef D3_CREATEDLL
#define D3_API __declspec(dllexport) 
#else
#define D3_API __declspec(dllimport) 
#endif

// Boost thread lib causes these warnings under MS C++
#	pragma warning(disable:4251 4275 4786 4512 4800)

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0400
#endif
#endif

// MetaColumnString objects longer than this limit are treated as special strings as follows:
//   Oracle: the generated datatype will be LONG
//   SQL Server: the generated datatype will be LONG Text
//
// In both cases this means in theory a maximum length of 2GB, though for now this should be
// limited to a much smaller size (say 64K) as in all other aspects, they are not treated any
// different than other string columns.
#define D3_MAX_CONVENTIONAL_STRING_LENGTH		4000
#define D3_STREAMBUFFER_SIZE								4*1024
#define D3_MAX_LOB_SIZE											2048*D3_STREAMBUFFER_SIZE  // 8MB

// Passwords are stored as 128bit MD5 CRC values
#define D3_PASSWORD_SIZE										16

typedef unsigned char D3_BYTE;

// @@Includes
#include <stdint.h>

#if defined(_MSC_VER) && defined(D3_CREATEDLL)
  // Microsoft had to implement the secure version of sprintf in their
  // own strange way... 
  // This snprintf works like the unix version.
  int snprintf(char* pszBuffer, size_t size, const char* pszFormat, ...);

	// add a POSIX equivalent Sleep
	void sleep(long millisecs);
#endif

// USE ENVIRONMENT32 or ENVIRONMENT64 to differentiate between 32 and 64 bit
#if defined(_M_X64) || defined(__amd64__)
	#define ENVIRONMENT64
#else
	#define ENVIRONMENT32
#endif

#ifdef ENVIRONMENT64
	#define PRINTF_POINTER_MASK		"0x%016x"
	typedef uint64_t							pointer_as_int;
#else
	#define PRINTF_POINTER_MASK		"0x%08x"
	typedef uint32_t							pointer_as_int;
#endif


// Define one of these macros for Oracle OTL
// OTL_ORA8
// OTL_ORA9I
// OTL_ORA10G
// OTL_ORA10G_R2
// OTL_ORA11G
// OTL_ORA11G_R2
#if defined(OTL_ORA8) || defined(OTL_ORA9I) || defined(OTL_ORA10G) || defined(OTL_ORA10G_R2) || defined(OTL_ORA11G) || defined(OTL_ORA11G_R2)
	#define APAL_SUPPORT_OTL
#endif 

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stack>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <algorithm>
#include <stdio.h>
#include <math.h>
#include <float.h>

#include <limits.h>
#include <assert.h>
#include <malloc.h>
#include <stdlib.h>
// @@End

// Needed to populate ice identities
// #include <Ice/Ice.h>


// Simple forwards for XML DOM support
//
namespace CSL
{
	namespace XML
	{
		class Document;
		class Node;
		class Element;
	}
}

#if	defined(_MSC_VER) && !defined(__STDC__)
#	ifdef _DEBUG
#		define _CRTDBG_MAP_ALLOC
#		include <crtdbg.h>
#	endif
#endif


// Macro that defines types for a class as follows:
//
// The macro:
//
// D3_CLASS_TYPESDEFS(Session)
//
// expands to:
//
//   class Session;
//   typedef	Session*														SessionPtr;
//   typedef	::std::vector<SessionPtr>						SessionPtrVect;
//   typedef	SessionPtrVect*											SessionPtrVectPtr;
//   typedef	SessionPtrVect::iterator						SessionPtrVectItr;
//   typedef	::std::list<SessionPtr>							SessionPtrList;
//   typedef	SessionPtrList*											SessionPtrListPtr;
//   typedef	SessionPtrList::iterator						SessionPtrListItr;
//
#define D3_CLASS_TYPEDEFS(d3class) \
		class d3class; \
		typedef	d3class* 															d3class##Ptr; \
		typedef	::std::vector<d3class##Ptr>						d3class##PtrVect; \
		typedef	d3class##PtrVect*											d3class##PtrVectPtr; \
		typedef	d3class##PtrVect::iterator						d3class##PtrVectItr; \
		typedef	::std::list<d3class##Ptr>							d3class##PtrList; \
		typedef	d3class##PtrList*											d3class##PtrListPtr; \
		typedef	d3class##PtrList::iterator						d3class##PtrListItr


#define	D3_UNDEFINED_ID								UINT_MAX

// D3Date is a simple date class that is used as the value element for Columns of type dbfDate
//
D3_CLASS_TYPEDEFS(D3Date);

// D3 types
//
namespace D3
{
	// Generate typedefs for D3 classes
	//
	D3_CLASS_TYPEDEFS(Class);
	D3_CLASS_TYPEDEFS(Object);
	D3_CLASS_TYPEDEFS(Archiver);

	D3_CLASS_TYPEDEFS(ODBCDatabase);
	D3_CLASS_TYPEDEFS(OTLDatabase);

	// D3 Framework stuff
	//
	D3_CLASS_TYPEDEFS(Session);
	D3_CLASS_TYPEDEFS(DatabaseWorkspace);

	D3_CLASS_TYPEDEFS(MetaDatabase);
	D3_CLASS_TYPEDEFS(MetaEntity);
	D3_CLASS_TYPEDEFS(MetaColumn);
	D3_CLASS_TYPEDEFS(MetaColumnString);
	D3_CLASS_TYPEDEFS(MetaColumnChar);
	D3_CLASS_TYPEDEFS(MetaColumnShort);
	D3_CLASS_TYPEDEFS(MetaColumnBool);
	D3_CLASS_TYPEDEFS(MetaColumnInt);
	D3_CLASS_TYPEDEFS(MetaColumnLong);
	D3_CLASS_TYPEDEFS(MetaColumnFloat);
	D3_CLASS_TYPEDEFS(MetaColumnDate);
	D3_CLASS_TYPEDEFS(MetaColumnBlob);
	D3_CLASS_TYPEDEFS(MetaColumnBinary);
	D3_CLASS_TYPEDEFS(MetaKey);
	D3_CLASS_TYPEDEFS(MetaRelation);

	D3_CLASS_TYPEDEFS(Role);
	D3_CLASS_TYPEDEFS(User);
	D3_CLASS_TYPEDEFS(RoleUser);

	D3_CLASS_TYPEDEFS(Database);
	D3_CLASS_TYPEDEFS(Entity);
	D3_CLASS_TYPEDEFS(Column);
	D3_CLASS_TYPEDEFS(ColumnString);
	D3_CLASS_TYPEDEFS(ColumnChar);
	D3_CLASS_TYPEDEFS(ColumnShort);
	D3_CLASS_TYPEDEFS(ColumnBool);
	D3_CLASS_TYPEDEFS(ColumnInt);
	D3_CLASS_TYPEDEFS(ColumnLong);
	D3_CLASS_TYPEDEFS(ColumnFloat);
	D3_CLASS_TYPEDEFS(ColumnDate);
	D3_CLASS_TYPEDEFS(ColumnBlob);
	D3_CLASS_TYPEDEFS(ColumnBinary);
	D3_CLASS_TYPEDEFS(Key);
	D3_CLASS_TYPEDEFS(InstanceKey);
	D3_CLASS_TYPEDEFS(TemporaryKey);
	D3_CLASS_TYPEDEFS(Relation);
	D3_CLASS_TYPEDEFS(ResultSet);
	D3_CLASS_TYPEDEFS(PagedResultSet);

#ifdef D3_OBJECT_LINKS
	D3_CLASS_TYPEDEFS(ObjectLink);
#endif

	// Miscellaenous types
	//
	typedef ::std::list<std::string>		StringList;
	typedef StringList::iterator				StringListItr;

	// General Typedef's
	//
	typedef unsigned int								DatabaseIndex;
	typedef unsigned int								EntityIndex;
	typedef unsigned int								ColumnIndex;
	typedef unsigned int								KeyIndex;
	typedef unsigned int								RelationIndex;

	typedef int													DatabaseID;
	typedef int													EntityID;
	typedef int													ColumnID;
	typedef int													KeyID;
	typedef int													RelationID;

	typedef int													RoleID;
	typedef int													UserID;
	typedef int													RoleUserID;
	typedef int													SessionID;

#	define	DATABASE_ID_MAX							INT_MAX
#	define	ENTITY_ID_MAX								INT_MAX
#	define	COLUMN_ID_MAX								INT_MAX
#	define	KEY_ID_MAX									INT_MAX
#	define	RELATION_ID_MAX							INT_MAX

#	define	ROLE_ID_MAX									INT_MAX
#	define	USER_ID_MAX									INT_MAX
#	define	ROLEUSER_ID_MAX							INT_MAX

	typedef std::map<MetaDatabasePtr, DatabasePtr>	DatabasePtrMap;
	typedef DatabasePtrMap::iterator								DatabasePtrMapItr;

	/** @name Relation-Collections
			Each Entity must maintain two seperate collections of Relation objects,
			one where it is the source (Child Relations) and the other where it is
			the target (Parent Relations).

			Complications arise when the Entity is \a cached.
			Since in such circumstances only one instance of the Entity exist in the
			global database (see MetaDatabase::GetGlobalDatabase()), such	an Entity must
			keep one Relation object per Database if the related Entity is not part
			of a \a cached Entity.

			The following typedefs support a generalised implementation of relation collections:
	*/
	//@{
	typedef std::map<DatabasePtr, RelationPtr>						RelationPtrMap;
	typedef RelationPtrMap*																RelationPtrMapPtr;
	typedef RelationPtrMap::iterator											RelationPtrMapItr;
	typedef std::vector<RelationPtrMapPtr>								DatabaseRelationVect;
	typedef DatabaseRelationVect*													DatabaseRelationVectPtr;
	typedef DatabaseRelationVect::iterator								DatabaseRelationVectItr;
	//@}




	//! An enum specifying the target RDBMS.
	/*! At present, only Oracle or SQL Server are supported */
	enum TargetRDBMS
	{
		SQLServer,		//!< An SQLServer Database (0)
		Oracle,				//!< An Oracle Database (1)
		NumTargetRDBMSs		//! Must be at end of list, this enum shows how many targets have been defined (see usage)
	};


} // end namespace D3

#include "D3MDDB.h"

#include "Entity.h"
#include "Column.h"
#include "Key.h"
#include "Relation.h"
#include "ResultSet.h"

#include "D3MetaDatabase.h"
#include "D3MetaEntity.h"
#include "D3MetaColumn.h"
#include "D3MetaColumnChoice.h"
#include "D3MetaKey.h"
#include "D3MetaKeyColumn.h"
#include "D3MetaRelation.h"

#else
#define AP3_OS_TARGET_LINUX
#endif /* _MSC_VER */
