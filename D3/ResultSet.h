#ifndef _D3_ResultSet_H_
#define _D3_ResultSet_H_

#include <boost/thread/recursive_mutex.hpp>

// All temporary SQLServer stored procedures this method creates begin with this name
#define APAL_SQLSERVER_TSP_PREFIX		"#sp_rslts_"
#define APAL_DEFAULT_PAGE_SIZE			20

namespace D3
{
	//! The purpose of the ResultSet class is to allow clients to fetch and iterate over a set of objects of a particular type
	/*! This is an abstract class. The sole purpose is to provide a common interface to more
			specific instances. In particular, the Initialise() method needs to be implemented by
			subclasses.
	*/
	class D3_API ResultSet : public Object
	{
		public:
			class iterator;

		protected:
			friend class Database;
			friend class Entity;
			friend class iterator;

			// Standard D3 stuff
			//
			D3_CLASS_DECL_PV(ResultSet);

		protected:
			unsigned long						m_lID;												//!< This' unique identifier within the scope of m_pDatabase
			DatabasePtr							m_pDatabase;									//!< The resultset holds objects from this database
			MetaEntityPtr						m_pMetaEntity;								//!< This ResultSet includes objects of this MetaEntity type
			bool										m_bKeepObjects;								//!< If true, objects fetched remain in the database, else they are discarded when navigating to a new page or closing the result set
			bool										m_bInitialised;								//!< Must be true before any navigation methods can be called
			EntityPtrListPtr				m_pListEntity;								//!< If m_bHasResult=true, contains the entities in the current page
			unsigned long						m_uTotalSize;									//!< The total number of records in the set
			unsigned long						m_uPageSize;									//!< The size of each page
			unsigned long						m_uCurrentPage;								//!< The page currently available in m_pListEntity
			unsigned long						m_uFirst;											//!< The ordinal number for the first record in the page
			unsigned long						m_uLast;											//!< The ordinal number for the first record in the page
			std::string							m_strJSONKey;									//!< If this has been created using the JSONKey Create method, this member will store the original key


			//! Default constructor
			ResultSet(DatabasePtr pDB = NULL, MetaEntityPtr pME = NULL);

		public:
			/** @name Iteration
					This class supports iteration. This means that you can treat a ResultSet
					object similar to a std::list<D3::Entity*> collection.

					This group of methods simply exist to mimic stl like behaviour.
			*/
			//@{
			//! Allow ResultSet::iterator to iterate records in the current page
			class D3_API iterator : public EntityPtrListItr
			{
				protected:
					ResultSetPtr			m_pResultSet;			//!< for dummy iterators this is NULL

				public:
					//! The default cunstructor returns an iterator that is considered to be at the end of the collection
					iterator() : m_pResultSet(NULL)	{}

					//! This constructor passes all work to the stl iterator
					iterator(const EntityPtrListItr& itr, ResultSetPtr pResultSet) : EntityPtrListItr(itr), m_pResultSet(pResultSet) {}

					iterator&		operator=(const iterator & itr)
					{
						m_pResultSet = itr.m_pResultSet;

						((EntityPtrListItr*) this)->operator=(itr);

						return *this;
					}

					bool				operator!=(const iterator & itr)
					{
						if (!m_pResultSet && !itr.m_pResultSet)
							return false;

						return (EntityPtrListItr&) *this != (const EntityPtrListItr&) itr;
					}


					bool				operator==(const iterator & itr)
					{
						if (!m_pResultSet && !itr.m_pResultSet)
							return true;

						return (EntityPtrListItr&) *this == (const EntityPtrListItr&) itr;
					}
			};

			//! This method creates the ResultSet
			/*! This method creates the ResultSet

					@pDatabase Any objects that are made resident will end up inside this database

					@pME		The MetaEntity who's instances we want to access

					@strSQLWHEREClause The WHERE clause without the keyword WHERE that will be used to filter the ResultSet (can be empty)

					@strSQLORDERBYClause The ORDER BY clause without the keyword ORDER BY that will be used to sort the ResultSet (can be empty)

					@uPageSize The number of records that each page should contain.

					If successful, this will define everything that is needed to navigate the result set
					However, nothing is loaded at that stage so you call GetPage() to load get the first page
					of objects.

					/note Specific implementations are expected to throw an Exception if an error occurs
			*/
			static	ResultSetPtr		Create(	DatabasePtr					pDatabase,
																			MetaEntityPtr				pME,
																			const std::string&	strSQLWHEREClause,
																			const std::string	& strSQLORDERBYClause,
																			unsigned long				uPageSize);

