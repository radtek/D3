#ifndef INC_D3_OTLDATABASE_H
#define INC_D3_OTLDATABASE_H

// MODULE: OTLDatabase header
//
// NOTE: This module implements OTL specific ako Database
//;
// ===========================================================
// File Info:
//
// $Author: pete $
// $Date: 2013/02/24 22:15:10 $
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
#ifdef APAL_SUPPORT_OTL			// Skip entire file if no OTL support is wanted

#include "D3.h"
#include "D3Date.h"
#include "D3Funcs.h"
#include "Database.h"
#include "OTLParams.h"


namespace D3
{
	// Some forwards we'll need
	class OTLColumn;
	typedef	OTLColumn*		OTLColumnPtr;

	class OTLRecord;
	typedef	OTLRecord*		OTLRecordPtr;

	//! OTLColumn is a helper class, it is used by OTLRecord
	class OTLColumn
	{
		friend class OTLDatabase;
		friend class OTLRecord;

		protected:
			MetaColumnPtr			m_pMetaColumn;	//!< Could be NULL
			char *						m_szName;				//!< The name of the column as supplied through OTL
			int								m_iType;				//!< The column type supplied by otl but converted to D3
			bool							m_bNull;				//!< Indicator if the value is NULL or not
			bool							m_bLazyFetch;		//!< If true and m_pMetaColumn is specified and marked LazyFetch, the value of this column is a bool. If the bool is true, the BLOB has a value otherwise the BLOB is NULL

			//! The column value
			/*! The OTLColumn objects m_iType member identifies which of the union member values is valied. The
					value is meaningless if the OTLColumn objects m_bNull member is true.
					\note If m_iType is a MetaColumn::dbfString, the pstrVal will point to a valid stl
					string which will be deallocated at destruction time. If m_iType is MetaColumn::dbfDate,
					the pdtVal member will be a correctly allocated D3::D3Date object which will be deallocated
					in the destructor.
			*/
			union Value
			{
				std::string*			pstrVal;
				char							cVal;
				short							sVal;
				bool							bVal;
				int								iVal;
				long							lVal;
				float							fVal;
				D3Date*						pdtVal;
				otl_long_string*	otlLOB;
			}	m_Value;

		public:
			//! The default constructor simply initialises a column to be undefined
			OTLColumn();

			//! The destructor deletes pstrVal or pdtVal if the column is a MetaColumn::dbfString or MetaColumn::dbfDate type
			~OTLColumn();

			//! Assign this's value to the column passed in
			const char *	GetName				()		{ return m_szName; }

			//! This method is used by OTLRecord during its ctor to initialise all columns
			/*! The first argument passed in is an otl_column_desc column describing this. This
					structure is used to initialise this' type and name. The second parameter is
					optional. If supplied, it determines this' datatype and if a conversion is
					necessary, the conversion takes place when the data is fetched from the OTL stream.
			*/
			void					Init					(const otl_column_desc* desc, MetaColumnPtr pMC = NULL, bool bLazyFetch = true);

			//! The fetchValue method simple retrieves the value for this column from the otl stream passed in
			void					FetchValue		(otl_nocommit_stream& oRslts);

			//! Assign this's value to the column passed in
			void					AssignToD3Column(ColumnPtr pCol);

			//! Return true if this is NULL
			bool					IsNull()			{ return m_bNull; }

			/*! @name Value conversion methods
					These methods can be used to convert the value of an OTLColumn to another value.
					You should always verify first that this->IsNull() is false before using
					conversion methods.
			*/
			//@{
			//!	Convert to stl string
			std::string		AsString();
			//!	Convert to char
			char					AsChar();
			//!	Convert to short
			short					AsShort();
			//!	Convert to bool
			bool					AsBool();
			//!	Convert to int
			int						AsInt();
			//!	Convert to long
			long					AsLong();
			//!	Convert to float
			float					AsFloat();
			//!	Convert to D3Date
			D3Date				AsDate();

