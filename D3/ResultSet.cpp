// MODULE: ResultSet source
//;
// ===========================================================
// File Info:
//
// $Author: pete $
// $Date: 2012/09/27 01:39:03 $
// $Revision: 1.26 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 27/08/03 - R1 - Hugp
//
// Created class
//
// -----------------------------------------------------------

// @@DatatypeInclude
#include "D3Types.h"

#include <string.h>				// uses _strupr
#include <sstream>				// uses ostringstream
// @@End
// @@Includes
#include "ResultSet.h"
#include "OTLDatabase.h"
#include "ODBCDatabase.h"
#include "Session.h"

// Needs JSON
#include <json/json.h>


namespace D3
{
	// ==========================================================================
	// ResultSet::iterator class implementation
	//








	// ==========================================================================
	// ResultSet class implementation
	//

	// Standard D3 stuff
	//
	D3_CLASS_IMPL_PV(ResultSet, Object);



	ResultSet::ResultSet(DatabasePtr pDB, MetaEntityPtr pME)
	: m_bKeepObjects(false),
		m_lID(0),
		m_pDatabase(pDB),
		m_pMetaEntity(pME),
		m_pListEntity(NULL),
		m_bInitialised(false),
		m_uTotalSize(0),
		m_uPageSize(0),
		m_uCurrentPage(0),
		m_uFirst(0),
		m_uLast(0)
	{
		assert(m_pDatabase);
		assert(m_pMetaEntity);

		if (m_pDatabase->GetMetaDatabase() != m_pMetaEntity->GetMetaDatabase())
			throw Exception(__FILE__, __LINE__, Exception_error, "ResultSet::ctor(): Database % does not have entity %s", m_pDatabase->GetMetaDatabase()->GetAlias().c_str(), m_pMetaEntity->GetFullName().c_str());

		// This will assign a proper m_lID to this and record this with the database that owns it
		m_pDatabase->On_ResultSetCreated(this);

		if (m_pMetaEntity->IsCached())
			m_bKeepObjects = true;
	}




	ResultSet::~ResultSet()
	{
		try
		{
			DeleteObjectList();
		}
		catch(...)
		{
		}

		m_pDatabase->On_ResultSetDeleted(this);
	}




	void ResultSet::SetKeepObjects(bool bKeepObjects)
	{
		if (!m_pMetaEntity->IsCached())
			m_bKeepObjects = bKeepObjects;
	}




	void ResultSet::DeleteObjectList()
	{
		try
		{
			if (m_pListEntity)
			{
				if (!m_bKeepObjects)
				{
					while (!m_pListEntity->empty())
						delete m_pListEntity->front();
				}
				else
				{
					// Inform all entity objects that they no longer belong to this
					EntityPtrListItr		itr;
					EntityPtr						pEntity;

					for ( itr =  m_pListEntity->begin();
								itr != m_pListEntity->end();
								itr++)
					{
						pEntity = *itr;
						pEntity->On_RemovedFromResultSet(this);
					}
				}

				delete m_pListEntity;
				m_pListEntity = NULL;
			}
		}
		catch(...)
		{
			delete m_pListEntity;
			m_pListEntity = NULL;
			throw;
		}
	}





	void ResultSet::CreateObjectList(EntityPtrListPtr pListEntity)
	{
		EntityPtrListItr		itr;
		EntityPtr						pEntity;


		assert(m_pListEntity==NULL);

		if (pListEntity)
		{
			m_pListEntity = pListEntity;

			for ( itr =  m_pListEntity->begin();
						itr != m_pListEntity->end();
						itr++)
			{
				pEntity = *itr;
				pEntity->On_AddedToResultSet(this);
			}
		}
	}





	void ResultSet::On_EntityDeleted(EntityPtr pEntity)
	{
		assert(m_pListEntity);
		m_pListEntity->remove(pEntity);
	}





