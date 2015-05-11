#include "D3Types.h"

#include "XMLImporterExporter.h"

#include "Database.h"
#include "Entity.h"
#include "Column.h"
#include "Key.h"
#include "Relation.h"
#include "D3Funcs.h"

// #include <SystemFuncs.h>

#include <iostream>
#include <fstream>

#include <list>
#include <vector>
#include <string>


namespace D3
{
	//===========================================================================
	//
	// XMLImportFileProcessor::Element implementation
	//
	void XMLImportFileProcessor::Element::SetAttributes(const CAttributeList& attrList)
	{
		for (int i = 0; i < attrList.GetLength(); i++)
		{
			m_listAttributes.insert(AttributeList::value_type(attrList.GetName(i), attrList.GetValue(i)));
		}
	}



	void XMLImportFileProcessor::Element::Clear()
	{
		m_strName.erase();
		m_listAttributes.clear();
		m_strValue.erase();
	}



	std::string XMLImportFileProcessor::Element::GetAttribute(const std::string & strName)
	{
		for (AttributeListItr itrAttribute = m_listAttributes.find(strName); itrAttribute != m_listAttributes.end();)
			return (*itrAttribute).second;

		return "";
	}



	void XMLImportFileProcessor::Element::Dump(int iIndent)
	{
		AttributeListItr itrAttribute;


		for (int i=0; i < iIndent; i++)
			std::cout << " ";

		std::cout <<   "<" << m_strName << "> ";

		for ( itrAttribute  = m_listAttributes.begin();
					itrAttribute != m_listAttributes.end();
					itrAttribute++)
		{
			std::cout << "[" << itrAttribute->first << "/" << itrAttribute->second << "] ";
		}

		if (!m_strValue.empty())
			std::cout << m_strValue;

		std::cout << "\n";
	}








	//===========================================================================
	//
	// XMLImportFileProcessor::ElementStack implementation
	//
	std::string XMLImportFileProcessor::ElementStack::GetPath()
	{
		ElementStackItr		itrElements;
		std::string				strPath;


		for ( itrElements  = begin();
					itrElements != end();
					itrElements++)
		{
			strPath += "<";
			strPath += (*itrElements).m_strName;
			strPath += ">";
		}


		return strPath;
	}



	bool XMLImportFileProcessor::ElementStack::IsParentOf(const ElementStack& aElementStack) const
	{
		ElementStack::const_iterator		itrElementThis, itrElementsOther;


		if (empty() || aElementStack.empty() ||
				size() != aElementStack.size() + 1)
			return false;


		for ( itrElementThis  = begin(),			itrElementsOther  = aElementStack.begin();
					itrElementThis != end()			&&	itrElementsOther != aElementStack.end();
					itrElementThis++,               itrElementsOther++)
		{
			if ((*itrElementThis).m_strName != (*itrElementsOther).m_strName)
				return false;
		}


		return true;
	}



	bool XMLImportFileProcessor::ElementStack::IsSiblingOf(const ElementStack& aElementStack) const
	{
		ElementStack::const_iterator		itrElementThis, itrElementsOther;
		int															iMax;


		if (empty() || aElementStack.empty() ||
				size() != aElementStack.size())
			return false;

		iMax = size() - 1;

		for ( itrElementThis  = begin(), itrElementsOther  = aElementStack.begin();;
					itrElementThis++,          itrElementsOther++)
		{
			if ((*itrElementThis).m_strName != (*itrElementsOther).m_strName)
				return false;

			iMax--;

			if (!iMax)
				break;
		}


		return true;
	}



	bool XMLImportFileProcessor::ElementStack::IsDistantParentOf(const ElementStack& aElementStack) const
	{
		ElementStack::const_iterator		itrElementThis, itrElementsOther;


		if (empty() || aElementStack.empty() ||
				size() >= aElementStack.size())
			return false;


		for ( itrElementThis  = begin(), itrElementsOther  = aElementStack.begin();
					itrElementThis != end() && itrElementsOther != aElementStack.end();
					itrElementThis++,          itrElementsOther++)
		{
			if ((*itrElementThis).m_strName != (*itrElementsOther).m_strName)
				return false;
		}


		return true;
	}