			//!	Write to JSON stream
			std::ostringstream& WriteToJSONStream(std::ostringstream& ostrm, bool bJSONEncode = false);
			//@}
	};





	// Types needed by OTLRecord
	typedef	std::vector<OTLColumn>						OTLColumnVect;
	typedef	OTLColumnVect::iterator						OTLColumnVectItr;






	//! The OTLRecord class is a OTLDatabase helper.
	/*! An OTLRecord object is typically created once an otl_nocommit_stream object has been properly opened.
			Once the OTLRecord has been created, you'll use this object to navigate from record to
			record.

			\code
			otl_nocommit_stream		oRslts;
			OTLRecord							otlRec;			// The OTLRecord we'll be using to navigate the otl_nocommit_stream
			OTLColumnPtr					pOtlCol;		// An OTLColumn* we'll fetch from the OTLRecord
			int										idxCol;			// We'll use this to navigate the OTLRecord objects OTL column objects


			// Assume oConnection is a valid otl connection, so let's open a resultset
			//
			oRslts.open(50, "SELECT * FROM MyTable", oConnection);

			// Let's create a OTLRecord and navigate through all records
			//
			otlRec.Init(oRslts);

			while (otlRec.Next())
			{
				// Navigate all columns from this record
				for (idxCol = 0; idxCol < otlRec.ColumnCount(); idxCol++)
				{
					pCol = otlRec.Column[idxCol];

					// Work with each column here
				}
			}
			\endcode
	*/
	class OTLRecord
	{
		friend class OTLDatabase;

		protected:
			otl_nocommit_stream*			m_pOTLStream;			//!< The resultset of which this OTLRecord object holds a single record
			OTLColumnVect							m_vectOTLColumn;	//!< An stl vector holding the OTLColumn objects for this record
			bool											m_bEmpty;					//!< True if this is empty
			bool											m_bLazyFetch;			//!< If true, columns marked as LazyFetch will not be loaded
			int												m_nDestCols;			//!< The number of columns in the select (may be more - but never less than m_vectOTLColumn.size() - after Init() is called)

		public:
			//! The constructor initialises this from the otl_nocommit_stream passed in
			OTLRecord() : m_pOTLStream(NULL), m_bEmpty(true), m_nDestCols(0), m_bLazyFetch(true) {}

			//! The Init method initialises this from the otl_nocommit_stream passed in
			/*! @param otlStream	The otl_nocommit_stream object this will navigate with next()

					@param pvectMC		A pointer to an stl vector holding D3 MetaColumn objects.
														This parameter can be NULL. If it is specified, the
														method will verify that the number of columns in the
														otl stream match the number of columns in this vector
														and that the name of each column in the otl stream matches
														the name of the columns in this vector. The method will
														also set the type of its internal OTLColumns to match the
														type of the MetaColumn objects in this vector.

					D3 often issues queries based on MetaEntity details. Therefore, an otl
					stream frequently is expected to contain records consisting of all
					columns from a given MetaEntity. In these situations, you can initialise
					this with pvectMC = myMetaEntity.GetMetaColumns(). You can then navigate
					this OTLRecord and conveniently and most efficiently assign the values
					to an instance of the MetaEntity by using the AssignToD3Entity method.

					If you don't supply a pvectMC parameter, you are expected to retrieve the values
					of each OTLColumn manually using the OTLColumn objects AsXXXXX methods.

					If you supply a pvectMC parameter and the columns missmatch, the method will throw
					a D3::Exception.

					Make sure you also catch otl_exception& objects.

					\note Init() does NOT navigate the internal otl stream. Sending a
					Next() message after the Init() message will initialise this with the values
					from the first otl record.
			*/
			void					Init					(otl_nocommit_stream&	otlStream, MetaColumnPtrVectPtr pvectMC = NULL, bool bLazyFetch = true)		{ Init(otlStream, pvectMC, NULL, bLazyFetch); }