	ResultSetPtr ResultSet::Create(	DatabasePtr					pDB,
																	MetaEntityPtr				pME,
																	const std::string&	strSQLWHEREClause,
																	const std::string&	strSQLORDERBYClause,
																	unsigned long				uPageSize)
	{
		ResultSet*		pRS = NULL;


		switch (pME->GetMetaDatabase()->GetTargetRDBMS())
		{
			case Oracle:
				pRS = OraclePagedResultSet::Create(pDB, pME, strSQLWHEREClause, strSQLORDERBYClause, uPageSize);
				break;

			case SQLServer:
				pRS = OraclePagedResultSet::Create(pDB, pME, strSQLWHEREClause, strSQLORDERBYClause, uPageSize);
//				pRS = SQLServerPagedResultSet::Create(pDB, pME, strSQLWHEREClause, strSQLORDERBYClause, uPageSize);
				break;

			default:
				throw D3::Exception(__FILE__, __LINE__, Exception_error, "ResultSet::Create(): Only know how to create ResultSets for SQLServer or Oracle.");
		}

		return pRS;
	}





	ResultSetPtr ResultSet::Create(	DatabasePtr					pDB,
																	RelationPtr					pRelation,
																	unsigned long				uPageSize,
																	const std::string*	pstrSQLWHEREClause,
																	const std::string*	pstrSQLORDERBYClause)
	{
		ResultSet*							pRS = NULL;
		MetaKeyPtr							pMetaKey;
		KeyPtr									pKey;
		ColumnPtrListItr				itrSrceColumn;
		MetaColumnPtrListItr		itrTrgtColumn;
		ColumnPtr								pSrce;
		MetaColumnPtr						pTrgt;
		std::string							strWHERE;
		std::string							strORDERBY;


		assert(pRelation);

		pMetaKey	= pRelation->GetMetaRelation()->GetChildMetaKey();
		pKey			= pRelation->GetParentKey();

		assert(pMetaKey);;
		assert(pKey);
		assert(pDB->GetMetaDatabase() == pMetaKey->GetMetaEntity()->GetMetaDatabase());

		// Build the SQL where clause
		for ( itrTrgtColumn =  pMetaKey->GetMetaColumns()->begin(),			itrSrceColumn =  pKey->GetColumns().begin();
					itrTrgtColumn != pMetaKey->GetMetaColumns()->end()    &&	itrSrceColumn != pKey->GetColumns().end();
					itrTrgtColumn++,																					itrSrceColumn++)
		{
			pTrgt = *itrTrgtColumn;
			pSrce = *itrSrceColumn;

			if (!strWHERE.empty())
				strWHERE += " AND ";

			strWHERE += pTrgt->GetName();

			if (pSrce->IsNull())
			{
				strWHERE += " IS NULL";
			}
			else
			{
				strWHERE += '=';
				strWHERE += pSrce->AsSQLString();
			}
		}

		// Complement the WHERE predicate if additional filter criteria was provided
		if (pstrSQLWHEREClause && !pstrSQLWHEREClause->empty())
			strWHERE = std::string("(") + strWHERE + ") AND (" + *pstrSQLWHEREClause + ')';

		// If provided, use passed-in ORDER BY otherwise build it based on the childrens foreign key
		if (pstrSQLORDERBYClause && !pstrSQLORDERBYClause->empty())
		{
			strORDERBY = *pstrSQLORDERBYClause;
		}
		else
		{
			// Simply sort by the full child key
			for ( itrTrgtColumn =  pMetaKey->GetMetaColumns()->begin();
						itrTrgtColumn != pMetaKey->GetMetaColumns()->end();
						itrTrgtColumn++)
			{
				pTrgt = *itrTrgtColumn;

				if (!strORDERBY.empty())
					strORDERBY += ",";

				strORDERBY += pTrgt->GetName();
			}
		}

		switch (pMetaKey->GetMetaEntity()->GetMetaDatabase()->GetTargetRDBMS())
		{
			case Oracle:
				pRS = OraclePagedResultSet::Create(pDB, pMetaKey->GetMetaEntity(), strWHERE, strORDERBY, uPageSize);
				break;

			case SQLServer:
				pRS = OraclePagedResultSet::Create(pDB, pMetaKey->GetMetaEntity(), strWHERE, strORDERBY, uPageSize);
//				pRS = SQLServerPagedResultSet::Create(pDB, pMetaKey->GetMetaEntity(), strWHERE, strORDERBY, uPageSize);
				break;

			default:
				throw D3::Exception(__FILE__, __LINE__, Exception_error, "ResultSet::Create(): Only know how to create ResultSets for SQLServer or Oracle.");
		}

		return pRS;
	}