	//===========================================================================
	//
	// XMLImportFileProcessor implementation
	//
	void XMLImportFileProcessor::StartDocument()
	{
		if (!m_pDatabase || !m_pListME || m_pListME->empty())
			throw CSAXException("XMLImportFileProcessor requires database and a list of meta entities which can't be empty.");

		cout << "Start importing records into database " << m_pDatabase->GetMetaDatabase()->GetName() << endl;
	}



	void XMLImportFileProcessor::EndDocument()
	{
		if (m_bDocumentEnded)
			return;

		if (!m_bDatabaseFound)
		{
			std:: string		strMessage;

			strMessage  = "XMLImportFileProcessor: Database ";
			strMessage += m_pDatabase->GetMetaDatabase()->GetName();
			strMessage += " is not listed in the import file.";

			throw CSAXException(strMessage);
		}
		else
		{
			bool			bError = false;

			while (!m_pListME->empty())
			{
				cout << "  " << m_pListME->front()->GetName() << " not found in import file!\n";
				m_pListME->pop_front();
				bError = true;
			}

			while (!m_listDiscardME.empty())
			{
				cout << "  " << m_listDiscardME.front()->GetName() << " not found in import file!\n";
				m_listDiscardME.pop_front();
				bError = true;
			}

			if (bError)
				throw CSAXException("Not all entities requested were found in the XML import file");

			cout << "Finished importing records into database " << m_pDatabase->GetMetaDatabase()->GetName() << endl;
			cout << "Records imported..: " << m_lRecCountTotal << endl;

			if (m_bDebug)
				cout << "Element count.....: " << m_iElementCount << endl;
		}

		m_bDocumentEnded = true;
	}