			//! The Init method initialises this from the otl_nocommit_stream passed in
			/*! @param otlStream	The otl_nocommit_stream object this will navigate with next()

					@param plistMC		A pointer to an stl list holding D3 MetaColumn objects.
														The method will verify that the number of columns in the
														otl stream match the number of columns in this list
														and that the name of each column in the otl stream matches
														the name of the columns in this list. The method will
														also set the type of its internal OTLColumns to match the
														type of the MetaColumn objects in this list.

					If you supply a MetaEntity and the columns missmatch, the method will throw
					a D3::Exception.

					Make sure you also catch otl_exception& objects.

					\note Init() does NOT navigate the internal otl stream. Sending a
					Next() message after the Init() message will initialise this with the values
					from the first otl record.
			*/
			void					Init					(otl_nocommit_stream&	otlStream, MetaColumnPtrListPtr plistMC, bool bLazyFetch = true)					{ Init(otlStream, NULL, plistMC, bLazyFetch); }

			//! This method returns true if the attached otl_nocommit_stream has been navigated all the way to the end
			bool					Eof						()					{ return m_pOTLStream ? m_pOTLStream->eof() : true; }

			//! Fetch the next record.
			/*! For this method to succeed, the Init() message must have been processed
					successfully.
					The first time you call this method, the first record will be loaded.
					Successive calls fetch successive records.

					Make sure you also catch otl_exception& objects.
			*/
			bool					Next					();

			//! Returns the number of columns for this OTLRecord
			unsigned int	size		()							{ return m_vectOTLColumn.size(); }

			//! Returns true if this is empty
			/*! This is always true after construction and before the Init() has been
					processed successfully. Init() will set this to false if the otl stream
					associated with this is not at eof().
			*/
			bool					empty		()							{ return m_bEmpty; }

			//! Get the column at ordinal position idx.
			/*! The method throws D3::Exception if an attempt is made to access
					a column with idx < 0 or idx > ColumnCount().
			*/
			OTLColumn&		operator[](unsigned int idx);

			//! Get the column with the name indicated by str.
			/*! The method throws D3::Exception if a column with the given name does
					not exist.

					\note OTL treats column names case insensitive.
			*/
			OTLColumn&		operator[](const std::string& str);

			//! Assign the OTLColumn values to the columns in the vector
			/*! @param pvectCol A vector of ako D3 Column objects to populate
			                    with the correct values (or NULL) from this.

					The Init() method and the AssignToD3Entity() method work together.
					If you supply a MetaColumnPtrVectPtr to the Init() method, than
					you can call this method with a ColumnPtrVectPtr which reflects
					instances of the MetaColumns passed in to Init().

					Most typically, this methood is used in conjunction with a MetaEntity
					and Entity objects which are instances of the MetaEntity object.
					Firstly, you'd initialise this with the MetaEntity's MetaColumn
					vector: Init(otlSTtream, myMetaEntity.GetMetaColumns()), then you create instances
					of the MetaEntity and populate the instance from this using this method
					AssignToD3Entity(myInstanceEntity.GetColumns()).
			*/
			void					AssignToD3Entity(ColumnPtrVectPtr pvectCol)		{ AssignToD3Entity(pvectCol, NULL); }

			//! Assign the OTLColumn values to the columns in the list
			/*! @param plistCol A list of ako D3 Column objects to populate
			                    with the correct values (or NULL) from this.

					The Init() method and the AssignToD3Entity() method work together.
					If you supply a MetaColumnPtrListPtr to the Init() method, than
					you can call this method with a ColumnPtrListPtr which reflects
					instances of the MetaColumns passed in to Init().
			*/
			void					AssignToD3Entity(ColumnPtrListPtr plistCol)		{ AssignToD3Entity(NULL, plistCol); }

		protected:

			//! Internal Init() method to which this' public Init() methods delegate
			/*! @param otlStream	The otl_nocommit_stream object this will navigate with next()

					@param pvectMC		A pointer to an stl vector holding D3 MetaColumn objects.
														This parameter can be NULL. If it is specified, the
														method will verify that the number of columns in the
														otl stream match the number of columns in this vector
														and that the name of each column in the otl stream matches
														the name of the columns in this vector. The method will
														also set the type of its internal OTLColumns to match the
														type of the MetaColumn objects in this vector.

					@param plistMC		A pointer to an stl list holding D3 MetaColumn objects.
														This parameter can be NULL. If it is specified, the
														method will verify that the number of columns in the
														otl stream match the number of columns in this list
														and that the name of each column in the otl stream matches
														the name of the columns in this list. The method will
														also set the type of its internal OTLColumns to match the
														type of the MetaColumn objects in this list.

					In the case of errors, the method throws D3::Exceptions or otl_exceptions.

					\note Both parameters, pvectMC and plistMC can be NULL. If one is specified,
					you should leave the other NULL. If both are specified, pvectMC is used.

			*/
			void					Init					(otl_nocommit_stream& otlStream, MetaColumnPtrVectPtr pvectMC, MetaColumnPtrListPtr plistMC, bool bLazyFetch = true);

			//! Internal helper
			void					AssignToD3Entity(ColumnPtrVectPtr pvectCol, ColumnPtrListPtr plistCol);

	};









	//! The OTLStreamPool class is used to keep otl stream objects alive and use them like prepared statements.
	/*! Each OTLStreamPool member holds multiple streams for exactly one entity type (D3::MetaEntity).
		There are there types of streams, INSERT, DELETE and SELECT streams.

		INSERT streams require all columns to be supplied to the stream in the same order as they are listed
		in the meta entities meta column collection. If a value is NULL, you should supply an otl_value<type>
		which is set to NULL as input to the stream.
	*/
	class OTLStreamPool
	{
		public:
			//! The OTLStream class holds a string that reflects the streams sql statement and a native stream
			class OTLStream
			{
				friend class OTLStreamPool;

				protected:
					otl_nocommit_stream*		m_pStream;
					std::string							m_strSQL;
					bool										m_bLazyFetch;

					//! ctor: LazyFetch only applies to select's and is by default true
					OTLStream(const std::string & strSQL, otl_connect& otlConnection, bool bLazyFetch = true) : m_pStream(NULL), m_strSQL(strSQL), m_bLazyFetch(bLazyFetch)
					{
						m_pStream = new otl_nocommit_stream(1, strSQL.c_str(), otlConnection);
					}

					~OTLStream()
					{
						if (m_pStream)
						{
							m_pStream->close();
							delete m_pStream;
						}
					}

				public:
					//! The only public methd: cast operator that treats this like an otl_nocommit_stream
					otl_nocommit_stream&		GetNativeStream() { return *m_pStream; }
					std::string&						GetSQL() { return m_strSQL; }
																	operator otl_nocommit_stream* () { return m_pStream; }
			};

			typedef OTLStream*										OTLStreamPtr;
			typedef std::list<OTLStreamPtr>				OTLStreamPtrList;
			typedef OTLStreamPtrList*							OTLStreamPtrListPtr;
			typedef OTLStreamPtrList::iterator		OTLStreamPtrListItr;

		protected:
  		boost::recursive_mutex					m_mtxExclusive;								//!< This mutex serialises access to the pool
  		otl_connect&										m_otlConnection;							//!< The connection to use for streams in this pool
  		MetaEntityPtr										m_pMetaEntity;								//!< This stream pool is for D3::Entity objects of this type
			OTLStreamPtrList								m_listInsertStream;						//!< Contains streams to "INSERT" an instance
			OTLStreamPtrList								m_listDeleteStream;						//!< Contains streams to "DELETE" a single instance by primary key
			OTLStreamPtrList								m_listSelectStream;						//!< Contains streams to "SELECT" a single instance by primary key
			OTLStreamPtrList								m_listLFSelectStream;					//!< Contains streams to "SELECT" a single instance by primary key (uses LazyFetch)