	/* static */
	ResultSetPtr ResultSet::Create(	DatabaseWorkspacePtr	pDBWS,
																	const std::string&		strJSONKey,
																	unsigned long					uPageSize)
	{
		std::ostringstream			ostrm;
		DatabasePtr							pDB;
		MetaKeyPtr							pMK;
		MetaColumnPtrListItr		itrMC;
		MetaColumnPtr						pMC;
		Json::Value							jsonKey;
		Json::Reader						jsonReader;
		bool										bSuccess = false;
		std::string							strSQLWHEREClause;
		bool										bFirstWHERE = true;
		std::string							strSQLORDERBYClause;
		bool										bFirstORDER = true;
		std::string							strMsg;
		int											idx;
		ResultSetPtr						pRslt;


		try
		{
			if (!pDBWS)
				throw "No workspace";

			if (!jsonReader.parse(strJSONKey, jsonKey))
				throw "JSON parse error";

			// Determine the Key
			Json::Value&	nKeyID = jsonKey["KeyID"];

			if (nKeyID.type() != Json::uintValue && nKeyID.type() != Json::intValue)
				throw "KeyID missing or incorrect type";

			Json::Value&	aryColumns = jsonKey["Columns"];

			if (aryColumns.type() != Json::arrayValue)
				throw "Columns missing or not an array";

			// Let's get the meta key and the instance database
			pMK = MetaKey::GetMetaKeyByID(nKeyID.asUInt());

			if (!pMK)
				throw "KeyID does not exist";

			pDB = pDBWS->GetDatabase(pMK->GetMetaEntity()->GetMetaDatabase());

			if (!pDB)
				throw "Unable to locate instance database in work space";

			if (aryColumns.size() != pMK->GetColumnCount())
				throw "Requested keytype has doesn't match the number of columns specified";

			// Build a where predicate and order by clause
			for ( itrMC =  pMK->GetMetaColumns()->begin(), idx = 0;
						itrMC != pMK->GetMetaColumns()->end();
						itrMC++, idx++)
			{
				Json::Value&	nColumn = aryColumns[idx];
				pMC = *itrMC;

				if (nColumn.type() != Json::nullValue)
				{
					switch (pMC->GetType())
					{
						case MetaColumn::dbfDate:
						case MetaColumn::dbfString:
							if (nColumn.type() != Json::stringValue)
								throw "Incorrect column value - expected string type";

							if (bFirstWHERE)
								bFirstWHERE = false;
							else
								strSQLWHEREClause += " AND ";

							strSQLWHEREClause += pMC->GetName() + " LIKE '" + nColumn.asString() + "%'";
							break;

						case MetaColumn::dbfBool:
							if (nColumn.type() != Json::booleanValue)
								throw "JSON";

						case MetaColumn::dbfChar:
						case MetaColumn::dbfShort:
						case MetaColumn::dbfInt:
						case MetaColumn::dbfLong:
						{
							std::string	strValue;

							switch (nColumn.type())
							{
								case Json::intValue:
									Convert(strValue, (long) nColumn.asInt());
									break;

								case Json::uintValue:
									Convert(strValue, (long) nColumn.asUInt());
									break;
							}

							if (!strValue.empty())
							{
								if (bFirstWHERE)
									bFirstWHERE = false;
								else
									strSQLWHEREClause += " AND ";

								strSQLWHEREClause += pMC->GetName() + " LIKE '" + strValue + "%'";
							}

							break;
						}

						case MetaColumn::dbfFloat:
						{
							if (nColumn.type() != Json::realValue)
								throw "Incorrect column value - expected float type";

							float f = (float) nColumn.asDouble();
							std::string	strValue;

							Convert(strValue, f);

							if (bFirstWHERE)
								bFirstWHERE = false;
							else
								strSQLWHEREClause += " AND ";

							strSQLWHEREClause += pMC->GetName() + "=" + strValue;
							break;
						}
					}
				}

				if (bFirstORDER)
					bFirstORDER = false;
				else
					strSQLORDERBYClause += ',';

				strSQLORDERBYClause += pMC->GetName();
			}
		}
		catch (const char* pszErr)
		{
			throw Exception(__FILE__, __LINE__, Exception_error, "ResultSet::Create(): %s. Input:%s", pszErr, strJSONKey.c_str());
		}

		pRslt = Create(pDB, pMK->GetMetaEntity(), strSQLWHEREClause, strSQLORDERBYClause, uPageSize);

		pRslt->m_strJSONKey = strJSONKey;

		return pRslt;
	}




