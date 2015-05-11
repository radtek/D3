#ifndef INC_D3_DATABASE_H
#define INC_D3_DATABASE_H

// MODULE: Database header
//
// NOTE: This module implements basic functionality for the MR OO Model. It is a
// pure virtual class and objects of this class cannot be instantiated. Descending
// classes must be used to interact with a physical datastore.
//;
// ===========================================================
// File Info:
//
// $Author: daellenj $
// $Date: 2014/08/23 21:47:46 $
// $Revision: 1.41 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 03/10/02 - R2 - Hugp
//
// Added support for APAL3TaskManager
//
//
// 08/04/03 - R3 - HugP
//
// Added reference counting support. The main reason for this change
// is to enable the concept of a APAL3Task database which can be exposed
// through a COM interface.
//
// The destructor is now protected. Destruction of database objects are
// done by external clients by sending the object the ReleseRef() message
// instead of deleting the object.
//
// Implemented virtual methods implementing empty Import methods
// so that these methods can be wrapped in the COM interface
// regardless of the target build type.
//
//
// 03/06/03 - R3 - HugP
//
// Implemented TMS compliant ImportShipments method
//
// Implemented GetObjectStatisticsText(), a debugging helper
// -----------------------------------------------------------
#include "D3.h"
#include "D3MDDB.h"
#include "D3BitMask.h"
#include "Exception.h"
#include "Key.h"
#include "D3Date.h"
#include <boost/thread/recursive_mutex.hpp>
#include <set>
#include <list>


extern const char szMetaDatabaseFlags[];					// solely used to ensure type safety (no implementation needed)
extern const char szMetaDatabasePermissions[];		// solely used to ensure type safety (no implementation needed)

namespace D3
{
	//! Properties for the Dictionary MetaDatabase object
	/*! You will create an object of this class and populate all the attributes at system
			start.

			The purpose of this class is to provide a simple means for you to pass on the
			necessary details to initialise the D3 data dictionary.

			A typical use of this class is:

			\code
			bool start_system()
			{
				MetaDatabaseDefinition	MD;

				MD.m_eTargetRDBMS			= MetaDatabaseDefinition::SQLServer;

				MD.m_strServer				= "MySQLServer";
				MD.m_strUserID				= "Charly";
				MD.m_strUserPWD				= "Ru2s4t?";
				MD.m_strName					= "D3MetaDictionary"
				MD.m_strDataFilePath	= "c:\\MSSQL7\\databases\\D3MetaDictionary.mdf";
				MD.m_strLogFilePath		= "c:\\MSSQL7\\databases\\D3MetaDictionary.ldf";
				MD.m_iInitialSize			= 10;			// 10MB initial size
				MD.m_iMaxSize					= 10000		// 10GB maximum size
				MD.m_iGrowthSize			= 10;			// 10MB growth step
				MD.m_iVersionMajor		= 6;			//\
				MD.m_iVersionMinor		= 1;      // > Must match the current hardcoded MetaDictionary Version
				MD.m_iVersionRevision	= 2;      ///
				MD.m_strTimeZone			= "PST-8PDT01:00:00,M4.1.0/02:00:00,M10.1.0/02:00:00";

				try
				{
					D3::MetaDatabase::InitMetaDictionary(MD);
				}
				catch(D3::Exception & e)
				{
					e.ReportError();
					return false;
				}
				catch(...)
				{
					D3::ReportError("Some silly error occurred initialising the Meta Dictionary.");
					return false;
				}

				return true;
			}
			\endcode
	*/
	struct MetaDatabaseDefinition
	{
		TargetRDBMS				m_eTargetRDBMS;					//!< See TargetRDBMS above
		std::string				m_strDriver;						//!< The name of the ODBC drievr (only applies to ODBC DSN)
		std::string				m_strServer;						//!< The name of the server
		std::string				m_strUserID;						//!< The name of the user to use for connections
		std::string				m_strUserPWD;						//!< The password of the user to use for connections
		std::string				m_strName;							//!< The actual database name
		std::string				m_strAlias;							//!< The alias for the database (this is a logical name)
		std::string				m_strDataFilePath;			//!< The path of the data file on the server
		std::string				m_strLogFilePath;				//!< The path of the log file on the server
		int								m_iInitialSize;					//!< The initial size of the database in MB
		int								m_iMaxSize;							//!< The maximum size of the database in MB
		int								m_iGrowthSize;					//!< The growth size of the database in MB
		const Class*			m_pInstanceClass;				//!< The instance class (ako D3::Database) implementing access to such databases
		int								m_iVersionMajor;				//!< The major version of the database
		int								m_iVersionMinor;				//!< The mainor version of the database
		int								m_iVersionRevision;			//!< The revison of the database
		std::string				m_strTimeZone;					//!< The TimeZone details for the server that hosts the database. If missing defaults to the time zone of the system running the binary.
	};

	typedef std::list< MetaDatabaseDefinition >			MetaDatabaseDefinitionList;
	typedef MetaDatabaseDefinitionList::iterator		MetaDatabaseDefinitionListItr;







	//! We store all MetaDatabase objects in a map keyed by ID (which also identifies the same object in the metaDictionary database)
	/*! Note that objects which are not stored in the meta dictionary (e.g. MetaDatabase objects
			created at system startup to enable access to the the MetaDictionary database)
			are assigned MetaDatabase::M_uNextInternalID which is then decremented by 1.. The
			initial value	of MetaDatabase::M_uNextInternalID is DATABASE_ID_MAX.
			MetaDatabase objects without a physical representation on disk are therefore
			identifiable if GetID() returns a value > MetaDatabase::M_uNextInternalID.
	*/
	typedef std::map< DatabaseID, MetaDatabasePtr >				MetaDatabasePtrMap;
	typedef MetaDatabasePtrMap::iterator									MetaDatabasePtrMapItr;




	// Some forward decalrations required for database alert management
	class DatabaseAlertManager;
	typedef DatabaseAlertManager*		DatabaseAlertManagerPtr;

	class MetaDatabaseAlert;
	typedef MetaDatabaseAlert*														MetaDatabaseAlertPtr;
	typedef std::map<std::string, MetaDatabaseAlertPtr>		MetaDatabaseAlertPtrMap;
	typedef MetaDatabaseAlertPtrMap::iterator							MetaDatabaseAlertPtrMapItr;

	class DatabaseAlert;
	typedef DatabaseAlert*																DatabaseAlertPtr;
	typedef std::map<std::string, DatabaseAlertPtr>				DatabaseAlertPtrMap;
	typedef DatabaseAlertPtrMap::iterator									DatabaseAlertPtrMapItr;
	typedef std::list<DatabaseAlertPtr>										DatabaseAlertPtrList;
	typedef DatabaseAlertPtrList::iterator								DatabaseAlertPtrListItr;

	typedef void (*DatabaseAlertListenerFunc)(DatabaseAlertPtr pDBAlert);




	//! Instances of Database::DatabaseAlertManager class monitor SQL Server database alerts.
	/*! D3::DatabaseAlertManager allows clients to trap updates to tables. In SQL Server,
			you can specify a SELECT statement to monitor. SQL Server will trigger the alert
			if an action is performed on the database where the specified SELECT statement would
			produce different results before and after the update.

			While you can have multiple database alert managers running at the same time, each of
			these can only trap alerts from a single database.

			Some prerequisites must be met for the alert system to work under SQL Server:
			-# only versions of SQL Server 2005 and later support database alerts because they rely on	the SQL Service Broker which is not available in earlier versions
			-# you must use the SQL Server Native Client Driver to connect with SQL Server as the Service Broker is not supported by the generic SQL Server Driver
			-# the Service Broker is not enabled by default; to enable it, you can use SQL Server Management Studio, open the databases Property and choose Options, nbavigate to Service Borker and set Broker Enabled to enable (alternatively you can use the TSQL statement ALTER DATATABLE [database_name] SET ENABLE_BROKER)

			Instances of D3::DatabaseAlertManager rely on other objects for much of the work:

			* apal3::SQLServerDBAlertThread: The thread inherits from both, D3::DatabaseAlertManager and apal3::Thread.
			* D3::MetaDatabaseAlert: One object will be created at system start for each <Alert> element encountered uiner a meta databases <Alerts> element in the XML initialisation file
			* D3::DatabaseAlert: These objects are bound to a D3::MetaDatabaseAlert and created automatically when you call apal3::SQLServerDBAlertThread::AddAlertListener

			Usage:

			-# create DatabaseAlertManager
			-# DatabaseAlertManager::AddAlertListener(call for any alert you're interested in)
			-# DatabaseAlert::MonitorDatabaseAlerts(): Call this useing a separate thread

			Note: If this already has a DatabaseAlert with this name:
						- the Listener will be added
						- the timeout for the DatabaseAlert will be set to iTimeout if iTimeout is > 0
						If not, it will create and register an alert and:
						- Add the passed in Listener
						- set the timeout to iTimout if > 0 or to the max timeout for alerts under the given RDBMS
	*/
	class D3_API DatabaseAlertManager
	{
		friend class MetaDatabase;
		friend class Database;
		friend class ODBCDatabase;
		friend class OTLDatabase;
		friend class MetaDatabaseAlert;
		friend class DatabaseAlert;

		public:
			//! Listener frequence
			enum Frequency
			{
				recurring,		//!< repeat
				once					//!< stop after first notification
			};

		protected:
			MetaDatabasePtr						m_pMetaDatabase;					//!< This DatabaseAlertManager processes alerts for this database type only
			DatabaseWorkspacePtr			m_pDBWS;									//!< The database workspace this uses
			DatabasePtr								m_pDatabase;							//!< The database that is used to process alerts (of type m_pMetaDatabase)
			DatabaseAlertPtrMap				m_mapDatabaseAlert;				//!< The map contains all DatabaseAlert objects keyed by alert name
			bool											m_bAlertsInitialised;			//!< If true, database alert processing has been successfully enabled on the database of type m_pMetaDatabase in this' database workspace and will need to be disabled before deleting this
			bool											m_bIsMonitoring;					//!< If true, MonitorDatabaseAlerts is pending
			bool											m_bShouldStop;						//!< If true, should not register new alerts and stop processing on the first oportunity (set by Terminate())
			std::string								m_strAlertService;				//!< Name that is used for the SQL Server alert service and queue

		public:
			//! Standard constructor
			DatabaseAlertManager(MetaDatabasePtr pMetaDatabase);
			//! Standard destructor
			~DatabaseAlertManager();

			//! Add an alert
			/*! Clients typically do:
					-# create DatabaseAlertManager
					-# DatabaseAlertManager::AddAlertListener(): call for any alert you're interested in
					-# DatabaseAlert::MonitorDatabaseAlerts(): Call this useing a separate thread

					Note: If this already has a DatabaseAlert with this name:
								- the Listener will be added
								- the timeout for the DatabaseAlert will be set to iTimeout if iTimeout is > 0
								If not, it will create and register an alert and:
								- Add the passed in Listener
								- set the timeout to iTimout if > 0 or to the max timeout for alerts under the given RDBMS
			*/
			void											AddAlertListener(const std::string & strAlertName, DatabaseAlertListenerFunc, int iTimeout = 0, Frequency frequency = recurring);
			//! Remove an alert listener
			void											RemoveAlertListener(const std::string & strAlertName, DatabaseAlertListenerFunc);
			//! Disable an alert listener
			void											DisableAlertListener(const std::string & strAlertName, DatabaseAlertListenerFunc);
			//! Disable an alert listener
			void											EnableAlertListener(const std::string & strAlertName, DatabaseAlertListenerFunc);