		public:
			OTLStreamPool(otl_connect& otlConnection, MetaEntityPtr pME) : m_otlConnection(otlConnection), m_pMetaEntity(pME) {}
			~OTLStreamPool();

			OTLStreamPtr						FetchInsertStream();
			OTLStreamPtr						FetchDeleteStream();
			OTLStreamPtr						FetchSelectStream(bool bLazyFetch = true);

			void					ReleaseInsertStream(OTLStreamPtr pStrm);
			void					ReleaseDeleteStream(OTLStreamPtr pStrm);
			void					ReleaseSelectStream(OTLStreamPtr pStrm);

		private:
			OTLStreamPtr						CreateInsertStream();
			OTLStreamPtr						CreateDeleteStream();
			OTLStreamPtr						CreateSelectStream(bool bLazyFetch = true);
	};


	typedef OTLStreamPool*									OTLStreamPoolPtr;
	typedef std::vector<OTLStreamPoolPtr>		OTLStreamPoolPtrVect;






	//! The OTLDatabase class interfaces with the physical datastore through Sergei Kuchin's OTL template library.
	/*! This class implements access to the physical store through Sergei Kuchin's
			<I><B>Oracle, ODBC and DB2-CLI Template Library</B></I> (better known as OTL template library).
			Details and the latest source code can be found at <A HREF="http://otl.sourceforge.net">Sourceforge</A>.

			Currently, this class supports Oracle and SQL Server only. For Oracle, no
			ODBC driver is required as OTL interacts with the Oracle CLI directly. For
			SQLServer an ODBC driver is required.

			There is a potential to enhance this class with support for other RDBMS, but
			this necessitates changing built in mechanisms to maintain referential integrity
			to D3 as well as other outside clients. These mechanism currently only work
			reliably for Oracle and SQLServer.
	*/
	class D3_API OTLDatabase : public Database
	{
		friend class MetaDatabase;
		friend class OTLXMLImportFileProcessor;

		D3_CLASS_DECL(OTLDatabase);

		protected:
			bool											m_bIsConnected;				//!< If true we have a valid connection
			int												m_iTransactionCount;	//!< If > 0 we have transactions pending
			otl_connect								m_oConnection;				//!< Represents the Oracle connection
			StringList								m_listAlerts;					//!< A list of alerts currently registered
			int												m_iPendingAlertID;		//!< The ID of
			OTLStreamPoolPtrVect			m_vectOTLStreamPool;	//!< Contains OTL Stream Pools for each meta entity used so far


			//! Protected constructor
			/*! Use MetaDatabase::CreateInstance() to create an instance of a database
			    that you can navigate and manipulate.
			*/
			OTLDatabase();
			//! Protected constructor
			/*! Use MetaDatabase::CreateInstance() to create an instance of a database
			    that you can navigate and manipulate.
			*/
			OTLDatabase(MetaDatabasePtr pMetaDatabase, DatabaseWorkspacePtr pDatabaseWorkspace, ExceptionContextPtr pEC = ExceptionContext::GetDefaultExceptionContext());
			//! Protected Destructor
			/*! This destructor is automatically called by the owning workspace
			*/
			virtual ~OTLDatabase();

		public:
			//! Make Cancel Call and exit from Blocking Call DBMS_ALERT.WAITONE.
			virtual bool							Disconnect();
			//! Physical connect to the physical store.
			virtual bool							Connect();
			//! Returns true if this is physically connected with the physical store.
			virtual bool							IsConnected()
			{
				return m_oConnection.connected;
			}