	std::ostream & ResultSet::InfoAsJSON(std::ostream & ostrm)
	{
		SessionPtr					pSession = GetSession();

		return m_pMetaEntity->AsJSON(pSession ? pSession->GetRoleUser() : NULL, ostrm, NULL, true);
	}




	std::ostream & ResultSet::DataAsJSON(std::ostream & ostrm)
	{
		EntityPtrListItr		itr;
		EntityPtr						pEntity;
		SessionPtr					pSession = GetSession();
		bool								bFirst = true;


		ostrm << "{";
		ostrm << "\"totalResultsAvailable\":" << GetTotalRecords() << ',';
		ostrm << "\"totalResultsReturned\":"  << (!m_pListEntity || m_pListEntity->empty() ?  0 : m_pListEntity->size()) << ',';
		ostrm << "\"firstResultPosition\":" << GetNoFirstInPage() << ',';
		ostrm << "\"Results\":[";

		if (m_pListEntity)
		{
			for ( itr =  m_pListEntity->begin();
						itr != m_pListEntity->end();
						itr++)
			{
				pEntity = *itr;

				if (pSession && pSession->GetRoleUser()->GetUser()->HasAccess(pEntity))
					pEntity->AsJSON(pSession ? pSession->GetRoleUser() : NULL, ostrm, &bFirst);
			}
		}

		ostrm << "]}";

		return ostrm;
	}











	// ==========================================================================
	// OraclePagedResultSet class implementation
	//

	// Standard D3 stuff
	//
	D3_CLASS_IMPL(OraclePagedResultSet, ResultSet);




	/* We try and build a query a bit like this:

			select ITMCLS,ITMCOD,TYPE,ITMDSC,CASHGT,CASLEN,CASWID,CASWGT,CRSHIX
				FROM  (select ITMCLS,ITMCOD,TYPE,ITMDSC,CASHGT,CASLEN,CASWID,CASWGT,CRSHIX, ROW_NUMBER() OVER (ORDER BY CASWGT DESC) R
								 FROM  ap3_product_data
								 WHERE CRSHIX=4)
				WHERE R BETWEEN 2001 AND 2010

			Here we assumed that the parameters passed are:
							- pME									= MetaDatabase::GetMetaDatabase("RTCISDB")->GetMetaEntity("ap3_product_data");
							- strSQLWHEREClause		= "CRSHIX=4"
							- strSQLORDERBYClause = "CASWGT DESC"
							- uPageSize						=	10

							We'd expect the above query to be generated if we called GetPage(200);


			Notes:	- If no ORDER BY clause is supplied, we oder by primary key

							- the base query excludes the 2000 AND 2010 part, this is
								generated depending on the page we want

							- when Initialising we also execute the SQL
								select count(*) FROM  ap3_product_data WHERE CRSHIX=4

	*/
	ResultSetPtr OraclePagedResultSet::Create(DatabasePtr					pDB,
																						MetaEntityPtr				pME,
																						const std::string&	strSQLWHEREClause,
																						const std::string&	strSQLORDERBYClause,
																						unsigned long				uPageSize)
	{
//		if (pME->GetMetaDatabase()->GetTargetRDBMS() != Oracle)
//			throw Exception(__FILE__, __LINE__, Exception_error, "OraclePagedResultSet::ctor() can only be called for entities in Oracle databases.");

		OraclePagedResultSet*	pRS;

		pRS = new OraclePagedResultSet(pDB, pME);

		pRS->m_uPageSize = uPageSize;
		pRS->BuildQueryStrings(strSQLWHEREClause, strSQLORDERBYClause);

		return pRS;
	}