	void XMLImportFileProcessor::StartElement(const std::string& strElementName, const CAttributeList& attrList)
	{
		m_iElementCount++;
		m_CurrentPath.push_back(Element());

		m_CurrentPath.back().m_strName = strElementName;
		m_CurrentPath.back().SetAttributes(attrList);

		if (m_bSkipToNextSibling)
		{
			if (!m_CurrentPath.IsSiblingOf(m_MarkedPath))
				return;

			m_bSkipToNextSibling = false;
		}

		switch (m_eParseState)
		{
			case ExpectRootElement:
				if (m_CurrentPath.back().m_strName == "APALDBData")
				{
					m_eParseState = ExpectDatabaseListElement;
					m_CurrentPath.back().m_eType = Element::RootElement;
				}
				else
				{
					std::string		strMessage;

					strMessage  = "XMLImportFileProcessor: The root element in XML must be <APALDBData>!";

					throw CSAXException(strMessage);
				}

				if (m_bDebug)
					m_CurrentPath.back().Dump(GetCurrentLevel() * 2);

				On_BeforeProcessRootElement();
				break;

			case ExpectDatabaseListElement:
				if (m_CurrentPath.back().m_strName == "DatabaseList")
				{
					m_eParseState = ExpectDatabaseElement;
					m_CurrentPath.back().m_eType = Element::DatabaseListElement;
				}
				else
				{
					std::string		strMessage;

					strMessage  = "XMLImportFileProcessor: While expecting <DatabaseList> found <";
					strMessage += GetCurrentPath();
					strMessage += ">!";

					throw CSAXException(strMessage);
				}

				if (m_bDebug)
					m_CurrentPath.back().Dump(GetCurrentLevel() * 2);

				On_BeforeProcessDatabaseListElement();
				break;

			case ExpectDatabaseElement:
				if (m_CurrentPath.back().m_strName == "Database")
				{
					m_eParseState = ExpectVersionMajorElement;
					m_CurrentPath.back().m_eType = Element::DatabaseElement;
				}
				else
				{
					std::string		strMessage;

					strMessage  = "XMLImportFileProcessor: While expecting <Database> found <";
					strMessage += GetCurrentPath();
					strMessage += ">!";

					throw CSAXException(strMessage);
				}

				if (m_bDebug)
					m_CurrentPath.back().Dump(GetCurrentLevel() * 2);

				On_BeforeProcessDatabaseElement();
				break;

			case ExpectVersionMajorElement:
				if (m_CurrentPath.back().m_strName == "VersionMajor")
				{
					m_CurrentPath.back().m_eType = Element::VersionMajorElement;
				}
				else
				{
					std::string		strMessage;

					strMessage  = "XMLImportFileProcessor: While expecting <VersionMajor> found <";
					strMessage += GetCurrentPath();
					strMessage += ">!";

					throw CSAXException(strMessage);
				}

				m_bProcessingElement = true;

				On_BeforeProcessVersionMajorElement();
				break;

			case ExpectVersionMinorElement:
				if (m_CurrentPath.back().m_strName == "VersionMinor")
				{
					m_CurrentPath.back().m_eType = Element::VersionMinorElement;
				}
				else
				{
					std::string		strMessage;

					strMessage  = "XMLImportFileProcessor: While expecting <VersionMinor> found <";
					strMessage += GetCurrentPath();
					strMessage += ">!";

					throw CSAXException(strMessage);
				}

				m_bProcessingElement = true;

				On_BeforeProcessVersionMinorElement();
				break;

			case ExpectVersionRevisionElement:
				if (m_CurrentPath.back().m_strName == "VersionRevision")
				{
					m_CurrentPath.back().m_eType = Element::VersionRevisionElement;
				}
				else
				{
					std::string		strMessage;

					strMessage  = "XMLImportFileProcessor: While expecting <VersionRevision> found <";
					strMessage += GetCurrentPath();
					strMessage += ">!";

					throw CSAXException(strMessage);
				}

				m_bProcessingElement = true;

				On_BeforeProcessVersionRevisionElement();
				break;

			case ExpectEntityListElement:
				if (m_CurrentPath.back().m_strName == "EntityList")
				{
					m_eParseState = ExpectEntityElement;
					m_CurrentPath.back().m_eType = Element::EntityListElement;
				}
				else
				{
					std::string		strMessage;

					strMessage  = "XMLImportFileProcessor: While expecting <EntityList> found <";
					strMessage += GetCurrentPath();
					strMessage += ">!";

					throw CSAXException(strMessage);
				}

				if (m_bDebug)
					m_CurrentPath.back().Dump(GetCurrentLevel() * 2);

				On_BeforeProcessEntityListElement();
				break;

			case ExpectEntityElement:
				if (m_CurrentPath.back().m_strName == "Entity")
				{
					m_eParseState = ExpectColumnElement;
					m_CurrentPath.back().m_eType = Element::EntityElement;
					m_CurrentEntityPath = m_CurrentPath;
				}
				else
				{
					std::string		strMessage;

					strMessage  = "XMLImportFileProcessor: While expecting <Entity> found <";
					strMessage += GetCurrentPath();
					strMessage += ">!";

					throw CSAXException(strMessage);
				}

				if (m_bDebug)
					m_CurrentPath.back().Dump(GetCurrentLevel() * 2);

				On_BeforeProcessEntityElement();
				break;

			case ExpectColumnElement:
				m_CurrentPath.back().m_eType = Element::ColumnElement;

				if (!m_CurrentEntityPath.IsChildOf(m_CurrentPath))
				{
					std::string		strMessage;

					strMessage  = "XMLImportFileProcessor: While expecting some column element found <";
					strMessage += GetCurrentPath();
					strMessage += ">!";

					throw CSAXException(strMessage);
				}

				m_bProcessingElement = true;

				On_BeforeProcessColumnElement();
				break;
		}
	}


	void XMLImportFileProcessor::EndElement(const std::string& strElementName)
	{
		if (m_bSkipToNextSibling)
		{
			m_CurrentPath.pop_back();
			return;
		}

		switch (m_CurrentPath.back().m_eType)
		{
			case Element::RootElement:
				On_AfterProcessRootElement();
				break;

			case Element::DatabaseListElement:
				On_AfterProcessDatabaseListElement();

				m_eParseState = ExpectDatabaseListElement;
				break;

			case Element::DatabaseElement:
				On_AfterProcessDatabaseElement();

				m_eParseState = ExpectDatabaseElement;
				break;

			case Element::VersionMajorElement:
				m_bProcessingElement = false;

				On_AfterProcessVersionMajorElement();

				m_eParseState = ExpectVersionMinorElement;

				if (m_bDebug)
					m_CurrentPath.back().Dump(GetCurrentLevel() * 2);
				break;

			case Element::VersionMinorElement:
				m_bProcessingElement = false;

				On_AfterProcessVersionMinorElement();

				m_eParseState = ExpectVersionRevisionElement;

				if (m_bDebug)
					m_CurrentPath.back().Dump(GetCurrentLevel() * 2);
				break;

			case Element::VersionRevisionElement:
				m_bProcessingElement = false;

				On_AfterProcessVersionRevisionElement();

				m_eParseState = ExpectEntityListElement;

				if (m_bDebug)
					m_CurrentPath.back().Dump(GetCurrentLevel() * 2);
				break;

			case Element::EntityListElement:
				On_AfterProcessEntityListElement();

				m_eParseState = ExpectEntityListElement;
				break;

			case Element::EntityElement:
				On_AfterProcessEntityElement();

				m_eParseState = ExpectEntityElement;
				break;

			case Element::ColumnElement:
				m_bProcessingElement = false;

				On_AfterProcessColumnElement();

				if (m_bDebug)
					m_CurrentPath.back().Dump(GetCurrentLevel() * 2);
				break;
		}

		m_CurrentPath.pop_back();
	}



