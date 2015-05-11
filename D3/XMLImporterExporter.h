#ifndef _XMLImporterExporter_h_
#define _XMLImporterExporter_h_

#include "SAXParser.h"
#include "SAX/XMLReader.h"
#include "SAX/HandlerBase.h"
#include "SAX/Configurable.h"
#include "SAX/EntityResolver.h"
#include "SAX/LexicalHandler.h"
#include "SAX/NamespaceHandler.h"
#include "SAX/AttributeList.h"
#include "SAX/AttributeListImp.h"
#include "SAX/SAXException.h"
#include "SAX/EntityResolverImp.h"
#include "SAX/Locator.h"
#include "XMLUtils.h"
#include "XMLException.h"

using namespace CSL::XML;

namespace D3
{
	class XMLImportFileProcessor: public CHandlerBase, public CLexicalHandler, public CNamespaceHandler
	{
		public:
			typedef std::map<std::string, std::string>			AttributeList;
			typedef AttributeList::iterator									AttributeListItr;

			class Element
			{
				public:
					enum Type
					{
						None,
						RootElement,
						DatabaseListElement,
						DatabaseElement,
						VersionMajorElement,
						VersionMinorElement,
						VersionRevisionElement,
						EntityListElement,
						EntityElement,
						ColumnElement
					};

					std::string			m_strName;
					AttributeList		m_listAttributes;
					std::string			m_strValue;
					Type						m_eType;

					Element() : m_eType(None) {}

					void						SetAttributes(const CAttributeList& attrList);
					void						Clear();
					std::string			GetAttribute(const std::string & strName);
					void						Dump(int iIndent = 0);
			};

			class ElementStack : public std::list<Element>
			{
				public:
					ElementStack() {}

					unsigned int		GetLevel()																								{ return size(); }
					std::string			GetPath();
					Element::Type		GetLastElementType()																			{ if (empty()) return Element::None; return back().m_eType; }

					bool						IsParentOf(const ElementStack& aElementStack) const;
					bool						IsChildOf(const ElementStack& aElementStack) const				{ return aElementStack.IsParentOf(*this); }
					bool						IsSiblingOf(const ElementStack& aElementStack) const;
					bool						IsDistantParentOf(const ElementStack& aElementStack) const;
					bool						IsDistantChildOf(const ElementStack& aElementStack) const	{ return aElementStack.IsDistantParentOf(*this); }
			};

			typedef	ElementStack::iterator	ElementStackItr;

		private:
			const CLocator*							m_pLocator;

		protected:
			enum ParseState
			{
				ExpectRootElement,
				ExpectDatabaseListElement,
				ExpectDatabaseElement,
				ExpectVersionMajorElement,
				ExpectVersionMinorElement,
				ExpectVersionRevisionElement,
				ExpectEntityListElement,
				ExpectEntityElement,
				ExpectColumnElement
			};

			std::string									m_strAppName;						// The name of the application (needed to verify XML content)
			D3::DatabasePtr							m_pDatabase;						// The database into which objects should be imported
			D3::MetaEntityPtrListPtr		m_pListME;							// The list of meta entities of which all objects should be imported
			D3::MetaEntityPtrList				m_listDiscardME;				// The list of requested meta entities which were not imported
			D3::MetaEntityPtr						m_pCurrentME;						// The meta entity that is currently being imported
			D3::MetaEntityPtr						m_pLastME;							// The last meta entity that was successfully imported
			ParseState									m_eParseState;
			bool												m_bSkipToNextSibling;		// Use SkipToNextSibling() to make the parser ignore elements from the current element until the start of the next sibling
			bool												m_bDatabaseFound;				// True if the database element for the requested database was found in the XML file
			int													m_iElementCount;
			ElementStack								m_CurrentPath;
			ElementStack								m_MarkedPath;
			ElementStack								m_CurrentEntityPath;
			bool												m_bProcessingElement;
			bool												m_bDebug;								// If true, print all non-skipped elements to cout
			long												m_lRecCountCurrent;
			long												m_lRecCountTotal;
			bool												m_bDocumentEnded;
			std::set<std::string>				m_setUKColumns;					// Is initialised during On_BeforeProcessEntityElement with all column names which are part of a unique key
			unsigned int								m_uVersionMajor;				// Major version from import file
			unsigned int								m_uVersionMinor;				// Minor version from import file
			unsigned int								m_uVersionRevision;			// Revision version from import file