	// Here we build a query like:
	//
	//  SELECT A, B, C, HasD, HasE FROM
	// (SELECT A, B, C, NVL2(D,1,0) AS HasD, NVL2(E,1,0) AS HasE, ROW_NUMBER() OVER (ORDER BY PKCol1, PKCol2) R FROM Table WHERE PKCol1=x AND PKCol2=y) WHERE R BETWEEN n AND m
	//
	// As you can see we need the SELECT list twice:
	//
	//   - The first select list simply prefixes all lazyfetch columns as HasCOLUMNNAME
	//   - The second select lists lazyfetch columns as NVL2(COLUMNNAME,1,0) AS HasCOLUMNNAME
	//     and appends the row selector column ", ROW_NUMBER() OVER (ORDER BY PKCol1, PKCol2) R"
	//
	void OraclePagedResultSet::BuildQueryStrings(const std::string&	strSQLWHEREClause, const std::string&	strSQLORDERBYClause)
	{
		std::string						strSQLPKCols;
		std::string						strWhereClause;
		std::string						strFirstSelectList;
		std::string						strSecondSelectList;
		unsigned int					idx;
		MetaColumnPtr					pMC;
		bool									bFirst = true;
		SessionPtr						pSession = GetSession();


		// Build filter
		if (!strSQLWHEREClause.empty())
		{
			strWhereClause = " WHERE ";
			strWhereClause += strSQLWHEREClause;
		}

		if (pSession && !pSession->GetRoleUser()->GetUser()->GetRLSPredicate(m_pMetaEntity).empty())
		{
			if (!strWhereClause.empty())
				strWhereClause += " AND ";
			else
				strWhereClause += " WHERE ";

			strWhereClause += pSession->GetRoleUser()->GetUser()->GetRLSPredicate(m_pMetaEntity);
		}


		// 1. Build the first and second select string
		for (idx = 0; idx < m_pMetaEntity->GetMetaColumnsInFetchOrder()->size(); idx++)
		{
			pMC = m_pMetaEntity->GetMetaColumnsInFetchOrder()->operator[](idx);

			if (pMC->IsDerived())
				break;

			if (bFirst)
			{
				bFirst = false;
			}
			else
			{
				strFirstSelectList += ',';
				strSecondSelectList += ',';
			}

			if (pMC->IsLazyFetch())
			{
				// We don't want the value but an indicator
				strFirstSelectList += "Has";
				strFirstSelectList += pMC->GetName();

				switch (m_pMetaEntity->GetMetaDatabase()->GetTargetRDBMS())
				{
					case Oracle:
						if (pMC->IsStreamed())
						{
							strSecondSelectList += "CASE WHEN dbms_lob.getLength(";
							strSecondSelectList += pMC->GetName();
							strSecondSelectList += ") > 0 THEN 1 ELSE 0 END AS Has";
							strSecondSelectList += pMC->GetName();
						}
						else
						{
							strSecondSelectList += "NVL2(";
							strSecondSelectList += pMC->GetName();
							strSecondSelectList += ", 1, 0) AS Has";
							strSecondSelectList += pMC->GetName();
						}
						break;

					case SQLServer:
						strSecondSelectList += "CASE WHEN ";
						strSecondSelectList += pMC->GetName();
						strSecondSelectList += " IS NULL THEN CONVERT(bit, 0) ELSE CONVERT(bit, 1) END AS Has";
						strSecondSelectList += pMC->GetName();
						break;
				}
			}
			else
			{
				strFirstSelectList += pMC->GetName();
				strSecondSelectList += pMC->GetName();
			}
		}

		// This is a real paged result set so we do:
		// 2. Build all the parts making up the two SQL queries we need (see next steps)
		strSQLPKCols			= SetKeyColsSQLQueryPart();

		// 3. Build a query that returns the total number of records in the result set
		m_strSQLCountQuery = "SELECT COUNT(*) FROM ";
		m_strSQLCountQuery += m_pMetaEntity->GetName();

		if (!strWhereClause.empty())
			m_strSQLCountQuery += strWhereClause;

		// 4. Build the query prefix
		m_strSQLBaseQuery = "SELECT ";
		m_strSQLBaseQuery += strFirstSelectList;
		m_strSQLBaseQuery += " FROM (SELECT ";
		m_strSQLBaseQuery += strSecondSelectList;
		m_strSQLBaseQuery += ",ROW_NUMBER() OVER (ORDER BY ";

		if (!strSQLORDERBYClause.empty())
			m_strSQLBaseQuery += strSQLORDERBYClause;
		else
			m_strSQLBaseQuery += strSQLPKCols;

		m_strSQLBaseQuery += ") R FROM ";
		m_strSQLBaseQuery += m_pMetaEntity->GetName();

		if (!strWhereClause.empty())
			m_strSQLBaseQuery += strWhereClause;

		switch (m_pMetaEntity->GetMetaDatabase()->GetTargetRDBMS())
		{
			case Oracle:
				m_strSQLBaseQuery += ") WHERE R BETWEEN ";
				break;

			case SQLServer:
				m_strSQLBaseQuery += ") tmp WHERE tmp.R BETWEEN ";
				break;
		}

		m_bInitialised = true;
	}