			//! Terminate: call this method if you wish the DatabaseAlertManager to terminate
			/*! Typically, you call MonitorDatabaseAlerts in a specific therad and that call will not
					return until a message is received. If you want to stop waiting, you should call
					this method from another thread. While the method will return immediately, the thread
					doing the MonitorDatabaseAlerts() may not return immediately. You should continue
					until it returns before you destroy this.
			*/
			void											Terminate();

			//! MonitorDatabaseAlerts does not return until an alert has been received
			/*! This method should be called in its own thread as it will not return until an event fires.
					The method may return NULL. If it does, it indicates some error. You should be careful and not
					simply invoke this method again if that happens. A better way would be to let the thread sleep
					for a small while before calling this again. If it returns NULL a certain number of times, you
					should stop processing as there is most likely a problem with the system itself.

					If a valid alert is received, DatabaseAlertPtr contains all the inormation received. By the
					time the method returns these post-conditions are met:

					-# if the alert is enabled, all enabled listeners will have been notfied
					-# any listeners which where notified and only require one notification will have been deleted
					-# if the underlying RDBMS requires re-registration alter an alert, the alert will be marked for reregistration and registered again when this method is called
			*/
			DatabaseAlertPtr					MonitorDatabaseAlerts();

			//! Returns true if this is monitoring alerts
			bool											IsMonitoringAlerts();

			//! Returns a unique name for an alert service and queue (useful for SQL Server)
			const std::string &				AlertServiceName();

		protected:
			//! Return true if this has pending alerts
			bool											HasPendingAlerts();
			//! Set's this monitoring flag (used inernally by MonitorDatabaseAlerts())
			void											SetMonitoringFlag(bool bMonitoring);
			//! GetDatabaseAlert with the given name
			DatabaseAlertPtr					GetDatabaseAlert(const std::string & strAlertName);
			//! Notification sent when a new DatabaseAlert based on this is instantiated
			void											On_DatabaseAlertCreated(DatabaseAlertPtr pDBAlert);
			//! Notification sent when a DatabaseAlert based on this is being deleted
			void											On_DatabaseAlertDeleted(DatabaseAlertPtr pDBAlert);
			//! Gives this a chance to do some work in a threadsafe way before entering the blocking Database::MonitorDatabaseAlerts call
			void											On_BeforeMonitorDatabaseAlerts();
			//! Gives this a chance to do some work in a threadsafe way after returning from the blocking Database::MonitorDatabaseAlerts call
			void											On_AfterMonitorDatabaseAlerts(DatabaseAlertPtr & pDBAlert);

			//! The mutex that prevents concurrent access to this' resources
			virtual boost::recursive_mutex &		GetMutex() = 0;

	};



	//! This class describes alerts that can be intercepted by a database objects WaitForAlerts method
	/*! A MetaDatabaseAlert is an object that has these primary attributes:

			- a query statement (only relevant for SQL Server):

				The statement must follow the rules outlined here:

					http://msdn.microsoft.com/en-us/library/ms181122.aspx

				A query could be: "SELECT A,B,C FROM dbo.TBL" (note the dbo. part which is mandatory).
				Registering a query like that is a bit like obtaining a resultset and telling SQL Server
				to send a notification if any transaction on the server renders the resultset out-of-date.
				IOW, any DELETE, INSERT or an UPDATE that involves one of the columns A,B or C on table TBL
				causes SQL Server to post a notification.

			- a timeout: an integer value representing seconds.

				Special Notes for SQL Server: In theory, if you register a listener with
				a SQLServerDBAlertThread, the MetaDatabaseAlert will receive a timeout notification after this number of
				seconds have lapsed. Unfortunately, I discovered that this is not a particularly reliable
				setting. The minimum timeout I ever managed to achieve is 65 seconds (all under SQL Server
				2005 Express). I have experienced timeouts which suggest that whatever you specify as timeout
				is interpreted by SQL Server as the following "realtimeout" that SQL Server uses:

					realtimeout = ((timeout / 60) + 1) * 65

			- a name: The name identifies the alert. If you have multiple alerts, you must
				make sure that the names are unique. Any alerts returned by the RDBMS are
				identified by this name.

				The name is what the SQL Server should report. This can be anything you
				like, important is only that no two MetaDatabaseAlerts registered against the same SQLServerDBAlertThread
				specify the same name. The reason is that teh SQLQNSerice uses this name to identify
				which listener should receive the notification.

	*/
	class D3_API MetaDatabaseAlert
	{
		friend class DatabaseAlert;
		friend class MetaDatabase;
		friend class Database;

		protected:
			MetaDatabasePtr				m_pMetaDatabase;	//!< The MetaDatabase this belongs to
			std::string						m_strName;				//!< The name to look out for (see class spec for details)
			std::string						m_strSQL;					//!< The SQL query (see class spec for details)
			DatabaseAlertPtrList	m_listAlerts;			//!< All actual instances of this (not currently used for anything)

			//! Only MetaDatabase::InitialiseMetaAlert create instances
			MetaDatabaseAlert(MetaDatabasePtr pMetaDatabase, const std::string& strName, const std::string& strSQL);
			//! Only MetaDatabase objects delete these
			~MetaDatabaseAlert();

		public:
			//! Get the name
			const std::string&		GetName() const { return m_strName; }

		protected:
			//! Get the SELECT statement to watch
			const std::string&		GetSQL() const	{ return m_strSQL; }
			//! returns true if pDBAlert is already known to this
			bool									HasDatabaseAlert(DatabaseAlertPtr pDBAlert);
			//! Notification sent when a new DatabaseAlert based on this is instantiated
			void									On_DatabaseAlertCreated(DatabaseAlertPtr pDBAlert);
			//! Notification sent when a DatabaseAlert based on this is being deleted
			void									On_DatabaseAlertDeleted(DatabaseAlertPtr pDBAlert);
	};






	//! Instances of this class are actual alerts based on a specific MetaDatabaseAlert
	/*! A DatabaseAlert is an object the represents an actual instance of an MetaDatabaseAlert
			Each instance of an alert belongs to a Database instance and is based on a MetaDatabaseAlert.
			Instances have these features:

			- Listeners: Each DatabaseAlert can have many listeners (call-back function). When
				an alert fires, each listeners call back function is invoked. The function
				receives this as the only argument. The interface to add/remove a listener is
				AddListener() and RemoveListener(). The AddListener method allows you to
				specify a frequency (recurring or once). If once is specified, the listener
				will be removed after the first notification.

			- Timeout: A DatabaseAlert can have a timeout. The default is the maximum time
				allowed by the RDBMS in use (refer to the RDBMS documentation on Alerts).
				If you call Database::MonitorDatabaseAlerts() and no alert is received in
				the timeout period (in seconds), the alert should fire with a timeout notification.

			- Notification Details: During notification (i.e. when the callback functions
				are invoked), this will have a valid GetState(). The GetState() is one of -1 (error),
				0 (normal) or 1 (timeout).

			- Enabled/Disabled: A DatabaseAlert can be enabled or disabled. In a disabled
				state, any alerts received are ignored.

			- Registered/Unregistered: After creating a DatabaseAlert, it must be registered via
				Database::RegisterDatabaseAlert() before a call to Database::MonitorDatabaseAlerts()
				will have any effect.

			\note
			Clients typically call DatabaseAlterManager::AddAlertListener to trap events which
			makes it unecessary for clients to deal with DatabaseAlter objects directly except
			when checking the type and state of the alert when such objects are passed to the alter
			notification handler function.
	*/
	class D3_API DatabaseAlert
	{
		friend class DatabaseAlertManager;
		friend class MetaDatabaseAlert;
		friend class MetaDatabase;
		friend class Database;
		friend class ODBCDatabase;
		friend class OTLDatabase;

		protected:
			//! The listener struct simply stores listeners and the type
			struct Listener
			{
				friend class DatabaseAlertManager;

				DatabaseAlertListenerFunc							pfnCallBack;	//<! Call this when an alert fires
				DatabaseAlertManager::Frequency				frequency;		//<! once: throw away listener after first alert, recurring = call this again on next alert
				bool																	bEnabled;			//<! true: call callback, false: do not call callback

				Listener(DatabaseAlertListenerFunc pfn, DatabaseAlertManager::Frequency f) : pfnCallBack(pfn), frequency(f), bEnabled(true) {}
			};

			// Internal types
			typedef Listener*								ListenerPtr;
			typedef std::list<Listener>			ListenerList;
			typedef ListenerList::iterator	ListenerListItr;

			DatabaseAlertManagerPtr	m_pDatabaseAlertManager;	//!< The DatabaseAlertManager that deals with this
			MetaDatabaseAlertPtr		m_pMetaDatabaseAlert;			//!< This is an  instance of this MetaDatabaseAlert
			std::string							m_strMessage;							//!< The last message received, only relevant during notification
			int											m_iTimeout;								//!< Receive a notification either after this time has elapsed or when such a name was received (default is 65 seconds - the min imum)
			bool										m_bRegistered;						//!< If true, the DatabaseAlert has been successfully registered
			bool										m_bEnabled;								//!< If true, the alert will not notify
			ListenerList						m_listListeners;					//!< The list of listeners which must be notified when the event fires
			int											m_iStatus;								//!< The status is only relevant when Database::MonitorDatabaseAlerts returns an object. If errors, m_status is < 0, if normal notification m_iStatus is 0 and if timeout is signaled, m_iStatis is > 0

			//! Only DatabaseAlertManager objects create instances
			DatabaseAlert(DatabaseAlertManagerPtr pDatabaseAlertManager, MetaDatabaseAlertPtr pMetaDatabaseAlert, int iTimeout = 0);;
			//! Only DatabaseAlertManager objects delete instances
			~DatabaseAlert();

		public:
			//! Get the name
			const std::string&		GetName() const { return m_pMetaDatabaseAlert->GetName(); };
			//! Get the last message (useful only when processing an alert notification)
			const std::string&		GetMessage() const { return m_strMessage; };
			//! Get the status (useful only when processing an alert notification)
			int										GetStatus() const { return m_iStatus; };

		protected:
			//! Get the name
			const std::string&		GetSQL() const		{ return m_pMetaDatabaseAlert->GetSQL(); };
			//! Set the status
			void									SetStatus(int i)	{ m_iStatus = i; }
			//! Set the timeout
			void									SetTimeout(int i)	{ m_iTimeout = i; }
			//! Returns true if this has listeners
			bool									HasListeners()		{ return !m_listListeners.empty(); }
			//! Returns true if this is registered
			bool									IsRegistered()		{ return m_bRegistered; }
			//! Returns true if this is enabled
			bool									IsEnabled()				{ return m_bEnabled; }
			//! Disable (while disabled, notifications are not passed on)
			void									Disable()					{ SetEnabled(false); }
			//! Enable (while enabled, notifications invoke registered listener callbacks)
			void									Enable()					{ SetEnabled(true); }

