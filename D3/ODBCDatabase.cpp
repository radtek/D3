// MODULE: ODBCDatabase source
//;
//; IMPLEMENTATION CLASS: ODBCDatabase
//;
// ===========================================================
// File Info:
//
// $Author: lalit $
// $Date: 2014/12/16 12:54:22 $
// $Revision: 1.86 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 25/04/06 - 1.4 - Hugp
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

#ifdef APAL_SUPPORT_ODBC				// If not defind, skip entire file

#include "ODBCDatabase.h"
#include "Entity.h"
#include "Column.h"
#include "Key.h"
#include "Relation.h"
#include "ResultSet.h"
#include "Exception.h"
#include "XMLImporterExporter.h"
#include "HSTopic.h"

#include <boost/thread/recursive_mutex.hpp>

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
#include <SAX/InputSource.h>
#include <DOMWriter.h>
#include <XMLUtils.h>
#include <XMLException.h>

#include <sstream>

#include <Codec.h>


// Stolen from SQL Server Native Client header and needed for Query Notification
//
// Note: For some reason, Microsoft has not included this with the SQL Server 2005 ODBC headers
//       I assume the reason was that SQL QN was really designed for .NET users as apparently
//       only Erland Somerskeg has ever tried using it outside of .NET (OLE DB). Perhaps
//       future versions will have these defined, so if you get compile errors here it is
//       a sign the Microsoft finally noticed and fixed their mistake.
//
#define SQL_SOPT_SS_BASE                            1225
#define SQL_SOPT_SS_QUERYNOTIFICATION_TIMEOUT       (SQL_SOPT_SS_BASE+8) // Notification timeout
#define SQL_SOPT_SS_QUERYNOTIFICATION_MSGTEXT       (SQL_SOPT_SS_BASE+9) // Notification message text
#define SQL_SOPT_SS_QUERYNOTIFICATION_OPTIONS       (SQL_SOPT_SS_BASE+10)// SQL service broker name

#define LIBODBC_FETCH_SIZE	100



namespace D3
{
	#ifdef _MONITORODBCFUNC


		class MonitorODBCFunc
		{
			protected:
				void*					m_pObject;
				std::string		m_strMessage;

			public:
				MonitorODBCFunc(char* szMsg, void* pObj)
					: m_pObject(pObj)
				{
					m_strMessage = szMsg;

					for (unsigned int idx = 36 - strlen(szMsg); idx > 0; idx--)
						m_strMessage += '.';

					ReportDiagnostic("%s: Entered by database %x", m_strMessage.c_str(), m_pObject);
				}



				~MonitorODBCFunc()
				{
					ReportDiagnostic("%s: Exited  by database %x", m_strMessage.c_str(), m_pObject);
				}
		};


		#define MONITORFUNC(msg, db) MonitorODBCFunc mf(msg, db)

	#else

		#define MONITORFUNC(msg, db)

	#endif


	//===========================================================================
	//
	// Instances of this class manage the ODBCDatabase::m_uConnectionBusyCount flag and
	// connection. This class is the emans to avoid the "Connection is busy with
	// results for another command" odbc errors.
	//
	class ConnectionManager
	{
		friend class ODBCDatabase;

		protected:
			ODBCDatabasePtr										m_pDB;
			ODBCDatabase::ODBCConnectionPtr		m_pTempConnection;

			ConnectionManager(ODBCDatabasePtr pDB) : m_pDB(pDB), m_pTempConnection(NULL) {};
			~ConnectionManager() { idle(); };

			bool								isBusy()				{ return m_pDB->m_uConnectionBusyCount; }

//			void								busy()					{ m_pDB->m_uConnectionBusyCount++; }

			void								idle()
			{
				m_pDB->m_uConnectionBusyCount--;

				if (m_pTempConnection)
					ODBCDatabase::ReleaseODBCConnection(m_pDB->GetMetaDatabase(), m_pTempConnection);
			}

			ODBCDatabase::ODBCConnectionPtr		connection()
			{
				if (m_pDB->m_uConnectionBusyCount++)
				{
					if (!m_pTempConnection)
						m_pTempConnection = ODBCDatabase::CreateODBCConnection(m_pDB->GetMetaDatabase());

					return m_pTempConnection;
				}

				return m_pDB->m_pConnection;
			}
	};


	//===========================================================================
	//
	// This module subclasses XMLImportFileProcessor in order to deal with specifics
	// applicable when importing data into an ODBC database
	//
	class ODBCXMLImportFileProcessor : public XMLImportFileProcessor
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

				State								state;
				std::string					value;
				std::stringstream*	pStrm;

				ColumnData() : state(undefined), pStrm(NULL) {};

				~ColumnData()
				{
					delete pStrm;
				};