			//! This method creates the ResultSet to navigate trough children in a relation
			/*! This method creates the ResultSet to navigate trough children in a relation

					@pDatabase Any objects that are made resident will end up inside this database

					@pRelation A relation who's children this will navigate

					Optionally, you can provide a WHERE predicate and an ORDER BY clause. This allows you
					to ensure the result set only spans a subset of the children and allows you to determine
					the order in which they are returned.

					If successful, this will define everything that is needed to navigate the result set
					However, nothing is loaded at that stage so you call GetPage() to load get the first page
					of objects.

					/note Specific implementations are expected to throw an Exception if an error occurs
			*/
			static	ResultSetPtr		Create(	DatabasePtr					pDatabase,
																			RelationPtr					pRelation,
																			unsigned long				uPageSize,
																			const std::string*	pstrSQLWHEREClause = NULL,
																			const std::string*	pstrSQLORDERBYClause = NULL);

			//! This method creates the ResultSet from a JSON Key
			/*! This method creates the ResultSet from a JSON Key

					@pDBWS Any objects that are made resident will end up inside this database workspace

					@strJSONKey A key in JSON format, e.g.: {"KeyID":44,"Columns":['A',0]}

					If successful, this will define everything that is needed to navigate the result set
					However, nothing is loaded at that stage so you call GetPage() to load get the first page
					of objects.

					/note This method ignores columns with a value of NULL. If the column is a String type,
					the search will look for all objects which start with the given characters (e.g. if the
					string value given is 'A', the serach will be for LIKE 'A%')

					/note You can re-use these result sets. Once you have created such a result set,
					you can refresh it by using the Refresh method and passing it a
			*/
			static	ResultSetPtr		Create(	DatabaseWorkspacePtr	pDBWS,
																			const std::string&		strJSONKey,
																			unsigned long					uPageSize);

			//! The destructor deletes At this level, deletes the current page (overload if this is not appropriate)
			virtual ~ResultSet();

			//@{
			//! This set of methods in combination with ReseltSet::iterator allow clients to access details about the current page in a stdc++ like manner
			/*! Use these methdos in combination with the ResultSet::iterator to navigate the objects
					in the current page. Pages numbers are base 0. Changing the page will invalidate any iterators,
					so make sure you reset iterators by calling resultSet.begin().

					The safest way to iterate of a RecordSet is as follows
					 /code
					 // Assume ResultSet rs exists
					 unsigned int uPageNo = 0;
					 ResultSet::iterator	itr;
					 EntityPtr						pObject;

					 while (rs.GetPage(uPageNo)
					 {
						 for ( itr =  rs.begin();
									 itr != rs.end();
									 itr++)
						 {
							 pObject = *itr;
							 // work with object here
						 }
						 uPage++;
					 }
					 /endcode
			*/
			iterator									begin()										{ return m_pListEntity ? iterator(m_pListEntity->begin(), this) : iterator(); }
			iterator									end()											{ return m_pListEntity ? iterator(m_pListEntity->end(), this) : iterator(); }
			bool											empty()										{ return m_pListEntity ? m_pListEntity->empty() : true; }
			EntityPtrList::size_type	size()										{ return m_pListEntity ? m_pListEntity->size() : 0; }

			virtual bool							GetPage(unsigned long uPageNo, unsigned long uPageSize) = 0;
			virtual bool							GetNextPage() = 0;
			virtual bool							GetPreviousPage() = 0;
			virtual bool							GetFirstPage() = 0;
			virtual bool							GetLastPage() = 0;
			//@}

			//! Return this JSONKey. This member is only specified if this has been created using the Creat method that takes a strJSONKey as input
			const std::string&				GetOriginalJSONKey()			{ return m_strJSONKey; }

			//! Return this's ID. This ID can be used later to retrieve this from the Database to which it belongs.
			unsigned long							GetID()										{ return m_lID; }

			//! This method indicates whether or not objects are kept or released after disposing a page (see also KeepObjects(bool))
			bool											KeepObjects()							{ return m_bKeepObjects; }