			//! The public can add a listener.
			/*! When the alert is received, the passed in listener will be executed. The listener function has this footprint:

					void	listener(DatabaseAlert* pAlert, const std::string& strMessage);

					The strMessage may or may not contain anything useful. In Oracle,
					it is the dbms_alert.signal that is typically called in an update, insert or delete
					trigger determines what the message is. So if your listener was called
					as a result of a trigger calling dbms_alert.signal("AlertName", "Test"),
					then inside your listener function this condition is true:
						pAlert->GetName() == "AlertName" && pAlert->GetMessage() == "Test"

					In SQL Server the message is one of Insert, Delete, Update or Truncate.
			*/
			void									AddListener(DatabaseAlertListenerFunc, DatabaseAlertManager::Frequency eFrequency = DatabaseAlertManager::recurring);
			//! The public can remove a listener
			void									RemoveListener(DatabaseAlertListenerFunc);
			//! Disable a listener (keeps it but will not notify it)
			void									DisableListener(DatabaseAlertListenerFunc);
			//! Enable a listener
			void									EnableListener(DatabaseAlertListenerFunc);

			//! Received if an SQL Server Query Notification has been received
			void									NotifyListeners();
			//! Helper to enable/disable a DatabaseAlert
			void									SetEnabled(bool bEnabled)		{ m_bEnabled = bEnabled; }
			//! Returns the listener with the given callback function (or NULL if unknown)
			ListenerPtr						GetListener(DatabaseAlertListenerFunc);
	};






	//! Each object of this class describes all the elements of a physical database they provide access to.
	/*! One instance of this class is generated automatically at startup: the MetaDictionary. However, the
			MetaDictionary must be initialised explicitly by an application by calling:

			InitMetaDictionary(MetaDatabaseDefinition & MD);

			The MetaDictionary MetaDatabase object provides access to a physical database containing Meta Data
			describing other physical databases.

			Assume that your application needs access to two different physical databases, an Oracle database
			and an SQLServer database. You start off by creating a Meta Data database as follows:

			\code
			#include "D3.h"
			#include "Exception.h"
			#include "Database.h"

			using namespace D3;

			ClassPtrList			Class::M_clsTbl;		// Must initialise D3 Class model explicitly!
			ExceptionContext	gEC("MyApp.log");		// Must have a global exception context (we log to file MyApp.log)


			int main(int argc, char* argv[])
			{
				MetaDatabaseDefinition		MD;
				MetaDatabasePtr						pDictionary;

				// Correctly fill MD
				MD.m_strServer						= "MyServer"
				MD.m_strUserID						= "MyUserID"
				MD.m_strUserPWD						= "MyPassword"
				MD.m_strName							= "MyAppMetaData";
				MD.m_strDataFilePath			= "c:\\MSSQL7\\data\\MyAppMetaData.mdf";
				MD.m_strLogFilePath				= "c:\\MSSQL7\\data\\MyAppMetaData.ldf";
				MD.m_eTargetRDBMS					= SQLServer;
				MD.m_iInitialSize					= 10;					// 10MB
				MD.m_iMaxSize							= 1000;				// 1GB
				MD.m_iGrowthSize					= 10;					// 10MB
				MD.m_pInstanceClass				= (const Class*) &ODBCDatabase::ClassObject();
				MD.m_strInstanceInterface = "DatabaseI";

				try
				{
					// Always initialise the metaDictionary first
					//
					if (MetaDatabase::InitMetaDictionary(MD))
					{
						pDictionary = MetaDatabase::GetMetaDictionary();

						if (pDictionary)
						{
							// Create a physical database which we can populate later with data
							// describing the databases the final application will have to deal with
							//
							pDictionary->CreatePhysicalDatabase();

							// Create source code (see MetaDatabase::CreateSourceCode())
							//
							pDictionary->CreateSourceCode();
						}
					}
				}
				catch (Exception & e)		// catch D3 exceptions
				{
					e.LogError();	// Write the error to the default exception context gEC
												// and do error clean up
				}

				return 0;
			}
			\endcode

			When you run the above program, D3 will have generated a new SQLServer database on
			server \a MyServer called \a MyAppMetaData. This is a special database and any D3
			application will require exactly one instance of such a database for the purpose
			of configuring the application at startup.

			You now use any tool you like (including D3 itself) to add data to this database
			which describes the database(s) that you will	be using.	For example, if you need
			to document two databases, an Oracle database (MyOracleDB) and an SQLServer database
			(MySQLServerDB), you add relevant data describing both databases to the meta
			data database.

			Once you have a correctly setup and populated meta database, you can replace the code
			fragmet in the above code:

			\code
						pDictionary = MetaDatabase::GetMetaDictionary();

						if (pDictionary)
						{
							// Create a physical database which we can populate later with data
							// describing the databases the final application will have to deal with
							//
							pDictionary->CreatePhysicalDatabase();

							// Create source code (see MetaDatabase::CreateSourceCode())
							//
							pDictionary->CreateSourceCode();
						}
			\endcode

			with the following code fragment:

			\code
						// Load data from the meta database. This will have the effect that new
						// D3 meta objects will be created and made available to clients thereafter.
						//
						if (MetaDatabase::LoadMetaDictionaryData())
						{
							DatabaseWorkspace		myDBWS;
							DatabasePtr					pMyOracleDB = myDBWS.GetDatabase("MyOracleDB");
							DatabasePtr					pMySQLServerDB = myDBWS.GetDatabase("MySQLServerDB");

							// ... now work with pMyOracleDB and pMySQLServerDB
						}
			\endcode

			Provided your meta data database correctly documents the "MyOracleDB" and
			"MySQLServerDB" databases, the code above will succeed. When myDBWS goes out
			of scope, all workspace databases will be released also.

			Each MetaDatabase object also maintains one global DatabaseWorkspace object.
			This workspace is created the first time a client request an instance of this
			threough a call to CreateInstance(). If this object has any MetaEntoity objects
			which are marked as cached (Cached() == true), the system automatically fetches
			all rows from the physical database and caches these in this DatabaseWorkspace.

			Such cached objects remain in memory until the MetaDatabase is destroyed, i.e.
			at system shut-down time.
	*/
	class D3_API MetaDatabase : public Object
	{
		friend class MetaEntity;				//!< Sends protected notifications
		friend class Database;					//!< Sends protected notifications
		friend class ODBCDatabase;			//!< Sends protected notifications
		friend class OTLDatabase;				//!< Sends protected notifications
		friend class MetaDatabaseAlert;	//!< Accesses m_mtxExclusive

		D3_CLASS_DECL(MetaDatabase);

		public:
			//! This class implements a bitmask specifying database specific characteristics
			struct Flags : public Bits<szMetaDatabaseFlags>
			{
				BITS_DECL(MetaDatabase, Flags);

				//@{ Primitive masks
				static const Mask HasVersionInfo;			//!< 0x00000001 - Column can't be NULL
				static const Mask HasObjectLinks;			//!< 0x00000002 - Column's value is generated by the RDBMS
				//@}

				//@{ Combo masks
				static const Mask Default;						//!< Default is HasVersionInfo
				//@}
			};


			//! This class implements a bitmask specifying database specific permissions
			struct Permissions : public Bits<szMetaDatabasePermissions>
			{
				BITS_DECL(MetaDatabase, Permissions);

				//@{ Primitive masks
				static const Mask Create;							//!< 0x00000001 - Database can be created
				static const Mask Read;								//!< 0x00000002 - Database can be read
				static const Mask Write;							//!< 0x00000004 - Database can be written
				static const Mask Delete;							//!< 0x00000004 - Database can be deleted
				//@}

				//@{ Combo masks
				static const Mask Default;						//!< Default is Create | Read | Write | Delete
				//@}
			};

		protected:
			//! This class can be used by subclasses to ensure transactions are stricly serialized
			class TransactionManager
			{
				friend class MetaDatabase;

				protected:
					boost::recursive_mutex								m_mtxBeginTransaction;		//!< Used only in BeginTransaction
					boost::recursive_mutex								m_mtxAccessTransaction;		//!< Used during transactions, blocks access to m_pTransactionInfo

					MetaDatabasePtr												m_pMD;										//!< This manager serializes transactions for instances of this meta database only
					DatabasePtr														m_pDB;										//!< The Database object that owns the transaction
					long																	m_lThreadID;							//!< The ID of the thread that started the BeginTransaction
					int																		m_iCount;									//!< Increments each time BeginTransaction is called during a transaction and decrements each time Commit or RollbackTransaction is called
					boost::recursive_mutex::scoped_lock*	m_pLock;									//!< If not NULL, is an exclusive lock on m_mtxBeginTransaction
					bool																	m_bRollback;							//!< Increments each time BeginTransaction is called during a transaction and decrements each time Commit or RollbackTransaction is called
					time_t																m_tLastAccessed;					//!< Number of seconds elapsed since midnight (00:00:00), January 1, 1970 when this was last accessed (set by SessionI::Touch())

				public:
					//! Simple constructor
					TransactionManager() : m_pMD(m_pMD), m_pDB(NULL), m_iCount(0), m_pLock(NULL), m_bRollback(false)	{}

					//! Call this method in BeginTransaction
					/*! This method does not return until a transaction can be established.
							If the database already has a transaction, it simply increments the counter.
							You must hang on to the lock this method returns until all your work during the BeginTransaction operation is done.
					*/
					boost::recursive_mutex::scoped_lock*	BeginTransaction(DatabasePtr pDB);

					//! Call this method whenever you want to work with a transaction or to check if the database has a tranasaction
					/*! If this method returns NULL, the database does not have a transaction and you must not do any transactional work.
							If this method returns a lock, hang on to it until you're done with the work and then delete the lock.
					*/
					boost::recursive_mutex::scoped_lock*	UseTransaction(DatabasePtr pDB);

					//! Call this method in CommitTransaction
					/*! If this method returns NULL, the database does not have a transaction and you must not do any transactional work.
							If this method returns a lock, hang on to it until you're done with the work and then delete the lock.
					*/
					boost::recursive_mutex::scoped_lock*	CommitTransaction(DatabasePtr pDB)			{ return EndTransaction(pDB, false); }

					//! Call this method in RollbackTransaction
					/*! If this method returns NULL, the database does not have a transaction and you must not do any transactional work.
							If this method returns a lock, hang on to it until you're done with the work and then delete the lock.
					*/
					boost::recursive_mutex::scoped_lock*	RollbackTransaction(DatabasePtr pDB)		{ return EndTransaction(pDB, true); }

					//! Call this method during the actual CommitTransaction or RollbackTransaction management
					/*! Because you may have nested transaction, you must not complete a transaction until this method returns true.
							If the method returns tru and you are processing a CommitTransaction request, make sure you also check
							the return value of ShouldRollback and if it returns true, you rollback the transaction instead of committing it
					*/
					int																		GetTransactionLevel(DatabasePtr pDB);

					//! Call this method while handling a CommitTransaction
					/*! If the method returns a value of less than 1, the actual transaction has finished and can be committed or rolled back.
							Since it is possible that a nested Transaction requested a roll back which wasn't actioned on because the rollback
							request was in a nested transaction, you must call ShouldRollback and if it returns true roll the transaction
							back instead of committing it.
					*/
					bool																	ShouldRollback(DatabasePtr pDB);

					//! Kill timed out transaction
					/*! This method forecfully terminates a pending transaction if it has been last accessed more
							than uiTransactionTimeout secs ago.
					*/
					void																	KillTimedOutTransaction(unsigned int uiTransactionTimeout);

