#ifndef INC_D3_ODBCDATABASE_H
#define INC_D3_ODBCDATABASE_H

// MODULE: ODBCDatabase header
//
// NOTE: This module implements ODBC specific ako Database
//;
// ===========================================================
// File Info:
//
// $Author: pete $
// $Date: 2014/10/10 22:23:50 $
// $Revision: 1.19 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 03/09/03 - R2 - Hugp
//
// Created
//
// -----------------------------------------------------------
#ifdef APAL_SUPPORT_ODBC				// If not defind, skip entire file

#define IN_ODBCXX

#ifdef _DEBUG
#define ODBCXX_DEBUG
#endif

#include "D3.h"
#include "Database.h"
#include "D3Funcs.h"

// Include ODBC stuff
//
#include <odbc++/drivermanager.h>
#include <odbc++/connection.h>
#include <odbc++/preparedstatement.h>
#include <odbc++/resultset.h>
#include <odbc++/databasemetadata.h>
#include <odbc++/resultsetmetadata.h>
#include <odbc++/callablestatement.h>

namespace D3
{
	//! The ODBCDatabase class interfaces with the physical datastore through Manush Dodunekov's libodbcxx library.
	/*! This class implements access to the physical store through Manush Dodunekov's libodbcxx library.
			Details and the latest source code can be found at <A HREF="http://libodbcxx.sourceforge.net">Sourceforge</A>.

			Currently, this class supports Oracle and SQL Server only. An ODBC driver is
			required in both cases. The class has been tested under Windows, Linux and HP-UX.

			There is a potential to enhance this class with support for other RDBMS, but
			this necessitates changing built in mechanisms to maintain referential integrity
			to D3 as well as other outside clients. These mechanism currently only work
			reliably for Oracle and SQLServer.
	*/
	class D3_API ODBCDatabase : public Database
	{
		friend class MetaDatabase;
		friend class ODBCXMLImportFileProcessor;
		friend class SQLServerPagedResultSet;
		friend class DatabaseAlertManager;
		friend class ConnectionManager;

		D3_CLASS_DECL(ODBCDatabase);

		protected:
			// Connection Pooling >>>>>>>>>>>>>>>>
			// We pool connections
			#define ODBC_DATABASE_MAX_POOLSIZE				20

			// The native ODBC connection pool is used for native ODBC calls
			// Native ODBC calls are necessary in cases where odbc++ doesn't
			// work.
			struct NativeConnection
			{
				SQLHANDLE				hEnv;
				SQLHANDLE				hCon;
			};

			typedef odbc::Connection*																		ODBCConnectionPtr;
			typedef std::list<ODBCConnectionPtr>												ODBCConnectionPtrList;
			typedef ODBCConnectionPtrList::iterator											ODBCConnectionPtrListItr;
			typedef std::map<MetaDatabasePtr, ODBCConnectionPtrList>		ODBCConnectionPtrListMap;
			typedef ODBCConnectionPtrListMap::iterator									ODBCConnectionPtrListMapItr;

			typedef NativeConnection*																		NativeConnectionPtr;
			typedef std::list<NativeConnectionPtr>											NativeConnectionPtrList;
			typedef NativeConnectionPtrList::iterator										NativeConnectionPtrListItr;
			typedef std::map<MetaDatabasePtr, NativeConnectionPtrList>	NativeConnectionPtrListMap;
			typedef NativeConnectionPtrListMap::iterator								NativeConnectionPtrListMapItr;

			static boost::recursive_mutex					M_Mutex;													// Protects access to connection pools
			static boost::recursive_mutex					M_CreateSPMutex;									// Prevents concurrent access to CreateProcedure
			static ODBCConnectionPtrListMap				M_mapODBCConnectionPtrLists;			// ODBC Connection Pool
			static NativeConnectionPtrListMap			M_mapNativeConnectionPtrLists;		// Native Connection Pool
			static bool														M_bQNInitialised;									// If true, any obsolete Query Notification Services and Queues have already been removed during startup