	void XMLImportFileProcessor::Characters(const XMLChar ch[], int start, int length)
	{
		if (m_bProcessingElement)
		{
			// Are we dealing with a two byte UTF-8 encoded unicode character?
			if (length == 2 && (ch[0] & 0xE0) == 0xC0)
			{
				// Here we deal with two byte UTF-8 encoded unicode characters. Here is how the two byte UTF-8
				// character sequence maps to unicode and the unicode character range it can represent:
				//
				//			UTF-8               Unicode              Character Range
				//      -----------------   -----------------    ---------------
				//			110xxxxx 10xxxxxx   00000xxx xxxxxxxx    0x80 to 0x7FF
				//
				// Since we only bother about extended ASCII characters (0x80 to 0xFF) we ignore the MSB of the Unicode
				// value derived from the two-byte UTF-8 character sequence.
				//
				m_CurrentPath.back().m_strValue += (char) ((ch[0] & 0x1F) << 6) | (ch[1] & 0x3F);
			}
			else
			{
				if (m_CurrentPath.back().m_strValue.empty())
				{
					m_CurrentPath.back().m_strValue.assign(ch + start, length);
				}
				else
				{
					std::string		strMore(ch + start, length);
					m_CurrentPath.back().m_strValue += strMore;
				}
			}
		}
	}



	void XMLImportFileProcessor::Warning(CSAXException& e)
	{
		throw e;
	}


	void XMLImportFileProcessor::Error(CSAXException& e)
	{
		throw e;
	}


	void XMLImportFileProcessor::FatalError(CSAXException& e)
	{
		throw e;
	}



	void XMLImportFileProcessor::SkipToNextSibling()
	{
		Mark();
		m_bSkipToNextSibling = true;

		switch (m_CurrentPath.back().m_eType)
		{
			case Element::RootElement:
				m_eParseState = ExpectRootElement;
				break;

			case Element::DatabaseListElement:
				m_eParseState = ExpectDatabaseListElement;
				break;

			case Element::DatabaseElement:
				m_eParseState = ExpectDatabaseElement;
				break;

			case Element::VersionMajorElement:
				m_eParseState = ExpectVersionMinorElement;
				break;

			case Element::VersionMinorElement:
				m_eParseState = ExpectVersionRevisionElement;
				break;

			case Element::VersionRevisionElement:
				m_eParseState = ExpectEntityListElement;
				break;

			case Element::EntityListElement:
				m_eParseState = ExpectEntityListElement;
				break;

			case Element::EntityElement:
				m_eParseState = ExpectEntityElement;
				break;

			case Element::ColumnElement:
				m_eParseState = ExpectColumnElement;
				break;
		}
	}



	void XMLImportFileProcessor::On_BeforeProcessRootElement()
	{
	}


	void XMLImportFileProcessor::On_BeforeProcessDatabaseListElement()
	{
	}



	void XMLImportFileProcessor::On_BeforeProcessDatabaseElement()
	{
		// Check if the database is the one we want
		//
		if (m_CurrentPath.back().GetAttribute("Alias") != m_pDatabase->GetMetaDatabase()->GetAlias())
			SkipToNextSibling();
		else
			m_bDatabaseFound = true;
	}



	void XMLImportFileProcessor::On_BeforeProcessVersionMajorElement()
	{
	}