			//! Register an alert
			/*! Alerts are typically fired by TRIGGER mechanisms implemented in the oracle database.

					Let's assume we would like a notification inside our application whenever a row is added
					to table XYZ. We'd implement a trigger for the table in Oracle as follows:

					CREATE TRIGGER ai_XYZ
					AFTER
					INSERT ON XYZ
					BEGIN
						DBMS_ALERT.SIGNAL('INSERT__XYZ', 'New row(s) added to XYZ');
					END;

					Now, whenever rows are added to table XYZ, Oracle fires an "INSERT__XYZ" alert once the
					update was commited.

					You can now simply register the INSERT__XYZ alert with this method and then call WaitForAlert().
					WaitForAlert() will not return until it receives the INSERT__XYZ alert notification is received
					or until RemoveAlert() is called.

					You can register multiple alerts. Each time you register an alert successfully, this method will
					return the ID of the alert. After registering multiple alerts, you can cal WaiForAlert() without
					an argument to wait for any of the alerts you registered. When WaitForAlert() returns wit a non
					negative value, the value returned is the alerts ID as returned by this method when the alert was
					first registerd. If you don't want to keep track of alert id's, you can simply compare the value
					returned by WaitForAlert() with GetAlertID("alertname") to determine which alert fired.

					You can call WaitForAlert again as soon as it returns. You can also remove an alert by calling
					RemoveAlert(). If you	call RemoveAlert() without specifying an alert name, all alerts will be
					removed.

					\note You will use your own database instance to do an alert blocking call with WaitForAlert().
					A single database will either wait for one alert or for many alerts but not both. If you do
					not want to wait any longer, another thread should call CancelWaitForAlert(). You are not expected
					to remove alerts and then add new ones. The usage of the alert methods are: register alert(s),
					wait for alert(s), process alert, repeate wait for alert(s) or exit.
			*/
			virtual int								RegisterAlert(const std::string & strAlertName);
			//! WaitForAlert() waits for one or more alerts registered with the RegisterAlert() method. See RegisterAlert() for more details.
			virtual int								WaitForAlert(const std::string & strAlertName = "");
			//! CancelWaitForAlert() cancels a pending WaitForAlert()
			virtual void							CancelWaitForAlert();
			//! Remove a previously registered alert and cancels a pending WaitForEvent() on this alert. See RegisterAlert() for more details.
			virtual int								RemoveAlert(const std::string & strAlertName = "");
			//! Alert helper: Returns the internal index of the alert or a negative value if the alert is not registered
			virtual int								GetAlertID(const std::string & strAlertName);
			//! Given an Alert ID, get the name of the alert (at this level, does nothing and returns -1, see specific sub-classes for details)
			virtual std::string				GetAlertName(int iAlertID);
			//! Get the list of registered alerts	(at this level, does nothing and returns NULL, see specific sub-classes for details)
			virtual StringList*				GetAlertList()		{ return &m_listAlerts; }

			//! Start a transaction.
			virtual bool							BeginTransaction();
			//! Make all updates to the database performed since the preceeding call to BeginTransaction() permanent.
			virtual bool							CommitTransaction();
			//! Undo all updates to the database performed since the preceeding call to BeginTransaction().
			virtual bool							RollbackTransaction();
			//! Return true if this has a pending transaction.
			virtual bool							HasTransaction()									{ return m_iTransactionCount > 0; }


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
			virtual std::ostringstream & ExecuteQueryAsJSON(const std::string & strSQL, std::ostringstream	& oResultSets);

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

			//! Export data to an XML file
			/*! Adds XML elements to the specified XML document as children of the specified pParentNode.
			    If the optional pListME parameter (std::list< MetaEntity* >*) is supplied, only data from the
					MetaEntites listed will be exported. If this parameter is NULL, data from all tables will
					be exported. The only exception is the version info table which will never be exported.
			*/
			virtual long							ExportToXML(const std::string & strAppName, const std::string & strXMLFileName, MetaEntityPtrListPtr pListME = NULL, const std::string & strD3MDDBIDFilter = "");