			static ODBCConnectionPtr							CreateODBCConnection(MetaDatabasePtr pMDB);
			static void														ReleaseODBCConnection(MetaDatabasePtr pMDB, ODBCConnectionPtr & pODBCConnection);

			static NativeConnectionPtr						CreateNativeConnection(MetaDatabasePtr pMDB);
			static void														ReleaseNativeConnection(MetaDatabasePtr pMDB, NativeConnectionPtr & pNativeConnection);

			static void														DeleteConnectionPools();
			// <<<<<<<<<<<<<<<<<< Connection Pooling

			bool																	m_bIsConnected;							// If true we have a valid connection
			ODBCConnectionPtr											m_pConnection;							// ODBC connection
			unsigned int													m_uConnectionBusyCount;			// >0 if the connection is currently processing result sets

			ODBCDatabase();
			virtual ~ODBCDatabase();

		public:
			virtual bool							Disconnect();
			virtual bool							Connect();
			virtual bool							IsConnected()
			{
				return m_pConnection != NULL && m_bIsConnected;
			}

			//! Start a transaction.
			virtual bool							BeginTransaction();
			//! Make all updates to the database performed since the preceeding call to BeginTransaction() permanent.
			virtual bool							CommitTransaction();
			//! Undo all updates to the database performed since the preceeding call to BeginTransaction().
			virtual bool							RollbackTransaction();
			//! Return true if this has a pending transaction.
			virtual bool							HasTransaction();

			static	void							UnInitialise();

			//! Load the specified column from the specified entity (primarily used for LazyFetch columns).
			/*! @param	pColumn		The instance column to refresh.

					\note	This method always incurs a database roundtrip.
					Pre-requisites:
					- pColumn->GetEntity()->IsNew() must return false
			*/
			virtual ColumnPtr					LoadColumn(ColumnPtr pColumn);

			//! Load the object matching the specified key
			/*! @param	pKey			The key to search for. This can be an InstanceKey or
														a temporary key. The object returned will be an instance
														of the same MetaEntity as the keys MetaKey's MetaEntity.

					@param	bRefresh	This parameter has significance only if the object is
														already memory resident. If bRefresh is true, the memory
														resident copy is updated with the latest values from
														the physical store. If bRefresh is false, the memory
														resident copy will be returned without accessing the
														physical store.

					@return	EntityPtr	Pointer to the Entity object found or NULL if no match
														exists.

					\note	This method will always return NULL if the key passed in is
					not a UNIQUE key (i.e. pKey->GetMetaKey()->IsUnique() must be true).
			*/
			virtual EntityPtr					LoadObject(KeyPtr pKey, bool bRefresh = false, bool bLazyFetch = true);

			//! Loads all children from a relationship
			/*! @param pRelation	The Relation who's children need to be loaded
					@param	bRefresh	This parameter has significance only if the object is
														already memory resident. If bRefresh is true, the memory
														resident copy is updated with the latest values from
														the physical store. If bRefresh is false, the memory
														resident copy will be returned without accessing the
														physical store.

					@return long			The method returns the number of objects loaded or
														a negative value if the operation failed.
			*/
			virtual long							LoadObjects(RelationPtr pRelation, bool bRefresh = false, bool bLazyFetch = true);

			//! Loads objects where the values of the instance of the MetaKey passed matches the values of the key passed in.
			/*! This method checks the cache first and if found, returns the cached key.
			/*! @param pMetaKey		The MetaKey we lookup.
					@param pKey				The Key values.
					@param	bRefresh	This parameter has significance only if the object is
														already memory resident. If bRefresh is true, the memory
														resident copy is updated with the latest values from
														the physical store. If bRefresh is false, the memory
														resident copy will be returned without accessing the
														physical store.

					@return long			The method returns the number of objects loaded or
														a negative value if the operation failed.
			*/
			long											LoadObjects(MetaKeyPtr pMetaKey, KeyPtr pKey, bool bRefresh = false, bool bLazyFetch = true);