	void XMLImportFileProcessor::On_BeforeProcessVersionMinorElement()
	{
	}



	void XMLImportFileProcessor::On_BeforeProcessVersionRevisionElement()
	{
	}



	void XMLImportFileProcessor::On_BeforeProcessEntityListElement()
	{
		MetaEntityPtrListItr	itrME;
		MetaEntityPtr					pME = NULL;


		// Do we want this entity?
		//
		for ( itrME  = m_pListME->begin();
					itrME != m_pListME->end();
					itrME++)
		{
			pME = *itrME;

			if (pME->GetName() == m_CurrentPath.back().GetAttribute("Name"))
				break;

			pME = NULL;
		}

		if (pME)
		{
			// Discard all meta entities preceeding the one wanted
			//
			while (m_pListME->front() != pME)
			{
				m_listDiscardME.push_back(m_pListME->front());
				m_pListME->pop_front();
			}

			m_pListME->pop_front();
			m_pCurrentME = pME;
			CollectKeyColumns();

			ReportInfo("XMLImportFileProcessor::On_BeforeProcessEntityListElement(): Begin importing %s.", pME->GetName().c_str());
			std::cout << "  " << pME->GetName() << std::string(40 - pME->GetName().size(), '.') << ":" << std::string(12, ' ');
		}
		else
		{
			// Check if this is an entity we discarded
			//
			for ( itrME  = m_listDiscardME.begin();
						itrME != m_listDiscardME.end();
						itrME++)
			{
				pME = *itrME;

				if (pME->GetName() == m_CurrentPath.back().GetAttribute("Name"))
				{
					// It has been discarded, now we process it only if there are no meta entities pending
					// which the found meta entity depends on
					//
					MetaEntityPtrListItr	itrMEPending;
					MetaEntityPtr					pMEPending;
					bool									bCanProcess = true;
					unsigned int					i;


					// Check if pME depends on any of the entities still in m_pListME
					//
					for ( itrMEPending  = m_pListME->begin();
								itrMEPending != m_pListME->end();
								itrMEPending++)
					{
						pMEPending = *itrMEPending;

						for ( i = 0; i < pME->GetParentMetaRelationCount(); i++)
						{
							if (pME->GetParentMetaRelation(i)->GetParentMetaKey()->GetMetaEntity() == pMEPending)
							{
								bCanProcess = false;
								break;
							}
						}

						if (!bCanProcess)
							break;
					}

					if (bCanProcess)
					{
						// Check if pME depends on any of the discarded entities
						//
						for ( itrMEPending  = m_listDiscardME.begin();
									itrMEPending != m_listDiscardME.end();
									itrMEPending++)
						{
							pMEPending = *itrMEPending;

							if (pME == pMEPending)
								continue;

							for ( i = 0; i < pME->GetParentMetaRelationCount(); i++)
							{
								if (pME->GetParentMetaRelation(i)->GetParentMetaKey()->GetMetaEntity() == pMEPending)
								{
									bCanProcess = false;
									break;
								}
							}

							if (!bCanProcess)
								break;
						}
					}

					if (!bCanProcess)
					{
						std::string			strMessage;

						strMessage += "XMLImportFileProcessor: The order of entities in the import file is inappropriate. Entity ";
						strMessage += pMEPending->GetName();
						strMessage += " must appear before entity ";
						strMessage += pME->GetName();
						strMessage += ".";

//						throw CSAXException(strMessage);

						// For now, a warning will do (since triggers are disabled during this operation, the order isn't important)
						ReportWarning("XMLImportFileProcessor::On_BeforeProcessEntityListElement(): %s.", strMessage.c_str());
					}

					m_listDiscardME.remove(pME);
					m_pCurrentME = pME;
					CollectKeyColumns();

					ReportInfo("XMLImportFileProcessor::On_BeforeProcessEntityListElement(): Begin importing %s.", pME->GetFullName().c_str());
					std::cout << "  " << pME->GetName() << std::string(40 - pME->GetName().size(), '.') << ":" << std::string(12, ' ');
					return;
				}
			}

			SkipToNextSibling();
		}
	}



	void XMLImportFileProcessor::On_BeforeProcessEntityElement()
	{
	}



	void XMLImportFileProcessor::On_BeforeProcessColumnElement()
	{
	}