	bool OraclePagedResultSet::GetPage(unsigned long uPageNo, unsigned long uPageSize)
	{
		std::ostringstream	osql;


		if (!m_bInitialised)
			return false;

		// Remember page size
		SetPageSize(uPageSize);

		// This should set the total number of records in the query (we do this every time because the db can change between page calls)
		m_pDatabase->ExecuteSingletonSQLCommand(m_strSQLCountQuery, m_uTotalSize);

		DeleteObjectList();

		// Build full query
		osql << m_strSQLBaseQuery << (uPageNo * m_uPageSize) + 1 << " AND " << (uPageNo * m_uPageSize) + m_uPageSize;

		CreateObjectList(m_pDatabase->LoadObjects(m_pMetaEntity, osql.str(), true));

		m_uCurrentPage = uPageNo;
		m_uFirst       = std::min(m_uTotalSize, uPageNo * m_uPageSize + 1);
		m_uLast				 = m_uFirst + (m_pListEntity ? m_pListEntity->size() - 1 : 0);

		return m_pListEntity && !m_pListEntity->empty() ? true : false;
	}





	std::string OraclePagedResultSet::SetKeyColsSQLQueryPart()
	{
		std::ostringstream		osql;
		MetaKeyPtr						pPMK;
		MetaColumnPtrListItr	itrMC;
		MetaColumnPtr					pMC;
		bool									bFirst = true;


		pPMK = m_pMetaEntity->GetPrimaryMetaKey();

		for (	itrMC =		pPMK->GetMetaColumns()->begin();
					itrMC !=	pPMK->GetMetaColumns()->end();
					itrMC++)
		{
			pMC = *itrMC;

			if (bFirst)
				bFirst = false;
			else
				osql << ",";

			osql << pMC->GetName();
		}

		return osql.str();
	}










	// ==========================================================================
	// SQLServerPagedResultSet class implementation
	//

	// Statics
	boost::recursive_mutex	SQLServerPagedResultSet::M_mtxExclusive;


	// Standard D3 stuff
	//
	D3_CLASS_IMPL(SQLServerPagedResultSet, ResultSet);