			//! Load all instances of the specified MetaEntity
			/*!	@param	pMetaEntity A pointer to a meta entity object which identifies
															the instances that should be returned

					@param	bRefresh	This parameter has significance only if an object is
														already memory resident. If bRefresh is true, the memory
														resident copy is updated with the latest values from
														the physical store. If bRefresh is false, the memory
														resident copy will be left as is.

					@return long			The method returns the number of objects loaded or
														a negative value if the operation failed.
			*/
			virtual long							LoadObjects(MetaEntityPtr pMetaEntity, bool bRefresh = false, bool bLazyFetch = true);

			//! Make objects of a specific type resident
			/*! This method allows you to specify a query string of any complexity. The only
					rules that you need to observe are:

					- The query must return all rows from the MetaEntity specified

					- These rows must be at the start of each row in the returned resultset

					- They must exactly match the order in which they are specified in
					  the MetaEntity.

					@param	pMetaEntity A pointer to a meta entity object which describes
															the instances the query is expected to return.

					@param	strSQL			The SQL statement to execute.

					@param	bRefresh		If true, the method refreshes objects which are already
															memory resident. If false, it skips details of memory
															resident objects (this is the default).

					@return EntityPtrListPtr	The method returns an stl list containing pointers
																		to the Entity objects the query returned. The list
																		reflects the objects in the sequence determined by the
																		query.

					\note You must free the returned collection when no longer needed.
			*/
			virtual EntityPtrListPtr	LoadObjects(MetaEntityPtr pMetaEntity, const std::string & strSQL, bool bRefresh = false, bool bLazyFetch = true)	{ return LoadObjects(pMetaEntity, NULL, strSQL, bRefresh, bLazyFetch); }

			//! Loads objects by calling the stored procedure specified
			/*! This method allows you to call a stored procedure to retrieve multiple records from multiple resultsets in a single call.

					@param	listME			A list containig pointers to D3::MetaEntity objects to be loaded.
															The order must match the order of results returned by the specified
															stored procedure.
															If no records are found for a given MetaEntity, the stored procedure
															must return an empty resultset.
															Each MetaEntity must belong to the same MetaDatabase this belongs to.

					@param	pfnCreateProc A function pointer to a function with the prototype:
															void CreateProc(DatabasePtr pDB)
															This function will be called if the first call to invoke the procedure
															fails with "procedure does not exist". This will be passed in as the
															database. The procedure should simply call this->CreateStoredProcedure()
															without catching any errors. If CreateStoredProcedure throws, this method
															passed the exception on to the callee. If not, it will retry calling
															the stored procedure, but this time pass on any exceptions thrown.

					@param	strProcName	The name of the stored procedure.

					@param	strProcArgs The arguments to be passed to the stored procedure

					@param	bRefresh		If true, the method refreshes objects which are already
															memory resident. If false, it skips details of memory
															resident objects (this is the default).

					@param	bLazyFetch	If true, BLOB's will not be fetched

					\note This method may throw
			*/
			virtual void							LoadObjectsThroughStoredProcedure(MetaEntityPtrList & listME, CreateStoredProcedureFunc, const std::string & strProcName, const std::string & strProcArgs, bool bRefresh = false, bool bLazyFetch = true);

			//! Create stored procedure
			/*! This method must be overloaded by none abstract classes based on this.
					The method simply creates the stored procedure requested if it doesn't exist.
			*/
			virtual void							CreateStoredProcedure(const std::string & strSPBody);

			//! Executes the stored procedure and returns the resultsets in the form of a json structure (see [D3::Database::ExecuteStoredProcedureAsJSON] for more details)
			virtual void							ExecuteStoredProcedureAsJSON(std::ostringstream& ostrm, CreateStoredProcedureFunc, const std::string & strProcName, const std::string & strProcArgs);

			//! ExecuteSingletonSQLCommand allows you to execute an SQL Statement which returns a single value
			/*! This method is expected to execute a query that simply returns a single resultset with
					a single row and a single column. A classic example would be a query that uses an aggregate function
					to retrieve a single value from the database such as:

						SELECT AVG(weight) FROM Product;
						SELECT COUNT(*) FROM Product;

					etc.
			*/
			virtual void							ExecuteSingletonSQLCommand(const std::string & strSQL, unsigned long & lResult);