				void	Clear()
				{
					state = undefined;
				}
			};

			typedef std::map<string, ColumnData>	ColumnDataMap;
			typedef ColumnDataMap::iterator				ColumnDataMapItr;

			ColumnDataMap								m_mapColumnData;
			odbc::PreparedStatement*		m_pPrepStmnt;
			ODBCDatabase*								m_pDB;
			unsigned long								m_lMaxValue;
			bool												m_bFirstRecord;


			// Trap notifications
			//
			virtual void		On_BeforeProcessEntityListElement();
			virtual void		On_BeforeProcessEntityElement();

			virtual void		On_AfterProcessEntityListElement();
			virtual void		On_AfterProcessEntityElement();
			virtual void		On_AfterProcessColumnElement();

		public:
			ODBCXMLImportFileProcessor(const std::string & strAppName, ODBCDatabase* pDB, MetaEntityPtrListPtr pListME)
			 :	XMLImportFileProcessor(strAppName, pDB, pListME),
					m_pDB(pDB),
					m_pPrepStmnt(NULL),
					m_bFirstRecord(true)
			{}

			void						SetValue(MetaColumnPtr pMC, ColumnData & colData, int idxCol);

			void						TidyUp(const char * pszMsg);

			// Returns a string containing key values in the current <entity> tag in the form [col1:x][col2:y],...[coln:z]
			string					ReportKeys();
	};



	void ODBCXMLImportFileProcessor::On_BeforeProcessEntityListElement()
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

			try
			{
				m_pDB->BeforeImportData(m_pCurrentME);

				m_pDB->BeginTransaction();

				m_lMaxValue = 0;
				bSuccess = true;
			}
			catch(Exception& e)
			{
				e.LogError();
			}
			catch (odbc::SQLException& exODBC)
			{
				ReportError("ODBCXMLImportFileProcessor::On_BeforeProcessEntityListElement: ODBC-Exception. %s.", exODBC.getMessage().c_str());
			}
			catch (...)
			{
			}

			if (!bSuccess)
				throw CSAXException("ODBCXMLImportFileProcessor::On_BeforeProcessEntityListElement: Function failed, import terminated.");
		}
	}



	void ODBCXMLImportFileProcessor::On_BeforeProcessEntityElement()
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



	void ODBCXMLImportFileProcessor::On_AfterProcessEntityListElement()
	{
		if (!m_bSkipToNextSibling)
		{
			try
			{
				m_pDB->CommitTransaction();

				// Free prepared statement
				delete m_pPrepStmnt;
				m_pPrepStmnt = NULL;
				m_bFirstRecord = true;

				m_pDB->AfterImportData(m_pCurrentME, m_lMaxValue);
			}
			catch (Exception & e)
			{
				// Free prepared statement
				delete m_pPrepStmnt;
				m_pPrepStmnt = NULL;
				m_bFirstRecord = true;

				e.LogError();
				TidyUp("ODBCXMLImportFileProcessor::On_AfterProcessEntityListElement()");
			}
			catch (odbc::SQLException& e)
			{
				// Free prepared statement
				delete m_pPrepStmnt;
				m_pPrepStmnt = NULL;
				m_bFirstRecord = true;

				ReportError("ODBCXMLImportFileProcessor::On_AfterProcessEntityListElement(): %s", e.getMessage().c_str());
				TidyUp("ODBCXMLImportFileProcessor::On_AfterProcessEntityListElement()");
			}
			catch (...)
			{
				// Free prepared statement
				delete m_pPrepStmnt;
				m_pPrepStmnt = NULL;
				m_bFirstRecord = true;

				ReportError("ODBCXMLImportFileProcessor::On_AfterProcessEntityListElement(): Unspecified error.");
				TidyUp("ODBCXMLImportFileProcessor::On_AfterProcessEntityListElement()");
			}
		}

		XMLImportFileProcessor::On_AfterProcessEntityListElement();
	}



	void ODBCXMLImportFileProcessor::On_AfterProcessEntityElement()
	{
		if (!m_bSkipToNextSibling)
		{
			MetaColumnPtrVect::iterator			itrMEC;
			MetaColumnPtr										pMC;
			int															idxCol;


			try
			{
				if (m_bFirstRecord)
				{
					bool														bFirst = true;
					std::string											strCols, strVals, strSQL;


					// We need to stream the attributes in the correct order
					for ( itrMEC =  m_pCurrentME->GetMetaColumnsInFetchOrder()->begin();
								itrMEC != m_pCurrentME->GetMetaColumnsInFetchOrder()->end();
								itrMEC++)
					{
						pMC = *itrMEC;

						if (bFirst)
						{
							bFirst = false;
						}
						else
						{
							strCols += ',';
							strVals += ',';
						}

						strCols += pMC->GetName();

						// colData will tell us if pMC is supplied or not
						ColumnData&	colData = m_mapColumnData[pMC->GetName()];

						if (colData.state == ColumnData::undefined)
						{
							if (!pMC->IsDefaultValueNull())
							{
								// There is a default value for this column, so lets use it
								//
								switch (pMC->GetType())
								{
									case MetaColumn::dbfString:
									case MetaColumn::dbfDate:
										if (pMC->GetType() == MetaColumn::dbfDate && m_pDB->GetMetaDatabase()->GetTargetRDBMS() == Oracle)
										{
											strVals += "TO_DATE('";
											strVals += pMC->GetDefaultValue();
											strVals += "','YYYY-MM-DD HH24:MI:SS')";
										}
										else
										{
											strVals += '\'';
											strVals += pMC->GetDefaultValue();
											strVals += '\'';
										}
										break;

									case MetaColumn::dbfBinary:
										switch (m_pDB->GetMetaDatabase()->GetTargetRDBMS())
										{
											case SQLServer:
												strVals += std::string("0x") + pMC->GetDefaultValue();
												break;

											case Oracle:
												strVals += std::string("hextoraw('") + pMC->GetDefaultValue() + "')";
												break;
										}
										break;

									default:
										strVals += pMC->GetDefaultValue();
								}
							}
							else
							{
								// We need to provide some sensible default value for this column if it is
								// mandatory
								//
								if (pMC->IsMandatory())
								{
									switch (pMC->GetType())
									{
										case MetaColumn::dbfBlob:
										case MetaColumn::dbfString:
											strVals += "''";
											break;

										case MetaColumn::dbfBinary:
											switch (m_pDB->GetMetaDatabase()->GetTargetRDBMS())
											{
												case SQLServer:
													strVals += "0x0";
													break;

												case Oracle:
													strVals += "hextoraw('0')";
													break;
											}
											break;

										case MetaColumn::dbfDate:
										{
											D3Date				dtNow = D3Date::Now(m_pDB->GetMetaDatabase()->GetTimeZone());

											switch (m_pDB->GetMetaDatabase()->GetTargetRDBMS())
											{
												case SQLServer:
													strVals += '\'';
													strVals += dtNow.AsString(3);
													strVals += '\'';
													break;

												case Oracle:
													strVals += "TO_DATE('";
													strVals += dtNow.AsString();
													strVals += "','YYYY-MM-DD HH24:MI:SS')";
													break;
											}

											break;
										}

										default:
											if (pMC->IsAutoNum())
												strVals += "?";
											else
												strVals += "0";
									}
								}
								else
								{
									strVals += "NULL";
								}
							}
						}
						else
						{
							if (pMC->GetType() == MetaColumn::dbfDate && m_pDB->GetMetaDatabase()->GetTargetRDBMS() == Oracle)
							{
								strVals += "TO_DATE(?, 'YYYY-MM-DD HH24:MI:SS')";
								break;
							}
							else
							{
								strVals += '?';
							}
						}
					}

					// Create a prepared statement like "INSERT INTO tblname (Col1,Col2,..,Coln) VALUES (?,?,..,?)"
					//
					strSQL = "INSERT INTO ";
					strSQL+= m_pCurrentME->GetName();
					strSQL+= " (";
					strSQL+= strCols;
					strSQL+= ") VALUES (";
					strSQL+= strVals;
					strSQL+= ")";

					// We must prepare this statement
					//
					assert(m_pPrepStmnt==NULL);
					m_pPrepStmnt = m_pDB->m_pConnection->prepareStatement(strSQL);
					assert(m_pPrepStmnt);

					ReportDiagnostic("ODBCXMLImportFileProcessor::On_AfterProcessEntityElement().: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, 1, strSQL.c_str());

					m_bFirstRecord = false;
				}

				// We need to stream the attributes in the correct order
				idxCol = 0;

				for ( itrMEC =  m_pCurrentME->GetMetaColumnsInFetchOrder()->begin();
							itrMEC != m_pCurrentME->GetMetaColumnsInFetchOrder()->end();
							itrMEC++)
				{
					pMC = *itrMEC;

					ColumnData&	colData = m_mapColumnData[pMC->GetName()];

					// Ignore undefined columns except...
					if (colData.state == ColumnData::undefined)
					{
						// ...AutoNum columns
						if (pMC->IsAutoNum())
						{
							colData.state = ColumnData::defined;
							Convert(colData.value, m_lRecCountCurrent + 1);
						}
						else
						{
							continue;
						}
					}

					SetValue(pMC, colData, ++idxCol);
				}

				// Now do the insert
				m_pPrepStmnt->execute();
			}
			catch (odbc::SQLException& e)
			{
				if (m_pCurrentME->GetMetaDatabase()->GetTargetRDBMS() == SQLServer && (e.getErrorCode() == 2601 || e.getErrorCode() == 2627))
				{
					// constraint violation (typically caused by duplicates)
					// issue warning, skip record and continue
					ReportWarning("ODBCXMLImportFileProcessor::On_AfterProcessEntityElement(): Rec %i, entity %s has duplicate. More details follow...", m_lRecCountCurrent + 1, m_pCurrentME->GetFullName().c_str());
					ReportWarning("Duplicate record found: %s", ReportKeys().c_str());
					m_lRecCountCurrent--;
				}
				else
				{
					ReportError("ODBCXMLImportFileProcessor::On_AfterProcessEntityElement(): Rec %i, insert failed, more details follow...", m_lRecCountCurrent + 1);
					D3::Exception::GenericExceptionHandler(__FILE__, __LINE__);
					TidyUp("ODBCXMLImportFileProcessor::On_AfterProcessEntityElement()");
				}
			}
			catch (...)
			{
				ReportError("ODBCXMLImportFileProcessor::On_AfterProcessEntityElement(): Rec %i, insert failed, more details follow...", m_lRecCountCurrent + 1);
				D3::Exception::GenericExceptionHandler(__FILE__, __LINE__);
				TidyUp("ODBCXMLImportFileProcessor::On_AfterProcessEntityElement()");
			}
		}

		XMLImportFileProcessor::On_AfterProcessEntityElement();
	}



	void ODBCXMLImportFileProcessor::On_AfterProcessColumnElement()
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



	void ODBCXMLImportFileProcessor::SetValue(MetaColumnPtr pMC, ColumnData& colData, int idxCol)
	{
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
					ReportWarning("ODBCXMLImportFileProcessor::SetValue(): Removing trailing blanks from value '%s' for column %s.", colData.value.c_str(), pMC->GetFullName().c_str());
					colData.value.erase(posLastNonBlank + 1);
				}

				// If value is too big, truncate it and report a warning
				if (colData.value.length() > pMC->GetMaxLength())
				{
					ReportWarning("ODBCXMLImportFileProcessor::SetValue(): Value for column %s too long and has been truncated.", pMC->GetFullName().c_str());
					colData.value.resize(pMC->GetMaxLength());
				}

				// Treat like NULL if the length is 0
				if (colData.value.empty())
					colData.state = ColumnData::null;
			}

			// If this is a LOB, using streaming
			if (pMC->IsStreamed())
			{
				if (colData.state == ColumnData::defined)
				{
					if (pMC->GetType() == MetaColumn::dbfString)
					{
						// Creates a stream if it doesn't exist
						if (!colData.pStrm)
							colData.pStrm = new std::stringstream(std::ios_base::in | std::ios_base::out);
						else
							colData.pStrm->str("");

						*(colData.pStrm) << colData.value;

						m_pPrepStmnt->setAsciiStream(idxCol, colData.pStrm, colData.value.size());
					}
					else
					{
						int			lData;

						if (!colData.pStrm)
							colData.pStrm = new std::stringstream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
						else
							colData.pStrm->str("");

						lData	= APALUtil::base64_decode(colData.value, *(colData.pStrm));
						lData = colData.pStrm->str().size();

						m_pPrepStmnt->setBinaryStream(idxCol, colData.pStrm, lData);
					}
				}
				else
				{
					if (pMC->IsMandatory())
					{
						if (pMC->GetType() == MetaColumn::dbfString)
						{
							if (!colData.pStrm)
								colData.pStrm = new std::stringstream(std::ios_base::in | std::ios_base::out);
							else
								colData.pStrm->str("");

							m_pPrepStmnt->setAsciiStream(idxCol, colData.pStrm, 0);
						}
						else
						{
							if (!colData.pStrm)
								colData.pStrm = new std::stringstream(std::ios_base::in | std::ios_base::out | std::ios_base::binary);
							else
								colData.pStrm->str("");

							m_pPrepStmnt->setBinaryStream(idxCol, colData.pStrm, 0);
						}
					}
					else
					{
						if (pMC->GetType() == MetaColumn::dbfString)
							m_pPrepStmnt->setNull(idxCol, odbc::Types::LONGVARCHAR);
						else
							m_pPrepStmnt->setNull(idxCol, odbc::Types::LONGVARBINARY);
					}
				}
			}
			else
			{
				// Set the parameter appropriately
				switch (pMC->GetType())
				{
					case MetaColumn::dbfString:
						if (colData.state != ColumnData::defined)
							m_pPrepStmnt->setNull(idxCol, odbc::Types::VARCHAR);
						else
							m_pPrepStmnt->setString(idxCol, colData.value);

						break;

					case MetaColumn::dbfChar:
						if (colData.state == ColumnData::defined)
						{
							char			c;
							Convert(c, colData.value);
							m_pPrepStmnt->setByte(idxCol, c);
						}
						else
						{
							m_pPrepStmnt->setNull(idxCol, odbc::Types::TINYINT);
						}

						break;

					case MetaColumn::dbfShort:
						if (colData.state == ColumnData::defined)
						{
							short			s;
							Convert(s, colData.value);
							m_pPrepStmnt->setShort(idxCol, s);
						}
						else
						{
							m_pPrepStmnt->setNull(idxCol, odbc::Types::SMALLINT);
						}

						break;

					case MetaColumn::dbfBool:

						if (colData.state == ColumnData::defined)
						{
							bool			b;
							Convert(b, colData.value);
							m_pPrepStmnt->setBoolean(idxCol, b);
						}
						else
						{
							m_pPrepStmnt->setNull(idxCol, odbc::Types::BIT);
						}

						break;

					case MetaColumn::dbfInt:
						if (colData.state == ColumnData::defined)
						{
							int				i;
							Convert(i, colData.value);
							m_pPrepStmnt->setInt(idxCol, i);

							if (pMC->IsAutoNum())
								if ((unsigned long) i > m_lMaxValue)
									m_lMaxValue = (unsigned long) i;
						}
						else
						{
							m_pPrepStmnt->setNull(idxCol, odbc::Types::INTEGER);
						}

						break;

					case MetaColumn::dbfLong:
						if (colData.state == ColumnData::defined)
						{
							long			i;
							Convert(i, colData.value);
							m_pPrepStmnt->setLong(idxCol, i);

							if (pMC->IsAutoNum())
								m_lMaxValue = std::max((unsigned long) i, m_lMaxValue);
						}
						else
						{
							m_pPrepStmnt->setNull(idxCol, odbc::Types::INTEGER);
						}

						break;

					case MetaColumn::dbfFloat:
						if (colData.state == ColumnData::defined)
						{
							float			f;
							Convert(f, colData.value);
							m_pPrepStmnt->setFloat(idxCol, f);
						}
						else
						{
							if (m_pDB->GetMetaDatabase()->GetTargetRDBMS() == Oracle)
								m_pPrepStmnt->setNull(idxCol, odbc::Types::REAL);
							else
								m_pPrepStmnt->setNull(idxCol, odbc::Types::FLOAT);
						}

						break;

					case MetaColumn::dbfDate:
					{
						D3Date					dt;
						bool						bValidDate = false;

						if (colData.state == ColumnData::defined)
						{
							try
							{
								dt = colData.value;
								bValidDate = true;
							}
							catch (...)
							{
								ReportWarning("D3::ODBCXMLImportFileProcessor::SetValue(): Rec %i, error setting %s to '%s', more details follow...", m_lRecCountCurrent + 1, pMC->GetFullName().c_str(), colData.value.c_str());
								D3::Exception::GenericExceptionHandler(__FILE__,__LINE__);
							}
						}

						if (bValidDate || pMC->IsMandatory())
						{
							dt.AdjustToTimeZone(m_pDB->GetMetaDatabase()->GetTimeZone());

							switch (m_pDB->GetMetaDatabase()->GetTargetRDBMS())
							{
								case Oracle:
									m_pPrepStmnt->setTimestamp(idxCol, dt.AsString());
									break;

								case SQLServer:
								{
									m_pPrepStmnt->setString(idxCol, dt.AsString(3));

									break;
								}
							}
						}
						else
						{
							if (m_pDB->GetMetaDatabase()->GetTargetRDBMS() == Oracle)
								m_pPrepStmnt->setNull(idxCol, odbc::Types::DATE);
							else
								m_pPrepStmnt->setNull(idxCol, odbc::Types::TIMESTAMP);
						}

						break;
					}

					case MetaColumn::dbfBlob:
						// If we get here it's because the BLOB is NULL
						m_pPrepStmnt->setNull(idxCol, odbc::Types::LONGVARBINARY);
						break;

					case MetaColumn::dbfBinary:
						if (colData.state == ColumnData::defined)
						{
							unsigned char * pBuf;
							int							l = pMC->GetMaxLength();

							try
							{
								pBuf = new unsigned char [pMC->GetMaxLength()];
								pBuf = APALUtil::base64_decode((const unsigned char*) colData.value.c_str(), colData.value.size(), l);
								m_pPrepStmnt->setBytes(idxCol, odbc::Bytes((const signed char*) pBuf, l));
							}
							catch (...)
							{
								delete [] pBuf;
								throw;
							}
						}
						else
						{
							m_pPrepStmnt->setNull(idxCol, odbc::Types::VARCHAR);
						}

						break;

					default:
						throw Exception(__FILE__, __LINE__, Exception_error, "ODBCXMLImportFileProcessor::SetValue(): The datatype of column %s can't be handled by current import mechanism", pMC->GetFullName().c_str());
				}
			}
		}
		catch (...)
		{
			D3::Exception::GenericExceptionHandler(__FILE__, __LINE__);
			TidyUp("ODBCXMLImportFileProcessor::SetValue()");
		}
	}



	void ODBCXMLImportFileProcessor::TidyUp(const char * pszMsg)
	{
		std::string			strMessage = "Error occurred in ";

		strMessage += pszMsg;

		try
		{
			while (m_pDB->HasTransaction())
				m_pDB->RollbackTransaction();

			delete m_pPrepStmnt;
			m_pPrepStmnt = NULL;

			m_pDB->AfterImportData(m_pCurrentME, 0);
		}
		catch(...){}

		throw CSAXException(strMessage);
	}



	std::string ODBCXMLImportFileProcessor::ReportKeys()
	{
		std::ostringstream								ostrm;
		std::set<std::string>::iterator		itrCol;

		for ( itrCol =  m_setUKColumns.begin();
					itrCol != m_setUKColumns.end();
					itrCol++)
		{
			ColumnData&		colData = m_mapColumnData[*itrCol];
			ostrm << '[' << *itrCol << ':' << colData.value << ']';
		}

		return ostrm.str();
	}





	//===========================================================================
	//
	// ODBCDatabase implementation
	//
	D3_CLASS_IMPL(ODBCDatabase, Database);

	boost::recursive_mutex											ODBCDatabase::M_Mutex;
	boost::recursive_mutex											ODBCDatabase::M_CreateSPMutex;
	ODBCDatabase::ODBCConnectionPtrListMap			ODBCDatabase::M_mapODBCConnectionPtrLists;
	ODBCDatabase::NativeConnectionPtrListMap		ODBCDatabase::M_mapNativeConnectionPtrLists;
	bool																				ODBCDatabase::M_bQNInitialised = false;



	/* static */
	ODBCDatabase::ODBCConnectionPtr ODBCDatabase::CreateODBCConnection(MetaDatabasePtr pMDB)
	{
		boost::recursive_mutex::scoped_lock		lk(M_Mutex);

		ODBCConnectionPtrListMapItr		itr;
		ODBCConnectionPtr							pCon = NULL;

		itr = M_mapODBCConnectionPtrLists.find(pMDB);

		if (itr == M_mapODBCConnectionPtrLists.end() || itr->second.empty())
		{
			pCon = odbc::DriverManager::getConnection(pMDB->GetConnectionString());
			pCon->setTransactionIsolation(odbc::Connection::TRANSACTION_READ_COMMITTED);
			pCon->setAutoCommit(true);
		}
		else
		{
			pCon = itr->second.front();
			itr->second.pop_front();
		}

		return pCon;
	}





	/* static */
	void ODBCDatabase::ReleaseODBCConnection(MetaDatabasePtr pMDB, ODBCDatabase::ODBCConnectionPtr & pODBCConnection)
	{
		boost::recursive_mutex::scoped_lock		lk(M_Mutex);

		ODBCConnectionPtrListMapItr	itr;

		itr = M_mapODBCConnectionPtrLists.find(pMDB);

		if (itr == M_mapODBCConnectionPtrLists.end())
		{
			M_mapODBCConnectionPtrLists[pMDB] = ODBCConnectionPtrList();
			itr = M_mapODBCConnectionPtrLists.find(pMDB);
		}

		if (itr->second.size() < ODBC_DATABASE_MAX_POOLSIZE)
		{
			M_mapODBCConnectionPtrLists[pMDB].push_back(pODBCConnection);
		}
		else
		{
			delete pODBCConnection;
		}

		pODBCConnection = NULL;
	}





	// Return an odbc++ connection
	/* static */
	ODBCDatabase::NativeConnectionPtr ODBCDatabase::CreateNativeConnection(MetaDatabasePtr pMDB)
	{
		boost::recursive_mutex::scoped_lock		lk(M_Mutex);

		NativeConnectionPtrListMapItr	itr;
		NativeConnectionPtr							pCon = NULL;

		itr = M_mapNativeConnectionPtrLists.find(pMDB);

		if (itr == M_mapNativeConnectionPtrLists.end() || itr->second.empty())
		{
			SQLRETURN								ret;
			SQLCHAR									dummy[256];
			SQLSMALLINT							dummySize;
			std::string							strErrPrefix("ODBCDatabase::CreateNativeConnection(): ");


			try
			{
				pCon = new NativeConnection();

				// Allocate environment handle
				ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &pCon->hEnv);

				if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
					throw	strErrPrefix + "Failed to allocate environment handle. ";

				// Set environment attributes
				ret = SQLSetEnvAttr(pCon->hEnv, SQL_ATTR_ODBC_VERSION, (void*)SQL_OV_ODBC3, NULL);

				if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
					throw GetODBCError(SQL_HANDLE_ENV, pCon->hEnv, strErrPrefix + "Failed to set environment attributes. ");

				// Allocate connection handle
				ret = SQLAllocHandle(SQL_HANDLE_DBC, pCon->hEnv, &pCon->hCon);

				if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
					throw GetODBCError(SQL_HANDLE_ENV, pCon->hEnv, strErrPrefix + "Failed to allocate connection handle. ");

				// Connect to database
				ret = SQLDriverConnect(pCon->hCon, 0, (SQLCHAR*) pMDB->GetConnectionString().c_str(), (SQLSMALLINT) pMDB->GetConnectionString().size(), dummy, 255, &dummySize, SQL_DRIVER_COMPLETE);

				if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
					throw GetODBCError(SQL_HANDLE_DBC, pCon->hCon, strErrPrefix + "Failed to allocate connect to database. ");

				return pCon;
			}
			catch (std::string & e)
			{
				// Release the resources
				if (pCon->hCon)
				{
					SQLFreeHandle(SQL_HANDLE_DBC, pCon->hCon);
					pCon->hCon = NULL;
				}

				if (pCon->hEnv)
				{
					SQLFreeHandle(SQL_HANDLE_ENV, pCon->hEnv);
					pCon->hEnv = NULL;
				}

				throw Exception(__FILE__, __LINE__, Exception_error, e.c_str());
			}
		}

		pCon = itr->second.front();
		itr->second.pop_front();

		return pCon;
	}





	/* static */
	void ODBCDatabase::ReleaseNativeConnection(MetaDatabasePtr pMDB, ODBCDatabase::NativeConnectionPtr & pNativeConnection)
	{
		boost::recursive_mutex::scoped_lock		lk(M_Mutex);

		NativeConnectionPtrListMapItr	itr;

		itr = M_mapNativeConnectionPtrLists.find(pMDB);

		if (itr == M_mapNativeConnectionPtrLists.end())
		{
			M_mapNativeConnectionPtrLists[pMDB] = NativeConnectionPtrList();
			itr = M_mapNativeConnectionPtrLists.find(pMDB);
		}

		if (itr->second.size() < ODBC_DATABASE_MAX_POOLSIZE)
		{
			M_mapNativeConnectionPtrLists[pMDB].push_back(pNativeConnection);
		}
		else
		{
			// Release the resources
			if (pNativeConnection->hCon)
			{
				SQLFreeHandle(SQL_HANDLE_DBC, pNativeConnection->hCon);
				pNativeConnection->hCon = NULL;
			}

			if (pNativeConnection->hEnv)
			{
				SQLFreeHandle(SQL_HANDLE_ENV, pNativeConnection->hEnv);
				pNativeConnection->hEnv = NULL;
			}

			delete pNativeConnection;
		}

		pNativeConnection = NULL;
	}





	/* static */
	void ODBCDatabase::DeleteConnectionPools()
	{
		boost::recursive_mutex::scoped_lock		lk(M_Mutex);

		ODBCConnectionPtrListMapItr		itrODBC;
		NativeConnectionPtrListMapItr	itrNative;

		// Discard all queued ODBCConnections
		while (!M_mapODBCConnectionPtrLists.empty())
		{
			itrODBC = M_mapODBCConnectionPtrLists.begin();

			while (!itrODBC->second.empty())
			{
				delete itrODBC->second.front();
				itrODBC->second.pop_front();
			}

			M_mapODBCConnectionPtrLists.erase(M_mapODBCConnectionPtrLists.begin());
		}

		// Discard all queued NativeConnections
		while (!M_mapNativeConnectionPtrLists.empty())
		{
			itrNative = M_mapNativeConnectionPtrLists.begin();

			while (!itrNative->second.empty())
			{
				NativeConnectionPtr pCon = itrNative->second.front();

				itrNative->second.pop_front();

				// Release the resources
				if (pCon->hCon)
				{
					SQLFreeHandle(SQL_HANDLE_DBC, pCon->hCon);
					pCon->hCon = NULL;
				}

				if (pCon->hEnv)
				{
					SQLFreeHandle(SQL_HANDLE_ENV, pCon->hEnv);
					pCon->hEnv = NULL;
				}

				delete pCon;
			}

			M_mapNativeConnectionPtrLists.erase(M_mapNativeConnectionPtrLists.begin());
		}
	}





	ODBCDatabase::ODBCDatabase()
	 : m_pConnection(NULL), m_bIsConnected(false), m_uConnectionBusyCount(0)
	{
	}



	ODBCDatabase::~ODBCDatabase ()
	{
		try
		{
			// We should never receive this message while transactions are pending, but throwing
			// an exception in a destructor is bad practise
			//
			if (HasTransaction())
			{
				while (HasTransaction())
					RollbackTransaction();

				ReportError("ODBCDatabase::~ODBCDatabase(): dtor invoked on an instance of MetaDatabase %s with a pending transaction. Transaction rolled back.", m_pMetaDatabase->GetName().c_str());
			}

			// Physically disconnect
			Disconnect();
		}
		catch(Exception& e)
		{
			e.LogError();
		}
		catch(odbc::SQLException& e)
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "ODBCDatabase::~ODBCDatabase(): ODBC Error: %s", e.getMessage().c_str());
		}
	}



	void ODBCDatabase::On_PostCreate()
	{
		Database::On_PostCreate();

#ifdef _DEBUG
		TraceAll();
#endif
	}



	// Connect using the associated MeytaDatabase's connection string
	//
	bool ODBCDatabase::Connect()
	{
		// Have we got a connection already???
		if (m_bIsConnected)
		{
			ReportWarning("ODBCDatabase::Connect(): This instance of database %s is already connected, ignoring request.", m_pMetaDatabase->GetAlias().c_str());
			return true;
		}

 		// We need a connection string
		//
 		if (!m_pMetaDatabase || m_pMetaDatabase->GetConnectionString().empty())
		{
			throw Exception(__FILE__, __LINE__, Exception_error, "ODBCDatabase::Connect(): Unable to connect to database as MetaDatabase object or connection string is not known.");
		}
		else
		{
			try
			{
				// If we have a connection object, we try and recover from the previous error
				if (m_pConnection)
				{
					delete m_pConnection;
					m_pConnection = NULL;
				}

				m_pConnection = CreateODBCConnection(m_pMetaDatabase);

				if (m_uTrace & D3DB_TRACE_SELECT)
					ReportInfo("ODBCDatabase::Connect().............: Database " PRINTF_POINTER_MASK, this);

				m_bIsConnected = true;

				if (!m_pMetaDatabase->VersionChecked() && !CheckDatabaseVersion())
				{
					Disconnect();
					throw Exception(__FILE__, __LINE__, Exception_error, "ODBCDatabase::Connect(): Database %s is not of the expected version %s!", m_pMetaDatabase->GetName().c_str(), m_pMetaDatabase->GetVersion().c_str());
				}

				// We use this so we can retrieve detailed meta data from libodbc++ for all columns returned by a query
				std::auto_ptr<odbc::Statement>	pStmnt(m_pConnection->createStatement(odbc::ResultSet::TYPE_SCROLL_INSENSITIVE, odbc::ResultSet::CONCUR_READ_ONLY));
				pStmnt->execute("SET NO_BROWSETABLE ON");
			}
			catch(odbc::SQLException& e)
			{
				// Translate exception to D3::Exception
				delete m_pConnection;
				m_pConnection = NULL;
				throw Exception(__FILE__, __LINE__, Exception_error, "ODBCDatabase::Connect(): An odbc++ error occurred connecting to %s. %s", m_pMetaDatabase->GetName().c_str(), e.getMessage().c_str());
			}
			catch(...)
			{
				// Translate exception to D3::Exception
				delete m_pConnection;
				m_pConnection = NULL;
				throw;
			}
		}

		return true;
	}



	bool ODBCDatabase::Disconnect ()
	{
		ClearCache();

		if (m_pConnection)
		{
			try
			{
				if (m_bIsConnected)
				{
					// Rollback all pending transactions
					//
					while (HasTransaction())
						RollbackTransaction();

					ReleaseODBCConnection(m_pMetaDatabase, m_pConnection);

					if (m_uTrace & D3DB_TRACE_SELECT)
						ReportInfo("ODBCDatabase::Disconnect()..........: Database " PRINTF_POINTER_MASK, this);

					m_bIsConnected = false;
				}
				else
				{
					delete m_pConnection;
					m_pConnection = NULL;
				}
			}
			catch (odbc::SQLException& e)
			{
				GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "ODBCDatabase::Disconnect(): An odbc++ error occurred disconnecting instance of %s. %s", m_pMetaDatabase->GetName().c_str(), e.getMessage().c_str());
				return false;
			}
		}
		return true;
	}



	bool ODBCDatabase::HasTransaction()
	{
		std::auto_ptr<boost::recursive_mutex::scoped_lock>	lk(m_pMetaDatabase->m_TransactionManager.UseTransaction(this));

		if (lk.get() != NULL)
			return true;

		return false;
	}



	bool ODBCDatabase::BeginTransaction()
	{
		try
		{
			Reconnect();

			std::auto_ptr<boost::recursive_mutex::scoped_lock>	lk(m_pMetaDatabase->m_TransactionManager.BeginTransaction(this));
			int																									iTransactionCount = m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this);

			if (iTransactionCount == 1)
				m_pConnection->setAutoCommit(false);

			if (m_uTrace & (D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT))
				ReportInfo("ODBCDatabase::BeginTransaction()....: Database " PRINTF_POINTER_MASK " (Transaction count: %u) transaction started.", this, iTransactionCount);
		}
		catch(odbc::SQLException& e)
		{
			CheckConnection(e);
			throw;
		}

		return true;
	}



	bool ODBCDatabase::CommitTransaction()
	{
		try
		{
			std::auto_ptr<boost::recursive_mutex::scoped_lock>	lk(m_pMetaDatabase->m_TransactionManager.CommitTransaction(this));
			int																									iTransactionCount = m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this);
			const char*																					pszMode = "committed";

			if (lk.get() == NULL)
				return false;

			Reconnect();

			if (iTransactionCount == 0)
			{
				if (!m_pMetaDatabase->m_TransactionManager.ShouldRollback(this))
				{
					m_pConnection->commit();
				}
				else
				{
					m_pConnection->rollback();
					pszMode = "rolled back";
				}

				m_pConnection->setAutoCommit(true);
			}

			if (m_uTrace & (D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT))
				ReportInfo("ODBCDatabase::CommitTransaction()...: Database " PRINTF_POINTER_MASK " (Transaction count: %u) %s.", this, iTransactionCount, pszMode);
		}
		catch(odbc::SQLException& e)
		{
			CheckConnection(e);
			ReportError("ODBCDatabase::CommitTransaction(): odbc::Connection::commit() failed. %s", (void*) e.getMessage().c_str());
			try
			{
				if (m_pConnection)
					m_pConnection->setAutoCommit(true);
			}
			catch (...) {}
			return false;
		}
		catch(...)
		{
			GenericExceptionHandler("ODBCDatabase::CommitTransaction(): odbc::Connection::commit() failed.");
			try
			{
				if (m_pConnection)
					m_pConnection->setAutoCommit(true);
			}
			catch (...) {}
			return false;
		}

		return true;
	}



	bool ODBCDatabase::RollbackTransaction()
	{
		try
		{
			std::auto_ptr<boost::recursive_mutex::scoped_lock>	lk(m_pMetaDatabase->m_TransactionManager.RollbackTransaction(this));
			int																									iTransactionCount = m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this);

			if (lk.get() == NULL)
				return false;

			Reconnect();

			if (iTransactionCount == 0)
			{
				m_pConnection->rollback();
				m_pConnection->setAutoCommit(true);
			}

			if (m_uTrace & (D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT))
				ReportInfo("ODBCDatabase::RollbackTransaction().: Database " PRINTF_POINTER_MASK " (Transaction count: %u) rolled back", this, iTransactionCount);
		}
		catch(odbc::SQLException& e)
		{
			CheckConnection(e);
			ReportError("ODBCDatabase::RollbackTransaction(): Database " PRINTF_POINTER_MASK " (Transaction count: 0) roll back failed. %s", this, (void*) e.getMessage().c_str());
			try
			{
				if (m_pConnection)
					m_pConnection->setAutoCommit(true);
			}
			catch (...) {}
			return false;
		}
		catch(...)
		{
			GenericExceptionHandler("ODBCDatabase::RollbackTransaction(): Database " PRINTF_POINTER_MASK " (Transaction count: 0) roll back failed.");
			try
			{
				if (m_pConnection)
					m_pConnection->setAutoCommit(true);
			}
			catch (...) {}
			return false;
		}

		return true;
	}



	bool ODBCDatabase::CheckDatabaseVersion()
	{
		ConnectionManager		conMgr(this);

		std::auto_ptr<odbc::Statement>	pStmnt;
		std::auto_ptr<odbc::ResultSet>	pRslts;
		MetaEntityPtr				pD3VI;
		std::string					strSQL;
		bool								bResult = false;

		pD3VI = GetMetaDatabase()->GetVersionInfoMetaEntity();

		// Return success if this has no version info information
		//
		if (!pD3VI)
			return true;

		switch (GetMetaDatabase()->GetTargetRDBMS())
		{
			case SQLServer:
			{
				strSQL  = "SELECT TOP 1 ";
				strSQL += pD3VI->AsSQLSelectList();
				strSQL += " FROM ";
				strSQL += pD3VI->GetName();
				strSQL += " ORDER BY LastUpdated DESC";
				break;
			}

			case Oracle:
			{
				strSQL  = "SELECT ";
				strSQL += pD3VI->AsSQLSelectList();
				strSQL += " FROM (SELECT * FROM ";
				strSQL += pD3VI->GetName();
				strSQL += " ORDER BY LastUpdated DESC) WHERE ROWNUM <=1";
				break;
			}
		}

		try
		{
			Reconnect();

			pStmnt.reset(conMgr.connection()->createStatement(odbc::ResultSet::TYPE_SCROLL_INSENSITIVE, odbc::ResultSet::CONCUR_READ_ONLY));

			if (m_uTrace & D3DB_TRACE_SELECT)
				ReportInfo("ODBCDatabase::CheckDatabaseVersion().......................: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this), strSQL.c_str());

			pRslts.reset(pStmnt->executeQuery(strSQL));

			if (pRslts->first())
			{
				while (!pRslts->isAfterLast())
				{
					// Check the database target
					//
					if (m_pMetaDatabase->GetAlias() != pRslts->getString(2))
						break;

					// Check Version Major
					//
					if (m_pMetaDatabase->GetVersionMajor() != pRslts->getInt(3))
						break;

					// Check Version Minor
					//
					if (m_pMetaDatabase->GetVersionMinor() != pRslts->getInt(4))
						break;

					// Check Version Revision
					//
					if (m_pMetaDatabase->GetVersionRevision() != pRslts->getInt(5))
						break;

					m_pMetaDatabase->VersionChecked(true);

					bResult = true;
					break;
				}
			}
		}
		catch(odbc::SQLException& e)
		{
			CheckConnection(e);
			throw;
		}

		return bResult;
	}




	ColumnPtr ODBCDatabase::LoadColumn(ColumnPtr pColumn)
	{
		ConnectionManager		conMgr(this);

		std::auto_ptr<odbc::Statement>	pStmnt;
		std::auto_ptr<odbc::ResultSet>	pRslts;
		std::string							strSQL;
		EntityPtrList						listEntity;
		bool										bFirst, bResult;
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
			Reconnect();

			if (m_uTrace & D3DB_TRACE_SELECT)
				ReportInfo("ODBCDatabase::LoadColumn().........: Database " PRINTF_POINTER_MASK ". SQL: %s", this, strSQL.c_str());

			pStmnt.reset(conMgr.connection()->createStatement(odbc::ResultSet::TYPE_SCROLL_INSENSITIVE, odbc::ResultSet::CONCUR_READ_ONLY));
			pRslts.reset(pStmnt->executeQuery(strSQL));

			if (pRslts->first())
			{
				while (!pRslts->isAfterLast())
				{
					bResult = false;

					switch (pColumn->GetMetaColumn()->GetType())
					{
						case MetaColumn::dbfString:
							bResult = pColumn->SetValue(pRslts->getString(1));
							break;

						case MetaColumn::dbfChar:
							bResult = pColumn->SetValue((char) pRslts->getByte(1));
							break;

						case MetaColumn::dbfShort:
							bResult = pColumn->SetValue((short) pRslts->getShort(1));
							break;

						case MetaColumn::dbfBool:
							bResult = pColumn->SetValue((bool) pRslts->getBoolean(1));
							break;

						case MetaColumn::dbfInt:
							bResult = pColumn->SetValue((int) pRslts->getInt(1));
							break;

						case MetaColumn::dbfLong:
							bResult = pColumn->SetValue((long) pRslts->getLong(1));
							break;

						case MetaColumn::dbfFloat:
							bResult = pColumn->SetValue((float) pRslts->getFloat(1));
							break;

						case MetaColumn::dbfDate:
						{
							odbc::Timestamp		odbcTimestamp;

							odbcTimestamp = pRslts->getTimestamp(1);

							if (!pRslts->wasNull())
							{
  							try
  							{
									D3Date		dt(odbcTimestamp, m_pMetaDatabase->GetTimeZone());
									dt.AdjustToTimeZone(D3TimeZone::GetLocalServerTimeZone());
    							bResult = pColumn->SetValue(dt);
    						}
         				catch (...)
			       		{
									bResult = false;
									throw;
  							}
							}

							break;
						}
						case MetaColumn::dbfBlob:
						{
							try
							{
								std::istream*			pStrm = pRslts->getBinaryStream(1);

								if (!pRslts->wasNull())
								{
  								try
  								{
										((ColumnBlobPtr) pColumn)->FromStream(*pStrm);
    								bResult = true;
    							}
         					catch (...)
			       			{
										bResult = false;
										throw;
  								}
								}
  						}
							catch (...)
							{
								bResult = false;
								throw;
							}
							break;
						}
						case MetaColumn::dbfBinary:
						{
							try
							{
								odbc::Bytes		bytes = pRslts->getBytes(1);

								if (!pRslts->wasNull())
								{
  								try
  								{
										((ColumnBinaryPtr) pColumn)->SetValue((const unsigned char*) bytes.getData(), bytes.getSize());
    								bResult = true;
    							}
         					catch (...)
			       			{
										bResult = false;
										throw;
  								}
								}
  						}
							catch (...)
							{
								bResult = false;
								throw;
							}
							break;
						}
					}

					// Deal with errors and nulls
					if (!bResult)
					{
						pColumn->MarkUnfetched();
					}
					else
					{
						pColumn->MarkFetched();

						if (pRslts->wasNull())
							pColumn->SetNull();
					}

					assert(!pRslts->next()); // this loop must only be traversed once!
					break;
				}
			}
		}
		catch(odbc::SQLException& e)
		{
			CheckConnection(e);
			throw;
		}

		return bResult ? pColumn : NULL;
	}




	EntityPtr ODBCDatabase::LoadObject(KeyPtr pKey, bool bRefresh, bool bLazyFetch)
	{
		InstanceKeyPtr		pIK;


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


			// Build SQL
			//
			strSQL  = "SELECT ";
			strSQL += pKey->GetMetaKey()->GetMetaEntity()->AsSQLSelectList(bLazyFetch);
			strSQL += " FROM ";
			strSQL += pKey->GetMetaKey()->GetMetaEntity()->GetName();

			switch (GetMetaDatabase()->GetTargetRDBMS())
			{
				case SQLServer:
					strSQL += " WITH(NOLOCK) WHERE ";
					break;

				case Oracle:
					strSQL += " WHERE ";
					break;
			}

			bFirst = true;

			for ( itrKeyCol =  pKey->GetColumns().begin();
						itrKeyCol != pKey->GetColumns().end();
						itrKeyCol++)
			{
				pCol = *itrKeyCol;

				if (!bFirst)
					strSQL += " AND ";
				else
					bFirst =  false;

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



	long ODBCDatabase::LoadObjects(RelationPtr pRelation, bool bRefresh, bool bLazyFetch)
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

		// Build SQL
		//
		strSQL  = "SELECT ";
		strSQL += pMetaKey->GetMetaEntity()->AsSQLSelectList(bLazyFetch);
		strSQL += " FROM ";
		strSQL += pMetaKey->GetMetaEntity()->GetName();

		switch (GetMetaDatabase()->GetTargetRDBMS())
		{
			case SQLServer:
				strSQL += " WITH(NOLOCK) WHERE ";
				break;

			case Oracle:
				strSQL += " WHERE ";
				break;
		}

		// Build the SQL where clause
		//
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



	long ODBCDatabase::LoadObjects(MetaKeyPtr pMetaKey, KeyPtr pKey, bool bRefresh, bool bLazyFetch)
	{
		ColumnPtrListItr				itrSrceColumn;
		MetaColumnPtrListItr		itrTrgtColumn;
		ColumnPtr								pSrce;
		MetaColumnPtr						pTrgt;
		std::string							strSQL;
		std::string							strWHERE;
		EntityPtrList						listEntity;


		assert(pMetaKey);;
		assert(pKey);
		assert(m_pMetaDatabase == pMetaKey->GetMetaEntity()->GetMetaDatabase());

		// Build SQL
		//
		strSQL  = "SELECT ";
		strSQL += pMetaKey->GetMetaEntity()->AsSQLSelectList(bLazyFetch);
		strSQL += " FROM ";
		strSQL += pMetaKey->GetMetaEntity()->GetName();

 		switch (GetMetaDatabase()->GetTargetRDBMS())
		{
			case SQLServer:
				strSQL += " WITH(NOLOCK) WHERE ";
				break;

			case Oracle:
				strSQL += " WHERE ";
				break;
		}

		// Build the SQL where clause
		//
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
		}
		catch (Exception & e)
		{
			e.LogError();
			return -1;
		}

		return listEntity.size();
	}



	long ODBCDatabase::LoadObjects(MetaEntityPtr pME, bool bRefresh, bool bLazyFetch)
	{
		std::string							strSQL;
		EntityPtrList						listEntity;


		// Build SQL
		//
		strSQL = "SELECT ";
		strSQL += pME->AsSQLSelectList(bLazyFetch);
		strSQL += " FROM ";
		strSQL += pME->GetName();

		switch (GetMetaDatabase()->GetTargetRDBMS())
		{
			case SQLServer:
				strSQL += " WITH(NOLOCK)";
				break;

			case Oracle:
				break;
		}

		LoadObjects(pME, &listEntity, strSQL, bRefresh, bLazyFetch);

		return listEntity.size();
	}



	EntityPtrListPtr ODBCDatabase::LoadObjects(MetaEntityPtr pMetaEntity, EntityPtrListPtr pEntityList, const std::string & strSQL, bool bRefresh, bool bLazyFetch)
	{
		ConnectionManager		conMgr(this);

		std::auto_ptr<odbc::Statement>	pStmnt;
		std::auto_ptr<odbc::ResultSet>	pRslts;
		EntityPtr								pObject;
		EntityPtrListPtr				pEL = pEntityList;
		bool										bFreeEL = false;
		bool										bSuccess = false;

 		// Build the query std::string
		if (!pMetaEntity)
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "ODBCDatabase::LoadObjects(): Received request to load data from unknown table.");
			return NULL;
		}

		try
		{
			Reconnect();

			if (!pEL)
			{
				pEL = new EntityPtrList();
				bFreeEL = true;
			}

			if (m_uTrace & D3DB_TRACE_SELECT)
				ReportInfo("ODBCDatabase::LoadObjects()................................: Database " PRINTF_POINTER_MASK ". SQL: %s", this, strSQL.c_str());

			pStmnt.reset(conMgr.connection()->createStatement(odbc::ResultSet::TYPE_SCROLL_INSENSITIVE, odbc::ResultSet::CONCUR_READ_ONLY));
			pStmnt->setFetchSize(LIBODBC_FETCH_SIZE);
			pRslts.reset(pStmnt->executeQuery(strSQL));

			if (pRslts->first())
			{
				while (!pRslts->isAfterLast())
				{
					pObject = PopulateObject(pMetaEntity, pRslts.get(), bRefresh, bLazyFetch);
					pEL->push_back(pObject);
					pRslts->next();
				}
			}

			if (m_uTrace & D3DB_TRACE_STATS)
				ReportInfo("ODBCDatabase::LoadObjects().........: Database " PRINTF_POINTER_MASK ". %d for SQL: %s", this, pEL->size(), strSQL.c_str());

			bSuccess = true;
		}
		catch(odbc::SQLException& e)
		{
			CheckConnection(e);
			if (bFreeEL) delete pEL;
			throw;
		}
		catch(...)
		{
			if (bFreeEL) delete pEL;
			throw;
		}

		if (!bSuccess)
		{
			if (bFreeEL) delete pEL;
			return NULL;
		}

		return pEL;
	}



	EntityPtr ODBCDatabase::FindEntity(MetaEntityPtr pMetaEntity, odbc::ResultSet* pRslts)
	{
		TemporaryKey			tmpKey = *(pMetaEntity->GetPrimaryMetaKey());
		ColumnPtrListItr	itrColumn;
		ColumnPtr					pCol;
		InstanceKeyPtr		pInstanceKey;
		bool							bResult = false;


		for (	itrColumn =  tmpKey.GetColumns().begin();
					itrColumn != tmpKey.GetColumns().end();
					itrColumn++)
		{

			pCol = *itrColumn;

			switch (pCol->GetMetaColumn()->GetType())
			{
				case MetaColumn::dbfString:
					bResult = pCol->SetValue(pRslts->getString(pCol->GetMetaColumn()->GetName()));
					break;

				case MetaColumn::dbfChar:
					bResult = pCol->SetValue((char) pRslts->getByte(pCol->GetMetaColumn()->GetName()));
					break;

				case MetaColumn::dbfShort:
					bResult = pCol->SetValue((short) pRslts->getShort(pCol->GetMetaColumn()->GetName()));
					break;

				case MetaColumn::dbfBool:
					bResult = pCol->SetValue((bool) pRslts->getBoolean(pCol->GetMetaColumn()->GetName()));
					break;

				case MetaColumn::dbfInt:
					bResult = pCol->SetValue((int) pRslts->getInt(pCol->GetMetaColumn()->GetName()));
					break;

				case MetaColumn::dbfLong:
					bResult = pCol->SetValue((long) pRslts->getLong(pCol->GetMetaColumn()->GetName()));
					break;

				case MetaColumn::dbfFloat:
					bResult = pCol->SetValue((float) pRslts->getFloat(pCol->GetMetaColumn()->GetName()));
					break;

				case MetaColumn::dbfDate:
				{
					odbc::Timestamp		odbcTimestamp;

					odbcTimestamp = pRslts->getTimestamp(pCol->GetMetaColumn()->GetName());

					if (!pRslts->wasNull())
					{
						try
						{
  						bResult = pCol->SetValue(D3Date(odbcTimestamp, m_pMetaDatabase->GetTimeZone()));
  					}
       			catch (...)
          	{
							pCol->SetNull();
						}
					}

					break;
				}

				case MetaColumn::dbfBinary:
				{
					odbc::Bytes bytes = pRslts->getBytes(pCol->GetMetaColumn()->GetName());
					bResult = pCol->SetValue((const unsigned char*) bytes.getData(), bytes.getSize());
					break;
				}

				default:
					bResult = false;
			}

			if (!bResult)
				break;

			if (pRslts->wasNull())
				pCol->SetNull();
		}

		if (bResult)
		{
			pInstanceKey = pMetaEntity->GetPrimaryMetaKey()->FindInstanceKey(&tmpKey, this);

			if (pInstanceKey)
				return pInstanceKey->GetEntity();
		}

		return NULL;
	}



	EntityPtr ODBCDatabase::PopulateObject(MetaEntityPtr pMetaEntity, odbc::ResultSet* pRslts, bool bRefresh, bool bLazyFetch)
	{
		unsigned int							i, iColIdx;
		ColumnPtr									pCol;
		MetaColumnPtr							pMetaCol;
		bool											bResult = false;
		bool											bDeletObject = false;
		bool											bDoAfterPopulate = false;
		EntityPtr									pObject = NULL;


		try
		{
			// See if the object exists
			//
			pObject = FindEntity(pMetaEntity, pRslts);

			if (!pObject)
			{
				bDeletObject = true;
				pObject = pMetaEntity->CreateInstance(this);
			}
			else
			{
				if (!bRefresh)
					return pObject;
			}

			if (!pObject)
				return NULL;

			bDoAfterPopulate = true;
			pObject->On_BeforePopulatingObject();

			iColIdx = 0;
			for (i = 0; i < pMetaEntity->GetMetaColumnsInFetchOrder()->size(); i++)
			{
				pMetaCol = (*(pMetaEntity->GetMetaColumnsInFetchOrder()))[i];
				pCol = pObject->GetColumn(pMetaCol);

				// Derived columns must be at the end
				//
				if (pMetaCol->IsDerived())
					break;

				iColIdx++;

				// For lazy fetch columns we retrieve the NOT NULL indicator only
				if (bLazyFetch && pMetaCol->IsLazyFetch())
				{
					bool bHasBLOB = pRslts->getBoolean(iColIdx);

					if (!bHasBLOB)
					{
						pCol->SetNull();
						pCol->MarkFetched();
					}
					else
					{
						pCol->MarkUnfetched();
					}

					continue;
				}

				switch (pMetaCol->GetType())
				{
					case MetaColumn::dbfString:
						bResult = pCol->SetValue(pRslts->getString(iColIdx));
						break;

					case MetaColumn::dbfChar:
						bResult = pCol->SetValue((char) pRslts->getByte(iColIdx));
						break;

					case MetaColumn::dbfShort:
						bResult = pCol->SetValue((short) pRslts->getShort(iColIdx));
						break;

					case MetaColumn::dbfBool:
						bResult = pCol->SetValue((bool) pRslts->getBoolean(iColIdx));
						break;

					case MetaColumn::dbfInt:
						bResult = pCol->SetValue((int) pRslts->getInt(iColIdx));
						break;

					case MetaColumn::dbfLong:
						bResult = pCol->SetValue((long) pRslts->getLong(iColIdx));
						break;

					case MetaColumn::dbfFloat:
						bResult = pCol->SetValue((float) pRslts->getFloat(iColIdx));
						break;

					case MetaColumn::dbfDate:
					{
						odbc::Timestamp		odbcTimestamp;

						odbcTimestamp = pRslts->getTimestamp(iColIdx);

						if (!pRslts->wasNull())
						{
  						try
  						{
    						bResult = pCol->SetValue(D3Date(odbcTimestamp, m_pMetaDatabase->GetTimeZone()));
    					}
         			catch (...)
			       	{
  							pCol->MarkUnfetched();
								throw;
  						}
						}

						break;
					}
					case MetaColumn::dbfBlob:
					{
						try
						{
							assert(pCol->GetMetaColumn()->GetType() == MetaColumn::dbfBlob);
							((ColumnBlobPtr) pCol)->FromStream( *(pRslts->getBinaryStream(iColIdx)));
  						bResult = true;
  					}
         		catch (...)
			      {
 							pCol->MarkUnfetched();
							throw;
						}

						break;
					}
					case MetaColumn::dbfBinary:
					{
						odbc::Bytes	bytes;
						bytes = pRslts->getBytes(iColIdx);
						bResult = pCol->SetValue((const unsigned char*) bytes.getData(), bytes.getSize());
						break;
					}
				}

				if (!bResult)
				{
					pCol->MarkUnfetched();
					throw Exception(__FILE__, __LINE__, Exception_error, "Failed to populate column %s.", pCol->GetMetaColumn()->GetFullName().c_str());
				}
				else
				{
					if (pRslts->wasNull())
						pCol->SetNull();

					pCol->MarkFetched();
				}
			}
		}
		catch(...)
		{
 			if (bDoAfterPopulate)
 				pObject->On_AfterPopulatingObject();

			if (bDeletObject)
				delete pObject;

			throw;		// re-throw the exception
		}

		pObject->On_AfterPopulatingObject();


		return pObject;
	}





	int ODBCDatabase::ExecuteSQLCommand(const std::string & strSQL, bool bReadOnly)
	{
		std::auto_ptr<boost::recursive_mutex::scoped_lock>	lk(m_pMetaDatabase->m_TransactionManager.UseTransaction(this));

		ConnectionManager		conMgr(this);

		if (lk.get() == NULL && !bReadOnly)
			throw Exception(__FILE__, __LINE__, Exception_error, "ODBCDatabase::ExecuteSQLCommand(): Method invoked without a pending transaction.");

		int																			iRowCount;
		std::auto_ptr<odbc::PreparedStatement>	pPreparedStmnt;


		try
		{
			Reconnect();

			pPreparedStmnt.reset(conMgr.connection()->prepareStatement(strSQL));

			if (m_uTrace)
				ReportInfo("ODBCDatabase::ExecuteSQLCommand()..........................: Database " PRINTF_POINTER_MASK ". SQL: %s", this, strSQL.c_str());

			iRowCount = pPreparedStmnt->executeUpdate();

			// We must process all resultsets as otherwise the update may fail. Only the last
			// resultset contains the actual records deleted from the immediate target table.
			//
			while (pPreparedStmnt->getMoreResults())
			{
				iRowCount = pPreparedStmnt->getUpdateCount();
			}

			// odbc++ returns -1 if no rows where affected
			//
			if (iRowCount < 0)
				iRowCount = 0;
		}
		catch(odbc::SQLException& e)
		{
			CheckConnection(e);
			throw;
		}

		return iRowCount;
	}



	void  ODBCDatabase::ExecuteSingletonSQLCommand(const std::string & strSQL, unsigned long & lResult)
	{
		ConnectionManager		conMgr(this);

		try
		{
			Reconnect();

			std::auto_ptr<odbc::PreparedStatement> pPreparedStmnt(conMgr.connection()->prepareStatement(strSQL));

			if (m_uTrace)
				ReportInfo("ODBCDatabase::ExecuteSingletonSQLCommand().................: Database " PRINTF_POINTER_MASK ". SQL: %s", this, strSQL.c_str());

			std::auto_ptr<odbc::ResultSet> pRslts(pPreparedStmnt->executeQuery());

			if (pRslts.get() && pRslts->next())
			{
				lResult = pRslts->getLong(1);

				if (pRslts->wasNull())
					lResult = 0;
			}
		}
		catch(odbc::SQLException& e)
		{
			CheckConnection(e);
			throw;
		}
	}




	std::ostringstream & ODBCDatabase::ExecuteQueryAsJSON(const std::string & strSQL, std::ostringstream	& oResultSets)
	{
		ConnectionManager		conMgr(this);

		odbc::PreparedStatement*	pStmnt = NULL;
		odbc::ResultSet*					pRslts = NULL;
		bool											bFirst = true;

		try
		{
			Reconnect();

			pStmnt = conMgr.connection()->prepareStatement(strSQL);

			if (m_uTrace)
				ReportInfo("ODBCDatabase::ExecuteQueryAsJSON().........................: Database " PRINTF_POINTER_MASK ". SQL: %s", this, strSQL.c_str());

			pRslts = pStmnt->executeQuery();

			oResultSets << '[';

			while(true)
			{
				// filter messages (which are returned as NULL results)
				while (!pRslts && pStmnt->getMoreResults())
					pRslts = pStmnt->getResultSet();

				if (!pRslts)
					break;

				if (bFirst)
					bFirst = false;
				else
					oResultSets << ',';

				oResultSets << '[';

				WriteJSONToStream(oResultSets, pRslts);

				delete pRslts;
				pRslts = NULL;
				oResultSets << ']';
			}

			oResultSets << ']';
		}
		catch(odbc::SQLException& e)
		{
			CheckConnection(e);
			delete pRslts;
			delete pStmnt;
			throw;
		}
		catch(...)
		{
			delete pRslts;
			delete pStmnt;
			throw;
		}

		delete pRslts;
		delete pStmnt;

		return oResultSets;
	}




	std::ostringstream & ODBCDatabase::QueryToJSONWithMetaData(const std::string & strSQL, std::ostringstream	& ostrm)
	{
		ConnectionManager		conMgr(this);

		odbc::PreparedStatement*	pStmnt = NULL;
		odbc::ResultSet*					pRslts = NULL;
		bool											bFirst = true;

		try
		{
			Reconnect();

			pStmnt = conMgr.connection()->prepareStatement(strSQL);

			if (m_uTrace)
				ReportInfo("ODBCDatabase::QueryToJSONWithMetaData()....................: Database " PRINTF_POINTER_MASK ". SQL: %s", this, strSQL.c_str());

			pRslts = pStmnt->executeQuery();

			ostrm << '[';

			while(true)
			{
				// let's process meta data first
				bool	bFirst = true;

				// filter messages (which are returned as NULL results)
				while (!pRslts && pStmnt->getMoreResults())
					pRslts = pStmnt->getResultSet();

				if (!pRslts)
					break;

				if (bFirst)
					bFirst = false;
				else
					ostrm << ',';

				ostrm << '{';

				// deal with meta data
				odbc::ResultSetMetaData*	pRsltMD = pRslts->getMetaData();

				ostrm << "\"MetaData\":[";

				bFirst = true;

				if (pRsltMD->getColumnCount() > 0)
				{
					for (SQLUSMALLINT i = 1; i <= pRsltMD->getColumnCount(); i++)
					{
						if (bFirst)
							bFirst = false;
						else
							ostrm << ',';

						ostrm << '{';

						ostrm << "\"CatalogName\":";
						ostrm << '"' << pRsltMD->getCatalogName(i) << '"' << ',';

						ostrm << "\"ColumnDisplaySize\":";
						ostrm << pRsltMD->getColumnDisplaySize(i) << ',';

						ostrm << "\"ColumnLabel\":";
						ostrm << '"' << pRsltMD->getColumnLabel(i) << '"' << ',';

						ostrm << "\"ColumnName\":";
						ostrm << '"' << pRsltMD->getColumnName(i) << '"' << ',';

						ostrm << "\"BaseTableName\":";
						ostrm << '"' << pRsltMD->getBaseTableName(i) << '"' << ',';

						ostrm << "\"BaseColumnName\":";
						ostrm << '"' << pRsltMD->getBaseColumnName(i) << '"' << ',';

						ostrm << "\"ColumnType\":";
						ostrm << pRsltMD->getColumnType(i) << ',';

						ostrm << "\"ColumnTypeName\":";
						ostrm << '"' << pRsltMD->getColumnTypeName(i) << '"' << ',';

						ostrm << "\"Precision\":";
						ostrm << pRsltMD->getPrecision(i) << ',';

						ostrm << "\"Scale\":";
						ostrm << pRsltMD->getScale(i) << ',';

						ostrm << "\"SchemaName\":";
						ostrm << '"' << pRsltMD->getSchemaName(i) << '"' << ',';

						ostrm << "\"TableName\":";
						ostrm << '"' << pRsltMD->getTableName(i) << '"';

						ostrm << '}';
					}
				}

				ostrm << "],\"Data\":[";
				bFirst = true;

				while (pRslts->next())
				{
					int													idx;
					odbc::ResultSetMetaData*		pMD = pRslts->getMetaData();
					std::ostringstream					ovalue;
					bool												bFirstCol = true;

					if (bFirst)
						bFirst = false;
					else
						ostrm << ',';

					ostrm << '[';

					for (idx = 0; idx < pMD->getColumnCount(); idx++)
					{
						if (bFirstCol)
							bFirstCol = false;
						else
							ostrm << ',';

						switch (pMD->getColumnType(idx+1))
						{
							/** An SQL BIT */
							case odbc::Types::BIT:
								ovalue << (pRslts->getBoolean(idx+1) ? "true" : "false");
								break;

							/** An SQL VARCHAR (variable length less than 4K) */
							case odbc::Types::VARCHAR:
								ovalue << '"' << JSONEncode(pRslts->getString(idx+1)) << '"';
								break;

							/** An SQL TINYINT */
							case odbc::Types::TINYINT:
							/** An SQL INTEGER */
							case odbc::Types::INTEGER:
							/** An SQL SMALLINT */
							case odbc::Types::SMALLINT:
								ovalue << pRslts->getInt(idx+1);
								break;

							/** An SQL TIMESTAMP */
							case odbc::Types::TIMESTAMP:
							{
								D3Date			dt(pRslts->getTimestamp(idx+1), m_pMetaDatabase->GetTimeZone());
								ovalue << '"' << dt.AsUTCISOString(3) << '"';
								break;
							}

							/** An SQL DATE */
							case odbc::Types::DATE:
								ovalue << '"' << pRslts->getDate(idx+1).toString() << '"';
								break;

							/** An SQL FLOAT */
							case odbc::Types::REAL:
							/** An SQL FLOAT */
							case odbc::Types::FLOAT:
								ovalue << pRslts->getFloat(idx+1);
								break;

							default:
								std::cout << "Column " << pMD->getColumnType(idx+1) << " is of type " << pMD->getColumnTypeName(idx+1) << " which is not currently handled!\n";
						}

						if (pRslts->wasNull())
							ostrm << "null";
						else
							ostrm << ovalue.str();

						ovalue.str("");
					}

					ostrm << ']';
				}

				delete pRslts;
				pRslts = NULL;
				ostrm << "]}";
			}

			ostrm << ']';
		}
		catch(odbc::SQLException& e)
		{
			CheckConnection(e);
			delete pRslts;
			delete pStmnt;
			throw;
		}
		catch(...)
		{
			delete pRslts;
			delete pStmnt;
			throw;
		}

		delete pRslts;
		delete pStmnt;

		return ostrm;
	}




	bool ODBCDatabase::UpdateObject(EntityPtr pObj)
	{
		MONITORFUNC("ODBCDatabase::UpdateObject()", this);

		std::auto_ptr<boost::recursive_mutex::scoped_lock>	lk(m_pMetaDatabase->m_TransactionManager.UseTransaction(this));

		if (lk.get() == NULL)
			throw Exception(__FILE__, __LINE__, Exception_error, "ODBCDatabase::UpdateObject(): Method invoked without a pending transaction.");

		std::string								strSQL, strTemp, strWHERE;
		bool											bFirst = true;
		unsigned int							idx;
		bool											bDelete = false, bHasBlob = false;
		Entity::UpdateType				iUpdateType;
		ColumnPtrListItr					itrKeyCol;
		ColumnPtr									pCol, pColIDENTITY = NULL;
		InstanceKeyPtr						pPrimaryKey;
		int												iRowCount = 0;


		// Update
		try
		{
			if (!pObj)
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
					for (idx = 0; idx < pObj->GetColumnCount(); idx++)
					{
						pCol = pObj->GetColumn(idx);

						if (pCol->GetMetaColumn()->IsDerived())
							break;

						if (!pCol->GetMetaColumn()->IsAutoNum())
						{
							// Parameterize blobs
							if (pCol->GetMetaColumn()->GetType() == MetaColumn::dbfBlob)
							{
								if(!bFirst)
								{
									strSQL  += ',';
									strTemp += ',';
								}

								if (pCol->IsNull())
								{
									strTemp += "NULL";
								}
								else
								{
									strTemp += '?';
									bHasBlob = true;
								}

								strSQL  += pCol->GetMetaColumn()->GetName();
								bFirst = false;
							}
							else
							{
								if(!bFirst)
								{
									strSQL  += ',';
									strTemp += ',';
								}

								strTemp += pCol->AsSQLString();
								strSQL  += pCol->GetMetaColumn()->GetName();
								bFirst = false;
							}
						}
					}

					strSQL += ") VALUES (";
					strSQL += strTemp;
					strSQL += ");";

					// Also get new value for IDENTITY column if there is one
					pPrimaryKey = pObj->GetPrimaryKey();

					if (pPrimaryKey->GetMetaKey()->IsAutoNum())
					{
						// The first column in the primary key MUST be the AutoNum column
						pColIDENTITY = pPrimaryKey->GetColumns().front();

						switch (GetMetaDatabase()->GetTargetRDBMS())
						{
							case SQLServer:
							{
								strSQL += "SELECT SCOPE_IDENTITY();";
								break;
							}
							case Oracle:
							{
								strSQL +=  "SELECT seq_";
								strSQL += pCol->GetEntity()->GetMetaEntity()->GetName().c_str();
								strSQL += "_";
								strSQL += pCol->GetMetaColumn()->GetName().c_str();
								strSQL += ".CURRVAL FROM DUAL;";
								break;
							}
						}
					}
					break;
				}

				case Entity::SQL_Update:
				{
					strSQL  = "UPDATE ";
					strSQL += pObj->GetMetaEntity()->GetName();
					strSQL += " ";

					bFirst = true;
					for (idx = 0; idx < pObj->GetColumnCount(); idx++)
					{
						pCol = pObj->GetColumn(idx);

						if (pCol->GetMetaColumn()->IsDerived())
							break;

						if (pCol->IsDirty())
						{
							assert(!pCol->GetMetaColumn()->IsAutoNum());

							if (pCol->GetMetaColumn()->IsMandatory() && pCol->IsNull())
								throw Exception(__FILE__, __LINE__, Exception_error, "ODBCDatabase::Update(): Can't update column %s with NULL value.", pCol->GetMetaColumn()->GetFullName().c_str());

							if (bFirst)
							{
								bFirst = false;
								strSQL += "SET ";
							}
							else
							{
								strSQL += ',';
							}

							strSQL += pCol->GetMetaColumn()->GetName();
							strSQL += '=';

							// blob type has different handling
							if(pCol->GetMetaColumn()->GetType() == MetaColumn::dbfBlob)
							{
								if (pCol->IsNull())
								{
									strSQL += "NULL";
								}
								else
								{
									strSQL += '?';
									bHasBlob = true;
								}
							}
							else
							{
								strSQL += pCol->AsSQLString();
							}
						}
					}

					// If we have not changed any columns, return success
					//
					if (bFirst)
						return true;

					strSQL += " WHERE ";
					strSQL += strWHERE;
					break;
				}

				default:			// No work if default type is none of the above
					return true;
			}

			// Reconnect if necessary
			Reconnect();

			if (m_uTrace & (D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT))
				ReportInfo("ODBCDatabase::UpdateObject()........: Database " PRINTF_POINTER_MASK ". SQL: %s", this, strSQL.c_str());

			// Do the actual update
			//
			std::auto_ptr<odbc::PreparedStatement>	pPreparedStmnt(m_pConnection->prepareStatement(strSQL, odbc::ResultSet::TYPE_FORWARD_ONLY, odbc::ResultSet::CONCUR_READ_ONLY));
			std::auto_ptr<odbc::ResultSet>					pRslts;
			long																		lNewID;

			if (bHasBlob)
			{
				int idxBlob=0;

				// let's find all columns which are blobs, not null and (if the update type is UPDATE) dirty
				for (idx = 0; idx < pObj->GetColumnCount(); idx++)
				{
					pCol = pObj->GetColumn(idx);

					// Don't bother with derived columns
					if (pCol->GetMetaColumn()->IsDerived())
						break;

					// Is it a non-null blob?
					if (pCol->GetMetaColumn()->GetType() == MetaColumn::dbfBlob && !pCol->IsNull())
					{
						if (iUpdateType != Entity::SQL_Update || pCol->IsDirty())
						{
							ColumnBlobPtr									pBlob = (ColumnBlobPtr) pCol;
							std::stringstream::pos_type		pos = pBlob->m_Stream.tellg();
							pPreparedStmnt->setBinaryStream(++idxBlob, (std::istream*) &(pBlob->m_Stream), pBlob->m_Stream.str().length());
							pBlob->m_Stream.clear();
							pBlob->m_Stream.seekg(pos);
						}
					}
				}
			}

			pPreparedStmnt->executeQuery();

			// We must process all resultsets as otherwise the update may fail. Only the last
			// resultset contains the actual records deleted from the immediate target table.
			//
			while (true)
			{
				pRslts.reset(pPreparedStmnt->getResultSet());

				// Typically, update statements produce NULL result sets from which we get the update count. If
				// there are mutliple updates (via triggers for example), the last NULL result set returned will
				// contain the rows affected by the original query.
				// INSERT statements on tables with an IDENTITY column will also fetch the new IDENTITY value
				// which will be returned as a none-NULL result set
				if (!pRslts.get())
					iRowCount = pPreparedStmnt->getUpdateCount();
				else
				{
					pRslts->next();
					lNewID = pRslts->getLong(1);
				}

				if (!pPreparedStmnt->getMoreResults())
					break;
			}

			if (iRowCount != 1)
				ReportWarning("ODBCDatabase::UpdateObject(): The SQL %s affected %i rows but method expected 1 row.", strSQL.c_str(), iRowCount);

			switch (iUpdateType)
			{
				case Entity::SQL_Delete:
					delete pObj;
					break;

				case Entity::SQL_Insert:
					if (pColIDENTITY)
					{
						pPrimaryKey->On_BeforeUpdate();
						pColIDENTITY->SetValue(lNewID);
						pPrimaryKey->On_AfterUpdate();
					}

					pObj->MarkClean();
					break;

				case Entity::SQL_Update:
					pObj->MarkClean();
					break;
			}
		}
		catch(odbc:: SQLException& e)
		{
			CheckConnection(e);
			throw Exception(__FILE__, __LINE__, Exception_error, "ODBCDatabase::UpdateObject(): ODBC error occurred executing statement %s. %s", (void*) strSQL.c_str(), (void*) e.getMessage().c_str());
		}
		catch(Exception &)
		{
			throw;
		}


		return true;
	}



	/* static */
	bool ODBCDatabase::CreatePhysicalDatabase(MetaDatabasePtr pMD)
	{
		// This must be initialised
		//
		if (!pMD->GetInitialised())
		{
			ReportError("ODBCDatabase::CreateSQLServerDatabase(): Method called on an uninitialised MetaDatabase object.");
			return false;
		}

		// This must be initialised
		//
		if (!pMD->AllowCreate())
		{
			ReportWarning("ODBCDatabase::CreateSQLServerDatabase(): Method called on an MetaDatabase object with AllowCreate==false. Request ignored.");
			return true;
		}

		switch (pMD->GetTargetRDBMS())
		{
			case SQLServer:
				return CreateSQLServerDatabase(pMD);

			case Oracle:
				return CreateOracleDatabase(pMD);
		}

		return false;
	}



	// Create a SQLServer database
	//
	/* static */
	bool ODBCDatabase::CreateSQLServerDatabase(MetaDatabasePtr pMD)
	{
		std::auto_ptr<odbc::Connection>	pCon;
		std::auto_ptr<odbc::Statement>	pStmt;
		MetaEntityPtr			pD3VI,pME;
		std::string				strMasterConnection;
		std::string				strSQL;
		char							szTemp[32];
		unsigned int			idx;


		// Build the master connection string (In SQL Server, we connect to thre master database do DROP DATABASE etc)
		//
		strMasterConnection  = "DRIVER=";
		strMasterConnection += pMD->GetDriver();
		strMasterConnection += ";SERVER=";
		strMasterConnection += pMD->GetServer();
		strMasterConnection += ";DATABASE=master";
		if (D3::ToUpper(pMD->GetUserID()) == "WINDOWS")
		{
			strMasterConnection += ";TRUSTED_CONNECTION=YES";
		}
		else
		{
			strMasterConnection += ";UID=";
			strMasterConnection += pMD->GetUserID();
			strMasterConnection += ";PWD=";
			strMasterConnection += APALUtil::decryptPassword(pMD->GetUserPWD());
		}

		// ============== Drop existing database

		// Connect using master connection
		//
		pCon.reset(odbc::DriverManager::getConnection(strMasterConnection));
		pStmt.reset(pCon->createStatement());

		// Drop the existing database
		//
		try
		{
			strSQL = std::string("DROP DATABASE ") + pMD->GetName();
			pStmt->executeUpdate(strSQL);
		}
		catch(odbc::SQLException& e)
		{
			switch (e.getErrorCode())
			{
				case 3701:	// Database does not exist
					break;

				case 3702:	// Database in use
					throw Exception(0, 0, Exception_error, "ODBCDatabase::CreateSQLServerDatabase(): %s failed because database is currently in use. %s", strSQL.c_str(), e.getMessage().c_str());

				default:
					throw Exception(0, 0, Exception_error, "ODBCDatabase::CreateSQLServerDatabase(): %s failed. %s", strSQL.c_str(), e.getMessage().c_str());
			}
		}

		// ============== Create the new database

		// Create the actual database
		//
		strSQL  = "CREATE DATABASE ";
		strSQL += pMD->GetName();
		strSQL += " ON ( NAME = '";
		strSQL += pMD->GetName();
		strSQL += "_dat', FILENAME = '";
		strSQL += pMD->GetDataFilePath();
		strSQL += "', SIZE = ";
		strSQL += ltoa(pMD->GetInitialSize(), szTemp, 10);
		strSQL += "MB, MAXSIZE = ";
		strSQL += ltoa(pMD->GetMaxSize(), szTemp, 10);
		strSQL += "MB, FILEGROWTH = ";
		strSQL += ltoa(pMD->GetGrowthSize(), szTemp, 10);
		strSQL += "MB )";
		strSQL += " LOG ON ( NAME = '";
		strSQL += pMD->GetName();
		strSQL += "_log', FILENAME = '";
		strSQL += pMD->GetLogFilePath();
		strSQL += "', SIZE = ";
		strSQL += ltoa(pMD->GetInitialSize(), szTemp, 10);
		strSQL += "MB, MAXSIZE = ";
		strSQL += ltoa(pMD->GetMaxSize(), szTemp, 10);
		strSQL += "MB, FILEGROWTH = ";
		strSQL += ltoa(pMD->GetGrowthSize(), szTemp, 10);
		strSQL += "MB )";

		pStmt->executeUpdate(strSQL);

		// Now connect to the database we created above
		pStmt.reset();
		pCon.reset(odbc::DriverManager::getConnection(pMD->GetConnectionString()));
		pStmt.reset(pCon->createStatement());

		// Create each table
		for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
		{
			pME = pMD->GetMetaEntity(idx);
			pStmt->executeUpdate(pME->AsCreateTableSQL(SQLServer));
		}

		// Create all indexes
		//
		for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
		{
			pME = pMD->GetMetaEntity(idx);

			for (unsigned int idx2 = 0; idx2 < pME->GetMetaKeyCount(); idx2++)
			{
				strSQL = pME->GetMetaKey(idx2)->AsCreateIndexSQL(SQLServer);

				if (!strSQL.empty())
					pStmt->executeUpdate(strSQL);
			}
		}

		// Create all triggers
		//
		for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
		{
			pME = pMD->GetMetaEntity(idx);
			CreateTriggers(SQLServer, pCon.get(), pME);
		}

		// If this database has a D3 version info table, insert a new row
		//
		pD3VI = pMD->GetVersionInfoMetaEntity();

		if (pD3VI)
		{
			char							szTemp[32];
			odbc::Timestamp		odbcTimestamp;		// ctor makes this like Now()


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
			strSQL += ", '";
			strSQL += odbcTimestamp.toString();
			strSQL += "')";

			pStmt->executeUpdate(strSQL);
		}

		return true;
	}



	// Create a SQLServer database
	//
	bool ODBCDatabase::CreateOracleDatabase(MetaDatabasePtr pMD)
	{
		MetaEntityPtr			pD3VI, pME;

		std::auto_ptr<odbc::Connection>		pCon;
		std::auto_ptr<odbc::Statement>		pStmt;

		std::string				strSQL;
		char							szTemp[32];
		unsigned int			idx;

		// ============== Delete existing database

		// Connect to the driver
		pCon.reset(odbc::DriverManager::getConnection(pMD->GetConnectionString()));
		pStmt.reset(pCon->createStatement());

		// Drop the tablespace
		//
		strSQL  = "DROP TABLESPACE ";
		strSQL += pMD->GetName();
		strSQL += " INCLUDING CONTENTS AND DATAFILES CASCADE CONSTRAINTS";

		pStmt->executeUpdate(strSQL);

		// Drop all Sequences
		for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
		{
			pME = pMD->GetMetaEntity(idx);

 			strSQL = pME->AsDropSequenceSQL(Oracle);

 			if (!strSQL.empty())
 				pStmt->executeUpdate(strSQL);
		}

		// Remove physical file
		remove(pMD->GetDataFilePath().c_str());

		ReportInfo("ODBCDatabase::CreateOracleDatabase(): Existing %s database deleted successfully.", pMD->GetName().c_str());

		// ============== Create new database

		// Connect to the driver
		pStmt.reset();
		pCon.reset(odbc::DriverManager::getConnection(pMD->GetConnectionString()));
		pStmt.reset(pCon->createStatement());

		// Create the physical database
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

		pStmt->executeUpdate(strSQL);

		// Create Package for CascadeUpdate Triggers
		CreateUpdateCascadePackage(pCon.get());

		// Create each table
		for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
		{
			pME = pMD->GetMetaEntity(idx);
			pStmt->executeUpdate(pME->AsCreateTableSQL(Oracle));
		}

		// Create all Sequences
		for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
		{
			pME = pMD->GetMetaEntity(idx);

			strSQL = pME->AsCreateSequenceSQL(Oracle);

			if (!strSQL.empty())
				pStmt->executeUpdate(strSQL);
		}

		// Create all Triggers for Sequences
		for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
		{
			pME = pMD->GetMetaEntity(idx);

			strSQL = pME->AsCreateSequenceTriggerSQL(Oracle);

			if (!strSQL.empty())
				pStmt->executeUpdate(strSQL);
		}

		// Create all indexes
		for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
		{
			pME = pMD->GetMetaEntity(idx);

			for (unsigned int idx2 = 0; idx2 < pME->GetMetaKeyCount(); idx2++)
			{
				strSQL = pME->GetMetaKey(idx2)->AsCreateIndexSQL(Oracle);

				if (!strSQL.empty())
					pStmt->executeUpdate(strSQL);
			}
		}

		// Create all triggers
		for (idx = 0; idx < pMD->GetMetaEntities()->size(); idx++)
		{
			pME = pMD->GetMetaEntity(idx);
			CreateTriggers(Oracle, pCon.get(), pME);
		}

		// If this database has a D3 version info table, insert a new row
		pD3VI = pMD->GetVersionInfoMetaEntity();

		if (pD3VI)
		{
			char							szTemp[32];
			odbc::Timestamp		odbcTimestamp;		// ctor makes this like Now()
			std::string				strTimeStamp;

			// The timestamp is a pain in the arse for oracle, this works
			strTimeStamp  = "TO_DATE('";
			strTimeStamp += odbcTimestamp.toString();
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

			pStmt->executeUpdate(strSQL);
		}

		return true;
	}



	void ODBCDatabase::CreateTriggers(TargetRDBMS eTargetDB, odbc::Connection* pCon, MetaEntityPtr pMetaEntity)
	{
		std::auto_ptr<odbc::Statement>	pStmt;
		std::string				strSQL;
		unsigned int			idx;

		// Create CascadeUpdate/Delete Packages for Child
		//
		for (idx = 0; idx < pMetaEntity->GetChildMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetChildMetaRelation(idx)->AsCreateCascadePackage(eTargetDB);

			if (!strSQL.empty())
			{
				pStmt.reset(pCon->createStatement());
				pStmt->executeUpdate(strSQL);
			}
		}

		// Create CascadeUpdate/Delete Packages for Parents
		//
		for (idx = 0; idx < pMetaEntity->GetParentMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetParentMetaRelation(idx)->AsCreateCascadePackage(eTargetDB);

			if (!strSQL.empty())
			{
				pStmt.reset(pCon->createStatement());
				pStmt->executeUpdate(strSQL);
			}
		}

		// Create CheckInsertUpdate triggers
		//
		for (idx = 0; idx < pMetaEntity->GetParentMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetParentMetaRelation(idx)->AsCreateCheckInsertUpdateTrigger(eTargetDB);

			if (!strSQL.empty())
			{
				pStmt.reset(pCon->createStatement());
				pStmt->executeUpdate(strSQL);
			}
		}

		// Create CascadeDelete triggers
		//
		for (idx = 0; idx < pMetaEntity->GetChildMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetChildMetaRelation(idx)->AsCreateCascadeDeleteTrigger(eTargetDB);

			if (!strSQL.empty())
			{
				pStmt.reset(pCon->createStatement());
				pStmt->executeUpdate(strSQL);
			}
		}

		// Create CascadeClearRef triggers
		//
		for (idx = 0; idx < pMetaEntity->GetChildMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetChildMetaRelation(idx)->AsCreateCascadeClearRefTrigger(eTargetDB);

			if (!strSQL.empty())
			{
				pStmt.reset(pCon->createStatement());
				pStmt->executeUpdate(strSQL);
			}
		}

		// Create VerifyDelete triggers
		//
		for (idx = 0; idx < pMetaEntity->GetChildMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetChildMetaRelation(idx)->AsCreateVerifyDeleteTrigger(eTargetDB);

			if (!strSQL.empty())
			{
				pStmt.reset(pCon->createStatement());
				pStmt->executeUpdate(strSQL);
			}
		}

		// Create UpdateForeignRef triggers
		//
		for (idx = 0; idx < pMetaEntity->GetChildMetaRelationCount(); idx++)
		{
			strSQL = pMetaEntity->GetChildMetaRelation(idx)->AsUpdateForeignRefTrigger(eTargetDB);

			if (!strSQL.empty())
			{
				pStmt.reset(pCon->createStatement());
				pStmt->executeUpdate(strSQL);
			}
		}

		// Create custom insert triggers
		//
		strSQL = pMetaEntity->GetCustomInsertTrigger(eTargetDB);

		if (!strSQL.empty())
		{
			pStmt.reset(pCon->createStatement());
			pStmt->executeUpdate(strSQL);
		}

		// Create custom update triggers
		//
		strSQL = pMetaEntity->GetCustomUpdateTrigger(eTargetDB);

		if (!strSQL.empty())
		{
			pStmt.reset(pCon->createStatement());
			pStmt->executeUpdate(strSQL);
		}

		// Create custom delete triggers
		//
		strSQL = pMetaEntity->GetCustomDeleteTrigger(eTargetDB);

		if (!strSQL.empty())
		{
			pStmt.reset(pCon->createStatement());
			pStmt->executeUpdate(strSQL);
		}
	}



	void ODBCDatabase::CreateUpdateCascadePackage(odbc::Connection* pCon)
	{
		std::string				strSQL;
		std::auto_ptr<odbc::Statement>	pStmt;

		strSQL  = "create or replace package update_cascade\n";
		strSQL += "as\n";
		strSQL += "procedure on_table( p_table_name      in varchar2, ";
		strSQL += "p_preserve_rowid  in boolean default TRUE, ";
		strSQL += "p_use_dbms_output in boolean default FALSE );\n";
		strSQL += "end update_cascade;";

		pStmt.reset(pCon->createStatement());
		pStmt->executeUpdate(strSQL);


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

		pStmt.reset(pCon->createStatement());
		pStmt->executeUpdate(strSQL);
	}



	/* static */
	void ODBCDatabase::UnInitialise()
	{
		try
		{
			DeleteConnectionPools();
			odbc::DriverManager::shutdown();
		}
		catch(odbc::SQLException& e)
		{
			ReportError("odbc::DriverManager::shutdown() error: %s", e.getMessage().c_str());
		}
		catch(...)
		{
			ReportError("odbc::DriverManager::shutdown() unknown error occurred");
		}
	}


	// Backup all records of the entities listed in pListME to the specified XML file. The method
	// returns the number of records written to the XML file or -1 if an error occurred..
	//
	long ODBCDatabase::ExportToXML(const std::string & strAppName, const std::string & strXMLFileName, MetaEntityPtrListPtr pListME, const std::string & strD3MDDBIDFilter)
	{
		MetaEntityPtr								pMetaEntity;
		MetaEntityPtrList						listMEOrig, listME;
		MetaEntityPtrListItr				itrTrgt, itrSrce;
		MetaColumnPtrListItr				itrKeyMC;
		MetaColumnPtrVect::iterator	itrMEC;
		MetaColumnPtr								pMC;
		std::ofstream								fxml;
		int													idx;
		long												lRecCountTotal=0, lRecCountCurrent;


		try
		{
			std::cout << "Starting to export data from " << m_pMetaDatabase->GetAlias() << ' ' << m_pMetaDatabase->GetVersion() << std::endl;

			// Make sure we have a connection
			Reconnect();

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
					std::string									strSQL;


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
					std::auto_ptr<odbc::Statement> pStmnt(m_pConnection->createStatement(odbc::ResultSet::TYPE_SCROLL_INSENSITIVE, odbc::ResultSet::CONCUR_READ_ONLY));
					pStmnt->setFetchSize(LIBODBC_FETCH_SIZE);
					std::auto_ptr<odbc::ResultSet> pRslts(pStmnt->executeQuery(strSQL));

					if (pRslts->first())
					{
						while (!pRslts->isAfterLast())
						{
							std::string								strValue;

							lRecCountCurrent++;

							// Write Entity header
							//
							fxml << "\t\t\t\t<Entity>\n";

							for ( itrMEC =  pMetaEntity->GetMetaColumnsInFetchOrder()->begin(), idx=0;
										itrMEC != pMetaEntity->GetMetaColumnsInFetchOrder()->end();
										itrMEC++, idx++)
							{
								pMC = *itrMEC;

								// Derived columns must be at the end
								//
								if (pMC->IsDerived())
									break;

								// Write Column header
								//
								fxml << "\t\t\t\t\t<" << pMC->GetName() << " NULL=\"";

								if (pMC->IsStreamed())
								{
									std::istream*				pistrm = pRslts->getBinaryStream(idx+1);

									if (!pRslts->wasNull())
									{
										std::ostringstream	ostrm;
										char								buffer[D3_STREAMBUFFER_SIZE];
										unsigned int				nRead = D3_STREAMBUFFER_SIZE;

										while (nRead == D3_STREAMBUFFER_SIZE)
										{
											pistrm->read(buffer, D3_STREAMBUFFER_SIZE);
											nRead = pistrm->gcount();

											if (nRead)
												ostrm.write(buffer, nRead);
										}

										if (pMC->IsEncodedValue())
											strValue = APALUtil::base64_encode(ostrm.str());
										else
											strValue = XMLEncode(ostrm.str());
									}
								}
								else
								{
									switch (pMC->GetType())
									{
										case MetaColumn::dbfChar:
										case MetaColumn::dbfShort:
										case MetaColumn::dbfBool:
										case MetaColumn::dbfInt:
										case MetaColumn::dbfLong:
										case MetaColumn::dbfFloat:
											strValue = pRslts->getString(idx+1);
											break;

										case MetaColumn::dbfDate:
										{
											try
											{
												strValue = D3Date(pRslts->getTimestamp(idx+1), m_pMetaDatabase->GetTimeZone()).AsISOString();
											}
											catch(...)
											{
												if(!pMC->IsPrimaryKeyMember() && pMC->IsDefaultValueNull())
												{
													strValue = "";
												}
											}
											break;
										}

										case MetaColumn::dbfString:
											strValue = pRslts->getString(idx+1);

											if (!pRslts->wasNull())
											{
												if (pMC->IsEncodedValue())
													strValue = APALUtil::base64_encode(strValue);
												else
													strValue = XMLEncode(pRslts->getString(idx+1));
											}

											break;

										case MetaColumn::dbfBinary:
										{
											odbc::Bytes		bytes = pRslts->getBytes(idx+1);

											if (!pRslts->wasNull())
												APALUtil::base64_encode(strValue, (const unsigned char*) bytes.getData(), bytes.getSize());

											break;
										}
									}
								}

								if (pRslts->wasNull() || (pMC->GetType() == MetaColumn::dbfString) && strValue.empty() || (pMC->GetType() == MetaColumn::dbfDate) && strValue.empty())
									fxml << "1\"></" << pMC->GetName() << ">\n";
								else
									fxml << "0\">" << strValue << "</" << pMC->GetName() << ">\n";
							}

							fxml << "\t\t\t\t</Entity>\n";

							// Report every 100th record written
							if ((lRecCountCurrent % 100) == 0)
								std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent;

							pRslts->next();
						}
					}

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
			// </D3Test>
			//
			fxml << "\t\t</Database>\n";
			fxml << "\t</DatabaseList>\n";
			fxml << "</APALDBData>\n";
			fxml.close();

			std::cout << "Finished to export data from " << m_pMetaDatabase->GetName() << std::endl;
			std::cout << "Total Records exported: " << lRecCountTotal << std::endl;
		}
		catch(odbc::SQLException& e)
		{
			CheckConnection(e);
			std::cout << "ODBC Exception caught: " << e.getMessage() << std::endl;
			return -1;
		}

		return lRecCountTotal;
	}



	// Restore the entities listed in pListME and present in the XML file (or all found in the XML
	// file for this database is pListME is NULL).
	//
	// The method returns the number of records restored.
	//
	long ODBCDatabase::ImportFromXML(const std::string & strAppName, const std::string & strXMLFileName, MetaEntityPtrListPtr pListME)
	{
		CSAXParser									xmlParser;
		MetaEntityPtrList						listMEOrig, listME;
		MetaEntityPtrListItr				itrTrgt, itrSrce;
		ODBCXMLImportFileProcessor	xmlProcessor(strAppName, this, &listME);


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

		return xmlProcessor.GetTotalRecordCount();
	}



 	void ODBCDatabase::BeforeImportData(MetaEntityPtr pMetaEntity)
  {
		std::auto_ptr<odbc::Statement>	pStmnt;
		MetaColumnPtr						pAutoNumMC;
		std::string							strSQL;


		try
		{
			Reconnect();

			// Get a statement to work with
			//
  		pStmnt.reset(m_pConnection->createStatement());

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
  			pStmnt->execute(strSQL);

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
  			pStmnt->execute(strSQL);

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
   				pStmnt->execute(strSQL);
     	}
		}
		catch(odbc::SQLException&e)
		{
			CheckConnection(e);
			throw;
		}
	}



  void ODBCDatabase::AfterImportData(MetaEntityPtr pMetaEntity, unsigned long lMaxKeyVal)
  {
		std::auto_ptr<odbc::Statement>			pStmnt;
		std::auto_ptr<odbc::ResultSet>			pRslts;
		MetaColumnPtr						pAutoNumMC;
		std::string							strSQL;


		try
		{
			Reconnect();

			// Create statement
			pStmnt.reset(m_pConnection->createStatement());

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
 				pStmnt->execute(strSQL);

			// Enable all constraints
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
 				pStmnt->execute(strSQL);

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

    				pStmnt->execute(strSQL);
						break;

					case Oracle:
					{
						if (lMaxKeyVal == 0)
							break;

						std::string			strSequenceName;
						std::string			strTemp;
						unsigned long		lNxtVal;
						long						lStepVal;


						// We need to reset the Sequence such that the next time it is called it returns
						// lMaxKeyVal+1
						strSequenceName  = "seq_";
						strSequenceName += pMetaEntity->GetName();
						strSequenceName += "_";
						strSequenceName += pAutoNumMC->GetName();

						// Get the next value from the sequence
						strSQL  = "SELECT ";
						strSQL += strSequenceName;
						strSQL += ".NEXTVAL FROM DUAL";

						pStmnt.reset(m_pConnection->createStatement(odbc::ResultSet::TYPE_SCROLL_INSENSITIVE, odbc::ResultSet::CONCUR_READ_ONLY));
						pRslts.reset(pStmnt->executeQuery(strSQL));

						if (!pRslts->first())
							throw Exception(__FILE__, __LINE__, Exception_error, "ODBCDatabase::AfterImportData(): Failed to reset sequence %s.", strSequenceName.c_str());

						lNxtVal = pRslts->getLong(1);

						// Calculate necessary adjustment
					  lStepVal = lMaxKeyVal - lNxtVal;

					  if (lStepVal != 0)
					  {
        			pStmnt.reset(m_pConnection->createStatement());

  					  // Let's adjust the increment
  					  //
							Convert(strTemp, lStepVal);

              strSQL  = "ALTER SEQUENCE ";
              strSQL += strSequenceName;
              strSQL += " INCREMENT BY ";
              strSQL += strTemp;

        			pStmnt->execute(strSQL);

              // Get the next value from the sequence (we don't need details)
              //
              strSQL  = "SELECT ";
              strSQL += strSequenceName;
              strSQL += ".NEXTVAL FROM DUAL";

        			pStmnt->execute(strSQL);

  					  // Let's reset the increment
  					  //
              strSQL  = "ALTER SEQUENCE ";
              strSQL += strSequenceName;
              strSQL += " INCREMENT BY 1";

        			pStmnt->execute(strSQL);
            }

            break;
       		}
 	 			}
	    }
		}
		catch (odbc::SQLException& e)
		{
			CheckConnection(e);
			throw;
		}
	}





	EntityPtrListPtr ODBCDatabase::CallPagedObjectSP( ResultSetPtr	pRS,
																										MetaEntityPtr	pMetaEntity,
																										unsigned long uPageNo,
																										unsigned long uPageSize,
																										unsigned long & lStart,
																										unsigned long & lEnd,
																										unsigned long & lTotal)
	{
		std::auto_ptr<odbc::Statement>	pStmnt;
		std::auto_ptr<odbc::ResultSet>	pRslts;
		ODBCDatabasePtr						pDB;
		EntityPtr									pObject;
		EntityPtrListPtr					pEL = NULL;
		bool											bSuccess = false;
		std::ostringstream				osql;


		if (!pMetaEntity)
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "ODBCDatabase::CallPagedObjectSP(): Received request to load data from unknown table.");
			return NULL;
		}

		try
		{
			assert(pRS->IsKindOf(SQLServerPagedResultSet::ClassObject()));
			SQLServerPagedResultSet* pSPRS = (SQLServerPagedResultSet*) pRS;
			assert(pSPRS->GetStoredProcedureDatabase()->IsKindOf(ODBCDatabase::ClassObject()));

			// We use the resultsets connection
			pDB = (ODBCDatabase*) pSPRS->GetStoredProcedureDatabase();

			pDB->Reconnect();

			osql << "{call " << pSPRS->GetStoredProcedureName() << '(' << uPageNo << ',' << uPageSize << ")}";

			if (m_uTrace != D3DB_TRACE_NONE)
				ReportInfo("ODBCDatabase::CallPagedObjectSP............................: Database " PRINTF_POINTER_MASK ". SQL: %s", this, osql.str().c_str());

			ConnectionManager		conMgr(pDB);
			pStmnt.reset(conMgr.connection()->createStatement(odbc::ResultSet::TYPE_FORWARD_ONLY, odbc::ResultSet::CONCUR_READ_ONLY));
			pStmnt->setFetchSize(LIBODBC_FETCH_SIZE);
			pRslts.reset(pStmnt->executeQuery(osql.str()));

			// The first resultset contains first record #,  last record # and total record #
			if (pRslts.get() && pRslts->next())
			{
				lStart = pRslts->getLong("Start");

				if (pRslts->wasNull())
					lStart = 0;

				lEnd = pRslts->getLong("End");

				if (pRslts->wasNull())
					lEnd = 0;

				lTotal = pRslts->getLong("Total");

				if (pRslts->wasNull())
					lTotal = 0;

				assert(!pRslts->next());	// WE SHOULD HAVE THE LAST RECORD
			}

			pRslts.reset();

			if (!pStmnt->getMoreResults())
				throw string("Failed to fetch data through procedure call ") + osql.str();

			pRslts.reset(pStmnt->getResultSet());

			pEL = new EntityPtrList();

			while (pRslts.get() && pRslts->next())
			{
				pObject = PopulateObject(pMetaEntity, pRslts.get(), true);
				pEL->push_back(pObject);
			}

			bSuccess = true;
		}
		catch(odbc::SQLException& e)
		{
			pDB->CheckConnection(e);
			delete pEL;
			throw;
		}
		catch(...)
		{
			delete pEL;
			throw;
		}

		if (!bSuccess)
		{
			delete pEL;
			return NULL;
		}

		return pEL;
	}




	// Loads objects by calling the stored procedure specified
	void ODBCDatabase::LoadObjectsThroughStoredProcedure(MetaEntityPtrList & listME, CreateStoredProcedureFunc pfnCreateProc, const std::string & strProcName, const std::string & strProcArgs, bool bRefresh, bool bLazyFetch)
	{
		ConnectionManager		conMgr(this);

		std::auto_ptr<odbc::Statement>		pStmnt;
		std::auto_ptr<odbc::ResultSet>		pRslts;
		std::ostringstream								osql;
		MetaEntityPtr											pME;
		MetaEntityPtrListItr							itrME;
		bool															bFirst = true;


		if (listME.empty())
		{
			GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "ODBCDatabase::LoadObjectsThroughStoredProcedure(): Must supply a list of MetaEntity objects to load.");
			return;
		}