	/* static */
	ResultSetPtr SQLServerPagedResultSet::Create(	DatabasePtr					pDB,
																								MetaEntityPtr				pME,
																								const std::string&	strSQLWHEREClause,
																								const std::string	& strSQLORDERBYClause,
																								unsigned long				uPageSize)
	{
		if (pME->GetMetaDatabase()->GetTargetRDBMS() != SQLServer)
			throw Exception(__FILE__, __LINE__, Exception_error, "SQLServerPagedResultSet::ctor() can only be called for entities in SQLServer databases.");

		SQLServerPagedResultSet*		pRS = new SQLServerPagedResultSet(pDB, pME);

		pRS->m_uPageSize = uPageSize;
		pRS->CreateProcedure(strSQLWHEREClause, strSQLORDERBYClause);

		return pRS;
	}





	SQLServerPagedResultSet::~SQLServerPagedResultSet()
	{
		// This class uses temporary stored procedures to accomplish it's goal.
		// Temporary stored procedures are deleted automatically by SQLServer when
		// the connection is closed. The connection that created the temporary
		// stored procedure is closed implicitly when this m_dbWS destructor is called.
		DropProcedure();
	}




	bool SQLServerPagedResultSet::GetPage(unsigned long uPageNo, unsigned long uPageSize)
	{
		assert(m_bInitialised);
		DeleteObjectList();

		// Remember page size
		SetPageSize(uPageSize);

		CreateObjectList(m_pDatabase->CallPagedObjectSP(this, m_pMetaEntity, uPageNo, m_uPageSize, m_uFirst, m_uLast, m_uTotalSize));

		m_uCurrentPage = uPageNo;
		m_uFirst       = std::min(m_uTotalSize, uPageNo * m_uPageSize + 1);
		m_uLast				 = m_uFirst + (m_pListEntity ? m_pListEntity->size() - 1 : 0);

		return m_pListEntity && !m_pListEntity->empty() ? true : false;
	}





	std::string	SQLServerPagedResultSet::BuildProcedure(const std::string & strWhereClause, const std::string & strOrderByClause)
	{
		switch (m_pMetaEntity->GetMetaDatabase()->GetTargetRDBMS())
		{
			case SQLServer:
			{
				std::ostringstream		osql;
				MetaColumnPtrListItr	itr;
				MetaColumnPtr					pMC;
				std::string						strPKCreateCols, strPKCols, strPKCompare;
				SessionPtr						pSession = GetSession();
				bool									bFirst = true;


				// Build the primary key column list needed in the stored procedure query
				for ( itr =  m_pMetaEntity->GetPrimaryMetaKey()->GetMetaColumns()->begin();
							itr != m_pMetaEntity->GetPrimaryMetaKey()->GetMetaColumns()->end();
							itr++)
				{
					if (itr != m_pMetaEntity->GetPrimaryMetaKey()->GetMetaColumns()->begin())
					{
						strPKCreateCols += ',';
						strPKCols += ',';
						strPKCompare += " AND ";
					}

					pMC = *itr;

					strPKCreateCols += pMC->GetName();
					strPKCreateCols += " ";
					strPKCreateCols += pMC->GetTypeAsSQLString(m_pMetaEntity->GetMetaDatabase()->GetTargetRDBMS());

					strPKCols += pMC->GetName();

					if (pMC->IsMandatory())
					{
						strPKCompare += "a.";
						strPKCompare += pMC->GetName();
						strPKCompare += " = t.";
						strPKCompare += pMC->GetName();
					}
					else
					{
						strPKCompare += "(";
						strPKCompare += "a.";
						strPKCompare += pMC->GetName();
						strPKCompare += " = t.";
						strPKCompare += pMC->GetName();
						strPKCompare += " OR ";
						strPKCompare += "(a.";
						strPKCompare += pMC->GetName();

						strPKCompare += " IS NULL AND t.";
						strPKCompare += pMC->GetName();
						strPKCompare += " IS NULL))";
					}
				}

				// clear the stream
				osql.str("");

				// Build the procedure
				osql << "CREATE PROCEDURE " << GetStoredProcedureName() << " (@PageNo INT, @PageSize INT) AS\n";
				osql << "BEGIN" << std::endl;
				osql << "  SET NOCOUNT ON" << std::endl;
				osql << std::endl;
				osql << "  DECLARE @Temp" << m_pMetaEntity->GetName() << " TABLE ([tmp ID] INT identity, " << strPKCreateCols << ")\n";
				osql << std::endl;
				osql << "  INSERT INTO @Temp" << m_pMetaEntity->GetName() << " (" << strPKCols << ") SELECT " << strPKCols << " FROM " << m_pMetaEntity->GetName() << "  WITH(NOLOCK)";

				if (!strWhereClause.empty())
					osql << " WHERE " << strWhereClause;

				if (pSession && !pSession->GetRoleUser()->GetUser()->GetRLSPredicate(m_pMetaEntity).empty())
					if (!strWhereClause.empty())
						osql << " AND " << pSession->GetRoleUser()->GetUser()->GetRLSPredicate(m_pMetaEntity);
					else
						osql << " WHERE " << pSession->GetRoleUser()->GetUser()->GetRLSPredicate(m_pMetaEntity);

				if (!strOrderByClause.empty())
					osql << " ORDER BY " << strOrderByClause;

				osql << std::endl;

				osql << std::endl;
				osql << "  SELECT (@PageNo * @PageSize + 1) AS 'Start', (@PageNo + 1) * @PageSize AS 'End', (SELECT COUNT(*) FROM @Temp" << m_pMetaEntity->GetName() << ") AS 'Total'" << std::endl;
				osql << std::endl;
				osql << "  SELECT ";
				osql << m_pMetaEntity->AsSQLSelectList(true, "a");
				osql << " FROM " << m_pMetaEntity->GetName() << " a WITH(NOLOCK) INNER JOIN @Temp" << m_pMetaEntity->GetName() << " t on " << strPKCompare << " WHERE t.[tmp ID] BETWEEN (@PageNo * @PageSize + 1) AND ((@PageNo + 1) * @PageSize)" << std::endl;
				osql << std::endl;
				osql << "END" << std::endl;

				return osql.str();
			}

			case Oracle:
			default:
				throw Exception(__FILE__, __LINE__, Exception_error, "SQLServerPagedResultSet::BuildProcedure(): Only supported for SQLServer");
		}

		return "";
	}