			//! ExecuteQueryAsJSON executes an arbitrary query and writes the results to the stream passed in as JSON
			/*! The result sets are written to oResultSets in the form:
					[
						[
							{"col1":val1,"col2":val2,...,"coln":valn]},
							{"col1":val1,"col2":val2,...,"coln":valn]},
							...
						],
						[
							{"col1":val1,"col2":val2,...,"coln":valn]},
							{"col1":val1,"col2":val2,...,"coln":valn]},
							...
						]
						...
					]

					Since these structures can be very large and difficult or impossible to transmit/parse
					make sure you limit the scope.

					The method might throw an exception. You must not assume that osql has not been
					written to if an exception is thrown.

					WARNING: This method poses a security risk as it does not observe D3 security. It is
					intended for use via ID3 so clients the submitted queries do not ciolate such access.
			*/
			virtual std::ostringstream& ExecuteQueryAsJSON(const std::string & strSQL, std::ostringstream	& oResultSets);

			//! Same as ExecuteQueryAsJSON but here we return the JSON different
			/*! The result sets are written as follows:
					[
						{
							MetaData:
							[
								{col1-details},
								{col2-details},
								...
							],
							Data:
							[
								[col1-val,col2-val,...],
								[col1-val,col2-val,...],
								...
							]
						},
						...
					]
			*/
			virtual std::ostringstream& QueryToJSONWithMetaData(const std::string & strSQL, std::ostringstream	& oResultSets);

			//! Update an Object.
			/*! This method updates a single object in the database.

					The update type is determined by the value returned by pObj->GetObjectType()
					which can be Entity::SQL_None, Entity::SQL_Update, Entity::SQL_Delete or
					Entity::SQL_Insert.

					@param	pObj		The object to update.

					@return bool		If true, update has succeeded otherwise it failed.

					\note For this method to succeed, you must have a pending transaction
					(i.e. you must have called BeginTransaction() on this).

					If the method succeedes, the following are facts:

					-	If the update was a DELETE, the object will have been deleted from the
						physical store as well as this cache.

					- If the update was an UPDATE, the object will be considered CLEAN after
						the update (i.e. pObj->GetUpdateType() will return Entity::SQL_None).

					- If the update was an INSERT and the object had an AutoNum (IDENTITY)
						column, the method will have retrieved the new value. The object is
						considered clean after the INSERT  (i.e. pObj->GetUpdateType() will
						return Entity::SQL_None).

					- If the update was NONE, the method returns with success immediately.
			*/
			bool											UpdateObject(EntityPtr pObj);

			//! This method does a blocking call and does not return until an Alert fires
			virtual DatabaseAlertPtr	MonitorDatabaseAlerts(DatabaseAlertManagerPtr pDBAlertMngr);

			//! Export data to an XML file
			/*! Adds XML elements to the specified XML document as children of the specified pParentNode.
			    If the optional pListME parameter (std::list< MetaEntity* >*) is supplied, only data from the
					MetaEntites listed will be exported. If this parameter is NULL, data from all tables will
					be exported. The only exception is the version info table which will never be exported.
			*/
			virtual long							ExportToXML(const std::string & strAppName, const std::string & strXMLFileName, MetaEntityPtrListPtr pListME = NULL, const std::string & strD3MDDBIDFilter = "");

			//! Import data from an XML file create with ExportToXML()
			/*! This method attempts to import data from an XML file which is expected to be compatible
					with the structure of the one that ExportToXML() creates. The method takes care to import
					the data in the correct sequence so that dependent records are imported after the records they
					depend on. If you supply the pListME parameter (std::list< MetaEntity* >*), only records of
					the meta entity type included in the list will be imported provided they are also included in the
					XML file. On SQL Server, the method ensures that IDENTITY_INSERT is turned off so that IDENTITY
					column values are preserved.

					\note Although data is inserted into thhe physical database, the data loaded will NOT be resident
					upon return.
			*/
			virtual long							ImportFromXML(const std::string & strAppName, const std::string & strXMLFileName, MetaEntityPtrListPtr pListME = NULL);