				protected:
					//! Helper for BeginTransaction which does the actual work
					boost::recursive_mutex::scoped_lock*	CreateTransaction(DatabasePtr pDB);
					//! Helper which does the actual work for the Commit and RollbackTransaction methods
					boost::recursive_mutex::scoped_lock*	EndTransaction(DatabasePtr pDB, bool bRollback);
			};



			static DatabaseID									M_uNextInternalID;			//!< If no internal ID is passed to the constructor, use this and decrement afterwards (used for meta dictionary definition meta obejcts)
			static MetaDatabasePtrMap					M_mapMetaDatabase;			//!< All MetaDatabase objects in the system
			static MetaDatabasePtr						M_pDictionaryDatabase;	//!< The Mother of all MetaDatabase objects
			static DatabaseWorkspacePtr				M_pDatabaseWorkspace;		//!< The global workspace which stores all cached objects

			//! This is the ID of this so that (MetaDatabase::GetMetaDatabaseByID(this->m_uID) == this)
			/*! Note: If this member is < MetaDatabase::M_uNextInternalID it is the ID of the physical object in the database)
			*/
			DatabaseID												m_uID;
			TargetRDBMS												m_eTargetRDBMS;					//!< The RDBMS type
			std::string												m_strDriver;						//!< The name of the driver (only relevant for ODBC DSN)
			std::string												m_strServer;						//!< The name of the server
			std::string												m_strUserID;						//!< The name of the user to use for connections
			std::string												m_strUserPWD;						//!< The password of the user to use for connections
			std::string												m_strName;							//!< The actual database name
			std::string												m_strDataFilePath;			//!< The path of the data file on the server
			std::string												m_strLogFilePath;				//!< The path of the log file on the server
			int																m_iInitialSize;					//!< The initial size of the database in MB
			int																m_iMaxSize;							//!< The maximum size of the database in MB
			int																m_iGrowthSize;					//!< The growth size of the database in MB

			//! The alias for the database.
			/*! An alias provides an alternative to addressing a specific database. For example,
					in different implementations, the physical database name for say the standard APAL3
					database my be different. E.g. one client may want the database to be called
					P3T3_DB_301 and another DB_P3T3, etc. While the actual database name reflects the
					physical name, the alias for both implementations can be the same.
			*/
			std::string												m_strAlias;

			const Class*											m_pInstanceClass;				//!< The const Class* which knows how to create the crrect instances type
			std::string												m_strInstanceInterface;	//!< The name of the interface class that can wrap an instance of this type
			std::string												m_strProsaName;					//!< A user friendly name
			std::string												m_strDescription;				//!< A description of the entity
			std::string												m_strConnectionString;	//!< A connection string that instances of this meta type can use to connect

			unsigned int											m_uVersionMajor;				//!< Instances of this class can only work with instances of actual databases where the most recent row in the D3 version info table has a matching version major (only checked if HasVersionInfo() is true)
			unsigned int											m_uVersionMinor;				//!< Instances of this class can only work with instances of actual databases where the most recent row in the D3 version info table has a matching version minor (only checked if HasVersionInfo() is true)
			unsigned int											m_uVersionRevision;			//!< Instances of this class can only work with instances of actual databases where the most recent row in the D3 version info table has a matching version revision (only checked if HasVersionInfo() is true)

			Flags															m_Flags;								//!< See MetaDatabase::Flags above
			Permissions												m_Permissions;					//!< See MetaDatabase::Permissions above

			DatabasePtrList										m_listDatabase;					//!< All Database instances based on this
			MetaEntityPtrVect									m_vectMetaEntity;				//!< All MetaEntity objects for this MetaDatabase
			MetaRelationPtrList								m_listRootRelation;			//!< All MetaRelation objects for this MetaDatabase which are system entry points

			bool															m_bInitialised;					//!< Initially false. Once this is correctly initialised, changes to true. Once true clients can call CreateInstance().
			bool															m_bVersionChecked;			//!< If HasVersionInfo() is true, the first time an instance connects it should check D3 version info. See also member VersionChecked().

			MetaDatabaseAlertPtrMap						m_mapMetaDatabaseAlert;	//!< This map stores database alert templates by name

			boost::recursive_mutex						m_mtxExclusive;					//!< This Mutex is used to serialise modifications to m_listDatabase

			TransactionManager								m_TransactionManager;		//!< Use to serialise transactions between m_listDatabase members (use GetTransactionManager to access this)
			MetaEntityPtrList									m_listMEDependency;			//!< Contains all of this meta entity objects ordered by their dependency

			D3TimeZone												m_TimeZone;							//!< The TimeZone details for the server that hosts the database. If missing defaults to the time zone of the system running the binary.

			std::string												m_strHSTopicsJSON;			//!< JSON string containing an array of help topics associated with this

			//! Protected ctor()'s (this class creates instances by itself)
			/*! You will never create instances of this class manually. Instead, you will
					correctly populate a MetaDatabaseDefinition object and pass this to the
					static function:

					InitMetaDictionary(MetaDatabaseDefinition & MD);

					The MetaDatabaseDefinition object will contain all the details necessary so
					that the static LoadMetaDictionaryData() method can access a physical database containing
					meta data for all databases needed by an implementation and instantiate the
					corresponding D3 meta objects.
			*/
			MetaDatabase(DatabaseID uID = M_uNextInternalID--);

		public:
			//! The destructor closes all instances of this and deletes all it's associated meta objects.
			/*!	While the destructor attempts to do a proper clean up, you should not rely on this
					feature. A much better approach is to ensure that all DatabaseWorkspace objects are closed
					prior to destroying a MetaDatabase object.
			*/
			~MetaDatabase();


			//! Call this before you do anything else with this class (unregisteration happens automatically on exit)
			static bool									RegisterClassFactories();

			//! The MetaDictionary is automatically built at system startup and contains the database structure for databases containing meta data.
			/*! You may want access to the MetaDictionary particularly after modifying it
					so that you can:

					- generate a new meta data Database (CreatePhysicalDatabase())

					- generate a new MetaDictionary header file (CreateCPPHeader)

					You should under no circumstances make structural changes to the returned object!
			*/
			static MetaDatabasePtr			GetMetaDictionary()								{ return M_pDictionaryDatabase; }

			//! Get the MetaDatabase object with the name specified.
			/*! The system will search it's global list of MetaDatabase objects firstly for one
					with a matching physical name. If this fails, it will search for one with a
					matching alias.
			*/
			static MetaDatabasePtr			GetMetaDatabase(const std::string & strName);

			//! Get the MetaDatabase object at the specified index
			/*! The system stores databases in a map keyed by the database alias. The index here is
					equivalent with the position of the item in the map.
			*/
			static MetaDatabasePtr			GetMetaDatabase(unsigned int idx);

			//! Get the MetaDatabase object that matches the passed in MetaDatabaseDefinitions
			/*! The system will search it's global list of MetaDatabase objects for one
					that matches the spoecified Alias, Name and Version.
			*/
			static MetaDatabasePtr			GetMetaDatabase(MetaDatabaseDefinition & MD);

			//! Initialises the meta dictionary database.
			/*!	Before you can work with D3 objects, you MUST make a call to this static
					method and provide the meta data Database details in the form of a
					MetaDatabaseDefinition object to the method.

					The method initialises the Dictionary MetaDatabase object which is the key
					to the entire meta model. It also creates a global DatabaseWorkspace.

					The meta dictionary database is a MetaDatabase which is initialised by a call
					to this function. Whatever you want to do with D3, you must call this static
					method before you do anything else with D3.

					To ensure you get proper error information from D3, you should setup a global
					ExceptionContext before you call this method.

					The method simply initialises the Meta Dictionary MetaDatabase object by creating
					it's fixed D3 meta objects. It does not interact with the physical meta data
					database which should be specified in the MetaDatabaseDefinition.

					Before your application terminates, you must call the UnInitialise() member!
			*/
			static void									Initialise(MetaDatabaseDefinition & MD);

			//! UnInitialises the meta dictionary database.
			/*!	Call this static method before your application exits to free resources]
					needed by the Dictionary.

					\note This method not only destroys the Dictionary MetaDatabase, it destroys
					all MetaDatabase objects in the system. In turn, each MetaDatabase object that
					is being destroyed will also destroy all its Database instances. This may
					have undesirable side effects.
					For this reason, a good approach is to ensure that all DatabaseWorkspace
					objects in use by D3 clients are deleted beofore calling UnInitialise().

					\todo Currently this method calls odbc::DriverManager::shutdown() which is
					a bit ugly. The problem is any other place is not quite right and this works
					find as long as D3 clients do not use odbc++ directly in which case this may
					cause problems.
			*/
			static void									UnInitialise();

			//! Returns a list of all MetaDatabases in the system.
			static MetaDatabasePtrMap&	GetMetaDatabases()								{ return M_mapMetaDatabase; };

			//! Loads meta data from the MetaDictionary MetaDatabase and creates corresponding D3 meta objects.
			/*!	You must call #InitMetaDictionary() before you call this method. The meta data database (there
					should be exactly one and no more for each D3 application) contains descriptions of all physical
					databases a D3 application needs to access at run-time.

					\note It is perfectly legal to have mutliple versions of the same database definitions stored
					in the MetaDictionary. Furthermore, D3 does not know what RDBMS to use, where the database resides
					physically nor how to access it. This is why you need to populate a MetaDatabaseDefinition
					structure with all the relevant details for each custom database you wish to access. This method
					will then attempt to load meta information from the MetaDictionary corresponding to the each
					MetaDatabaseDefinition structure you pass inside listMDDefs.

					If bLoadHelp is true (default), the D3HSDB database (if specified) will be scand for any help topics
					associated with the meta objects loaded. If D3 is used for maintenance purpose (i.e. to create and
					restore databases, bLoadHelp should be false).
			*/
			static void									LoadMetaDictionary(MetaDatabaseDefinitionList & listMDDefs, bool bLoadHelp = true);

			//! Called by #LoadMetaDictionary().
			/*!	The method expects that the object passed in is an instance of the D3MetaDatabase
					MetaEntity and constructs a new MetaDatabase object based on pD3MDB which acts as
					a template.
					The method propagates the construction process to MetaEntity.
			*/
			static void									LoadMetaObject(D3MetaDatabasePtr pD3MDB, MetaDatabaseDefinition & aMDDef);

			//! Initialise Alerts
			/*! This method expects an XML Node as input which has the format:

					<Alerts>
						<Alert>
							<Name>RBAC Changed</Name>
							<SQLQuery>
								SELECT ID, Name, Enabled, Features FROM dbo.D3Role;
								SELECT ID, Name, Password, Enabled FROM dbo.D3User;
								SELECT RoleID, UserID FROM dbo.D3RoleUser;
								SELECT RoleID, MetaDatabaseID, AccessRights FROM dbo.D3DatabasePermission;
								SELECT RoleID, MetaEntityID, AccessRights FROM dbo.D3EntityPermission;
								SELECT RoleID, MetaColumnID, AccessRights FROM dbo.D3ColumnPermission
							</SQLQuery>
						</Alert>
					</Alerts>

					Note: The SQLQuery element is only relevant for SQL Server.

					The method creates a DatabaseAlert object for each Alert
					element found and adds it to this' list of alerts.

					Clients can then call:
						dbAlert = MetaDatabase::GetAlert("RBAC Changes");
						dbAlert.AddListener(UpdateRBACObjects);

					See DatabaseAlert for more details.
			*/
			void												InitialiseMetaAlerts(CSL::XML::Node* pAlerts);