			//! Use this method to indicate whether or not objects should be kept or released
			/*! The default is that resultsets release all objects that are made resident for a page
					when you move from one page to the next. You can use this method to set KeppObjects to
					true in which case objects contained in the current page are not removed form memory
					when navigating to another page.

					/note ResultSets contaning cached objects always match KeepObject()==true
			*/
			void											SetKeepObjects(bool bKeepObjects);

			//! Allows the client to change the size of a page
			virtual void							SetPageSize(unsigned long uPageSize)		{ m_uPageSize = uPageSize  > 0 ? uPageSize : APAL_DEFAULT_PAGE_SIZE; }

			//@{
			//! Get details about the ResultSet
			/*! These details are meaningless if empty() returns true.
					Some subclass might not be able to return all this data
					accurately. Consult the relevant documentation.
			*/
			virtual unsigned long			GetPageSize()							{ return m_uPageSize; }
			virtual unsigned long			GetCurrentPageNo()				{ return m_uCurrentPage; }
			virtual unsigned long			GetNoFirstInPage()				{ return m_uFirst; }
			virtual unsigned long			GetNoLastInPage()					{ return m_uLast; }
			virtual unsigned long			GetTotalRecords()					{ return m_uTotalSize; }
			virtual unsigned long			GetNumberOfPages()				{ return m_uPageSize > 0 && m_uTotalSize > 0 ? (m_uTotalSize + (m_uPageSize - 1)) / m_uPageSize : 0; }
			//@}

			//! Returns the database to which owns this
			DatabasePtr								GetDatabase()							{ return m_pDatabase; }

			//! Returns the meta entity who's instances this comprises
			MetaEntityPtr							GetMetaEntity()						{ return m_pMetaEntity; }

			//! Returns column definitions as JSON
			std::ostream &						InfoAsJSON(std::ostream & ostrm);

			//! Returns the data in the current page as JSON
			std::ostream &						DataAsJSON(std::ostream & ostrm);

			//! Returns the session this result set is associated with or NULL if it isn't associated with a session
			SessionPtr								GetSession()							{ return m_pDatabase ? m_pDatabase->GetDatabaseWorkspace()->GetSession() : NULL; }

	protected:
			//! If m_bKeepObjects is false deletes all objects in the current page otherwise deletes only the collection
			virtual void							DeleteObjectList();

			//! This method populates the m_pListEntity from the pListEntity passed in
			virtual void							CreateObjectList(EntityPtrListPtr pListEntity);

			//! Notification an entity that is a member of this sends before it's death
			virtual void							On_EntityDeleted(EntityPtr pEntity);
	};









	//! The purpose of the OraclePagedResultSet class is to allow clients to fetch and iterate over a set of objects of a particular type from an orcale database page by page
	class D3_API OraclePagedResultSet : public ResultSet
	{
		// Standard D3 stuff
		//
		D3_CLASS_DECL(OraclePagedResultSet);

		protected:
			std::string							m_strSQLCountQuery;						//! The query that returns the total number in the result set
			std::string							m_strSQLBaseQuery;						//! The base query that ruturns a particular set of records by appending a string containing "#FirstRecordRequested AND #LastRecordRequested"

			//! Protected constructor only needed for D3 class factory
			OraclePagedResultSet(DatabasePtr pDB = NULL, MetaEntityPtr pME = NULL)
				:	ResultSet(pDB, pME)
			{}

		public:
			//! This method creates the OraclePagedResultSet
			/*! This method creates the OraclePagedResultSet

					@pDatabase Any objects that are made resident will end up inside this database

					@pME		The MetaEntity who's instances we want to access

					@strSQLWHEREClause The WHERE clause without the keyword WHERE that will be used to filter the ResultSet (can be empty)

					@strSQLORDERBYClause The ORDER BY clause without the keyword ORDER BY that will be used to sort the ResultSet (can be empty)

					@uPageSize The number of records that each page should contain.

					If successful, this will define everything that is needed to navigate the result set
					However, nothing is loaded at that stage so you call GetPage() to load get the first page
					of objects.

					/note Specific implementations are expected to throw an Exception if an error occurs
			*/
			static	ResultSetPtr		Create(	DatabasePtr					pDatabase,
																			MetaEntityPtr				pME,
																			const std::string&	strSQLWHEREClause,
																			const std::string	& strSQLORDERBYClause,
																			unsigned long				uPageSize);

			//! Fetches the Page and populates the internal collection with the objects from that page
			/*! \note: All Page numbers are base 0
			*/
			bool										GetPage(unsigned long uPageNo, unsigned long uPageSize);

