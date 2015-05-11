// MODULE: OTLDatabase source
//;
//; IMPLEMENTATION CLASS: OTLDatabase
//;
// ===========================================================
// File Info:
//
// $Author: pete $
// $Date: 2014/10/09 03:24:56 $
// $Revision: 1.52 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 25/04/06 - 1.9 - Hugp
//
// Added default values to meta columns and added code here
// to use this value if non is provided in the import file
//
//
//
// 03/09/03 - R1 - Hugp
//
// Created
//
// -----------------------------------------------------------

#include "D3Types.h"

#ifdef APAL_SUPPORT_OTL			// Skip entire file if no OTL support is wanted

#include "OTLDatabase.h"
#include "Entity.h"
#include "Column.h"
#include "Key.h"
#include "Relation.h"
#include "ResultSet.h"
#include "D3Date.h"
#include "Exception.h"
#include "XMLImporterExporter.h"
#include "HSTopic.h"

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

#include "Codec.h"

#include <math.h>

namespace D3
{
	#ifdef _MONITOROTLFUNC

		class MonitorOTLFunc
		{
			protected:
				void*					m_pObject;
				std::string		m_strMessage;

			public:
				MonitorOTLFunc(char* szMsg, void* pObj, int iTC)
					: m_pObject(pObj)
				{
					m_strMessage = szMsg;

					for (unsigned int idx = 36 - strlen(szMsg); idx > 0; idx--)
						m_strMessage += '.';

					ReportDiagnostic("%s: Entered by database %x (Transaction count: %u)", m_strMessage.c_str(), m_pObject, iTC);
				}

				~MonitorOTLFunc()
				{
					ReportDiagnostic("%s: Exited  by database %x", m_strMessage.c_str(), m_pObject);
				}
		};

		#define MONITORFUNC(msg, db, tc) MonitorOTLFunc mf(msg, db, tc)

	#else

		#define MONITORFUNC(msg, db, tc)

	#endif


	//===========================================================================
	//
	// This module subclasses XMLImportFileProcessor in order to deal with specifics
	// applicable when importing data into an OTL database
	//
	class OTLXMLImportFileProcessor : public XMLImportFileProcessor
	{
		protected:
			struct ColumnData
			{
				enum State
				{
					undefined,
					null,
					defined
				};
				State						state;
				std::string			value;
				unsigned char*	pData;

				ColumnData() : state(undefined), pData(NULL) {};
				~ColumnData()
				{
					Clear();
				}

				void	Clear()
				{
					state = undefined;
					if (pData)
					{
						delete [] pData;
						pData = NULL;
					}
				}
			};

			typedef std::map<string, ColumnData>	ColumnDataMap;
			typedef ColumnDataMap::iterator				ColumnDataMapItr;

			ColumnDataMap								m_mapColumnData;
			OTLStreamPoolPtr						m_pStrmPool;
			OTLStreamPool::OTLStream*		m_pStrm;
			OTLDatabase*								m_pDB;
			unsigned long								m_lMaxValue;
			int													m_iErrorCount;

			// Trap notifications
			//
			virtual void		On_BeforeProcessEntityListElement();
			virtual void		On_BeforeProcessEntityElement();

			virtual void		On_AfterProcessEntityListElement();
			virtual void		On_AfterProcessEntityElement();
			virtual void		On_AfterProcessColumnElement();

		public:
			OTLXMLImportFileProcessor(const std::string & strAppName, OTLDatabase* pDB, MetaEntityPtrListPtr pListME)
			 :	XMLImportFileProcessor(strAppName, pDB, pListME),
					m_pDB(pDB),
					m_pStrmPool(NULL),
					m_pStrm(NULL),
					m_iErrorCount(0)
			{}

			void						SetValue(MetaColumnPtr pMC, ColumnData & colData);

			void						TidyUp(const char * pszMsg);

			int							GetErrorCount()	{ return m_iErrorCount; }
	};



	void OTLXMLImportFileProcessor::On_BeforeProcessEntityListElement()
	{
		MetaColumnPtrVect::iterator			itrMEC;
		MetaColumnPtr										pMC;
		bool														bSuccess = false;


		XMLImportFileProcessor::On_BeforeProcessEntityListElement();

		if (!m_bSkipToNextSibling)
		{
			// Lets create ColumnData structures for all columns the meta entity knows
			m_mapColumnData.clear();

			if (m_pCurrentME)
			{
				for ( itrMEC =  m_pCurrentME->GetMetaColumnsInFetchOrder()->begin();
							itrMEC != m_pCurrentME->GetMetaColumnsInFetchOrder()->end();
							itrMEC++)
				{
					pMC = *itrMEC;

					ColumnData&	colData = m_mapColumnData[pMC->GetName()];
				}
			}

			// Fetch a stream pool for this operation
			m_pStrmPool = m_pDB->GetOTLStreamPool(m_pCurrentME);
			m_pStrm = m_pStrmPool->FetchInsertStream();

			try
			{
				m_pDB->BeforeImportData(m_pCurrentME);

				m_pDB->BeginTransaction();

				m_lMaxValue = 0;

				bSuccess = true;
			}
			catch(...)
			{
				D3::GenericExceptionHandler("OTLXMLImportFileProcessor::On_BeforeProcessEntityListElement: Caught exception.");
				m_iErrorCount++;
			}

			if (!bSuccess)
				throw CSAXException("OTLXMLImportFileProcessor::On_BeforeProcessEntityListElement: Function failed, import terminated.");
		}
	}



	void OTLXMLImportFileProcessor::On_BeforeProcessEntityElement()
	{
		ColumnDataMapItr								itrCD;


		XMLImportFileProcessor::On_BeforeProcessEntityElement();

		// Lets mark all ColumnData in our structure as undefined
		for ( itrCD =  m_mapColumnData.begin();
					itrCD != m_mapColumnData.end();
					itrCD++)
		{
			ColumnData&	colData = itrCD->second;
			colData.Clear();
		}
	}



	void OTLXMLImportFileProcessor::On_AfterProcessEntityListElement()
	{
		if (!m_bSkipToNextSibling)
		{
			try
			{
				m_pDB->CommitTransaction();
				m_pDB->AfterImportData(m_pCurrentME, m_lMaxValue);

				// Free stream we stole from pool
				if (m_pStrmPool && m_pStrm)
					m_pStrmPool->ReleaseInsertStream(m_pStrm);

				m_pStrmPool = NULL;
				m_pStrm = NULL;
			}
			catch (...)
			{
				// Free stream we stole from pool
				if (m_pStrmPool && m_pStrm)
					m_pStrmPool->ReleaseInsertStream(m_pStrm);

				m_pStrmPool = NULL;
				m_pStrm = NULL;

				m_iErrorCount++;
				D3::GenericExceptionHandler("OTLXMLImportFileProcessor::On_AfterProcessEntityListElement(): Error caught.");
				TidyUp("OTLXMLImportFileProcessor::On_AfterProcessEntityListElement()");
			}
		}

		XMLImportFileProcessor::On_AfterProcessEntityListElement();
	}



	void OTLXMLImportFileProcessor::On_AfterProcessEntityElement()
	{
		if (!m_bSkipToNextSibling)
		{
			try
			{
				MetaColumnPtrVect::iterator			itrMEC;
				MetaColumnPtr										pMC;


				// We need to stream the attributes in the correct order
				for ( itrMEC =  m_pCurrentME->GetMetaColumnsInFetchOrder()->begin();
							itrMEC != m_pCurrentME->GetMetaColumnsInFetchOrder()->end();
							itrMEC++)
				{
					pMC = *itrMEC;

					ColumnData&	colData = m_mapColumnData[pMC->GetName()];

					SetValue(pMC, colData);
				}
			}
			catch(otl_exception& e)
			{
				if (e.code == 1)
				{
					// constraint violation (typically caused by duplicates)
					// issue warning, skip record and continue
					ReportWarning("OTLXMLImportFileProcessor::On_AfterProcessEntityElement(): Failed to execute %s, record skipped. Oracle error: %s", m_pStrm->GetSQL().c_str(), (char*) e.msg);
					m_lRecCountCurrent--;
				}
				else
				{
					ReportError("OTLXMLImportFileProcessor::On_AfterProcessEntityElement(): Failed to execute %s. %s", m_pStrm->GetSQL().c_str(), e.msg);
					TidyUp("OTLXMLImportFileProcessor::On_AfterProcessEntityElement()");
				}
			}
			catch (...)
			{
				m_iErrorCount++;
				D3::GenericExceptionHandler("OTLXMLImportFileProcessor::On_AfterProcessEntityElement(): Error caught.");
				TidyUp("OTLXMLImportFileProcessor::On_AfterProcessEntityElement()");
			}
		}

		XMLImportFileProcessor::On_AfterProcessEntityElement();
	}



	void OTLXMLImportFileProcessor::On_AfterProcessColumnElement()
	{
		if (!m_bSkipToNextSibling)
		{
			ColumnData&		colData = m_mapColumnData[m_CurrentPath.back().m_strName];

			if (m_CurrentPath.back().GetAttribute("NULL") == "0")
			{
				colData.value	= m_CurrentPath.back().m_strValue;
				colData.state	= ColumnData::defined;
			}
			else
			{
				colData.state = ColumnData::null;
			}
		}

		XMLImportFileProcessor::On_AfterProcessColumnElement();
	}



	void OTLXMLImportFileProcessor::SetValue(MetaColumnPtr pMC, ColumnData& colData)
	{
		bool			bHandled = false;

		try
		{
			// Deal with strings fist:
			// 1. decode encoded strings
			// 2. remove trailing spaces
			// 3. if empty, treat like NULL
			if (colData.state == ColumnData::defined && pMC->GetType() == MetaColumn::dbfString)
			{
				if (pMC->IsEncodedValue())
					colData.value = APALUtil::base64_decode(colData.value);

				// Remove trailing spaces (SQL Server can't handle this)
				size_t	posLastNonBlank = colData.value.find_last_not_of(' ');

				if (posLastNonBlank != std::string::npos && posLastNonBlank < colData.value.size() - 1)
				{
					ReportWarning("OTLXMLImportFileProcessor::SetValue(): Removing trailing blanks from value '%s' for column %s.", colData.value.c_str(), pMC->GetFullName().c_str());
					colData.value.erase(posLastNonBlank + 1);
				}

				// If value is too big, truncate it and report a warning
				if (colData.value.length() > pMC->GetMaxLength())
				{
					ReportWarning("OTLXMLImportFileProcessor::SetValue(): Value for column %s too long and has been truncated.", pMC->GetFullName().c_str());
					colData.value.resize(pMC->GetMaxLength());
				}

				// Treat like NULL if the length is 0
				if (colData.value.empty())
					colData.state = ColumnData::null;
			}

			if (colData.state != ColumnData::defined)
			{
				if (pMC->IsMandatory())
				{
					if (pMC->IsAutoNum())
					{
						// Use the record counter for AutoNum columns missing in the import file
						Convert(colData.value, m_lRecCountCurrent + 1);
						colData.state = ColumnData::defined;
					}
					else if (pMC->IsDefaultValueNull())
					{
						// Use default value
						colData.value = pMC->GetDefaultValue();
						colData.state = ColumnData::defined;
					}
					else
					{
						// Provide some sensible default
						switch (pMC->GetType())
						{
							case MetaColumn::dbfString:
							case MetaColumn::dbfBlob:
								colData.value = "";
								colData.state = ColumnData::defined;
								break;

							case MetaColumn::dbfChar:
							case MetaColumn::dbfBool:
							case MetaColumn::dbfShort:
							case MetaColumn::dbfInt:
							case MetaColumn::dbfLong:
							case MetaColumn::dbfFloat:
							case MetaColumn::dbfBinary:
								colData.value = "0";
								colData.state = ColumnData::defined;
								break;

							case MetaColumn::dbfDate:
							{
								D3Date			dt;
								colData.value = dt.AsString();
								colData.state = ColumnData::defined;
								break;
							}
						}
					}
				}
				else
				{
					colData.state = ColumnData::null;
				}
			}

			if (colData.state == ColumnData::null)
			{
				if (pMC->IsStreamed())
					m_pStrm->GetNativeStream() << otl_long_string();
				else
					m_pStrm->GetNativeStream() << otl_null();

				bHandled = true;
			}
			else
			{
				if (pMC->IsStreamed())
				{
					switch (pMC->GetType())
					{
						case MetaColumn::dbfString:
						{
							m_pStrm->GetNativeStream() << otl_long_string(colData.value.c_str(), colData.value.size(), colData.value.size());

							bHandled = true;
							break;
						}

						case MetaColumn::dbfBlob:
						{
							int							lng;
							unsigned char*	pDecoded = NULL;

							try
							{
								colData.pData = APALUtil::base64_decode(colData.value, lng);
								m_pStrm->GetNativeStream() << otl_long_string(colData.pData, lng, lng);
							}
							catch (...)
							{
								delete pDecoded;
								throw;
							}

							bHandled = true;
							break;
						}
					}
				}
				else
				{
					switch (pMC->GetType())
					{
						case MetaColumn::dbfString:
						{
							m_pStrm->GetNativeStream() << colData.value;

							bHandled = true;
							break;
						}

						case MetaColumn::dbfChar:
						{
							char			c;
							short			i;
							Convert(c, colData.value);
							i = c;
							m_pStrm->GetNativeStream() << i;
							bHandled = true;
							break;
						}

						case MetaColumn::dbfBool:
						{
							bool			b;
							short			i;
							Convert(b, colData.value);
							i = b ? 1 : 0;
							m_pStrm->GetNativeStream() << i;
							bHandled = true;
							break;
						}

						case MetaColumn::dbfShort:
						{
							short			i;
							Convert(i, colData.value);
							m_pStrm->GetNativeStream() << i;
							bHandled = true;
							break;
						}

						case MetaColumn::dbfInt:
						{
							int			i;
							Convert(i, colData.value);
							m_pStrm->GetNativeStream() << i;
							bHandled = true;
							break;
						}

						case MetaColumn::dbfLong:
						{
							long			i;
							Convert(i, colData.value);
							m_pStrm->GetNativeStream() << i;
							bHandled = true;
							break;
						}

						case MetaColumn::dbfFloat:
						{
							float			i;
							Convert(i, colData.value);
							m_pStrm->GetNativeStream() << i;
							bHandled = true;
							break;
						}

						case MetaColumn::dbfDate:
						{
							D3Date				dt(colData.value);
							m_pStrm->GetNativeStream() << (otl_datetime) dt;
							bHandled = true;
							break;
						}

						case MetaColumn::dbfBinary:
						{
							unsigned int			nSize;

							try
							{
								colData.pData = new unsigned char [pMC->GetMaxLength()];

								nSize = APALUtil::base64_decode((unsigned char *) colData.value.c_str(), colData.value.size(), colData.pData, pMC->GetMaxLength());

								m_pStrm->GetNativeStream() << otl_long_string(colData.pData, pMC->GetMaxLength(), nSize);
								bHandled = true;
							}
							catch (...)
							{
								D3::Exception::GenericExceptionHandler(__FILE__, __LINE__);
							}

							break;
						}
					}
				}

				if (pMC->IsAutoNum())
				{
					unsigned long l;
					Convert(l, colData.value);
					m_lMaxValue = std::max(m_lMaxValue, l);
				}
			}

			if (!bHandled)
				std::cout << "Did not process column " << pMC->GetMetaEntity()->GetName() << "." << pMC->GetName() << std::endl;

		}
		catch(...)
		{
			m_iErrorCount++;
			D3::GenericExceptionHandler("OTLXMLImportFileProcessor::SetValue(): Failed to set value '%s' for column %s.", colData.value.c_str(), pMC->GetName().c_str());
		}
	}



	void OTLXMLImportFileProcessor::TidyUp(const char * pszMsg)
	{
		std::string			strMessage = "Error occurred in ";

		strMessage += pszMsg;

		try
		{
			if (m_pDB->HasTransaction())
				m_pDB->RollbackTransaction();

			m_pDB->AfterImportData(m_pCurrentME, 0);
		}
		catch(...){}

		throw CSAXException("Error in OTLXMLImportFileProcessor::On_BeforeProcessEntityListElement.");
	}





	//===========================================================================
	//
	// OTLColumn implementation
	//


	OTLColumn::OTLColumn()
		: m_pMetaColumn(NULL),
			m_szName(NULL),
			m_iType(MetaColumn::dbfUndefined),
			m_bNull(0),
			m_bLazyFetch(true)
	{
	}



	OTLColumn::~OTLColumn()
	{
		switch ((MetaColumn::Type) m_iType)
		{
			case MetaColumn::dbfString:
				delete m_Value.pstrVal;
				break;

			case MetaColumn::dbfDate:
				delete m_Value.pdtVal;
				break;

			case MetaColumn::dbfBinary:
			case MetaColumn::dbfBlob:
				delete m_Value.otlLOB;
				break;

		}
	}




	void OTLColumn::Init(const otl_column_desc* desc, MetaColumnPtr pMC, bool bLazyFetch)
	{
		// Use the OTL supplied name
		//
		m_szName = (char*) desc->name;
		m_bLazyFetch = bLazyFetch;
		m_pMetaColumn = pMC;

		if (m_pMetaColumn)
		{
			if (m_pMetaColumn->IsLazyFetch() && bLazyFetch)
			{
				m_iType = MetaColumn::dbfBool;

				if (ToUpper(std::string("Has") + m_pMetaColumn->GetName()) != ToUpper(m_szName))
					throw Exception(__FILE__, __LINE__, Exception_error, "OTLColumn::Init(): Column names mismatch. Column named %s from the OTL stream mismatches with expected D3 column HAS%s.", (ToUpper(m_szName)).c_str(), ToUpper(pMC->GetName()).c_str());
			}
			else
			{
				m_iType = m_pMetaColumn->GetType();

				switch ((MetaColumn::Type) m_iType)
				{
					case MetaColumn::dbfString:
						m_Value.pstrVal = new std::string;
						break;

					case MetaColumn::dbfDate:
						m_Value.pdtVal = new D3Date;
						break;

					case MetaColumn::dbfBlob:
						m_Value.otlLOB = new otl_long_string(D3_MAX_LOB_SIZE);
						break;

					case MetaColumn::dbfBinary:
						m_Value.otlLOB = new otl_long_string(m_pMetaColumn->GetMaxLength());
						break;
				}

				if (ToUpper(m_pMetaColumn->GetName()) != ToUpper(m_szName))
					throw Exception(__FILE__, __LINE__, Exception_error, "OTLColumn::Init(): Column names mismatch. Column named %s from the OTL stream mismatches with expected D3 column %s.", (ToUpper(m_szName)).c_str(), ToUpper(pMC->GetName()).c_str());
			}

			return;
		}

		// Convert the OTL supplied data type to the D3 data type and if need be,
		// allocate memory for the value
		//
		switch (desc->otl_var_dbtype)
		{
			case otl_var_char:					// null terminated string
				m_iType = MetaColumn::dbfString;
				m_Value.pstrVal = new std::string;
				break;

			case otl_var_double:				// 8-byte floating point number
				m_iType = MetaColumn::dbfFloat;
				break;

			case otl_var_float:					// 4-byte floating point number
				m_iType = MetaColumn::dbfFloat;
				break;

			case otl_var_int:						// 32-bit signed integer
			case otl_var_unsigned_int:	// 32-bit unsigned integer
				m_iType = MetaColumn::dbfInt;
				break;

			case otl_var_long_int:			// 32-signed integer
				m_iType = MetaColumn::dbfLong;
				break;

			case otl_var_short:					// 16-bit signed integer
				m_iType = MetaColumn::dbfShort;
				break;

			case otl_var_timestamp:			// datatype that is mapped into TIMESTAMP_STRUCT, ODBC and DB2-CLI only
				m_iType = MetaColumn::dbfDate;
				m_Value.pdtVal = new D3Date;
				break;

			case otl_var_varchar_long:	// datatype that is mapped into LONG in Oracle 7/8, TEXT in MS SQL Server and Sybase, CLOB in DB2
			case otl_var_bigint:				// MS SQL Server, DB2, MySQL, PostgreSQL, etc. BIGINT (64-bit signed integer) type
			case otl_var_blob:					// datatype that is mapped into BLOB in Oracle 8
				m_iType = MetaColumn::dbfBlob;
				m_Value.otlLOB = new otl_long_string(D3_MAX_LOB_SIZE);
				break;

			case otl_var_raw:
			case otl_var_raw_long:
				m_iType = MetaColumn::dbfBinary;
				m_Value.otlLOB = new otl_long_string(D3_MAX_LOB_SIZE);
				break;

			case otl_var_clob:					// datatype that is mapped into CLOB in Oracle 8
			case otl_var_db2date:				// DB2 DATE type
			case otl_var_db2time:				// DB2 TIME type
			case otl_var_ltz_timestamp:	// Oracle 9i TIMESTAMP WITH LOCAL TIME ZONE type
			case otl_var_tz_timestamp:	// Oracle 9i TIMESTAMP WITH TIME ZONE type
			default:
				throw Exception(__FILE__, __LINE__, Exception_error, "OTLColumn::Init(): Record contains unsupported column otl_var_dbtype &i", desc->otl_var_dbtype);
		}
	}



	void OTLColumn::FetchValue(otl_nocommit_stream& oRslts)
	{
		switch ((MetaColumn::Type) m_iType)
		{
			case MetaColumn::dbfUndefined:
				throw Exception(__FILE__, __LINE__, Exception_error, "OTLColumn::FetchValue(): Attemping to fetch a value from an otl stream into an uninitialised OTLColumn object");

			case MetaColumn::dbfString:
				oRslts >> *(m_Value.pstrVal);
				break;

			case MetaColumn::dbfChar:
				oRslts >> m_Value.lVal;										// oRslts >> m_Value.cVal throws otl_exception
				Convert(m_Value.cVal, m_Value.lVal);
				break;

			case MetaColumn::dbfShort:
				oRslts >> m_Value.sVal;
				break;

			case MetaColumn::dbfBool:
			{
				long lVal;

				oRslts >> lVal;
				m_Value.bVal = lVal == 0 ? false : true;

				break;
			}

			case MetaColumn::dbfInt:
				oRslts >> m_Value.iVal;
				break;

			case MetaColumn::dbfLong:
				oRslts >> m_Value.lVal;
				break;

			case MetaColumn::dbfFloat:
				oRslts >> m_Value.fVal;
				break;

			case MetaColumn::dbfDate:
			{
				otl_datetime		otlDT;

				oRslts >> otlDT;
				if (!oRslts.is_null())
					*(m_Value.pdtVal) = otlDT;

				break;
			}

			case MetaColumn::dbfBlob:
			case MetaColumn::dbfBinary:
			{
				oRslts >> *(m_Value.otlLOB);
				break;
			}

			default:
				m_bNull = true;
				return;
		}

		m_bNull = oRslts.is_null();
	}