			XMLImportFileProcessor(const std::string & strAppName, DatabasePtr pDB, MetaEntityPtrListPtr pListME)
			:	m_strAppName(strAppName),
				m_pLocator(NULL),
				m_pDatabase(pDB),
				m_pListME(pListME),
				m_iElementCount(0),
				m_pCurrentME(NULL),
				m_pLastME(NULL),
				m_eParseState(ExpectRootElement),
				m_bSkipToNextSibling(false),
				m_bDatabaseFound(false),
				m_bProcessingElement(false),
				m_bDebug(false),
				m_lRecCountCurrent(0),
				m_lRecCountTotal(0),
				m_bDocumentEnded(false),
				m_uVersionMajor(0),
				m_uVersionMinor(0),
				m_uVersionRevision(0)
			{}

			// CHandlerBase
			void SetDocumentLocator(const CLocator& loc) { m_pLocator = &loc; }

			void StartDocument();
			void EndDocument();
			void StartElement(const std::string& name, const CAttributeList& attrList);
			void EndElement(const std::string& name);
			void Characters(const XMLChar ch[], int start, int length);
			void Warning(CSAXException& e);
			void Error(CSAXException& e);
			void FatalError(CSAXException& e);

			// CLexicalHandler
			void StartDTD(const std::string& name, const std::string& publicId, const std::string& systemId) {}
			void EndDTD() {}
			void StartEntity(const std::string& name) {}
			void EndEntity(const std::string& name) {}
			void StartCDATA() {}
			void EndCDATA() {}
			void Comment(const XMLChar ch[], int start, int length) {}

			// CNamespaceHandler
			void StartNamespaceDeclScope(const std::string& prefix, const std::string& uri) {}
			void EndNamespaceDeclScope(const std::string& prefix) {}

			// Parsing helpers
			//
			ElementStack		GetMark()														{	return m_CurrentPath;	}
			void						Mark()															{	m_MarkedPath = m_CurrentPath;	}

			unsigned int		GetCurrentLevel()										{ return m_CurrentPath.GetLevel(); }
			std::string			GetCurrentPath()										{ return m_CurrentPath.GetPath();	}
			unsigned int		GetMarkLevel()											{ return m_MarkedPath.GetLevel(); }
			std::string			GetMarkPath()												{ return m_MarkedPath.GetPath();	}
			void						SkipToNextSibling();

			void						CollectKeyColumns();

			// Notifications
			//
			virtual void		On_BeforeProcessRootElement();
			virtual void		On_BeforeProcessDatabaseListElement();
			virtual void		On_BeforeProcessDatabaseElement();
			virtual void		On_BeforeProcessVersionMajorElement();
			virtual void		On_BeforeProcessVersionMinorElement();
			virtual void		On_BeforeProcessVersionRevisionElement();
			virtual void		On_BeforeProcessEntityListElement();
			virtual void		On_BeforeProcessEntityElement();
			virtual void		On_BeforeProcessColumnElement();

			virtual void		On_AfterProcessRootElement();
			virtual void		On_AfterProcessDatabaseListElement();
			virtual void		On_AfterProcessDatabaseElement();
			virtual void		On_AfterProcessVersionMajorElement();
			virtual void		On_AfterProcessVersionMinorElement();
			virtual void		On_AfterProcessVersionRevisionElement();
			virtual void		On_AfterProcessEntityListElement();
			virtual void		On_AfterProcessEntityElement();
			virtual void		On_AfterProcessColumnElement();

		public:
			long						GetTotalRecordCount()								{ return m_lRecCountTotal; }

			virtual void		EndWithSuccess()										{ EndDocument(); }
	};
}

#endif /* _XMLImportFileProcessor_h_ */