		protected:
			//! ExecuteSQLCommand allows you to execute an SQL Statement.
			/*! Precondition:  bReadOnly || HasTransaction()

					@param	strSQL			The SQL statement to execute.

					@return int					Returns -1 if an error occurred or the number of
															rows affected by the update statement.
			*/
			virtual int								ExecuteSQLCommand(const std::string & strSQL, bool bReadOnly);

			//! Same as the public LoadObjects() method with one small difference.
			/*! The difference is that you can pass an EntityPtrListPtr to the method.
					If this parameter is NOT NULL, the method will append the objects
					it spans to the	end of the collection and return a pointer to the
					collection that was passed in. If the pointer is NULL, the method will
					create a new, heap based collection and return a pointer to it in which
					case it will be the callers responsibility to discard the collection
					when no longer required.
			*/
			EntityPtrListPtr					LoadObjects(MetaEntityPtr pMetaEntity, EntityPtrListPtr pEntityList, const std::string & strSQL, bool bRefresh, bool bLazyFetch = true);

			void											ClearCache()											{}
			virtual bool							CheckDatabaseVersion();

			// LoadObjects() helpers
			EntityPtr									PopulateObject(MetaEntityPtr pMetaEntity, odbc::ResultSet* pRslts, bool bRefresh, bool bLazyFetch = true);
			EntityPtr									FindEntity(MetaEntityPtr pMetaEntity, odbc::ResultSet* pRslts);

			//! Create Physcial Database.
			/*! Clients use MetaDictionary::CreatePhysicalDatabase() to create a
					physical database. This method is called in turn in order to ensure
					that the correct mechanisms are used and the correct database type
					is created.
			*/
			static bool 							CreatePhysicalDatabase(MetaDatabasePtr pMD);
			//! Creates SQL Server Database
			static bool 							CreateSQLServerDatabase(MetaDatabasePtr pMD);
			//! Creates Oracle Database
			static bool 							CreateOracleDatabase(MetaDatabasePtr pMD);
			//! Create referential integrity triggers for the Table in the physical database
			static void								CreateTriggers(TargetRDBMS eTargetDB, odbc::Connection* pCon, MetaEntityPtr pMetaEntity);
			//! Create Cascade Update Package in the physical database
			static void								CreateUpdateCascadePackage(odbc::Connection* pCon);

			//! BeforeForImport disables all triggers and enables identity inserts
			/*! This method is called before inserting data into a table during bulk
			    importing of data.
			    @param pME is the meta entity for which data is to be imported
			*/
			void											BeforeImportData(MetaEntityPtr pME);
			//! AfterForImport enables all triggers and resets identity insert mechanisms
			/*! This method is called after inserting data into a table during bulk
			    importing of data.

			    @param pME is the meta entity for which data is to be imported

			    @param lMaxKeyVal the maximum value of any key that was inserted

			    \note In Oracle, this methods ensures the sequence associated with
			    this is correctly reset so that next time a value is requested it
			    returns lMaxKeyVal + 1.
			*/
			void											AfterImportData(MetaEntityPtr pME, unsigned long lMaxKeyVal);

			//! Drop any existing QN Services and Queues
			/*! This method is called the first time InitialiseDatabaseAlerts() is called to
					clean the system of existing QN resources
			*/
			void											DropObsoleteQNResources();

			//! A helper for D3::SQLServerPagedResultSet
			virtual EntityPtrListPtr	CallPagedObjectSP(ResultSetPtr	pRS,
																									MetaEntityPtr	pMetaEntity,
																									unsigned long uPageNo,
																									unsigned long uPageSize,
																									unsigned long & lStart,
																									unsigned long & lEnd,
																									unsigned long & lTotal);