#ifdef _DEBUG
		// Verify list
		for (	itrME =  listME.begin();
					itrME != listME.end();
					itrME++)
		{
			pME = *itrME;
			assert(pME->GetMetaDatabase() == GetMetaDatabase());
		}
#endif

		try
		{
			Reconnect();

			osql << "{call " << strProcName << '(' << strProcArgs << ")}";

			if (m_uTrace != D3DB_TRACE_NONE)
				ReportInfo("ODBCDatabase::LoadObjectsThroughStoredProcedure............: Database " PRINTF_POINTER_MASK ". SQL: %s", this, osql.str().c_str());

			pStmnt.reset(conMgr.connection()->createStatement(odbc::ResultSet::TYPE_FORWARD_ONLY, odbc::ResultSet::CONCUR_READ_ONLY));
			pStmnt->setFetchSize(LIBODBC_FETCH_SIZE);

			while (true)
			{
				try
				{
					pRslts.reset(pStmnt->executeQuery(osql.str()));
					bFirst = true;
					break;
				}
				catch(odbc::SQLException&e)
				{
					// SQL Error 2812: The stored procedure does not exists
					if (!pfnCreateProc || !bFirst || e.getErrorCode() != 2812)
						throw;

					bFirst = false;

					// Let's call the create SP proc passed in
					(pfnCreateProc)(this);
				}
			}

			itrME = listME.begin();

			while (true)
			{
				pME = *itrME;

				while (pRslts.get() && pRslts->next())
				{
					PopulateObject(pME, pRslts.get(), bRefresh, bLazyFetch);
				}

				pRslts.reset();

				itrME++;

				if (itrME != listME.end())
				{
					if (!pStmnt->getMoreResults())
						throw string("No resultset for ") + pME->GetName();

					pRslts.reset(pStmnt->getResultSet());
				}
				else
				{
					if (pStmnt->getMoreResults())
						throw string("More resultsets returned than expected");

					break;
				}
			}
		}
		catch(odbc::SQLException&e)
		{
			CheckConnection(e);
			throw;
		}
	}




	void ODBCDatabase::CreateStoredProcedure(const std::string & strSPBody)
	{
		boost::recursive_mutex::scoped_lock		lk(M_CreateSPMutex);

		try
		{
			ExecuteSQLReadCommand(strSPBody);
		}
		catch (odbc::SQLException& x)
		{
			// SQL Server Error 2714 - SP exists (we're happy with that)
			if (x.getErrorCode() != 2714)
				throw;
		}
	}




	//! Executes the stored procedure and returns the resultsets in the form of a json structure (see [D3::Database::ExecuteStoredProcedureAsJSON] for more details)
	void ODBCDatabase::ExecuteStoredProcedureAsJSON(std::ostringstream& ostrm, CreateStoredProcedureFunc pfnCreateProc, const std::string & strProcName, const std::string & strProcArgs)
	{
		ConnectionManager		conMgr(this);

		std::auto_ptr<odbc::Statement>					pStmnt;
		std::auto_ptr<odbc::ResultSet>					pRslts;
		std::ostringstream											osql;
		bool																		bFirst = true;


		try
		{
			Reconnect();

			osql << "{call " << strProcName << "(" << strProcArgs << ")}";

			if (m_uTrace != D3DB_TRACE_NONE)
				ReportInfo("ODBCDatabase::ExecuteStoredProcedureAsJSON.................: Database " PRINTF_POINTER_MASK ". SQL: %s", this, osql.str().c_str());

			pStmnt.reset(conMgr.connection()->createStatement(odbc::ResultSet::TYPE_FORWARD_ONLY, odbc::ResultSet::CONCUR_READ_ONLY));
			pStmnt->setFetchSize(LIBODBC_FETCH_SIZE);

			while (true)
			{
				try
				{
					pRslts.reset(pStmnt->executeQuery(osql.str()));
					bFirst = true;
					break;
				}
				catch(odbc::SQLException&e)
				{
					if (!pfnCreateProc || !bFirst)
						throw;

					// SQL Error 2812: The stored procedure does not exists
					if (e.getErrorCode() == 2812)
					{
						bFirst = false;

						// Let's call the create SP proc passed in
						(pfnCreateProc)(this);
					}
				}
			}

			ostrm << '[';

			while(true)
			{
				if (!pRslts.get())
				{
					if (pStmnt->getMoreResults())
						pRslts.reset(pStmnt->getResultSet());
					else
						break;
				}

				if (bFirst)
					bFirst = false;
				else
					ostrm << ',';

				ostrm << '[';
				WriteJSONToStream(ostrm, pRslts.get());
				ostrm << ']';

				pRslts.reset();
			}

			ostrm << ']';
		}
		catch(odbc::SQLException& e)
		{
			CheckConnection(e);
			throw;
		}
	}





	// This blocking method will not return until an alert fires
	DatabaseAlertPtr ODBCDatabase::MonitorDatabaseAlerts(DatabaseAlertManagerPtr pDBAlertManager)
	{
		NativeConnectionPtr			pCon = CreateNativeConnection(m_pMetaDatabase);
		SQLHANDLE								hStmnt = NULL;
		SQLRETURN								ret;
		SQLLEN									iLen;
		SQLCHAR									sVarChar[2048];
		std::string							strSQL, strErrPrefix("ODBCDatabase::MonitorDatabaseAlerts(): ");
		DatabaseAlertPtr				pAlert = NULL;

		try
		{
			// statement we execute
			strSQL  = "WAITFOR (RECEIVE CAST(CAST(message_body AS XML) AS VARCHAR(MAX)) AS msg FROM ";
			strSQL += pDBAlertManager->AlertServiceName();
			strSQL += "_Queue)";

			// Allocate a statement handle
			ret = SQLAllocHandle(SQL_HANDLE_STMT, pCon->hCon, &hStmnt);

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_DBC, pCon->hCon, strErrPrefix + "Failed to allocate a statement handle. ");

			// Trace if enabled
			if (m_uTrace & (D3DB_TRACE_SELECT | D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE))
				ReportInfo("ODBCDatabase::MonitorDatabaseAlerts()......................: Database " PRINTF_POINTER_MASK ", Waiting for notification. SQL: %s", this, strSQL.c_str());

			// Wait for query notification (this will block)
			ret = SQLExecDirect(hStmnt, (SQLCHAR*) strSQL.c_str(), SQL_NTS);

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_STMT, hStmnt, strErrPrefix + "Wait for query notification failed. ");

			// Fetch the message
			ret = SQLFetch(hStmnt);

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_STMT, hStmnt, strErrPrefix + "Failed to fetch the message. ");

			// Get the message_body
			ret = SQLGetData(hStmnt, 1, SQL_C_CHAR, sVarChar, sizeof(sVarChar)/sizeof(SQLCHAR), &iLen);

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_STMT, hStmnt, strErrPrefix + "Failed to fetch the message. ");

			// Trace if enabled
			if (m_uTrace & (D3DB_TRACE_SELECT | D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE))
				ReportInfo("ODBCDatabase::MonitorDatabaseAlerts()......................: Database " PRINTF_POINTER_MASK ", Got notification %s.", this, sVarChar);
		}
		catch (std::string & e)
		{
			if (hStmnt) SQLFreeHandle(SQL_HANDLE_STMT, hStmnt);
			ReleaseNativeConnection(m_pMetaDatabase, pCon);
			throw Exception(__FILE__, __LINE__, Exception_error, e.c_str());
		}

		if (hStmnt) SQLFreeHandle(SQL_HANDLE_STMT, hStmnt);
		ReleaseNativeConnection(m_pMetaDatabase, pCon);

		pAlert = ParseAlertMessage(pDBAlertManager, std::string((char*) sVarChar));

		// All request require re-registration after firing
		if (pAlert)
			pAlert->m_bRegistered = false;

		return pAlert;
	}




	// Discard exiting QN resources
	void ODBCDatabase::DropObsoleteQNResources()
	{
		ConnectionManager		conMgr(this);

		std::auto_ptr<odbc::Statement>	pStmnt;
		std::auto_ptr<odbc::ResultSet>	pRslts;
		std::string							strSQL;
		MetaDatabasePtrMapItr		itr;
		MetaDatabasePtr					pMDB;
		std::list<std::string>	listServices;
		std::list<std::string>	listQueues;


		// Remove all services and queues for this server and each database
		for ( itr =  MetaDatabase::GetMetaDatabases().begin();
					itr != MetaDatabase::GetMetaDatabases().end();
					itr++)
		{
			// Two iterations, first for services and second for queues
			for (int i = 0; i < 2; i++)
			{
				pMDB = itr->second;

				// build the statement
				// select name from sys.services where name like 'host_databasename_%'
				if (i == 0)
					strSQL = "SELECT name FROM sys.services WHERE name LIKE '";
				else
					strSQL = "SELECT name FROM sys.service_queues WHERE name LIKE '";

				strSQL += GetSystemHostName();
				strSQL += '_';
				strSQL += pMDB->GetName();
				strSQL += "_%'";

				try
				{
					Reconnect();

					pStmnt.reset(conMgr.connection()->createStatement(odbc::ResultSet::TYPE_SCROLL_INSENSITIVE, odbc::ResultSet::CONCUR_READ_ONLY));
					pStmnt->setFetchSize(LIBODBC_FETCH_SIZE);

					if (m_uTrace & D3DB_TRACE_SELECT)
						ReportInfo("ODBCDatabase::DropObsoleteQNResources()....................: Database " PRINTF_POINTER_MASK ". SQL: %s", this, strSQL.c_str());

					pRslts.reset(pStmnt->executeQuery(strSQL));

					if (pRslts->first())
					{
						while (!pRslts->isAfterLast())
						{
							if (i == 0)
								listServices.push_back(pRslts->getString(1));
							else
								listQueues.push_back(pRslts->getString(1));

							pRslts->next();
						}
					}

					pRslts.reset();
				}
				catch(odbc::SQLException& e)
				{
					CheckConnection(e);
					throw;
				}
			}
		}

		// Drop Services
		while (!listServices.empty())
		{
			try
			{
				strSQL = "DROP SERVICE ";
				strSQL += listServices.front();
				this->ExecuteSQLReadCommand(strSQL);
			}
			catch(odbc::SQLException& e)
			{
				CheckConnection(e);
				throw;
			}

			listServices.pop_front();
		}

		// Drop Queues
		while (!listQueues.empty())
		{
			try
			{
				strSQL = "DROP QUEUE ";
				strSQL += listQueues.front();
				this->ExecuteSQLReadCommand(strSQL);
			}
			catch(odbc::SQLException& e)
			{
				CheckConnection(e);
				throw;
			}

			listQueues.pop_front();
		}

		M_bQNInitialised = true;
	}



	//! Initialise DatabaseAlertResources
	bool ODBCDatabase::InitialiseDatabaseAlerts(DatabaseAlertManagerPtr pDBAlertMngr)
	{
		std::string					strSQL;


		// Drop existing QN resources if not already done
		if (!M_bQNInitialised)
			DropObsoleteQNResources();

		// Create Queue
		strSQL  = "CREATE QUEUE ";
		strSQL += pDBAlertMngr->AlertServiceName();
		strSQL += "_Queue";

		ExecuteSQLReadCommand(strSQL);

		// Create Service on this queue
		strSQL  = "CREATE SERVICE ";
		strSQL += pDBAlertMngr->AlertServiceName();
		strSQL += " ON QUEUE ";
		strSQL += pDBAlertMngr->AlertServiceName();
		strSQL += "_Queue ([http://schemas.microsoft.com/SQL/Notifications/PostQueryNotification])";

		ExecuteSQLReadCommand(strSQL);

		// Should trap errors because it would be ok if these statements failed in case the
		// thingies already existed

		return true;
	}





	//! UnInitialise DatabaseAlertResources
	void ODBCDatabase::UnInitialiseDatabaseAlerts(DatabaseAlertManagerPtr pDBAlertMngr)
	{
		std::string					strSQL;

		// Drop Service
		strSQL  = "DROP SERVICE ";
		strSQL += pDBAlertMngr->AlertServiceName();

		ExecuteSQLReadCommand(strSQL);

		// Drop Queue
		strSQL  = "DROP QUEUE ";
		strSQL += pDBAlertMngr->AlertServiceName();
		strSQL += "_Queue";

		ExecuteSQLReadCommand(strSQL);

		// Should trap errors because it would be ok if these statements failed in case the
		// thingies already existed
	}