	void XMLImportFileProcessor::On_AfterProcessRootElement()
	{
	}



	void XMLImportFileProcessor::On_AfterProcessDatabaseListElement()
	{
	}



	void XMLImportFileProcessor::On_AfterProcessDatabaseElement()
	{
	}



	void XMLImportFileProcessor::On_AfterProcessVersionMajorElement()
	{
		Convert(m_uVersionMajor, m_CurrentPath.back().m_strValue);
	}



	void XMLImportFileProcessor::On_AfterProcessVersionMinorElement()
	{
		Convert(m_uVersionMinor, m_CurrentPath.back().m_strValue);
	}



	void XMLImportFileProcessor::On_AfterProcessVersionRevisionElement()
	{
		std::string		strDBVersion = m_pDatabase->GetMetaDatabase()->GetVersion();
		char					szBuffer[128];
		std::string		strFileVersion;

		Convert(m_uVersionRevision, m_CurrentPath.back().m_strValue);

#ifdef AP3_OS_TARGET_WIN32
		_snprintf(szBuffer, 128, "V%i.%02i.%04i", m_uVersionMajor, m_uVersionMinor, m_uVersionRevision);
#else
		snprintf(szBuffer, 128, "V%i.%02i.%04i", m_uVersionMajor, m_uVersionMinor, m_uVersionRevision);
#endif
		strFileVersion = szBuffer;

		if (strDBVersion < strFileVersion)
			throw CSAXException(std::string("XMLImportFileProcessor: Version ") + strFileVersion + " of the import file is newer than the expected database version " + strDBVersion + ", can't process input file.");

		if (strDBVersion > strFileVersion)
		{
			std::cout << "WARNING: Database version " << strFileVersion << " of import file is older than the\n";
			std::cout << "         expected database version " << strDBVersion << ". This could cause problems.\n";
			std::cout << "         Please review the log when done!\n";

			ReportWarning("XMLImportFileProcessor: WARNING: Database version %s of import file is older than the expected database version %s.", strFileVersion.c_str(), strDBVersion.c_str());
		}
	}



	void XMLImportFileProcessor::On_AfterProcessEntityListElement()
	{
		m_lRecCountTotal += m_lRecCountCurrent;

		if (!m_bSkipToNextSibling)
		{
			std::cout << std::string(12, '\b') << std::setw(12) << m_lRecCountCurrent << std::endl;
			m_pLastME = m_pCurrentME;
			m_pCurrentME = NULL;

			ReportInfo("XMLImportFileProcessor::On_AfterProcessEntityListElement():  Imported %i records into %s.", m_lRecCountCurrent, m_pLastME->GetFullName().c_str());
		}

		m_lRecCountCurrent = 0;

		if (m_pListME->empty() && m_listDiscardME.empty())
			EndWithSuccess();
	}



	void XMLImportFileProcessor::On_AfterProcessEntityElement()
	{
		if (!m_bSkipToNextSibling)
		{
			m_lRecCountCurrent++;

			// Report every 100th record loaded
			if ((m_lRecCountCurrent % 100) == 0)
				std::cout << std::string(12, '\b') << std::setw(12) << m_lRecCountCurrent;
		}
	}



	void XMLImportFileProcessor::On_AfterProcessColumnElement()
	{
	}



	// Populates member m_setUKColumns with the names of all columns which take part in a unique key
	// (this is used to skip and report duplicates)
	void XMLImportFileProcessor::CollectKeyColumns()
	{
		MetaKeyPtrVect::iterator			itrMK;
		MetaKeyPtr										pMK;
		MetaColumnPtrList::iterator		itrMC;
		MetaColumnPtr									pMC;


		m_setUKColumns.clear();

		if (m_pCurrentME)
		{
			for ( itrMK =  m_pCurrentME->GetMetaKeys()->begin();
						itrMK != m_pCurrentME->GetMetaKeys()->end();
						itrMK++)
			{
				pMK = *itrMK;

				if (pMK->IsUnique())
				{
					for ( itrMC =  pMK->GetMetaColumns()->begin();
								itrMC != pMK->GetMetaColumns()->end();
								itrMC++)
					{
						pMC = *itrMC;
						m_setUKColumns.insert(pMC->GetName());
					}
				}
			}
		}
	}


}