			//! Initialise DatabaseAlertResources
			/*! This method throws an error at this level, though it is only called if you try to use alert notifications.
					\note At this level, the method retruns with success (assuming nothing needs to be done to enable DatabaseAlerts for this type of database)
			*/
			virtual bool							InitialiseDatabaseAlerts(DatabaseAlertManagerPtr pDBAlertMngr);

			//! UnInitialise DatabaseAlertResources
			/*! This method can be overloaded by subclasses to handle database alert requests.
					You typically overload this method if you need to do extra work to enable database
					alerts so that you can remove these resources on exit.
					\note At this level, the method retruns with success (assuming nothing needs to be done to enable DatabaseAlerts for this type of database)
			*/
			virtual void							UnInitialiseDatabaseAlerts(DatabaseAlertManagerPtr pDBAlertMngr);

			//! Register DatabaseAlert
			/*! This method is used internally to register a DatabaseAlert.
					The purpose of the method is to ensure that a subsequent call to MonitorDatabaseAlerts() will catch notifications of the requested type.
					\note At this level, the method always throws an exception.
			*/
			virtual void							RegisterDatabaseAlert(DatabaseAlertPtr pDBAlert);

			//! Verbose method to register a database alert
			virtual void							RegisterDatabaseAlert(const std::string & strServiceName, const std::string & strAlertName, const std::string & strQuery, int iTimeOut);

			//! UnRegister DatabaseAlert
			/*! This method is used internally to register a DatabaseAlert.
					The purpose of the method is to ensure that we no longer trap the passed in DatabaseAlert type.
					\note At this level, the method always throws an exception.
			*/
			virtual void							UnRegisterDatabaseAlert(DatabaseAlertPtr pDBAlert);

			//! Bare bones ODBC error handler
			/*!	Return a string that consists of the strMsg passed in with additional error info appended

					The handle type must much the handle passed in and be one of these consts:

						SQL_HANDLE_ENV
						SQL_HANDLE_DBC
						SQL_HANDLE_STMT
			*/
			static std::string				GetODBCError(SQLSMALLINT iHandleType, SQLHANDLE& hHandle, const std::string& strMsg);

			//! Helper to MonitorDatabaseAlerts() which parses an XML alert message we receive from SQL Server
			DatabaseAlertPtr					ParseAlertMessage(DatabaseAlertManagerPtr pDBAlertManager, const std::string& strXMLMessage);

			//@{
			//! Set a JSON array to the currently loaded D3::Meta[Type] objects
			/*!	The following methods assign JSON arrays containing the id (numeric) and title (string) of HSTopic
					records which applies to the current target and is linked with the meta type object via the relevant
					HSMeta[Type]Topic associative entity. The methods only retrieve this data but do not make HSTopic objects
					resident.
			*/
			virtual void							SetMetaDatabaseHSTopics();
			virtual void							SetMetaEntityHSTopics();
			virtual void							SetMetaColumnHSTopics();
			virtual void							SetMetaKeyHSTopics();
			virtual void							SetMetaRelationHSTopics();
			//@}

			//! This method can be called while handling an odbc::SQLException.
			/*! If the error turns out to be a connection problem, the connection will be considered closed (i.e. IsConnected() will return false).
					You can then try to send the Reconnect() message which may potentially recover from the problem.
					If the method discovers the exception to be a "disconnected" error code, a warning will be written to the log.
			*/
			void											CheckConnection(odbc::SQLException& e);

			//! This method is called if a method that requires a connection discovers that IsConnected() returns false
			/*! The method attampts to reconnect to the database. The method logs warnings in both cases: if the connection is still down or if the
					connection was recovered.
			*/
			void											Reconnect();

			//! Helper: writes the data from the current record in to the stream provided
			void											WriteJSONToStream(std::ostringstream & ostrm, odbc::ResultSet* pRslts);

			// This notification is sent by the associated MetaDatabase object
			// once the object has been constructed and initialised
			virtual void							On_PostCreate();
	};

}	// end namespace D3

#endif /* APAL_SUPPORT_ODBC	*/			// If not defind, skip entire file

#endif /* INC_D3_ODBCDATABASE_H */