	void OTLColumn::AssignToD3Column(ColumnPtr pCol)
	{
		// Some sanity checks
		assert(pCol);
		assert(!m_pMetaColumn || pCol->GetMetaColumn() == m_pMetaColumn);

		pCol->MarkUnfetched();

		// Just assign NULL if this is NULL and return
		//
		if (m_bNull)
		{
			pCol->SetNull();
			pCol->MarkFetched();
			return;
		}

		// LazyFetch columns
		//
		if (m_pMetaColumn && m_pMetaColumn->IsLazyFetch() && m_bLazyFetch)
		{
			bool bHasBLOB = AsBool();

			if (bHasBLOB)
				return;

			pCol->SetNull();
			pCol->MarkFetched();
			return;
		}

		// Conversion depends on pCol's type
		//
		switch (pCol->GetMetaColumn()->GetType())
		{
			case MetaColumn::dbfString:
				if (m_iType == pCol->GetMetaColumn()->GetType())
					pCol->SetValue(*(m_Value.pstrVal));
				else
					pCol->SetValue(AsString());
				break;

			case MetaColumn::dbfChar:
				if (m_iType == pCol->GetMetaColumn()->GetType())
					pCol->SetValue(m_Value.cVal);
				else
					pCol->SetValue(AsChar());
				break;

			case MetaColumn::dbfShort:
				if (m_iType == pCol->GetMetaColumn()->GetType())
					pCol->SetValue(m_Value.sVal);
				else
					pCol->SetValue(AsShort());

				break;

			case MetaColumn::dbfBool:
				if (m_iType == pCol->GetMetaColumn()->GetType())
					pCol->SetValue(m_Value.bVal);
				else
					pCol->SetValue(AsBool());
				break;

			case MetaColumn::dbfInt:
				if (m_iType == pCol->GetMetaColumn()->GetType())
					pCol->SetValue(m_Value.iVal);
				else
					pCol->SetValue(AsInt());

				break;

			case MetaColumn::dbfLong:
				if (m_iType == pCol->GetMetaColumn()->GetType())
					pCol->SetValue(m_Value.lVal);
				else
					pCol->SetValue(AsLong());

				break;

			case MetaColumn::dbfFloat:
				if (m_iType == pCol->GetMetaColumn()->GetType())
					pCol->SetValue(m_Value.fVal);
				else
					pCol->SetValue(AsFloat());

				break;

			case MetaColumn::dbfDate:
				if (m_iType == pCol->GetMetaColumn()->GetType())
					pCol->SetValue(*(m_Value.pdtVal));
				else
					pCol->SetValue(AsDate());

				break;

			case MetaColumn::dbfBlob:
				if (m_iType == pCol->GetMetaColumn()->GetType())
				{
					otl_long_string&	lob(*(m_Value.otlLOB));
					D3::ColumnBlobPtr pBlob = (D3::ColumnBlobPtr) pCol;
					pBlob->WriteRaw((char*) &(lob[0]), lob.len());
				}

				break;

			case MetaColumn::dbfBinary:
				if (m_iType == pCol->GetMetaColumn()->GetType())
				{
					otl_long_string&	lob(*(m_Value.otlLOB));
					D3::ColumnBinaryPtr pBinary = (D3::ColumnBinaryPtr) pCol;
					pBinary->SetValue((unsigned char*) &(lob[0]), lob.len());
				}

				break;
		}

		// Mark this fetched
		pCol->MarkFetched();
	}






	//===========================================================================
	//
	// OTLRecord implementation
	//


	void OTLRecord::Init(otl_nocommit_stream&	otlStream, MetaColumnPtrVectPtr pvectMC, MetaColumnPtrListPtr plistMC, bool bLazyFetch)
	{
		MetaColumnPtr						pMC = NULL;
		otl_column_desc*				desc;
		unsigned int						idx;
		MetaColumnPtrListItr		itrMC;


		// Throw an exception if this is already initialised
		//
		if (m_pOTLStream)
			throw Exception(__FILE__, __LINE__, Exception_error, "OTLRecord::Init(): Object is already initialised!");

		m_pOTLStream = &otlStream;

		// LazyFetch only relevant for SELECTs
		m_bLazyFetch = bLazyFetch;

		// Get result set description
		//
		desc = m_pOTLStream->describe_select(m_nDestCols);

		// Check if the vector or list of meta column has fewer columns than the query returns.
		// Resize m_vectOTLColumn such that it can supply all the data for the destination object
		//
		if (pvectMC)
		{
			m_vectOTLColumn.resize(m_nDestCols < (int) pvectMC->size() ? m_nDestCols : pvectMC->size());
		}
		else if (plistMC)
		{
			m_vectOTLColumn.resize(m_nDestCols < (int) plistMC->size() ? m_nDestCols : plistMC->size());

			itrMC = plistMC->begin();
		}
		else
		{
			m_vectOTLColumn.resize(m_nDestCols);
		}

		for (idx = 0; idx < m_vectOTLColumn.size(); idx++)
		{
			pMC = NULL;

			if (pvectMC)
				pMC = (*pvectMC)[idx];
			else if (plistMC)
				pMC = *itrMC++;

			m_vectOTLColumn[idx].Init(&desc[idx], pMC, m_bLazyFetch);
		}

		// Make sure that empty() reflects the truth
		//
		if (m_pOTLStream && !m_pOTLStream->eof())
			m_bEmpty = false;
	}



/*
	void OTLRecord::Init(otl_nocommit_stream&	otlStream, MetaColumnPtrVectPtr pvectMC, MetaColumnPtrListPtr plistMC)
	{
		MetaColumnPtr						pMC = NULL;
		otl_column_desc*				desc;
		int											desc_len, idx;
		MetaColumnPtrListItr		itrMC;


		// Throw an exception if this is already initialised
		//
		if (m_pOTLStream)
			throw Exception(__FILE__, __LINE__, Exception_error, "OTLRecord::Init(): Object is already initialised!");

		m_pOTLStream = &otlStream;

		// Get result set description
		//
		desc = m_pOTLStream->describe_select(desc_len);

		// resize the OTLColumn vector
		//
		if (desc_len)
			m_vectOTLColumn.resize(desc_len);

		// If a vector or list of meta column is supplied, make sure we have a matching number of columns
		//
		if (pvectMC)
		{
			if (pvectMC->size() != m_vectOTLColumn.size())
			{
				// This could only be OK if we have derived columns
				//
				if (pvectMC->size() < m_vectOTLColumn.size() ||	!(*pvectMC)[m_vectOTLColumn.size()]->IsDerived())
					throw Exception(__FILE__, __LINE__, Exception_error, "OTLRecord::Init(): Vector of D3 MetaColumns has fewer or more columns than the otl stream!");
			}
		}
		else
		{
			if (plistMC)
			{
				if (plistMC->size() != m_vectOTLColumn.size())
					throw Exception(__FILE__, __LINE__, Exception_error, "OTLRecord::Init(): List of D3 MetaColumns has fewer or more columns than the otl stream!");

				itrMC = plistMC->begin();
			}
		}

		for (idx = 0; idx < m_vectOTLColumn.size(); idx++)
		{
			if (pvectMC)
			{
				pMC = (*pvectMC)[idx];
			}
			else
			{
				if (plistMC)
					pMC = *itrMC;
			}

			m_vectOTLColumn[idx].Init(&desc[idx], pMC);

			if (pvectMC)
			{
			}
			else
			{
				if (plistMC)
					itrMC++;
			}
		}

		// Make sure that empty() reflects the truth
		//
		if (m_pOTLStream && !m_pOTLStream->eof())
			m_bEmpty = false;
	}
*/



	bool OTLRecord::Next()
	{
		unsigned int				idx;


		// We can't be at the end of the file
		//
		if (Eof())
			return false;

		// Fetch each column columns
		//
		for (idx = 0; idx < m_vectOTLColumn.size(); idx++)
			m_vectOTLColumn[idx].FetchValue(*m_pOTLStream);

		// Flush columns which we're not interested in
		//
		if ((int) idx < m_nDestCols)
		{
			std::string		strDummy;

			for (; (int) idx < m_nDestCols; idx++)
				(*m_pOTLStream) >> strDummy;
		}

		return true;
	}



	OTLColumn& OTLRecord::operator[](unsigned int idx)
	{
		if (idx > m_vectOTLColumn.size())
			throw Exception(__FILE__, __LINE__, Exception_error, "OTLRecord::operator[]: The index value %i is out-of-bounds!", idx);

		return m_vectOTLColumn[idx];
	}



	OTLColumn& OTLRecord::operator[](const std::string& str)
	{
		std::string			strTS = ToUpper(str);


		// Fetch each column columns
		//
		for (unsigned int idx = 0; idx < m_vectOTLColumn.size(); idx++)
		{
			if (ToUpper(m_vectOTLColumn[idx].GetName()) == strTS)
				return m_vectOTLColumn[idx];
		}

		throw Exception(__FILE__, __LINE__, Exception_error, "OTLRecord::operator[]: The record does not have a column named %s!", str.c_str());
	}




	void OTLRecord::AssignToD3Entity(ColumnPtrVectPtr pvectCol, ColumnPtrListPtr plistCol)
	{
		unsigned int			idx;


		if (!pvectCol && !plistCol)
			throw Exception(__FILE__, __LINE__, Exception_error, "OTLRecord::AssignToD3Entity(): Neither a list nor a vector of instance columns supplied");

		if (pvectCol)
		{
			if (pvectCol->size() != size())
			{
				// This could only be OK if we have derived columns
				//
				if (pvectCol->size() < size() ||	!(*pvectCol)[size()]->GetMetaColumn()->IsDerived())
					throw Exception(__FILE__, __LINE__, Exception_error, "OTLRecord::AssignToD3Entity(): The vector of columns passed in has fewer or more columns than this");
			}

			for (idx = 0; idx < pvectCol->size(); idx++)
				m_vectOTLColumn[idx].AssignToD3Column((*pvectCol)[idx]);
		}
		else
		{
			ColumnPtr						pCol;
			ColumnPtrListItr		itrCol;


			if (plistCol->size() != size())
				throw Exception(__FILE__, __LINE__, Exception_error, "OTLRecord::AssignToD3Entity(): The list columns passed in has fewer or more columns than this");

			for ( itrCol =  plistCol->begin(), idx = 0;
						itrCol != plistCol->end();
						itrCol++, idx++)
			{
				pCol = *itrCol;
				m_vectOTLColumn[idx].AssignToD3Column(pCol);
			}
		}
	}



	std::string OTLColumn::AsString()
	{
		std::string outVal;


		switch ((MetaColumn::Type) m_iType)
		{
			case MetaColumn::dbfString:
				outVal = *(m_Value.pstrVal);
				break;

			case MetaColumn::dbfChar:
				Convert(outVal, m_Value.cVal);
				break;

			case MetaColumn::dbfShort:
				Convert(outVal, m_Value.sVal);
				break;

			case MetaColumn::dbfBool:
				Convert(outVal, m_Value.bVal);
				break;

			case MetaColumn::dbfInt:
				Convert(outVal, m_Value.iVal);
				break;

			case MetaColumn::dbfLong:
				Convert(outVal, m_Value.lVal);
				break;

			case MetaColumn::dbfFloat:
				Convert(outVal, m_Value.fVal);
				break;

			case MetaColumn::dbfDate:
				Convert(outVal, *(m_Value.pdtVal));
				break;
		}

		return outVal;
	}



	char OTLColumn::AsChar()
	{
		char outVal;


		switch ((MetaColumn::Type) m_iType)
		{
			case MetaColumn::dbfString:
				Convert(outVal, *(m_Value.pstrVal));
				break;

			case MetaColumn::dbfChar:
				outVal = m_Value.cVal;
				break;

			case MetaColumn::dbfShort:
				Convert(outVal, m_Value.sVal);
				break;

			case MetaColumn::dbfBool:
				Convert(outVal, m_Value.bVal);
				break;

			case MetaColumn::dbfInt:
				Convert(outVal, m_Value.iVal);
				break;

			case MetaColumn::dbfLong:
				Convert(outVal, m_Value.lVal);
				break;

			case MetaColumn::dbfFloat:
				Convert(outVal, m_Value.fVal);
				break;

			case MetaColumn::dbfDate:
				Convert(outVal, *(m_Value.pdtVal));
				break;
		}

		return outVal;
	}



	short OTLColumn::AsShort()
	{
		short		outVal;


		switch ((MetaColumn::Type) m_iType)
		{
			case MetaColumn::dbfString:
				Convert(outVal, *(m_Value.pstrVal));
				break;

			case MetaColumn::dbfChar:
				Convert(outVal, m_Value.cVal);
				break;

			case MetaColumn::dbfShort:
				outVal = m_Value.sVal;
				break;

			case MetaColumn::dbfBool:
				Convert(outVal, m_Value.bVal);
				break;

			case MetaColumn::dbfInt:
				Convert(outVal, m_Value.iVal);
				break;

			case MetaColumn::dbfLong:
				Convert(outVal, m_Value.lVal);
				break;

			case MetaColumn::dbfFloat:
				Convert(outVal, m_Value.fVal);
				break;

			case MetaColumn::dbfDate:
				Convert(outVal, *(m_Value.pdtVal));
				break;
		}

		return outVal;
	}



	bool OTLColumn::AsBool()
	{
		bool		outVal;


		switch ((MetaColumn::Type) m_iType)
		{
			case MetaColumn::dbfString:
				Convert(outVal, *(m_Value.pstrVal));
				break;

			case MetaColumn::dbfChar:
				Convert(outVal, m_Value.cVal);
				break;

			case MetaColumn::dbfShort:
				Convert(outVal, m_Value.sVal);
				break;

			case MetaColumn::dbfBool:
				outVal = m_Value.bVal;
				break;

			case MetaColumn::dbfInt:
				Convert(outVal, m_Value.iVal);
				break;

			case MetaColumn::dbfLong:
				Convert(outVal, m_Value.lVal);
				break;

			case MetaColumn::dbfFloat:
				Convert(outVal, m_Value.fVal);
				break;

			case MetaColumn::dbfDate:
				Convert(outVal, *(m_Value.pdtVal));
				break;
		}

		return outVal;
	}



	int OTLColumn::AsInt()
	{
		int outVal;


		switch ((MetaColumn::Type) m_iType)
		{
			case MetaColumn::dbfString:
				Convert(outVal, *(m_Value.pstrVal));
				break;

			case MetaColumn::dbfChar:
				Convert(outVal, m_Value.cVal);
				break;

			case MetaColumn::dbfShort:
				Convert(outVal, m_Value.sVal);
				break;

			case MetaColumn::dbfBool:
				Convert(outVal, m_Value.bVal);
				break;

			case MetaColumn::dbfInt:
				outVal = m_Value.iVal;
				break;

			case MetaColumn::dbfLong:
				Convert(outVal, m_Value.lVal);
				break;

			case MetaColumn::dbfFloat:
				Convert(outVal, m_Value.fVal);
				break;

			case MetaColumn::dbfDate:
				Convert(outVal, *(m_Value.pdtVal));
				break;
		}

		return outVal;
	}



	long OTLColumn::AsLong()
	{
		long outVal;


		switch ((MetaColumn::Type) m_iType)
		{
			case MetaColumn::dbfString:
				Convert(outVal, *(m_Value.pstrVal));
				break;

			case MetaColumn::dbfChar:
				Convert(outVal, m_Value.cVal);
				break;

			case MetaColumn::dbfShort:
				Convert(outVal, m_Value.sVal);
				break;

			case MetaColumn::dbfBool:
				Convert(outVal, m_Value.bVal);
				break;

			case MetaColumn::dbfInt:
				Convert(outVal, m_Value.iVal);
				break;

			case MetaColumn::dbfLong:
				outVal = m_Value.lVal;
				break;

			case MetaColumn::dbfFloat:
				Convert(outVal, m_Value.fVal);
				break;

			case MetaColumn::dbfDate:
				Convert(outVal, *(m_Value.pdtVal));
				break;
		}

		return outVal;
	}



	float OTLColumn::AsFloat()
	{
		float outVal;


		switch ((MetaColumn::Type) m_iType)
		{
			case MetaColumn::dbfString:
				Convert(outVal, *(m_Value.pstrVal));
				break;

			case MetaColumn::dbfChar:
				Convert(outVal, m_Value.cVal);
				break;

			case MetaColumn::dbfShort:
				Convert(outVal, m_Value.sVal);
				break;

			case MetaColumn::dbfBool:
				Convert(outVal, m_Value.bVal);
				break;

			case MetaColumn::dbfInt:
				Convert(outVal, m_Value.iVal);
				break;

			case MetaColumn::dbfLong:
				Convert(outVal, m_Value.lVal);
				break;

			case MetaColumn::dbfFloat:
				outVal = m_Value.fVal;
				break;

			case MetaColumn::dbfDate:
				Convert(outVal, *(m_Value.pdtVal));
				break;
		}

		return outVal;
	}



	D3Date OTLColumn::AsDate()
	{
		D3Date outVal;


		switch ((MetaColumn::Type) m_iType)
		{
			case MetaColumn::dbfString:
				outVal = *(m_Value.pstrVal);
				break;

			case MetaColumn::dbfChar:
				Convert(outVal, m_Value.cVal);
				break;

			case MetaColumn::dbfShort:
				Convert(outVal, m_Value.sVal);
				break;

			case MetaColumn::dbfBool:
				Convert(outVal, m_Value.bVal);
				break;

			case MetaColumn::dbfInt:
				Convert(outVal, m_Value.iVal);
				break;

			case MetaColumn::dbfLong:
				Convert(outVal, m_Value.lVal);
				break;

			case MetaColumn::dbfFloat:
				Convert(outVal, m_Value.fVal);
				break;

			case MetaColumn::dbfDate:
				outVal = *(m_Value.pdtVal);
				break;
		}

		return outVal;
	}



	std::ostringstream& OTLColumn::WriteToJSONStream(std::ostringstream& ostrm, bool bJSONEncode)
	{
		ostrm << '"' << m_szName << "\":";

		if (m_bNull)
		{
			ostrm << "null";
		}
		else
		{
			switch ((MetaColumn::Type) m_iType)
			{
				case MetaColumn::dbfString:
					if (bJSONEncode)
						ostrm << '"' << JSONEncode(*(m_Value.pstrVal)) << '"';
					else
						ostrm << '"' << *(m_Value.pstrVal) << '"';
					break;

				case MetaColumn::dbfChar:
					ostrm << m_Value.cVal;
					break;

				case MetaColumn::dbfShort:
					ostrm << m_Value.sVal;
					break;

				case MetaColumn::dbfBool:
					ostrm << m_Value.bVal ? "true" : "false";
					break;

				case MetaColumn::dbfInt:
					ostrm << m_Value.iVal;
					break;

				case MetaColumn::dbfLong:
					ostrm << m_Value.lVal;
					break;

				case MetaColumn::dbfFloat:
					ostrm << m_Value.fVal;
					break;

				case MetaColumn::dbfDate:
					ostrm << '"' << m_Value.pdtVal->AsUTCISOString(3) << '"';
					break;

				case MetaColumn::dbfBlob:
				case MetaColumn::dbfBinary:
				{
					otl_long_string&	lob(*(m_Value.otlLOB));
					std::string				strBlob;
					APALUtil::base64_encode(strBlob, (unsigned char*) &(lob[0]), lob.len());
					ostrm << '"' << strBlob << '"';
					break;
				}
			}
		}

		return ostrm;
	}








	//===========================================================================
	//
	// OTLStreamPool implementation
	//
	OTLStreamPool::~OTLStreamPool()
	{
		boost::recursive_mutex::scoped_lock			lock(m_mtxExclusive);
		OTLStreamPtr														pStrm;


		while (!m_listInsertStream.empty())
		{
			pStrm = m_listInsertStream.front();
			m_listInsertStream.pop_front();
			delete pStrm;
		}

		while (!m_listDeleteStream.empty())
		{
			pStrm = m_listDeleteStream.front();
			m_listDeleteStream.pop_front();
			delete pStrm;
		}

		while (!m_listSelectStream.empty())
		{
			pStrm = m_listSelectStream.front();
			m_listSelectStream.pop_front();
			delete pStrm;
		}

		while (!m_listLFSelectStream.empty())
		{
			pStrm = m_listLFSelectStream.front();
			m_listLFSelectStream.pop_front();
			delete pStrm;
		}
	}



	OTLStreamPool::OTLStreamPtr OTLStreamPool::FetchSelectStream(bool bLazyFetch)
	{
		boost::recursive_mutex::scoped_lock			lock(m_mtxExclusive);
		OTLStreamPtr				 										pStrm;
		OTLStreamPtrListPtr											pStreamList;

		if (bLazyFetch)
			pStreamList = &m_listLFSelectStream;
		else
			pStreamList = &m_listSelectStream;

		if (pStreamList->empty())
			return CreateSelectStream(bLazyFetch);

		pStrm = pStreamList->front();
		pStreamList->pop_front();

		return pStrm;
	}



	void OTLStreamPool::ReleaseSelectStream(OTLStreamPool::OTLStreamPtr pStrm)
	{
		boost::recursive_mutex::scoped_lock			lock(m_mtxExclusive);

		if (pStrm->m_bLazyFetch)
			m_listLFSelectStream.push_back(pStrm);
		else
			m_listSelectStream.push_back(pStrm);
	}