			//! Import data from an XML file create with ExportToXML()
			/*! This method attempts to import data from and XML DOM which is expected to be compatible
					with the structure of the one that ExportToXML() creates. The method takes care to import
					the data in the correct sequence so that dependent records are imported after the records they
					depend on. If you supply the pListME parameter (std::list< MetaEntity* >*), only records of
					the meta entity type included in the list will be imported provided they are also included in the
					XML DOM. On SQL Server, the method ensures that IDENTITY_INSERT is turned off so that IDENTITY
					column values are preserved.

					\note Although data is inserted into thhe physical database, the same data will NOT be resident
					upon return.
			*/
			virtual long							ImportFromXML(const std::string & strAppName, const std::string & strXMLFileName, MetaEntityPtrListPtr pListME = NULL);

		protected:
			//! ExecuteSQLCommand allows you to execute an SQL Statement to read or update
			/*! Precondition: bReadOnly || HasTransaction()

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
			EntityPtrListPtr					LoadObjects(MetaEntityPtr pMetaEntity, EntityPtrListPtr pEntityList, const std::string & strSQL, bool bRefresh, bool bLazyFetch);

			//! This method is called when you call LoadObject(pKey, bRefresh) and pKey is a primary key
			/*! The method gets an otl stream from a pool of streams to navigate the database.
			    The first call is obviously a bit slower but successive calls are expected to
					be significantly more efficient. This method is only available if this module is
					compiled with the D3_ENABLE_OTLSTREAMPOOLS macro defined
			*/
			EntityPtr									LoadObjectWithPrimaryKey(KeyPtr pKey, bool bRefresh, bool bLazyFetch);

			//! This method is called by UpdateObject() when the D3_ENABLE_OTLSTREAMPOOLS macro defined and the update type is INSERT
			/*! The method gets an otl stream from a pool of streams to insert new records into the database.
			    The first call is obviously a bit slower but successive calls are expected to
					be significantly more efficient. This method is only available if this module is
					compiled with the D3_ENABLE_OTLSTREAMPOOLS macro defined
			*/
			bool											InsertObject(EntityPtr pObj);

			//! Clears all cached data
			void											ClearCache()											{}

			//! Returns true if the actual database version matches the version identified by this' MetaDatabase object
			virtual bool							CheckDatabaseVersion();

			//! Internal LoadObjects() helper
			EntityPtr									PopulateObject(MetaEntityPtr pMetaEntity, OTLRecord& otlRec, bool bRefresh);

			//! Internal LoadObjects() helper
			EntityPtr									FindObject(MetaEntityPtr pMetaEntity, OTLRecord& otlRec);

			//! Create Physcial Database.
			/*! Clients use MetaDictionary::CreatePhysicalDatabase() to create a
					physical database. This method is called in turn in order to ensure
					that the correct mechanisms are used and the correct database type
					is created.
			*/
			static bool								CreatePhysicalDatabase(MetaDatabasePtr pMD);
			//! Create an Oracle database. Currently, OTL is used exclusively for Oracle.
			static bool								CreateOracleDatabase(MetaDatabasePtr pMD);
			//! Create an ORCALE trigger
			static void								CreateTriggers(TargetRDBMS eTargetDB, otl_connect& pCon, MetaEntityPtr pMetaEntity);
			//! Create an ORACLE package that automatically updates references to a key if the keys columns are updated
			static void								CreateUpdateCascadePackage(otl_connect& pCon);

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

			//! Notification sent by a MetaDatabase after it has created an instance of this type
			/*! This method is intercepted to initialise the otl stream pool. Once initialised,
					the message is routed to the Database::On_PostCreate().
			*/
			virtual void							On_PostCreate();

			//! Returns a pointer to the otl stream pool for the meata entity
			OTLStreamPoolPtr					GetOTLStreamPool(MetaEntityPtr pME);

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
	};

}	// end namespace D3

#endif // APAL_SUPPORT_OTL

#endif /* INC_D3_OTLDATABASE_H */