			//! Create a physical database structure reflecting this MetaDatabase.
			MetaDatabaseAlertPtr				GetMetaDatabaseAlert(const std::string& strName);

			//! Create a physical database structure reflecting this MetaDatabase.
			/*! This method attempts to create the physical database this object represents.
					If the database already exists, it will be deleted. If successful, the database
					will contain no actual data. However, the method will have created tables,
					constraints, indexes and triggers.
					\note For this method to suceed, no instances of this MetaDatabase can exist.
			*/
			bool												CreatePhysicalDatabase();

			//! Create ako Database object based on this MetaDatabase.
			/*! The first time this method is called, the system
					will automatically create another database, the global database m_pGlobalDB, The
					global DB will create and make resident Entity objects associated with MetaEntity
					objects of this MetaDatabase marked as cached for each row in the respective table.
			*/
			DatabasePtr									CreateInstance(DatabaseWorkspacePtr pDatabaseWorkspace);

			//! Returns this' global database.
			/*! Each MetaDatabase object maintains a global database object. The global database
					object is created automatically the first time a client requests an instance of
					this through CreateInstance(). D3 uses the global database to automatically
					fetch and cache all instances of associated MetaEntity objects marked as cached.
			*/
			DatabasePtr									GetGlobalDatabase();

			//! Returns a list of all instances of this.
			/*! \note Depending on your application, the list can contain transient instances.
					Therefore, this method should be used with utmost care.
			*/
			DatabasePtrList&						GetDatabases()										{ return m_listDatabase; }

			//! GetID returns a unique identifier which can be used to retrieve the same object using GetMetaDatabaseByID(unsigned int)
			DatabaseID									GetID() const											{ return m_uID; }
			//! Return the meta relation where GetID() == uID
			static MetaDatabasePtr			GetMetaDatabaseByID(DatabaseID uID)
			{
				MetaDatabasePtrMapItr	itr = M_mapMetaDatabase.find(uID);
				return (itr == M_mapMetaDatabase.end() ? NULL : itr->second);
			}

			//! Returns the connection string that is used to connect to the physical database through ODBC.
			const std::string &					GetConnectionString() const				{ return m_strConnectionString; }
			//! Returns the ODBC Driver Name
			/*! At present, this module does support ODBC DSN's. Instead, it has explicit knowledge how
					connection strings must be configured for the explicitly supported platforms. Therefore,
					no entries are required in ODBC.ini files.
			*/
			const std::string &					GetDriver() const									{ return m_strDriver; }
			//! Returns the Server Name
			const std::string &					GetServer() const									{ return m_strServer; }

			//! Returns the physical database name.
			const std::string &					GetName() const										{ return m_strName; }
			//! Returns the alias for the database.
			const std::string &					GetAlias() const									{ if (!m_strAlias.empty()) return m_strAlias; return GetName(); }
			//! Returns the fully qualified name of the database (for completeness only, it returns the same as GetName()).
			const std::string &					GetFullName() const								{ return m_strName; }
			//! Returns the UserID
			const std::string &					GetUserID() const									{ return m_strUserID; }
			//! Returns the User Password
			const std::string &					GetUserPWD() const								{ return m_strUserPWD; }
			//! Returns the Date File Path
			const std::string &					GetDataFilePath() const						{ return m_strDataFilePath; }
			//! Returns the Log File Path
			const std::string &					GetLogFilePath() const						{ return m_strLogFilePath; }
			//! Returns the IntialSize of Database
			int 												GetInitialSize() const						{ return m_iInitialSize; }
			//! Returns the Growth Size of Database
			int 												GetGrowthSize() const							{ return m_iGrowthSize; }
			//! Returns the Maximum Size of Database
			int 												GetMaxSize() const								{ return m_iMaxSize; }
			//! Returns the Dataabse is intialised or not
			bool 												GetInitialised() const						{ return m_bInitialised; }

			//! Returns the name meaningful to the user.
			const std::string	&					GetProsaName() const							{ return m_strProsaName; }
			//! Returns the decription for this database.
			const std::string	&					GetDescription() const						{ return m_strDescription; }

			//! Returns the database servers time zone
			D3TimeZone									GetTimeZone() const								{ return m_TimeZone; }

			//@{ Characteristics
			//! Returns this' permission bitmask
			const Flags&								GetFlags() const									{ return m_Flags; }

			//! Returns true if instances of this meta database have a [alias]D3VI table that stores version information
			bool 												HasVersionInfo() const						{ return (m_Flags & Flags::HasVersionInfo); }
			//! Sets or clears the flag that indicates if instances of this meta database have a [alias]D3VI table that stores version information
			void												HasVersionInfo(bool bEnable)			{ bEnable ? m_Flags |= Flags::HasVersionInfo : m_Flags &= ~Flags::HasVersionInfo; }
			//! Returns true if instances of this meta database have a generic object link table
			bool 												HasObjectLinks() const						{ return (m_Flags & Flags::HasObjectLinks); }
			//! Sets or clears the flag that indicates if instances of this meta database have a generic object link table
			void												HasObjectLinks(bool bEnable)			{ bEnable ? m_Flags |= Flags::HasObjectLinks : m_Flags &= ~Flags::HasObjectLinks; }
			//@}

			//@{ Permissions
			//! Returns this' permission bitmask
			const Permissions&					GetPermissions() const						{ return m_Permissions; }

			//! Returns true if instances of this can be created
			bool 												AllowCreate() const								{ return (m_Permissions & Permissions::Create); }
			//! Returns true if data can be read from instances of this
			bool 												AllowRead() const									{ return (m_Permissions & Permissions::Read); }
			//! Returns true if data can be written to instances of this
			bool 												AllowWrite() const								{ return (m_Permissions & Permissions::Write); }
			//! Returns true if instances of this can be deleted
			bool 												AllowDelete() const								{ return (m_Permissions & Permissions::Delete); }

			//! Sets the Create permission to on or off
			void 												AllowCreate(bool bEnable)					{ bEnable ? m_Permissions |= Permissions::Create : m_Permissions &= ~Permissions::Create; }
			//! Sets the Read permission to on or off
			void 												AllowRead(bool bEnable)						{ bEnable ? m_Permissions |= Permissions::Read : m_Permissions &= ~Permissions::Read; }
			//! Sets the Write permission to on or off
			void 												AllowWrite(bool bEnable)					{ bEnable ? m_Permissions |= Permissions::Write : m_Permissions &= ~Permissions::Write; }
			//! Sets the Delete permission to on or off
			void 												AllowDelete(bool bEnable)					{ bEnable ? m_Permissions |= Permissions::Delete : m_Permissions &= ~Permissions::Delete; }
			//@}

			//! Returns the Major version of the meta database.
			unsigned int								GetVersionMajor() const						{ return m_uVersionMajor; }
			//! Returns the Minor version of the meta database.
			unsigned int								GetVersionMinor() const						{ return m_uVersionMinor; }
			//! Returns the Minor version of the meta database.
			unsigned int								GetVersionRevision() const				{ return m_uVersionRevision; }
			//! Returns the Version as a string
			std::string									GetVersion() const;

			//! Returns the MetaEntity object with the specified name or NULL if it does not exist.
			MetaEntityPtr								GetMetaEntity(const std::string & strName);
			//! Returns the MetaEntity object with the specified ID or NULL if it does not exist.
			MetaEntityPtr								GetMetaEntity(EntityIndex eEntityIdx)	{ return m_vectMetaEntity[eEntityIdx]; }
			//! Returns a vector holding all MetaEntity objects for this MetaDatabase.
			MetaEntityPtrVectPtr				GetMetaEntities()									{ return &m_vectMetaEntity; }

			//! Returns the D3 VersionInfo MetaEntity object
			MetaEntityPtr								GetVersionInfoMetaEntity()				{ return GetMetaEntity(GetVersionInfoMetaEntityName()); }
			//! Returns the physical name of the D3 VersionInfo MetaEntity object (if it exists)
			std::string									GetVersionInfoMetaEntityName();

			//! Returns the D3::Class factory that knows how to create instances of this (ako of Database objects)
			const Class*								GetInstanceClass()								{ return  m_pInstanceClass;}
			//! Returns the name of the interface that is expected to wrap an instance of this type
			const std::string &					GetInstanceInterface()						{ return  m_strInstanceInterface;}

			//! Converts an ISO8601 date-time string ('YYYY-MM-DDThh:mm:ss[.fff]Z' format) to a database server local date-time string (YYYY-MM-DD hh:mm:ss[.fff]) without time zone details
			std::string									GetISOToServerDate(const std::string & strISODate) const;

			//! Converts the date supplied database server local time into an ISO8601 date-time string ('YYYY-MM-DDThh:mm:ss[.fff]Z' format) including time zone offset
			std::string									GetServerDateToISO(const std::string & strServerDate) const;

			//! This method builds source code for this module
			/*! This method generates the following source code:

					- "D3" + GetAlias() + ".h" : The header file that contains enums, constants and
					  typedefs that can be used to efficiently address elements

					- For each MetaEntity which specifies an InstanceClass and belongs to this,
					  build the files: InstanceClass + "Base.h" and InstanceClass + "Base.cpp".
						These files decler/implement class InstanceClassBase. To utilise this, you
						are expected to create: class InstanceClass : public InstanceClassBase.

					- "I_" + GetAlias() + "Base.ice": Contains the ICE interface declarations for
						the base interface for this

					- "IF" + GetAlias() + ".Base.h": Contains the c++ declarations supporting the
						generated base ICE interface declarations for the base interface for this

					- "IF" + GetAlias() + ".Base.cpp": Contains the c++ implementation supporting the
						generated base ICE interface declarations for the base interface for this.

					- "[template]I_" + GetAlias() + ".ice": Contains the ICE interface declarations for
						interfaces derived from the base interfaces for this. While fully functional, this
						template serves simply as a template of what is the bare minimum required to
						implement interfaces based on the generated base interfaces (see above)

					- "[template]I_" + GetAlias() + ".h": Contains the c++ declarations supporting the
						generated template ICE interface declarations.  While fully functional, this
						template serves simply as a template of what is the bare minimum required to
						implement interfaces based on the generated base interfaces (see above)

					- "[template]I_" + GetAlias() + ".cpp": Contains the c++ implementation supporting the
						generated template ICE interface declarations.  While fully functional, this
						template serves simply as a template of what is the bare minimum required to
						implement interfaces based on the generated base interfaces (see above)

					/note All files are created in subdirectory "./Generated". If this directory doesn't exist,
					the method call will fail.
			*/
			bool												GenerateSourceCode();

			//@{
			/** When an instance of this receives a Connect() message, it should use
					this method once the physical connection to the RDBMS has been
					established as follows:

					\code
					if (!m_pMetaDatabase->VersionChecked())
					{
						// Check D3 version info
						...

						// Mark MetaDatabase as VersionChecked

						//
						m_pMetaDatabase->VersionChecked(true);
					}
					\endcode

					\note If a MetaDatabase is set HasVersionInfo() == false, this method
					always returns true.
			*/
			void												VersionChecked(bool bVersionChecked)											{ m_bVersionChecked = bVersionChecked; }
			bool												VersionChecked()																					{ return HasVersionInfo() ? m_bVersionChecked : true; }
			//@}