	OTLStreamPool::OTLStreamPtr OTLStreamPool::CreateSelectStream(bool bLazyFetch)
	{
		std::string											strSQL;
		MetaKeyPtr											pMetaKey = m_pMetaEntity->GetPrimaryMetaKey();
		MetaColumnPtrList::iterator			itrMKC;
		MetaColumnPtr										pMC;
		char														szColNum[32];
		unsigned int										iColNum = 1;
		bool														bFirst;
		OTLStreamPtr										pStrm;


		// Build SQL (here we ensure that column order is D3 driven instead of by the database)
		//
		strSQL  = "SELECT ";
		strSQL += m_pMetaEntity->AsSQLSelectList(bLazyFetch);
		strSQL += " FROM ";
		strSQL += pMetaKey->GetMetaEntity()->GetName();

		bFirst = true;

		// Build the SQL where clause for the primary metakey
		//
		for ( itrMKC =  pMetaKey->GetMetaColumns()->begin();
					itrMKC != pMetaKey->GetMetaColumns()->end();
					itrMKC++)
		{
			pMC = *itrMKC;

			sprintf(szColNum, "%u", iColNum++);

			if (bFirst)
			{
				strSQL += " WHERE ";
				bFirst = false;
			}
			else
			{
				strSQL += " AND ";
			}

			if (pMC->IsMandatory())
			{
				strSQL += pMC->GetName();
				strSQL += "=:",
				strSQL += szColNum;
			}
			else
			{
				strSQL += "NULLIF(";
				strSQL += pMC->GetName();
				if (pMC->GetType() == MetaColumn::dbfDate)
					strSQL += ",CAST(:";
				else
					strSQL += ",:";
				strSQL += szColNum;
			}

			switch (pMC->GetType())
			{
				case MetaColumn::dbfString:
				{
					char	buff[32];

					strSQL += "<char[";
					sprintf(buff, "%i", pMC->GetMaxLength() + 1);
					strSQL += buff;
					strSQL += "]>";
					break;
				}

				case MetaColumn::dbfChar:
				case MetaColumn::dbfShort:
				case MetaColumn::dbfBool:
					strSQL += "<short>";
					break;

				case MetaColumn::dbfInt:
					strSQL += "<int>";
					break;

				case MetaColumn::dbfLong:
					strSQL += "<long>";
					break;

				case MetaColumn::dbfFloat:
					strSQL += "<float>";
					break;

				case MetaColumn::dbfDate:
					strSQL += "<timestamp>";
					strSQL += " AS TIMESTAMP)";
					break;

				case MetaColumn::dbfBlob:
					strSQL += "<blob>";
					break;

				case MetaColumn::dbfBinary:
				{
					char	buff[32];

					strSQL += "<raw[";
					sprintf(buff, "%i", pMC->GetMaxLength());
					strSQL += buff;
					strSQL += "]>";
					break;
				}
			}

			if (!pMC->IsMandatory())
				strSQL += ") IS NULL";
		}

		pStrm = new OTLStream(strSQL, m_otlConnection);

		return pStrm;
	}



	OTLStreamPool::OTLStreamPtr OTLStreamPool::FetchInsertStream()
	{
		boost::recursive_mutex::scoped_lock			lock(m_mtxExclusive);
		OTLStreamPtr														pStrm;

		if (m_listInsertStream.empty())
			return CreateInsertStream();

		pStrm = m_listInsertStream.front();
		m_listInsertStream.pop_front();

		return pStrm;
	}



	void OTLStreamPool::ReleaseInsertStream(OTLStreamPool::OTLStreamPtr  pStrm)
	{
		boost::recursive_mutex::scoped_lock			lock(m_mtxExclusive);

		m_listInsertStream.push_back(pStrm);
	}



	OTLStreamPool::OTLStreamPtr OTLStreamPool::CreateInsertStream()
	{
		std::string											strSQL, strSQLCols, strSQLVals;
		std::string											strSQLRtrCols, strSQLRtrVals;
		MetaKeyPtr											pMetaKey = m_pMetaEntity->GetPrimaryMetaKey();
		MetaColumnPtrVect::iterator			itrMEC;
		MetaColumnPtr										pMC;
		bool														bFirstCol = true, bFirstStream = true;
		OTLStreamPtr										pStrm;


		// Make sure the connection can support blobs
		m_otlConnection.set_max_long_size(D3_MAX_LOB_SIZE);

		// Build SQL (here we ensure that column order is D3 driven instead of by the database)
		//
		strSQLCols = " COLUMNS(";
		strSQLVals = " VALUES(";

		for ( itrMEC =  m_pMetaEntity->GetMetaColumnsInFetchOrder()->begin();
					itrMEC != m_pMetaEntity->GetMetaColumnsInFetchOrder()->end();
					itrMEC++)
		{
			pMC = *itrMEC;

			// We can stop because derived columns are always at the end of the collection
			if (pMC->IsDerived())
				break;

			// Remember the autonum column (if there is one)
			// NOTE: In contrast to SQL Server where you do not need to provide
			// values for autonum columns, in Oracle values for columns with sequences
			// must be provided as a NULL value. During the insert, a trigger will supply
			// the correct value and the value of the assigned autonum can be obtained
			// through the sequence (e.g. SELECT SQ_T3RUN_ID.CURRVAL FROM DUAL)
			if (bFirstCol)
			{
				bFirstCol = false;
			}
			else
			{
				strSQLCols += ",";
				strSQLVals += ",";
			}

			if (pMC->IsStreamed())
			{
				// Under Oracle, streamed columns must be dealt with in a very unique way
				// INSERT INTO table COLUMNS(a,b,c,d) VALUES(:a<int>,:b<char[20]>,empty_clob(),empty_blob()) returning c,d into :c<clob>,:d<blob>"
				if (bFirstStream)
				{
					bFirstStream = false;
				}
				else
				{
					strSQLRtrCols += ",";
					strSQLRtrVals += ",";
				}

				strSQLCols += pMC->GetName();
				strSQLRtrCols += pMC->GetName();

				switch (pMC->GetType())
				{
					case MetaColumn::dbfString:
					{
						strSQLVals += "empty_clob()";
						strSQLRtrVals += ":";
						strSQLRtrVals += pMC->GetName();
						strSQLRtrVals += "<clob>";
						break;
					}

					case MetaColumn::dbfBlob:
					{
						strSQLVals += "empty_blob()";
						strSQLRtrVals += ":";
						strSQLRtrVals += pMC->GetName();
						strSQLRtrVals += "<blob>";
						break;
					}
				}
			}
			else
			{
				strSQLCols += pMC->GetName();
				strSQLVals += ':';
				strSQLVals += pMC->GetName();

				switch (pMC->GetType())
				{
					case MetaColumn::dbfString:
					{
						char	buff[32];

						strSQLVals += "<char[";
						sprintf(buff, "%i", pMC->GetMaxLength() + 1);
						strSQLVals += buff;
						strSQLVals += "]>";
						break;
					}

					case MetaColumn::dbfChar:
					case MetaColumn::dbfShort:
					case MetaColumn::dbfBool:
						strSQLVals += "<short>";
						break;

					case MetaColumn::dbfInt:
						strSQLVals += "<int>";
						break;

					case MetaColumn::dbfLong:
						strSQLVals += "<long>";
						break;

					case MetaColumn::dbfFloat:
						strSQLVals += "<float>";
						break;

					case MetaColumn::dbfDate:
						strSQLVals += "<timestamp>";
						break;

					case MetaColumn::dbfBlob:
						strSQLVals += "<blob>";
						break;

					case MetaColumn::dbfBinary:
					{
						char	buff[32];

						strSQLVals += "<raw[";
						sprintf(buff, "%i", pMC->GetMaxLength());
						strSQLVals += buff;
						strSQLVals += "]>";
						break;
					}

				}
			}
		}

		strSQLCols += ")";
		strSQLVals += ")";

		strSQL  = "INSERT INTO ";
		strSQL += pMetaKey->GetMetaEntity()->GetName();
		strSQL += strSQLCols;
		strSQL += strSQLVals;

		if (!bFirstStream)
		{
			strSQL += " RETURNING ";
			strSQL += strSQLRtrCols;
			strSQL += " INTO ";
			strSQL += strSQLRtrVals;
		}

		pStrm = new OTLStream(strSQL, m_otlConnection);


		return pStrm;
	}














	//===========================================================================
	//
	// OTLDatabase implementation
	//
	D3_CLASS_IMPL(OTLDatabase, Database);


	static	int	G_otl_initialized = 0;

	OTLDatabase::OTLDatabase()
	 : m_iTransactionCount(0), m_bIsConnected(false), m_iPendingAlertID(-1)
	{
		if (!G_otl_initialized)
		{
			G_otl_initialized = otl_connect::otl_initialize(1); // initialize OCI environment

			if (!G_otl_initialized)
			{
				throw Exception(0,0, Exception_error, "OTLDatabase::OTLDatabase(): otl_initialize failed!");
			}
		}
	}