/*
	//! Register DatabaseAlert
	bool ODBCDatabase::RegisterDatabaseAlert(DatabaseAlertPtr pDBAlert)
	{
		NativeConnectionPtr			pCon = CreateNativeConnection(m_pMetaDatabase);
		SQLHANDLE								hStmnt = NULL;
		SQLRETURN								ret;
		std::string							strErrPrefix("ODBCDatabase::RegisterDatabaseAlert(): ");


		try
		{
			// Allocate a statement handle
			ret = SQLAllocHandle(SQL_HANDLE_STMT, pCon->hCon, &hStmnt);

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_DBC, pCon->hCon, strErrPrefix + "Failed to allocate a statement handle. ");

			// Set MSGTEXT statement attribute
			ret = SQLSetStmtAttr(hStmnt, SQL_SOPT_SS_QUERYNOTIFICATION_MSGTEXT, (SQLPOINTER) pDBAlert->GetName().c_str(), (SQLINTEGER) pDBAlert->GetName().size());

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_STMT, hStmnt, strErrPrefix + "Failed to set MSGTEXT statement attribute. ");

			// Set OPTIONS statement attribute
			std::string	strQNOptions;

			strQNOptions  = "service=";
			strQNOptions += pDBAlert->m_pDatabaseAlertManager->AlertServiceName();

			ret = SQLSetStmtAttr(hStmnt, SQL_SOPT_SS_QUERYNOTIFICATION_OPTIONS, (SQLPOINTER) strQNOptions.c_str(), (SQLINTEGER) strQNOptions.size());

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_STMT, hStmnt, strErrPrefix + "Failed to set OPTIONS statement attribute. ");

			// Set TIMEOUT statement attribute
			ret = SQLSetStmtAttr(hStmnt, SQL_SOPT_SS_QUERYNOTIFICATION_TIMEOUT, (void*) pDBAlert->m_iTimeout, NULL);

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_STMT, hStmnt, strErrPrefix + "Failed to set TIMEOUT statement attribute. ");

			// Trace if enabled
			if (m_uTrace & D3DB_TRACE_UPDATE)
				ReportInfo("ODBCDatabase::RegisterDatabaseAlert()......................: Database " PRINTF_POINTER_MASK ", SQL: %s", this, pDBAlert->GetSQL().c_str());

			// Do the Query Notification registration
			ret = SQLExecDirect(hStmnt, (SQLCHAR*) pDBAlert->GetSQL().c_str(), SQL_NTS);

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_STMT, hStmnt, strErrPrefix + "Query Notification registration failed. ");
		}
		catch (std::string & e)
		{
			if (hStmnt) SQLFreeHandle(SQL_HANDLE_STMT, hStmnt);
			ReleaseNativeConnection(m_pMetaDatabase, pCon);
			throw Exception(__FILE__, __LINE__, Exception_error, e.c_str());
		}

		if (hStmnt) SQLFreeHandle(SQL_HANDLE_STMT, hStmnt);
		ReleaseNativeConnection(m_pMetaDatabase, pCon);

		pDBAlert->m_bRegistered = true;
		pDBAlert->m_bEnabled = true;

		return true;
	}
*/
	//! Register DatabaseAlert
	void ODBCDatabase::RegisterDatabaseAlert(DatabaseAlertPtr pDBAlert)
	{
		RegisterDatabaseAlert(pDBAlert->m_pDatabaseAlertManager->AlertServiceName(), pDBAlert->GetName(), pDBAlert->GetSQL(), pDBAlert->m_iTimeout);

		pDBAlert->m_bRegistered = true;
		pDBAlert->m_bEnabled = true;
	}




	//! Register DatabaseAlert
	void ODBCDatabase::RegisterDatabaseAlert(const std::string & strServiceName, const std::string & strAlertName, const std::string & strSQL, int iTimeout)
	{
		NativeConnectionPtr			pCon = CreateNativeConnection(m_pMetaDatabase);
		SQLHANDLE								hStmnt = NULL;
		SQLRETURN								ret;
		std::string							strErrPrefix("ODBCDatabase::RegisterDatabaseAlert(): ");
		int											iTimeOut = std::max(1, iTimeout);

		try
		{
			// Allocate a statement handle
			ret = SQLAllocHandle(SQL_HANDLE_STMT, pCon->hCon, &hStmnt);

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_DBC, pCon->hCon, strErrPrefix + "Failed to allocate a statement handle. ");

			// Set MSGTEXT statement attribute
			ret = SQLSetStmtAttr(hStmnt, SQL_SOPT_SS_QUERYNOTIFICATION_MSGTEXT, (SQLPOINTER) strAlertName.c_str(), (SQLINTEGER) strAlertName.size());

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_STMT, hStmnt, strErrPrefix + "Failed to set MSGTEXT statement attribute. ");

			// Set OPTIONS statement attribute
			std::string	strQNOptions;

			strQNOptions  = "service=";
			strQNOptions += strServiceName;

			ret = SQLSetStmtAttr(hStmnt, SQL_SOPT_SS_QUERYNOTIFICATION_OPTIONS, (SQLPOINTER) strQNOptions.c_str(), (SQLINTEGER) strQNOptions.size());

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_STMT, hStmnt, strErrPrefix + "Failed to set OPTIONS statement attribute. ");

			// Set TIMEOUT statement attribute
			ret = SQLSetStmtAttr(hStmnt, SQL_SOPT_SS_QUERYNOTIFICATION_TIMEOUT, (void*) iTimeOut, NULL);

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_STMT, hStmnt, strErrPrefix + "Failed to set TIMEOUT statement attribute. ");

			// Trace if enabled
			if (m_uTrace & D3DB_TRACE_UPDATE)
				ReportInfo("ODBCDatabase::RegisterDatabaseAlert()......................: Database " PRINTF_POINTER_MASK ", Before registration. SQL: %s", this, strSQL.c_str());

			// Do the Query Notification registration
			ret = SQLExecDirect(hStmnt, (SQLCHAR*) strSQL.c_str(), SQL_NTS);

			if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO)
				throw GetODBCError(SQL_HANDLE_STMT, hStmnt, strErrPrefix + "Query Notification registration failed. ");

			// Trace if enabled
			if (m_uTrace & D3DB_TRACE_UPDATE)
				ReportInfo("ODBCDatabase::RegisterDatabaseAlert()......................: Database " PRINTF_POINTER_MASK ", After registration.", this);
		}
		catch (std::string & e)
		{
			if (hStmnt) SQLFreeHandle(SQL_HANDLE_STMT, hStmnt);
			ReleaseNativeConnection(m_pMetaDatabase, pCon);
			throw Exception(__FILE__, __LINE__, Exception_error, e.c_str());
		}

		if (hStmnt) SQLFreeHandle(SQL_HANDLE_STMT, hStmnt);
		ReleaseNativeConnection(m_pMetaDatabase, pCon);
	}





	//! UnRegister DatabaseAlert
	void ODBCDatabase::UnRegisterDatabaseAlert(DatabaseAlertPtr pDBAlert)
	{
		// The best we can do in SQL server is to re register the alert with the
		// shortest possible timeout

		pDBAlert->m_iTimeout = 1;
		RegisterDatabaseAlert(pDBAlert);
		pDBAlert->m_bEnabled = false;
	}





	/*	Return a string that consists of the strMsg passed in with additional error info appended

			The handle type must much the handle passed in and be one of these consts:

				SQL_HANDLE_ENV
				SQL_HANDLE_DBC
				SQL_HANDLE_STMT
	*/
	/* static */
	std::string ODBCDatabase::GetODBCError(SQLSMALLINT iHandleType, SQLHANDLE& hHandle, const std::string& strMsg)
	{
		SQLCHAR							szSqlState[6], szMsg[SQL_MAX_MESSAGE_LENGTH];
		SQLINTEGER					iNativeError;
		SQLSMALLINT					i, iMsgLen;
		SQLRETURN						rc;
		std::ostringstream	ostrm;

		ostrm << strMsg;

		// Get the status records.
		i = 1;
		while (true)
		{
			rc = SQLGetDiagRec(iHandleType, hHandle, i, szSqlState, &iNativeError, szMsg, sizeof(szMsg), &iMsgLen);

			if (rc != SQL_SUCCESS && rc != SQL_SUCCESS_WITH_INFO)
				break;

			ostrm << " SQLState: " << szSqlState << ", SQLError: [" << iNativeError << "] " << szMsg << endl;
			i++;
		}

		return ostrm.str();
	}





	// Parse an SQL Server Notification.
	// See http://msdn.microsoft.com/en-us/library/ms189308.aspx for details
	DatabaseAlertPtr ODBCDatabase::ParseAlertMessage(DatabaseAlertManagerPtr pDBAlertManager, const std::string& strXMLMessage)
	{
		CSL::XML::DOMBuilder		domBuilder;
		CSL::XML::Document *		pXMLDoc = NULL;
		std::istringstream			istrm(strXMLMessage);
		CSL::XML::CInputSource	xmlIn(&istrm);
		CSL::XML::Node*					pNode;
		CSL::XML::Node*					pChildNode;
		CSL::XML::Node*					pAttribute;
		std::string							strSource, strType, strInfo, strMessage;
		DatabaseAlertPtr				pAlert = NULL;


		try
		{
			domBuilder.setFeature("white-space-in-element-content", false); // remove ignorable whitespace

			pXMLDoc = domBuilder.parseDOMInputSource(&xmlIn);

			pNode	= pXMLDoc->getFirstChild();

			// We're after the "QueryNotification" node
			if (pNode && pNode->getLocalName() == "QueryNotification")
			{
				// We're after the "QueryNotification" node's attributes: Source and Info
				CSL::XML::NamedNodeMap*	pNodeMap = pNode->getAttributes();

				pAttribute = pNodeMap->getNamedItem("source");
				if (pAttribute) strSource = pAttribute->getNodeValue();

				pAttribute = pNodeMap->getNamedItem("type");
				if (pAttribute) strType = pAttribute->getNodeValue();

				pAttribute = pNodeMap->getNamedItem("info");
				if (pAttribute) strInfo = pAttribute->getNodeValue();

				pNodeMap->release();
			}

			pChildNode = pNode->getFirstChild();

			while (pChildNode)
			{
				// We're after the "QueryNotification/Message" node
				if (pChildNode->getLocalName() == "Message")
				{
					strMessage = pChildNode->getFirstChild()->getNodeValue();
					break;
				}

				pChildNode = pChildNode->getNextSibling();
			}
		}
		catch (CSL::XML::CXMLException &)
		{
			if (pXMLDoc)
				pXMLDoc->release();

			return NULL;
		}

		if (pXMLDoc) pXMLDoc->release();

		// Trace if enabled
		if (m_uTrace & (D3DB_TRACE_SELECT | D3DB_TRACE_DELETE | D3DB_TRACE_UPDATE))
			ReportInfo("ODBCDatabase::ParseAlertMessage()..........................: Database " PRINTF_POINTER_MASK ", Message: %s, Source: %s, Type: %s, Info: %s.", this, strMessage.c_str(), strSource.c_str(), strType.c_str(), strInfo.c_str());

		// let's see what we've got
		pAlert = pDBAlertManager->GetDatabaseAlert(strMessage);

		if (pAlert)
		{
			if (strSource == "data")
			{
				pAlert->m_iStatus = 0;
				pAlert->m_strMessage = strInfo;
			}
			else if (strSource == "timeout")
			{
				pAlert->m_iStatus = 1;
			}
			else
			{
				pAlert->m_iStatus = -1;
				pAlert->m_strMessage = strSource + "/" + strInfo;
			}
		}

		return pAlert;
	}





	void ODBCDatabase::SetMetaDatabaseHSTopics()
	{
		ConnectionManager		conMgr(this);

		string										strSQL, strTargetID, strTargetMask, strTargetFilter, strCurrAlias, strPrevAlias, strTitle;
		int												iID;
		MetaDatabasePtr						pMD;
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

		try
		{
			Reconnect();

			strSQL = "SELECT mot.MetaDatabaseAlias, t.ID, t.Title FROM HSMetaDatabaseTopic mot LEFT JOIN HSTopic t ON mot.TopicID=t.ID ";
			strSQL += strTargetFilter;
			strSQL += " ORDER BY mot.MetaDatabaseAlias, t.Title";

			std::auto_ptr<odbc::Statement> pStmnt(conMgr.connection()->createStatement(odbc::ResultSet::TYPE_SCROLL_INSENSITIVE, odbc::ResultSet::CONCUR_READ_ONLY));
			pStmnt->setFetchSize(LIBODBC_FETCH_SIZE);

			if (m_uTrace & D3DB_TRACE_SELECT)
				ReportInfo("ODBCDatabase::SetMetaDatabaseHSTopics()....................: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this), strSQL.c_str());

			std::auto_ptr<odbc::ResultSet> pRslts(pStmnt->executeQuery(strSQL));

			if (pRslts->first())
			{
				while (!pRslts->isAfterLast())
				{
					// Get the strAlias, iID and strTitle;
					strCurrAlias	= pRslts->getString(1);
					iID						= pRslts->getInt(2);
					strTitle			= pRslts->getString(3);

					// Handle group breaks
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

					pRslts->next();
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
		}
		catch (odbc::SQLException& e)
		{
			CheckConnection(e);
			throw;
		}
	}





	void ODBCDatabase::SetMetaEntityHSTopics()
	{
		ConnectionManager		conMgr(this);

		string										strSQL, strTargetID, strTargetMask, strTargetFilter, strCurrAlias, strPrevAlias, strCurrEntity, strPrevEntity, strTitle;
		int												iID;
		MetaDatabasePtr						pMD;
		MetaEntityPtr							pME;
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

		try
		{
			Reconnect();

			strSQL = "SELECT mot.MetaDatabaseAlias, mot.MetaEntityName, t.ID, t.Title FROM HSMetaEntityTopic mot LEFT JOIN HSTopic t ON mot.TopicID=t.ID ";
			strSQL += strTargetFilter;
			strSQL += " ORDER BY mot.MetaDatabaseAlias, mot.MetaEntityName, t.Title";

			std::auto_ptr<odbc::Statement> pStmnt(conMgr.connection()->createStatement(odbc::ResultSet::TYPE_SCROLL_INSENSITIVE, odbc::ResultSet::CONCUR_READ_ONLY));
			pStmnt->setFetchSize(LIBODBC_FETCH_SIZE);

			if (m_uTrace & D3DB_TRACE_SELECT)
				ReportInfo("ODBCDatabase::SetMetaEntityHSTopics()......................: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this), strSQL.c_str());

			std::auto_ptr<odbc::ResultSet> pRslts(pStmnt->executeQuery(strSQL));

			if (pRslts->first())
			{
				while (!pRslts->isAfterLast())
				{
					// Get the strAlias, iID and strTitle;
					strCurrAlias	= pRslts->getString(1);
					strCurrEntity	= pRslts->getString(2);
					iID						= pRslts->getInt(3);
					strTitle			= pRslts->getString(4);

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

					pRslts->next();
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
		}
		catch (odbc::SQLException& e)
		{
			CheckConnection(e);
			throw;
		}
	}





	void ODBCDatabase::SetMetaColumnHSTopics()
	{
		ConnectionManager		conMgr(this);

		string										strSQL, strTargetID, strTargetMask, strTargetFilter, strCurrAlias, strPrevAlias, strCurrEntity, strPrevEntity, strCurrColumn, strPrevColumn, strTitle;
		int												iID;
		MetaDatabasePtr						pMD;
		MetaEntityPtr							pME;
		MetaColumnPtr							pMC;
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

		try
		{
			Reconnect();

			strSQL = "SELECT mot.MetaDatabaseAlias, mot.MetaEntityName, mot.MetaColumnName, t.ID, t.Title FROM HSMetaColumnTopic mot LEFT JOIN HSTopic t ON mot.TopicID=t.ID ";
			strSQL += strTargetFilter;
			strSQL += " ORDER BY mot.MetaDatabaseAlias, mot.MetaEntityName, mot.MetaColumnName, t.Title";

			std::auto_ptr<odbc::Statement> pStmnt(conMgr.connection()->createStatement(odbc::ResultSet::TYPE_SCROLL_INSENSITIVE, odbc::ResultSet::CONCUR_READ_ONLY));
			pStmnt->setFetchSize(LIBODBC_FETCH_SIZE);

			if (m_uTrace & D3DB_TRACE_SELECT)
				ReportInfo("ODBCDatabase::SetMetaColumnHSTopics()......................: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this), strSQL.c_str());

			std::auto_ptr<odbc::ResultSet> pRslts(pStmnt->executeQuery(strSQL));

			if (pRslts->first())
			{
				while (!pRslts->isAfterLast())
				{
					// Get the strAlias, iID and strTitle;
					strCurrAlias	= pRslts->getString(1);
					strCurrEntity	= pRslts->getString(2);
					strCurrColumn	= pRslts->getString(3);
					iID						= pRslts->getInt(4);
					strTitle			= pRslts->getString(5);

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

					pRslts->next();
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
		}
		catch (odbc::SQLException& e)
		{
			CheckConnection(e);
			throw;
		}
	}





	void ODBCDatabase::SetMetaKeyHSTopics()
	{
		ConnectionManager		conMgr(this);

		string										strSQL, strTargetID, strTargetMask, strTargetFilter, strCurrAlias, strPrevAlias, strCurrEntity, strPrevEntity, strCurrKey, strPrevKey, strTitle;
		int												iID;
		MetaDatabasePtr						pMD;
		MetaEntityPtr							pME;
		MetaKeyPtr								pMK;
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

		try
		{
			Reconnect();

			strSQL = "SELECT mot.MetaDatabaseAlias, mot.MetaEntityName, mot.MetaKeyName, t.ID, t.Title FROM HSMetaKeyTopic mot LEFT JOIN HSTopic t ON mot.TopicID=t.ID ";
			strSQL += strTargetFilter;
			strSQL += " ORDER BY mot.MetaDatabaseAlias, mot.MetaEntityName, mot.MetaKeyName, t.Title";

			std::auto_ptr<odbc::Statement> pStmnt(conMgr.connection()->createStatement(odbc::ResultSet::TYPE_SCROLL_INSENSITIVE, odbc::ResultSet::CONCUR_READ_ONLY));
			pStmnt->setFetchSize(LIBODBC_FETCH_SIZE);

			if (m_uTrace & D3DB_TRACE_SELECT)
				ReportInfo("ODBCDatabase::SetMetaKeyHSTopics().........................: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this), strSQL.c_str());

			std::auto_ptr<odbc::ResultSet> pRslts(pStmnt->executeQuery(strSQL));

			if (pRslts->first())
			{
				while (!pRslts->isAfterLast())
				{
					// Get the strAlias, iID and strTitle;
					strCurrAlias	= pRslts->getString(1);
					strCurrEntity	= pRslts->getString(2);
					strCurrKey		= pRslts->getString(3);
					iID						= pRslts->getInt(4);
					strTitle			= pRslts->getString(5);

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

					pRslts->next();
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
		}
		catch (odbc::SQLException& e)
		{
			CheckConnection(e);
			throw;
		}
	}





	void ODBCDatabase::SetMetaRelationHSTopics()
	{
		ConnectionManager		conMgr(this);

		string										strSQL, strTargetID, strTargetMask, strTargetFilter, strCurrAlias, strPrevAlias, strCurrEntity, strPrevEntity, strCurrRelation, strPrevRelation, strTitle;
		int												iID;
		MetaDatabasePtr						pMD;
		MetaEntityPtr							pME;
		MetaRelationPtr						pMR;
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

		try
		{
			Reconnect();

			strSQL = "SELECT mot.MetaDatabaseAlias, mot.MetaEntityName, mot.MetaRelationName, t.ID, t.Title FROM HSMetaRelationTopic mot LEFT JOIN HSTopic t ON mot.TopicID=t.ID ";
			strSQL += strTargetFilter;
			strSQL += " ORDER BY mot.MetaDatabaseAlias, mot.MetaEntityName, mot.MetaRelationName, t.Title";

			std::auto_ptr<odbc::Statement> pStmnt(conMgr.connection()->createStatement(odbc::ResultSet::TYPE_SCROLL_INSENSITIVE, odbc::ResultSet::CONCUR_READ_ONLY));
			pStmnt->setFetchSize(LIBODBC_FETCH_SIZE);

			if (m_uTrace & D3DB_TRACE_SELECT)
				ReportInfo("ODBCDatabase::SetMetaRelationHSTopics().........................: Database " PRINTF_POINTER_MASK " (Transaction count: %u). SQL: %s", this, m_pMetaDatabase->m_TransactionManager.GetTransactionLevel(this), strSQL.c_str());

			std::auto_ptr<odbc::ResultSet> pRslts(pStmnt->executeQuery(strSQL));

			if (pRslts->first())
			{
				while (!pRslts->isAfterLast())
				{
					// Get the strAlias, iID and strTitle;
					strCurrAlias			= pRslts->getString(1);
					strCurrEntity			= pRslts->getString(2);
					strCurrRelation		= pRslts->getString(3);
					iID								= pRslts->getInt(4);
					strTitle					= pRslts->getString(5);

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

					pRslts->next();
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
		}
		catch (odbc::SQLException& e)
		{
			CheckConnection(e);
			throw;
		}
	}






	// Disconnect error codes (these are error codes returned by odbc::SQLException if the connection to the database failed)
	void ODBCDatabase::CheckConnection(odbc::SQLException& e)
	{
		switch (e.getErrorCode())
		{
			case 0:
				if (e.getMessage().find("Communication link failure") != std::string::npos)
					m_bIsConnected = false;
				break;

			case 64:
			case 10054:
				m_bIsConnected = false;
		}

		if (!m_bIsConnected)
			ReportError("ODBCDatabase::CheckConnection()............................: Database " PRINTF_POINTER_MASK " lost its connection.", this);
	}






	// Attempts to reestablish a connection if it isn't already connected
	void ODBCDatabase::Reconnect()
	{
		if (!m_bIsConnected)
		{
			Connect();
			ReportInfo("ODBCDatabase::ReconnectConnection()........................: Database " PRINTF_POINTER_MASK " succeeded reconnecting. ", this);
		}
	}





	// helper that writes the current record in pRslts to ostrm
	void ODBCDatabase::WriteJSONToStream(ostringstream & ostrm, odbc::ResultSet* pRslts)
	{
		bool												bFirstRec = true;

		while (pRslts->next())
		{
			int													idx;
			odbc::ResultSetMetaData*		pMD = pRslts->getMetaData();
			std::ostringstream					ovalue;
			bool												bFirstCol = true;

			if (bFirstRec)
				bFirstRec = false;
			else
				ostrm << ',';

			ostrm << '{';

			for (idx = 0; idx < pMD->getColumnCount(); idx++)
			{
				if (bFirstCol)
					bFirstCol = false;
				else
					ostrm << ',';

				ostrm << '"' << pMD->getColumnName(idx+1) << "\":";

				switch (pMD->getColumnType(idx+1))
				{
					/** An SQL BIT */
					case odbc::Types::BIT:
						ovalue << (pRslts->getBoolean(idx+1) ? "true" : "false");
						break;

					/** An SQL VARCHAR (variable length less than 4K) */
					case odbc::Types::VARCHAR:
						ovalue << '"' << JSONEncode(pRslts->getString(idx+1)) << '"';
						break;

					/** An SQL TINYINT */
					case odbc::Types::TINYINT:
					/** An SQL INTEGER */
					case odbc::Types::INTEGER:
					/** An SQL SMALLINT */
					case odbc::Types::SMALLINT:
						ovalue << pRslts->getInt(idx+1);
						break;

					/** An SQL TIMESTAMP */
					case odbc::Types::TIMESTAMP:
					{
						D3Date			dt(pRslts->getTimestamp(idx+1), m_pMetaDatabase->GetTimeZone());
						ovalue << '"' << dt.AsUTCISOString(3) << '"';
						break;
					}

					/** An SQL DATE */
					case odbc::Types::DATE:
						ovalue << '"' << pRslts->getDate(idx+1).toString() << '"';
						break;

					/** An SQL FLOAT */
					case odbc::Types::REAL:
					/** An SQL FLOAT */
					case odbc::Types::FLOAT:
						ovalue << pRslts->getFloat(idx+1);
						break;

					default:
						std::cout << "Column " << pMD->getColumnType(idx+1) << " is of type " << pMD->getColumnTypeName(idx+1) << " which is not currently handled!\n";
				}

				if (pRslts->wasNull())
					ostrm << "null";
				else
					ostrm << ovalue.str();

				ovalue.str("");
			}

			ostrm << '}';
		}
	}



} // end namespace D3

#endif /* APAL_SUPPORT_ODBC */