			//! Returns this' target database type
			const TargetRDBMS &					GetTargetRDBMS() const																		{ return m_eTargetRDBMS; }

			//! Returns the target database of this as a string
			std::string									GetTargetRDBMSAsString();

			//! Returns a list of this' MetaEntity objects ordered by dependency.
			/*! This method returns a list of MetaDatabasePtr objects ordered by dependency.
					A MetaEntity is considered dependent on another MetaEntity of this if it has
					references to the other MetaEntity. References to MetaEntities in other MetaDatabase
					objects as well as recursive references are ignored by this method. The list the
					method returns will list MetaEntityPtr of meta entities which have no references
					first.
			*/
			MetaEntityPtrList&					GetDependencyOrderedMetaEntities();

			//! Debug aid: The method dumps this and all its objects to cout
			void												Dump(int nIndentSize = 0, bool bDeep = true);

			//! Returns all known and accessible instances of D3::MetaDatabase as a JSON stream
			static std::ostream &				AllAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm);

			//! Writes this meta database's accessible components as JSON to the stream passed in
			std::ostream &							AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, bool* pFirstChild = NULL, bool bShallow = false);

			//! This method can be called periodically to rollback transactions which have been inactive for uiTransactionTimeout secs or more
			void												KillTimedOutTransaction(unsigned int uiTransactionTimeout)				{ m_TransactionManager.KillTimedOutTransaction(uiTransactionTimeout); }

			//! Returns a MetaDatabaseDefinition for self
			MetaDatabaseDefinition			GetMetaDatabaseDefinition();

			//! Saves the meta dictionary definitions itself to the meta dictionary database
			/*! The purpose of this method is to save Meta objects to the meta dictionary.
					This may seem odd, but currently the Meta objects to access the meta dictionary
					itself are hard coded and this is a reliable way to convert them in case we
					want to extract some of the definitions and move them to an external database
			*/
			static void									StoreMDDefsToMetaDictionary();

		protected:
			//! Builds a connection string - only used if the connection string in the meta data database for this is empty.
			void												SetConnectionString(const std::string strConnectionString)	{ m_strConnectionString = strConnectionString; }

			/*! @name Notifications
					Notifications are messages sent by dependents to notify special events.
			*/
			//@{
			//!	An new instance of this has been created; add it to the list of instances.
			void												On_InstanceCreated(DatabasePtr pDatabase);
			//!	An new MetaEntity of this has been created; add it to the vector of MetaEntity objects and assign it an EntityIdx.
			void												On_MetaEntityCreated(MetaEntityPtr pMetaEntity);
			//!	An new MetaRelation which is a Root relation of this has been created; add it to the list of root relations.
			void												On_RootMetaRelationCreated(MetaRelationPtr pMetaRelation);

			//!	An instance of this has been deleted; remove it from the list of instances.
			void												On_InstanceDeleted(DatabasePtr pDatabase);
			//!	An MetaEntity of this has been deleted; set it's location in the vector of MetaEntity objects to NULL
			void												On_MetaEntityDeleted(MetaEntityPtr pMetaEntity);
			//!	An MetaRelation which is a Root relation of this has been deleted; remove it from the list of root relations.
			void												On_RootMetaRelationDeleted(MetaRelationPtr pMetaRelation);

			//! Notification sent when a new MetaDatabaseAlert based on this is instantiated
			void												On_MetaDatabaseAlertCreated(MetaDatabaseAlertPtr pMDBAlert);
			//! Notification sent when a MetaDatabaseAlert based on this is being deleted
			void												On_MetaDatabaseAlertDeleted(MetaDatabaseAlertPtr pMDBAlert);
			//@}

			//! Returns true if the DatabasePtr passed in is an instance of this.
			bool												IsInstance(DatabasePtr);

			//! Returns true if this is the meta dictionary MetaDatabase object
			bool												IsMetaDictionary()			{ return this == M_pDictionaryDatabase; }

			//! Method is called after a MetaDatabase object has been loaded from the meta data database.
			/*! This method verifies that the data contained in the database is correct and returns true
					if all is fine.
			*/
			bool												VerifyMetaData();

			//! Yeah, it would be great if this method had been documented by the HPSUX Guru
			void												xputenv(std::string m_strENV)
			{
				#ifdef AP3_OS_TARGET_HPUX
					putenv(m_strENV.c_str());
				#endif
			}

			//@{
			//! A group of helpers required to generate c++ code specialising basic D3 classes and interface files enabling an ICE interface

			//! Create C++ files which simplify the interaction between D3 objects and clients.
			/*! You call this method on your own MetaDatabase object to generate the C++ files.

					If you do not specify a file name, the system will create a file with the following
					name:

					D3 + GetAlias() + .h

					where GetAlias() returns this' alias if specified or else this' name.

					The method also creates C++ header and source files for specialised MetaEntity
					objects in your MetaDatabase. Please refer to MetaEntity::CreateSpecialisedCPPHeader()
					for more details.
			*/
			bool												CreateCPPFiles();

			//! A helper called by GenerateSourceCode that generates ICE source code for this
			bool												CreateICEFiles();
			//! A helper called by CreateICEInterface that creates forward declarations other base interfaces can use to reference interfaces outside their scope
			bool												CreateICEForwardDecls();
			//! A helper called by CreateICEInterface that creates the base ICE interfaces for this MetaDatabase
			bool												CreateICEBaseInterface();
			//! A helper called by CreateICEInterface that creates the ICE interfaces template for this MetaDatabase
			bool												CreateICETemplateInterface();
			//! A helper called by CreateICEInterface that generates the IFxxx.h headers for base interfaces
			bool												CreateHBaseInterface();
			//! A helper called by CreateICEInterface that generates the IFxxx.h headers for interface
			bool												CreateHTemplateInterface();
			//! A helper called by CreateICEInterface that generates the IFxxx.h headers for base interfaces
			bool												CreateCPPBaseInterface();
			//! A helper called by CreateICEInterface that generates the IFxxx.h headers for interface
			bool												CreateCPPTemplateInterface();
			//@}

			//! Sets the JSON HSTopic array
			void												SetHSTopics(const std::string & strHSTopicsJSON) { m_strHSTopicsJSON = strHSTopicsJSON; }
	};












	//! The Database class provides a common interface for any type of database.
	/*! A Database object is an instance of a MetaDatabase object. and belongs to
			a DatabaseWorkspace. It is an abstarct class which provides a generic
			interface for all databases.

			This class does not directly support physical store I/O. Such functionallity
			is implemented by Subclasses of this class. You can create your own subclass
			of this class. To ensure that D3 creates an instance of your subclass when
			CreateInstance is called on the corresponding MetaDatabase object, you set
			the InstanceClass column in the meta data database for you MetaDatabase
			accordingly. For this to work correctly, your class must use the D3 Class
			macros D3_CLASS_DECL() and D3_CLASS_IMPL(). If you need to perform any additional initialisation after the
			database has been created, intercept the On_PostCreate() message sent by
			the MetaDatabase object but be sure to chain to the ancestores On_PostCreate()
			once you have performed your object specific initialisation actions.
	*/
	class D3_API Database : public Object
	{
		public:
			typedef void (*CreateStoredProcedureFunc)(DatabasePtr pDatabase);

		protected:
			// Trace levels
			#define	D3DB_TRACE_NONE							0x00
			#define	D3DB_TRACE_SELECT						0x01
			#define	D3DB_TRACE_UPDATE						0x02
			#define	D3DB_TRACE_DELETE						0x04
			#define	D3DB_TRACE_INSERT						0x08
			#define	D3DB_TRACE_TRANSACTIONS			0x10
			#define	D3DB_TRACE_STATS						0x20

			friend class MetaDatabase;
			friend class MetaDatabase::TransactionManager;
			friend class DatabaseAlert;
			friend class DatabaseWorkspace;
			friend class SQLServerPagedResultSet;
			friend class ResultSet;
			friend class DatabaseAlertManager;

			D3_CLASS_DECL_PV(Database);

			MetaDatabasePtr						m_pMetaDatabase;					//!< The first database created is the global DB
			DatabaseWorkspacePtr			m_pDatabaseWorkspace;			//!< This instance belongs to this workspace
			bool											m_bInitialised;						//!< True once the object received and successfully handled the On_PostCreate() notification
			ResultSetPtrListPtr				m_plistResultSet;					//!< A list of result sets (the list exists only if resultsets have been added during the lifecycle of this)
			bool											m_bTraceUpdates;					//!< If true, INSERT, DELETE and UPDATE statements are logged
			unsigned int							m_uTrace;									//!< The trace level (see TraceXXX() methods)
			unsigned long							m_lNextRsltSetID;					//!< The next result set ID this will assign to a ResultSet that belongs to this

			//! Constructor called by MetaObject::CreateInstance()
			Database() : m_pMetaDatabase(NULL), m_pDatabaseWorkspace(NULL), m_bInitialised(false), m_plistResultSet(NULL), m_uTrace(D3DB_TRACE_NONE), m_lNextRsltSetID(1) {};
			//! Destructor removes this from DatabaseWorkspace and the MetaDatabase's list of instance databases.
			~Database();

		public:
			//! Return the MetaDatabase object.
			MetaDatabasePtr						GetMetaDatabase()					{ return m_pMetaDatabase; }

			//! Return the DatabaseWorkspace object.
			DatabaseWorkspacePtr			GetDatabaseWorkspace()		{ return m_pDatabaseWorkspace; }

			//! Physical disconnect from the physical store.
			virtual bool							Disconnect() = 0;
			//! Physical connect to the physical store.
			virtual bool							Connect() = 0;
			//! Returns true if this is physically connected with the physical store.
			virtual bool							IsConnected ()						{ return false; }

			//@{
			//! Logging functions
			/*! These functions can be set temporarily on a single database to log database actions

					TraceSelect(bool)	: if bool=true, traces SELECT, otherwise disables tracing SELECT
					TraceUpdate(bool)	: if bool=true, traces UPDATE, otherwise disables tracing UPDATE
					TraceDelete(bool)	: if bool=true, traces DELETE, otherwise disables tracing DELETE
					TraceInsert(bool)	: if bool=true, traces INSERT, otherwise disables tracing INSERT

					TraceAll():  same as: TraceSelect(true);TraceUpdate(true);TraceDelete(true);TraceInsert(true);
					TraceNone(): same as: TraceSelect(false);TraceUpdate(false);TraceDelete(false);TraceInsert(false);

					TraceChanges(): same as: TraceUpdate(true);TraceDelete(true);TraceInsert(true);
			*/
			void	TraceSelect(bool on)	{ on ? m_uTrace |= D3DB_TRACE_SELECT : m_uTrace &= ~D3DB_TRACE_SELECT; }
			void	TraceUpdate(bool on)	{ on ? m_uTrace |= D3DB_TRACE_UPDATE : m_uTrace &= ~D3DB_TRACE_UPDATE; }
			void	TraceDelete(bool on)	{ on ? m_uTrace |= D3DB_TRACE_DELETE : m_uTrace &= ~D3DB_TRACE_DELETE; }
			void	TraceInsert(bool on)	{ on ? m_uTrace |= D3DB_TRACE_INSERT : m_uTrace &= ~D3DB_TRACE_INSERT; }
			void	TraceStats(bool on)		{ on ? m_uTrace |= D3DB_TRACE_STATS  : m_uTrace &= ~D3DB_TRACE_STATS;  }
			void	TraceTransactions(bool on)	{ on ? m_uTrace |= D3DB_TRACE_TRANSACTIONS : m_uTrace &= ~D3DB_TRACE_TRANSACTIONS; }

			void	TraceAll()				{ m_uTrace = D3DB_TRACE_SELECT | D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT | D3DB_TRACE_TRANSACTIONS | D3DB_TRACE_STATS; }
			void	TraceNone()				{ m_uTrace = D3DB_TRACE_NONE; }

			void	TraceChanges()		{ m_uTrace = D3DB_TRACE_UPDATE | D3DB_TRACE_DELETE | D3DB_TRACE_INSERT | D3DB_TRACE_TRANSACTIONS; }
			//@}

			//! Wait for an alert and returns the Alert ID of the alert that fired or a negative value in case of an error (at this level, does nothing and returns -1, see specific sub-classes for details)
			virtual DatabaseAlertPtr	MonitorDatabaseAlerts(DatabaseAlertManagerPtr pManager);

			//! Register an alert and return the Alert ID (at this level, does nothing and returns -1, see specific sub-classes for details)
			virtual int								RegisterAlert (const std::string & strAlertName)			{ return -1; }
			//! Wait for an alert and returns the Alert ID of the alert that fired or a negative value in case of an error (at this level, does nothing and returns -1, see specific sub-classes for details)
			virtual int								WaitForAlert (const std::string & strAlertName)				{ return -1; }
			//! Remove an alert (at this level, does nothing and returns -1, see specific sub-classes for details)
			virtual int								RemoveAlert (const std::string & strAlertName = "")		{ return -1; }
			//! Get the Alert ID of a registered alert (at this level, does nothing and returns -1, see specific sub-classes for details)
			virtual int								GetAlertID(const std::string & strAlertName)					{ return -1; }
			//! Given an Alert ID, get the name of the alert (at this level, does nothing and returns -1, see specific sub-classes for details)
			virtual std::string				GetAlertName(int iAlertID)														{ return ""; }
			//! Get the list of registered alerts	(at this level, does nothing and returns NULL, see specific sub-classes for details)
			virtual StringList*				GetAlertList()																				{ return NULL; }

			//! Start a transaction.
			virtual bool							BeginTransaction() = 0;
			//! Make all updates to the database performed since the preceeding call to BeginTransaction() permanent.
			virtual bool							CommitTransaction() = 0;
			//! Undo all updates to the database performed since the preceeding call to BeginTransaction().
			virtual bool							RollbackTransaction() = 0;
			//! Return true if this has a pending transaction.
			virtual bool							HasTransaction() = 0;

			//! Return the ExceptionContext associated with this, same as GetDatabaseWorkspace()->GetExceptionContext().
			ExceptionContextPtr				GetExceptionContext();

			//! Builds a useless string that tells ya how many instances of each entity are held by this.
			std::string								GetObjectStatisticsText();

			//! [Re]load the specified column from the database. This method is primarily used for LazyFetch columns.
			/*! @param	pColumn		The instance column to refresh.

					Pre-requisites:
					- pColumn->GetEntity()->IsNew() must return false

					Returns pColumn if successful or NULL if unsuccessful. The method does not throw!

					\note	This method always incurs a database roundtrip.
			*/
			virtual ColumnPtr					LoadColumn(ColumnPtr pColumn) = 0;

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
			virtual EntityPtr					LoadObject(KeyPtr pKey, bool bRefresh = false, bool bLazyFetch = true) = 0;

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
			virtual long							LoadObjects(RelationPtr pRelation, bool bRefresh = false, bool bLazyFetch = true) = 0;

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
			virtual long							LoadObjects(MetaKeyPtr pMetaKey, KeyPtr pKey, bool bRefresh = false, bool bLazyFetch = true) = 0;

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
			virtual long							LoadObjects(MetaEntityPtr pMetaEntity, bool bRefresh = false, bool bLazyFetch = true) = 0;

			//@{
			//! Make objects of a specific type resident
			/*! This method allows you to specify a query string of any complexity. The only
					rules that you need to observe are:

					- The query must return all columns (in the correct order) from the MetaEntity specified


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
			virtual EntityPtrListPtr	LoadObjects(MetaEntityPtr pMetaEntity, const std::string & strSQL, bool bRefresh = false, bool bLazyFetch = true) = 0;
			virtual EntityPtrListPtr	LoadObjects(MetaEntityPtr pMetaEntity, const char * strSQL, bool bRefresh = false, bool bLazyFetch = true)	{ return LoadObjects(pMetaEntity, std::string(strSQL), bRefresh, bLazyFetch); }
			//@}

			//! Loads objects through a query
			/*! This method allows you to specify a MetaEntity, a SQL WHERE predicate and a SQL ORDER BY clause
					and returns a list of objects loaded.

					The passed in MetaEntity must belong to the same meta database this belongs to.
					If strWHERE is empty, all records are loaded.
					If strOrderBy is empty, output order is arbitrary.
					If bRefresh is true, objects already resident will be refreshed.

					Callers delete the returned list when done.

					\note This method may throw!
			*/
			virtual EntityPtrListPtr	LoadObjectsThroughQuery(MetaEntityPtr pME, const std::string & strWhere, const std::string & strOrderBy, bool bRefresh = false, bool bLazyFetch = true);

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
			virtual void							LoadObjectsThroughStoredProcedure(MetaEntityPtrList & listME, CreateStoredProcedureFunc, const std::string & strProcName, const std::string & strProcArgs, bool bRefresh = false, bool bLazyFetch = true)
			{
				throw std::runtime_error("Database::LoadObjectsThroughStoredProcedure() not implemented");
			}

			//! Create stored procedure
			/*! This method must be overloaded by none abstract classes based on this.
					The method simply creates the stored procedure requested if it doesn't exist.
					If the procedure exists already, the mothod will not throw.
			*/
			virtual void							CreateStoredProcedure(const std::string & strSQL)
			{
				throw std::runtime_error("Database::CreateStoredProcedure() not implemented");
			}

			//! Executes the stored procedure and returns the resultsets in the form of a json structure
			/*! This method allows you to call a stored procedure to retrieve multiple records from multiple resultsets in a single call.

					@param	ostrm	Results will be written to this stream in the form

					- array of result sets
					  - contains arrays of records
						  - contains columns and values

					[
						[
							{
								"name": "hugo",
								"age": 12
							},
							{
								"name": "fred",
								"age": 22
							}
						],
						[
							{
								"name": "fiat",
								"model": "uno"
							},
							{
								"name": "audi",
								"model": "quatro"
							}
						],
						[
							{
								"driver": "fred",
								"car": "audi"
							},
							{
								"driver": "hugo",
								"car": "audi"
							}
						]
					]

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

					\note This method may throw
			*/
			virtual void							ExecuteStoredProcedureAsJSON(std::ostringstream& ostrm, CreateStoredProcedureFunc, const std::string & strProcName, const std::string & strProcArgs)
			{
				throw std::runtime_error("Database::ExecuteStoredProcedureAsJSON() not implemented");
			}

			//! ExecuteSQLCommand allows you to execute a read only SQL Statement (though read only is not enforcable)
			/*! This pure virtual method must be overloaded by none abstract classes based on this.
					See specific subclasses for more details.

					@param	strSQL			The SQL statement to execute.

					@return int					Returns -1 if an error occurred or the number of
															rows affected by the update statement.
			*/
			virtual int								ExecuteSQLReadCommand(const std::string & strSQL)						{ return ExecuteSQLCommand(strSQL, true); }

			//! ExecuteSQLCommand allows you to execute an SQL Statement to update a database
			/*! This pure virtual method must be overloaded by none abstract classes based on this.
					See specific subclasses for more details.

					It requires a tr4ansaction.

					@param	strSQL			The SQL statement to execute.

					@return int					Returns -1 if an error occurred or the number of
															rows affected by the update statement.
			*/
			virtual int								ExecuteSQLUpdateCommand(const std::string & strSQL) 		{ return ExecuteSQLCommand(strSQL, false); }

			//! ExecuteSingletonSQLCommand allows you to execute an SQL Statement which returns a single value
			/*! This method is expected to execute a query that simply returns a single resultset with
					a single row and a single column. A classic example would be a query that uses an aggregate function
					to retrieve a single value from the database such as:

						SELECT AVG(weight) FROM Product;
						SELECT COUNT(*) FROM Product;

					etc.
			*/
			virtual void							ExecuteSingletonSQLCommand(const std::string & strSQL, unsigned long & lResult) = 0;

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
			virtual std::ostringstream& ExecuteQueryAsJSON(const std::string & strSQL, std::ostringstream	& oResultSets) = 0;

			//! Same as ExecuteQueryAsJSON but here we return the JSON with meta data.
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
			virtual std::ostringstream& QueryToJSONWithMetaData(const std::string & strSQL, std::ostringstream	& oResultSets) { return oResultSets; }

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
			virtual bool							UpdateObject(EntityPtr pObj) = 0;


			//@{
			/** Just for backwards compatibility */

			virtual void							SetLastError(const char * pszLastError);
			virtual void							SetLastError(const std::string & strLastError)		{ SetLastError(strLastError.c_str()); };
			//@}

			//! Deletes all objects associated with this database object
			/*!	This method is automatically called when a database object is deleted.
					The method is also useful if you use a database object over a prolonged
					period of time but wish to discard all objects which it contains so that
					you have a clear starting point before you load new objects from the
					physical store.
			*/
			void											DeleteAllObjects();

			//! Find a resultset belonging to this by ID
			ResultSetPtr							GetResultSet(unsigned long lID);

			//! Export data to an XML file
			/*! Adds XML elements to the specified XML document as children of the specified pParentNode.
			    If the optional pListME parameter (std::list< MetaEntity* >*) is supplied, only data from the
					MetaEntites listed will be exported. If this parameter is NULL, data from all tables will
					be exported. The only exception is the version info table which will never be exported.
			*/
			virtual long							ExportToXML(const std::string & strAppName, const std::string & strXMLFileName, MetaEntityPtrListPtr pListME = NULL, const std::string & strD3MDDBIDFilter = "") = 0;

			//! Filter exported data to an XML file
			/*! Modifies existing SQL query to contain only selected Meta Dictionaries
			*/
			virtual std::string				FilterExportToXML(MetaEntityPtr pMetaEntity, const std::string & strD3MDDBIDFilter);

			//! Import data from an XML file create with ExportToXML()
			/*! This method attempts to import data from and XML DOM which is expected to be compatible
					with the structure of the one that ExportToXML() creates. The method takes care to import
					the data in the correct sequence so that dependent records are imported after the records they
					depend on. If you supply the pListME parameter (std::list< MetaEntity* >*), only records of
					the meta entity type included in the list will be imported provided they are also included in the
					XML DOM. On SQL Server, the method ensures that IDENTITY_INSERT is turned off so that IDENTITY
					column values are preserved.

					\note Although data is accessed from the physical database to populate the XML DOM, the data
					that has been added to the XML DOM is NOT resident upon return.
			*/
			virtual long							ImportFromXML(const std::string & strAppName, const std::string & strXMLFileName, MetaEntityPtrListPtr pListME = NULL) = 0;


			//! Export RBAC settings to an JSON file
			/*! Parameters:

					@strRootName:			The name of the root object (external clients should
														choose a name that makes sure we do not accidentially
														import RBAC settings from a meta dictionary for one client
														into another clients meta dictionary database). A good
														idea would be to use a [APAL3_TARGET[-APAL3_INSTANCE]]

					@strJSONFileName:	The name of the JSON file to write.

					RBAC setting files differ from other JSON export files in that they store RBAC data without
					hard links to meta dictionary data. Instead, links between permissions and actual meta types
					are stored by name. This means that you can reimport an RBAC export file even after you made
					changes to a meta dictionary. (see also ImportRBACFromJSON for more details).

					Note: The only way to import these files is via D3::Database::ImportRBACFromJSON()

				  Returns: the total number of records exported
			*/
			virtual long							ExportRBACToJSON(const std::string & strRootName, const std::string & strJSONFileName);

			//! Import RBAC settings from an JSON file
			/*! Parameters:

					@strRootName:			The name of the root element (external clients should
														choose a name that makes sure we do not accidentially
														import RBAC settings from a meta dictionary for one client
														into another clients meta dictionary database). A good
														idea would be to use a [APAL3_TARGET[-APAL3_INSTANCE]]

					@strJSONFileName:	The name of the JSON file. This filemust have been created
														with by Database::ExportRBACToJSON.

					The method does the following:

					1) Delete all RBAC settings attached to the current meta dictionary
					2) Make the meta dictionary resident
					3) Add RBAC settings from import file
						 a) Remove all existing Role and User records
						 b) Add all Role, User and RoleUser records from backup
						 c) for each D3[Type]Permission, find the related D3Meta[Type] and if it
						    if it exists, create a new D3[Type]Permission record, set its attributes
								and link it to the D3Meta[Type]
						 d) Recreate internal security objects

				  Returns: the total number of records imported
			*/
			virtual long							ImportRBACFromJSON(MetaDatabaseDefinitionList& listMDDefs, const std::string & strRootName, const std::string & strJSONFileName);

		protected:
			//! Pure virtual function implementing ExecuteSQLReadCommand or ExecuteSQLUpdateCommand
			/*! This pure virtual method must be overloaded by none abstract classes based on this.
					Precondition: bReadOnly || HasTransaction()

					@param	strSQL			The SQL statement to execute.

					@return int					Returns -1 if an error occurred or the number of
															rows affected by the update statement.
			*/
			virtual int								ExecuteSQLCommand(const std::string & strSQL, bool bReadOnly) = 0;

			//! Initialise DatabaseAlertResources
			/*! This method throws an error at this level, though it is only called if you try to use alert notifications.
					\note At this level, the method retruns with success (assuming nothing needs to be done to enable DatabaseAlerts for this type of database)
			*/
			virtual bool							InitialiseDatabaseAlerts(DatabaseAlertManagerPtr pDBAlertMngr)			{ return true; }

			//! UnInitialise DatabaseAlertResources
			/*! This method can be overloaded by subclasses to handle database alert requests.
					You typically overload this method if you need to do extra work to enable database
					alerts so that you can remove these resources on exit.
					\note At this level, the method retruns with success (assuming nothing needs to be done to enable DatabaseAlerts for this type of database)
			*/
			virtual void							UnInitialiseDatabaseAlerts(DatabaseAlertManagerPtr pDBAlertMngr)		{}

			//! Register DatabaseAlert
			/*! This method is used internally to register a DatabaseAlert.
					The purpose of the method is to ensure that a subsequent call to MonitorDatabaseAlerts() will catch notifications of the requested type.
					\note At this level, the method always throws an exception.
			*/
			virtual void							RegisterDatabaseAlert(DatabaseAlertPtr pDBAlert);

			//! UnRegister DatabaseAlert
			/*! This method is used internally to register a DatabaseAlert.
					The purpose of the method is to ensure that we no longer trap the passed in DatabaseAlert type.
					\note At this level, the method always throws an exception.
			*/
			virtual void							UnRegisterDatabaseAlert(DatabaseAlertPtr pDBAlert);

			//! If this is the global database, it loads all entities marked as cached once a connection has been obtained.
			void											LoadCache();

			//! Returns true if this is the Global database for it's MetaDatabase
			bool											IsGlobalDatabase();

			//! Returns true if the physical database version matches the version it's MetaDatabase specifies.
			/*! This method is called the first Connect() is called on an instance of a MetaDatabase.

					The method returns with success immediately if it's MetaDatabase has HasVersionInfo()
					set to false or has no D3 version info MetaEntity.

					If both exist, the method reads the row with the most recent LastUpdated value
					and compares the Name, VersionMajor and VersionMinro with the values of it's
					MetaDatabase object. If they match the method returns true, if not it returns false.

					This method is called internally. If it fails, subclasses should cause the Connect()
					to fail also as there is a version problem.
			*/
			virtual bool							CheckDatabaseVersion() = 0;

			// This notification is sent by the associated MetaDatabase object
			// once the object has been constructed and initialised
			//
			virtual void							On_PostCreate();

			// A helper that does nothing at this level
			virtual EntityPtrListPtr	CallPagedObjectSP(ResultSetPtr	pRS,
																									MetaEntityPtr	pMetaEntity,
																									unsigned long uPageNo,
																									unsigned long uPageSize,
																									unsigned long & lStart,
																									unsigned long & lEnd,
																									unsigned long & lTotal)	{ return NULL; }

			//@{
			//! Notifications sent by ResultSet objects after creation and before destruction.
			/*! The method simply adds or remove the ResultSet from the databases list
			*/
			void											On_ResultSetCreated(ResultSetPtr pResultSet);
			void											On_ResultSetDeleted(ResultSetPtr pResultSet);
			//@}

			//@{
			//! Set a JSON array containing basic HSTopic info which apply to the currently loaded D3::Meta[Type] objects
			/*!	The following methods assign JSON arrays containing the id (numeric) and title (string) of HSTopic
					records which applies to the current target and is linked with the meta type object via the relevant
					HSMeta[Type]Topic associative entity. The methods only retrieve this data but do not make HSTopic objects
					resident.
			*/
			virtual void							SetMetaDatabaseHSTopics() = 0;
			virtual void							SetMetaEntityHSTopics() = 0;
			virtual void							SetMetaColumnHSTopics() = 0;
			virtual void							SetMetaKeyHSTopics() = 0;
			virtual void							SetMetaRelationHSTopics() = 0;
			//@}
	};




	//! The DatabaseWorkspace class is the main object for D3 clients.
	/*!	The DatabaseWorkspace can hold instances of different MetaDatabase
			objects. However, a DatabaseWorkspace can only hold <b>one</b>
			instance of a given MetaDatabase object and no more.

			Since D3 allows relations between Entity objects from different databases
			(and even different RDBMS systems), the DatabaseWorkspace provides the
			glue.

			Consider an Oracle database with table \a ORDER and an SQLServer database with
			a table \a Order and assume that you had configured a relationship between
			the two tables. You might simply access the Oracle database and make an
			instance of \a ORDER memory resident. If you navigate the relations with
			\a Order, the system needs an SQLServer database to satisfy the relation.

			D3 accomplishes this task by obtaining the DatabaseWorkspace object which is
			the parent of the Oracle database instance. Since it also knows the MetaDatabase
			of the SQLServer database, it will simply request the SQLServer database from the
			DatabaseWorkspace. If it exists already, it will get the existing copy and if not,
			a new instance will be created and associated with the DatabaseWorkspace.
	*/
	class D3_API DatabaseWorkspace : public Object
	{
		friend class MetaDatabase;
		friend class Database;

		D3_CLASS_DECL(DatabaseWorkspace);

		protected:
			DatabasePtrMap						m_mapDatabase;		//!< A map of databases used by this workspace keyed by MetaDatabasePtr
			ExceptionContextPtr				m_pEC;						//!< The exception context to be used for this DatabaseWorkspace
			SessionPtr								m_pSession;				//!< Points to the session that owns this DatabaseWorkspace (or NULL if this DatabaseWorkspace is not owned by a Session)

		public:
			//! Create a DatabaseWorkspace
			/*! The constructor does not create any database instances. Use the GetDatabase()
					methods to create/retrieve an instance of MetaDatabase objects.

					The constructor allows you to specify an exception context specific to the
					workspace. If you specify your own context, any errors occured while
					interacting with the DatabaseWorkspace will be logged to the specified
					exception context.
			*/
			DatabaseWorkspace(ExceptionContextPtr pEC = ExceptionContext::GetDefaultExceptionContext(), SessionPtr pSession = NULL) : m_pEC(pEC), m_pSession(pSession)	{};

			//!	The DatabaseWorkspace destructor will automatically free all resources allocated during the object's life time.
			~DatabaseWorkspace();

			//@{
			//! Retrieve the workspace's Database instance for a MetaDatabase.
			/*! If you pass the database name as a string, the method searches
					for a MetaDatabase object with either a matching name or a
					matching alias.

					These are high level methods which can fail, therefore you
					should setup an appropriate try/catch block.

					If the instance does not exist, a new one will be created and
					connected automatically.
			*/
			DatabasePtr								GetDatabase(MetaDatabasePtr);
			DatabasePtr								GetDatabase(const std::string & strDatabaseName);
			//@}

			//! Return the map of databases (note: databases are added as requested and keyed by MetaDatabasePtr)
			DatabasePtrMap&						GetDatabases()			{ return m_mapDatabase; }

			//! Get the exception context associated with this
			ExceptionContextPtr				GetExceptionContext()																	{ return m_pEC; };
			//! Set the exception context associated with this
			void											SetExceptionContext(ExceptionContextPtr	pEC = NULL)
			{
				if (pEC)
				{
					m_pEC = pEC;
				}
				else
				{
					SetDefaultExceptionContext();
				}
			}
			//! Replace the current exception context with the default exception context in use for the application.
			void											SetDefaultExceptionContext()													{ m_pEC = ExceptionContext::GetDefaultExceptionContext(); }

			//! Returns the session that owns this DatabaseWorkspace.
			/*! Note that not all DatabaseWorkspace objects are owned by sessions and that this method returns
					NULL if this DatabaseWorkspace is not owned by a session.
			*/
			SessionPtr								GetSession()																					{ return m_pSession; }

		protected:
			//! Retrieve the workspace's Database instance for a MetaDatabase. Returns NULL if no instance exists.
			DatabasePtr								FindDatabase(MetaDatabasePtr);

			//@{
			//! Notifications sent by the Database object after it has been created or before it is being deleted.
			/*! The method simply adds or remove the Database object from the workspaces database map
			*/
			void											On_DatabaseCreated(DatabasePtr pDatabase);
			void											On_DatabaseDeleted(DatabasePtr pDatabase);
			//@}
	};


	// Some inline MetaDatabase and Database methods which depend on DatabaseWorkspace
	//
	inline	DatabasePtr						MetaDatabase::GetGlobalDatabase()	{ return M_pDatabaseWorkspace->GetDatabase(this); }
	inline	ExceptionContextPtr		Database::GetExceptionContext()		{ return m_pDatabaseWorkspace ? m_pDatabaseWorkspace->GetExceptionContext() : ExceptionContext::GetDefaultExceptionContext(); };
	inline	bool									Database::IsGlobalDatabase()			{ return m_pDatabaseWorkspace->FindDatabase(m_pMetaDatabase) == this; }

}	// end namespace D3

#endif