	OTLDatabase::~OTLDatabase ()
	{
		try
		{
			// We should never receive this message while transactions are pending, but throwing
			// an exception in a destructor is bad practise
			//
			if (HasTransaction())
			{
				RollbackTransaction();
				ReportError("OTLDatabase::~OTLDatabase(): dtor invoked on an instance of MetaDatabase %s with a pending transaction. Transaction rolled back.", m_pMetaDatabase->GetName().c_str());
			}

			// Physically disconnect
			//
			if (IsConnected())
				Disconnect();
		}
		catch(Exception& e)
		{
			e.LogError();
		}
		catch(otl_exception& p)
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::~OTLDatabase(): OTL Error: %s", p.msg);
		}
	}



	// Connect using the associated MeytaDatabase's connection string
	//
	bool OTLDatabase::Connect()
	{
		// We need a connection string
		//
		if (!m_pMetaDatabase || m_pMetaDatabase->GetConnectionString().empty())
		{
			throw Exception(__FILE__, __LINE__, Exception_error, "OTLDatabase::Connect(): Unable to connect to database as MetaDatabase object or connection string is not known.");
		}
		else
		{
			try
			{
				m_oConnection.rlogon(m_pMetaDatabase->GetConnectionString().c_str());
				m_oConnection.auto_commit_off();

				if (m_uTrace & D3DB_TRACE_SELECT)
					ReportInfo("OTLDatabase::Connect().............: Database " PRINTF_POINTER_MASK, this);

				if (!m_pMetaDatabase->VersionChecked() && !CheckDatabaseVersion())
				{
					Disconnect();
					throw Exception(__FILE__, __LINE__, Exception_error, "OTLDatabase::Connect(): Database %s is not of the expected version %s!", m_pMetaDatabase->GetName().c_str(), m_pMetaDatabase->GetVersion().c_str());
				}
			}
			catch(otl_exception& p)
			{
				// Translate exception to D3::Exception
				//
				throw Exception(__FILE__, __LINE__, Exception_error, "OTLDatabase::Connect(): An OTL error occurred connecting to database %s. %s", m_pMetaDatabase->GetName().c_str(), p.msg);
			}
		}

		return true;
	}



	bool OTLDatabase::Disconnect ()
	{
		OTLStreamPoolPtrVect::size_type				idx;


		// Clear the chache
		//
		ClearCache();

		// Delete all stream pools
		//
		for (idx = 0; idx < m_vectOTLStreamPool.size(); idx++)
		{
			if (m_vectOTLStreamPool[idx])
			{
				delete m_vectOTLStreamPool[idx];
				m_vectOTLStreamPool[idx] = NULL;
			}
		}

		if (IsConnected())
		{
			try
			{
				// Rollback all pending transactions
				//
				while (HasTransaction())
					RollbackTransaction();

				// Remove any of the registered alerts
				//
				RemoveAlert();

				m_oConnection.auto_commit_on();
				m_oConnection.logoff();

				if (m_uTrace & D3DB_TRACE_SELECT)
					ReportInfo("OTLDatabase::Disconnect()..........: Database " PRINTF_POINTER_MASK, this);

				m_bIsConnected = false;
			}
			catch(otl_exception & e)
			{
				GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::Disconnect(): An OTL error occurred disconnecting instance of %s. %s", m_pMetaDatabase->GetName().c_str(), e.msg);
				return false;
			}
		}

		return true;
	}



	int OTLDatabase::RegisterAlert(const std::string & strAlertName)
	{
		std::string			strSQL;


		assert(IsConnected());

		if (strAlertName.empty() || GetAlertID(strAlertName) >= 0)
		{
			ReportError("OTLDatabase::RegisterDBAlert(): Alert name '%s' is invalid or already registered.", strAlertName.c_str());
			return -1;
		}

		try
		{
			// Build the sql
			//
			strSQL += "call dbms_alert.register('";
			strSQL += ToUpper(strAlertName);
			strSQL += "')";

			// Registering the Alert
			//
			otl_cursor::direct_exec(m_oConnection, strSQL.c_str());

			// Add the alert name to the list of registered alert names
			//
			m_listAlerts.push_back(strAlertName);

			return m_listAlerts.size() - 1;
		}
		catch (otl_exception & e)
		{
			ReportError("OTLDatabase::RegisterDBAlert(): An OTL error occurred while registering alert %s. %s", e.msg);
		}

		return -1;
	}



	int OTLDatabase::WaitForAlert(const std::string & strAlertName)
	{
		otl_nocommit_stream	oAlertRslt;
		std::string					strSQL;
		char								szName[50];
		char								szMessage[50];
		int									iStatus;


		assert(IsConnected());
		assert(m_iPendingAlertID == -1);

		if (m_listAlerts.empty() || (!strAlertName.empty() && GetAlertID(strAlertName) < 0))
		{
			ReportError("OTLDatabase::WaitForAlert(): Method called without registering the alert");
			return -1;
		}

		try
		{
			m_oConnection.set_max_long_size(D3_MAX_LOB_SIZE);

			// If strAlertName is unspecified, we do a WAITANY otherwise we do a WAITONE
			//
			strSQL = "call dbms_alert";

			if (strAlertName.empty())
			{
				m_iPendingAlertID = 0;

				strSQL += ".waitany(:f1<char[50],out>, :f2<char[50],out>, :f3<int,out>, 10)";

				while (true)
				{
					// Execute the query, fetch result and close resultset
					//
					oAlertRslt.open(1, strSQL.c_str(), m_oConnection);		// Waiting for the requested alert
					oAlertRslt >> szName >> szMessage >> iStatus;
					oAlertRslt.close();

					if (iStatus == 1)
						if (m_iPendingAlertID == -1)
							return -1;

					if (iStatus == 0)
					{
						m_iPendingAlertID = -1;
						return GetAlertID(szName);
					}
				}
			}
			else
			{
				m_iPendingAlertID = GetAlertID(strAlertName);

				strSQL += ".waitone('";
				strSQL += ToUpper(strAlertName);
				strSQL += "', :f1<char[50],out>, :f2<int,out>, 10)";

				while (true)
				{
					// Execute the query, fetch result and close resultset
					//
					oAlertRslt.open(1, strSQL.c_str(), m_oConnection);
					oAlertRslt >> szMessage >> iStatus;
					oAlertRslt.close();

					if (iStatus == 1)
						if (m_iPendingAlertID == -1)
							return -1;

					if (iStatus == 0)
					{
						m_iPendingAlertID = -1;
						return GetAlertID(strAlertName);
					}
				}
			}
		}
		catch(otl_exception & e)
		{
			ReportError("OTLDatabase::WaitForAlert(): OTL error executing %s. %s.", strSQL.c_str(), e.msg);
		}
		catch(...)
		{
		}

		m_iPendingAlertID = -1;

		return -1;
	}



	void OTLDatabase::CancelWaitForAlert()
	{
		std::string					strSQL;


		if (m_iPendingAlertID < 0)
			return;

		assert(IsConnected());
		assert(!GetAlertName(m_iPendingAlertID).empty());

		try
		{
			m_iPendingAlertID = -1;
		}
		catch(otl_exception & e)
		{
			ReportError("OTLDatabase::CancelWaitForAlert(): OTL error executing %s. %s.", strSQL.c_str(), e.msg);
		}
	}



	int OTLDatabase::RemoveAlert(const std::string & strAlertName)
	{
		assert(IsConnected());

		if (m_listAlerts.empty())
			return -1;

		try
		{
			std::string strSQL;


			// If no name is specified, remove all alerts
			//
			strSQL = "call dbms_alert.remove";

			if (strAlertName.empty())
			{
				strSQL += "all";
			}
			else
			{
				if (GetAlertID(strAlertName) < 0)
				{
					ReportError("OTLDatabase::RemoveAlert(): Alert %s is unknown.", strAlertName.c_str());
					return -1;
				}

				strSQL += "('";
				strSQL += ToUpper(strAlertName);
				strSQL += "')";
			}

			otl_cursor::direct_exec(m_oConnection, strSQL.c_str());

			if (strAlertName.empty())
				m_listAlerts.clear();
			else
				m_listAlerts.remove(strAlertName);

			return 0;
		}
		catch (otl_exception & e)
		{
			ReportError("OTLDatabase::RemoveAlert(): An OTL error occurred. %s", e.msg);
		}

		return -1;
	}



	int OTLDatabase::GetAlertID(const std::string & strAlertName)
	{
		StringListItr 		itr;
		int								idx = 0;


		for (	itr  = m_listAlerts.begin();
					itr != m_listAlerts.end();
					itr++)
		{
			if (ToUpper(strAlertName) == ToUpper(*itr))
				return idx;

			idx++;
		}

		return -1;
	}



	std::string OTLDatabase::GetAlertName(int iAlertID)
	{
		StringListItr 		itr;
		int								idx = 0;


		for (	itr  = m_listAlerts.begin();
					itr != m_listAlerts.end();
					itr++)
		{
			if (iAlertID == idx)
				return *itr;

			idx++;
		}

		return "";
	}



	bool OTLDatabase::BeginTransaction()
	{
		if (!m_oConnection.connected)
			return false;

		m_iTransactionCount++;

		if (m_uTrace & (D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT))
			ReportInfo("OTLDatabase::BeginTransaction()....: Database " PRINTF_POINTER_MASK " (Transaction count: %u)", this, m_iTransactionCount);

		return true;
	}



	bool OTLDatabase::CommitTransaction()
	{
		try
		{
			if (!m_iTransactionCount)
				return false;

			m_iTransactionCount--;

			if (!m_iTransactionCount)
				m_oConnection.commit();

			if (m_uTrace & (D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT))
				ReportInfo("OTLDatabase::CommitTransaction()...: Database " PRINTF_POINTER_MASK " (Transaction count: %u) committed.", this, m_iTransactionCount);
		}
		catch(...)
		{
			GetExceptionContext()->GenericExceptionHandler(NULL, 0, "OTLDatabase::CommitTransaction()...: Database " PRINTF_POINTER_MASK " (Transaction count: %u) commit failed.", this, m_iTransactionCount);
			return false;
		}

		if (m_uTrace & (D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT))
			ReportInfo("OTLDatabase::CommitTransaction()...: Database " PRINTF_POINTER_MASK " (Transaction count: %u)", this, m_iTransactionCount);

		return true;
	}



	bool OTLDatabase::RollbackTransaction()
	{
		try
		{
			if (!m_iTransactionCount)
				return true;

			m_iTransactionCount = 0;

			m_oConnection.rollback();

			if (m_uTrace & (D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT))
				ReportInfo("OTLDatabase::RollbackTransaction().: Database " PRINTF_POINTER_MASK " (Transaction count: %u) rolled back.", this, m_iTransactionCount);
		}
		catch(...)
		{
			GetExceptionContext()->GenericExceptionHandler(NULL, 0, "OTLDatabase::RollbackTransaction().: Database " PRINTF_POINTER_MASK " (Transaction count: %u) rollback failed.", this, m_iTransactionCount);
			return false;
		}

		if (m_uTrace & (D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT))
			ReportInfo("OTLDatabase::RollbackTransaction().: Database " PRINTF_POINTER_MASK " (Transaction count: %u)", this, m_iTransactionCount);

		return true;
	}



	bool OTLDatabase::CheckDatabaseVersion()
	{
		MetaEntityPtr				pD3VI;
		std::string					strSQL;
		bool								bResult = false;
		otl_nocommit_stream	oRslts;
		OTLRecord						otlRec;
		bool								bFirst;
		unsigned int				idx;


		pD3VI = GetMetaDatabase()->GetVersionInfoMetaEntity();

		// Return success if this has no version info information
		//
		if (!pD3VI)
			return true;

		switch (this->GetMetaDatabase()->GetTargetRDBMS())
		{
			case SQLServer:
			{
				strSQL  = "SELECT TOP 1 ";
				bFirst = true;

				for (idx = 0; idx < pD3VI->GetMetaColumnsInFetchOrder()->size(); idx++)
				{
					if (!bFirst)
						strSQL += ",";

					strSQL += pD3VI->GetMetaColumnsInFetchOrder()->operator[](idx)->GetName();
					bFirst = false;
				}

				strSQL += " FROM ";
				strSQL += pD3VI->GetName();
				strSQL += " ORDER BY LastUpdated DESC";
				break;
			}

			case Oracle:
			{
				strSQL  = "SELECT ";
				bFirst = true;

				for (idx = 0; idx < pD3VI->GetMetaColumnsInFetchOrder()->size(); idx++)
				{
					if (!bFirst)
						strSQL += ",";

					strSQL += pD3VI->GetMetaColumnsInFetchOrder()->operator[](idx)->GetName();
					bFirst = false;
				}

				strSQL += " FROM (SELECT * FROM ";
				strSQL += pD3VI->GetName();
				strSQL += " ORDER BY LastUpdated DESC, VersionMajor DESC, VersionMinor DESC, VersionRevision DESC) WHERE ROWNUM <=1";
				break;
			}
		}

		try
		{
			// Set the environment correctly
			//
			m_oConnection.set_max_long_size(D3_MAX_LOB_SIZE);

			// Fetch the resultset
			//
			oRslts.open(50, strSQL.c_str(), m_oConnection);

			otlRec.Init(oRslts, pD3VI->GetMetaColumnsInFetchOrder());

			while(otlRec.Next())
			{
				if (m_pMetaDatabase->GetAlias()						!=                otlRec["Name"]            .AsString()	  ||
						m_pMetaDatabase->GetVersionMajor()		!= (unsigned int) otlRec["VersionMajor"]    .AsInt()			||
						m_pMetaDatabase->GetVersionMinor()		!= (unsigned int) otlRec["VersionMinor"]    .AsInt()			||
						m_pMetaDatabase->GetVersionRevision()	!= (unsigned int) otlRec["VersionRevision"] .AsInt())
				{

					break;
				}
				else
				{
					m_pMetaDatabase->VersionChecked(true);
					bResult = true;
					break;
				}
			}
		}
		catch(otl_exception& p)
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::CheckDatabaseVersion(): An OTL+ error occurred executing SQL statement %s. %s", strSQL.c_str(), p.msg);
			return false;
		}
		catch(...)
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::CheckDatabaseVersion(): An unknown exception error occurred executing SQL statement %s. %s", strSQL.c_str());
			return false;
		}

		return bResult;
	}



	EntityPtr OTLDatabase::LoadObjectWithPrimaryKey(KeyPtr pKey, bool bRefresh, bool bLazyFetch)
	{
		OTLStreamPoolPtr									pStrmPool;
		OTLStreamPool::OTLStreamPtr				pStrm;
		ColumnPtrList::iterator						itrC;
		ColumnPtr													pColumn;
		OTLRecord													otlRec;
		EntityPtr													pObject = NULL;


		try
		{
			// Get the OTLStreamPool
			pStrmPool = GetOTLStreamPool(pKey->GetMetaKey()->GetMetaEntity());

			pStrm = pStrmPool->FetchSelectStream();		// Ignores bLazyFetch flag!

			// Push the key
			//
			for ( itrC =  pKey->GetColumns().begin();
						itrC != pKey->GetColumns().end();
						itrC++)
			{
				pColumn = *itrC;

				if (pColumn->IsNull())
				{
					pStrm->GetNativeStream() << otl_null();
				}
				else
				{
					switch (pColumn->GetMetaColumn()->GetType())
					{
						case MetaColumn::dbfString:
							pStrm->GetNativeStream() << pColumn->GetString().c_str();
							break;

						case MetaColumn::dbfChar:
						case MetaColumn::dbfBool:
							pStrm->GetNativeStream() << pColumn->AsShort();
							break;

						case MetaColumn::dbfShort:
							pStrm->GetNativeStream() << pColumn->GetShort();
							break;

						case MetaColumn::dbfInt:
							pStrm->GetNativeStream() << pColumn->GetInt();
							break;

						case MetaColumn::dbfLong:
							pStrm->GetNativeStream() << pColumn->GetLong();
							break;

						case MetaColumn::dbfFloat:
							pStrm->GetNativeStream() << pColumn->GetFloat();
							break;

						case MetaColumn::dbfDate:
							pStrm->GetNativeStream() << (otl_datetime) pColumn->GetDate();
							break;
					}
				}
			}


			if (m_uTrace & D3DB_TRACE_SELECT)
				ReportInfo("OTLDatabase::LoadObjectWithPrimaryKey()......................: Database " PRINTF_POINTER_MASK ", SQL: %s, Key: %s", this, pStrm->GetSQL().c_str(), pKey->AsString().c_str());

			otlRec.Init(pStrm->GetNativeStream(), pKey->GetMetaKey()->GetMetaEntity()->GetMetaColumnsInFetchOrder());

			while(otlRec.Next())
			{
				assert(pObject == NULL);
				pObject = PopulateObject(pKey->GetMetaKey()->GetMetaEntity(), otlRec, bRefresh);
			}

			pStrmPool->ReleaseSelectStream(pStrm);
		}
		catch(otl_exception& p)
		{
			// intercept OTL exceptions
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::LoadObjectWithPrimaryKey(): An OTL error occurred fetching record by primary key. PrimaryKey [Name/Value]: [%s/%s]. OTL message: %s", pKey->GetMetaKey()->GetFullName().c_str(), pKey->AsString().c_str(), p.msg);
		}
		catch(Exception & e)
		{
			e.LogError();
		}
		catch(std::bad_alloc&)
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::LoadObjectWithPrimaryKey(): Out of memory");
		}
		catch(...)
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::LoadObjectWithPrimaryKey(): An unspecified error occurred fetching record by primary key. PrimaryKey [Name/Value]: [%s/%s]", pKey->GetMetaKey()->GetFullName().c_str(), pKey->AsString().c_str());
		}


		return pObject;
	}




	EntityPtr OTLDatabase::LoadObject(KeyPtr pKey, bool bRefresh, bool bLazyFetch)
	{
		InstanceKeyPtr		pIK;


		// Call fast method if this is a primary key
		//
		if (pKey->GetMetaKey()->IsPrimary())
			return LoadObjectWithPrimaryKey(pKey, bRefresh, bLazyFetch);

		// The key must exist and be unique
		//
		if (!pKey || !pKey->GetMetaKey()->IsUnique())
			return NULL;

		// Real simple if this is an instance key and no refresh required
		//
		if (!bRefresh && pKey->IsKindOf(InstanceKey::ClassObject()))
			return pKey->GetEntity();

		// Search the cache first
		//
		if (!bRefresh && (pIK = pKey->GetMetaKey()->FindInstanceKey(pKey, this)) != NULL)
		{
			return pIK->GetEntity();
		}
		else
		{
			std::string							strSQL;
			EntityPtrList						listEntity;
			bool										bFirst;
			ColumnPtrListItr				itrKeyCol;
			ColumnPtr								pCol;


			// Build SQL (here we ensure that order is determined by D3 and not the database)
			//
			strSQL  = "SELECT ";
			strSQL += pKey->GetMetaKey()->GetMetaEntity()->AsSQLSelectList(bLazyFetch);
			strSQL += " FROM ";
			strSQL += pKey->GetMetaKey()->GetMetaEntity()->GetName();
			strSQL += " WHERE ";

			bFirst = true;
			for ( itrKeyCol =  pKey->GetColumns().begin();
						itrKeyCol != pKey->GetColumns().end();
						itrKeyCol++)
			{
				pCol = *itrKeyCol;

				if (!bFirst)
					strSQL += " AND ";
				else
					bFirst = false;

				strSQL += pCol->GetMetaColumn()->GetName();
				strSQL += "=";
				strSQL += pCol->AsSQLString();
			}

			assert(!bFirst); // we must have at least one argument in the WHERE predicate

			LoadObjects(pKey->GetMetaKey()->GetMetaEntity(), &listEntity, strSQL, bRefresh, bLazyFetch);

			assert(listEntity.size() < 2);

			if (listEntity.size() == 1)
				return listEntity.front();
		}


		return NULL;
	}



	long OTLDatabase::LoadObjects(RelationPtr pRelation, bool bRefresh, bool bLazyFetch)
	{
		MetaRelationPtr					pMR;
		MetaKeyPtr							pMetaKey;
		KeyPtr									pKey;
		ColumnPtrListItr				itrSrceColumn;
		MetaColumnPtrListItr		itrTrgtColumn;
		ColumnPtr								pSrce;
		MetaColumnPtr						pTrgt;
		std::string							strSQL;
		std::string							strWHERE;
		EntityPtrList						listEntity;


		pMetaKey	= pRelation->GetMetaRelation()->GetChildMetaKey();
		pKey			= pRelation->GetParentKey();

		assert(pMetaKey);;
		assert(pKey);
		assert(m_pMetaDatabase == pMetaKey->GetMetaEntity()->GetMetaDatabase());

		// Build SQL (here we ensure that column order is D3 driven instead of by the database)
		//
		strSQL  = "SELECT ";
		strSQL += pMetaKey->GetMetaEntity()->AsSQLSelectList(bLazyFetch);
		strSQL += " FROM ";
		strSQL += pMetaKey->GetMetaEntity()->GetName();
		strSQL += " WHERE ";

		// Build the SQL where clause
		//
		for ( itrTrgtColumn =  pMetaKey->GetMetaColumns()->begin(),			itrSrceColumn =  pKey->GetColumns().begin();
					itrTrgtColumn != pMetaKey->GetMetaColumns()->end()		&&	itrSrceColumn != pKey->GetColumns().end();
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
				strWHERE += " = ";
				strWHERE += pSrce->AsSQLString();
			}
		}

		if (strWHERE.empty())
			return -1;

		strSQL += strWHERE;

		// Special attention for switched relations
		//
		pMR = pRelation->GetMetaRelation();

		if (pMR->IsChildSwitch() && pMR->GetSwitchColumn())
		{
			strSQL += " AND ";
			strSQL += pMR->GetSwitchColumn()->GetMetaColumn()->GetName();
			strSQL += " = ";
			strSQL += pMR->GetSwitchColumn()->AsSQLString();
		}

		try
		{
			LoadObjects(pMetaKey->GetMetaEntity(), &listEntity, strSQL, bRefresh, bLazyFetch);
		}
		catch (Exception & e)
		{
			e.LogError();
			return -1;
		}

		return listEntity.size();
	}



	long OTLDatabase::LoadObjects(MetaKeyPtr pMetaKey, KeyPtr pKey, bool bRefresh, bool bLazyFetch)
	{
		assert(pMetaKey);;
		assert(pKey);

		// Call fast method if this is a primary key
		//
		if (pMetaKey->IsPrimary())
		{
			// If pMetaKey is the primary key, use fast load method
			//
			if (pKey->GetMetaKey() != pMetaKey)
			{
				TemporaryKey		aTempPK(*pMetaKey, *pKey);

				if (LoadObjectWithPrimaryKey(&aTempPK, bRefresh, bLazyFetch))
					return 1;
			}
			else
			{
				if (LoadObjectWithPrimaryKey(pKey, bRefresh, bLazyFetch))
					return 1;
			}

			return 0;
		}
		else
		{
			ColumnPtrListItr				itrSrceColumn;
			MetaColumnPtrListItr		itrTrgtColumn;
			ColumnPtr								pSrce;
			MetaColumnPtr						pTrgt;
			std::string							strSQL;
			std::string							strWHERE;
			EntityPtrList						listEntity;


			assert(m_pMetaDatabase == pMetaKey->GetMetaEntity()->GetMetaDatabase());

			// Build SQL (here we ensure that column order is D3 driven instead of by the database)
			//
			strSQL  = "SELECT ";
			strSQL += pMetaKey->GetMetaEntity()->AsSQLSelectList(bLazyFetch);
			strSQL += " FROM ";
			strSQL += pMetaKey->GetMetaEntity()->GetName();
			strSQL += " WHERE ";

			// Build the SQL where clause
			//
			for ( itrTrgtColumn =  pMetaKey->GetMetaColumns()->begin(),			itrSrceColumn =  pKey->GetColumns().begin();
						itrTrgtColumn != pMetaKey->GetMetaColumns()->end()		&&	itrSrceColumn != pKey->GetColumns().end();
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
					strWHERE += " = ";
					strWHERE += pSrce->AsSQLString();
				}
			}

			if (strWHERE.empty())
				return -1;

			strSQL += strWHERE;

			try
			{
				LoadObjects(pMetaKey->GetMetaEntity(), &listEntity, strSQL, bRefresh, bLazyFetch);
					return listEntity.size();
			}
			catch (Exception & e)
			{
				e.LogError();
			}
		}

		return -1;
	}



	long OTLDatabase::LoadObjects(MetaEntityPtr pME, bool bRefresh, bool bLazyFetch)
	{
		std::string							strSQL;
		EntityPtrList						listEntity;


		// Build SQL (here we ensure that column order is D3 driven instead of by the database)
		//
		strSQL  = "SELECT ";
		strSQL += pME->AsSQLSelectList(bLazyFetch);
		strSQL += " FROM ";
		strSQL += pME->GetName();

		LoadObjects(pME, &listEntity, strSQL, bRefresh, bLazyFetch);

		return listEntity.size();
	}



	ColumnPtr OTLDatabase::LoadColumn(ColumnPtr pColumn)
	{
		otl_nocommit_stream			oRslts;
		OTLRecord								otlRec;			// The OTLRecord we'll be using to navigate the otl_nocommit_stream
		MetaColumnPtrVect				vectMC;
		std::string							strSQL;
		EntityPtrList						listEntity;
		bool										bFirst, bResult=false;
		ColumnPtrListItr				itrKeyCol;
		ColumnPtr								pCol;
		KeyPtr									pKey;


		assert(pColumn);
		assert(!pColumn->GetEntity()->IsNew());
		assert(pColumn->GetEntity()->GetMetaEntity()->GetMetaDatabase() == this->GetMetaDatabase());

		pKey = pColumn->GetEntity()->GetPrimaryKey();

		// Build SQL
		strSQL  = "SELECT ";
		strSQL += pColumn->GetMetaColumn()->GetName();
		strSQL += " FROM ";
		strSQL += pColumn->GetEntity()->GetMetaEntity()->GetName();
		strSQL += " WHERE ";

		bFirst = true;

		for ( itrKeyCol =  pKey->GetColumns().begin();
					itrKeyCol != pKey->GetColumns().end();
					itrKeyCol++)
		{
			pCol = *itrKeyCol;

			if (!bFirst)
				strSQL += ", ";

			bFirst =  false;
			strSQL += pCol->GetMetaColumn()->GetName();
			strSQL += "=";
			strSQL += pCol->AsSQLString();
		}

		assert(!bFirst);  // assert if we have an empty WHERE clause

		try
		{
			bFirst = true;

			if (m_uTrace & D3DB_TRACE_SELECT)
				ReportInfo("OTLDatabase::LoadObjects().........: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_iTransactionCount, strSQL.c_str());

			m_oConnection.set_max_long_size(D3_MAX_LOB_SIZE);
			oRslts.open(1,strSQL.c_str(),m_oConnection);
			vectMC.push_back(pColumn->GetMetaColumn());
			otlRec.Init(oRslts, &vectMC, false);

			while (otlRec.Next())
			{
				assert(bFirst);		// this loop should ever only be traversed once!!!

				otlRec[0].AssignToD3Column(pColumn);
				bResult = true;

				bFirst = false;
			}

			oRslts.close();
		}
		catch(...)
		{
			pColumn->MarkUnfetched();
			Exception::GenericExceptionHandler(__FILE__,__LINE__);
		}

		return bResult ? pColumn : NULL;
	}



	EntityPtrListPtr OTLDatabase::LoadObjects(MetaEntityPtr pMetaEntity, EntityPtrListPtr pEntityList, const std::string & strSQL, bool bRefresh, bool bLazyFetch)
	{
		EntityPtr								pObject;
		EntityPtrListPtr				pEL = pEntityList;
		bool										bFreeEL = false, bSuccess = false;
		otl_nocommit_stream			oRslts;
		OTLRecord								otlRec;			// The OTLRecord we'll be using to navigate the otl_nocommit_stream


		// Build the query std::string
		//
		if (!pMetaEntity)
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::LoadObjects(): Received request to load data from unknown table.");
			return NULL;
		}

		try
		{
			if (pMetaEntity->GetName() == "D3User")
				pObject = NULL;

			if (!pEL)
			{
				pEL = new EntityPtrList();
				bFreeEL = true;
			}

			if (m_uTrace & D3DB_TRACE_SELECT)
				ReportInfo("OTLDatabase::LoadObjects().........: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_iTransactionCount, strSQL.c_str());

			m_oConnection.set_max_long_size(D3_MAX_LOB_SIZE);
			oRslts.open(50,strSQL.c_str(),m_oConnection);
			otlRec.Init(oRslts, pMetaEntity->GetMetaColumnsInFetchOrder());

			while(otlRec.Next())
			{
				pObject = PopulateObject(pMetaEntity, otlRec, bRefresh);
				pEL->push_back(pObject);
			}

			if (m_uTrace & D3DB_TRACE_STATS)
				ReportInfo("OTLDatabase::LoadObjects().........: Database " PRINTF_POINTER_MASK " (Transaction count: %u). %d for SQL: %s", this, m_iTransactionCount, pEL->size(), strSQL.c_str());

			oRslts.close();
			bSuccess = true;
		}
		catch(otl_exception& p)
		{
			// intercept OTL exceptions
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::LoadObjects(): An OTL error occurred executing SQL statement %s. %s", strSQL.c_str(), p.msg);
		}
		catch(Exception & e)
		{
			e.LogError();
		}
		catch(std::bad_alloc&)
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::LoadObjects(): Out of memory");
		}
		catch(...)
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::LoadObjects(): An unspecified error occurred executing SQL statement %s.", strSQL.c_str());
		}

		if (!bSuccess)
		{
			try
			{
				oRslts.close();
				if (bFreeEL) delete pEL;
			} catch(...) {}

			return NULL;
		}

		return pEL;
	}



	EntityPtr OTLDatabase::FindObject(MetaEntityPtr pMetaEntity, OTLRecord& otlRec)
	{
		TemporaryKey			tmpKey = *(pMetaEntity->GetPrimaryMetaKey());
		InstanceKeyPtr		pInstanceKey;
		ColumnPtrListItr	itrColumn;
		ColumnPtr					pCol;


		for (	itrColumn =  tmpKey.GetColumns().begin();
					itrColumn != tmpKey.GetColumns().end();
					itrColumn++)
		{
			pCol = *itrColumn;
			otlRec[pCol->GetMetaColumn()->GetName()].AssignToD3Column(pCol);
		}

		pInstanceKey = pMetaEntity->GetPrimaryMetaKey()->FindInstanceKey(&tmpKey, this);

		if (pInstanceKey)
			return pInstanceKey->GetEntity();


		return NULL;
	}



	EntityPtr OTLDatabase::PopulateObject(MetaEntityPtr pMetaEntity, OTLRecord& otlRec, bool bRefresh)
	{
		unsigned int							i;
		EntityPtr									pEntity = NULL;


		// See if the object exists
		//
		pEntity = FindObject(pMetaEntity, otlRec);

		if (!pEntity)
		{
			pEntity = pMetaEntity->CreateInstance(this);
		}
		else
		{
			if (!bRefresh)
				return pEntity;
		}

		if (!pEntity)
			return NULL;

		pEntity->On_BeforePopulatingObject();

		try
		{
			unsigned int	nCols = otlRec.size() < pEntity->GetMetaEntity()->GetMetaColumnsInFetchOrder()->size() ? otlRec.size() : pEntity->GetMetaEntity()->GetMetaColumnsInFetchOrder()->size();

			for (i = 0; i < nCols; i++)
				otlRec[i].AssignToD3Column(pEntity->GetColumn(pEntity->GetMetaEntity()->GetMetaColumnsInFetchOrder()->operator[](i)));
		}
		catch(...)
		{
			pEntity->On_AfterPopulatingObject();
			throw;		// re-throw the exception
		}

		pEntity->On_AfterPopulatingObject();

		return pEntity;
	}





	int OTLDatabase::ExecuteSQLCommand(const std::string & strSQL, bool bReadOnly)
	{
		long int									iRowCount = -1;
		otl_nocommit_stream				oRslts;


		if (!bReadOnly && !HasTransaction())
			throw Exception(__FILE__, __LINE__, Exception_error, "OTLDatabase::ExecuteSQLCommand(): Method invoked without a pending transaction.");

		try
		{
			// We must process all resultsets as otherwise the update may fail. Only the last
			// resultset contains the actual records deleted from the immediate target table.
			//
			oRslts.open(1, strSQL.c_str(), m_oConnection);
			iRowCount = oRslts.get_rpc();
			oRslts.close();

		}
		catch(otl_exception& p)
		{
			// Translate Exception
			throw Exception(__FILE__, __LINE__, Exception_error, "OTLDatabase::ExecuteSQLCommand(): An OTL error occurred executing SQL statement %s. %s", strSQL.c_str(), p.msg);
		}

		return iRowCount;
	}



	void  OTLDatabase::ExecuteSingletonSQLCommand(const std::string & strSQL, unsigned long & lResult)
	{
		otl_nocommit_stream			oRslts;
		long										lVal;
		bool										bSuccess = false;


		try
		{
			if (m_uTrace & (D3DB_TRACE_SELECT | D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT))
				ReportInfo("OTLDatabase::ExecuteSingletonSQLCommand().........: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_iTransactionCount, strSQL.c_str());

			m_oConnection.set_max_long_size(D3_MAX_LOB_SIZE);
			oRslts.open(1,strSQL.c_str(),m_oConnection);

			if (!oRslts.eof())
			{
				oRslts >> lVal;

				if (!oRslts.is_null())
				{
					lResult = (unsigned long) lVal;
					bSuccess = true;
				}
			}

			oRslts.close();

			if (!bSuccess)
			{
				std::ostringstream	strm;
				strm << "OTLDatabase::ExecuteSingletonSQLCommand: Query " << strSQL << " must return a single numeric value";
				throw std::runtime_error(strm.str());
			}
		}
		catch(const otl_exception& exOTL)
		{
			std::ostringstream	strm;
			strm << "OTLDatabase::ExecuteSingletonSQLCommand: Query " << strSQL << " failed. " << exOTL.msg;
			throw std::runtime_error(strm.str());
		}
	}




	std::ostringstream& OTLDatabase::ExecuteQueryAsJSON(const std::string & strSQL, std::ostringstream	& ostrm)
	{
		otl_nocommit_stream			oRslts;
		OTLRecord								otlRec;
		bool										bFirst = true;


		try
		{
			if (m_uTrace & D3DB_TRACE_SELECT)
				ReportInfo("OTLDatabase::ExecuteQueryAsJSON()..: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_iTransactionCount, strSQL.c_str());

			m_oConnection.set_max_long_size(D3_MAX_LOB_SIZE);
			oRslts.open(50,strSQL.c_str(),m_oConnection);
			otlRec.Init(oRslts);

			ostrm << "[[";

			while(otlRec.Next())
			{
				if (bFirst)
					bFirst = false;
				else
					ostrm << ',';

				ostrm << '{';

				for (unsigned int i = 0; i < otlRec.size(); i++)
				{
					OTLColumn&		otlCol = otlRec[i];

					if (i)
						ostrm << ',';

					otlCol.WriteToJSONStream(ostrm, true);
				}

				ostrm << '}';
			}

			ostrm << "]]";

			oRslts.close();
		}
		catch(...)
		{
			D3::GenericExceptionHandler("OTLDatabase::ExecuteQueryAsJSON(): Executing query %s failed.", strSQL.c_str());
			throw std::runtime_error("OTLDatabase::ExecuteQueryAsJSON() failed!");
		}

		return ostrm;
	}




	bool OTLDatabase::InsertObject(EntityPtr pObj)
	{
		OTLStreamPoolPtr									pStrmPool = NULL;
		OTLStreamPool::OTLStreamPtr				pStrm = NULL;
		MetaColumnPtrVect::iterator				itrMEC;
		MetaColumnPtr											pMC;
		ColumnPtr													pColumn, pAutoCol = NULL;
		bool															bResult = false;
		std::string												strSQL;
		bool															bFirst = true;
		bool															bTrace = 	m_uTrace & (D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT);


		// Get the OTLStreamPool
		pStrmPool = GetOTLStreamPool(pObj->GetMetaEntity());

		try
		{
			pStrm = pStrmPool->FetchInsertStream();

			// Push all columns...(Note: otl automatically executes the update once values for
			//                           all columns have been pushed onto the stream)
			//
			for ( itrMEC =  pObj->GetMetaEntity()->GetMetaColumnsInFetchOrder()->begin();
						itrMEC != pObj->GetMetaEntity()->GetMetaColumnsInFetchOrder()->end();
						itrMEC++)
			{
				pMC = *itrMEC;

				pColumn = pObj->GetColumn(pMC);

				// ... except derived columns (we stop here because no normal columns are to follow derived columns)
				//
				if (pColumn->GetMetaColumn()->IsDerived())
					break;

				// Remember the autonum column (if there is one)
				// NOTE: In contrast to SQL Server where you do not need to provide
				// values for autonum columns, in Oracle values for columns with sequences
				// must be provided albeit the value can be NULL (which it always will
				// be here)
				//
				if (pColumn->GetMetaColumn()->IsAutoNum())
				{
					pAutoCol = pColumn;
				}

				if (bTrace)
				{
					if (bFirst)
						bFirst = false;
					else
						strSQL += ", ";
				}

				if (pColumn->IsNull())
				{
					pStrm->GetNativeStream() << otl_null();

					if (bTrace)
						strSQL += "NULL";
				}
				else
				{
					if (pColumn->GetMetaColumn()->IsStreamed())
					{
						switch (pColumn->GetMetaColumn()->GetType())
						{
							case MetaColumn::dbfString:
								pStrm->GetNativeStream() << otl_long_string(pColumn->GetString().data(), pColumn->GetString().size(), pColumn->GetString().size());
								break;

							case MetaColumn::dbfBlob:
								((ColumnBlob*) pColumn)->WriteOTLStream(pStrm->GetNativeStream());
								break;
						}

						if (bTrace)
							strSQL += "[LOB-Data]";
					}
					else
					{
						switch (pColumn->GetMetaColumn()->GetType())
						{
							case MetaColumn::dbfString:
								pStrm->GetNativeStream() << pColumn->GetString().c_str();
								break;

							case MetaColumn::dbfChar:
							case MetaColumn::dbfBool:
								pStrm->GetNativeStream() << pColumn->AsShort();
								break;

							case MetaColumn::dbfShort:
								pStrm->GetNativeStream() << pColumn->GetShort();
								break;

							case MetaColumn::dbfInt:
								pStrm->GetNativeStream() << pColumn->GetInt();
								break;

							case MetaColumn::dbfLong:
								pStrm->GetNativeStream() << pColumn->GetLong();
								break;

							case MetaColumn::dbfFloat:
								pStrm->GetNativeStream() << pColumn->GetFloat();
								break;

							case MetaColumn::dbfDate:
								pStrm->GetNativeStream() << (otl_datetime) pColumn->GetDate();
								break;

							case MetaColumn::dbfBlob:
								assert(pColumn->GetMetaColumn()->GetType() == MetaColumn::dbfBlob);
								((ColumnBlobPtr) pColumn)->WriteOTLStream(pStrm->GetNativeStream());
								break;

							case MetaColumn::dbfBinary:
								const Data&	data = pColumn->GetData();
								pStrm->GetNativeStream() << otl_long_string(data, pColumn->GetMetaColumn()->GetMaxLength(), data.length());
								break;
						}

						if (bTrace)
							strSQL += pColumn->AsSQLString();
					}
				}
			}

			if (bTrace)
				ReportDiagnostic("OTLDatabase::InsertObject()........: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s <== using: (%s)", this, m_iTransactionCount, pStrm->GetSQL().c_str(), strSQL.c_str());

			// We may have to update the internal key
			//
			if (pAutoCol)
			{
				otl_nocommit_stream	oRslts;
				InstanceKeyPtr			pPrimaryKey = pObj->GetPrimaryKey();
				std::string					strSEQ;
				long int						counter;
				std::string					strSequenceName;


				strSequenceName  = "seq_";
				strSequenceName += pAutoCol->GetEntity()->GetMetaEntity()->GetName().c_str();
				strSequenceName += "_";
				strSequenceName += pAutoCol->GetMetaColumn()->GetName().c_str();
				strSequenceName  = strSequenceName.substr(0,30);

				strSEQ =  "SELECT ";
				strSEQ += strSequenceName;
				strSEQ += ".CURRVAL FROM DUAL";

				oRslts.open(1,strSEQ.c_str(),m_oConnection);

				pPrimaryKey->On_BeforeUpdate();

				oRslts >> counter;

				switch (pAutoCol->GetMetaColumn()->GetType())
				{
					case MetaColumn::dbfChar:
						pAutoCol->SetValue((char) counter);
						break;

					case MetaColumn::dbfShort:
						pAutoCol->SetValue((short) counter);
						break;

					case MetaColumn::dbfInt:
						pAutoCol->SetValue((int) counter);
						break;

					case MetaColumn::dbfLong:
						pAutoCol->SetValue((long) counter);
						break;

					default:
						throw Exception(__FILE__, __LINE__, Exception_error, "OTLDatabase::InsertObject(): Can't set AutoNum column %s because the column is not of any of the expected data-types for AutoNum columns.", pAutoCol->GetMetaColumn()->GetFullName().c_str());
				}

				pPrimaryKey->On_AfterUpdate();

				oRslts.close();
			}

			pObj->MarkClean();

			bResult = true;
		}
		catch(otl_exception& p)
		{
			// intercept OTL exceptions
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::InsertObject(): An OTL error occurred inserting a %s record. OTL message: %s", pObj->GetMetaEntity()->GetFullName().c_str(), p.msg);
		}
		catch(Exception & e)
		{
			e.LogError();
		}
		catch(std::bad_alloc&)
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::InsertObject(): Out of memory");
		}
		catch(...)
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "OTLDatabase::InsertObject(): An unspecified error occurred inserting an %s record.", pObj->GetMetaEntity()->GetFullName().c_str());
		}

		// Now let's release the stream
		//
		if (pStrmPool && pStrm)
			pStrmPool->ReleaseInsertStream(pStrm);


		return bResult;
	}




	bool OTLDatabase::UpdateObject(EntityPtr pObj)
	{
		if (pObj->GetUpdateType() == Entity::SQL_Insert)
		{
			return InsertObject(pObj);
		}
		else
		{
			MONITORFUNC("OTLDatabase::UpdateObject()", this, m_iTransactionCount);

			std::string												strSQL, strTemp, strWHERE,strSEQ;
			bool															bFirst = true;
			bool															bDelete = false;
			ColumnPtrListItr									itrKeyCol;
			ColumnPtr													pCol;
			InstanceKeyPtr										pPrimaryKey;
			otl_nocommit_stream								oRslts;
			Entity::UpdateType								iUpdateType;
			long int													counter;
			ColumnPtrList											listStreamedCols;
			ColumnPtrListItr									itrListStreamedCols;
			MetaColumnPtrVect::iterator				itrMEC;
			MetaColumnPtr											pMC;


			// Update
			//
			try
			{
				if (!pObj || !HasTransaction())
					return false;

				iUpdateType = pObj->GetUpdateType();

				if (iUpdateType == Entity::SQL_Update || iUpdateType == Entity::SQL_Delete)
				{
					// Build WHERE predicate based on primary key
					//
					bFirst = true;
					strWHERE = "";

					for ( itrKeyCol =  pObj->GetOriginalPrimaryKey()->GetColumns().begin();
								itrKeyCol != pObj->GetOriginalPrimaryKey()->GetColumns().end();
								itrKeyCol++)
					{
						pCol = *itrKeyCol;

						if (!bFirst)
							strWHERE += " AND ";

						bFirst =  false;

						strWHERE += pCol->GetMetaColumn()->GetName();

						if (pCol->IsNull())
						{
							strWHERE += " IS NULL";
						}
						else
						{
							strWHERE += "=";
							strWHERE += pCol->AsSQLString();
						}
					}

					assert(!strWHERE.empty());
				}

				// Build SQL std::string depending on update type
				//
				switch (iUpdateType)
				{
					case Entity::SQL_Delete:
					{
						strSQL  = "DELETE FROM ";
						strSQL += pObj->GetMetaEntity()->GetName();
						strSQL += " WHERE ";
						strSQL += strWHERE;
						bDelete = true;

						break;
					}

					case Entity::SQL_Insert:
					{
						strSQL  = "INSERT INTO ";
						strSQL += pObj->GetMetaEntity()->GetName();
						strSQL += " (";

						strTemp = "";
						bFirst = true;

						for ( itrMEC =  pObj->GetMetaEntity()->GetMetaColumnsInFetchOrder()->begin();
									itrMEC != pObj->GetMetaEntity()->GetMetaColumnsInFetchOrder()->end();
									itrMEC++)
						{
							pMC = *itrMEC;

							pCol = pObj->GetColumn(pMC);


							if (pCol->GetMetaColumn()->IsDerived())
								break;

							if (!pCol->GetMetaColumn()->IsAutoNum())
							{
								if (bFirst)
								{
									bFirst = false;
								}
								else
								{
									strSQL  += ", ";
									strTemp += ", ";
								}

								strSQL  += pCol->GetMetaColumn()->GetName();

								// Blob values will have to be supplied before running query
								if (pCol->GetMetaColumn()->GetType() == MetaColumn::dbfBlob)
								{
									if (!pCol->IsNull())
									{
										strTemp += ":";
										strTemp += pCol->GetMetaColumn()->GetName();
										strTemp += "<raw_long>";
										listStreamedCols.push_back(pCol);
									}
									else
									{
										strTemp += "NULL";
									}
								}
								else
								{
									strTemp += pCol->AsSQLString();
								}

							}
						}

						strSQL += ") VALUES (";
						strSQL += strTemp;
						strSQL += ")";
						break;
					}

					case Entity::SQL_Update:
					{
						strSQL  = "UPDATE ";
						strSQL += pObj->GetMetaEntity()->GetName();
						strSQL += " ";

						bFirst = true;

						for ( itrMEC =  pObj->GetMetaEntity()->GetMetaColumnsInFetchOrder()->begin();
									itrMEC != pObj->GetMetaEntity()->GetMetaColumnsInFetchOrder()->end();
									itrMEC++)
						{
							pMC = *itrMEC;

							pCol = pObj->GetColumn(pMC);

							if (pCol->GetMetaColumn()->IsDerived())
								break;

							if (pCol->IsDirty())
							{
								assert(!pCol->GetMetaColumn()->IsAutoNum());

								if (pCol->GetMetaColumn()->IsMandatory() && pCol->IsNull())
									throw Exception(__FILE__, __LINE__, Exception_error, "OTLDatabase::Update(): Can't update column %s with NULL value.", pCol->GetMetaColumn()->GetFullName().c_str());

								if (bFirst)
								{
									bFirst = false;
									strSQL += "SET ";
								}
								else
								{
									strSQL += ", ";
								}

								strSQL += pCol->GetMetaColumn()->GetName();
								strSQL += "=";

								if (pCol->IsNull())
								{
									strSQL += "NULL";
								}
								else
								{
									if (pCol->GetMetaColumn()->IsStreamed())
									{
										switch (pCol->GetMetaColumn()->GetType())
										{
											case MetaColumn::dbfString:
												strSQL += "empty_clob()";
												break;
											case MetaColumn::dbfBlob:
												strSQL += "empty_blob()";
												break;
											default:
												throw Exception(__FILE__, __LINE__, Exception_error, "OTLDatabase::Update(): Column %s is marked streamed but isn't of type BLOB or varchar[>=4096].", pCol->GetMetaColumn()->GetFullName().c_str());
										}

										listStreamedCols.push_back(pCol);
									}
									else
									{
										strSQL += pCol->AsSQLString();
									}
								}
							}
						}

						// If we have not changed any columns, return success
						//
						if (bFirst)
							return true;

						strSQL += " WHERE ";
						strSQL += strWHERE;

						// If we have to deal with LOB's, append the RETURNING clause
						if (!listStreamedCols.empty())
						{
							std::string			strBLOBCols;
							std::string			strBLOBVals;

							bFirst = true;

							for ( itrListStreamedCols =  listStreamedCols.begin();
										itrListStreamedCols != listStreamedCols.end();
										itrListStreamedCols++)
							{
								pCol = *itrListStreamedCols;

								if (bFirst)
								{
									bFirst = false;
								}
								else
								{
									strBLOBCols += ",";
									strBLOBVals += ",";
								}

								strBLOBCols += pCol->GetMetaColumn()->GetName();

								strBLOBVals += ":";
								strBLOBVals += pCol->GetMetaColumn()->GetName();

								switch (pCol->GetMetaColumn()->GetType())
								{
									case MetaColumn::dbfString:
										strBLOBVals += "<clob>";
										break;
									case MetaColumn::dbfBlob:
										strBLOBVals += "<blob>";
										break;
								}
							}

							strSQL += " RETURNING ";
							strSQL += strBLOBCols;
							strSQL += " INTO ";
							strSQL += strBLOBVals;
						}

						break;
					}

					default:			// No work if default type is none of the above
						return true;
				}

				if (m_uTrace & (D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT))
					ReportInfo("OTLDatabase::UpdateObject()........: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_iTransactionCount, strSQL.c_str());

				// Choose a different OTL mechanism based on whether or not there are streamed columns
				if (!listStreamedCols.empty())
				{
					// Initialize the OTL objects we need
					m_oConnection.set_max_long_size(D3_MAX_LOB_SIZE);
					otl_nocommit_stream oStrm(1, strSQL.c_str(),	m_oConnection);

					// Store streams
					while (!listStreamedCols.empty())
					{
						pCol = listStreamedCols.front();
						listStreamedCols.pop_front();

						switch (pCol->GetMetaColumn()->GetType())
						{
							case MetaColumn::dbfString:
								oStrm << otl_long_string(pCol->GetString().data(), pCol->GetString().size(), pCol->GetString().size());
								break;

							case MetaColumn::dbfBlob:
								((ColumnBlobPtr) pCol)->WriteOTLStream(oStrm);
								break;
						}
					}
				}
				else
				{
					otl_cursor::direct_exec(m_oConnection, strSQL.c_str());
				}

				switch (iUpdateType)
				{
					case Entity::SQL_Delete:
						delete pObj;
						break;

					case Entity::SQL_Insert:
						pPrimaryKey = pObj->GetPrimaryKey();

						if (pPrimaryKey->GetMetaKey()->IsAutoNum())
						{
							// The first column in the primary key MUST be the AutoNum column
							//
							pCol = pPrimaryKey->GetColumns().front();
							switch (this->GetMetaDatabase()->GetTargetRDBMS())
							{
								case SQLServer:
								{
									break;
								}
								case Oracle:
								{
									std::string					strSequenceName;

									strSequenceName  = "seq_";
									strSequenceName += pCol->GetEntity()->GetMetaEntity()->GetName().c_str();
									strSequenceName += "_";
									strSequenceName += pCol->GetMetaColumn()->GetName().c_str();
									strSequenceName  = strSequenceName.substr(0,30);

									strSEQ =  "SELECT ";
									strSEQ += strSequenceName;
									strSEQ += ".CURRVAL FROM DUAL";

									oRslts.open(1,strSEQ.c_str(),m_oConnection);

									break;
								}
							}

							pPrimaryKey->On_BeforeUpdate();

							oRslts >> counter;

							switch (pCol->GetMetaColumn()->GetType())
							{
								case MetaColumn::dbfChar:
									pCol->SetValue((char) counter);
									break;

								case MetaColumn::dbfShort:
									pCol->SetValue((short) counter);
									break;

								case MetaColumn::dbfInt:
									pCol->SetValue((int) counter);
									break;

								case MetaColumn::dbfLong:
									pCol->SetValue((long) counter);
									break;

								default:
									throw Exception(__FILE__, __LINE__, Exception_error, "OTLDatabase::Update(): Can't set AutoNum column %s because the column is not of any of the expected data-types for AutoNum columns.", pCol->GetMetaColumn()->GetFullName().c_str());
							}

							pPrimaryKey->On_AfterUpdate();

							oRslts.close();
						}

						pObj->MarkClean();
						break;

					case Entity::SQL_Update:
						pObj->MarkClean();
						break;
				}
			}
			catch(otl_exception& p)
			{
				try
				{
					oRslts.close();
				}
				catch(...)
				{
				}

				throw Exception(__FILE__, __LINE__, Exception_error, "OTLDatabase::UpdateObject(): OTL error occured executing statement %s: %s", (void*) strSQL.c_str(), p.msg);
			}
			catch(Exception &)
			{
				try
				{
					oRslts.close();
				}
				catch(...)
				{
				}

				throw;
			}


			return true;
		}
	}




	// This method creates the Dictionary database
	//
	/* static */
	bool OTLDatabase::CreatePhysicalDatabase(MetaDatabasePtr pMD)
	{
		// This must be initialised
		//
		if (!pMD->GetInitialised())
		{
			ReportError("OTLDatabase::CreateDatabase(): Method called on an uninitialised MetaDatabase object.");
			return false;
		}

		// This must be initialised
		//
		if (!pMD->AllowCreate())
		{
			ReportWarning("OTLDatabase::CreateDatabase(): Method called on an MetaDatabase object with AllowCreate==false. Request ignored.");
			return true;
		}

		switch (pMD->GetTargetRDBMS())
		{
			case SQLServer:
				ReportError("OTLDatabase::CreateDatabase(): Oracle OCI API does not support MS SQL Server.");
				return false;

			case Oracle:
				return CreateOracleDatabase(pMD);

		}

		return false;
	}



	// Create a ORACLE database
	//
	/* static */
	bool OTLDatabase::CreateOracleDatabase(MetaDatabasePtr pMD)
	{
		MetaEntityPtr			pD3VI,pME;
		std::string				strMasterConnection;
		std::string				strSQL;
		char							szTemp[32];
		unsigned int			idx;
		otl_connect				oCon;
		std::string				oConnect;


		otl_connect::otl_initialize(1); // initialize OCI environment


		try
		{
			// =========== Drop the existing tablespace

			// Connect to the database (the 2nd parameter's value stands for AUTOCOMMIT)
			//
			oCon.rlogon(pMD->GetConnectionString().c_str(), true);

			// Delete the database
			//
			try
			{
				otl_cursor::direct_exec(oCon, "SET ROLE DBA");
				strSQL  = "DROP TABLESPACE ";
				strSQL += pMD->GetName();
				strSQL += " INCLUDING CONTENTS AND DATAFILES CASCADE CONSTRAINTS";
				otl_cursor::direct_exec(oCon, strSQL.c_str());

				// Let's sleep a couple of secs to give oracle a bit of time to finish its bits b4 we delete the physical file
				//
				SleepThread(5);
			}
			catch(...)
			{
				ReportWarning("OLTDatabase::CreateOracleDatabase(): Unable to drop tablespace %s, assuming that this tablespace does not exists.", pMD->GetName().c_str());
			}

			// Remove physical file. Note: if the tablespace does not exist, the above
			// direct_exec will cause an exception to be thrown in which case we continue
			//
			if (remove(pMD->GetDataFilePath().c_str()) != 0)
			{
				ReportWarning("OTLDatabase::CreateOracleDatabase(): Failed to remove physical TABLESPACE file %s, assuming that this file does not exist.", pMD->GetDataFilePath().c_str());
			}

			// =========== Drop existing sequences

			// Drop all Sequences
			//
			for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
			{
				pME = pMD->GetMetaEntity(idx);

				strSQL = pME->AsDropSequenceSQL(Oracle);

				if (!strSQL.empty())
				{
					try
					{
						otl_cursor::direct_exec(oCon, strSQL.c_str());
					}
					catch (...)
					{
						ReportWarning("OLTDatabase::CreateOracleDatabase(): Droping sequence failed (%s). Attempting to continue.", strSQL.c_str());
					}
				}
			}

			// =========== Create the bits n'pieces needed

			// Reconnect to the database (the 2nd parameter's value stands for AUTOCOMMIT)
			//
			oCon.logoff();
			oCon.rlogon(pMD->GetConnectionString().c_str(), true);

			// Create physical table space
			//
			otl_cursor::direct_exec(oCon, "SET ROLE DBA");
			strSQL  = "CREATE TABLESPACE ";
			strSQL += pMD->GetName();
			strSQL += " DATAFILE '";
			strSQL += pMD->GetDataFilePath();
			strSQL += "' SIZE ";
			strSQL += ltoa(pMD->GetInitialSize(), szTemp, 10);
			strSQL += "M ";
			strSQL += "AUTOEXTEND ON ";
			strSQL += "NEXT ";
			strSQL += ltoa(pMD->GetGrowthSize(), szTemp, 10);
			strSQL += "M ";
			strSQL += "MAXSIZE ";
			strSQL += ltoa(pMD->GetMaxSize(), szTemp, 10);
			strSQL += "M ";

			otl_cursor::direct_exec(oCon, strSQL.c_str());

			// Create package which updates foreign keys if a referenced key changes
			//
			CreateUpdateCascadePackage(oCon);

			// Create each table
			//
			for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
			{
				pME = pMD->GetMetaEntity(idx);
				otl_cursor::direct_exec(oCon, pME->AsCreateTableSQL(Oracle).c_str());
			}

			// Create all Sequences
			//
			for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
			{
				pME = pMD->GetMetaEntity(idx);
				strSQL = pME->AsCreateSequenceSQL(Oracle);

				if (!strSQL.empty())
					otl_cursor::direct_exec(oCon, strSQL.c_str());
			}

			// Create all Triggers for Sequences
			//
			for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
			{
				pME = pMD->GetMetaEntity(idx);
				strSQL = pME->AsCreateSequenceTriggerSQL(Oracle);

				if (!strSQL.empty())
					otl_cursor::direct_exec(oCon, strSQL.c_str());
			}

			// Create all indexes
			//
			for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
			{
				pME = pMD->GetMetaEntity(idx);

				for (unsigned int idx2 = 0; idx2 < pME->GetMetaKeyCount(); idx2++)
				{
					strSQL = pME->GetMetaKey(idx2)->AsCreateIndexSQL(Oracle);

					if (!strSQL.empty())
						otl_cursor::direct_exec(oCon, strSQL.c_str());
				}
			}

			// Create all triggers
			//
			for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
			{
				pME = pMD->GetMetaEntity(idx);
				CreateTriggers(Oracle, oCon, pME);
			}

			// If this database has a D3 version info table, insert a new row
			//
			pD3VI = pMD->GetVersionInfoMetaEntity();

			if (pD3VI)
			{
				D3Date						dtNow = D3Date::Now();
				char							szTemp[32];
				std::string				strTimeStamp;


				// The timestamp is a pain in oracle, this works
				//
				strTimeStamp  = "TO_DATE('";
				strTimeStamp += dtNow.AsString();
				strTimeStamp += "', 'YYYY-MM-DD HH24:MI:SS')";

				strSQL  = "INSERT INTO ";
				strSQL += pMD->GetVersionInfoMetaEntityName();
				strSQL += " (Name, VersionMajor, VersionMinor, VersionRevision, LastUpdated) VALUES ('";
				strSQL += pMD->GetAlias();
				strSQL += "', ";
				strSQL += ltoa(pMD->GetVersionMajor(), szTemp, 10);
				strSQL += ", ";
				strSQL += ltoa(pMD->GetVersionMinor(), szTemp, 10);
				strSQL += ", ";
				strSQL += ltoa(pMD->GetVersionRevision(), szTemp, 10);
				strSQL += ", ";
				strSQL += strTimeStamp;
				strSQL += ")";
				otl_cursor::direct_exec(oCon,strSQL.c_str());
			}
		}
		catch(otl_exception& p)
		{
			ReportError("OTLDatabase::CreateOracleDatabase(): OTL error occurred executing statement [%s]. More error details: %s", p.stm_text, p.msg);
			return false;
		}


		return true;
	}



	/* static */
	void OTLDatabase::CreateTriggers(TargetRDBMS eTargetDB, otl_connect& pCon, MetaEntityPtr pMetaEntity)
	{
		std::string				strSQL;
		unsigned int			idx;
		// Create CascadeUpdate/Delete Packages for Child
		//
		for (idx = 0; idx < pMetaEntity->GetChildMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetChildMetaRelation(idx)->AsCreateCascadePackage(eTargetDB);

			if (!strSQL.empty())
			{
				otl_cursor::direct_exec(pCon, strSQL.c_str());
			}
		}

		// Create CascadeUpdate/Delete Packages for Parents
		//
		for (idx = 0; idx < pMetaEntity->GetParentMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetParentMetaRelation(idx)->AsCreateCascadePackage(eTargetDB);

			if (!strSQL.empty())
			{
				otl_cursor::direct_exec(pCon, strSQL.c_str());
			}
		}

		// Create CheckInsertUpdate triggers
		//
		for (idx = 0; idx < pMetaEntity->GetParentMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetParentMetaRelation(idx)->AsCreateCheckInsertUpdateTrigger(eTargetDB);

			if (!strSQL.empty())
				otl_cursor::direct_exec(pCon, strSQL.c_str());
		}

		// Create CascadeDelete triggers
		//
		for (idx = 0; idx < pMetaEntity->GetChildMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetChildMetaRelation(idx)->AsCreateCascadeDeleteTrigger(eTargetDB);

			if (!strSQL.empty())
				otl_cursor::direct_exec(pCon, strSQL.c_str());
		}

		// Create CascadeClearRef triggers
		//
		for (idx = 0; idx < pMetaEntity->GetChildMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetChildMetaRelation(idx)->AsCreateCascadeClearRefTrigger(eTargetDB);

			if (!strSQL.empty())
				otl_cursor::direct_exec(pCon, strSQL.c_str());
		}

		// Create VerifyDelete triggers
		//
		for (idx = 0; idx < pMetaEntity->GetChildMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetChildMetaRelation(idx)->AsCreateVerifyDeleteTrigger(eTargetDB);

			if (!strSQL.empty())
				otl_cursor::direct_exec(pCon, strSQL.c_str());
		}

		// Create UpdateForeignRef triggers
		//
		for (idx = 0; idx < pMetaEntity->GetChildMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetChildMetaRelation(idx)->AsUpdateForeignRefTrigger(eTargetDB);

			if (!strSQL.empty())
				otl_cursor::direct_exec(pCon, strSQL.c_str());
		}

		// Create custom insert triggers
		//
		strSQL = pMetaEntity->GetCustomInsertTrigger(eTargetDB);

		if (!strSQL.empty())
			otl_cursor::direct_exec(pCon, strSQL.c_str());

		// Create custom update triggers
		//
		strSQL = pMetaEntity->GetCustomUpdateTrigger(eTargetDB);

		if (!strSQL.empty())
			otl_cursor::direct_exec(pCon, strSQL.c_str());

		// Create custom delete triggers
		//
		strSQL = pMetaEntity->GetCustomDeleteTrigger(eTargetDB);

		if (!strSQL.empty())
			otl_cursor::direct_exec(pCon, strSQL.c_str());
	}



	/* static */
	void OTLDatabase::CreateUpdateCascadePackage(otl_connect& pCon)
	{
		std::string				strSQL;


		try
		{
			strSQL  = "create or replace package update_cascade\n";
			strSQL += "as\n";
			strSQL += "procedure on_table( p_table_name      in varchar2, ";
			strSQL += "p_preserve_rowid  in boolean default TRUE, ";
			strSQL += "p_use_dbms_output in boolean default FALSE );\n";
			strSQL += "end update_cascade;";

			otl_cursor::direct_exec(pCon,strSQL.c_str());

			strSQL = "";
			strSQL +="create or replace package body update_cascade\n";
			strSQL += "as\n";

			strSQL += "type cnameArray is table of user_cons_columns.column_name%type\n";
			strSQL += "index by binary_integer;\n";

			strSQL += "sql_stmt           varchar2(32000);\n";
			strSQL += "use_dbms_output    boolean default FALSE;\n";
			strSQL += "preserve_rowid     boolean default TRUE;\n";

			strSQL += "function q( s in varchar2 ) return varchar2\n";
			strSQL += "is\n";
			strSQL += "begin\n";
			strSQL += "return '\"'||s||'\"';\n";
			strSQL += "end q;\n";

			strSQL += "function pkg_name( s in varchar2 ) return varchar2\n";
			strSQL += "is\n";
			strSQL += "begin\n";
			strSQL += "return q( 'u' || s || 'p' );\n";
			strSQL += "end pkg_name;\n";


			strSQL += "function view_name( s in varchar2 ) return varchar2\n";
			strSQL += "is\n";
			strSQL += "begin\n";
			strSQL += "return q( 'u' || s || 'v' );\n";
			strSQL += "end view_name;\n";

			strSQL += "function trigger_name( s in varchar2, s2 in varchar2 ) return varchar2\n";
			strSQL += "is\n";
			strSQL += "begin\n";
			strSQL += "return q( 'u' || s || s2 );\n";
			strSQL += "end trigger_name;\n";


			strSQL += "function strip( s in varchar2 ) return varchar2\n";
			strSQL += "is\n";
			strSQL += "begin\n";
			strSQL += "return ltrim(rtrim(s));\n";
			strSQL += "end strip;\n";

			strSQL += "procedure add( s in varchar2 )\n";
			strSQL += "is\n";
			strSQL += "begin\n";
			strSQL += "if ( use_dbms_output ) then\n";
			strSQL += "dbms_output.put_line( chr(9) || s );\n";
			strSQL += "else\n";
			strSQL += "sql_stmt := sql_stmt || chr(10) || s;\n";
			strSQL += "end if;\n";
			strSQL += "end add;\n";



			strSQL += "procedure execute_immediate\n";
			strSQL += "as\n";
			strSQL += "exec_cursor     integer default dbms_sql.open_cursor;\n";
			strSQL += "rows_processed  number  default 0;\n";
			strSQL += "begin\n";
			strSQL += "if ( use_dbms_output ) then\n";
			strSQL += "dbms_output.put_line( '/' );\n";
			strSQL += "else\n";
			strSQL += "dbms_sql.parse(exec_cursor, sql_stmt, dbms_sql.native );\n";
			strSQL += "rows_processed := dbms_sql.execute(exec_cursor);\n";
			strSQL += "dbms_sql.close_cursor( exec_cursor );\n";
			strSQL += "dbms_output.put_line(\n";
			strSQL += "substr( sql_stmt, 2, instr( sql_stmt,chr(10),2)-2 ) );\n";
			strSQL += "sql_stmt := NULL;\n";
			strSQL += "end if;\n";
			strSQL += "exception\n";
			strSQL += "when others then\n";
			strSQL += "if dbms_sql.is_open(exec_cursor) then\n";
			strSQL += "dbms_sql.close_cursor(exec_cursor);\n";
			strSQL += "end if;\n";
			strSQL += "raise;\n";
			strSQL += "end;\n";

			strSQL += "procedure get_pkey_names\n";
			strSQL += "( p_table_name      in out user_constraints.table_name%type,";
			strSQL += "  p_pkey_names         out cnameArray,";
			strSQL += "  p_constraint_name in out user_constraints.constraint_name%type )\n";
			strSQL += "is\n";
			strSQL += "begin\n";
			strSQL += "select table_name, constraint_name\n";
			strSQL += "into p_table_name, p_constraint_name\n";
			strSQL += "from user_constraints\n";
			strSQL += "where ( table_name = p_table_name\n";
			strSQL += "or table_name = upper(p_table_name) )\n";
			strSQL += "and constraint_type = 'P' ;\n";

			strSQL += "for x in ( select column_name , position\n";
			strSQL += "from user_cons_columns\n";
			strSQL += "where constraint_name = p_constraint_name\n";
			strSQL += "order by position ) loop\n";
			strSQL += "p_pkey_names( x.position ) := x.column_name;\n";
			strSQL += "end loop;\n";
			strSQL += "end get_pkey_names;\n";


			strSQL += "procedure write_spec( p_table_name in user_constraints.table_name%type,\n";
			strSQL += "p_pkey_names in cnameArray )\n";
			strSQL += "is\n";
			strSQL += "l_comma     char(1) default ' ';\n";
			strSQL += "begin\n";
			strSQL += "add( 'create or replace package ' || pkg_name(p_table_name) );\n";
			strSQL += "add( 'as' );\n";
			strSQL += "add( '--' );\n";
			strSQL += "add( '    rowCnt    number default 0;' );\n";
			strSQL += "add( '    inTrigger boolean default FALSE;' );\n";
			strSQL += "add( '--' );\n";

			strSQL += "for i in 1 .. 16 loop\n";
			strSQL += "begin\n";
			strSQL += "add( '    type C' || strip(i) || '_type is table of ' ||\n";
			strSQL += "q(p_table_name) || '.' || q(p_pkey_names(i)) ||\n";
			strSQL += "'%type index by binary_integer;' );\n";
			strSQL += "add( '--' );\n";
			strSQL += "add( '    empty_C' || strip(i) || ' C' || strip(i) || '_type;' );\n";
			strSQL += "add( '    old_C' || strip(i) || '   C' || strip(i) || '_type;' );\n";
			strSQL += "add( '    new_C' || strip(i) || '   C' || strip(i) || '_type;' );\n";
			strSQL += "add( '--' );\n";
			strSQL += "exception\n";
			strSQL += "when no_data_found then exit;\n";
			strSQL += "end;\n";
			strSQL += "end loop;\n";

			strSQL += "add( '--' );\n";
			strSQL += "add( '    procedure reset;' );\n";
			strSQL += "add( '--' );\n";
			strSQL += "add( '    procedure do_cascade;' );\n";
			strSQL += "add( '--' );\n";

			strSQL += "add( '    procedure add_entry' );\n";
			strSQL += "add( '    ( ' );\n";
			strSQL += "for i in 1 .. 16 loop\n";
			strSQL += "begin\n";
			strSQL += "add( '        ' || l_comma || 'p_old_C' || strip(i) || ' in ' ||\n";
			strSQL += "q(p_table_name) || '.' || q(p_pkey_names(i)) || '%type ' );\n";
			strSQL += "l_comma := ',';\n";
			strSQL += "add( '        ,p_new_C' || strip(i) || ' in out ' ||\n";
			strSQL += "q(p_table_name) || '.' || q(p_pkey_names(i)) || '%type ' );\n";
			strSQL += "exception\n";
			strSQL += "when no_data_found then exit;\n";
			strSQL += "end;\n";
			strSQL += "end loop;\n";
			strSQL += "add( '     );' );\n";
			strSQL += "add( '--' );\n";
			strSQL += "add( 'end ' || pkg_name(p_table_name) || ';' );\n";

			strSQL += "end write_spec;\n";


			strSQL += "procedure write_body\n";
			strSQL += "( p_table_name         in user_constraints.table_name%type,\n";
			strSQL += "p_pkey_names      in cnameArray,\n";
			strSQL += "p_constraint_name    in user_constraints.constraint_name%type )\n";
			strSQL += "is\n";
			strSQL += "l_col_cnt         number default 0;\n";
			strSQL += "l_comma         char(1) default ' ';\n";
			strSQL += "l_pkey_str      varchar2(2000);\n";
			strSQL += "l_pkey_name_str varchar2(2000);\n";
			strSQL += "l_other_col_str varchar2(2000);\n";
			strSQL += "begin\n";
			strSQL += "add( 'create or replace package body ' || pkg_name(p_table_name) );\n";
			strSQL += "add( 'as' );\n";
			strSQL += "add( '--' );\n";

			strSQL += "add( '    procedure reset ' );\n";
			strSQL += "add( '    is' );\n";
			strSQL += "add( '    begin' );\n";
			strSQL += "add( '--' );\n";
			strSQL += "add( '        if ( inTrigger ) then return; end if;' );\n";
			strSQL += "add( '--' );\n";
			strSQL += "add( '        rowCnt := 0;' );\n";
			strSQL += "for i in 1 .. 16 loop\n";
			strSQL += "begin\n";
			strSQL += "if (p_pkey_names(i) = p_pkey_names(i)) then\n";
			strSQL += "l_col_cnt := l_col_cnt+1;\n";
			strSQL += "end if;\n";
			strSQL += "add( '        old_C' || strip(i) || ' := empty_C' || strip(i) || ';' );\n";
			strSQL += "add( '        new_C' || strip(i) || ' := empty_C' || strip(i) || ';' );\n";
			strSQL += "exception\n";
			strSQL += "when no_data_found then exit;\n";
			strSQL += "end;\n";
			strSQL += "end loop;\n";
			strSQL += "add( '    end reset;' );\n";
			strSQL += "add( '--' );\n";

			strSQL += "add( '    procedure add_entry ' );\n";
			strSQL += "add( '    ( ' );\n";
			strSQL += "for i in 1 .. 16 loop\n";
			strSQL += "begin\n";
			strSQL += "add( '        ' || l_comma || 'p_old_C' || strip(i) || ' in ' ||\n";
			strSQL += "q(p_table_name) || '.' || q(p_pkey_names(i)) || '%type ' );\n";
			strSQL += "l_comma := ',';\n";
			strSQL += "add( '        ,p_new_C' || strip(i) || ' in out ' ||\n";
			strSQL += "q(p_table_name) || '.' || q(p_pkey_names(i)) || '%type ' );\n";
			strSQL += "exception\n";
			strSQL += "when no_data_found then exit;\n";
			strSQL += "end;\n";
			strSQL += "end loop;\n";
			strSQL += "add( '     )' );\n";
			strSQL += "add( '    is' );\n";
			strSQL += "add( '    begin' );\n";
			strSQL += "add( '--' );\n";
			strSQL += "add( '        if ( inTrigger ) then return; end if;' );\n";
			strSQL += "add( '--' );\n";
			strSQL += "add( '        if ( ' );\n";
			strSQL += "for i in 1 .. l_col_cnt loop\n";
			strSQL += "if ( i <> 1 ) then\n";
			strSQL += "add( '            OR' );\n";
			strSQL += "end if;\n";
			strSQL += "add( '             p_old_C' || strip(i) || ' <> ' ||\n";
			strSQL += "'p_new_C' || strip(i) );\n";
			strSQL += "end loop;\n";
			strSQL += "add( '         ) then ' );\n";
			strSQL += "add( '        rowCnt := rowCnt + 1;' );\n";

			strSQL += "for i in 1 .. l_col_cnt loop\n";
			strSQL += "add( '        old_C' || strip(i) ||\n";
			strSQL += "'( rowCnt ) := p_old_C' || strip(i) || ';' );\n";
			strSQL += "add( '        new_C' || strip(i) ||\n";
			strSQL += "'( rowCnt ) := p_new_C' || strip(i) || ';' );\n";

			strSQL += "add( '        p_new_C' || strip(i) ||\n";
			strSQL += "' := p_old_C' || strip(i) || ';' );\n";
			strSQL += "end loop;\n";

			strSQL += "add( '        end if;' );\n";
			strSQL += "add( '    end add_entry;' );\n";

			strSQL += "add( '--' );\n";

			strSQL += "l_comma := ' ';\n";
			strSQL += "for i in 1 .. l_col_cnt loop\n";
			strSQL += "l_pkey_str      := l_pkey_str||l_comma||'$$_C' || strip(i) || '(i)';\n";
			strSQL += "l_pkey_name_str := l_pkey_name_str || l_comma || q(p_pkey_names(i));\n";
			strSQL += "l_comma := ',';\n";
			strSQL += "end loop;\n";

			strSQL += "for x in ( select column_name\n";
			strSQL += "from user_tab_columns\n";
			strSQL += "where table_name = p_table_name\n";
			strSQL += "and column_name not in\n";
			strSQL += "( select column_name\n";
			strSQL += "from user_cons_columns\n";
			strSQL += "where constraint_name = p_constraint_name )\n";
			strSQL += "order by column_id )\n";
			strSQL += "loop\n";
			strSQL += "l_other_col_str := l_other_col_str || ',' || q(x.column_name);\n";
			strSQL += "end loop;\n";

			strSQL += "add( '    procedure do_cascade' );\n";
			strSQL += "add( '    is' );\n";
			strSQL += "add( '    begin' );\n";
			strSQL += "add( '--' );\n";
			strSQL += "add( '        if ( inTrigger ) then return; end if;' );\n";
			strSQL += "add( '        inTrigger := TRUE;' );\n";
			strSQL += "add( '--' );\n";
			strSQL += "add( '        for i in 1 .. rowCnt loop' );\n";
			strSQL += "add( '            insert into ' || p_table_name || ' ( ' );\n";
			strSQL += "add( '            ' || l_pkey_name_str );\n";
			strSQL += "add( '            ' || l_other_col_str || ') select ');\n";
			strSQL += "add( '            ' || replace( l_pkey_str, '$$', 'new' ) );\n";
			strSQL += "add( '            ' || l_other_col_str );\n";

			strSQL += "add( '            from ' || q(p_table_name) || ' a' );\n";
			strSQL += "add( '            where (' || l_pkey_name_str || ' ) = ' );\n";
			strSQL += "add( '                  ( select ' || replace(l_pkey_str,'$$','old') );\n";
			strSQL += "add( '                      from dual );' );\n";
			strSQL += "add( '--' );\n";
			strSQL += "if ( preserve_rowid ) then\n";
			strSQL += "add( '            update ' || q(p_table_name) || ' set ' );\n";
			strSQL += "add( '            ( ' || l_pkey_name_str || ' ) = ' );\n";
			strSQL += "add( '            ( select ' );\n";
			strSQL += "for i in 1 .. l_col_cnt loop\n";
			strSQL += "if ( i <> 1 ) then add( '                  ,' ); end if;\n";
			strSQL += "add( '                 decode( ' || q(p_pkey_names(i)) ||\n";
			strSQL += "replace(', old_c$$(i), new_c$$(i), old_c$$(i) )', '$$',strip(i)) );\n";
			strSQL += "end loop;\n";
			strSQL += "add( '              from dual )' );\n";
			strSQL += "add( '            where ( ' || l_pkey_name_str || ' ) =' );\n";
			strSQL += "add( '                  ( select ' || replace(l_pkey_str,'$$','new') );\n";
			strSQL += "add( '                      from dual )' );\n";
			strSQL += "add( '               OR ( ' || l_pkey_name_str || ' ) =' );\n";
			strSQL += "add( '                  ( select ' || replace(l_pkey_str,'$$','old') );\n";
			strSQL += "add( '                      from dual );' );\n";
			strSQL += "end if;\n";

			strSQL += "for x in ( select table_name, constraint_name\n";
			strSQL += "from user_constraints\n";
			strSQL += "where r_constraint_name = p_constraint_name\n";
			strSQL += "and constraint_type = 'R' ) loop\n";

			strSQL += "l_comma := ' ';\n";
			strSQL += "l_other_col_str := '';\n";
			strSQL += "for y in ( select column_name\n";
			strSQL += "from user_cons_columns\n";
			strSQL += "where constraint_name = x.constraint_name\n";
			strSQL += "order by position ) loop\n";
			strSQL += "l_other_col_str := l_other_col_str || l_comma || q(y.column_name);\n";
			strSQL += "l_comma := ',';\n";
			strSQL += "end loop;\n";

			strSQL += "add( '--' );\n";
			strSQL += "add( '            update ' || q( x.table_name ) || ' set ');\n";
			strSQL += "add( '            ( ' || l_other_col_str || ' ) = ' );\n";
			strSQL += "add( '            ( select  ' || replace( l_pkey_str, '$$', 'new' ) );\n";
			strSQL += "add( '                from dual )' );\n";
			strSQL += "add( '            where ( ' || l_other_col_str || ' ) = ' );\n";
			strSQL += "add( '                  ( select  ' ||\n";
			strSQL += "replace( l_pkey_str, '$$', 'old' ) );\n";
			strSQL += "add( '                    from dual );' );\n";

			strSQL += "end loop;\n";

			strSQL += "add( '--' );\n";
			strSQL += "add( '            delete from ' || q(p_table_name)  );\n";
			strSQL += "add( '             where ( ' || l_pkey_name_str || ' ) = ' );\n";
			strSQL += "add( '                   ( select ' || \n";
			strSQL += "replace( l_pkey_str, '$$', 'old' ) );\n";
			strSQL += "add( '                       from dual);' );\n";
			strSQL += "add( '        end loop;' );\n";

			strSQL += "add( '--' );\n";
			strSQL += "add( '        inTrigger := FALSE;' );\n";
			strSQL += "add( '        reset;' );\n";
			strSQL += "add( '   exception' );\n";
			strSQL += "add( '       when others then' );\n";
			strSQL += "add( '          inTrigger := FALSE;' );\n";
			strSQL += "add( '          reset;' );\n";
			strSQL += "add( '          raise;' );\n";
			strSQL += "add( '    end do_cascade;' );\n";
			strSQL += "add( '--' );\n";
			strSQL += "add( 'end ' || pkg_name( p_table_name ) || ';' );\n";

			strSQL += "end write_body;\n";


			strSQL += "procedure write_bu_trigger( p_table_name in user_constraints.table_name%type,\n";
			strSQL += "p_pkey_names in cnameArray )\n";
			strSQL += "is\n";
			strSQL += "l_comma char(1) default ' ';\n";
			strSQL += "begin\n";
			strSQL += "add( 'create or replace trigger ' || trigger_name( p_table_name, '1' ) );\n";
			strSQL += "add( 'before update of ' );\n";
			strSQL += "for i in 1 .. 16 loop\n";
			strSQL += "begin\n";
			strSQL += "add( '   ' || l_comma || q(p_pkey_names(i)) );\n";
			strSQL += "l_comma := ',';\n";
			strSQL += "exception\n";
			strSQL += "when no_data_found then exit;\n";
			strSQL += "end;\n";
			strSQL += "end loop;\n";
			strSQL += "add( 'on ' || q(p_table_name) );\n";
			strSQL += "add( 'begin ' || pkg_name(p_table_name) || '.reset; end;' );\n";
			strSQL += "end write_bu_trigger;\n";


			strSQL += "procedure write_bufer_trigger\n";
			strSQL += "( p_table_name in user_constraints.table_name%type,\n";
			strSQL += "p_pkey_names in cnameArray )\n";
			strSQL += "is\n";
			strSQL += "l_comma   char(1) default ' ';\n";
			strSQL += "begin\n";
			strSQL += "add( 'create or replace trigger '||trigger_name( p_table_name, '2' ) );\n";
			strSQL += "add( 'before update of ' );\n";

			strSQL += "for i in 1 .. 16 loop\n";
			strSQL += "begin\n";
			strSQL += "add( '   ' || l_comma || q(p_pkey_names(i)) );\n";
			strSQL += "l_comma := ',';\n";
			strSQL += "exception\n";
			strSQL += "when no_data_found then exit;\n";
			strSQL += "end;\n";
			strSQL += "end loop;\n";
			strSQL += "add( 'on ' || q(p_table_name) );\n";
			strSQL += "add( 'for each row' );\n";
			strSQL += "add( 'begin ' );\n";
			strSQL += "add( '   ' || pkg_name(p_table_name) || '.add_entry(' );\n";

			strSQL += "l_comma := ' ';\n";
			strSQL += "for i in 1 .. 16 loop\n";
			strSQL += "begin\n";
			strSQL += "add( '      ' || l_comma || ':old.' || q(p_pkey_names(i)) );\n";
			strSQL += "add( '      ,:new.' || q(p_pkey_names(i)) );\n";
			strSQL += "l_comma := ',';\n";
			strSQL += "exception\n";
			strSQL += "when no_data_found then exit;\n";
			strSQL += "end;\n";
			strSQL += "end loop;\n";
			strSQL += "add( '      );' );\n";
			strSQL += "add( 'end;' );\n";
			strSQL += "end write_bufer_trigger;\n";

			strSQL += "procedure write_au_trigger( p_table_name in user_constraints.table_name%type,\n";
			strSQL += "p_pkey_names in cnameArray )\n";
			strSQL += "is\n";
			strSQL += "l_comma  char(1) default ' ';\n";
			strSQL += "begin\n";
			strSQL += "add( 'create or replace trigger ' || trigger_name( p_table_name, '3' ) );\n";
			strSQL += "add( 'after update of ' );\n";
			strSQL += "for i in 1 .. 16 loop\n";
			strSQL += "begin\n";

			strSQL += "add( '   ' || l_comma || q(p_pkey_names(i)) );\n";
			strSQL += "l_comma := ',';\n";
			strSQL += "exception\n";
			strSQL += "when no_data_found then exit;\n";
			strSQL += "end;\n";
			strSQL += "end loop;\n";
			strSQL += "add( 'on ' || q(p_table_name) );\n";
			strSQL += "add( 'begin ' || pkg_name(p_table_name) || '.do_cascade; end;' );\n";
			strSQL += "end write_au_trigger;\n";

			strSQL += "procedure on_table( p_table_name      in varchar2,\n";
			strSQL += "p_preserve_rowid  in boolean default TRUE,\n";
			strSQL += "p_use_dbms_output in boolean default FALSE )\n";
			strSQL += "is\n";
			strSQL += "l_table_name         user_constraints.table_name%type default p_table_name;\n";
			strSQL += "l_constraint_name   user_constraints.constraint_name%type;\n";
			strSQL += "l_pkey_names        cnameArray;\n";
			strSQL += "l_comma                char(1) default ' ';\n";
			strSQL += "begin\n";
			strSQL += "use_dbms_output := p_use_dbms_output;\n";
			strSQL += "preserve_rowid  := p_preserve_rowid;\n";

			strSQL += "get_pkey_names( l_table_name, l_pkey_names, l_constraint_name );\n";

			strSQL += "sql_stmt := NULL;\n";
			strSQL += "write_spec( l_table_name, l_pkey_names );\n";
			strSQL += "execute_immediate;\n";
			strSQL += "write_body( l_table_name, l_pkey_names, l_constraint_name );\n";
			strSQL += "execute_immediate;\n";
			strSQL += "write_bu_trigger( l_table_name, l_pkey_names );\n";
			strSQL += "execute_immediate;\n";
			strSQL += "write_bufer_trigger( l_table_name, l_pkey_names );\n";
			strSQL += "execute_immediate;\n";
			strSQL += "write_au_trigger( l_table_name, l_pkey_names );\n";
			strSQL += "execute_immediate;\n";


			strSQL += "end on_table;\n";

			strSQL += "end update_cascade;\n";

			otl_cursor::direct_exec(pCon,strSQL.c_str());
		}
		catch(otl_exception& p)
		{
			ReportError("MetaDatabase::CreateUpdateCascadePackage(): OTL error: %s", p.msg);
		}
	}



	/* static */
	void	OTLDatabase::UnInitialise()
	{
	}



	long OTLDatabase::ImportFromXML(const std::string & strAppName, const std::string & strXMLFileName, MetaEntityPtrListPtr pListME)
	{
		CSAXParser									xmlParser;
		MetaEntityPtrList						listMEOrig, listME;
		MetaEntityPtrListItr				itrTrgt, itrSrce;
		OTLXMLImportFileProcessor		xmlProcessor(strAppName, this, &listME);


		xmlParser.SetDocumentHandler(&xmlProcessor);
		xmlParser.SetErrorHandler(&xmlProcessor);

		try
		{
			// Get all meta entities for this database ordered by dependency
			//
			listMEOrig = GetMetaDatabase()->GetDependencyOrderedMetaEntities();

			// If a list of meta entities was supplied, remove all those not supplied
			// from the ordered. Either way, remove the version info table.
			//
			for ( itrSrce  = listMEOrig.begin();
						itrSrce != listMEOrig.end();
						itrSrce++)
			{
				if (*itrSrce == GetMetaDatabase()->GetVersionInfoMetaEntity())
					continue;

				if (pListME)
				{
					for ( itrTrgt  = pListME->begin();
								itrTrgt != pListME->end();
								itrTrgt++)
					{
						if (*itrSrce == *itrTrgt)
						{
							listME.push_back(*itrSrce);
							break;
						}
					}
				}
				else
				{
					listME.push_back(*itrSrce);
				}
			}

			xmlParser.Parse(strXMLFileName.c_str());
		}
		catch (CXMLException& e)
		{
			std::cout << e.GetMsg() << std::endl;
			return -1;
		}

		if (xmlProcessor.GetErrorCount() > 0)
			std::cout << "WARNING: The importer reported " << xmlProcessor.GetErrorCount() << " errors, please check the log!!!" << std::endl;

		return xmlProcessor.GetTotalRecordCount();
	}



	void OTLDatabase::BeforeImportData(MetaEntityPtr pMetaEntity)
	{
		MetaColumnPtr						pAutoNumMC;
		std::string							strSQL;


		try
		{
			// Disable all triggers
			//
			switch (m_pMetaDatabase->GetTargetRDBMS())
			{
				case SQLServer:
					strSQL = "ALTER TABLE ";
					strSQL+= pMetaEntity->GetName();
					strSQL+= " DISABLE TRIGGER ALL";
					break;

				case Oracle:
					strSQL = "ALTER TABLE ";
					strSQL+= pMetaEntity->GetName();
					strSQL+= " DISABLE ALL TRIGGERS";
					break;
			}

			if (!strSQL.empty())
				otl_cursor::direct_exec(m_oConnection,strSQL.c_str());

			// Disable all Constraints
			//
			strSQL = "";

			switch (m_pMetaDatabase->GetTargetRDBMS())
			{
				case SQLServer:
					strSQL = "ALTER TABLE ";
					strSQL+= pMetaEntity->GetName();
					strSQL+= " NOCHECK CONSTRAINT ALL";
					break;

				case Oracle:
					strSQL = "ALTER TABLE ";
					strSQL = "begin\n";
					strSQL+= "declare\n";
					strSQL+= "v_sql varchar2(1000);\n";
					strSQL+= "begin\n";
					strSQL+= "for rec in (select table_name,constraint_name,CONSTRAINT_TYPE from user_constraints where table_name='";
					strSQL+= ToUpper(pMetaEntity->GetName());
					strSQL+= "' and CONSTRAINT_TYPE='R')\n";
					strSQL+= "loop\n";
					strSQL+= "v_sql := 'alter table '||rec.table_name||' disable constraint '||rec.constraint_name;\n";
					strSQL+= "execute immediate v_sql;\n";
					strSQL+= "end loop;\n";
					strSQL+= "exception\n";
					strSQL+= "when others then\n";
					strSQL+= "dbms_output.put_line('Exception:'||sqlerrm);\n";
					strSQL+= "end;\n";
					strSQL+= "end;\n";
					break;
			}
			if (!strSQL.empty())
				otl_cursor::direct_exec(m_oConnection,strSQL.c_str());

			// Has this got an identity column?
			//
			pAutoNumMC = pMetaEntity->GetAutoNumMetaColumn();

			if (pAutoNumMC)
			{
				// Enable IDENTITY insert
				//
				strSQL = "";

				switch (m_pMetaDatabase->GetTargetRDBMS())
				{
					case SQLServer:
						strSQL = "SET IDENTITY_INSERT ";
						strSQL+= pMetaEntity->GetName();
						strSQL+= " ON";
						break;

					case Oracle:
						// D3 implements IDY_ triggers for this purpose
						// and they have already been disabled above
						break;
				}

				if (!strSQL.empty())
					otl_cursor::direct_exec(m_oConnection,strSQL.c_str());
			}

		}
		catch(otl_exception& p)
		{
			// intercept OTL exceptions
			throw Exception(__FILE__, __LINE__, Exception_error, "OTLDatabase::BeforeImportData(): An OTL error occurred executing SQL statement %s. %s", strSQL.c_str(), p.msg);
		}
	}







	void OTLDatabase::AfterImportData(MetaEntityPtr pMetaEntity, unsigned long lMaxKeyVal)
	{
		std::string							strSQL;


		try
		{
			MetaColumnPtr						pAutoNumMC;
			otl_nocommit_stream			pRslts;


			// Enable all triggers
			//
			switch (m_pMetaDatabase->GetTargetRDBMS())
			{
				case SQLServer:
					strSQL = "ALTER TABLE ";
					strSQL+= pMetaEntity->GetName();
					strSQL+= " ENABLE TRIGGER ALL";
					break;

				case Oracle:
					strSQL = "ALTER TABLE ";
					strSQL+= pMetaEntity->GetName();
					strSQL+= " ENABLE ALL TRIGGERS";
					break;
			}

			if (!strSQL.empty())
				otl_cursor::direct_exec(m_oConnection,strSQL.c_str());

			// Enable all Constraints
			//
			strSQL = "";

			switch (m_pMetaDatabase->GetTargetRDBMS())
			{
				case SQLServer:
					strSQL = "ALTER TABLE ";
					strSQL+= pMetaEntity->GetName();
					strSQL+= " CHECK CONSTRAINT ALL";
					break;

				case Oracle:
					strSQL = "ALTER TABLE ";
					strSQL = "begin\n";
					strSQL+= "declare\n";
					strSQL+= "v_sql varchar2(1000);\n";
					strSQL+= "begin\n";
					strSQL+= "for rec in (select table_name,constraint_name,CONSTRAINT_TYPE from user_constraints where table_name='";
					strSQL+= ToUpper(pMetaEntity->GetName());
					strSQL+= "' and CONSTRAINT_TYPE='R')\n";
					strSQL+= "loop\n";
					strSQL+= "v_sql := 'alter table '||rec.table_name||' enable novalidate constraint '||rec.constraint_name;\n";
					strSQL+= "execute immediate v_sql;\n";
					strSQL+= "end loop;\n";
					strSQL+= "exception\n";
					strSQL+= "when others then\n";
					strSQL+= "dbms_output.put_line('Exception:'||sqlerrm);\n";
					strSQL+= "end;\n";
					strSQL+= "end;\n";
					break;
			}
			if (!strSQL.empty())
				otl_cursor::direct_exec(m_oConnection,strSQL.c_str());

			// Has this got an identity column?
			//
			pAutoNumMC = pMetaEntity->GetAutoNumMetaColumn();

			if (pAutoNumMC)
			{
				// Reset IDENTITY insert mechanism
				//
				strSQL = "";

				switch (m_pMetaDatabase->GetTargetRDBMS())
				{
					case SQLServer:
						strSQL = "SET IDENTITY_INSERT ";
						strSQL+= pMetaEntity->GetName();
						strSQL+= " OFF";

						otl_cursor::direct_exec(m_oConnection,strSQL.c_str());

						break;

					case Oracle:

						if (lMaxKeyVal == 0)
							break;

						std::string			strSequenceName;
						std::string			strTemp;
						unsigned long		lNxtVal;
						std::string			strNxtVal;
						long						lStepVal;
						bool						updatedSequence = false;
						otl_connect			oCon;						// let's use our own connection with autocommit set to true

						oCon.rlogon(m_pMetaDatabase->GetConnectionString().c_str(), true);

						// We need to reset the Sequence such that the next time it is called it returns
						// lMaxKeyVal+1
						//
						strSequenceName  = "seq_";
						strSequenceName += pMetaEntity->GetName();
						strSequenceName += "_";
						strSequenceName += pAutoNumMC->GetName();
						strSequenceName  = strSequenceName.substr(0,30);

						// Get the next value from the sequence
						//
						strSQL  = "SELECT ";
						strSQL += strSequenceName;
						strSQL += ".NEXTVAL FROM DUAL";

						oCon.set_max_long_size(D3_MAX_LOB_SIZE);
						pRslts.set_all_column_types(otl_all_num2str | otl_all_date2str);
						pRslts.open(50,strSQL.c_str(),oCon);
						pRslts>>strNxtVal;

						Convert(lNxtVal,strNxtVal);

						pRslts.close();

						while (true)
						{
							updatedSequence = true;

							// Calculate necessary adjustment
							//
							lStepVal = lMaxKeyVal - lNxtVal;

							if (lStepVal != 0)
							{
								// Let's adjust the increment
								//
								Convert(strTemp, lStepVal);

								strSQL  = "ALTER SEQUENCE ";
								strSQL += strSequenceName;
								strSQL += " INCREMENT BY ";
								strSQL += strTemp;

								otl_cursor::direct_exec(oCon,strSQL.c_str());

								// Get the next value from the sequence (we don't need details)
								//
								strSQL  = "SELECT ";
								strSQL += strSequenceName;
								strSQL += ".NEXTVAL FROM DUAL";

								oCon.set_max_long_size(D3_MAX_LOB_SIZE);
								pRslts.set_all_column_types(otl_all_num2str | otl_all_date2str);
								pRslts.open(50,strSQL.c_str(),oCon);
								pRslts.close();

								// Let's reset the increment
								//
								strSQL  = "ALTER SEQUENCE ";
								strSQL += strSequenceName;
								strSQL += " INCREMENT BY 1";

								otl_cursor::direct_exec(oCon,strSQL.c_str());
							}

							break;
						}

					if(updatedSequence == false)
					{
						throw Exception(__FILE__, __LINE__, Exception_error, "OTLDatabase::AfterImportData(): Failed to reset sequence %s.", strSequenceName.c_str());
					}
				}
			}
		}
		catch(otl_exception& p)
		{
			// intercept OTL exceptions
			throw Exception(__FILE__, __LINE__, Exception_error, "OTLDatabase::AfterImportData(): An OTL error occurred executing SQL statement %s. %s", strSQL.c_str(), p.msg);
		}
		catch (...)
		{
			throw;
		}
	}



	// Backup all records of the entities listed in pListME to the specified XML file. The method
	// returns the number of records written to the XML file or -1 if an error occurred..
	//
	long OTLDatabase::ExportToXML(const std::string & strAppName, const std::string & strXMLFileName, MetaEntityPtrListPtr pListME, const std::string & strD3MDDBIDFilter)
	{
		MetaEntityPtr								pMetaEntity;
		MetaEntityPtrList						listMEOrig, listME;
		MetaEntityPtrListItr				itrTrgt, itrSrce;
		MetaColumnPtrListItr				itrKeyMC;
		MetaColumnPtrVect::iterator	itrMEC;
		MetaColumnPtr								pMC;
		std::ofstream								fxml;
		long												lRecCountTotal=0, lRecCountCurrent;


		try
		{
			std::cout << "Starting to export data from " << m_pMetaDatabase->GetAlias() << ' ' << m_pMetaDatabase->GetVersion() << std::endl;

			// Open the output file
			//
			fxml.open(strXMLFileName.c_str());

			if (!fxml.is_open())
			{
				std::cout << "Failed to open " << strXMLFileName << " for writing!\n";
				return -1;
			}

			// Write the header which will look something like:
			//
			// <!-- APAL3DB V6.02.03 - 27/01/2005 22:29:00 -->
			// <APALDBData>
			// 	<DatabaseList>
			// 		<Database Alias="APAL3DB">
			// 			<VersionMajor>5</VersionMajor>
			// 			<VersionMinor>10</VersionMinor>
			// 			<VersionRevision>10</VersionRevision>
			//
			// Note: for the meta dictionary, the first line above will not include the date/time stamp
			// as it is expected that this file will go into CVS
			//
			if (m_pMetaDatabase == MetaDatabase::GetMetaDictionary())
				fxml << "<!-- " << m_pMetaDatabase->GetAlias() << ' ' << m_pMetaDatabase->GetVersion() << " -->\n";
			else
				fxml << "<!-- " << m_pMetaDatabase->GetAlias() << ' ' << m_pMetaDatabase->GetVersion() << " - " << SystemDateTimeAsStandardString() << " -->\n";

			fxml << "<APALDBData>\n";
			fxml << "\t<DatabaseList>\n";
			fxml << "\t\t<Database Alias=\"" << m_pMetaDatabase->GetAlias() << "\">\n";
			fxml << "\t\t\t<VersionMajor>" << m_pMetaDatabase->GetVersionMajor() << "</VersionMajor>\n";
			fxml << "\t\t\t<VersionMinor>" << m_pMetaDatabase->GetVersionMinor() << "</VersionMinor>\n";
			fxml << "\t\t\t<VersionRevision>" << m_pMetaDatabase->GetVersionRevision() << "</VersionRevision>\n";

			// Get all meta entities for this database ordered by dependency
			//
			listMEOrig = GetMetaDatabase()->GetDependencyOrderedMetaEntities();

			// If a list of meta entities was supplied, remove all those not supplied
			// from the ordered. Either way, remove the version info table.
			//
			for ( itrSrce  = listMEOrig.begin();
						itrSrce != listMEOrig.end();
						itrSrce++)
			{
				if (*itrSrce == GetMetaDatabase()->GetVersionInfoMetaEntity())
					continue;

				if (pListME)
				{
					for ( itrTrgt  = pListME->begin();
								itrTrgt != pListME->end();
								itrTrgt++)
					{
						if (*itrSrce == *itrTrgt)
						{
							listME.push_back(*itrSrce);
							break;
						}
					}
				}
				else
				{
					listME.push_back(*itrSrce);
				}
			}

			while (!listME.empty())
			{
				pMetaEntity = listME.front();

				if (pMetaEntity)
				{
					otl_nocommit_stream 		otlRslts;
					OTLRecord								otlRec;
					std::string							strSQL;

					// Report to user
					std::cout << "  " << pMetaEntity->GetName() <<  std::string(40 - pMetaEntity->GetName().size(), '.') << ":" << std::string(12, ' ');

					lRecCountCurrent = 0;

					// Write Entitylist header
					//
					fxml << "\t\t\t<EntityList Name=\"" << pMetaEntity->GetName() << "\">\n";

					// Create the "SELECT col1, col2, ...coln FROM tablename" statement
					//
					strSQL = "SELECT ";
					strSQL += pMetaEntity->AsSQLSelectList(false);
					strSQL += " FROM ";
					strSQL += pMetaEntity->GetName();

					strSQL += this->FilterExportToXML(pMetaEntity, strD3MDDBIDFilter);

					// Order by primary key
					//
					pMC = NULL;

					for (	itrKeyMC  = pMetaEntity->GetPrimaryMetaKey()->GetMetaColumns()->begin();
								itrKeyMC != pMetaEntity->GetPrimaryMetaKey()->GetMetaColumns()->end();
								itrKeyMC++)
					{
						// The first time add order by clause
						//
						if (!pMC)
							strSQL += " ORDER BY ";
						else
							strSQL += ",";

						pMC = *itrKeyMC;
						strSQL += pMC->GetName();
					}

					// Fetch all instances
					//
					m_oConnection.set_max_long_size(D3_MAX_LOB_SIZE);
					otlRslts.open(50,strSQL.c_str(),m_oConnection);
					otlRec.Init(otlRslts, pMetaEntity->GetMetaColumnsInFetchOrder(), false);

					while(otlRec.Next())
					{
						unsigned int							i;
						MetaColumnPtr							pMC;

						lRecCountCurrent++;

						// Write Entity header
						//
						fxml << "\t\t\t\t<Entity>\n";

						for ( itrMEC =  pMetaEntity->GetMetaColumnsInFetchOrder()->begin(), i=0;
									itrMEC != pMetaEntity->GetMetaColumnsInFetchOrder()->end();
									itrMEC++, i++)
						{
							pMC = *itrMEC;

							// Derived columns must be at the end
							//
							if (pMC->IsDerived())
								break;

							// Write Column header
							//
							fxml << "\t\t\t\t\t<" << pMC->GetName() << " NULL=\"";

							// For now, BLOB's are always NULL
							if (otlRec[i].IsNull())
							{
								fxml << "1\"></" << pMC->GetName() << ">\n";
							}
							else
							{
								switch (pMC->GetType())
								{
									case MetaColumn::dbfString:
									{
										std::string			str;

										if (pMC->IsEncodedValue())
											str = APALUtil::base64_encode(otlRec[i].AsString());
										else
											str = XMLEncode(otlRec[i].AsString());

										if (str.empty())
											fxml << "1\"></" << pMC->GetName() << ">\n";
										else
											fxml << "0\">" << str << "</" << pMC->GetName() << ">\n";

										break;
									}

									case MetaColumn::dbfBlob:
									case MetaColumn::dbfBinary:
									{
										otl_long_string&			lob(*(otlRec[i].m_Value.otlLOB));
										std::string						strEncodedData;

										APALUtil::base64_encode(strEncodedData, &(lob[0]), lob.len());
										fxml << "0\">" << strEncodedData << "</" << pMC->GetName() << ">\n";
										break;
									}

									case MetaColumn::dbfDate:
									{
										fxml << "0\">" << D3Date(otlRec[i].AsString(), m_pMetaDatabase->GetTimeZone()).AsISOString() << "</" << pMC->GetName() << ">\n";;
										break;
									}

									default:
										fxml << "0\">" << XMLEncode(otlRec[i].AsString()) << "</" << pMC->GetName() << ">\n";
								}
							}
						}

						fxml << "\t\t\t\t</Entity>\n";

						// Report every 100th record written
						if ((lRecCountCurrent % 100) == 0)
							std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent;
					}

					otlRslts.close();

					fxml << "\t\t\t</EntityList>\n";

					std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent << std::endl;

					lRecCountTotal += lRecCountCurrent;
				}

				listME.pop_front();
			}

			// Write the header which will look something like:
			//
			//     </Database>
			//   </DatabaseList>
			// </APALDBData>
			//
			fxml << "\t\t</Database>\n";
			fxml << "\t</DatabaseList>\n";
			fxml << "</APALDBData>\n";
			fxml.close();

			std::cout << "Finished to export data from " << m_pMetaDatabase->GetName() << std::endl;
			std::cout << "Total Records exported: " << lRecCountTotal << std::endl;
		}
		catch(otl_exception & e)
		{
			std::cout << "OTL exception caught: " << e.msg << std::endl;
			return -1;
		}

		return lRecCountTotal;
	}



	void OTLDatabase::On_PostCreate()
	{
		// Some sanity checks (this must NOT be initialised and both MetaDatabase and DatabaseWorkspace must be defined)
		//
		if (m_pMetaDatabase)
		{
			OTLStreamPoolPtrVect::size_type				idx;

			m_vectOTLStreamPool.resize(m_pMetaDatabase->GetMetaEntities()->size() + 1);

			for (idx = 0; idx < m_vectOTLStreamPool.size(); idx++)
				m_vectOTLStreamPool[idx] = NULL;
		}

		Database::On_PostCreate();
	}



	OTLStreamPoolPtr OTLDatabase::GetOTLStreamPool(MetaEntityPtr pME)
	{
		OTLStreamPoolPtr									pStrmPool = NULL;
		OTLStreamPoolPtrVect::size_type		idx = pME->GetEntityIdx();

		// Get the OTLStreamPool
		//
		assert(idx < m_vectOTLStreamPool.size());

		pStrmPool = m_vectOTLStreamPool[idx];

		if (!pStrmPool)
		{
			pStrmPool = new OTLStreamPool(m_oConnection, pME);
			m_vectOTLStreamPool[idx] = pStrmPool;
		}

		return pStrmPool;
	}





	void OTLDatabase::SetMetaDatabaseHSTopics()
	{
		string										strSQL, strTargetID, strTargetMask, strTargetFilter, strCurrAlias, strPrevAlias, strTitle;
		int												iID;
		MetaDatabasePtr						pMD;
		otl_nocommit_stream				oRslts;
		ostringstream							ostrm;
		bool											bFirst;


		// Build select for MetaDatabase help topics
		bFirst = true;
		Convert(strTargetMask, (int) HSTopic::GetCurrentTargetMask());

		switch (GetMetaDatabase()->GetTargetRDBMS())
		{
			case SQLServer:
			{
				strTargetFilter = "WHERE t.Targets&";
				strTargetFilter += strTargetMask;
				strTargetFilter += "=";
				strTargetFilter += strTargetMask;
				break;
			}

			case Oracle:
			{
				strTargetFilter = "WHERE BITAND(t.Targets,";
				strTargetFilter += strTargetMask;
				strTargetFilter += ")=";
				strTargetFilter += strTargetMask;
				break;
			}
		}

		strSQL = "SELECT mot.MetaDatabaseAlias, t.ID, t.Title FROM HSMetaDatabaseTopic mot LEFT JOIN HSTopic t ON mot.TopicID=t.ID ";
		strSQL += strTargetFilter;
		strSQL += " ORDER BY mot.MetaDatabaseAlias, t.Title";

		// open the query
		oRslts.open(50, strSQL.c_str(), m_oConnection);

		if (m_uTrace & D3DB_TRACE_SELECT)
			ReportInfo("OTLDatabase::SetMetaDatabaseHSTopics().....................: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this), strSQL.c_str());

		while (!oRslts.eof())
		{
			// read a record
			oRslts >> strCurrAlias;;
			oRslts >> iID;
			oRslts >> strTitle;

			// handle group breaks
			if (strCurrAlias != strPrevAlias)
			{
				if (!strPrevAlias.empty())
				{
					pMD = MetaDatabase::GetMetaDatabase(strPrevAlias);
					if (pMD)
					{
						ostrm << ']';
						pMD->SetHSTopics(ostrm.str());
					}
				}

				bFirst = true;
				ostrm.str("");
				ostrm << '[';
				strPrevAlias = strCurrAlias;
			}

			if (bFirst)
				bFirst = false;
			else
				ostrm << ',';

			ostrm << "{\"id\":" << iID << ",\"title\":\"" << JSONEncode(strTitle) << "\"}";
		}

		if (!strPrevAlias.empty())
		{
			pMD = MetaDatabase::GetMetaDatabase(strPrevAlias);
			if (pMD)
			{
				ostrm << ']';
				pMD->SetHSTopics(ostrm.str());
			}
		}
	}





	void OTLDatabase::SetMetaEntityHSTopics()
	{
		string										strSQL, strTargetID, strTargetMask, strTargetFilter, strCurrAlias, strPrevAlias, strCurrEntity, strPrevEntity, strTitle;
		int												iID;
		MetaDatabasePtr						pMD;
		MetaEntityPtr							pME;
		otl_nocommit_stream				oRslts;
		ostringstream							ostrm;
		bool											bFirst;


		// Build select for MetaEntity help topics
		Convert(strTargetMask, (int) HSTopic::GetCurrentTargetMask());

		switch (GetMetaDatabase()->GetTargetRDBMS())
		{
			case SQLServer:
			{
				strTargetFilter = "WHERE t.Targets&";
				strTargetFilter += strTargetMask;
				strTargetFilter += "=";
				strTargetFilter += strTargetMask;
				break;
			}

			case Oracle:
			{
				strTargetFilter = "WHERE BITAND(t.Targets,";
				strTargetFilter += strTargetMask;
				strTargetFilter += ")=";
				strTargetFilter += strTargetMask;
				break;
			}
		}

		strSQL = "SELECT mot.MetaDatabaseAlias, mot.MetaEntityName, t.ID, t.Title FROM HSMetaEntityTopic mot LEFT JOIN HSTopic t ON mot.TopicID=t.ID ";
		strSQL += strTargetFilter;
		strSQL += " ORDER BY mot.MetaDatabaseAlias, mot.MetaEntityName, t.Title";

		// open the query
		oRslts.open(50, strSQL.c_str(), m_oConnection);

		if (m_uTrace & D3DB_TRACE_SELECT)
			ReportInfo("OTLDatabase::SetMetaEntityHSTopics().......................: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this), strSQL.c_str());

		while (!oRslts.eof())
		{
			// read a record
			oRslts >> strCurrAlias;
			oRslts >> strCurrEntity;
			oRslts >> iID;
			oRslts >> strTitle;

			// Handle group breaks
			if (strCurrAlias != strPrevAlias || strCurrEntity != strPrevEntity)
			{
				if (!strPrevAlias.empty() && !strPrevEntity.empty())
				{
					pMD = MetaDatabase::GetMetaDatabase(strPrevAlias);
					if (pMD)
					{
						pME = pMD->GetMetaEntity(strPrevEntity);
						if (pME)
						{
							ostrm << ']';
							pME->SetHSTopics(ostrm.str());
						}
					}
				}

				bFirst = true;
				ostrm.str("");
				ostrm << '[';
				strPrevAlias = strCurrAlias;
				strPrevEntity = strCurrEntity;
			}

			if (bFirst)
				bFirst = false;
			else
				ostrm << ',';

			ostrm << "{\"id\":" << iID << ",\"title\":\"" << JSONEncode(strTitle) << "\"}";
		}

		if (!strPrevAlias.empty() && !strPrevEntity.empty())
		{
			pMD = MetaDatabase::GetMetaDatabase(strPrevAlias);
			if (pMD)
			{
				pME = pMD->GetMetaEntity(strPrevEntity);
				if (pME)
				{
					ostrm << ']';
					pME->SetHSTopics(ostrm.str());
				}
			}
		}
	}





	void OTLDatabase::SetMetaColumnHSTopics()
	{
		string										strSQL, strTargetID, strTargetMask, strTargetFilter, strCurrAlias, strPrevAlias, strCurrEntity, strPrevEntity, strCurrColumn, strPrevColumn, strTitle;
		int												iID;
		MetaDatabasePtr						pMD;
		MetaEntityPtr							pME;
		MetaColumnPtr							pMC;
		otl_nocommit_stream				oRslts;
		ostringstream							ostrm;
		bool											bFirst;


		// Build select for MetaEntity help topics
		Convert(strTargetMask, (int) HSTopic::GetCurrentTargetMask());

		switch (GetMetaDatabase()->GetTargetRDBMS())
		{
			case SQLServer:
			{
				strTargetFilter = "WHERE t.Targets&";
				strTargetFilter += strTargetMask;
				strTargetFilter += "=";
				strTargetFilter += strTargetMask;
				break;
			}

			case Oracle:
			{
				strTargetFilter = "WHERE BITAND(t.Targets,";
				strTargetFilter += strTargetMask;
				strTargetFilter += ")=";
				strTargetFilter += strTargetMask;
				break;
			}
		}

		strSQL = "SELECT mot.MetaDatabaseAlias, mot.MetaEntityName, mot.MetaColumnName, t.ID, t.Title FROM HSMetaColumnTopic mot LEFT JOIN HSTopic t ON mot.TopicID=t.ID ";
		strSQL += strTargetFilter;
		strSQL += " ORDER BY mot.MetaDatabaseAlias, mot.MetaEntityName, mot.MetaColumnName, t.Title";

		// open the query
		oRslts.open(50, strSQL.c_str(), m_oConnection);

		if (m_uTrace & D3DB_TRACE_SELECT)
			ReportInfo("OTLDatabase::SetMetaColumnHSTopics().......................: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this), strSQL.c_str());

		while (!oRslts.eof())
		{
			// read a record
			oRslts >> strCurrAlias;
			oRslts >> strCurrEntity;
			oRslts >> strCurrColumn;
			oRslts >> iID;
			oRslts >> strTitle;

			// Handle group breaks
			if (strCurrAlias != strPrevAlias || strCurrEntity != strPrevEntity || strCurrColumn != strPrevColumn)
			{
				if (!strPrevAlias.empty() && !strPrevEntity.empty() && !strPrevColumn.empty())
				{
					pMD = MetaDatabase::GetMetaDatabase(strPrevAlias);
					if (pMD)
					{
						pME = pMD->GetMetaEntity(strPrevEntity);
						if (pME)
						{
							pMC = pME->GetMetaColumn(strPrevColumn);
							if (pMC)
							{
								ostrm << ']';
								pMC->SetHSTopics(ostrm.str());
							}
						}
					}
				}

				bFirst = true;
				ostrm.str("");
				ostrm << '[';
				strPrevAlias = strCurrAlias;
				strPrevEntity = strCurrEntity;
				strPrevColumn = strCurrColumn;
			}

			if (bFirst)
				bFirst = false;
			else
				ostrm << ',';

			ostrm << "{\"id\":" << iID << ",\"title\":\"" << JSONEncode(strTitle) << "\"}";
		}

		if (!strPrevAlias.empty() && !strPrevEntity.empty() && !strPrevColumn.empty())
		{
			pMD = MetaDatabase::GetMetaDatabase(strPrevAlias);
			if (pMD)
			{
				pME = pMD->GetMetaEntity(strPrevEntity);
				if (pME)
				{
					pMC = pME->GetMetaColumn(strPrevColumn);
					if (pMC)
					{
						ostrm << ']';
						pMC->SetHSTopics(ostrm.str());
					}
				}
			}
		}
	}





	void OTLDatabase::SetMetaKeyHSTopics()
	{
		string										strSQL, strTargetID, strTargetMask, strTargetFilter, strCurrAlias, strPrevAlias, strCurrEntity, strPrevEntity, strCurrKey, strPrevKey, strTitle;
		int												iID;
		MetaDatabasePtr						pMD;
		MetaEntityPtr							pME;
		MetaKeyPtr								pMK;
		otl_nocommit_stream				oRslts;
		ostringstream							ostrm;
		bool											bFirst;


		// Build select for MetaEntity help topics
		Convert(strTargetMask, (int) HSTopic::GetCurrentTargetMask());

		switch (GetMetaDatabase()->GetTargetRDBMS())
		{
			case SQLServer:
			{
				strTargetFilter = "WHERE t.Targets&";
				strTargetFilter += strTargetMask;
				strTargetFilter += "=";
				strTargetFilter += strTargetMask;
				break;
			}

			case Oracle:
			{
				strTargetFilter = "WHERE BITAND(t.Targets,";
				strTargetFilter += strTargetMask;
				strTargetFilter += ")=";
				strTargetFilter += strTargetMask;
				break;
			}
		}

		strSQL = "SELECT mot.MetaDatabaseAlias, mot.MetaEntityName, mot.MetaKeyName, t.ID, t.Title FROM HSMetaKeyTopic mot LEFT JOIN HSTopic t ON mot.TopicID=t.ID ";
		strSQL += strTargetFilter;
		strSQL += " ORDER BY mot.MetaDatabaseAlias, mot.MetaEntityName, mot.MetaKeyName, t.Title";

		// open the query
		oRslts.open(50, strSQL.c_str(), m_oConnection);

		if (m_uTrace & D3DB_TRACE_SELECT)
			ReportInfo("OTLDatabase::SetMetaKeyHSTopics()..........................: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this), strSQL.c_str());

		while (!oRslts.eof())
		{
			// read a record
			oRslts >> strCurrAlias;
			oRslts >> strCurrEntity;
			oRslts >> strCurrKey;
			oRslts >> iID;
			oRslts >> strTitle;

			// Handle group breaks
			if (strCurrAlias != strPrevAlias || strCurrEntity != strPrevEntity || strCurrKey != strPrevKey)
			{
				if (!strPrevAlias.empty() && !strPrevEntity.empty() && !strPrevKey.empty())
				{
					pMD = MetaDatabase::GetMetaDatabase(strPrevAlias);
					if (pMD)
					{
						pME = pMD->GetMetaEntity(strPrevEntity);
						if (pME)
						{
							pMK = pME->GetMetaKey(strPrevKey);
							if (pMK)
							{
								ostrm << ']';
								pMK->SetHSTopics(ostrm.str());
							}
						}
					}
				}

				bFirst = true;
				ostrm.str("");
				ostrm << '[';
				strPrevAlias = strCurrAlias;
				strPrevEntity = strCurrEntity;
				strPrevKey = strCurrKey;
			}

			if (bFirst)
				bFirst = false;
			else
				ostrm << ',';

			ostrm << "{\"id\":" << iID << ",\"title\":\"" << JSONEncode(strTitle) << "\"}";
		}

		if (!strPrevAlias.empty() && !strPrevEntity.empty() && !strPrevKey.empty())
		{
			pMD = MetaDatabase::GetMetaDatabase(strPrevAlias);
			if (pMD)
			{
				pME = pMD->GetMetaEntity(strPrevEntity);
				if (pME)
				{
					pMK = pME->GetMetaKey(strPrevKey);
					if (pMK)
					{
						ostrm << ']';
						pMK->SetHSTopics(ostrm.str());
					}
				}
			}
		}
	}





	void OTLDatabase::SetMetaRelationHSTopics()
	{
		string										strSQL, strTargetID, strTargetMask, strTargetFilter, strCurrAlias, strPrevAlias, strCurrEntity, strPrevEntity, strCurrRelation, strPrevRelation, strTitle;
		int												iID;
		MetaDatabasePtr						pMD;
		MetaEntityPtr							pME;
		MetaRelationPtr						pMR;
		otl_nocommit_stream				oRslts;
		ostringstream							ostrm;
		bool											bFirst;


		// Build select for MetaEntity help topics
		Convert(strTargetMask, (int) HSTopic::GetCurrentTargetMask());

		switch (GetMetaDatabase()->GetTargetRDBMS())
		{
			case SQLServer:
			{
				strTargetFilter = "WHERE t.Targets&";
				strTargetFilter += strTargetMask;
				strTargetFilter += "=";
				strTargetFilter += strTargetMask;
				break;
			}

			case Oracle:
			{
				strTargetFilter = "WHERE BITAND(t.Targets,";
				strTargetFilter += strTargetMask;
				strTargetFilter += ")=";
				strTargetFilter += strTargetMask;
				break;
			}
		}

		strSQL = "SELECT mot.MetaDatabaseAlias, mot.MetaEntityName, mot.MetaRelationName, t.ID, t.Title FROM HSMetaRelationTopic mot LEFT JOIN HSTopic t ON mot.TopicID=t.ID ";
		strSQL += strTargetFilter;
		strSQL += " ORDER BY mot.MetaDatabaseAlias, mot.MetaEntityName, mot.MetaRelationName, t.Title";

		// open the query
		oRslts.open(50, strSQL.c_str(), m_oConnection);

		if (m_uTrace & D3DB_TRACE_SELECT)
			ReportInfo("OTLDatabase::SetMetaRelationHSTopics()..........................: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this), strSQL.c_str());

		while (!oRslts.eof())
		{
			// read a record
			oRslts >> strCurrAlias;
			oRslts >> strCurrEntity;
			oRslts >> strCurrRelation;
			oRslts >> iID;
			oRslts >> strTitle;

			// Handle group breaks
			if (strCurrAlias != strPrevAlias || strCurrEntity != strPrevEntity || strCurrRelation != strPrevRelation)
			{
				if (!strPrevAlias.empty() && !strPrevEntity.empty() && !strPrevRelation.empty())
				{
					pMD = MetaDatabase::GetMetaDatabase(strPrevAlias);
					if (pMD)
					{
						pME = pMD->GetMetaEntity(strPrevEntity);
						if (pME)
						{
							pMR = pME->GetChildMetaRelation(strPrevRelation);

							if (pMR)
							{
								ostrm << ']';
								pMR->SetHSTopics(ostrm.str());
							}
						}
					}
				}

				bFirst = true;
				ostrm.str("");
				ostrm << '[';
				strPrevAlias = strCurrAlias;
				strPrevEntity = strCurrEntity;
				strPrevRelation = strCurrRelation;
			}

			if (bFirst)
				bFirst = false;
			else
				ostrm << ',';

			ostrm << "{\"id\":" << iID << ",\"title\":\"" << JSONEncode(strTitle) << "\"}";
		}

		if (!strPrevAlias.empty() && !strPrevEntity.empty() && !strPrevRelation.empty())
		{
			pMD = MetaDatabase::GetMetaDatabase(strPrevAlias);
			if (pMD)
			{
				pME = pMD->GetMetaEntity(strPrevEntity);
				if (pME)
				{
					pMR = pME->GetChildMetaRelation(strPrevRelation);

					if (pMR)
					{
						ostrm << ']';
						pMR->SetHSTopics(ostrm.str());
					}
				}
			}
		}
	}






} // end namespace D3

#endif /*APAL_SUPPORT_OTL*/			// Skip entire file if no OTL support is wanted