			//! Fetches the next Page and populates the internal collection with the objects from that page
			bool										GetNextPage()									{ return GetPage(m_uCurrentPage + 1, m_uPageSize);	}

			//! Fetches the previous Page and populates the internal collection with the objects from that page
			bool										GetPreviousPage()							{ return GetPage(m_uCurrentPage - 1, m_uPageSize);	}

			//! Fetches the first Page and populates the internal collection with the objects from that page
			bool										GetFirstPage()								{ return GetPage(0, m_uPageSize);	}

			//! Fetches the first Page and populates the internal collection with the objects from that page
			bool										GetLastPage()									{ return GetPage(GetNumberOfPages(), m_uPageSize); }

		protected:
			//! Build the query strings and stores these in m_strSQLCountQuery and m_strSQLBaseQuery
			void										BuildQueryStrings(const std::string&	strSQLWHEREClause, const std::string&	strSQLORDERBYClause);

			//! Builds a comma separated list of the columns comprising the primary key
			std::string							SetKeyColsSQLQueryPart();

			//! Builds a comma separated list of the columns comprising the entity
			std::string							SetEntityColsSQLQueryPart();

	};








	//! The purpose of the SQLServerPagedResultSet class is to allow clients to fetch and iterate over a set of objects of a particular type in sets of objects
	/*!	This class actually builds a stored procedure when Initialise is called and calls this procedure when calls to GetPage() are made. The procedure is destroyed
			with the ResultSet or with the next call to Initialise().
	*/
	class D3_API SQLServerPagedResultSet : public ResultSet
	{
		friend class ODBCDatabase;

		// Standard D3 stuff
		//
		D3_CLASS_DECL(SQLServerPagedResultSet);

			//! Protected constructor only needed for D3 class factory (use SQLServerPagedResultSet::Create() to create an instance of this class)
			SQLServerPagedResultSet(DatabasePtr pDB = NULL, MetaEntityPtr pME = NULL)
				:	ResultSet(pDB, pME)
			{}

		protected:
			static boost::recursive_mutex	M_mtxExclusive;	//!< Used to guarantee unique procedure names

			DatabaseWorkspace							m_dbWS;					//!< This class uses a temporary procedure which will reside inside this database workspace
			std::string										m_strProcName;	//!< The name of the stored procedure

		public:
			virtual ~SQLServerPagedResultSet();

			//! This method creates the SQLServerPagedResultSet
			/*! This method will create and register a stored procedure and can
					potentially throw exceptions (std::bad_alloc, D3::Exception and odbc::SQLException)
			*/
			static	ResultSetPtr		Create(	DatabasePtr					pDatabase,
																			MetaEntityPtr				pME,
																			const std::string&	strSQLWHEREClause,
																			const std::string	& strSQLORDERBYClause,
																			unsigned long				uPageSize);

			//! Fetches the Page and populates the internal collection with the objects from that page
			/*! \note: All Page numbers are base 0
			*/
			bool										GetPage(unsigned long uPageNo, unsigned long uPageSize);

			//! Fetches the next Page and populates the internal collection with the objects from that page
			bool										GetNextPage()									{ return GetPage(m_uCurrentPage + 1, m_uPageSize);	}

			//! Fetches the previous Page and populates the internal collection with the objects from that page
			bool										GetPreviousPage()							{ return GetPage(m_uCurrentPage - 1, m_uPageSize);	}

			//! Fetches the first Page and populates the internal collection with the objects from that page
			bool										GetFirstPage()								{ return GetPage(0, m_uPageSize);	}

			//! Fetches the first Page and populates the internal collection with the objects from that page
			bool										GetLastPage()									{ return GetPage(GetNumberOfPages(), m_uPageSize); }

		protected:

			//! Builds an sql string that can be used to create a stored procedure
			std::string							BuildProcedure(const std::string & strWhereClause, const std::string & strOrderByClause);

			//! Creates the stored procedure
			void										CreateProcedure(const std::string & strWhereClause, const std::string & strOrderByClause);

			//! Drop the stored procedure
			void										DropProcedure();

			//! Returns the connection that owns this temporary procedure
			DatabasePtr							GetStoredProcedureDatabase();

			//! Builds and returns this stored procedure name
			const std::string &			GetStoredProcedureName();

	};
}

#endif /* _D3_ResultSet_H_ */