	void SQLServerPagedResultSet::CreateProcedure(const std::string & strWhereClause, const std::string & strOrderByClause)
	{
		std::string				strSQL;
		DatabasePtr				pDB = m_dbWS.GetDatabase(m_pDatabase->GetMetaDatabase());

		strSQL = BuildProcedure(strWhereClause, strOrderByClause);

		pDB->ExecuteSQLReadCommand(strSQL);

		m_bInitialised = true;

	}





	void SQLServerPagedResultSet::DropProcedure()
	{
		try
		{
			if (m_bInitialised)
			{
				std::string				strSQL("DROP PROCEDURE ");
				DatabasePtr				pDB = m_dbWS.GetDatabase(m_pDatabase->GetMetaDatabase());

				strSQL += GetStoredProcedureName();

				pDB->ExecuteSQLReadCommand(strSQL);

				m_bInitialised = false;
			}
		}
		catch (...)
		{
			// don't worry if this fails, the procedure will die automatically with the connection
			ReportDiagnostic("SQLServerPagedResultSet::DropProcedure(): Error dropping procedure %s", m_strProcName.c_str());
		}
	}





	DatabasePtr	SQLServerPagedResultSet::GetStoredProcedureDatabase()
	{
		return m_dbWS.GetDatabase(m_pDatabase->GetMetaDatabase());
	}





	const std::string& SQLServerPagedResultSet::GetStoredProcedureName()
	{
		if (m_strProcName.empty())
		{
			boost::recursive_mutex::scoped_lock		lk(M_mtxExclusive);

			static unsigned long		id = 0;
			std::ostringstream			osql;

			osql << APAL_SQLSERVER_TSP_PREFIX << id++;

			m_strProcName = osql.str();
		}

		return m_strProcName;
	}





} // end namespace D3
