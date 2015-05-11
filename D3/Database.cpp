// MODULE: Database source
//;
//; IMPLEMENTATION CLASS: Database
//;
// ===========================================================
// File Info:
//
// $Author: lalit $
// $Date: 2014/12/16 12:54:23 $
// $Revision: 1.96 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 03/10/02 - R13 - Hugp
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
//
// 07/06/03 - R4 - HugP
//
// Changed TMSMakeShipmentResident and TMSMakeOrderResident to acquire
// details from the activity differently.
//
// Implemented the debug helper GetObjectStatisticsText
//
// Added optional flag to DeleteAllObjects to exclude TMS entities
//
// -----------------------------------------------------------

#include "D3Types.h"

#include <time.h>

int							g_GlobalD3DebugLevel = 0;

#include "Database.h"
#include "Entity.h"
#include "Column.h"
#include "Key.h"
#include "Relation.h"
#include "ResultSet.h"
#include "D3Funcs.h"
#include "ODBCDatabase.h"
#include "OTLDatabase.h"
#include "Session.h"

#include "D3MetaDatabase.h"
#include "D3MetaEntity.h"
#include "D3MetaColumn.h"
#include "D3MetaColumnChoice.h"
#include "D3MetaKey.h"
#include "D3MetaKeyColumn.h"
#include "D3MetaRelation.h"

#include "D3User.h"
#include "D3HistoricPassword.h"
#include "D3Role.h"
#include "D3RoleUser.h"
#include "D3RowLevelPermission.h"
#include "D3DatabasePermission.h"
#include "D3EntityPermission.h"
#include "D3ColumnPermission.h"

#include "HSTopic.h"
#include "HSTopicAssociation.h"
#include "HSTopicLink.h"
#include "HSResource.h"
#include "HSResourceUsage.h"
#include "HSMetaDatabaseTopic.h"
#include "HSMetaEntityTopic.h"
#include "HSMetaColumnTopic.h"
#include "HSMetaKeyTopic.h"
#include "HSMetaRelationTopic.h"

#include "Codec.h"
#include "md5.h"


// Required for RBAC import
// #include <ifstream>
#include <json/json.h>

// uses Centerpoint XML
#include <DOM/DOMBuilder.h>
#include <DOM/Document.h>
#include <DOM/Element.h>
#include <DOM/NamedNodeMap.h>
#include <DOM/NodeList.h>
#include <SAX/InputSource.h>
#include <XMLException.h>

//#include <project-parameters.h>

// if the following macro is defined, the generated source files will also build ICE
// interfaces to get and set the value of columns. For example, if the entity Warehouse
// defines a D3::MetaColumnString called Name, it will create these interfaces:
//
// string ID3::WarehouseI::GetName();
// void		ID3::WarehouseI::SetName(string);
//
// Using these however is not recommended. Best practice is to use the
// ID3::EntityI::GetColumn() interface instead.
//
#undef IMPLEMENT_ICE_COLUMN_INTERFACES


namespace D3
{
	//===========================================================================
	//
	// DatabaseAlertManager implementation
	//

	DatabaseAlertManager::DatabaseAlertManager(MetaDatabasePtr pMetaDatabase)
		: m_pMetaDatabase(pMetaDatabase),
			m_pDBWS(NULL),
			m_pDatabase(NULL),
			m_bAlertsInitialised(false),
			m_bIsMonitoring(false),
			m_bShouldStop(false)
	{
		assert(m_pMetaDatabase);
		m_pDBWS = new DatabaseWorkspace();
		m_pDatabase = m_pDBWS->GetDatabase(m_pMetaDatabase);
	}





	DatabaseAlertManager::~DatabaseAlertManager()
	{
		if (m_bIsMonitoring)
			ReportError("DatabaseAlertManager::~DatabaseAlertManager{): Function called on an object that is waiting for an alert!");

		// Discard all DatabaseAlerts
		while(!m_mapDatabaseAlert.empty())
			delete m_mapDatabaseAlert.begin()->second;

		if (m_bAlertsInitialised)
			m_pDatabase->UnInitialiseDatabaseAlerts(this);

		delete m_pDBWS;
	}





	void DatabaseAlertManager::AddAlertListener(const std::string & strAlertName, DatabaseAlertListenerFunc pfnCallBack, int iTimeout, DatabaseAlertManager::Frequency frequency)
	{
		boost::recursive_mutex::scoped_lock		lk(GetMutex());

		MetaDatabaseAlertPtr			pMDBAlert;
		DatabaseAlertPtr					pDBAlert;


		// Avoid duplicates
		pDBAlert = GetDatabaseAlert(strAlertName);

		if (!pDBAlert)
		{
			// Try and create one

			// Find meta alert
			pMDBAlert = m_pMetaDatabase->GetMetaDatabaseAlert(strAlertName);

			if (!pMDBAlert)
				throw Exception(__FILE__, __LINE__, Exception_error, "DatabaseAlertManager::AddAlertListener(): Alert %s not recognised", strAlertName.c_str());

			// Create alert
			pDBAlert = new DatabaseAlert(this, pMDBAlert, iTimeout);
		}
		else
		{
			if (iTimeout > 0)
				pDBAlert->SetTimeout(iTimeout);
		}

		pDBAlert->AddListener(pfnCallBack, frequency);

		if (!m_bAlertsInitialised)
			m_bAlertsInitialised = m_pDatabase->InitialiseDatabaseAlerts(this);

		m_pDatabase->RegisterDatabaseAlert(pDBAlert);
	}





	void DatabaseAlertManager::RemoveAlertListener(const std::string & strAlertName, DatabaseAlertListenerFunc pfnCallBack)
	{
		boost::recursive_mutex::scoped_lock		lk(GetMutex());

		DatabaseAlertPtr pDBAlert = GetDatabaseAlert(strAlertName);

		if (!pDBAlert)
			throw Exception(__FILE__, __LINE__, Exception_error, "DatabaseAlertManager::RemoveAlertListener(): Alert %s not recognised", strAlertName.c_str());

		pDBAlert->RemoveListener(pfnCallBack);

		m_pDatabase->UnRegisterDatabaseAlert(pDBAlert);

		if (!pDBAlert->HasListeners())
			delete pDBAlert;
	}





	void DatabaseAlertManager::DisableAlertListener(const std::string & strAlertName, DatabaseAlertListenerFunc pfnCallBack)
	{
		boost::recursive_mutex::scoped_lock		lk(GetMutex());

		DatabaseAlertPtr pDBAlert = GetDatabaseAlert(strAlertName);

		if (!pDBAlert)
			throw Exception(__FILE__, __LINE__, Exception_error, "DatabaseAlertManager::DisableAlertListener(): Alert %s not recognised", strAlertName.c_str());

		pDBAlert->DisableListener(pfnCallBack);
	}





	void DatabaseAlertManager::EnableAlertListener(const std::string & strAlertName, DatabaseAlertListenerFunc pfnCallBack)
	{
		boost::recursive_mutex::scoped_lock		lk(GetMutex());

		DatabaseAlertPtr pDBAlert = GetDatabaseAlert(strAlertName);

		if (!pDBAlert)
			throw Exception(__FILE__, __LINE__, Exception_error, "DatabaseAlertManager::EnableAlertListener(): Alert %s not recognised", strAlertName.c_str());

		pDBAlert->EnableListener(pfnCallBack);
	}





	void DatabaseAlertManager::Terminate()
	{
		boost::recursive_mutex::scoped_lock		lk(GetMutex());

		if (m_bIsMonitoring)
		{
			if (m_pDatabase->GetMetaDatabase()->GetTargetRDBMS() == SQLServer)
			{
#ifdef APAL_SUPPORT_ODBC
				assert(m_pDatabase->IsKindOf(ODBCDatabase::ClassObject()));
				ODBCDatabasePtr pDB = (ODBCDatabasePtr) m_pDatabase;

				// We cheat and simply register an alert that will fail immediately with info="invalid"
				pDB->RegisterDatabaseAlert(AlertServiceName(), "FAIL", "SELECT GETDATE()", 0);
#endif
			}
			else
			{
				DatabaseAlertPtr				pDBAlert;
				DatabaseAlertPtrMapItr	itr;

				// Disable all enabled alerts
				for ( itr =  m_mapDatabaseAlert.begin();
							itr != m_mapDatabaseAlert.end();
							itr++)
				{
					pDBAlert = itr->second;

					if (pDBAlert->IsRegistered())
						m_pDatabase->UnRegisterDatabaseAlert(pDBAlert);
				}
			}
		}

		m_bShouldStop = true;
	}





	//! UnRegister DatabaseAlert
	const std::string & DatabaseAlertManager::AlertServiceName()
	{
		if (m_strAlertService.empty())
		{
			std::string		strID;

			// strID is simply this' address
			Convert(strID, (unsigned long) this);

			// The name is HostName + "_" + MetaDatabase.Name + "-" + strID
			m_strAlertService = GetSystemHostName();
			m_strAlertService += '_';
			m_strAlertService += m_pMetaDatabase->GetName();
			m_strAlertService += '_';
			m_strAlertService += strID;
		}

		return m_strAlertService;
	}





	void DatabaseAlertManager::On_BeforeMonitorDatabaseAlerts()
	{
		boost::recursive_mutex::scoped_lock		lk(GetMutex());

		DatabaseAlertPtr				pDBAlert;
		DatabaseAlertPtrMap			mapDBAlert;
		DatabaseAlertPtrMapItr	itr;
		bool										bHasRegisteredAlerts = false;

		if (m_bShouldStop)
			throw Exception(__FILE__, __LINE__, Exception_error, "DatabaseAlertManager::On_BeforeMonitorDatabaseAlerts(): Do not call DatabaseAlertManager::MonitorDatabaseAlerts() after calling DatabaseAlertManager::Terminate()");

		// work with a copy as we may alter the original
		mapDBAlert = m_mapDatabaseAlert;

		// Let's check each DatabaseAlert
		for ( itr =  mapDBAlert.begin();
					itr != mapDBAlert.end();
					itr++)
		{
			pDBAlert = itr->second;

			if (!pDBAlert->HasListeners())
			{
				// Need need to keep these
				delete pDBAlert;
				continue;
			}

			// Make sure the enabled alerts are registered
			if (pDBAlert->IsEnabled())
			{
				if (!pDBAlert->IsRegistered())
					m_pDatabase->RegisterDatabaseAlert(pDBAlert);

				bHasRegisteredAlerts = true;
			}
		}

		if (!bHasRegisteredAlerts)
			throw Exception(__FILE__, __LINE__, Exception_error, "DatabaseAlertManager::On_BeforeMonitorDatabaseAlerts(); No alerts have been registered.");
	}





	void DatabaseAlertManager::On_AfterMonitorDatabaseAlerts(DatabaseAlertPtr & pDBAlert)
	{
		boost::recursive_mutex::scoped_lock		lk(GetMutex());

		if (pDBAlert)
			pDBAlert->NotifyListeners();
	}






	DatabaseAlertPtr DatabaseAlertManager::MonitorDatabaseAlerts()
	{
		DatabaseAlertPtr		pDBAlert;

		try
		{
			SetMonitoringFlag(true);
			On_BeforeMonitorDatabaseAlerts();
			pDBAlert = m_pDatabase->MonitorDatabaseAlerts(this);
			On_AfterMonitorDatabaseAlerts(pDBAlert);
			SetMonitoringFlag(false);
		}
		catch(...)
		{
			SetMonitoringFlag(false);
			throw;
		}

		return pDBAlert;
	}





	bool DatabaseAlertManager::IsMonitoringAlerts()
	{
		boost::recursive_mutex::scoped_lock		lk(GetMutex());
		return m_bIsMonitoring;
	}





	bool DatabaseAlertManager::HasPendingAlerts()
	{
		boost::recursive_mutex::scoped_lock		lk(GetMutex());

		DatabaseAlertPtr				pDBAlert;
		DatabaseAlertPtrMapItr	itr;

		for ( itr =  m_mapDatabaseAlert.begin();
					itr != m_mapDatabaseAlert.end();
					itr++)
		{
			pDBAlert = itr->second;

			if (pDBAlert->IsEnabled())
				return true;
		}

		return false;;
	}





	void DatabaseAlertManager::SetMonitoringFlag(bool bMonitoring)
	{
		boost::recursive_mutex::scoped_lock		lk(GetMutex());
		m_bIsMonitoring = bMonitoring;
	}




	void DatabaseAlertManager::On_DatabaseAlertCreated(DatabaseAlertPtr pDBAlert)
	{
		assert(pDBAlert);
		assert(pDBAlert->m_pDatabaseAlertManager == this);
		assert(!pDBAlert->GetName().empty());
		assert(m_mapDatabaseAlert.find(pDBAlert->GetName()) == m_mapDatabaseAlert.end());
		m_mapDatabaseAlert[pDBAlert->GetName()] = pDBAlert;
	}





	void DatabaseAlertManager::On_DatabaseAlertDeleted(DatabaseAlertPtr pDBAlert)
	{
		assert(pDBAlert);
		assert(pDBAlert->m_pDatabaseAlertManager == this);
		assert(!pDBAlert->GetName().empty());

		DatabaseAlertPtrMapItr	itr = m_mapDatabaseAlert.find(pDBAlert->GetName());
		assert(itr != m_mapDatabaseAlert.end());
		m_mapDatabaseAlert.erase(itr);
	}





	DatabaseAlertPtr DatabaseAlertManager::GetDatabaseAlert(const std::string & strAlertName)
	{
		DatabaseAlertPtrMapItr		itr = m_mapDatabaseAlert.find(strAlertName);

		if (itr == m_mapDatabaseAlert.end())
			return NULL;

		return itr->second;
	}






	//===========================================================================
	//
	// MetaDatabaseAlert implementation
	//

	MetaDatabaseAlert::MetaDatabaseAlert(MetaDatabasePtr pMetaDatabase, const std::string& strName, const std::string& strSQL)
		: m_pMetaDatabase(pMetaDatabase),
			m_strName(strName),
			m_strSQL(strSQL)
	{
		assert(m_pMetaDatabase);
		assert(!m_strName.empty());
		assert(m_listAlerts.empty());
		m_pMetaDatabase->On_MetaDatabaseAlertCreated(this);
	}





	MetaDatabaseAlert::~MetaDatabaseAlert()
	{
		m_pMetaDatabase->On_MetaDatabaseAlertDeleted(this);
	}






	bool MetaDatabaseAlert::HasDatabaseAlert(DatabaseAlertPtr pDBAlert)
	{
		DatabaseAlertPtrListItr itr;

		for ( itr =  m_listAlerts.begin();
					itr != m_listAlerts.end();
					itr++)
		{
			if (*itr == pDBAlert)
				return true;
		}

		return false;
	}





	void MetaDatabaseAlert::On_DatabaseAlertCreated(DatabaseAlertPtr pDBAlert)
	{
		assert(pDBAlert);
		assert(pDBAlert->m_pMetaDatabaseAlert == this);
		assert(!HasDatabaseAlert(pDBAlert));
		m_listAlerts.push_back(pDBAlert);
	}





	void MetaDatabaseAlert::On_DatabaseAlertDeleted(DatabaseAlertPtr pDBAlert)
	{
		assert(pDBAlert);
		assert(pDBAlert->m_pMetaDatabaseAlert == this);
		assert(HasDatabaseAlert(pDBAlert));
		m_listAlerts.remove(pDBAlert);
	}





	//===========================================================================
	//
	// DatabaseAlert implementation
	//

	DatabaseAlert::DatabaseAlert(DatabaseAlertManagerPtr pDatabaseAlertManager, MetaDatabaseAlertPtr pMetaDatabaseAlert, int iTimeout)
		: m_pDatabaseAlertManager(pDatabaseAlertManager),
			m_pMetaDatabaseAlert(pMetaDatabaseAlert),
			m_iTimeout(60),
			m_bRegistered(false),
			m_bEnabled(false),
			m_iStatus(-1)
	{
		assert(m_pDatabaseAlertManager);
		assert(m_pMetaDatabaseAlert);

		m_pDatabaseAlertManager->On_DatabaseAlertCreated(this);
		m_pMetaDatabaseAlert->On_DatabaseAlertCreated(this);
	}





	DatabaseAlert::~DatabaseAlert()
	{
		m_pDatabaseAlertManager->On_DatabaseAlertDeleted(this);
		m_pMetaDatabaseAlert->On_DatabaseAlertDeleted(this);
	}




	//! The public can add a listener.
	void DatabaseAlert::AddListener(DatabaseAlertListenerFunc pfnCallBack, DatabaseAlertManager::Frequency frequency)
	{
		ListenerListItr			itr;

		// If we already know this, enable it and set the frequency
		for ( itr =  m_listListeners.begin();
					itr != m_listListeners.end();
					itr++)
		{
			if (itr->pfnCallBack == pfnCallBack)
			{
				itr->frequency = frequency;
				itr->bEnabled = true;
				return;
			}
		}

		m_listListeners.push_back(Listener(pfnCallBack, frequency));
	}





	//! The public can remove a listener
	void DatabaseAlert::RemoveListener(DatabaseAlertListenerFunc pfnCallBack)
	{
		ListenerListItr			itr;

		for ( itr =  m_listListeners.begin();
					itr != m_listListeners.end();
					itr++)
		{
			if (itr->pfnCallBack == pfnCallBack)
			{
				m_listListeners.erase(itr);
				return;
			}
		}
	}





	void DatabaseAlert::DisableListener(DatabaseAlertListenerFunc pfnCallBack)
	{
		ListenerPtr		pListener = GetListener(pfnCallBack);

		if (pListener)
			pListener->bEnabled = false;
	}





	void DatabaseAlert::EnableListener(DatabaseAlertListenerFunc pfnCallBack)
	{
		ListenerPtr		pListener = GetListener(pfnCallBack);

		if (pListener)
			pListener->bEnabled = true;
	}





	//! Received if an SQL Server Query Notification has been received
	void DatabaseAlert::NotifyListeners()
	{
		if (m_bEnabled)
		{
			ListenerListItr			itr;

			for ( itr =  m_listListeners.begin();
						itr != m_listListeners.end();
					)
			{
				try
				{
					if (itr->bEnabled)
					{
						(itr->pfnCallBack)(this);

						if (itr->frequency == DatabaseAlertManager::once)
						{
							itr = m_listListeners.erase(itr);
							continue;
						}
					}
				}
				catch(...)
				{
					// make sure we continue notification even if a handler throws
					GenericExceptionHandler("DatabaseAlert::NotifyListeners(): Exception caught.");
				}

				itr++;
			}
		}
	}





	DatabaseAlert::ListenerPtr DatabaseAlert::GetListener(DatabaseAlertListenerFunc pfnCallBack)
	{
		ListenerListItr			itr;

		for ( itr =  m_listListeners.begin();
					itr != m_listListeners.end();
					itr++)
		{
			if (itr->pfnCallBack == pfnCallBack)
				return &(*itr);
		}

		return NULL;
	}






	// ==========================================================================
	// MetaDatabase::Flags class implementation
	//

	// Implement Flags bitmask
	BITS_IMPL(MetaDatabase, Flags);

	// Primitive Flags bitmask values
	PRIMITIVEMASK_IMPL(MetaDatabase, Flags, HasVersionInfo,			0x00000001);
	PRIMITIVEMASK_IMPL(MetaDatabase, Flags, HasObjectLinks,			0x00000002);

	// Combo Permissions bitmask values
	COMBOMASK_IMPL(MetaDatabase, Flags, Default, MetaDatabase::Flags::HasVersionInfo);


	// Implement Permissions bitmask
	BITS_IMPL(MetaDatabase, Permissions);

	// Primitive Permissions bitmask values
	PRIMITIVEMASK_IMPL(MetaDatabase, Permissions, Create,				0x00000001);
	PRIMITIVEMASK_IMPL(MetaDatabase, Permissions, Read,					0x00000002);
	PRIMITIVEMASK_IMPL(MetaDatabase, Permissions, Write,				0x00000004);
	PRIMITIVEMASK_IMPL(MetaDatabase, Permissions, Delete,				0x00000008);

	// Combo Permissions bitmask values
	COMBOMASK_IMPL(MetaDatabase, Permissions, Default,	MetaDatabase::Permissions::Create |
																											MetaDatabase::Permissions::Read |
																											MetaDatabase::Permissions::Write |
																											MetaDatabase::Permissions::Delete);



	//===========================================================================
	//
	// MetaDatabase::TransactionManager implementation
	//

	// Call this method in BeginTransaction and delete the returned pointer when done
	boost::recursive_mutex::scoped_lock* MetaDatabase::TransactionManager::BeginTransaction(DatabasePtr pDB)
	{
		boost::recursive_mutex::scoped_lock*		pAccessLock = NULL;

		while (!pAccessLock)
			pAccessLock = CreateTransaction(pDB);

		return pAccessLock;
	}





	boost::recursive_mutex::scoped_lock* MetaDatabase::TransactionManager::CreateTransaction(DatabasePtr pDB)
	{
		assert(pDB && pDB->GetMetaDatabase() == m_pMD);
		boost::recursive_mutex::scoped_lock*		pAccessLock;


		pAccessLock = UseTransaction(pDB);

		if (pAccessLock)
		{
			m_iCount++;

			// This is a nested transaction from the existing database so honour the request but warn
			ReportWarning("MetaDatabase::TransactionManager::BeginTransaction(): Honouring nesting of transaction for %s (database=[0x" PRINTF_POINTER_MASK "], thread=[% 5i], transaction-level=[%i]), but is it intentional?", m_pMD->GetName().c_str(), m_pDB, m_lThreadID, m_iCount);

			m_tLastAccessed = time(NULL);
		}
		else
		{
			pAccessLock = new boost::recursive_mutex::scoped_lock(m_mtxAccessTransaction);

			if (!m_pDB)
			{
				// We don't have a pending transaction so lets create one
				m_pDB = pDB;

				m_lThreadID = GetCurrentThreadID();

				m_iCount = 1;
				m_bRollback = false;
				m_pLock = new boost::recursive_mutex::scoped_lock(m_mtxBeginTransaction);
				m_tLastAccessed = time(NULL);

				if (m_pDB->m_uTrace & D3DB_TRACE_TRANSACTIONS)
					ReportInfo("TransactionManager::BeginTransaction.......................: Database " PRINTF_POINTER_MASK ".", m_pDB);
			}
			else
			{
				long lThreadID = m_lThreadID;

				delete pAccessLock;
				pAccessLock = NULL;

				// Since requests can come in from ICE, it is possible that the same thread
				// can request another transaction and therefore, the recursive lock will
				// be available for locking immediately. This situation can be recognised if
				// the blocking thread has the same ID as this thread. If this is the case,
				// we sleep and return NULL which will cause the BeginTransaction method
				// to call this method again.
				if (lThreadID == GetCurrentThreadID())
				{
#ifdef AP3_OS_TARGET_WIN32
					Sleep(500);		// find a better way for *nix
#else
					sleep(500);
#endif
				}
				else
				{
					boost::recursive_mutex::scoped_lock		lk(m_mtxBeginTransaction);
				}
			}
		}

		return pAccessLock;
	}





	// Call this method whenever you want to work with a transaction and hang on to the returned pointer until done
	// If this method returns NULL, you must not work with the transaction!
	boost::recursive_mutex::scoped_lock* MetaDatabase::TransactionManager::UseTransaction(DatabasePtr pDB)
	{
		assert(pDB && pDB->GetMetaDatabase() == m_pMD);
		boost::recursive_mutex::scoped_lock*		pAccessLock = new boost::recursive_mutex::scoped_lock(m_mtxAccessTransaction);

		if (m_pDB == pDB)
		{
			m_tLastAccessed = time(NULL);
			return pAccessLock;
		}

		delete pAccessLock;
		return NULL;
	}





	// Call this method whenever you want to end a transaction and hang on to the returned pointer until done
	// If this method returns NULL, you must stop the Commit or Rollback activity immediately and return with an error
	boost::recursive_mutex::scoped_lock* MetaDatabase::TransactionManager::EndTransaction(DatabasePtr pDB, bool bRollback)
	{
		assert(pDB && pDB->GetMetaDatabase() == m_pMD);
		boost::recursive_mutex::scoped_lock*		pAccessLock = new boost::recursive_mutex::scoped_lock(m_mtxAccessTransaction);

		if (m_pDB == pDB)
		{
			m_iCount--;

			if (bRollback)
				m_bRollback = true;

			if (m_pDB->m_uTrace & D3DB_TRACE_TRANSACTIONS)
			{
				if (m_bRollback)
					ReportInfo("TransactionManager::EndTransaction(Rollback)...............: Database " PRINTF_POINTER_MASK ".", m_pDB);
				else
					ReportInfo("TransactionManager::EndTransaction(Commit).................: Database " PRINTF_POINTER_MASK ".", m_pDB);
			}

			if (m_iCount < 1)
			{
				m_pDB = NULL;
				delete m_pLock;
				m_pLock = NULL;
			}
			else
			{
				m_tLastAccessed = time(NULL);
				ReportWarning("MetaDatabase::TransactionManager::EndTransaction(): Ending nested transaction for %s (database=[" PRINTF_POINTER_MASK "], thread=[% 5i], transaction-level=[%i])", m_pMD->GetName().c_str(), m_pDB, m_lThreadID, m_iCount);
			}

			return pAccessLock;
		}

		delete pAccessLock;
		return NULL;
	}





	//! Kill timed out transaction
	/*! This method forecfully terminates a pending transaction if it has been last accessed more
			than uiTransactionTimeout secs ago.
	*/
	void MetaDatabase::TransactionManager::KillTimedOutTransaction(unsigned int uiTransactionTimeout)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxAccessTransaction);

		if (m_pDB)
		{
			if ((unsigned int) (time(NULL) - m_tLastAccessed) > uiTransactionTimeout)
			{
				ReportError("MetaDatabase::TransactionManager::KillTimedOutTransaction(): Forcefully rolling back timed out transaction for %s (database=[" PRINTF_POINTER_MASK "], thread=[% 5i])", m_pMD->GetName().c_str(), m_pDB, m_lThreadID);

				while (m_iCount > 0)
					m_pDB->RollbackTransaction();
			}
		}
	}





	int MetaDatabase::TransactionManager::GetTransactionLevel(DatabasePtr pDB)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxAccessTransaction);

		if (pDB == m_pDB)
			return m_iCount;

		return 0;
	}





	bool MetaDatabase::TransactionManager::ShouldRollback(DatabasePtr pDB)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxAccessTransaction);

		if (m_bRollback)
			return true;

		return false;
	}










	//===========================================================================
	//
	// MetaDatabase implementation
	//
	D3_CLASS_IMPL(MetaDatabase, Object);

	// Statics: Note that the initialisation order is critical
	//
	DatabaseID																MetaDatabase::M_uNextInternalID = DATABASE_ID_MAX;
	MetaDatabasePtrMap												MetaDatabase::M_mapMetaDatabase;
	MetaDatabasePtr														MetaDatabase::M_pDictionaryDatabase;
	DatabaseWorkspacePtr											MetaDatabase::M_pDatabaseWorkspace;


	MetaDatabase::MetaDatabase(DatabaseID uID)
	 :	m_uID(uID),
			m_bInitialised(false),
			m_pInstanceClass(NULL),
			m_bVersionChecked(false)
	{
		m_TransactionManager.m_pMD = this;
		M_mapMetaDatabase.insert(MetaDatabasePtrMap::value_type(m_uID, this));
	}


	MetaDatabase::~MetaDatabase()
	{
		unsigned int	idx;


		// Delete all instances of this
		while (!m_listDatabase.empty())
			delete m_listDatabase.front();

		// Delete all MetaDatabaseAlert instances
		while (!m_mapMetaDatabaseAlert.empty())
			delete m_mapMetaDatabaseAlert.begin()->second;

		// Delete all MetaEntity objects
		for (idx = 0; idx < m_vectMetaEntity.size(); idx++)
			delete m_vectMetaEntity[idx];

		assert(M_mapMetaDatabase.find(m_uID) != M_mapMetaDatabase.end());
		M_mapMetaDatabase.erase(m_uID);
	}


	// Forces class factory registration
	//
	/* static */
	bool MetaDatabase::RegisterClassFactories()
	{
		const char*						pMsg = "Registering Class: %s";

		// Really only here to guarantee these classes are registered with the class factory
		//
		ReportDiagnostic(pMsg, D3MetaDatabase::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3MetaEntity::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3MetaColumn::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3MetaColumnChoice::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3MetaKey::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3MetaKeyColumn::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3MetaRelation::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3Role::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3User::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3HistoricPassword::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3RoleUser::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3Session::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3DatabasePermission::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3EntityPermission::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3ColumnPermission::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, D3RowLevelPermission::ClassObject().Name().c_str());

		ReportDiagnostic(pMsg, HSTopic::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, HSTopicAssociation::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, HSTopicLink::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, HSResource::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, HSResourceUsage::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, HSMetaDatabaseTopic::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, HSMetaEntityTopic::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, HSMetaColumnTopic::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, HSMetaKeyTopic::ClassObject().Name().c_str());
		ReportDiagnostic(pMsg, HSMetaRelationTopic::ClassObject().Name().c_str());

#ifdef APAL_SUPPORT_ODBC
		ReportDiagnostic(pMsg, ODBCDatabase::ClassObject().Name().c_str());
#endif

#ifdef APAL_SUPPORT_OTL
		ReportDiagnostic(pMsg, OTLDatabase::ClassObject().Name().c_str());
#endif

		return true;
	}


	// Static method which returns the MetaDatabase object with a name or Alias matching strName.
	//
	MetaDatabasePtr MetaDatabase::GetMetaDatabase(const std::string & strName)
	{
		MetaDatabasePtrMapItr	itr;
		MetaDatabasePtr				pMDB;


		// Find the matching MetaDatabase
		for ( itr =  M_mapMetaDatabase.begin();
					itr != M_mapMetaDatabase.end();
					itr++)
		{
			pMDB = itr->second;

			if (pMDB && (pMDB->GetName() == strName || pMDB->GetAlias() == strName))
				return pMDB;
		}

		return NULL;
	}


	// Static method which returns the n'th MetaDatabase object in the map where n = idx
	//
	MetaDatabasePtr MetaDatabase::GetMetaDatabase(unsigned int idx)
	{
		MetaDatabasePtrMapItr	itr;
		MetaDatabasePtr				pMDB;
		unsigned int					ipos = 0;


		// Find the matching MetaDatabase
		for ( itr =  M_mapMetaDatabase.begin();
					itr != M_mapMetaDatabase.end();
					itr++, ipos++)
		{
			pMDB = itr->second;

			if (ipos == idx)
				return pMDB;
		}

		return NULL;
	}


	// Static method which returns the MetaDatabase object with a name or Alias matching strName.
	//
	MetaDatabasePtr MetaDatabase::GetMetaDatabase(MetaDatabaseDefinition & MD)
	{
		MetaDatabasePtrMapItr	itr;
		MetaDatabasePtr				pMDB;


		// Find the matching MetaDatabase
		for ( itr =  M_mapMetaDatabase.begin();
					itr != M_mapMetaDatabase.end();
					itr++)
		{
			pMDB = itr->second;

			if (pMDB																														&&
					pMDB->m_strName						== MD.m_strName												&&
					pMDB->m_strAlias					== MD.m_strAlias											&&
					pMDB->m_uVersionMajor			== (unsigned) MD.m_iVersionMajor			&&
					pMDB->m_uVersionMinor			== (unsigned) MD.m_iVersionMinor			&&
					pMDB->m_uVersionRevision	== (unsigned) MD.m_iVersionRevision)
				return pMDB;
		}

		return NULL;
	}


	bool MetaDatabase::CreatePhysicalDatabase()
	{
		if (m_pInstanceClass)
		{
#ifdef APAL_SUPPORT_ODBC
			if (m_pInstanceClass->Name() == "ODBCDatabase")
				return ODBCDatabase::CreatePhysicalDatabase(this);
#endif

#ifdef APAL_SUPPORT_OTL
			if (m_pInstanceClass->Name() == "OTLDatabase")
				return OTLDatabase::CreatePhysicalDatabase(this);
#endif
		}

		throw Exception(__FILE__, __LINE__, Exception_error, "MetaDatabase::CreatePhysicalDatabase(): Don't know how to make database %s, the database class name is not supported in this build, unknown or invalid.", GetName().c_str());

		return false;
	}





	/*! This method expects an XML Node as input which has the format:

			<Alerts>
				<Alert>
					<Name>RBAC Changed</Name>
					<SQLQuery>
						SELECT ID, Name, Enabled, Features, IRSSettings, V3ParamSettings, T3ParamSettings, P3ParamSettings FROM dbo.D3Role;
						SELECT ID, Name, Password, Enabled FROM dbo.D3User;
						SELECT RoleID, UserID FROM dbo.D3RoleUser;
						SELECT RoleID, MetaDatabaseID, AccessRights FROM dbo.D3DatabasePermission;
						SELECT RoleID, MetaEntityID, AccessRights FROM dbo.D3EntityPermission;
						SELECT RoleID, MetaColumnID, AccessRights FROM dbo.D3ColumnPermission
					</SQLQuery>
				</Alert>
			</Alerts>

			The method creates a DatabaseAlert object for each Alert
			element found and adds it to this' list of alerts.
	*/
	void MetaDatabase::InitialiseMetaAlerts(CSL::XML::Node* pAlerts)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxExclusive);

		CSL::XML::Node*					pNode = pAlerts;
		CSL::XML::Node*					pChildNode;
		CSL::XML::NodeList*			pChildNodes = NULL;
		std::string							strName, strSQL;
		unsigned long						i;


		try
		{
			if (pNode && pNode->getNodeName() == "Alerts")
			{
				pNode	= pNode->getFirstChild();

				while (pNode)
				{
					// We're only interested into Alert elements
					if (pNode->getNodeName() == "Alert")
					{
						strName.erase();
						strSQL.erase();

						pChildNodes = pNode->getChildNodes();

						for (i = 0; i < pChildNodes->getLength(); i++)
						{
							pChildNode = pChildNodes->item(i);

							if (pChildNode)
							{
								// We're after the "Name" node
								if (pChildNode->getNodeName() == "Name")
									strName = pChildNode->getFirstChild()->getNodeValue();
								// and "SQLQuery" node
								else if (pChildNode->getNodeName() == "SQLQuery")
									strSQL = pChildNode->getFirstChild()->getNodeValue();
							}
						}

						// Create an alert if we have a name
						if (!strName.empty())
						{
							if (GetMetaDatabaseAlert(strName))
								ReportError("MetaDatabase::InitialiseMetaAlerts: Alert %s already exists, check you application initialisation file", strName.c_str());
							else
								new MetaDatabaseAlert(this, strName, strSQL);
						}
						else
						{
							ReportError("MetaDatabase::InitialiseMetaAlerts: An Alert was found but it has no Name, check you application initialisation file");
						}

						pChildNodes->release();
						pChildNodes = NULL;
					}
					pNode = pNode->getNextSibling();
				}
			}
		}
		catch (...)
		{
			if (pChildNodes)
				pChildNodes->release();
			throw;
		}
	}





	MetaDatabaseAlertPtr MetaDatabase::GetMetaDatabaseAlert(const std::string& strName)
	{
		MetaDatabaseAlertPtrMapItr	itr = m_mapMetaDatabaseAlert.find(strName);

		if (itr == m_mapMetaDatabaseAlert.end())
			return NULL;

		return itr->second;
	}





	// The first time we create an instance, we need to create our global instance
	// and load all entities declared as cached
	//
	DatabasePtr MetaDatabase::CreateInstance(DatabaseWorkspacePtr pDatabaseWorkspace)
	{
		DatabasePtr		pDB = NULL;
		DatabasePtr		pGlobalDB = NULL;


		// Sanity checks
		//
		assert(m_bInitialised);
		assert(pDatabaseWorkspace);

		if (!m_pInstanceClass)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaDatabase::CreateInstance(): Can't create instnace of MetaDatabase %s, not class factory available.", GetName().c_str());

		// First get the global database
		//
		pGlobalDB = M_pDatabaseWorkspace->FindDatabase(this);

		// If it doesn't exist, create it and load all instances of entities marked as cached
		//
		if (!pGlobalDB)
		{
			try
			{
				pGlobalDB = (DatabasePtr) m_pInstanceClass->CreateInstance();
				pGlobalDB->m_pMetaDatabase = this;
				pGlobalDB->m_pDatabaseWorkspace = M_pDatabaseWorkspace;
				pGlobalDB->On_PostCreate();
				pGlobalDB->Connect();
				pGlobalDB->LoadCache();
			}
			catch(...)
			{
				delete pGlobalDB;
				throw;
			}
		}

		if (pDatabaseWorkspace == M_pDatabaseWorkspace)
			return pGlobalDB;

		try
		{
			pDB = (DatabasePtr) m_pInstanceClass->CreateInstance();
			pDB->m_pMetaDatabase = this;
			pDB->m_pDatabaseWorkspace = pDatabaseWorkspace;
			pDB->On_PostCreate();
			pDB->Connect();
		}
		catch(...)
		{
			delete pDB;
			throw;
		}

		return pDB;
	}


	// NOTE: If you change this, also change XMLImportFileProcessor::On_AfterProcessVersionRevisionElement()
	std::string MetaDatabase::GetVersion() const
	{
		char		szBuffer[128];

		snprintf(szBuffer, 128, "V%i.%02i.%04i", m_uVersionMajor, m_uVersionMinor, m_uVersionRevision);

		return szBuffer;
	}


	MetaEntityPtr MetaDatabase::GetMetaEntity(const std::string & strName)
	{
		for (unsigned int idx = 0; idx < m_vectMetaEntity.size(); idx++)
			if (m_vectMetaEntity[idx] &&
				 (m_vectMetaEntity[idx]->GetName()	== strName	||
					m_vectMetaEntity[idx]->GetAlias() == strName))
				return m_vectMetaEntity[idx];

		return NULL;
	}



	std::string MetaDatabase::GetISOToServerDate(const std::string & strISODate) const
	{
		std::string		strServerDate;
		D3Date				dt(strISODate);

		dt.AdjustToTimeZone(GetTimeZone());

		switch (GetTargetRDBMS())
		{
			case SQLServer:
				strServerDate = dt.AsString(3);
				break;

			case Oracle:
				strServerDate = dt.AsString(6);
				break;
		}

		return strServerDate;
	}



	std::string MetaDatabase::GetServerDateToISO(const std::string & strServerDate) const
	{
		std::string		strISODate;
		D3Date				dt(strServerDate, GetTimeZone());

		switch (GetTargetRDBMS())
		{
			case SQLServer:
				strISODate = dt.AsISOString(3);
				break;

			case Oracle:
				strISODate = dt.AsISOString(6);
				break;
		}

		return strISODate;
	}



	std::string MetaDatabase::GetVersionInfoMetaEntityName()
	{
		std::string		strVersionInfoName;

		if (HasVersionInfo())
		{
			strVersionInfoName  = GetAlias();
			strVersionInfoName += "D3VI";
		}

		return strVersionInfoName;
	}



	// Generate Files supporting this MetaDictionary
	//
	bool MetaDatabase::GenerateSourceCode()
	{
		if (!CreateCPPFiles())
			return false;

		if (!CreateICEFiles())
			return false;

		return true;
	}



	// Creates a CPP header file for this database
	//
	bool MetaDatabase::CreateCPPFiles()
	{
		std::ofstream		fout;
		unsigned int		idx;
		MetaEntityPtr		pME;
		std::string			strFileName;


		// Build file name
		strFileName = "Generated/";
		strFileName += GetAlias();
		strFileName += ".h";

		// Open the Header file
		//
		fout.open(strFileName.c_str(), std::ios::out | std::ios::trunc);

		if (fout.fail())
		{
			ReportError("MetaDatabase::CreateCPPFiles(): Failed to open/create header file %s.", strFileName.c_str());
			return false;
		}

		// Write the include blocker
		//
		fout << "#ifndef _INC_" << GetAlias().c_str() << "_H_" << std::endl;
		fout << "#define _INC_" << GetAlias().c_str() << "_H_" << std::endl;
		fout <<  std::endl;
		fout << "// This header describes types for database " << GetAlias().c_str() << " and has been generated by D3." << std::endl;
		fout << "//" << std::endl;
		fout << std::endl;

		fout << "// All declarations are inside the D3 namespace" << std::endl;
		fout << "//" << std::endl;
		fout << "namespace D3" << std::endl;
		fout << "{" << std::endl;

		// Create table enum
		//
		fout << "\t// Tables in this database" << std::endl;
		fout << "\t//" << std::endl;
		fout << "\tenum " << GetAlias().c_str() << "_Tables" << std::endl;
		fout << "\t{" << std::endl;

		for (idx = 0; idx < m_vectMetaEntity.size(); idx++)
		{
			pME = m_vectMetaEntity[idx];

			if (idx > 0)
				fout << "," << std::endl;

			fout << "\t\t" << GetAlias().c_str() << "_" << pME->GetName().c_str();
		}

		fout << std::endl;
		fout << "\t};" << std::endl;

		// Let each entity do its bits
		//
		for (idx = 0; idx < m_vectMetaEntity.size(); idx++)
		{
			pME = m_vectMetaEntity[idx];
			pME->AddToCPPHeader(fout);
		}


		fout << "}\t\t// namespace D3 ends here" << std::endl;
		fout << std::endl;

		fout << "#endif /* _INC_" << GetAlias().c_str() << "_H_ */" << std::endl;
		fout.close();

		// Get each entity to create it's specialisation base class (if it is specialised)
		//
		for (idx = 0; idx < m_vectMetaEntity.size(); idx++)
		{
			pME = m_vectMetaEntity[idx];
			pME->CreateSpecialisedCPPHeader();
			pME->CreateSpecialisedCPPSource();
		}


		return true;
	}




	// Creates a CPP header file for this database
	//
	bool MetaDatabase::CreateICEFiles()
	{
		if (!CreateICEForwardDecls())
			return false;

		if (!CreateICEBaseInterface())
			return false;

		if (!CreateICETemplateInterface())
			return false;

		if (!CreateHBaseInterface())
			return false;

		if (!CreateHTemplateInterface())
			return false;

		if (!CreateCPPBaseInterface())
			return false;

		if (!CreateCPPTemplateInterface())
			return false;

		return true;
	}





	bool MetaDatabase::CreateICEForwardDecls()
	{
		std::ostringstream		ostrm;
		std::ofstream					fout;
		unsigned int					idx;
		MetaEntityPtr					pME;
		std::string						strFileName;
		int										iAlign = 60;


		// Build file name
		//
		strFileName = "Generated/I_";
		strFileName += GetAlias();
		strFileName += "Types.ice";

		// Open the Header file
		//
		fout.open(strFileName.c_str(), std::ios::out | std::ios::trunc);

		if (fout.fail())
		{
			ReportError("MetaDatabase::CreateICEForwardDecls(): Failed to open/create ice interface file %s.", strFileName.c_str());
			return false;
		}

		// Choose left allignment for fields
		fout << std::setiosflags(std::ios::left);

		// Write the include blocker
		//
		fout << "#ifndef _I_" << GetAlias() << "Types_ICE_\n";
		fout << "#define _I_" << GetAlias() << "Types_ICE_\n\n";
		fout << "//" << std::endl;
		fout << "// WARNING: This file has been generated by D3. You should NEVER change this file." << std::endl;
		fout << "//          Please refer to the documentation of MetaDatabase::CreateSourceCode()" << std::endl;
		fout << "//          which explains in detail how you can gegenerate this file." << std::endl;
		fout << "//" << std::endl;
		fout << "// This header declares Ice interfaces types for Database " << GetInstanceInterface() << "\n\n";

		fout << std::endl;

		fout << "module ID3\n";
		fout << "{\n";
		fout << "\t// ============================================================\n";
		fout << "\t//\n";
		fout << "\t// Begin forward declaration Base and Custom Interfaces\n";
		fout << "\t//\n\n";

		// Forward declare base interfaces
		fout << "\t// Base interface forward declarations\n";

		fout << "\tinterface " << GetInstanceInterface() << "Base;\n\n";

		for (idx = 0; idx < m_vectMetaEntity.size(); idx++)
		{
			pME = m_vectMetaEntity[idx];

			if (!pME->GetInstanceInterface().empty())
				fout << "\tinterface " << pME->GetInstanceInterface() << "Base;\n";
		}

		fout << std::endl;

		// Forward declare custom interfaces
		fout << "\t// Custom interfaces forward declarations\n";

		fout << "\tinterface " << GetInstanceInterface() << ";\n\n";

		for (idx = 0; idx < m_vectMetaEntity.size(); idx++)
		{
			pME = m_vectMetaEntity[idx];

			if (!pME->GetInstanceInterface().empty())
				fout << "\tinterface " << pME->GetInstanceInterface() << ";\n";
		}

		fout << std::endl;

		// Forward declare sequences for member entities
		fout << "\t// Custom sequences forward declarations\n";

		for (idx = 0; idx < m_vectMetaEntity.size(); idx++)
		{
			pME = m_vectMetaEntity[idx];

			if (!pME->GetInstanceInterface().empty())
			{
				ostrm << "\tsequence<" << pME->GetInstanceInterface() << "*>";
				fout << std::setw(iAlign) << ostrm.str();
				ostrm.str("");
				fout << Pluralize(pME->GetInstanceInterface()) << ";\n";
			}
		}

		fout << std::endl;

		// Forward declare custom result set interfaces
		fout << "\t// Custom ResultSet interfaces forward declarations\n";

		for (idx = 0; idx < m_vectMetaEntity.size(); idx++)
		{
			pME = m_vectMetaEntity[idx];

			if (!pME->GetInstanceInterface().empty())
				fout << "\tinterface " << pME->GetInstanceInterface() << "ResultSet;\n";
		}

		fout << "};\n\n\n\n";
		fout << "#endif /* _I_" << GetAlias() << "Types_ICE_ */\n";
		fout.close();

		return true;
	}





	bool MetaDatabase::CreateICEBaseInterface()
	{
		std::ostringstream			ostrm;
		std::ofstream						fout;
		MetaDatabasePtrMapItr		itr;
		unsigned int						idx1, idx2, idx3;
		MetaDatabasePtr					pMD;
		MetaEntityPtr						pME;
		MetaRelationPtr					pMR;
		std::string							strFileName;
		int											iAlign1 = 60, iAlign2 = 162;
		bool										bFirst;
#ifdef IMPLEMENT_ICE_COLUMN_INTERFACES
		MetaColumnPtr						pMC;
#endif

		// Build file name
		//
		strFileName = "Generated/I_";
		strFileName += GetAlias();
		strFileName += "Base.ice";

		// Open the Header file
		//
		fout.open(strFileName.c_str(), std::ios::out | std::ios::trunc);

		if (fout.fail())
		{
			ReportError("MetaDatabase::CreateICEBaseInterface(): Failed to open/create ice interface file %s.", strFileName.c_str());
			return false;
		}

		// Choose left allignment for fields
		fout << std::setiosflags(std::ios::left);

		// Write the include blocker
		//
		fout << "#ifndef _I_" << GetAlias() << "Base_ICE_\n";
		fout << "#define _I_" << GetAlias() << "Base_ICE_\n\n";
		fout << "//" << std::endl;
		fout << "// WARNING: This file has been generated by D3. You should NEVER change this file." << std::endl;
		fout << "//          Please refer to the documentation of MetaDatabase::CreateSourceCode()" << std::endl;
		fout << "//          which explains in detail how you can gegenerate this file." << std::endl;
		fout << "//" << std::endl;
		fout << "// This header declares base Ice interfaces for Database " << GetInstanceInterface() << "\n\n";

		fout << "#include \"I_D3.ice\"\n\n";

		// Include interface types
		if (this != GetMetaDictionary() && this->GetAlias() != "D3HSDB")
		{
			for ( itr =  MetaDatabase::GetMetaDatabases().begin();
						itr != MetaDatabase::GetMetaDatabases().end();
						itr++)
			{
				pMD = itr->second;

				if (pMD != GetMetaDictionary() && pMD->GetAlias() != "D3HSDB")
					fout << "#include \"I_" << pMD->GetAlias() << "Types.ice\"\n";
			}
		}
		else
		{
			fout << "#include \"I_" << GetAlias() << "Types.ice\"\n";
		}

		fout << std::endl;
		fout << std::endl;
		fout << std::endl;

		fout << "module ID3\n";
		fout << "{\n";
		fout << "\t// ============================================================\n";
		fout << "\t//\n";
		fout << "\t// Begin base interface declarations\n";
		fout << "\t//\n\n";

		// Create the base interface declaration for the database itself
		fout << "\tinterface " << GetInstanceInterface() << "Base extends Database\n";
		fout << "\t{\n";

		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			pME = m_vectMetaEntity[idx1];

			ostrm.str("");

			if (!pME->GetInstanceInterface().empty())
			{
				ostrm << "\t\tidempotent " << pME->GetInstanceInterface() << "ResultSet*";
				fout << std::setw(iAlign1) << ostrm.str();
				ostrm.str("");
				ostrm << Pluralize(pME->GetInstanceInterface()) << "(int iRecsPerPage, string strFilter, string strOrder)";
			}
			else
			{
				fout << std::setw(iAlign1) << "\t\tidempotent ResultSet*";
				ostrm << Pluralize(pME->GetName()) << "(int iRecsPerPage, string strFilter, string strOrder)";
			}

			fout << std::setw(iAlign2) << ostrm.str();
			fout << "throws APALSvcError;\n";
		}

		fout << "\t};\n\n\n\n";


		// Create the base interface declaration for each meta entity
		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			bFirst = true;
			pME = m_vectMetaEntity[idx1];

			if (!pME->GetInstanceInterface().empty())
			{
				fout << "\tinterface " << pME->GetInstanceInterface() << "Base extends Entity\n";
				fout << "\t{\n";

				// Create parent relation accessors
				for (idx2 = 0; idx2 < pME->GetParentMetaRelations()->size(); idx2++)
				{
					pMR = pME->GetParentMetaRelation(idx2);

					// Only generate generate this for the first child relation with this name
					//
					for (idx3 = 0; idx3 < idx2; idx3++)
					{
						if (pME->GetParentMetaRelation(idx3)->GetReverseName() == pMR->GetReverseName())
							break;
					}

					if (idx3 < idx2)
						continue;

					if (bFirst)
					{
						bFirst = false;
						fout << "\t\t// Parents accessors\n";
					}

					ostrm.str("");
					if (!pMR->GetParentMetaKey()->GetMetaEntity()->GetInstanceInterface().empty())
					{
						pMD = pMR->GetParentMetaKey()->GetMetaEntity()->GetMetaDatabase();
						ostrm << "\t\tidempotent " << pMR->GetParentMetaKey()->GetMetaEntity()->GetInstanceInterface() << '*';
					}
					else
					{
						ostrm << "\t\tidempotent Entity*";
					}

					fout << std::setw(iAlign1) << ostrm.str();
					ostrm.str("");
					ostrm << "Get" << pMR->GetReverseName() << "()";
					fout << std::setw(iAlign2) << ostrm.str() << "throws APALSvcError;\n";
				}

				if (!bFirst)
				{
					fout << std::endl;
					bFirst = true;
				}

				// Create child relation accessors
				for (idx2 = 0; idx2 < pME->GetChildMetaRelations()->size(); idx2++)
				{
					pMR = pME->GetChildMetaRelation(idx2);

					// Only generate generate this for the first child relation with this name
					//
					for (idx3 = 0; idx3 < idx2; idx3++)
					{
						if (pME->GetChildMetaRelation(idx3)->GetName() == pMR->GetName())
							break;
					}

					if (idx3 < idx2)
						continue;

					if (bFirst)
					{
						bFirst = false;
						fout << "\t\t// Children accessors\n";
					}

					ostrm.str("");
					if (!pMR->GetChildMetaKey()->GetMetaEntity()->GetInstanceInterface().empty())
					{
						pMD = pMR->GetChildMetaKey()->GetMetaEntity()->GetMetaDatabase();
						ostrm << "\t\tidempotent " << pMR->GetChildMetaKey()->GetMetaEntity()->GetInstanceInterface() << "ResultSet*";
					}
					else
					{
						ostrm << "\t\tidempotent ResultSet*";
					}

					fout << std::setw(iAlign1) << ostrm.str();
					ostrm.str("");
					ostrm << "Get" << pMR->GetName() << "(int iRecsPerPage)";
					fout << std::setw(iAlign2) << ostrm.str() << "throws APALSvcError;\n";
				}

#ifdef IMPLEMENT_ICE_COLUMN_INTERFACES
				if (!bFirst)
				{
					fout << std::endl;
					bFirst = true;
				}

				// Create attribute getters
				for (idx2 = 0; idx2 < pME->GetMetaColumns()->size(); idx2++)
				{
					pMC = pME->GetMetaColumn(idx2);

					if (bFirst)
					{
						bFirst = false;
						fout << "\t\t// Attribute Getters\n";
					}

					ostrm.str("");
					ostrm << "\t\tidempotent " << pMC->GetTypeAsIceIType();
					fout << std::setw(iAlign1) << ostrm.str();
					ostrm.str("");
					ostrm << "Get" << IcifyIdentifier(pMC->GetName()) << "()";
					fout << std::setw(iAlign2) << ostrm.str() << "throws APALSvcError;\n";
				}

				if (!bFirst)
				{
					fout << std::endl;
					bFirst = true;
				}

				// Create attribute setters
				for (idx2 = 0; idx2 < pME->GetMetaColumns()->size(); idx2++)
				{
					pMC = pME->GetMetaColumn(idx2);

					if (pMC->IsUpdatable())
					{
						if (bFirst)
						{
							bFirst = false;
							fout << "\t\t// Attribute Setters\n";
						}

						fout << std::setw(iAlign1) << "\t\tvoid ";
						ostrm.str("");
						ostrm << "Set" << IcifyIdentifier(pMC->GetName()) << '(' <<  pMC->GetTypeAsIceIType() << " newVal)";
						fout << std::setw(iAlign2) << ostrm.str() << "throws APALSvcError;\n";
					}
				}
#endif

				fout << "\t};\n\n\n\n";
			}
		}

		// Implement custom result set interfaces
		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			pME = m_vectMetaEntity[idx1];

			if (!pME->GetInstanceInterface().empty())
			{
				fout << "\tinterface " << pME->GetInstanceInterface() << "ResultSet extends ResultSet\n";
				fout << "\t{\n";

				ostrm.str("");
				ostrm << "\t\tidempotent " << Pluralize(pME->GetInstanceInterface());
				fout << std::setw(iAlign1) << ostrm.str();

				ostrm.str("");
				ostrm << "Get" << Pluralize(pME->GetInstanceInterface()) << "()";
				fout << std::setw(iAlign2) << ostrm.str() << "throws APALSvcError;\n";
				fout << "\t};\n\n\n\n";
			}
		}

		fout << "};\n\n\n\n";
		fout << "#endif /* _I_" << GetAlias() << "Base_ICE_ */\n";
		fout.close();

		return true;
	}





	bool MetaDatabase::CreateICETemplateInterface()
	{
		std::ofstream					fout;
		unsigned int					idx1;
		MetaEntityPtr					pME;
		std::string						strFileName;


		// -------------------------------------------------------
		// Phase II: Create a template ice interface file for this

		// Build file name
		//
		strFileName = "Generated/I_";
		strFileName += GetInstanceInterface();
		strFileName += ".ice.tmpl";

		// Open the Header file
		//
		fout.open(strFileName.c_str(), std::ios::out | std::ios::trunc);

		if (fout.fail())
		{
			ReportError("MetaDatabase::CreateICETemplateInterface(): Failed to open/create ice interface file %s.", strFileName.c_str());
			return false;
		}

		// Choose left allignment for fields
		fout << std::setiosflags(std::ios::left);

		// Write the include blocker
		//
		fout << "#ifndef _I_" << GetAlias() << "_ICE_\n";
		fout << "#define _I_" << GetAlias() << "_ICE_\n\n";

		fout << "// This template declares Ice interfaces derived from the base interfaces for Database " << GetInstanceInterface() << "\n\n";

		fout << "#include \"I_" << GetAlias() << "Base.ice\"\n\n";

		fout << "module ID3\n";
		fout << "{\n";

		fout << "\t// ============================================================\n";
		fout << "\t//\n";
		fout << "\t// Forward declarations\n";
		fout << "\t//\n\n";
		fout << "\t// todo: Add your forward declarations here\n\n\n\n";

		fout << "\t// ============================================================\n";
		fout << "\t//\n";
		fout << "\t// I_" << GetAlias() << " interface declarations\n";
		fout << "\t//\n\n";

		// Create the interface declaration for the database itself
		fout << "\tinterface " << GetInstanceInterface() << " extends " << GetInstanceInterface() << "Base\n";
		fout << "\t{\n";
		fout << "\t\t// todo: Add other interfaces you need for this class here\n";
		fout << "\t};\n\n\n\n";

		// Create the interface declaration for the database's entities
		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			pME = m_vectMetaEntity[idx1];

			if (pME->GetInstanceInterface().empty())
				continue;

			fout << "\tinterface " << pME->GetInstanceInterface() << " extends " << pME->GetInstanceInterface() << "Base\n";
			fout << "\t{\n";
			fout << "\t\t// todo: Add other interfaces you need for this class here\n";
			fout << "\t};\n\n\n\n";
		}

		fout << "};\n\n\n\n";
		fout << "#endif /* _I_" << GetAlias() << "_ICE_ */\n";
		fout.close();

		return true;
	}





	bool MetaDatabase::CreateHBaseInterface()
	{
		std::ostringstream		ostrm;
		std::ofstream					fout;
		MetaDatabasePtrMapItr	itr;
		unsigned int					idx1, idx2, idx3;
		MetaDatabasePtr				pMD;
		MetaEntityPtr					pME;
		MetaRelationPtr				pMR;
		std::string						strFileName;
		int										iAlign = 60;
		bool									bFirst = true;
#ifdef IMPLEMENT_ICE_COLUMN_INTERFACES
		MetaColumnPtr					pMC;
#endif


		// Build file name
		//
		strFileName = "Generated/IF";
		strFileName += GetAlias();
		strFileName += "Base.h";

		// Open the Header file
		//
		fout.open(strFileName.c_str(), std::ios::out | std::ios::trunc);

		if (fout.fail())
		{
			ReportError("MetaDatabase::CreateHBaseInterface(): Failed to open/create ice interface file %s.", strFileName.c_str());
			return false;
		}

		// Choose left allignment for fields
		fout << std::setiosflags(std::ios::left);

		// Write the include blocker
		//
		fout << "#ifndef _IF" << GetAlias() << "Base_H_\n";
		fout << "#define _IF" << GetAlias() << "Base_H_\n\n";

		fout << "// MODULE: IF" << GetAlias() << "Base includes the C++ declarations for the generated base interface I_" << GetAlias() << "Base.ice\n\n";

		fout << "// WARNING: This file has been generated by D3. You should NEVER change this file.\n";
		fout << "//          Please refer to the documentation of MetaDatabase::CreateSourceCode()\n";
		fout << "//          which explains in detail how you can gegenerate this file.\n\n";

		fout << "#include <" << GetAlias() << ".h>\n";
		fout << "#include \"APALIce.h\"\n";
		fout << "#include \"IFD3.h\"\n";

		// Include interface types
		if (this != GetMetaDictionary() && this->GetAlias() != "D3HSDB")
		{
			for ( itr =  MetaDatabase::GetMetaDatabases().begin();
						itr != MetaDatabase::GetMetaDatabases().end();
						itr++)
			{
				pMD = itr->second;

				if (pMD != GetMetaDictionary() && pMD->GetAlias() != "D3HSDB")
				{
					fout << "#include \"I_" << pMD->GetAlias() << "Types.h\"\n";
					fout << "#include \"I_" << pMD->GetAlias() << "Base.h\"\n";
				}
			}
		}
		else
		{
			fout << "#include \"I_" << GetAlias() << "Types.h\"\n";
			fout << "#include \"I_" << GetAlias() << "Base.h\"\n";
		}

		fout << std::endl;

		fout << "//! This namespace contains generated base interface declarations and declarations for classes specialising the base interfaces\n";
		fout << "namespace ID3\n";
		fout << "{\n";

		// ------------------------------------------------
		// Build the base interface c++ declaration for this
		fout << "\t//! The " << GetInstanceInterface() << "BaseI implements a basic inerface to an internal D3 object representing an instance of Database " << GetAlias() << "\n";
		fout << "\tclass " << GetInstanceInterface() << "BaseI : public virtual " << GetInstanceInterface() << "Base, public virtual DatabaseI\n";
		fout << "\t{\n";
		fout << "\t\tpublic:\n";

		// Create something like this for each MetaEntity belonging to this:
		// virtual WarehouseResultSetPrx							Warehouses		(Ice::Int iRecsPerPage, const ::std::string& strFilter, const ::std::string& strOrder, const Ice::Current& = Ice::Current());
		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			pME = m_vectMetaEntity[idx1];

			if (bFirst)
			{
				bFirst = false;
				fout << "\t\t\t//@{\n";
				fout << "\t\t\t//! The following set of methods enable access to objects through ResultSets\n";
			}

			if (!pME->GetInstanceInterface().empty())
			{
				ostrm << "\t\t\tvirtual " << pME->GetInstanceInterface() << "ResultSetPrx";
				fout << std::setw(iAlign) << ostrm.str();
				ostrm.str("");
				fout << std::setw(30) << Pluralize(pME->GetInstanceInterface()) << "(Ice::Int iRecsPerPage, const ::std::string& strFilter, const ::std::string& strOrder, const Ice::Current& = Ice::Current());\n";
			}
			else
			{
				fout << std::setw(iAlign) << "\t\t\tvirtual ResultSetPrx";
				fout << std::setw(30) << Pluralize(pME->GetName()) << "(Ice::Int iRecsPerPage, const ::std::string& strFilter, const ::std::string& strOrder, const Ice::Current& = Ice::Current());\n";
			}
		}

		if (!bFirst)
		{
			fout << "\t\t\t//@}\n\n";
			bFirst = true;
		}

		// Build a static member that can build a proxy interface based on an instance that it wraps
		fout << "\t\t\t//! Constructs a " << GetAlias() << " proxy object which belongs to the session specified and wraps the passed in D3::DatabasePtr\n";

		ostrm << "\t\t\tstatic " << GetAlias() << "Prx";
		fout << std::setw(iAlign) << ostrm.str();
		ostrm.str("");

		fout << std::setw(30) << "CreateProxy" << "(D3::LockedSessionPtr & pSession, D3::DatabaseWorkspacePtr pDBWS, D3::DatabasePtr pDB, const Ice::Current& iceCurrent);\n";

		fout << "\n\t};\n\n\n\n\n";


		// ------------------------------------------------
		// Build the base interface c++ declaration for this's entities
		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			pME = m_vectMetaEntity[idx1];

			if (!pME->GetInstanceInterface().empty())
			{
				fout << "\t//! " << pME->GetInstanceInterface() << "BaseI implements a standard interface into " << pME->GetInstanceClassName() << " objects\n";
				fout << "\t/*! The class implements direct access to attributes and relationships\n";
				fout << "\t*/\n";
				fout << "\tclass " << pME->GetInstanceInterface() << "BaseI : public virtual " << pME->GetInstanceInterface() << "Base, public virtual EntityI\n";
				fout << "\t{\n";
				fout << "\t\tpublic:\n";

				// Create parent relation accessors
				for (idx2 = 0; idx2 < pME->GetParentMetaRelations()->size(); idx2++)
				{
					pMR = pME->GetParentMetaRelation(idx2);

					// Only generate generate this for the first child relation with this name
					//
					for (idx3 = 0; idx3 < idx2; idx3++)
					{
						if (pME->GetParentMetaRelation(idx3)->GetReverseName() == pMR->GetReverseName())
							break;
					}

					if (idx3 < idx2)
						continue;

					if (bFirst)
					{
						bFirst = false;
						fout << "\t\t\t//@{\n";
						fout << "\t\t\t//! Parents accessors\n";
					}

					if (!pMR->GetParentMetaKey()->GetMetaEntity()->GetInstanceInterface().empty())
					{
						pMD = pMR->GetParentMetaKey()->GetMetaEntity()->GetMetaDatabase();
						ostrm << "\t\t\tvirtual " << pMR->GetParentMetaKey()->GetMetaEntity()->GetInstanceInterface() << "Prx";
					}
					else
					{
						ostrm << "\t\t\tvirtual EntityPrx";
					}

					fout << std::setw(iAlign - 1) << ostrm.str() << " Get";
					ostrm.str("");

					fout << std::setw(30 - 3) << pMR->GetReverseName() << "(const Ice::Current& = Ice::Current());\n";
				}

				if (!bFirst)
				{
					fout << "\t\t\t//@}\n\n";
					bFirst = true;
				}

				// Create child relation accessors
				for (idx2 = 0; idx2 < pME->GetChildMetaRelations()->size(); idx2++)
				{
					pMR = pME->GetChildMetaRelation(idx2);

					// Only generate generate this for the first child relation with this name
					//
					for (idx3 = 0; idx3 < idx2; idx3++)
					{
						if (pME->GetChildMetaRelation(idx3)->GetName() == pMR->GetName())
							break;
					}

					if (idx3 < idx2)
						continue;

					if (bFirst)
					{
						bFirst = false;
						fout << "\t\t\t//@{\n";
						fout << "\t\t\t//! Children accessors\n";
					}

					if (!pMR->GetChildMetaKey()->GetMetaEntity()->GetInstanceInterface().empty())
					{
						pMD = pMR->GetChildMetaKey()->GetMetaEntity()->GetMetaDatabase();
						ostrm << "\t\t\tvirtual " << pMR->GetChildMetaKey()->GetMetaEntity()->GetInstanceInterface() << "ResultSetPrx";
					}
					else
					{
						ostrm << "\t\t\tvirtual EntityPrx";
					}

					fout << std::setw(iAlign - 1) << ostrm.str() << " Get";
					ostrm.str("");

					fout << std::setw(30 - 3) << pMR->GetName() << "(Ice::Int iRecsPerPage, const Ice::Current& = Ice::Current());\n";
				}

#ifdef IMPLEMENT_ICE_COLUMN_INTERFACES
				if (!bFirst)
				{
					fout << "\t\t\t//@}\n\n";
					bFirst = true;
				}

				// Create attribute getters
				for (idx2 = 0; idx2 < pME->GetMetaColumns()->size(); idx2++)
				{
					pMC = pME->GetMetaColumn(idx2);

					if (bFirst)
					{
						bFirst = false;
						fout << "\t\t\t//@{\n";
						fout << "\t\t\t//! Attribute getters\n";
					}

					ostrm << "\t\t\tvirtual " << pMC->GetTypeAsIceCType();
					fout << std::setw(iAlign - 1) << ostrm.str() << " Get";
					ostrm.str("");
					fout << std::setw(30 - 3) << IcifyIdentifier(pMC->GetName()) << "(const Ice::Current& = Ice::Current());\n";
				}

				if (!bFirst)
				{
					fout << "\t\t\t//@}\n\n";
					bFirst = true;
				}

				// Create attribute setters
				for (idx2 = 0; idx2 < pME->GetMetaColumns()->size(); idx2++)
				{
					pMC = pME->GetMetaColumn(idx2);

					if (pMC->IsUpdatable())
					{
						if (bFirst)
						{
							bFirst = false;
							fout << "\t\t\t//@{\n";
							fout << "\t\t\t//! Attribute setters\n";
						}

						fout << std::setw(iAlign) << "\t\t\tvoid" << "Set";

						if (pMC->GetType() == MetaColumn::dbfString || pMC->GetType() == MetaColumn::dbfDate)
							fout << std::setw(30 - 3) << IcifyIdentifier(pMC->GetName()) << "(const " <<  pMC->GetTypeAsIceCType() << "& newVal, const Ice::Current& = Ice::Current());\n";
						else
							fout << std::setw(30 - 3) << IcifyIdentifier(pMC->GetName()) << "(" <<  pMC->GetTypeAsIceCType() << " newVal, const Ice::Current& = Ice::Current());\n";
					}
				}
#endif

				if (!bFirst)
				{
					fout << "\t\t\t//@}\n\n";
					bFirst = true;
				}

				// Build a member that can resolve the session and the member that this the interface we're generating here wraps
				fout << "\t\t\t//! The GetContext protected method is used to locate the underlying D3::" << pME->GetInstanceClassName() << " object\n";
				fout << "\t\t\t/*! Note that instances of this class are actually created on an \"as-needed\" basis and are transient.\n";
				fout << "\t\t\t\t\tIn other words, every single request from a client will result in the construction of an\n";
				fout << "\t\t\t\t\tinstance of this class, calling this method to determine the actual D3::" << pME->GetInstanceClassName() << " this represents,\n";
				fout << "\t\t\t\t\tcalling (and returning the result of) the relevant D3::" << pME->GetInstanceClassName() << " method and destroying this object.\n";
				fout << "\t\t\t*/\n";

				fout << std::setw(iAlign) << "\t\t\tvirtual void";
				fout << std::setw(30) << "GetContext" << "(D3::LockedSessionPtr & pSession, D3::DatabaseWorkspacePtr & pDBWS, D3::" << pME->GetInstanceClassName() << "Ptr & pEntity, const Ice::Identity & iceID);\n\n";

				// Build a static member that can build a proxy interface based on an instance that it wraps
				fout << "\t\t\t//! Constructs a " << pME->GetInstanceInterface() << " proxy object which belongs to the session specified and wraps the passed in D3::" << pME->GetInstanceClassName() << std::endl;
				fout << "\t\t\t/*! This member must be implemented in a source file that can see the declaration for the non abstract interface class\n";
				fout << "\t\t\t\t\t" << pME->GetInstanceInterface() << ". Use the macro APAL_DATABASE_PROXYFACTORY_IMPL to implement this member (see APALIce.h for info)\n";
				fout << "\t\t\t*/\n";

				ostrm << "\t\t\tstatic " << pME->GetInstanceInterface() << "Prx";
				fout << std::setw(iAlign) << ostrm.str();
				ostrm.str("");

				fout << std::setw(30) << "CreateProxy" << "(D3::LockedSessionPtr & pSession, D3::DatabaseWorkspacePtr pDBWS, D3::" << pME->GetInstanceClassName() << "Ptr pEntity, const Ice::Current& = Ice::Current());\n\n";

				fout << "\t};\n\n\n\n\n";
			}
		}

		// Implement specialised ResultSet interface C++ declarations
		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			pME = m_vectMetaEntity[idx1];

			if (!pME->GetInstanceInterface().empty())
			{
				fout << "\t//! Specialises ResultSet for " << pME->GetInstanceInterface() << " so that clients don't need to cast the collections objects to a custom type before calling custom methods\n";
				fout << "\tclass " << pME->GetInstanceInterface() << "ResultSetI : public virtual " << pME->GetInstanceInterface() << "ResultSet, public virtual ResultSetI\n";
				fout << "\t{\n";
				fout << "\t\tAPAL_ICE_INTERFACE_LOCATOR_DECL(" << pME->GetInstanceInterface() << "ResultSetI);\n\n";

				fout << "\t\tpublic:\n";

				fout << "\t\t\t//! Returns a sequence of " << pME->GetInstanceInterface() << " interfaces\n";
				ostrm << "\t\t\tvirtual " << Pluralize(pME->GetInstanceInterface());
				fout << std::setw(iAlign) << ostrm.str() << "Get";
				ostrm.str("");
				fout << std::setw(30 - 3) << Pluralize(pME->GetInstanceInterface()) << "(const Ice::Current& = Ice::Current());\n\n";


				fout << "\t\t\t//!	This method populates and Ice::Identity object such that it can be used to create a proxy object to ako ResultSetI\n";
				fout << std::setw(iAlign) << "\t\t\tstatic void";
				fout << std::setw(30) << "SetIceIdentity" << "(D3::LockedSessionPtr & pSession, D3::DatabaseWorkspacePtr pDBWS, D3::ResultSetPtr pResultSet, Ice::Identity & iceID);\n\n";


				fout << "\t\t\t//! This static member can build a proxy interface based on an instance that it wraps a D3::ResultSet containg interfaces of type " << pME->GetInstanceInterface() << std::endl;
				ostrm << "\t\t\tstatic " << pME->GetInstanceInterface() << "ResultSetPrx";
				fout << std::setw(iAlign) << ostrm.str();
				ostrm.str("");
				fout << std::setw(30) << "CreateProxy" << "(D3::LockedSessionPtr & pSession, D3::DatabaseWorkspacePtr pDBWS, D3::ResultSetPtr pResultSet, const Ice::Current& = Ice::Current());\n\n";

				fout << "\t};\n\n\n\n\n";
			}
		}

		fout << "} // End of namespace ID3\n\n";
		fout << "#endif /* _IF" << GetAlias() << "Base_H_ */\n";
		fout.close();

		return true;
	}





	bool MetaDatabase::CreateHTemplateInterface()
	{
		std::ofstream					fout;
		unsigned int					idx1;
		MetaEntityPtr					pME;
		std::string						strFileName;


		// Build file name
		//
		strFileName = "Generated/IF";
		strFileName += GetAlias();
		strFileName += ".h.tmpl";

		// Open the Header file
		//
		fout.open(strFileName.c_str(), std::ios::out | std::ios::trunc);

		if (fout.fail())
		{
			ReportError("MetaDatabase::CreateHTemplateInterface(): Failed to open/create ice interface file %s.", strFileName.c_str());
			return false;
		}

		// Choose left allignment for fields
		fout << std::setiosflags(std::ios::left);

		// Write the include blocker
		//
		fout << "#ifndef _IF" << GetAlias() << "_H_\n";
		fout << "#define _IF" << GetAlias() << "_H_\n\n";

		fout << "// MODULE: IF" << GetAlias() << " containes the C++ declarations for the Ice interface into " << GetAlias() << "\n\n";

		fout << "// WARNING: This file is only a generated TEMPLATE. It might help you to implement new interfaces in that you can copy\n";
		fout << "//          generated code. While this file is fully functional, IT IS HIGHLY UNLIKELY that you can remove \".tmpl\"\n";
		fout << "//          from the file name and and use it as is. More typical is that you use this file as a starting point and add\n";
		fout << "//          more funtionality to it. At a later stage, you might simply want to copy newly generated code from this file\n";
		fout << "//          and add it to the customised/enhanced version you have.\n\n";

		fout << "#include \"APALIce.h\"\n";
		fout << "#include \"IF" << GetAlias() << "Base.h\"\n";
		fout << "#include \"I_" << GetAlias() << ".h\"\n\n\n";

		fout << "namespace ID3\n";
		fout << "{\n";

		// ------------------------------------------------
		// Build the interface c++ declaration for each entity
		fout << "\t//! " << GetInstanceInterface() << "I implements a standard interface into " << GetProsaName() << " objects\n";
		fout << "\tclass " << GetInstanceInterface() << "I : public virtual " << GetInstanceInterface() << ", public virtual " << GetInstanceInterface() << "BaseI\n";
		fout << "\t{\n";
		fout << "\t\tAPAL_ICE_INTERFACE_LOCATOR_DECL(" << GetInstanceInterface() << "I);\n\n";

		fout << "\t\tpublic:\n";
		fout << "\t\t\t// todo: Add more interfaces here\n";
		fout << "\t};\n\n\n\n\n";

		// ------------------------------------------------
		// Build the interface c++ declaration for each entity
		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			pME = m_vectMetaEntity[idx1];

			if (!pME->GetInstanceInterface().empty())
			{
				fout << "\t//! " << pME->GetInstanceInterface() << "I implements a standard interface into " << pME->GetInstanceClassName() << " objects\n";
				fout << "\tclass " << pME->GetInstanceInterface() << "I : public virtual " << pME->GetInstanceInterface() << ", public virtual " << pME->GetInstanceInterface() << "BaseI\n";
				fout << "\t{\n";
				fout << "\t\tAPAL_ICE_INTERFACE_LOCATOR_DECL(" << pME->GetInstanceInterface() << "I);\n\n";

				fout << "\t\tpublic:\n";
				fout << "\t\t\t// todo: Add more interfaces here\n";
				fout << "\t};\n\n\n\n\n";
			}
		}

		fout << "} // End of namespace ID3\n\n";
		fout << "#endif /* _IF" << GetAlias() << "_H_ */\n";
		fout.close();

		return true;
	}





	bool MetaDatabase::CreateCPPBaseInterface()
	{
		std::ofstream					fout;
		unsigned int					idx1, idx2, idx3;
		MetaDatabasePtrMapItr	itr;
		MetaDatabasePtr				pMD;
		MetaEntityPtr					pME;
		MetaRelationPtr				pMR;
		std::string						strFileName;
#ifdef IMPLEMENT_ICE_COLUMN_INTERFACES
		MetaColumnPtr					pMC;
#endif


		// Build file name
		//
		strFileName = "Generated/IF";
		strFileName += GetAlias();
		strFileName += "Base.cpp";

		// Open the Header file
		//
		fout.open(strFileName.c_str(), std::ios::out | std::ios::trunc);

		if (fout.fail())
		{
			ReportError("MetaDatabase::CreateCPPBaseInterface(): Failed to open/create ice interface file %s.", strFileName.c_str());
			return false;
		}

		// Choose left allignment for fields
		fout << std::setiosflags(std::ios::left);

		// Write header
		fout << "// MODULE: IF" << GetAlias() << "Base source contains the generated C++ implementations for the generated base interface I_" << GetAlias() << "Base.ice\n\n";

		fout << "// WARNING: This file has been generated by D3. You should NEVER change this file.\n";
		fout << "//          Please refer to the documentation of MetaDatabase::CreateSourceCode()\n";
		fout << "//          which explains in detail how you can gegenerate this file.\n\n";

		fout << "#include \"types.h\"\n\n";

		fout << "#include \"IFAPAL.h\"\n\n";
		fout << "#include \"IF" << GetAlias() << "Base.h\"\n";

		// Include interface types
		if (this != GetMetaDictionary() && this->GetAlias() != "D3HSDB")
		{
			for ( itr =  MetaDatabase::GetMetaDatabases().begin();
						itr != MetaDatabase::GetMetaDatabases().end();
						itr++)
			{
				pMD = itr->second;

				if (pMD != GetMetaDictionary() && pMD->GetAlias() != "D3HSDB")
					fout << "#include \"IF" << pMD->GetAlias() << "Base.h\"\n";
			}
		}
		else
		{
			fout << "#include \"IF" << GetAlias() << "Base.h\"\n";
		}

		fout << std::endl;

		// Create includes for D3 types this interface wraps
		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			pME = m_vectMetaEntity[idx1];

			if (!pME->GetInstanceInterface().empty())
				fout << "#include <" << pME->GetInstanceClassName() << ".h>\n";
		}

		fout << std::endl;

		fout << "namespace ID3\n";
		fout << "{\n";

		// ------------------------------------------------
		// Build the base interface c++ implementation for this
		fout << "\t//===========================================================================\n";
		fout << "\t//\n";
		fout << "\t// " << GetInstanceInterface() << "BaseI implementation (ako DatabaseI interface)\n";
		fout << "\t//\n\n";

		// Create something like this for each MetaEntity belonging to this:
		/*
			LocationResultSetPrx APAL3DatabaseBaseI::Locations(::Ice::Int iRecsPerPage, const ::std::string& strFilter, const ::std::string& strOrder, const ::Ice::Current& iceCurrent)
			{
				D3::LockedSessionPtr							pSession;
				D3::DatabaseWorkspacePtr					pDBWS;
				D3::ResultSetPtr									pResultSet = NULL;
				D3::DatabasePtr										pDB;
				D3::MetaEntityPtr									pME;

				DatabaseI::GetContext(pSession, pDBWS, pDB, iceCurrent.id);

				if (pDB)
				{
					pME = pDB->GetMetaDatabase()->GetMetaEntity(D3::APAL3DB_AP3Location);
					pResultSet = D3::ResultSet::Create(pDB, pME, strFilter, strOrder, iRecsPerPage);
				}

				return LocationResultSetI::CreateProxy(pSession, pDBWS, pResultSet, iceCurrent);
			}
		*/
		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			pME = m_vectMetaEntity[idx1];

			if (!pME->GetInstanceInterface().empty())
			{
				fout << "\t" << pME->GetInstanceInterface() << "ResultSetPrx ";
				fout << GetInstanceInterface() << "BaseI::" << Pluralize(pME->GetInstanceInterface()) << "(Ice::Int iRecsPerPage, const std::string& strFilter, const std::string& strOrder, const Ice::Current& iceCurrent)\n";
			}
			else
			{
				fout << "\tResultSetPrx ";
				fout << GetInstanceInterface() << "BaseI::" << Pluralize(pME->GetName()) << "(Ice::Int iRecsPerPage, const std::string& strFilter, const std::string& strOrder, const Ice::Current& iceCurrent)\n";
			}

			fout << "\t{\n";
			fout << "\t\tD3::LockedSessionPtr               pSession;\n";
			fout << "\t\tD3::DatabaseWorkspacePtr           pDBWS;\n";
			fout << "\t\tD3::ResultSetPtr                   pResultSet = NULL;\n";
			fout << "\t\tD3::DatabasePtr                    pDB;\n";
			fout << "\t\tD3::MetaEntityPtr                  pME;\n\n";

			fout << "\t\tDatabaseI::GetContext(pSession, pDBWS, pDB, iceCurrent.id);\n\n";

			fout << "\t\tif (pDB)\n";
			fout << "\t\t{\n";
			fout << "\t\t\tpME = pDB->GetMetaDatabase()->GetMetaEntity(D3::" << GetAlias() << '_' << pME->GetName() << ");\n";
			fout << "\t\t\tpResultSet = D3::ResultSet::Create(pDB, pME, strFilter, strOrder, iRecsPerPage);\n";
			fout << "\t\t}\n\n";

			if (!pME->GetInstanceInterface().empty())
				fout << "\t\treturn " << pME->GetInstanceInterface() << "ResultSetI::CreateProxy(pSession, pDBWS, pResultSet, iceCurrent);\n";
			else
				fout << "\t\treturn ResultSetI::CreateProxy(pSession, pDBWS, pResultSet, iceCurrent);\n";

			fout << "\t}\n\n\n\n\n";
		}

		// ------------------------------------------------
		// Build the base interface c++ implementations each of this' entities

		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			pME = m_vectMetaEntity[idx1];

			if (!pME->GetInstanceInterface().empty())
			{
				fout << "\t//===========================================================================\n";
				fout << "\t//\n";
				fout << "\t// " << pME->GetInstanceInterface() << "BaseI implementation (this is ako EntityI)\n";
				fout << "\t//\n\n";

				// Create parent accessors which might look like this
				/*
          WarehousePrx WarehouseAreaBaseI::GetWarehouse(const ::Ice::Current& iceCurrent)
          {
            D3::LockedSessionPtr               pSession;
						D3::DatabaseWorkspacePtr					 pDBWS;
            D3::AP3WarehouseAreaPtr            pWarehouseArea = NULL;

            GetContext(pSession, pDBWS, pWarehouseArea, iceCurrent.id);

            return WarehouseBaseI::CreateProxy(pSession, pDBWS, pWarehouseArea ? pWarehouseArea->GetWarehouse() : NULL, iceCurrent);
          }
				*/
				for (idx2 = 0; idx2 < pME->GetParentMetaRelations()->size(); idx2++)
				{
					pMR = pME->GetParentMetaRelation(idx2);

					// Only generate generate this for the first child relation with this name
					//
					for (idx3 = 0; idx3 < idx2; idx3++)
					{
						if (pME->GetParentMetaRelation(idx3)->GetReverseName() == pMR->GetReverseName())
							break;
					}

					if (idx3 < idx2)
						continue;

					if (!pMR->GetParentMetaKey()->GetMetaEntity()->GetInstanceInterface().empty())
					{
						// The related parent has a specific interface class
						pMD = pMR->GetParentMetaKey()->GetMetaEntity()->GetMetaDatabase();
						fout << "\t" << pMR->GetParentMetaKey()->GetMetaEntity()->GetInstanceInterface() << "Prx ";
					}
					else
					{
						// The related parent has no specific interface class
						fout << "\tEntityPrx ";
					}

					fout << pME->GetInstanceInterface() << "BaseI::Get" << pMR->GetReverseName() << "(const Ice::Current& iceCurrent)\n";
					fout << "\t{\n";
					fout << "\t\tD3::LockedSessionPtr                                  pSession;\n";
					fout << "\t\tD3::DatabaseWorkspacePtr                              pDBWS;\n";
					fout << "\t\tD3::" << pME->GetInstanceClassName() << std::setw(50 - pME->GetInstanceClassName().size()) << "Ptr " << "pEntity = NULL;\n\n";

					fout << "\t\tGetContext(pSession, pDBWS, pEntity, iceCurrent.id);\n\n";

					if (!pMR->GetParentMetaKey()->GetMetaEntity()->GetInstanceInterface().empty())
					{
						// The related parent has a specific interface class
						pMD = pMR->GetParentMetaKey()->GetMetaEntity()->GetMetaDatabase();
						fout << "\t\treturn " << pMR->GetParentMetaKey()->GetMetaEntity()->GetInstanceInterface() << "Base";
					}
					else
					{
						// The related parent has no specific interface class
						fout << "\t\treturn Entity";
					}

					fout << "I::CreateProxy(pSession, pDBWS, pEntity ? pEntity->Get" << pMR->GetReverseName() << "() : NULL, iceCurrent);\n";
					fout << "\t}\n\n\n\n\n";
				}

				// Build children accessors which might look like this
				/*
					WarehouseAreaResultSetPrx WarehouseBaseI::GetWarehouseAreas(::Ice::Int iRecsPerPage, const ::Ice::Current& iceCurrent)
					{
						D3::LockedSessionPtr               pSession;
						D3::DatabaseWorkspacePtr					 pDBWS;
						D3::RelationPtr                    pRel;
						D3::ResultSetPtr                   pResultSet = NULL;
						D3::AP3WarehousePtr                pWarehouse = NULL;

						GetContext(pSession, pDBWS, pWarehouse, iceCurrent.id);

						if (pWarehouse)
						{
							pRel = pWarehouse->GetChildRelation(D3::APAL3DB_AP3Warehouse_CR_WarehouseAreas, pDBWS->GetDatabase("APAL3DB"));
							pResultSet = D3::ResultSet::Create(pDBWS->GetDatabase("APAL3DB"), pRel, iRecsPerPage);
						}

						return WarehouseAreaResultSetI::CreateProxy(pSession, pDBWS, pResultSet, iceCurrent);
					}
				*/
				for (idx2 = 0; idx2 < pME->GetChildMetaRelations()->size(); idx2++)
				{
					pMR = pME->GetChildMetaRelation(idx2);

					// Only generate generate this for the first child relation with this name
					//
					for (idx3 = 0; idx3 < idx2; idx3++)
					{
						if (pME->GetChildMetaRelation(idx3)->GetName() == pMR->GetName())
							break;
					}

					if (idx3 < idx2)
						continue;

					if (!pMR->GetChildMetaKey()->GetMetaEntity()->GetInstanceInterface().empty())
					{
						pMD = pMR->GetChildMetaKey()->GetMetaEntity()->GetMetaDatabase();
						fout << "\t" << pMR->GetChildMetaKey()->GetMetaEntity()->GetInstanceInterface() << "ResultSetPrx ";
					}
					else
					{
						fout << "\tResultSetPrx ";
					}

					fout << pME->GetInstanceInterface() << "BaseI::Get" << pMR->GetName() << "(Ice::Int iRecsPerPage, const Ice::Current& iceCurrent)\n";
					fout << "\t{\n";
					fout << "\t\tD3::LockedSessionPtr                                  pSession;\n";
					fout << "\t\tD3::DatabaseWorkspacePtr                              pDBWS;\n";
					fout << "\t\tD3::RelationPtr                                       pRel;\n";
					fout << "\t\tD3::ResultSetPtr                                      pResultSet = NULL;\n";
					fout << "\t\tD3::" << pME->GetInstanceClassName() << std::setw(50 - pME->GetInstanceClassName().size()) << "Ptr " << "pEntity = NULL;\n\n";

					fout << "\t\tGetContext(pSession, pDBWS, pEntity, iceCurrent.id);\n\n";

					fout << "\t\tif (pEntity)\n";
					fout << "\t\t{\n";
					fout << "\t\t\tpRel = pEntity->GetChildRelation(D3::" << GetAlias() << '_' << pME->GetName() << "_CR_" << pMR->GetUniqueName() << ", pDBWS->GetDatabase(\"" << GetAlias() << "\"));\n";
					fout << "\t\t\tpResultSet = D3::ResultSet::Create(pDBWS->GetDatabase(\"" << GetAlias() << "\"), pRel, iRecsPerPage);\n";
					fout << "\t\t}\n\n";

					if (!pMR->GetChildMetaKey()->GetMetaEntity()->GetInstanceInterface().empty())
					{
						pMD = pMR->GetChildMetaKey()->GetMetaEntity()->GetMetaDatabase();
						fout << "\t\treturn " << pMR->GetChildMetaKey()->GetMetaEntity()->GetInstanceInterface() << "ResultSetI::CreateProxy(pSession, pDBWS, pResultSet, iceCurrent);\n";
					}
					else
					{
						fout << "\t\treturn ResultSetI::CreateProxy(pSession, pDBWS, pResultSet, iceCurrent);\n";
					}

					fout << "\t}\n\n\n\n\n";
				}

#ifdef IMPLEMENT_ICE_COLUMN_INTERFACES
				// Build attribute getters. Here is a sample:
				/*
					::std::string WarehouseAreaBaseI::GetID(const Ice::Current& iceCurrent)
					{
						D3::LockedSessionPtr         pSession;
						D3::DatabaseWorkspacePtrPtr  pDBWS;
						D3::AP3WarehouseAreaPtr      pEntity = NULL;


						GetContext(pSession, pDBWS, pEntity, iceCurrent.id);

						if (!pEntity)
							throw Ice::ObjectNotExistException(__FILE__, __LINE__);

						return pEntity->GetID();
					}
				*/
				for (idx2 = 0; idx2 < pME->GetMetaColumns()->size(); idx2++)
				{
					pMC = pME->GetMetaColumn(idx2);

					fout << "\t" << pMC->GetTypeAsIceCType() << ' ' << pME->GetInstanceInterface() << "BaseI::Get" << IcifyIdentifier(pMC->GetName()) << "(const Ice::Current& iceCurrent)\n";
					fout << "\t{\n";
					fout << "\t\tD3::LockedSessionPtr                                  pSession;\n";
					fout << "\t\tD3::DatabaseWorkspacePtr                              pDBWS;\n";
					fout << "\t\tD3::" << pME->GetInstanceClassName() << std::setw(50 - pME->GetInstanceClassName().size()) << "Ptr " << "pEntity = NULL;\n\n";

					fout << "\t\tGetContext(pSession, pDBWS, pEntity, iceCurrent.id);\n\n";

					fout << "\t\tif (!pEntity)\n";
					fout << "\t\t\tthrow Ice::ObjectNotExistException(__FILE__, __LINE__);\n\n";

					if (pMC->GetType() == MetaColumn::dbfDate)
						fout << "\t\treturn pEntity->Get" << pMC->GetName() << "().AsString();\n";
					else if(pMC->GetType() == MetaColumn::dbfBlob)
						fout << "\t\treturn pEntity->Get" << pMC->GetName() << "Enc();\n";
					else
						fout << "\t\treturn pEntity->Get" << pMC->GetName() << "();\n";

					fout << "\t}\n\n\n\n\n";
				}

				// Build attribute setters. Here is a sample:
				/*
					void WarehouseAreaBaseI::SetID(const ::std::string& newVal, const Ice::Current& iceCurrent)
					{
						D3::LockedSessionPtr				pSession;
						D3::DatabaseWorkspacePtrPtr pDBWS;
						D3::AP3WarehouseAreaPtr			pEntity = NULL;


						GetContext(pSession, pDBWS, pEntity, iceCurrent.id);

						if (!pEntity)
							throw Ice::ObjectNotExistException(__FILE__, __LINE__);

						pEntity->SetID(newVal);
					}
				*/
				for (idx2 = 0; idx2 < pME->GetMetaColumns()->size(); idx2++)
				{
					pMC = pME->GetMetaColumn(idx2);

					if (pMC->IsUpdatable())
					{
						if (pMC->GetType() == MetaColumn::dbfString || pMC->GetType() == MetaColumn::dbfDate)
							fout << "\tvoid " << pME->GetInstanceInterface() << "BaseI::Set" << IcifyIdentifier(pMC->GetName()) << "(const " << pMC->GetTypeAsIceCType() << "& newVal, const Ice::Current& iceCurrent)\n";
						else
							fout << "\tvoid " << pME->GetInstanceInterface() << "BaseI::Set" << IcifyIdentifier(pMC->GetName()) << '(' << pMC->GetTypeAsIceCType() << " newVal, const Ice::Current& iceCurrent)\n";

						fout << "\t{\n";
						fout << "\t\tD3::LockedSessionPtr                                  pSession;\n";
						fout << "\t\tD3::DatabaseWorkspacePtr                              pDBWS;\n";
						fout << "\t\tD3::" << pME->GetInstanceClassName() << std::setw(50 - pME->GetInstanceClassName().size()) << "Ptr " << "pEntity = NULL;\n\n";

						fout << "\t\tGetContext(pSession, pDBWS, pEntity, iceCurrent.id);\n\n";

						fout << "\t\tif (!pEntity)\n";
						fout << "\t\t\tthrow Ice::ObjectNotExistException(__FILE__, __LINE__);\n\n";

						fout << "\t\tpEntity->Set" << pMC->GetName() << "(newVal);\n";
						fout << "\t}\n\n\n\n\n";
					}
				}
#endif

				// Build a member that can resolve the session and the member that the interface we're generating here wraps. Here is a sample:
				/*
					void WarehouseAreaBaseI::GetContext(D3::LockedSessionPtr & pSession, D3::DatabaseWorkspacePtr & pDBWS, D3::AP3WarehouseAreaPtr & pEntityOut, const Ice::Identity & iceID)
					{
						D3::EntityPtr			pEntity = NULL;

						EntityI::GetContext(pSession, pDBWS, pEntity, iceID);

						if (!pEntity || !pEntity->IsKindOf(D3::AP3WarehouseArea::ClassObject()))
							throw Ice::ObjectNotExistException(__FILE__, __LINE__);

						pEntityOut = (D3::AP3WarehouseAreaPtr) pEntity;
					}
				*/
				fout << "\tvoid " << pME->GetInstanceInterface() << "BaseI::GetContext(D3::LockedSessionPtr & pSession, D3::DatabaseWorkspacePtr & pDBWS, D3::" << pME->GetInstanceClassName() << "Ptr & pEntityOut, const Ice::Identity & iceID)\n";
				fout << "\t{\n";
				fout << "\t\tD3::EntityPtr                      pEntity = NULL;\n\n";

				fout << "\t\tEntityI::GetContext(pSession, pDBWS, pEntity, iceID);\n\n";

				fout << "\t\tif (!pEntity || !pEntity->IsKindOf(D3::" << pME->GetInstanceClassName() << "::ClassObject()))\n";
				fout << "\t\t\tthrow Ice::ObjectNotExistException(__FILE__, __LINE__);\n\n";

				fout << "\t\tpEntityOut = (D3::" << pME->GetInstanceClassName() << "Ptr) pEntity;\n";
				fout << "\t}\n\n\n\n\n";
			}
		}

		// Implement specialised ResultSet interface C++ declarations
		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			pME = m_vectMetaEntity[idx1];

			if (!pME->GetInstanceInterface().empty())
			{
				fout << "\t//===========================================================================\n";
				fout << "\t//\n";
				fout << "\t// " << pME->GetInstanceInterface() << "ResultSetI implementation\n";
				fout << "\t//\n\n";

				fout << "\tAPAL_ICE_INTERFACE_LOCATOR_IMPL(" << pME->GetInstanceInterface() << "ResultSetI, " << pME->GetInstanceInterface() << "RS);\n\n\n";

				// Implement the GetEntities method which might look like so:
				/*
					Locations LocationResultSetI::GetLocations(const ::Ice::Current& iceCurrent)
					{
						Locations									vectRslt;
						D3::LockedSessionPtr			pSession;
						D3::DatabaseWorkspacePtr	pDBWS;
						D3::ResultSet::iterator		itr;
						D3::EntityPtr							pEntity;


						ResultSetI::GetContext(pSession, pDBWS, pResultSet, iceCurrent.id);

						if (pResultSet)
						{
							for (	itr =  pResultSet->begin();
										itr != pResultSet->end();
										itr++)
							{
								pEntity = *itr;
								assert(pEntity->IsKindOf(D3::AP3Location::ClassObject()));
								vectRslt.push_back(LocationBaseI::CreateProxy(pSession, pDBWS, (D3::AP3LocationPtr) pEntity, iceCurrent));
							}
						}

						return vectRslt;
					}
				*/
				fout << "\t" << Pluralize(pME->GetInstanceInterface()) << " " << pME->GetInstanceInterface() << "ResultSetI::Get" << Pluralize(pME->GetInstanceInterface()) << "(const Ice::Current& iceCurrent)\n";
				fout << "\t{\n";
				fout << "\t\t" << std::setw(29) << Pluralize(pME->GetInstanceInterface()) << " vectRslt;\n";
				fout << "\t\tD3::LockedSessionPtr          pSession;\n";
				fout << "\t\tD3::DatabaseWorkspacePtr      pDBWS;\n";
				fout << "\t\tD3::ResultSetPtr              pResultSet;\n";
				fout << "\t\tD3::ResultSet::iterator       itr;\n";
				fout << "\t\tD3::EntityPtr                 pEntity;\n\n";

				fout << "\t\tResultSetI::GetContext(pSession, pDBWS, pResultSet, iceCurrent.id);\n\n";

				fout << "\t\tif (pResultSet)\n";
				fout << "\t\t{\n";
				fout << "\t\t\tfor ( itr =  pResultSet->begin();\n";
				fout << "\t\t\t      itr != pResultSet->end();\n";
				fout << "\t\t\t      itr++)\n";
				fout << "\t\t\t{\n";
				fout << "\t\t\t\tpEntity = *itr;\n";
				fout << "\t\t\t\tassert(pEntity->IsKindOf(D3::" << pME->GetInstanceClassName() << "::ClassObject()));\n";
				fout << "\t\t\t\tvectRslt.push_back(" << pME->GetInstanceInterface() << "BaseI::CreateProxy(pSession, pDBWS, (D3::" << pME->GetInstanceClassName() << "Ptr) pEntity, iceCurrent));\n";
				fout << "\t\t\t}\n";
				fout << "\t\t}\n\n";

				fout << "\t\treturn vectRslt;\n";
				fout << "\t}\n\n\n\n\n";

				// Build the Identity setterfor this. Here is a sample:
				/*
					static void LocationResultSetI::SetIceIdentity(D3::LockedSessionPtr & pSession, D3::DatabaseWorkspacePtr pDBWS, D3::ResultSetPtr pResultSet, Ice::Identity & iceID)
					{
						return ResultSetI::SetIceIdentity(pSession, pDBWS, pResultSet, iceID, Locator::Category);
					}
				*/
				fout << "\tvoid " << pME->GetInstanceInterface() << "ResultSetI::SetIceIdentity(D3::LockedSessionPtr & pSession, D3::DatabaseWorkspacePtr pDBWS, D3::ResultSetPtr pResultSet, Ice::Identity & iceID)\n";
				fout << "\t{\n";
				fout << "\t\tResultSetI::SetIceIdentity(pSession, pDBWS, pResultSet, iceID, Locator::Category);\n";
				fout << "\t}\n\n\n\n\n";

				// Build the Proxy generator for this. Here is a sample:
				/*
					LocationResultSetPrx LocationResultSetI::CreateProxy(D3::LockedSessionPtr & pSession, D3::DatabaseWorkspacePtr pDBWS, D3::ResultSetPtr pResultSet, const ::Ice::Current& iceCurrent)
					{
						LocationResultSetPrx					prxResultSet;
						Ice::Identity									iceResultSetID;

						if (pResultSet)
						{
							SetIceIdentity(pSession, pDBWS, pResultSet, iceResultSetID);
							prxResultSet = LocationResultSetPrx::uncheckedCast(iceCurrent.adapter->createProxy(iceResultSetID));
						}

						return prxResultSet;
					}
				*/
				fout << "\t" << pME->GetInstanceInterface() << "ResultSetPrx " << pME->GetInstanceInterface() << "ResultSetI::CreateProxy(D3::LockedSessionPtr & pSession, D3::DatabaseWorkspacePtr pDBWS, D3::ResultSetPtr pResultSet, const ::Ice::Current& iceCurrent)\n";
				fout << "\t{\n";
				fout << "\t\t" << std::setw(50) << (pME->GetInstanceInterface() + "ResultSetPrx ") << "prxResultSet;\n";
				fout << "\t\t" << std::setw(50) << "Ice::Identity" << "iceResultSetID;\n\n";

				fout << "\t\tif (pResultSet)\n";
				fout << "\t\t{\n";
				fout << "\t\t\tSetIceIdentity(pSession, pDBWS, pResultSet, iceResultSetID);\n";
				fout << "\t\t\tprxResultSet = " << pME->GetInstanceInterface() << "ResultSetPrx::uncheckedCast(iceCurrent.adapter->createProxy(iceResultSetID));\n";
				fout << "\t\t}\n";

				fout << "\t\treturn prxResultSet;\n";
				fout << "\t}\n\n\n\n\n";
			}
		}

		fout << "} // End of namespace ID3\n";
		fout.close();

		return true;
	}





	bool MetaDatabase::CreateCPPTemplateInterface()
	{
		std::ofstream					fout;
		unsigned int					idx1;
		MetaDatabasePtrMapItr	itr;
		MetaDatabasePtr				pMD;
		MetaEntityPtr					pME;
		std::string						strFileName;


		// Build file name
		//
		strFileName = "Generated/IF";
		strFileName += GetAlias();
		strFileName += ".cpp.tmpl";

		// Open the Header file
		//
		fout.open(strFileName.c_str(), std::ios::out | std::ios::trunc);

		if (fout.fail())
		{
			ReportError("MetaDatabase::CreateCPPTemplateInterface(): Failed to open/create ice interface file %s.", strFileName.c_str());
			return false;
		}

		// Choose left allignment for fields
		fout << std::setiosflags(std::ios::left);

		// Write header
		fout << "// MODULE: IF" << GetAlias() << " source contains the generated C++ implementations for the generated base interface I_" << GetAlias() << ".ice.tmpl\n\n";

		fout << "// WARNING: This file is only a generated TEMPLATE. It might help you to implement new interfaces in that you can copy\n";
		fout << "//          generated code. While this file is fully functional, IT IS HIGHLY UNLIKELY that you can remove \".tmpl\"\n";
		fout << "//          from the file name and and use it as is. More typical is that you use this file as a starting point and add\n";
		fout << "//          more funtionality to it. At a later stage, you might simply want to copy newly generated code from this file\n";
		fout << "//          and add it to the customised/enhanced version you have.\n\n";

		fout << "#include \"types.h\"\n\n";

		// Include interface types
		if (this != GetMetaDictionary() && this->GetAlias() != "D3HSDB")
		{
			for ( itr =  MetaDatabase::GetMetaDatabases().begin();
						itr != MetaDatabase::GetMetaDatabases().end();
						itr++)
			{
				pMD = itr->second;

				if (pMD != GetMetaDictionary() && pMD->GetAlias() != "D3HSDB")
					fout << "#include \"IF" << pMD->GetAlias() << ".h\"\n";
			}
		}
		else
		{
			fout << "#include \"IF" << GetAlias() << ".h\"\n";
		}

		fout << std::endl;

		// Create includes for D3 types this interface wraps
		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			pME = m_vectMetaEntity[idx1];

			if (!pME->GetInstanceInterface().empty())
				fout << "#include <" << pME->GetInstanceClassName() << ".h>\n";
		}

		fout << std::endl;

		fout << "namespace ID3\n";
		fout << "{\n\n";

		fout << "\t//===========================================================================\n";
		fout << "\t//\n";
		fout << "\t// " << GetInstanceInterface() << "I implementation\n";
		fout << "\t//\n\n";

		fout << "\tAPAL_ICE_INTERFACE_LOCATOR_IMPL(" << GetInstanceInterface() << "I, " << GetInstanceInterface() << ");\n";
		fout << "\tAPAL_DATABASE_PROXYFACTORY_IMPL(" << GetInstanceInterface() << "Base, " << GetInstanceInterface() << ", Database);\n\n\n\n\n";

		// Create the macro invovation for interface locator and proxy factory for each entity
		for (idx1 = 0; idx1 < m_vectMetaEntity.size(); idx1++)
		{
			pME = m_vectMetaEntity[idx1];

			if (!pME->GetInstanceInterface().empty())
			{
				fout << "\t//===========================================================================\n";
				fout << "\t//\n";
				fout << "\t// " << pME->GetInstanceInterface() << "I implementation\n";
				fout << "\t//\n\n";

				fout << "\tAPAL_ICE_INTERFACE_LOCATOR_IMPL(" << pME->GetInstanceInterface() << "I, " << pME->GetInstanceInterface() << ");\n";
				fout << "\tAPAL_ENTITY_PROXYFACTORY_IMPL(" << pME->GetInstanceInterface() << "Base, " << pME->GetInstanceInterface() << ", " << pME->GetInstanceClassName() << ");\n\n\n\n\n";
			}
		}

		fout << "} // End of namespace ID3\n";
		fout.close();

		return true;
	}





	std::string MetaDatabase::GetTargetRDBMSAsString()
	{
		switch (m_eTargetRDBMS)
		{
			case Oracle:
				return "Oracle";

			case SQLServer:
				return "SQLServer";
		}

		return "Unknown";
	}



	MetaEntityPtrList& MetaDatabase::GetDependencyOrderedMetaEntities()
	{
		if (m_listMEDependency.size() < m_vectMetaEntity.size())
		{
			MetaRelationPtrVectItr										itrPMR;
			MetaRelationPtr														pPMR;
			MetaEntityPtrVectItr											itrVectME;
			MetaEntityPtr															pME, pMEParent;
			bool																			bHasDependent;
			bool																			bSkipped;


			// Just in case this is called after a new meta entity was added
			m_listMEDependency.clear();

			// Now loop until we're done (bContinue will be false if we didn't find anything to process)
			while (m_listMEDependency.size() < m_vectMetaEntity.size())
			{
				bSkipped = false;

				for ( itrVectME =  m_vectMetaEntity.begin();
							itrVectME != m_vectMetaEntity.end();
							itrVectME++)
				{
					pME = *itrVectME;

					// got this already?
					if (find(m_listMEDependency.begin(), m_listMEDependency.end(), pME) != m_listMEDependency.end())
						continue;

					// Check if this has parent relations and if so, if all of the related parent entities have been processed already
					bHasDependent = false;

					for ( itrPMR =  pME->GetParentMetaRelations()->begin();
								itrPMR != pME->GetParentMetaRelations()->end();
								itrPMR++)
					{
						// Get the meta entity which is the parent in this relation
						pPMR = *itrPMR;
						pMEParent = pPMR->GetParentMetaKey()->GetMetaEntity();

						// If the parent entity is from another database ignore it
						if (pMEParent->GetMetaDatabase() != this)
							continue;

						// If this relations parent is this, we ignore the dependency
						if (pME == pMEParent)
							continue;

						// If the parent has already been processed, we ignore the dependency
						if (find(m_listMEDependency.begin(), m_listMEDependency.end(), pMEParent) != m_listMEDependency.end())
							continue;

						// If this parent has this directly or indirectly as a parent we have a cycle so ignore it as dependent
						if (pME->HasCyclicDependency(pMEParent))
							continue;

						bHasDependent = true;
						break;
					}

					if (bHasDependent)
					{
						bSkipped = true;
						continue;
					}

					m_listMEDependency.push_back(pME);

					// If we added one after skipping one, restart
					if (bSkipped)
						break;
				}
			}
		}


		return m_listMEDependency;
	}



	//! Debug aid: The method dumps this and all its objects to cout
	void MetaDatabase::Dump(int nIndentSize, bool bDeep)
	{
		unsigned int	idx;

		// Dump this
		//
		std::cout << std::setw(nIndentSize) << "";
		std::cout << "Dumping MetaDatabase " << m_strName.c_str() << std::endl;
		std::cout << std::setw(nIndentSize) << "";
		std::cout << "{" << std::endl;

		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Server            " << m_strServer << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "UserID            " << m_strUserID << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "UserPWD           " << m_strUserPWD << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Name              " << m_strName << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "DataFilePath      " << m_strDataFilePath << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "LogFilePath       " << m_strLogFilePath << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "InitialSize       " << m_iInitialSize << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "MaxSize           " << m_iMaxSize << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "GrowthSize        " << m_iGrowthSize << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "TargetRDBMS       " << GetTargetRDBMSAsString() << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "InstanceClass     " << m_pInstanceClass->Name() << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "InstanceInterface " << m_strInstanceInterface << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Alias             " << m_strAlias << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "ProsaName         " << m_strProsaName << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Description       " << m_strDescription << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Version           " << GetVersion() << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Flags             " << m_Flags.AsString() << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Permissions       " << m_Permissions.AsString() << std::endl;

		// Dump all related meta objects
		//
		if (bDeep)
		{
			std::cout << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "Entities:" << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "{" << std::endl;

			// Dump all MetaEntity objects
			//
			for (idx = 0; idx < m_vectMetaEntity.size(); idx++)
			{
				if (idx > 0)
					std::cout << std::endl;

				m_vectMetaEntity[idx]->Dump(nIndentSize+4, bDeep);
			}

			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "}" << std::endl;
		}

		std::cout << std::setw(nIndentSize) << "";
		std::cout << "}" << std::endl;
	}





	// Read meta data from the physical MetaDictionary store
	//
	void MetaDatabase::LoadMetaDictionary(MetaDatabaseDefinitionList & listMDDefs, bool bLoadHelp)
	{
		DatabaseWorkspace																dbWS;
		DatabasePtr																			pDB;
		MetaDatabasePtr																	pMetaDatabase;
		D3MetaDatabasePtr																pD3MDB;
		MetaDatabaseDefinitionListItr										itrMDDefs;
		MetaKeyPtr																			pSKMR;
		InstanceKeyPtrSetItr														itrKey;
		D3MetaRelationPtr																pD3MR;
		D3RolePtr																				pD3Role = NULL;
		D3Role::iterator																itrD3Roles;
		RolePtrMapItr																		itrRoles;
		D3UserPtr																				pD3User = NULL;
		D3User::iterator																itrD3Users;
		D3RoleUserPtr																		pD3RoleUser = NULL;
		D3RoleUser::iterator														itrD3RoleUsers;
		UserPtr																					pUser, pAdminUser = NULL;
		RolePtr																					pRole, pAdminRole = NULL;
		RoleUserPtr																			pRoleUser, pAdminRoleUser = NULL;
		bool																						bUpdateAdmin = false, bGrantAdmin = false;
		std::string																			strSysadminRole(D3_SYSADMIN_ROLE);
		std::string																			strSysadminUser(D3_SYSADMIN_USER);
		std::string																			strAdminRole(D3_ADMIN_ROLE);
		std::string																			strAdminUser(D3_ADMIN_USER);
		MetaEntity::Permissions													noRights;
		std::string																			strB64PWD;
		Data																						binPWD;


		if (!M_pDictionaryDatabase->m_bInitialised)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaDatabase::LoadMetaDictionary(): You must call MetaDatabase::Initialise() before calling this method");

		pDB = M_pDictionaryDatabase->CreateInstance(&dbWS);

		// Load all the details for the requested database
		D3MetaDatabase::MakeD3MetaDictionariesResident(pDB, listMDDefs);

		// For each MetaDatabaseDefinition, find the corresponding D3MetaDatabase object
		for ( itrMDDefs =  listMDDefs.begin();
					itrMDDefs != listMDDefs.end();
					itrMDDefs++)
		{
			// Find the matching D3::D3MetaDatabase object we loaded with MakeD3MetaDictionariesResident
			pD3MDB = D3MetaDatabase::LoadD3MetaDatabase(pDB, (*itrMDDefs).m_strAlias, (*itrMDDefs).m_iVersionMajor, (*itrMDDefs).m_iVersionMinor, (*itrMDDefs).m_iVersionRevision);

			char szBuffer[256];
			::snprintf(szBuffer, 256, " - Loading: %s [%d.%d.%d]", itrMDDefs->m_strAlias.c_str(), itrMDDefs->m_iVersionMajor, itrMDDefs->m_iVersionMinor, itrMDDefs->m_iVersionRevision);
			std::cout << D3Date().AsString() << szBuffer << std::endl;

			// Create matching D3::MetaDatabase object
			MetaDatabase::LoadMetaObject(pD3MDB, *itrMDDefs);
		}

		std::cout << D3Date().AsString() << " - Loading: Relations" << std::endl;

		// Iterate through all relation entity objects and create matching MetaRelation objects
		// We do this last because relations can be between entities of different databases. We
		// use the SK_D3MetaRelation key because it will sort relations in order of significance
		// from the parent entities perspective.
		//
		pSKMR = M_pDictionaryDatabase->GetMetaEntity(D3MDDB_D3MetaRelation)->GetMetaKey(D3MDDB_D3MetaRelation_SK_D3MetaRelation);

		for ( itrKey =  pSKMR->GetInstanceKeySet(pDB)->begin();
					itrKey != pSKMR->GetInstanceKeySet(pDB)->end();
					itrKey++)
		{
			pD3MR = (D3MetaRelationPtr) (*itrKey)->GetEntity();

			// Create matching MetaDatabase object
			//
			MetaRelation::LoadMetaObject(pD3MR);
		}

		std::cout << D3Date().AsString() << " - Verifying MetaDatabase objects" << std::endl;

		// Lets verify all the MetaDatabase objects that where created
		//
		for ( itrMDDefs =  listMDDefs.begin();
					itrMDDefs != listMDDefs.end();
					itrMDDefs++)
		{
			pMetaDatabase = MetaDatabase::GetMetaDatabase(*itrMDDefs);

			if (pMetaDatabase && !pMetaDatabase->VerifyMetaData())
				throw std::runtime_error("MetaDatabase::LoadMetaDictionary(): Verify Meta Data failed.");
		}

		// Load all Role objects
		for ( itrD3Roles =  D3Role::begin(pDB);
					itrD3Roles != D3Role::end(pDB);
					itrD3Roles++)
		{
			pD3Role = *itrD3Roles;
			pRole = new Role(pD3Role);

			if (!pAdminRole && pRole->GetName() == strAdminRole)
				pAdminRole = pRole;
		}

		std::cout << D3Date().AsString() << " - Loading: RBAC" << std::endl;

		// Load all User objects
		for ( itrD3Users =  D3User::begin(pDB);
					itrD3Users != D3User::end(pDB);
					itrD3Users++)
		{
			pD3User = *itrD3Users;
			pUser = new User(pD3User);

			if (!pAdminUser && pUser->GetName() == strAdminUser)
				pAdminUser = pUser;
		}

		// Load all RoleUser objects
		for ( itrD3RoleUsers =  D3RoleUser::begin(pDB);
					itrD3RoleUsers != D3RoleUser::end(pDB);
					itrD3RoleUsers++)
		{
			pD3RoleUser = *itrD3RoleUsers;
			pRoleUser = new RoleUser(pD3RoleUser);

			if (!pAdminRoleUser && pRoleUser->GetRole() == pAdminRole && pRoleUser->GetUser() == pAdminUser)
				pAdminRoleUser = pRoleUser;
		}

		// Create special, non-persistent sysadmin/sysadmin role, user and role user
		try
		{
			strB64PWD = "";
			strB64PWD = Settings::Singleton().SysadminPWD();

			// the sysadmin password is double encrypted, first MD5/Base64 and then XOR with a hard coded key of "TWO_crypt0"
			if (!strB64PWD.empty())
			{
				// sysadmin pwd
				/*
					1. generate the normal password string using d3_pwd.rb
					2. run: Tools\APAL_Password -p [pwd] -k "D3MDDB"
							[pwd] is result from 1)
					3. store the encrypted value in element APAL3\Security\SysadminPWD in the APALSvc xml file
				*/
				strB64PWD = APALUtil::decryptPassword(strB64PWD, "D3MDDB");
			}
		}
		catch(...)
		{
		}

		if (strB64PWD.empty())
			strB64PWD = D3_SYSADMIN_PWD;

		try
		{
			binPWD.base64_decode(strB64PWD);
		}
		catch(...)
		{
			binPWD.assign(NULL, 0);
		}

		if (binPWD.length() != 16)
		{
			ReportError("XML ini file contains incorrect value for APAL3\\Security\\SysadminPWD. The value must be a base64 encoded MD5 hash. Defaulting to hard coded password.");
			binPWD.base64_decode(D3_SYSADMIN_PWD);
		}

		new Role(strSysadminRole, true, Role::Features::Mask(ULONG_MAX), Role::IRSSettings::Mask(ULONG_MAX), Role::V3ParamSettings::Mask(ULONG_MAX), Role::T3ParamSettings::Mask(ULONG_MAX), Role::P3ParamSettings::Mask(ULONG_MAX));
		new User(strSysadminUser, binPWD, true);
		new RoleUser(D3::Role::M_sysadmin, D3::User::M_sysadmin);


		// If there is no strAdminRole Role, create one
		if (!pAdminRole)
		{
			// We need to create the strAdminRole role
			bUpdateAdmin = true;
			bGrantAdmin = true;

			pD3Role = D3Role::CreateD3Role(pDB);

			pD3Role->SetName(strAdminRole);
			pD3Role->SetEnabled(true);
			pD3Role->SetFeatures(LONG_MAX);
			pD3Role->SetIRSSettings(LONG_MAX);
			pD3Role->SetV3ParamSettings(LONG_MAX);
			pD3Role->SetT3ParamSettings(LONG_MAX);
			pD3Role->SetP3ParamSettings(LONG_MAX);

			// We need to create instances of all non-D3 meta databases
			D3MetaDatabase::iterator					itrD3MDDB;
			MetaDatabase::Permissions::Mask		rbac(MetaDatabase::Permissions::Create | MetaDatabase::Permissions::Read | MetaDatabase::Permissions::Write | MetaDatabase::Permissions::Delete);

			for (	itrD3MDDB =  D3MetaDatabase::begin(pDB);
						itrD3MDDB != D3MetaDatabase::end(pDB);
						itrD3MDDB++)
			{
				pD3MDB = *itrD3MDDB;

				if (pD3MDB->GetAlias() != "D3MDDB" && pD3MDB->GetAlias() != "D3HSDB")
				{
					D3DatabasePermissionPtr		pDBP = D3DatabasePermission::CreateD3DatabasePermission(pDB);
					pDBP->SetD3Role(pD3Role);
					pDBP->SetD3MetaDatabase(pD3MDB);
					pDBP->SetAccessRights(rbac.m_value);
				}
			}
		}

		// If there is no strAdminUser User, create one
		if (!pAdminUser)
		{
			// We need to create the strAdminUser role
			bUpdateAdmin = true;

			pD3User = D3User::CreateD3User(pDB);

			try
			{
				strB64PWD = "";
				strB64PWD = Settings::Singleton().AdminPWD();
			}
			catch(...)
			{
			}

			if (strB64PWD.empty())
				strB64PWD = D3_ADMIN_PWD;

			try
			{
				binPWD.base64_decode(strB64PWD);
			}
			catch(...)
			{
				binPWD.assign(NULL, 0);
			}

			if (binPWD.length() != 16)
			{
				ReportError("XML ini file contains incorrect value for APAL3\\Security\\AdminPWD. The value must be a base64 encoded MD5 hash. Defaulting to hard coded password.");
				binPWD.base64_decode(D3_ADMIN_PWD);
			}

			pD3User->SetName(strAdminUser);
			pD3User->SetPassword(binPWD);
			pD3User->SetEnabled(true);
		}

		// If there is no strAdminRole/strAdminUser RoleUser, create one
		if (!pAdminRoleUser)
		{
			// We need to create the strAdminRoleUser role
			bUpdateAdmin = true;

			pD3RoleUser = D3RoleUser::CreateD3RoleUser(pDB);

			pD3RoleUser->SetD3Role(pD3Role);
			pD3RoleUser->SetD3User(pD3User);
		}

		if (bUpdateAdmin)
		{
			try
			{
				G_bImportingRBAC = true;		// dispable RBAC update notifications
				pDB->BeginTransaction();
				if (pD3Role->IsNew())				pD3Role->Update(pDB, true);
				if (pD3User->IsNew())				pD3User->Update(pDB);
				if (pD3RoleUser->IsNew())		pD3RoleUser->Update(pDB);
				pDB->CommitTransaction();
				G_bImportingRBAC = false;		// enable RBAC update notifications
			}
			catch(...)
			{
				pDB->RollbackTransaction();
				G_bImportingRBAC = false;		// enable RBAC update notifications
				throw;
			}
		}

		// Assign special permissions for Administrator role
		if (!pAdminRole)
			pAdminRole = new Role(pD3Role);

		if (!pAdminUser)
			pAdminUser = new User(pD3User);

		if (!pAdminRoleUser)
			pAdminRoleUser = new RoleUser(pD3RoleUser);

		// Admin role can access D3MDDB, but has only read access to D3MetaDatabase, D3MetaEntity, D3MetaColumn
		MetaDatabasePtrMapItr			itrMDB;
		MetaDatabasePtrMap&				mapMDB(MetaDatabase::GetMetaDatabases());
		MetaDatabasePtr						pMDB;

		for (itrMDB = mapMDB.begin(); itrMDB != mapMDB.end(); itrMDB++)
		{
			pMDB = itrMDB->second;

			D3::Role::M_sysadmin->SetPermissions(pMDB, Permissions::Create | Permissions::Read | Permissions::Write | Permissions::Delete);

			// Administrator have permissions to D3MDDB and if we created the Admintrator role, we give access to all MetaDicts
			if (bGrantAdmin || pMDB == M_pDictionaryDatabase || pMDB->GetAlias() == "D3HSDB")
			{
				// read only access to help system
				if (pMDB->GetAlias() == "D3HSDB")
					pAdminRole->SetPermissions(pMDB, Permissions::Read);
				else
					pAdminRole->SetPermissions(pMDB, Permissions::Create | Permissions::Read | Permissions::Write | Permissions::Delete);
			}
		}

		// Make sure Administrator has read-only or no right to these
		pAdminRole->SetPermissions(M_pDictionaryDatabase->GetMetaEntity(D3MDDB_D3MetaDatabase),				MetaEntity::Permissions::Select);
		pAdminRole->SetPermissions(M_pDictionaryDatabase->GetMetaEntity(D3MDDB_D3MetaEntity),					MetaEntity::Permissions::Select);
		pAdminRole->SetPermissions(M_pDictionaryDatabase->GetMetaEntity(D3MDDB_D3MetaColumn),					MetaEntity::Permissions::Select);
		pAdminRole->SetPermissions(M_pDictionaryDatabase->GetMetaEntity(D3MDDB_D3MetaColumnChoice),		noRights);
		pAdminRole->SetPermissions(M_pDictionaryDatabase->GetMetaEntity(D3MDDB_D3MetaKey),						noRights);
		pAdminRole->SetPermissions(M_pDictionaryDatabase->GetMetaEntity(D3MDDB_D3MetaKeyColumn),			noRights);
		pAdminRole->SetPermissions(M_pDictionaryDatabase->GetMetaEntity(D3MDDB_D3MetaRelation),				noRights);

		// All roles can update the D3User
		for (	itrRoles =  Role::GetRoles().begin();
					itrRoles != Role::GetRoles().end();
					itrRoles++)
		{
			pRole = itrRoles->second;
			pRole->AddDefaultPermissions();
		}

		if (bLoadHelp)
		{
			// Finally, initialize HSTopics details for meta objects
			try
			{
				pDB = dbWS.GetDatabase("D3HSDB");
				if (pDB)
				{
					std::cout << D3Date().AsString() << " - Loading meta object related HSTopic info" << std::endl;
					pDB->SetMetaDatabaseHSTopics();
					pDB->SetMetaEntityHSTopics();
					pDB->SetMetaColumnHSTopics();
					pDB->SetMetaKeyHSTopics();
					pDB->SetMetaRelationHSTopics();
				}
			}
			catch (...)
			{
				GenericExceptionHandler("Loading meta object related HSTopic info failed. Assuming D3HSDB database does not yet exist.");
			}
		}

		std::cout << D3Date().AsString() << " - Completed successfully" << std::endl;
	}



	// Construct a MetaDatabase object from a D3MetaDatabase object (a D3MetaDatabase object
	// represents a record from the D3MetaDatabase table in the D3 MetaDictionary database)
	//
	/* static */
	void MetaDatabase::LoadMetaObject(D3MetaDatabasePtr pD3MDB, MetaDatabaseDefinition & aMDDef)
	{
		MetaDatabasePtr															pNewMD;
		D3MetaDatabase::D3MetaEntities::iterator		itrD3ME;
		D3MetaEntityPtr															pD3ME;

		assert(pD3MDB);

		pNewMD = new MetaDatabase(pD3MDB->GetID());

		pNewMD->m_eTargetRDBMS					= aMDDef.m_eTargetRDBMS;
		pNewMD->m_strDriver							= aMDDef.m_strDriver;
		pNewMD->m_strServer							= aMDDef.m_strServer;
		pNewMD->m_strUserID							= aMDDef.m_strUserID;
		pNewMD->m_strUserPWD						= aMDDef.m_strUserPWD;
		pNewMD->m_strName								= aMDDef.m_strName;
		pNewMD->m_strDataFilePath				= aMDDef.m_strDataFilePath;
		pNewMD->m_strLogFilePath				= aMDDef.m_strLogFilePath;
		pNewMD->m_iInitialSize					= aMDDef.m_iInitialSize;
		pNewMD->m_iMaxSize							= aMDDef.m_iMaxSize;
		pNewMD->m_iGrowthSize						= aMDDef.m_iGrowthSize;
		pNewMD->m_pInstanceClass				= aMDDef.m_pInstanceClass;

		if (!aMDDef.m_strTimeZone.empty())
			pNewMD->m_TimeZone	= D3TimeZone(aMDDef.m_strTimeZone);
		else
			pNewMD->m_TimeZone	= D3TimeZone::GetDefaultServerTimeZone();

		pNewMD->m_strAlias							= pD3MDB->GetAlias();
		pNewMD->m_strProsaName					= pD3MDB->GetProsaName();
		pNewMD->m_strDescription				= pD3MDB->GetDescription();
		pNewMD->m_uVersionMajor					= pD3MDB->GetVersionMajor();
		pNewMD->m_uVersionMinor					= pD3MDB->GetVersionMinor();
		pNewMD->m_uVersionRevision			= pD3MDB->GetVersionRevision();
		pNewMD->m_strInstanceInterface	= pD3MDB->GetInstanceInterface();

		// Set the construction flags
		//
		if (!pD3MDB->GetColumn(D3MDDB_D3MetaDatabase_Flags)->IsNull())
			pNewMD->m_Flags = MetaDatabase::Flags::Mask((unsigned long) pD3MDB->GetFlags());

		// Set the construction permissions
		//
		if (!pD3MDB->GetColumn(D3MDDB_D3MetaDatabase_AccessRights)->IsNull())
			pNewMD->m_Permissions = MetaDatabase::Permissions::Mask((unsigned long) pD3MDB->GetAccessRights());

		// Determine the instance class for this
		//
		if (!pNewMD->m_pInstanceClass)
			pNewMD->m_pInstanceClass = &(Class::Of("ODBCDatabase"));

		// Create MetaEntity objects
		//
		for ( itrD3ME =  pD3MDB->GetD3MetaEntities()->begin();
					itrD3ME != pD3MDB->GetD3MetaEntities()->end();
					itrD3ME++)
		{
			pD3ME = *itrD3ME;

			MetaEntity::LoadMetaObject(pNewMD, pD3ME);
		}
	}



	// This method discards all D3 objects in use by an application and
	// is typically called before the application shuts down.
	/* static */
	void MetaDatabase::UnInitialise()
	{
		// Delete all Users and Roles
		User::SaveAndDeleteRoleUsers();
		User::DeleteAll();
		Role::DeleteAll();

		// Delete all MetaDatabases except the MetaDictio9nary MetaDatabase
		while (M_mapMetaDatabase.size() > 1)
		{
			MetaDatabasePtr	pMDB = M_mapMetaDatabase.begin()->second;
			assert(!pMDB->IsMetaDictionary());		// The meta dictionary MetaDatabase must be the last item in the map (the highest key)
			delete pMDB;
		}

		delete M_pDatabaseWorkspace;
		delete M_pDictionaryDatabase;
	}



	// Initialise the MetaDictionary MetaDatabase
	//
	/* static */
	void MetaDatabase::Initialise(MetaDatabaseDefinition & MD)
	{
		MetaDatabasePtr				pMD;
		MetaEntityPtr					pME;
		MetaColumnPtr					pMC;
		MetaKeyPtr						pMK, pMKParent, pMKChild;
		MetaRelationPtr				pMR;


		// Create the meta dictionary MetaDatabase
		//
		M_pDictionaryDatabase = new MetaDatabase();
		M_pDatabaseWorkspace	= new DatabaseWorkspace();

		pMD = M_pDictionaryDatabase;

		// Initialise whatever can be initialised from the MetaDatabaseDefinition passed in
		//
		pMD->m_strDriver						= MD.m_strDriver;
		pMD->m_strServer						= MD.m_strServer;
		pMD->m_strUserID						= MD.m_strUserID;
		pMD->m_strUserPWD						= MD.m_strUserPWD;
		pMD->m_strName							= MD.m_strName;
		pMD->m_strAlias							= MD.m_strAlias;
		pMD->m_strDataFilePath			= MD.m_strDataFilePath;
		pMD->m_strLogFilePath				= MD.m_strLogFilePath;
		pMD->m_iInitialSize					= MD.m_iInitialSize;
		pMD->m_iMaxSize							= MD.m_iMaxSize;
		pMD->m_iGrowthSize					= MD.m_iGrowthSize;
		pMD->m_eTargetRDBMS					= MD.m_eTargetRDBMS;
		pMD->m_pInstanceClass				= MD.m_pInstanceClass;

		if (!MD.m_strTimeZone.empty())
			pMD->m_TimeZone	= D3TimeZone(MD.m_strTimeZone);
		else
			pMD->m_TimeZone	= D3TimeZone::GetDefaultServerTimeZone();

		// This's alias
		//
		pMD->m_strAlias							= "D3MDDB";
		pMD->m_strInstanceInterface = "D3MDDB";

		// Initialise the fixed Dictionary MetaDatabase object
		//
		pMD->m_strProsaName			= "D3 Meta Dictionary Database";
		pMD->m_strDescription		= "The D3 Meta Dictionary database describes the structure and data of other databases an application requires and is loaded by D3 at runtime.";
		pMD->m_uVersionMajor		= APAL_METADICTIONARY_VERSION_MAJOR;
		pMD->m_uVersionMinor		= APAL_METADICTIONARY_VERSION_MINOR;
		pMD->m_uVersionRevision	= APAL_METADICTIONARY_VERSION_REVISION;

		if (pMD->m_uVersionMajor		!= (unsigned) MD.m_iVersionMajor ||
				pMD->m_uVersionMinor		!= (unsigned) MD.m_iVersionMinor ||
				pMD->m_uVersionRevision	!= (unsigned) MD.m_iVersionRevision)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaDatabase::Initialise(): Current meta dictionary is V%i.%02i.%04i, but the initialisation file demanded V%i.%02i.%04i!", pMD->m_uVersionMajor,pMD->m_uVersionMinor, pMD->m_uVersionRevision, MD.m_iVersionMajor, MD.m_iVersionMinor, MD.m_iVersionRevision);

		// Set characteristics for MetaDictionary databases
		pMD->HasVersionInfo(true);
		pMD->HasObjectLinks(false);

		// Set permissions for MetaDictionary databases
		pMD->AllowCreate(true);
		pMD->AllowRead(true);
		pMD->AllowWrite(true);
		pMD->AllowDelete(true);

		// ===========================================================================================================
		// Create MetaDictionary System tables

		// Create D3MetaDatabase MetaEntity
		//
		pME = new MetaEntity(pMD, "D3MetaDatabase", "D3MetaDatabase");
		pME->m_strProsaName = "Meta Database";
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strInstanceInterface = "D3MetaDatabase";

		pMC = new MetaColumnLong		(pME, "ID",												MetaColumn::Flags::Mandatory | MetaColumn::Flags::AutoNum | MetaColumn::Flags::Hidden);
		pMC->m_strProsaName = "Unique ID";
		pMC->m_strDescription = "Unique identifier";
		pMC = new MetaColumnString	(pME, "Alias",								64, MetaColumn::Flags::Mandatory);
		pMC = new MetaColumnShort		(pME, "VersionMajor",							MetaColumn::Flags::Mandatory);
		pMC = new MetaColumnShort		(pME, "VersionMinor",							MetaColumn::Flags::Mandatory);
		pMC = new MetaColumnShort		(pME, "VersionRevision",					MetaColumn::Flags::Mandatory);
		pMC = new MetaColumnString	(pME, "InstanceInterface",		64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "ProsaName",						64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "Description",			 65535, MetaColumn::Flags::EncodedValue | MetaColumn::Flags::HiddenOnListView);
		pMC->m_strJSViewClass = "APALUI.widget.RTEObjectRowView";

		pMC = new MetaColumnLong		(pME, "Flags",										MetaColumn::Flags::Mandatory | MetaColumn::Flags::MultiChoice);
		pMC->m_strProsaName = "Characteristics";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,  "1", "Has Version Info")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,  "2", "Has Object Links")));

		pMC = new MetaColumnLong		(pME, "AccessRights",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::MultiChoice);
		pMC->m_strProsaName = "Access Rights";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,  "1", "Create")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,  "2", "Read")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 3.0F, MetaColumn::ColumnChoice( 3.0F,  "4", "Write")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 4.0F, MetaColumn::ColumnChoice( 4.0F,  "8", "Delete")));
		pMC->SetDefaultValue("15");

		pMK = new MetaKey(pME, "PK_D3MetaDatabase", MetaKey::Flags::PrimaryAutoNumKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("ID"));

		// These must be unique!!!
		//
		pMK = new MetaKey(pME, "SK_D3MetaDatabase", MetaKey::Flags::UniqueSecondaryKey | MetaKey::Flags::ConceptualKey);
		pMK->m_pInstanceClass = &(Class::Of("CK_D3MetaDatabase"));
		pMK->m_strProsaName = "By Alias/Version";
		pMK->AddMetaColumn(pME->GetMetaColumn("Alias"));
		pMK->AddMetaColumn(pME->GetMetaColumn("VersionMajor"));
		pMK->AddMetaColumn(pME->GetMetaColumn("VersionMinor"));
		pMK->AddMetaColumn(pME->GetMetaColumn("VersionRevision"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3MetaEntity MetaEntity
		//
		pME = new MetaEntity(pMD, "D3MetaEntity", "D3MetaEntity");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "Meta Entity";
		pME->m_strInstanceInterface = "D3MetaEntity";

		pMC = new MetaColumnLong		(pME, "ID",												MetaColumn::Flags::Mandatory | MetaColumn::Flags::AutoNum | MetaColumn::Flags::Hidden);
		pMC = new MetaColumnLong		(pME, "MetaDatabaseID",						MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;

		pMC = new MetaColumnFloat		(pME, "SequenceNo",								MetaColumn::Flags::Mandatory);
		pMC->m_strDescription = "This member determines the order of the entity in relation to other entities from the same database (in ascending order)";
		pMC->m_uFloatPrecision = 4;

		pMC = new MetaColumnString	(pME, "Name",									64, MetaColumn::Flags::Mandatory);
		pMC = new MetaColumnString	(pME, "MetaClass",						64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "InstanceClass",				64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "InstanceInterface",		64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSMetaClass",					64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSInstanceClass",			64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSViewClass",					64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSDetailViewClass",		64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "ProsaName",						64);
		pMC = new MetaColumnString	(pME, "Description",			 65535, MetaColumn::Flags::EncodedValue | MetaColumn::Flags::HiddenOnListView);
		pMC->m_strJSViewClass = "APALUI.widget.RTEObjectRowView";
		pMC = new MetaColumnString	(pME, "Alias",								64, MetaColumn::Flags::HiddenOnListView);

		pMC = new MetaColumnLong		(pME, "Flags",								MetaColumn::Flags::Mandatory | MetaColumn::Flags::MultiChoice);
		pMC->m_strProsaName = "Characteristics";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,   "8", "Hidden")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,  "16", "Cached")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 3.0F, MetaColumn::ColumnChoice( 3.0F,  "32", "Show conceptual key in detail screen title and along with arrow in list views")));

		pMC = new MetaColumnLong		(pME, "AccessRights",					MetaColumn::Flags::Mandatory | MetaColumn::Flags::MultiChoice);
		pMC->m_strProsaName = "Access Rights";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,  "1", "Insert")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,  "2", "Select")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 3.0F, MetaColumn::ColumnChoice( 3.0F,  "4", "Update")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 4.0F, MetaColumn::ColumnChoice( 4.0F,  "8", "Delete")));
		pMC->SetDefaultValue("15");

		pMK = new MetaKey(pME, "PK_D3MetaEntity", MetaKey::Flags::PrimaryAutoNumKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("ID"));

		// These must be unique!!!
		pMK = new MetaKey(pME, "SK_D3MetaEntity", MetaKey::Flags::UniqueSecondaryKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaDatabaseID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("Name"));

		pMK = new MetaKey(pME, "CK_D3MetaEntity", MetaKey::Flags::ConceptualKey);
		pMK->m_pInstanceClass = &(Class::Of("CK_D3MetaEntity"));
		pMK->m_strProsaName = "By Name";
		pMK->AddMetaColumn(pME->GetMetaColumn("Name"));

		pMK = new MetaKey(pME, "FK_D3MetaEntity_MetaDatabaseID", MetaKey::Flags::MandatoryForeignKey);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaDatabaseID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("SequenceNo"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3MetaColumn MetaEntity
		//
		pME = new MetaEntity(pMD, "D3MetaColumn", "D3MetaColumn");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "Meta Column";
		pME->m_strInstanceInterface = "D3MetaColumn";

		pMC = new MetaColumnLong		(pME, "ID",												MetaColumn::Flags::Mandatory | MetaColumn::Flags::AutoNum | MetaColumn::Flags::Hidden);
		pMC = new MetaColumnLong		(pME, "MetaEntityID",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnFloat		(pME, "SequenceNo",								MetaColumn::Flags::Mandatory);
		pMC->m_strDescription = "This member determines the order of the column in relation to other columns from the same entity (in ascending order)";
		pMC->m_uFloatPrecision = 4;
		pMC = new MetaColumnString	(pME, "Name",									64,	MetaColumn::Flags::Mandatory);
		pMC = new MetaColumnString	(pME, "ProsaName",						64);
		pMC->m_strDescription = "The user frindly name that is reflected to the user";
		pMC = new MetaColumnString	(pME, "Description",			 65535, MetaColumn::Flags::EncodedValue | MetaColumn::Flags::HiddenOnListView);
		pMC->m_strJSViewClass = "APALUI.widget.RTEObjectRowView";

		pMC = new MetaColumnChar		(pME, "Type",											MetaColumn::Flags::Mandatory | MetaColumn::Flags::SingleChoice);
		pMC->m_strProsaName = "Column Type";
		pMC->m_strDescription = "One of String, Char, Short, Bool, Int, Long, Float, Date or Blob";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,  "1", "String")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,  "2", "Char")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 3.0F, MetaColumn::ColumnChoice( 3.0F,  "3", "Short")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 4.0F, MetaColumn::ColumnChoice( 4.0F,  "4", "Bool")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 5.0F, MetaColumn::ColumnChoice( 5.0F,  "5", "Int")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 6.0F, MetaColumn::ColumnChoice( 6.0F,  "6", "Long")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 7.0F, MetaColumn::ColumnChoice( 7.0F,  "7", "Float")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 8.0F, MetaColumn::ColumnChoice( 8.0F,  "8", "Date")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 9.0F, MetaColumn::ColumnChoice( 9.0F,  "9", "Blob")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(10.0F, MetaColumn::ColumnChoice(10.0F, "10", "Binary")));
		pMC->SetDefaultValue("1");

		pMC = new MetaColumnChar		(pME, "Unit",											MetaColumn::Flags::SingleChoice);
		pMC->m_strProsaName = "Unit Type";
		pMC->m_strDescription = "Type of unit this column contains (only relevant if type of column is Int, Long or Float)";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,  "0", "generic")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,  "1", "inch")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 3.0F, MetaColumn::ColumnChoice( 3.0F,  "2", "foot")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 4.0F, MetaColumn::ColumnChoice( 4.0F,  "3", "yard")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 5.0F, MetaColumn::ColumnChoice( 5.0F,  "4", "centimeter")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 6.0F, MetaColumn::ColumnChoice( 6.0F,  "5", "meter")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 7.0F, MetaColumn::ColumnChoice( 7.0F,  "6", "pound")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 8.0F, MetaColumn::ColumnChoice( 8.0F,  "7", "gram")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 9.0F, MetaColumn::ColumnChoice( 9.0F,  "8", "kilogram")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(10.0F, MetaColumn::ColumnChoice(10.0F,  "9", "celsius")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(11.0F, MetaColumn::ColumnChoice(11.0F,  "10", "farenheit")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(12.0F, MetaColumn::ColumnChoice(12.0F,  "11", "percent")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(13.0F, MetaColumn::ColumnChoice(13.0F,  "12", "millisecond")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(14.0F, MetaColumn::ColumnChoice(14.0F,  "13", "second")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(15.0F, MetaColumn::ColumnChoice(15.0F,  "14", "minute")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(16.0F, MetaColumn::ColumnChoice(16.0F,  "15", "hour")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(17.0F, MetaColumn::ColumnChoice(17.0F,  "16", "day")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(18.0F, MetaColumn::ColumnChoice(18.0F,  "17", "week")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(19.0F, MetaColumn::ColumnChoice(19.0F,  "18", "month")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(20.0F, MetaColumn::ColumnChoice(20.0F,  "19", "year")));
		pMC->SetDefaultValue("0");

		pMC = new MetaColumnShort		(pME, "FloatPrecision",						MetaColumn::Flags::HiddenOnListView);
		pMC->m_strDescription = "The number if digits after the decimal point to display (only relevant if type of column is Float)";

		pMC = new MetaColumnInt			(pME, "MaxLength",								MetaColumn::Flags::Mandatory | MetaColumn::Flags::HiddenOnListView);
		pMC->m_strDescription = "The maximum number if digits this string can assume (only of relevance if this is a String)";

		pMC = new MetaColumnLong		(pME, "Flags",										MetaColumn::Flags::Mandatory | MetaColumn::Flags::MultiChoice);
		pMC->m_strProsaName = "Characteristics";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,    "4", "Mandatory")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,    "8", "AutoNum")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 3.0F, MetaColumn::ColumnChoice( 3.0F,   "16", "Derived")));

		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 4.0F, MetaColumn::ColumnChoice( 4.0F,   "64", "Single Choice")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 5.0F, MetaColumn::ColumnChoice( 5.0F,  "128", "Multi Choice")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 6.0F, MetaColumn::ColumnChoice( 6.0F,  "256", "Lazy Fetch")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 6.5F, MetaColumn::ColumnChoice( 6.5F,   "32", "Password")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 7.0F, MetaColumn::ColumnChoice( 7.0F, "1024", "Encoded Value")));

		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 8.0F, MetaColumn::ColumnChoice( 8.0F,    "2", "Hide from entity detail views")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 9.0F, MetaColumn::ColumnChoice( 9.0F,  "512", "Hide from entity list views")));

		pMC = new MetaColumnLong		(pME, "AccessRights",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::MultiChoice);
		pMC->m_strProsaName = "Access Rights";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,    "1", "Read")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,    "2", "Write")));
		pMC->SetDefaultValue("3");

		pMC = new MetaColumnString	(pME, "MetaClass",								64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "InstanceClass",						64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "InstanceInterface",				64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSMetaClass",							64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSInstanceClass",					64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSViewClass",							64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "DefaultValue",							64, MetaColumn::Flags::HiddenOnListView);

		pMK = new MetaKey(pME, "PK_D3MetaColumn", MetaKey::Flags::PrimaryAutoNumKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("ID"));

		// These must be unique!!!
		pMK = new MetaKey(pME, "SK_D3MetaColumn", MetaKey::Flags::UniqueSecondaryKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaEntityID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("Name"));

		pMK = new MetaKey(pME, "CK_D3MetaColumn", MetaKey::Flags::ConceptualKey);
		pMK->m_pInstanceClass = &(Class::Of("CK_D3MetaColumn"));
		pMK->m_strProsaName = "By Name";
		pMK->AddMetaColumn(pME->GetMetaColumn("Name"));

		pMK = new MetaKey(pME, "FK_D3MetaColumn_MetaEntityID", MetaKey::Flags::MandatoryForeignKey);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaEntityID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("SequenceNo"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3MetaColumnChoice MetaEntity
		//
		pME = new MetaEntity(pMD, "D3MetaColumnChoice", "D3MetaColumnChoice");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "Meta Column Choice";
		pME->m_strInstanceInterface = "D3MetaColumnChoice";

		pMC = new MetaColumnLong		(pME, "MetaColumnID",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnFloat		(pME, "SequenceNo",								MetaColumn::Flags::Mandatory);
		pMC = new MetaColumnString	(pME, "ChoiceVal",						32, MetaColumn::Flags::Mandatory);
		pMC = new MetaColumnString	(pME, "ChoiceDesc",					 256, MetaColumn::Flags::Mandatory);

		pMK = new MetaKey(pME, "PK_D3MetaColumnChoice", MetaKey::Flags::PrimaryKey | MetaKey::Flags::Foreign | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaColumnID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("SequenceNo"));

		pMK = new MetaKey(pME, "SK_D3MetaColumnChoice_Val", MetaKey::Flags::UniqueSecondaryKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaColumnID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("ChoiceVal"));

		pMK = new MetaKey(pME, "SK_D3MetaColumnChoice_Desc", MetaKey::Flags::UniqueSecondaryKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaColumnID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("ChoiceDesc"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3MetaKey MetaEntity
		//
		pME = new MetaEntity(pMD, "D3MetaKey", "D3MetaKey");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "Meta Key";
		pME->m_strInstanceInterface = "D3MetaKey";

		pMC = new MetaColumnLong		(pME, "ID",												MetaColumn::Flags::Mandatory | MetaColumn::Flags::AutoNum | MetaColumn::Flags::Hidden);
		pMC = new MetaColumnLong		(pME, "MetaEntityID",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnFloat		(pME, "SequenceNo",								MetaColumn::Flags::Mandatory);
		pMC->m_strDescription = "This member determines the order of the key in relation to other keys from the same entity (in ascending order)";
		pMC->m_uFloatPrecision = 4;

		pMC = new MetaColumnString	(pME, "Name",									64, MetaColumn::Flags::Mandatory);
		pMC = new MetaColumnString	(pME, "ProsaName",						64);
		pMC->m_strDescription = "The user friendly name that can be shown to the user";
		pMC = new MetaColumnString	(pME, "Description",			 65535,	MetaColumn::Flags::EncodedValue | MetaColumn::Flags::HiddenOnListView);
		pMC->m_strJSViewClass = "APALUI.widget.RTEObjectRowView";
		pMC->m_strDescription = "Help text explaining to the user what the key yields";
		pMC = new MetaColumnString	(pME, "MetaClass",						64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "InstanceClass",				64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSMetaClass",					64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSInstanceClass",			64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSViewClass",					64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "InstanceInterface",		64, MetaColumn::Flags::HiddenOnListView);

		pMC = new MetaColumnLong		(pME, "Flags",										MetaColumn::Flags::Mandatory | MetaColumn::Flags::MultiChoice);
		pMC->m_strProsaName = "Characteristics";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,  "1", "Mandatory")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,  "2", "Unique")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 3.0F, MetaColumn::ColumnChoice( 3.0F,  "4", "AutoNum")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 4.0F, MetaColumn::ColumnChoice( 4.0F,  "8", "Disable Quick Access")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 5.0F, MetaColumn::ColumnChoice( 5.0F,"256", "Use this key as default tab in Quick Access")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 6.0F, MetaColumn::ColumnChoice( 6.0F,"512", "Autocomplete this quick access dialog when first shown")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 7.0F, MetaColumn::ColumnChoice( 7.0F, "16", "Primary")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 8.0F, MetaColumn::ColumnChoice( 8.0F, "32", "Secondary")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 9.0F, MetaColumn::ColumnChoice( 9.0F, "64", "Foreign")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(10.0F, MetaColumn::ColumnChoice(10.0F,"128", "Conceptual")));
		pMC->SetDefaultValue("0");

		pMK = new MetaKey(pME, "PK_D3MetaKey", MetaKey::Flags::PrimaryAutoNumKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("ID"));

		pMK = new MetaKey(pME, "CK_D3MetaKey", MetaKey::Flags::ConceptualKey);
		pMK->m_pInstanceClass = &(Class::Of("CK_D3MetaKey"));
		pMK->m_strProsaName = "By Name";
		pMK->AddMetaColumn(pME->GetMetaColumn("Name"));

		pMK = new MetaKey(pME, "FK_D3MetaKey_MetaEntityID", MetaKey::Flags::MandatoryForeignKey);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaEntityID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("SequenceNo"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3MetaKeyColumn MetaEntity
		//
		pME = new MetaEntity(pMD, "D3MetaKeyColumn", "D3MetaKeyColumn");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "Meta Key Column";
		pME->m_strInstanceInterface = "D3MetaKeyColumn";

		pMC = new MetaColumnLong		(pME, "MetaKeyID",								MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnFloat		(pME, "SequenceNo",								MetaColumn::Flags::Mandatory);
		pMC->m_strDescription = "This member determines the order of the key column in relation to other key columns from the same key (in ascending order)";
		pMC->m_uFloatPrecision = 4;

		pMC = new MetaColumnLong		(pME, "MetaColumnID",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);

		pMK = new MetaKey(pME, "PK_D3MetaKeyColumn", MetaKey::Flags::PrimaryKey | MetaKey::Flags::Foreign | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaKeyID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("SequenceNo"));
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaColumnID"));

		pMK = new MetaKey(pME, "FK_D3MetaKeyColumn_MetaColumnID", MetaKey::Flags::MandatoryForeignKey);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaColumnID"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3MetaRelation MetaEntity
		//
		pME = new MetaEntity(pMD, "D3MetaRelation", "D3MetaRelation");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "Meta Relation";
		pME->m_strInstanceInterface = "D3MetaRelation";

		pMC = new MetaColumnLong		(pME, "ID",												MetaColumn::Flags::Mandatory | MetaColumn::Flags::AutoNum | MetaColumn::Flags::Hidden);
		pMC = new MetaColumnLong		(pME, "ParentKeyID",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnLong		(pME, "ChildKeyID",								MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnLong		(pME, "ParentEntityID",						MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnFloat		(pME, "SequenceNo",								MetaColumn::Flags::Mandatory);
		pMC->m_strDescription = "This member determines the order of the child relationship from the perspective of the parent)";
		pMC->m_uFloatPrecision = 4;
		pMC = new MetaColumnString	(pME, "Name",									64, MetaColumn::Flags::Mandatory);
		pMC = new MetaColumnString	(pME, "ProsaName",						64);
		pMC->m_strDescription = "The user friendly name that can be shown to the user";
		pMC = new MetaColumnString	(pME, "Description",			 65535,	MetaColumn::Flags::EncodedValue | MetaColumn::Flags::HiddenOnListView);
		pMC->m_strJSViewClass = "APALUI.widget.RTEObjectRowView";
		pMC->m_strDescription = "Help text explaining to the user what this is for";
		pMC = new MetaColumnString	(pME, "ReverseName",					64,	MetaColumn::Flags::Mandatory);
		pMC = new MetaColumnString	(pME, "ReverseProsaName",			64);
		pMC->m_strDescription = "The user friendly name that can be shown to the user";
		pMC = new MetaColumnString	(pME, "ReverseDescription",65535, MetaColumn::Flags::EncodedValue | MetaColumn::Flags::HiddenOnListView);
		pMC->m_strJSViewClass = "APALUI.widget.RTEObjectRowView";
		pMC->m_strDescription = "Help text explaining to the user what this is for";
		pMC = new MetaColumnString	(pME, "MetaClass",						64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "InstanceClass",				64,	MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "InstanceInterface",		64,	MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSMetaClass",					64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSInstanceClass",			64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSParentViewClass",		64, MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "JSChildViewClass",			64, MetaColumn::Flags::HiddenOnListView);

		pMC = new MetaColumnLong		(pME, "Flags",										MetaColumn::Flags::Mandatory | MetaColumn::Flags::MultiChoice);
		pMC->m_strProsaName = "Characteristics";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,      "1", "Cascade Delete")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,      "2", "Cascade Update References")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 3.0F, MetaColumn::ColumnChoice( 3.0F,      "4", "Prevent Delete")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 4.0F, MetaColumn::ColumnChoice( 4.0F,      "8", "Switched by Parent Column")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 5.0F, MetaColumn::ColumnChoice( 5.0F,     "16", "Switched by Child Column")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 6.0F, MetaColumn::ColumnChoice( 6.0F,     "32", "Hide Parent")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 7.0F, MetaColumn::ColumnChoice( 7.0F,     "64", "Hide Children")));
		pMC->SetDefaultValue("257");	// Cascade Delete and Can Add/Remove Children

		pMC = new MetaColumnLong		(pME, "SwitchColumnID",						MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnString	(pME, "SwitchColumnValue",	 256, MetaColumn::Flags::HiddenOnListView);

		pMK = new MetaKey(pME, "PK_D3MetaRelation",									MetaKey::Flags::PrimaryAutoNumKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("ID"));

		pMK = new MetaKey(pME, "SK_D3MetaRelation",									MetaKey::Flags::UniqueSecondaryKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("ParentEntityID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("SequenceNo"));

		pMK = new MetaKey(pME, "CK_D3MetaRelation",									MetaKey::Flags::ConceptualKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->m_pInstanceClass = &(Class::Of("CK_D3MetaRelation"));
		pMK->AddMetaColumn(pME->GetMetaColumn("Name"));

		pMK = new MetaKey(pME, "FK_D3MetaRelation_ParentKeyID",			MetaKey::Flags::MandatoryForeignKey | MetaKey::Flags::Unique);
		pMK->AddMetaColumn(pME->GetMetaColumn("ParentKeyID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("ChildKeyID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("Name"));

		pMK = new MetaKey(pME, "FK_D3MetaRelation_ChildKeyID",			MetaKey::Flags::MandatoryForeignKey);
		pMK->AddMetaColumn(pME->GetMetaColumn("ChildKeyID"));

		pMK = new MetaKey(pME, "FK_D3MetaRelation_SwitchColumnID",	MetaKey::Flags::OptionalForeignKey);
		pMK->AddMetaColumn(pME->GetMetaColumn("SwitchColumnID"));

		pME->On_AfterConstructingMetaEntity();

		// ===========================================================================================================
		// Create RBAC System tables

		// Create D3Role MetaEntity
		//
		pME = new MetaEntity(pMD, "D3Role", "D3Role");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "Role";
		pME->m_strInstanceInterface = "D3Role";

		pMC = new MetaColumnLong		(pME, "ID",												MetaColumn::Flags::Mandatory | MetaColumn::Flags::AutoNum | MetaColumn::Flags::Hidden);
		pMC->m_strProsaName = "Unique ID";
		pMC->m_strDescription = "Unique identifier";
		pMC = new MetaColumnString	(pME, "Name",									64, MetaColumn::Flags::Mandatory);
		pMC->AllowWrite(false);
		pMC = new MetaColumnBool		(pME, "Enabled",									MetaColumn::Flags::Mandatory);

		pMC = new MetaColumnLong		(pME, "Features",									MetaColumn::Flags::MultiChoice | MetaColumn::Flags::HiddenOnListView);
		pMC->m_strProsaName = "Features";
		pMC->m_strDescription = "Specifies which parts of the application a Role is allowed to access.";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
 		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(1.0F, MetaColumn::ColumnChoice( 1.0F,     "1", "Run P3 Algorithm")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(2.0F, MetaColumn::ColumnChoice( 2.0F,     "2", "Run T3 Algorithm")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(3.0F, MetaColumn::ColumnChoice( 3.0F,     "4", "Run V3 Algorithm")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(4.0F, MetaColumn::ColumnChoice( 4.0F,     "8", "Allow WarehouseMap viewing with applet")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(5.0F, MetaColumn::ColumnChoice( 5.0F,    "16", "Allow WarehouseMap updating with applet")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(8.0F, MetaColumn::ColumnChoice( 8.0F,   "128", "Allow Resolve Issues")));

#if defined (USING_AVLB)
		pMC = new MetaColumnLong		(pME, "IRSSettings",							MetaColumn::Flags::MultiChoice | MetaColumn::Flags::HiddenOnListView);
#else
		pMC = new MetaColumnLong		(pME, "IRSSettings",							MetaColumn::Flags::MultiChoice | MetaColumn::Flags::HiddenOnListView  | MetaColumn::Flags::HiddenOnDetailView);
#endif
		pMC->m_strProsaName = "IRS Settings";
		pMC->m_strDescription = "Specifies which features of IRS are available to members of a Role.";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
 		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,     "1", "Undo previous solutions")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,     "2", "Pull-Forward Smoothing")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 3.0F, MetaColumn::ColumnChoice( 3.0F,     "4", "Push-Back Smoothing")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 4.0F, MetaColumn::ColumnChoice( 4.0F,     "8", "Move order item to another Ship-From Shipment")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 5.0F, MetaColumn::ColumnChoice( 5.0F,    "16", "Move order item to another Ship-To Shipment")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 6.0F, MetaColumn::ColumnChoice( 6.0F,    "32", "Move order item to another Transportation Mode Shipment")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 7.0F, MetaColumn::ColumnChoice( 7.0F,   "512", "Allow Exclude order items")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 8.0F, MetaColumn::ColumnChoice( 8.0F,  "1024", "Allow Ship Straight order items")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 9.0F, MetaColumn::ColumnChoice( 9.0F,    "64", "Allow Cut Sub")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(10.0F, MetaColumn::ColumnChoice(10.0F,   "128", "Allow Requesting Solutions")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(11.0F, MetaColumn::ColumnChoice(11.0F,   "256", "Allow exporting/downloading solutions")));

#if defined (USING_AVLB)
		pMC = new MetaColumnLong		(pME, "V3ParamSettings",					MetaColumn::Flags::MultiChoice | MetaColumn::Flags::HiddenOnListView);
#else
		pMC = new MetaColumnLong		(pME, "V3ParamSettings",					MetaColumn::Flags::MultiChoice | MetaColumn::Flags::HiddenOnListView  | MetaColumn::Flags::HiddenOnDetailView);
#endif
		pMC->m_strProsaName = "Order optimization parameters";
		pMC->m_strDescription = "Specifies which types of order optimization parameters a Role is allowed to modify.";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
 		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(1.0F, MetaColumn::ColumnChoice( 1.0F,     "1", "Modify Ship-from location parameters")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(2.0F, MetaColumn::ColumnChoice( 2.0F,     "2", "Modify Lane parameters")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(3.0F, MetaColumn::ColumnChoice( 3.0F,     "4", "Modify Shipment parameters")));

		pMC = new MetaColumnLong		(pME, "T3ParamSettings",					MetaColumn::Flags::MultiChoice | MetaColumn::Flags::HiddenOnListView);
		pMC->m_strProsaName = "Truck loading parameters";
		pMC->m_strDescription = "Specifies which types of truck loading parameters a Role is allowed to modify.";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
 		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(1.0F, MetaColumn::ColumnChoice( 1.0F,     "1", "Modify Ship-from location parameters")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(2.0F, MetaColumn::ColumnChoice( 2.0F,     "2", "Modify Lane parameters")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(3.0F, MetaColumn::ColumnChoice( 3.0F,     "4", "Modify Shipment parameters")));

		pMC = new MetaColumnLong		(pME, "P3ParamSettings",					MetaColumn::Flags::MultiChoice | MetaColumn::Flags::HiddenOnListView);
		pMC->m_strProsaName = "Case pick parameters";
		pMC->m_strDescription = "Specifies which types of case pick parameters a Role is allowed to modify.";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
 		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(1.0F, MetaColumn::ColumnChoice( 1.0F,     "1", "Modify Ship-from location parameters")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(2.0F, MetaColumn::ColumnChoice( 2.0F,     "2", "Modify Customer parameters")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(3.0F, MetaColumn::ColumnChoice( 3.0F,     "4", "Modify Order parameters")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type(4.0F, MetaColumn::ColumnChoice( 4.0F,     "8", "Modify Zone parameters")));

		pMK = new MetaKey(pME, "PK_D3Role", MetaKey::Flags::PrimaryAutoNumKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("ID"));

		pMK = new MetaKey(pME, "SK_D3Role", MetaKey::Flags::UniqueSecondaryKey | MetaKey::Flags::Conceptual);
		pMK->m_strProsaName = "By Name";
		pMK->AddMetaColumn(pME->GetMetaColumn("Name"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3User MetaEntity
		//
		pME = new MetaEntity(pMD, "D3User", "D3User");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "User";
		pME->m_strInstanceInterface = "D3User";

		pMC = new MetaColumnLong		(pME, "ID",												MetaColumn::Flags::Mandatory | MetaColumn::Flags::AutoNum | MetaColumn::Flags::Hidden);
		pMC->m_strProsaName = "Unique ID";
		pMC->m_strDescription = "Unique identifier";
		pMC = new MetaColumnString	(pME, "Name",												64, MetaColumn::Flags::Mandatory);
		pMC->AllowWrite(false);
		pMC = new MetaColumnBinary	(pME, "Password",										16, MetaColumn::Flags::Mandatory | MetaColumn::Flags::Password | MetaColumn::Flags::HiddenOnListView);
		pMC = new MetaColumnBool		(pME, "Enabled",												MetaColumn::Flags::Mandatory);
		pMC = new MetaColumnString	(pME, "WarehouseAreaAccessList",	2000, MetaColumn::Flags::Hidden);
		pMC = new MetaColumnString	(pME, "CustomerAccessList",				4000, MetaColumn::Flags::Hidden);
		pMC = new MetaColumnString	(pME, "TransmodeAccessList",			4000, MetaColumn::Flags::Hidden);
		pMC = new MetaColumnInt			(pME, "PWDAttempts",										MetaColumn::Flags::Hidden);
		pMC->AllowWrite(false);
		pMC = new MetaColumnDate		(pME, "PWDExpires",											MetaColumn::Flags::Hidden);
		pMC->AllowWrite(false);
		pMC = new MetaColumnBool		(pME, "Temporary",											MetaColumn::Flags::Hidden);
		pMC->AllowWrite(false);
		pMC = new MetaColumnString	(pME, "Language",										25);

		pMK = new MetaKey(pME, "PK_D3User", MetaKey::Flags::PrimaryAutoNumKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("ID"));

		pMK = new MetaKey(pME, "SK_D3User", MetaKey::Flags::UniqueSecondaryKey | MetaKey::Flags::Conceptual);
		pMK->m_strProsaName = "By Name";
		pMK->AddMetaColumn(pME->GetMetaColumn("Name"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3HistoricPassword MetaEntity
		//
		pME = new MetaEntity(pMD, "D3HistoricPassword", "D3HistoricPassword", MetaEntity::Flags::Hidden);
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "Historic Password";
		pME->m_strInstanceInterface = "D3HistoricPassword";

		pMC = new MetaColumnLong		(pME, "UserID",													MetaColumn::Flags::Mandatory);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnBinary	(pME, "Password",										16, MetaColumn::Flags::Mandatory | MetaColumn::Flags::Password);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnDate		(pME, "PWDExpired");

		pMK = new MetaKey(pME, "PK_D3HistoricPassword", MetaKey::Flags::PrimaryKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->AddMetaColumn(pME->GetMetaColumn("UserID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("PWDExpired"));
		pMK->AddMetaColumn(pME->GetMetaColumn("Password"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3RoleUser MetaEntity
		//
		pME = new MetaEntity(pMD, "D3RoleUser", "D3RoleUser");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "Role/User";
		pME->m_strInstanceInterface = "D3RoleUser";

		pMC = new MetaColumnLong		(pME, "ID",												MetaColumn::Flags::Mandatory | MetaColumn::Flags::AutoNum | MetaColumn::Flags::Hidden);
		pMC->m_strProsaName = "Unique ID";
		pMC->m_strDescription = "Unique identifier";
		pMC = new MetaColumnLong		(pME, "RoleID",										MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnLong		(pME, "UserID",										MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;

		pMK = new MetaKey(pME, "PK_D3RoleUser", MetaKey::Flags::PrimaryAutoNumKey | MetaKey::Flags::HiddenOnQuickAccess);
		pMK->m_pInstanceClass = &(Class::Of("CK_D3RoleUser"));
		pMK->AddMetaColumn(pME->GetMetaColumn("ID"));

		pMK = new MetaKey(pME, "SK_D3RoleUser", MetaKey::Flags::UniqueSecondaryKey | MetaKey::Flags::Foreign);
		pMK->AddMetaColumn(pME->GetMetaColumn("RoleID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("UserID"));

		pMK = new MetaKey(pME, "FK_D3RoleUser_User", MetaKey::Flags::MandatoryForeignKey);
		pMK->AddMetaColumn(pME->GetMetaColumn("UserID"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3Session MetaEntity
		//
		pME = new MetaEntity(pMD, "D3Session", "D3Session");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "User Session";
		pME->m_strInstanceInterface = "D3Session";

		pME->m_Permissions = MetaEntity::Permissions::Select;

		pMC = new MetaColumnLong		(pME, "ID",												MetaColumn::Flags::Mandatory | MetaColumn::Flags::AutoNum | MetaColumn::Flags::Hidden);
		pMC->m_strProsaName = "Unique ID";
		pMC->m_strDescription = "Unique identifier";
		pMC = new MetaColumnLong		(pME, "RoleUserID",								MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnDate		(pME, "SignedIn");
		pMC->m_strProsaName = "Signed-In Date";
		pMC->m_strDescription = "The date/time the session was started.";
		pMC = new MetaColumnDate		(pME, "SignedOut");
		pMC->m_strProsaName = "Signed-Out Date";
		pMC->m_strDescription = "The date/time the session was terminated.";

		pMK = new MetaKey(pME, "PK_D3Session", MetaKey::Flags::PrimaryAutoNumKey);
		pMK->AddMetaColumn(pME->GetMetaColumn("ID"));

		pMK = new MetaKey(pME, "FK_D3Session_RoleUser", MetaKey::Flags::Foreign);
		pMK->AddMetaColumn(pME->GetMetaColumn("RoleUserID"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3DatabasePermission MetaEntity
		//
		pME = new MetaEntity(pMD, "D3DatabasePermission", "D3DatabasePermission");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "Role/Meta Database Permission";
		pME->m_strInstanceInterface = "D3DatabasePermission";

		pMC = new MetaColumnLong		(pME, "RoleID",										MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnLong		(pME, "MetaDatabaseID",						MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;

		pMC = new MetaColumnLong		(pME, "AccessRights",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::MultiChoice);
		pMC->m_strProsaName = "Access Rights";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,    "1", "Create")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,    "2", "Read")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 3.0F, MetaColumn::ColumnChoice( 3.0F,    "4", "Write")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 4.0F, MetaColumn::ColumnChoice( 4.0F,    "8", "Delete")));
		pMC->SetDefaultValue("15");

		pMK = new MetaKey(pME, "PK_D3DatabasePermission", MetaKey::Flags::PrimaryKey | MetaKey::Flags::Foreign);
		pMK->AddMetaColumn(pME->GetMetaColumn("RoleID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaDatabaseID"));

		pMK = new MetaKey(pME, "FK_D3DatabasePermission_MetaDatabase", MetaKey::Flags::MandatoryForeignKey);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaDatabaseID"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3EntityPermission MetaEntity
		//
		pME = new MetaEntity(pMD, "D3EntityPermission", "D3EntityPermission");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "Role/Meta Entity Permission";
		pME->m_strInstanceInterface = "D3EntityPermission";

		pMC = new MetaColumnLong		(pME, "RoleID",										MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnLong		(pME, "MetaEntityID",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;

		pMC = new MetaColumnLong		(pME, "AccessRights",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::MultiChoice);
		pMC->m_strProsaName = "Access Rights";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,    "1", "Insert")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,    "2", "Select")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 3.0F, MetaColumn::ColumnChoice( 3.0F,    "4", "Update")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 4.0F, MetaColumn::ColumnChoice( 4.0F,    "8", "Delete")));
		pMC->SetDefaultValue("15");

		pMK = new MetaKey(pME, "PK_D3EntityPermission", MetaKey::Flags::PrimaryKey | MetaKey::Flags::Foreign);
		pMK->AddMetaColumn(pME->GetMetaColumn("RoleID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaEntityID"));

		pMK = new MetaKey(pME, "FK_D3EntityPermission_MetaEntity", MetaKey::Flags::MandatoryForeignKey);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaEntityID"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3ColumnPermission MetaEntity
		//
		pME = new MetaEntity(pMD, "D3ColumnPermission", "D3ColumnPermission");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "Role/Meta Column Permission";
		pME->m_strInstanceInterface = "D3ColumnPermission";

		pMC = new MetaColumnLong		(pME, "RoleID",										MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnLong		(pME, "MetaColumnID",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;

		pMC = new MetaColumnLong		(pME, "AccessRights",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::MultiChoice);
		pMC->m_strProsaName = "Access Rights";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,    "1", "Read")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,    "2", "Write")));
		pMC->SetDefaultValue("3");

		pMK = new MetaKey(pME, "PK_D3ColumnPermission", MetaKey::Flags::PrimaryKey | MetaKey::Flags::Foreign);
		pMK->AddMetaColumn(pME->GetMetaColumn("RoleID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaColumnID"));

		pMK = new MetaKey(pME, "FK_D3ColumnPermission_MetaColumn", MetaKey::Flags::MandatoryForeignKey);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaColumnID"));

		pME->On_AfterConstructingMetaEntity();

		// Create D3RowLevelPermission MetaEntity
		//
		pME = new MetaEntity(pMD, "D3RowLevelPermission", "D3RowLevelPermission");
		pME->On_BeforeConstructingMetaEntity();

		pME->m_strProsaName = "Role/Row Level Permission";
		pME->m_strInstanceInterface = "D3RowLevelPermission";

		pMC = new MetaColumnLong		(pME, "UserID",										MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;
		pMC = new MetaColumnLong		(pME, "MetaEntityID",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::Hidden);
		pMC->m_Permissions = MetaColumn::Permissions::Read;

		pMC = new MetaColumnLong		(pME, "AccessRights",							MetaColumn::Flags::Mandatory | MetaColumn::Flags::SingleChoice);
		pMC->m_strProsaName = "Access Rights";
		pMC->m_pMapColumnChoice = new MetaColumn::ColumnChoiceMap();
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 1.0F, MetaColumn::ColumnChoice( 1.0F,    "1", "Allow")));
		pMC->m_pMapColumnChoice->insert(MetaColumn::ColumnChoiceMap::value_type( 2.0F, MetaColumn::ColumnChoice( 2.0F,    "2", "Deny")));
		pMC->SetDefaultValue("1");

		pMC = new MetaColumnString	(pME, "PKValueList",	2048);

		pMK = new MetaKey(pME, "PK_D3RowLevelPermission", MetaKey::Flags::PrimaryKey | MetaKey::Flags::Foreign);
		pMK->AddMetaColumn(pME->GetMetaColumn("UserID"));
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaEntityID"));

		pMK = new MetaKey(pME, "FK_D3RowLevelPermission_MetaEntity", MetaKey::Flags::MandatoryForeignKey);
		pMK->AddMetaColumn(pME->GetMetaColumn("MetaEntityID"));

		pME->On_AfterConstructingMetaEntity();

		// Create MetaRelation objects
		//

		// MD relations
		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaDatabase)			->GetMetaKey("PK_D3MetaDatabase");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3MetaEntity)				->GetMetaKey("FK_D3MetaEntity_MetaDatabaseID");
		pMR = new MetaRelation("D3MetaEntities",				"D3MetaDatabase",			pMKParent, pMKChild, "D3MetaDatabaseBase::D3MetaEntities",				MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaEntity)				->GetMetaKey("PK_D3MetaEntity");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3MetaColumn)				->GetMetaKey("FK_D3MetaColumn_MetaEntityID");
		pMR = new MetaRelation("D3MetaColumns",					"D3MetaEntity",				pMKParent, pMKChild, "D3MetaEntityBase::D3MetaColumns",						MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaEntity)				->GetMetaKey("PK_D3MetaEntity");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3MetaKey)						->GetMetaKey("FK_D3MetaKey_MetaEntityID");
		pMR = new MetaRelation("D3MetaKeys",						"D3MetaEntity",				pMKParent, pMKChild, "D3MetaEntityBase::D3MetaKeys",							MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaColumn)				->GetMetaKey("PK_D3MetaColumn");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3MetaKeyColumn)			->GetMetaKey("FK_D3MetaKeyColumn_MetaColumnID");
		pMR = new MetaRelation("D3MetaKeyColumns",			"D3MetaColumn",				pMKParent, pMKChild, "D3MetaColumnBase::D3MetaKeyColumns",				MetaRelation::Flags::PreventDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaColumn)				->GetMetaKey("PK_D3MetaColumn");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3MetaColumnChoice)	->GetMetaKey("PK_D3MetaColumnChoice");
		pMR = new MetaRelation("D3MetaColumnChoices",		"D3MetaColumn",				pMKParent, pMKChild, "D3MetaColumnBase::D3MetaColumnChoices",			MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaKey)					->GetMetaKey("PK_D3MetaKey");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3MetaKeyColumn)			->GetMetaKey("PK_D3MetaKeyColumn");
		pMR = new MetaRelation("D3MetaKeyColumns",			"D3MetaKey",					pMKParent, pMKChild, "D3MetaKeyBase::D3MetaKeyColumns",						MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaEntity)				->GetMetaKey("PK_D3MetaEntity");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3MetaRelation)			->GetMetaKey("SK_D3MetaRelation");
		pMR = new MetaRelation("ChildD3MetaRelations",	"ParentD3MetaEntity",	pMKParent, pMKChild, "D3MetaEntityBase::ChildD3MetaRelations",		MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaKey)					->GetMetaKey("PK_D3MetaKey");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3MetaRelation)			->GetMetaKey("FK_D3MetaRelation_ParentKeyID");
		pMR = new MetaRelation("ChildD3MetaRelations",	"ParentD3MetaKey",		pMKParent, pMKChild, "D3MetaKeyBase::ChildD3MetaRelations",				MetaRelation::Flags::PreventDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaKey)					->GetMetaKey("PK_D3MetaKey");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3MetaRelation)			->GetMetaKey("FK_D3MetaRelation_ChildKeyID");
		pMR = new MetaRelation("ParentD3MetaRelations",	"ChildD3MetaKey",			pMKParent, pMKChild, "D3MetaKeyBase::ParentD3MetaRelations",			MetaRelation::Flags::PreventDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaColumn)				->GetMetaKey("PK_D3MetaColumn");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3MetaRelation)			->GetMetaKey("FK_D3MetaRelation_SwitchColumnID");
		pMR = new MetaRelation("D3MetaRelationSwitches","SwitchD3MetaColumn",	pMKParent, pMKChild, "D3MetaColumnBase::D3MetaRelationSwitches",	MetaRelation::Flags::PreventDelete);

		// RBAC relations
		pMKParent = pMD->GetMetaEntity(D3MDDB_D3Role)							->GetMetaKey("PK_D3Role");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3RoleUser)					->GetMetaKey("SK_D3RoleUser");
		pMR = new MetaRelation("D3RoleUsers",						"D3Role",							pMKParent, pMKChild, "D3RoleBase::D3RoleUsers",										MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3User)							->GetMetaKey("PK_D3User");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3RoleUser)					->GetMetaKey("FK_D3RoleUser_User");
		pMR = new MetaRelation("D3RoleUsers",						"D3User",							pMKParent, pMKChild, "D3UserBase::D3RoleUsers",										MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3RoleUser)					->GetMetaKey("PK_D3RoleUser");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3Session)						->GetMetaKey("FK_D3Session_RoleUser");
		pMR = new MetaRelation("D3Sessions",						"D3RoleUser",					pMKParent, pMKChild, "D3RoleUserBase::D3Sessions",								MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3Role)							->GetMetaKey("PK_D3Role");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3DatabasePermission)->GetMetaKey("PK_D3DatabasePermission");
		pMR = new MetaRelation("D3DatabasePermissions",	"D3Role",							pMKParent, pMKChild, "D3RoleBase::D3DatabasePermissions",					MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaDatabase)			->GetMetaKey("PK_D3MetaDatabase");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3DatabasePermission)->GetMetaKey("FK_D3DatabasePermission_MetaDatabase");
		pMR = new MetaRelation("D3RolePermissions",			"D3MetaDatabase",			pMKParent, pMKChild, "D3MetaDatabaseBase::D3RolePermissions",			MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3Role)							->GetMetaKey("PK_D3Role");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3EntityPermission)	->GetMetaKey("PK_D3EntityPermission");
		pMR = new MetaRelation("D3EntityPermissions",		"D3Role",							pMKParent, pMKChild, "D3RoleBase::D3EntityPermissions",						MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaEntity)				->GetMetaKey("PK_D3MetaEntity");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3EntityPermission)	->GetMetaKey("FK_D3EntityPermission_MetaEntity");
		pMR = new MetaRelation("D3RolePermissions",			"D3MetaEntity",				pMKParent, pMKChild, "D3MetaEntityBase::D3RolePermissions",				MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3Role)							->GetMetaKey("PK_D3Role");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3ColumnPermission)	->GetMetaKey("PK_D3ColumnPermission");
		pMR = new MetaRelation("D3ColumnPermissions",		"D3Role",							pMKParent, pMKChild, "D3RoleBase::D3ColumnPermissions",						MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaColumn)				->GetMetaKey("PK_D3MetaColumn");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3ColumnPermission)	->GetMetaKey("FK_D3ColumnPermission_MetaColumn");
		pMR = new MetaRelation("D3RolePermissions",			"D3MetaColumn",				pMKParent, pMKChild, "D3MetaColumnBase::D3RolePermissions",				MetaRelation::Flags::CascadeDelete);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3User)							->GetMetaKey("PK_D3User");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3RowLevelPermission)->GetMetaKey("PK_D3RowLevelPermission");
		pMR = new MetaRelation("D3RowLevelPermissions",	"D3User",							pMKParent, pMKChild, "D3UserBase::D3RowLevelPermissions",					MetaRelation::Flags::CascadeDelete | MetaRelation::Flags::Hidden);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3MetaEntity)				->GetMetaKey("PK_D3MetaEntity");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3RowLevelPermission)->GetMetaKey("FK_D3RowLevelPermission_MetaEntity");
		pMR = new MetaRelation("D3RowLevelPermissions",	"D3MetaEntity",				pMKParent, pMKChild, "D3MetaEntityBase::D3RowLevelPermissions",		MetaRelation::Flags::CascadeDelete | MetaRelation::Flags::Hidden);

		pMKParent = pMD->GetMetaEntity(D3MDDB_D3User)							->GetMetaKey("PK_D3User");
		pMKChild = pMD->GetMetaEntity(D3MDDB_D3HistoricPassword)	->GetMetaKey("PK_D3HistoricPassword");
		pMR = new MetaRelation("D3HistoricPasswords",   "D3User",							pMKParent, pMKChild, "D3UserBase::D3HistoricPasswords",						MetaRelation::Flags::CascadeDelete | MetaRelation::Flags::Hidden);

		if (!pMD->VerifyMetaData())
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaDatabase::Initialise(): Failed to initialise MetaDictionary");
	}





	bool MetaDatabase::VerifyMetaData()
	{
		unsigned int		idx;
		bool						bSuccess = true;


		#ifdef AP3_OS_TARGET_HPUX
			std::string strEnvr;
		#endif

		// Build the connection string
		//
		switch (m_eTargetRDBMS)
		{
			case SQLServer:
#ifdef APAL_SUPPORT_ODBC
				if (m_pInstanceClass && m_pInstanceClass->Name() == "ODBCDatabase")
				{
					m_strConnectionString  = "DRIVER=";
					m_strConnectionString += m_strDriver;
					m_strConnectionString += ";SERVER=";
					m_strConnectionString += m_strServer;
					m_strConnectionString += ";DATABASE=";
					m_strConnectionString += m_strName;
					if (D3::ToUpper(m_strUserID) == "WINDOWS")
					{
						m_strConnectionString += ";TRUSTED_CONNECTION=YES";
					}
					else
					{
						m_strConnectionString += ";UID=";
						m_strConnectionString += m_strUserID;
						m_strConnectionString += ";PWD=";
						m_strConnectionString += APALUtil::decryptPassword(m_strUserPWD);
					}

					break;
				}
#endif

				ReportError("MetaDatabase::VerifyMetaData(): SQLServer databases require APAL_SUPPORT_ODBC and InstanceClass ODBCDatabase.");
				bSuccess = false;
				break;

			case Oracle:
#ifdef APAL_SUPPORT_ODBC
				if (m_pInstanceClass && m_pInstanceClass->Name() == "ODBCDatabase")
				{
#ifdef WIN32
					m_strConnectionString  = "DRIVER=";
					m_strConnectionString += m_strDriver;
					m_strConnectionString += ";DATABASE=";
					m_strConnectionString += m_strServer;
					if (D3::ToUpper(m_strUserID) == "WINDOWS")
					{
						m_strConnectionString += ";TRUSTED_CONNECTION=YES";
					}
					else
					{
						m_strConnectionString += ";UID=";
						m_strConnectionString += m_strUserID;
						m_strConnectionString += ";PWD=";
						m_strConnectionString += APALUtil::decryptPassword(m_strUserPWD);
					}
					m_strConnectionString += ";";
#else
					m_strConnectionString  = "DSN=";
					m_strConnectionString += m_strDriver;
					m_strConnectionString += ";SRVR=";
					m_strConnectionString += m_strServer;
					if (D3::ToUpper(m_strUserID) == "WINDOWS")
					{
						m_strConnectionString += ";TRUSTED_CONNECTION=YES";
					}
					else
					{
						m_strConnectionString += ";UID=";
						m_strConnectionString += m_strUserID;
						m_strConnectionString += ";PWD=";
						m_strConnectionString += APALUtil::decryptPassword(m_strUserPWD);
					}
					m_strConnectionString += ";";
#endif

					break;
				}
#endif

#ifdef APAL_SUPPORT_OTL
				if (m_pInstanceClass && m_pInstanceClass->Name() == "OTLDatabase")
				{
					m_strConnectionString  = m_strUserID;
					m_strConnectionString += "/";
					m_strConnectionString += APALUtil::decryptPassword(m_strUserPWD);
					m_strConnectionString += "@";
					m_strConnectionString += m_strServer;

					break;
				}
#endif /* APAL_SUPPORT_OTL */

			default:
				ReportError("MetaDatabase::VerifyMetaData(): MetaDatabase %s is of an unknown RDBMS type or associated instance class is not supported in this build.", GetName().c_str());
				bSuccess = false;
		}

		// Verify all MetaEntity objects
		//
		for (idx = 0; idx < m_vectMetaEntity.size(); idx++)
		{
			// Don't throw just yet, we really want to report all errors to make it easier to fix things for a user
			if (!m_vectMetaEntity[idx]->VerifyMetaData())
				bSuccess = false;
		}

		// Ensure we have D3 version info (this is a table that is not in the meta dictionary)

		//
		if (HasVersionInfo() && GetVersionInfoMetaEntity() == NULL)
		{
			try
			{
				MetaEntityPtr	pME;
				MetaColumnPtr	pMC;
				MetaKeyPtr		pMK;
				std::string		strPKName;


				// Create D3 version info MetaEntity
				//
				pME = new MetaEntity(this, GetVersionInfoMetaEntityName(), "Entity");
				pME->On_BeforeConstructingMetaEntity();

				pMC = new MetaColumnLong		(pME, "ID",										MetaColumn::Flags::Mandatory | MetaColumn::Flags::AutoNum | MetaColumn::Flags::Hidden);
				pMC = new MetaColumnString	(pME, "Name",							64, MetaColumn::Flags::Mandatory);
				pMC = new MetaColumnShort		(pME, "VersionMajor",					MetaColumn::Flags::Mandatory);
				pMC = new MetaColumnShort		(pME, "VersionMinor",					MetaColumn::Flags::Mandatory);
				pMC = new MetaColumnShort		(pME, "VersionRevision",			MetaColumn::Flags::Mandatory);
				pMC = new MetaColumnDate		(pME, "LastUpdated",					MetaColumn::Flags::Mandatory);

				strPKName  = "PK_";
				strPKName += GetAlias();
				strPKName += "D3VI";

				pMK = new MetaKey(pME, strPKName, MetaKey::Flags::PrimaryAutoNumKey | MetaKey::Flags::HiddenOnQuickAccess);
				pMK->AddMetaColumn(pME->GetMetaColumn("ID"));

				pME->On_AfterConstructingMetaEntity();
			}
			catch (Exception & e)
			{
				e.LogError();
				bSuccess = false;
			}
			catch (...)
			{
				ReportError("MetaDatabase::VerifyMetaData(): Unspecified error occurred creating/populating VersionInfo tables for database %s %s.", m_strAlias.c_str(), GetVersion().c_str());
				bSuccess = false;
			}
		}

		if (HasObjectLinks())
		{

			// todo: implement object link generation
			assert(false);
		}

		m_bInitialised = true;

		return bSuccess;
	}





	void MetaDatabase::On_InstanceCreated(DatabasePtr pDatabase)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxExclusive);

		assert(!IsInstance(pDatabase));
		m_listDatabase.push_back(pDatabase);

		// Notify all MetaEntity objects
		//
		for (unsigned int idx = 0; idx < m_vectMetaEntity.size(); idx++)
			m_vectMetaEntity[idx]->On_DatabaseCreated(pDatabase);
	}





	void MetaDatabase::On_InstanceDeleted(DatabasePtr pDatabase)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxExclusive);

		unsigned int													idx;
		DatabasePtrListItr										itrDatabase;


		assert(IsInstance(pDatabase));

		// Notify all MetaEntity objects
		//
		for (idx = 0; idx < m_vectMetaEntity.size(); idx++)
			m_vectMetaEntity[idx]->On_DatabaseDeleted(pDatabase);

		// Remove the instance from the list of instances
		//
		for ( itrDatabase  = m_listDatabase.begin();
					itrDatabase != m_listDatabase.end();
					itrDatabase++)
		{
			if (pDatabase == *itrDatabase)
			{
				m_listDatabase.erase(itrDatabase);
				return;
			}
		}
	}





	void MetaDatabase::On_MetaEntityCreated(MetaEntityPtr pMetaEntity)
	{
		unsigned int		idx = m_vectMetaEntity.size();

		assert(pMetaEntity);
		assert(pMetaEntity->GetMetaDatabase() == this);
		m_vectMetaEntity.resize(idx + 1);
		m_vectMetaEntity[idx] = pMetaEntity;
		pMetaEntity->m_uEntityIdx = idx;
	}





	void MetaDatabase::On_MetaEntityDeleted(MetaEntityPtr pMetaEntity)
	{
		assert(pMetaEntity);
		assert(pMetaEntity->GetMetaDatabase() == this);
		m_vectMetaEntity[pMetaEntity->m_uEntityIdx] = NULL;
	}





	//! Notification sent when a new MetaDatabaseAlert based on this is instantiated
	void MetaDatabase::On_MetaDatabaseAlertCreated(MetaDatabaseAlertPtr pMDBAlert)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxExclusive);

		assert(m_mapMetaDatabaseAlert.find(pMDBAlert->GetName()) == m_mapMetaDatabaseAlert.end());
		m_mapMetaDatabaseAlert[pMDBAlert->GetName()] = pMDBAlert;
	}





	//! Notification sent when a MetaDatabaseAlert based on this is being deleted
	void MetaDatabase::On_MetaDatabaseAlertDeleted(MetaDatabaseAlertPtr pMDBAlert)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxExclusive);
		MetaDatabaseAlertPtrMapItr itr = m_mapMetaDatabaseAlert.find(pMDBAlert->GetName());
		if (itr != m_mapMetaDatabaseAlert.end() )
			m_mapMetaDatabaseAlert.erase(itr);
	}





	bool MetaDatabase::IsInstance(DatabasePtr pDatabase)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxExclusive);

		DatabasePtrListItr	itrDatabase;


		assert(pDatabase);

		// The database must not be NULL and be an instance of this
		//
		if (!pDatabase || pDatabase->GetMetaDatabase() != this)
			return false;

		for ( itrDatabase  = m_listDatabase.begin();
					itrDatabase != m_listDatabase.end();
					itrDatabase++)
		{
			if (pDatabase == *itrDatabase)
				return true;
		}

		return false;
	}



	/* static */
	std::ostream & MetaDatabase::AllAsJSON(RoleUserPtr pRoleUser, std::ostream & ojson)
	{
		RolePtr											pRole = pRoleUser ? pRoleUser->GetRole() : NULL;
		D3::MetaDatabasePtr					pMD;
		D3::MetaDatabasePtrMapItr		itr;
		bool												bFirst = true;
		bool												bShallow = true;

		ojson << "[";

		for ( itr =  M_mapMetaDatabase.begin();
					itr != M_mapMetaDatabase.end();
					itr++)
		{
			pMD = itr->second;

			if (pRole->GetPermissions(pMD) & D3::MetaDatabase::Permissions::Read)
			{
				bFirst ? bFirst = false : ojson << ',';
				ojson << "\n\t";
				pMD->AsJSON(pRoleUser, ojson, NULL, bShallow);
			}
		}

		ojson << "\n]";


		return ojson;
	}



	std::ostream & MetaDatabase::AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, bool * pFirstSibling, bool bShallow)
	{
		RolePtr											pRole = pRoleUser ? pRoleUser->GetRole() : NULL;
		MetaDatabase::Permissions		permissions = pRole ? pRole->GetPermissions(this) : m_Permissions;

		if (permissions & MetaDatabase::Permissions::Read)
		{
			if (pFirstSibling)
			{
				if (*pFirstSibling)
					*pFirstSibling = false;
				else
					ostrm << ',';
			}

			ostrm << "{";
			ostrm << "\"ID\":"									<< GetID() << ",";

			ostrm << "\"TargetRDBMS\":";

			switch (m_eTargetRDBMS)
			{
				case Oracle:
					ostrm << "\"Oracle\",";
					break;

				case SQLServer:
					ostrm << "\"SQLServer\",";
					break;

				default:
					ostrm << "\"Undefined\",";
			}

			ostrm << "\"InstanceClass\":\""				<< JSONEncode(m_pInstanceClass->Name())	<< "\",";
			ostrm << "\"InstanceInterface\":\""		<< JSONEncode(m_strInstanceInterface)		<< "\",";
			ostrm << "\"Alias\":\""								<< JSONEncode(m_strAlias)								<< "\",";
			ostrm << "\"Name\":\""								<< JSONEncode(m_strName)								<< "\",";
			ostrm << "\"ProsaName\":\""						<< JSONEncode(m_strProsaName)						<< "\",";
/*
			std::string				strBase64Desc;
			APALUtil::base64_encode(strBase64Desc, (const unsigned char *) GetDescription().c_str(), GetDescription().size());
			ostrm << "\"Description\":\""					<< strBase64Desc												<< "\",";
*/
			ostrm << "\"Driver\":\""							<< JSONEncode(m_strDriver)							<< "\",";
			ostrm << "\"Server\":\""							<< JSONEncode(m_strServer)							<< "\",";
			ostrm << "\"UserID\":\""							<< JSONEncode(m_strUserID)							<< "\",";
			ostrm << "\"UserPWD\":\""							<< JSONEncode(m_strUserPWD)							<< "\",";
			ostrm << "\"DataFilePath\":\""				<< JSONEncode(m_strDataFilePath)				<< "\",";
			ostrm << "\"LogFilePath\":\""					<< JSONEncode(m_strLogFilePath)					<< "\",";
			ostrm << "\"InitialSize\":"						<< m_iInitialSize												<< ',';
			ostrm << "\"MaxSize\":"								<< m_iMaxSize														<< ',';
			ostrm << "\"GrowthSize\":"						<< m_iGrowthSize												<< ',';
			ostrm << "\"VersionMajor\":"					<< m_uVersionMajor											<< ',';
			ostrm << "\"VersionMinor\":"					<< m_uVersionMinor											<< ',';
			ostrm << "\"VersionRevision\":"				<< m_uVersionRevision										<< ',';
			ostrm << "\"HasVersionInfo\":"				<< (HasVersionInfo()	? "true" : "false")			<< ',';
			ostrm << "\"HasObjectLinks\":"				<< (HasObjectLinks()	? "true" : "false")			<< ',';
			ostrm << "\"AllowCreate\":"						<< (permissions & MetaDatabase::Permissions::Create			? "true" : "false")			<< ',';
			ostrm << "\"AllowRead\":"							<< (permissions & MetaDatabase::Permissions::Read				? "true" : "false")			<< ',';
			ostrm << "\"AllowWrite\":"						<< (permissions & MetaDatabase::Permissions::Write			? "true" : "false")			<< ',';
			ostrm << "\"AllowDelete\":"						<< (permissions & MetaDatabase::Permissions::Delete			? "true" : "false")			<< ',';
			ostrm << "\"HSTopics\":"							<< (m_strHSTopicsJSON.empty() ? "null" : m_strHSTopicsJSON);
			ostrm << "}";
		}

		return ostrm;
	}




	//! Returns a MetaDatabaseDefinition for self
	MetaDatabaseDefinition MetaDatabase::GetMetaDatabaseDefinition()
	{
		MetaDatabaseDefinition	aMDDef;

		aMDDef.m_eTargetRDBMS					= this->m_eTargetRDBMS;
		aMDDef.m_strDriver						= this->m_strDriver;
		aMDDef.m_strServer						= this->m_strServer;
		aMDDef.m_strUserID						= this->m_strUserID;
		aMDDef.m_strUserPWD						= this->m_strUserPWD;
		aMDDef.m_strName							= this->m_strName;
		aMDDef.m_strAlias							= this->m_strAlias;
		aMDDef.m_strDataFilePath			= this->m_strDataFilePath;
		aMDDef.m_strLogFilePath				= this->m_strLogFilePath;
		aMDDef.m_iInitialSize					= this->m_iInitialSize;
		aMDDef.m_iMaxSize							= this->m_iMaxSize;
		aMDDef.m_iGrowthSize					= this->m_iGrowthSize;
		aMDDef.m_pInstanceClass				= this->m_pInstanceClass;
		aMDDef.m_iVersionMajor				= this->m_uVersionMajor;
		aMDDef.m_iVersionMinor				= this->m_uVersionMinor;
		aMDDef.m_iVersionRevision			= this->m_uVersionRevision;

		return aMDDef;
	}



	//! Saves the MetaDictionary definitions itself to the meta dictionary database
	void MetaDatabase::StoreMDDefsToMetaDictionary()
	{
		DatabaseWorkspace																dbWS;
		DatabasePtr																			pDB = NULL;
		MetaDatabasePtr																	pMD = M_pDictionaryDatabase;
		MetaEntityPtr																		pME;
		MetaColumnPtr																		pMC;
		MetaKeyPtr																			pMK;
		MetaRelationPtr																	pMR;
		D3MetaDatabasePtr																pD3MD;
		D3MetaEntityPtr																	pD3ME;
		D3MetaColumnPtr																	pD3MC;
		D3MetaColumnChoicePtr														pD3MCC;
		D3MetaKeyPtr																		pD3MK;
		D3MetaKeyColumnPtr															pD3MKC;
		D3MetaRelationPtr																pD3MR;
		std::ostringstream															ostrm;
		InstanceKeyPtrSetPtr														pD3MEKeySet;
		InstanceKeyPtrSetItr														itrD3MEKeySet;


		try
		{
			if (!M_pDictionaryDatabase->m_bInitialised)
				throw Exception(__FILE__, __LINE__, Exception_error, "MetaDatabase::LoadMetaDictionary(): You must call MetaDatabase::Initialise() before calling this method");

			pDB = pMD->CreateInstance(&dbWS);

			// Delete the existing entry
			ostrm << "DELETE D3MetaDatabase WHERE ";
			ostrm << "Alias='" << pMD->GetAlias() << "' AND ";
			ostrm << "VersionMajor=" << pMD->GetVersionMajor() << " AND ";
			ostrm << "VersionMinor=" << pMD->GetVersionMinor() << " AND ";
			ostrm << "VersionRevision=" << pMD->GetVersionRevision();

			pDB->BeginTransaction();
			pDB->ExecuteSQLUpdateCommand(ostrm.str());
			pDB->CommitTransaction();

			// Create a D3MetaDatabase object from the MetaDictionary MetaDatabase object
			pD3MD = new D3MetaDatabase(pDB, pMD);

			// Create D3MetaEntity objects from the MetaEntity objects belonging to pMD
			for (unsigned int iME = 0; iME < pMD->m_vectMetaEntity.size(); iME++)
			{
				pME = pMD->m_vectMetaEntity[iME];
				pD3ME = new D3MetaEntity(pDB, pD3MD, pME, (float) iME);

				// Create D3MetaColumn objects from the MetaColumn objects belonging to pMD
				for (unsigned int iMC = 0; iMC < pME->m_vectMetaColumn.size(); iMC++)
				{
					pMC = pME->m_vectMetaColumn[iMC];
					pD3MC = new D3MetaColumn(pDB, pD3ME, pMC, (float) iMC);

					// Create D3MetaColumnChoice objects if this has choices
					if (pMC->GetChoiceMap())
					{
						MetaColumn::ColumnChoiceMapPtr	pCMap = pMC->GetChoiceMap();
						MetaColumn::ColumnChoiceMapItr	itrCM;

						for (	itrCM =	pCMap->begin(); itrCM != pCMap->end(); itrCM++)
						{
							pD3MCC = new D3MetaColumnChoice(pDB, pD3MC, itrCM->second);
						}
					}
				}

				// Create D3MetaKey objects from the MetaKey objects belonging to pMD
				for (unsigned int iMK = 0; iMK < pME->m_vectMetaKey.size(); iMK++)
				{
					pMK = pME->m_vectMetaKey[iMK];
					pD3MK = new D3MetaKey(pDB, pD3ME, pMK, (float) iMK);
					float fMKC = 0;

					// Create D3MetaKeyColumn objects
					for (MetaColumnPtrListItr itr = pMK->GetMetaColumns()->begin(); itr != pMK->GetMetaColumns()->end(); itr++)
					{
						fMKC += 1.0;
						pMC = *itr;

						for (D3MetaEntity::D3MetaColumns::iterator itrCols = pD3ME->GetD3MetaColumns()->begin(); itrCols != pD3ME->GetD3MetaColumns()->end(); itrCols++)
						{
							pD3MC = *itrCols;

							if (pD3MC->GetName() == pMC->GetName())
							{
								pD3MKC = new D3MetaKeyColumn(pDB, pD3MK, pD3MC, fMKC);
								break;
							}
						}
					}
				}
			}

			// Do relations last
			pD3MEKeySet = D3MetaEntity::GetAll(pDB);

			for (itrD3MEKeySet = pD3MEKeySet->begin(); itrD3MEKeySet != pD3MEKeySet->end(); itrD3MEKeySet++)
			{
				pD3ME = (D3MetaEntityPtr) (*itrD3MEKeySet)->GetEntity();
				pME = pMD->GetMetaEntity(pD3ME->GetName());

				// Create D3MetaKey objects from the MetaKey objects belonging to pMD
				InstanceKeyPtrSetPtr	pD3MKSet = D3MetaKey::GetAll(pDB);
				InstanceKeyPtrSetItr	itrD3MKSet;
				D3MetaKeyPtr					pParentD3MK;
				D3MetaKeyPtr					pChildD3MK;
				D3MetaColumnPtr				pSwitchD3MC;

				for (unsigned int iMR = 0; iMR < pME->m_vectChildMetaRelation.size(); iMR++)
				{
					pMR = pME->m_vectChildMetaRelation[iMR];

					// find the two keys
					pParentD3MK = pChildD3MK = NULL;
					for (itrD3MKSet = pD3MKSet->begin(); itrD3MKSet != pD3MKSet->end(); itrD3MKSet++)
					{
						pD3MK = (D3MetaKeyPtr) (*itrD3MKSet)->GetEntity();

						if (!pParentD3MK && pD3MK->GetName() == pMR->GetParentMetaKey()->GetName())
							pParentD3MK = pD3MK;
						else if (!pChildD3MK && pD3MK->GetName() == pMR->GetChildMetaKey()->GetName())
							pChildD3MK = pD3MK;

						if (pParentD3MK && pChildD3MK)
							break;
					}

					if (!pParentD3MK || !pChildD3MK)
					{
						ReportError("Can't find parent and/or child key of MetaRelation %s", pMR->GetFullName().c_str());
						break;
					}

					// find the switch coilumn if the relation is switched
					pSwitchD3MC = NULL;
					if (pMR->GetSwitchColumn())
					{
						D3MetaEntityPtr		pSwitchEntity = pMR->IsParentSwitch() ? pParentD3MK->GetD3MetaEntity() : pChildD3MK->GetD3MetaEntity();

						for (D3MetaEntity::D3MetaColumns::iterator itrCols = pSwitchEntity->GetD3MetaColumns()->begin(); itrCols != pSwitchEntity->GetD3MetaColumns()->end(); itrCols++)
						{
							pD3MC = *itrCols;

							if (pD3MC->GetName() == pMR->GetSwitchColumn()->GetMetaColumn()->GetName())
							{
								pSwitchD3MC = pD3MC;
								break;
							}
						}

						if (!pSwitchD3MC)
						{
							ReportError("Can't find switch column %s of MetaRelation %s", pMR->GetSwitchColumn()->GetMetaColumn()->GetFullName().c_str(), pMR->GetFullName().c_str());
							break;
						}
					}

					pD3MR = new D3MetaRelation(pDB, pParentD3MK, pChildD3MK, pSwitchD3MC, pMR, (float) iMR);
				}
			}

			pDB->BeginTransaction();
			pD3MD->Update(pDB, true);
			pDB->CommitTransaction();
		}
		catch(...)
		{
			if (pDB->HasTransaction())
				pDB->RollbackTransaction();

			D3::Exception::GenericExceptionHandler(__FILE__, __LINE__);
		}
	}






	//===========================================================================
	//
	// Database implementation
	//
	D3_CLASS_IMPL_PV(Database, Object);



	Database::~Database ()
	{
		// Delete any resultsets we still have
		if (m_plistResultSet)
		{
			ResultSetPtr		pRS;

			while (!m_plistResultSet->empty())
			{
				pRS = m_plistResultSet->front();
				delete pRS;
			}

			delete m_plistResultSet;
		}

		m_pMetaDatabase->On_InstanceDeleted(this);
		m_pDatabaseWorkspace->On_DatabaseDeleted(this);
	}



	// Throw if we get here as this should be dealt with by subclasses
	DatabaseAlertPtr Database::MonitorDatabaseAlerts(DatabaseAlertManagerPtr)
	{
		throw Exception(__FILE__, __LINE__, Exception_error, "Database::MonitorDatabaseAlerts(): Database %s does not support alerts!", m_pMetaDatabase->GetAlias().c_str());
		return NULL;
	}




	void Database::SetLastError(const char * pszLastError)
	{
		GetExceptionContext()->ReportErrorX(__FILE__, __LINE__, "Database::SetLastError(): %s", (void*) pszLastError);
	}



	// 07/06/03 - R4 - HugP
	//
	// Debug helper
	//
	std::string Database::GetObjectStatisticsText()
	{
		std::string			strStats, strTemp;
		MetaEntityPtr		pME;
		unsigned int		idx1, idx2, iObjectCount, iMaxLength = 0;


		strStats  = m_pMetaDatabase->GetName();
		char	buffer[100];
		sprintf(buffer, " (" PRINTF_POINTER_MASK ")", (int) this);
		strStats += buffer;
		strStats += " - Statistics:\n\n";

		// Determine maximum name length
		//
		for (idx1 = 0; idx1 <  m_pMetaDatabase->GetMetaEntities()->size(); idx1++)
		{

			pME = m_pMetaDatabase->GetMetaEntity(idx1);

			if (pME->GetName().size() > iMaxLength)
				iMaxLength = pME->GetName().size();
		}

		iMaxLength++;	// account for colon


		// Populate stats string
		//
		for (idx1 = 0; idx1 <  m_pMetaDatabase->GetMetaEntities()->size(); idx1++)
		{
			pME = m_pMetaDatabase->GetMetaEntity(idx1);

			iObjectCount =  pME->GetPrimaryMetaKey()->GetInstanceKeySet(this)->size();

			if (iObjectCount)
			{
				strStats += pME->GetName();

				for (idx2 = iMaxLength; idx2 > pME->GetName().size(); idx2--)

					strStats += ".";

				strStats += ": ";

				Convert(strTemp, iObjectCount);

				strStats += strTemp;
				strStats += "\n";
			}
		}

		strStats += "\n";


		return strStats;
	}



	void Database::On_PostCreate()
	{
		// Some sanity checks (this must NOT be initialised and both MetaDatabase and DatabaseWorkspace must be defined)
		//
		if (m_bInitialised || !m_pMetaDatabase || !m_pDatabaseWorkspace)
			throw Exception(__FILE__, __LINE__, Exception_error, "Database::On_PostCreate(): Initialisation sequence error!");

		m_pDatabaseWorkspace->On_DatabaseCreated(this);
		m_pMetaDatabase->On_InstanceCreated(this);
		m_uTrace = g_GlobalD3DebugLevel;
		m_bInitialised = true;
	}



	EntityPtrListPtr Database::LoadObjectsThroughQuery(MetaEntityPtr pME, const std::string & strWhere, const std::string & strOrderBy, bool bRefresh, bool bLazyFetch)
	{
		std::string		strSQL;

		assert(pME && m_pMetaDatabase == pME->GetMetaDatabase());

		strSQL = "SELECT ";
		strSQL += pME->AsSQLSelectList(bLazyFetch);
		strSQL += " FROM ";
		strSQL += pME->GetName();

		if (!strWhere.empty())
		{
			strSQL += " WHERE ";
			strSQL += strWhere;
		}

		if (!strOrderBy.empty())
		{
			strSQL += " ORDER BY ";
			strSQL += strOrderBy;
		}

		return this->LoadObjects(pME, strSQL, bRefresh, bLazyFetch);
	}



	void Database::DeleteAllObjects()
	{
		for (unsigned int idx = 0; idx <  m_pMetaDatabase->GetMetaEntities()->size(); idx++)
			m_pMetaDatabase->GetMetaEntity(idx)->DeleteAllObjects(this);
	}


	void Database::RegisterDatabaseAlert(DatabaseAlertPtr pDBAlert)
	{
		throw Exception(__FILE__, __LINE__, Exception_error, "Database::RegisterDatabaseAlert(): Failed to register alert %s. %s Databases do not support DatabaseAlerts.", pDBAlert->GetName().c_str(), m_pMetaDatabase->GetAlias().c_str());
	}


	void Database::UnRegisterDatabaseAlert(DatabaseAlertPtr pDBAlert)
	{
		throw Exception(__FILE__, __LINE__, Exception_error, "Database::UnRegisterDatabaseAlert(): Failed to unregister alert %s. %s Databases do not support DatabaseAlerts.", pDBAlert->GetName().c_str(), m_pMetaDatabase->GetAlias().c_str());
	}


	// Load all instances from each MetaEntity object marked cached (IsCached() == true)
	//
	void Database::LoadCache()
	{
		MetaEntityPtrList				listME;
		MetaEntityPtrListItr		itrME;
		MetaEntityPtr						pME;


		try
		{
			ReportInfo("Database::LoadCache(): Caching database %s...", m_pMetaDatabase->GetAlias().c_str());

			listME = m_pMetaDatabase->GetDependencyOrderedMetaEntities();

			for ( itrME  = listME.begin();
						itrME != listME.end();
						itrME++)
			{
				pME = *itrME;

				if (pME->IsCached())
					pME->LoadAll(this);
			}

			ReportInfo("Database::LoadCache(): ...done!");
		}
		catch(Exception & e)
		{
			e.AddMessage("Database::LoadCache(): Loading cache for database %s failed", m_pMetaDatabase->GetName().c_str());
			e.LogError();
			throw;
		}
		catch(...)
		{
			throw Exception(__FILE__, __LINE__, Exception_error, "Database::LoadCache(): Unspecified error occurred loading cache for database %s.", m_pMetaDatabase->GetName().c_str());
		}
	}



	// Add the newly created ResultSet to our list
	//
	void Database::On_ResultSetCreated(ResultSetPtr pResultSet)
	{
		assert(pResultSet);
		assert(pResultSet->GetDatabase() == this);

		pResultSet->m_lID = m_lNextRsltSetID++;

		if (!m_plistResultSet)
			m_plistResultSet = new ResultSetPtrList();

		m_plistResultSet->push_back(pResultSet);
	}



	// Remove the deleted ResultSet from our list
	//
	void Database::On_ResultSetDeleted(ResultSetPtr pResultSet)
	{
		assert(m_plistResultSet);
		m_plistResultSet->remove(pResultSet);
	}



	// Find a resultset belonging to this by ID
	//
	ResultSetPtr Database::GetResultSet(unsigned long lID)
	{
		if (m_plistResultSet)
		{
			ResultSetPtrListItr		itr;
			ResultSetPtr					pRS;

			for (	itr =  m_plistResultSet->begin();
						itr != m_plistResultSet->end();
						itr++)
			{
				pRS = *itr;

				if (pRS->GetID() == lID)
					return pRS;
			}
		}

		return NULL;
	}



	long Database::ExportRBACToJSON(const std::string & strRootName, const std::string & strJSONFileName)
	{
		MetaDatabaseDefinitionList			listMDDef;
		MetaDatabasePtrMapItr						itrMDB;
		MetaDatabasePtr									pMDB;
		D3User::iterator								itrUser;
		D3HistoricPassword::iterator		itrHistoricPassword;
		D3Role::iterator								itrRole;
		D3RoleUser::iterator						itrRoleUser;
		D3RowLevelPermission::iterator	itrRowLevelPermission;
		D3DatabasePermission::iterator	itrDatabasePermission;
		D3EntityPermission::iterator		itrEntityPermission;
		D3ColumnPermission::iterator		itrColumnPermission;
		std::ofstream										fjson;
		bool														bFirst;
		long														lRecCountTotal=0, lRecCountCurrent;


		try
		{
			// Make sure this is the MetaDictionary database
			if (m_pMetaDatabase != MetaDatabase::GetMetaDictionary())
			{
				std::cout << "Export RBAC only works on a MetaDictionary database\n";
				return -1;
			}

			std::cout << "Starting to export data from " << m_pMetaDatabase->GetAlias() << ' ' << m_pMetaDatabase->GetVersion() << std::endl;

			// Open the output file
			fjson.open(strJSONFileName.c_str());

			if (!fjson.is_open())
			{
				std::cout << "Failed to open " << strJSONFileName << " for writing!\n";
				return -1;
			}

			// Collect the currently loaded meta dictionaries so that we can make them resident.
			// At the same time, write the meta database aliases/versions as comment at the top
			//
			// {
			//	"[strRootName]":
			//	{
			//		"MetaDictionary":"D3MDDB V6.02.03",
			//		"CustomDatabases":
			//		[
			//			"APAL3DB V6.06.12",
			//			...
			//		],
			// ...
			bFirst = true;

			fjson << "{\n";
			fjson << "\t\"" << strRootName << "\":\n";
			fjson << "\t{\n";
			fjson << "\t\t\"MetaDictionary\":\"" << m_pMetaDatabase->GetAlias() << ' ' << m_pMetaDatabase->GetVersion() << "\",\n";
			fjson << "\t\t\"CustomDatabases\":\n";
			fjson << "\t\t[";

			for ( itrMDB =  MetaDatabase::GetMetaDatabases().begin();
						itrMDB != MetaDatabase::GetMetaDatabases().end();
						itrMDB++)
			{
				pMDB = itrMDB->second;

				if (pMDB != MetaDatabase::GetMetaDictionary())
				{
					if (bFirst)
						bFirst = false;
					else
						fjson << ',';

					fjson << "\n\t\t\t\"" << pMDB->GetAlias() << ' ' << pMDB->GetVersion() << '"';
					listMDDef.push_back(pMDB->GetMetaDatabaseDefinition());
				}
			}

			fjson << "\n\t\t],\n";

			// Let's make the relevant meta dictionary data resident
			D3MetaDatabase::MakeD3MetaDictionariesResident(this, listMDDef);

			// Dump Users
			lRecCountCurrent = 0;
			std::cout << "  RBAC Users...................: " << std::string(12, ' ');

			bFirst = true;
			fjson << "\t\t\"RBACUsers\":\n";
			fjson << "\t\t[";

			for (	itrUser  = D3User::begin(this);
						itrUser != D3User::end(this);
						itrUser++, lRecCountCurrent++)
			{
				if (bFirst)
					bFirst = false;
				else
					fjson << ',';

				(*itrUser)->ExportToJSON(fjson);

				if ((lRecCountCurrent % 100) == 0)
					std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent;
			}

			fjson << "\n\t\t],\n";

			std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent << std::endl;
			lRecCountTotal += lRecCountCurrent;

			// Dump HistoricPasswords
			lRecCountCurrent = 0;
			std::cout << "  RBAC HistoricPasswords.......: " << std::string(12, ' ');

			bFirst = true;
			fjson << "\t\t\"RBACHistoricPasswords\":\n";
			fjson << "\t\t[";

			for (	itrHistoricPassword  = D3HistoricPassword::begin(this);
						itrHistoricPassword != D3HistoricPassword::end(this);
						itrHistoricPassword++, lRecCountCurrent++)
			{
				if (bFirst)
					bFirst = false;
				else
					fjson << ',';

				(*itrHistoricPassword)->ExportToJSON(fjson);

				if ((lRecCountCurrent % 100) == 0)
					std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent;
			}

			fjson << "\n\t\t],\n";

			std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent << std::endl;
			lRecCountTotal += lRecCountCurrent;

			// Dump Roles
			lRecCountCurrent = 0;
			std::cout << "  RBAC Roles...................: " << std::string(12, ' ');

			bFirst = true;
			fjson << "\t\t\"RBACRoles\":\n";
			fjson << "\t\t[";

			for (	itrRole  = D3Role::begin(this);
						itrRole != D3Role::end(this);
						itrRole++, lRecCountCurrent++)
			{
				if (bFirst)
					bFirst = false;
				else
					fjson << ',';

				(*itrRole)->ExportToJSON(fjson);

				if ((lRecCountCurrent % 100) == 0)
					std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent;
			}

			fjson << "\n\t\t],\n";

			std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent << std::endl;
			lRecCountTotal += lRecCountCurrent;

			// Dump RoleUsers
			lRecCountCurrent = 0;
			std::cout << "  RBAC RoleUsers...............: " << std::string(12, ' ');

			bFirst = true;
			fjson << "\t\t\"RBACRoleUsers\":\n";
			fjson << "\t\t[";

			for (	itrRoleUser  = D3RoleUser::begin(this);
						itrRoleUser != D3RoleUser::end(this);
						itrRoleUser++, lRecCountCurrent++)
			{
				if (bFirst)
					bFirst = false;
				else
					fjson << ',';

				(*itrRoleUser)->ExportToJSON(fjson);

				if ((lRecCountCurrent % 100) == 0)
					std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent;
			}

			fjson << "\n\t\t],\n";

			std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent << std::endl;
			lRecCountTotal += lRecCountCurrent;

			// Dump RowLevelPermissions
			lRecCountCurrent = 0;
			std::cout << "  RBAC RowLevelPermissions.....: " << std::string(12, ' ');

			bFirst = true;
			fjson << "\t\t\"RBACRowLevelPermissions\":\n";
			fjson << "\t\t[";

			for (	itrRowLevelPermission  = D3RowLevelPermission::begin(this);
						itrRowLevelPermission != D3RowLevelPermission::end(this);
						itrRowLevelPermission++, lRecCountCurrent++)
			{
				if (bFirst)
					bFirst = false;
				else
					fjson << ',';

				(*itrRowLevelPermission)->ExportToJSON(fjson);

				if ((lRecCountCurrent % 100) == 0)
					std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent;
			}

			fjson << "\n\t\t],\n";

			std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent << std::endl;
			lRecCountTotal += lRecCountCurrent;

			// Dump DatabasePermissions
			lRecCountCurrent = 0;
			std::cout << "  RBAC DatabasePermissions.....: " << std::string(12, ' ');

			bFirst = true;
			fjson << "\t\t\"RBACDatabasePermissions\":\n";
			fjson << "\t\t[";

			for (	itrDatabasePermission  = D3DatabasePermission::begin(this);
						itrDatabasePermission != D3DatabasePermission::end(this);
						itrDatabasePermission++, lRecCountCurrent++)
			{
				if (bFirst)
					bFirst = false;
				else
					fjson << ',';

				(*itrDatabasePermission)->ExportToJSON(fjson);

				if ((lRecCountCurrent % 100) == 0)
					std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent;
			}

			fjson << "\n\t\t],\n";

			std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent << std::endl;
			lRecCountTotal += lRecCountCurrent;

			// Dump EntityPermissions
			lRecCountCurrent = 0;
			std::cout << "  RBAC EntityPermissions.......: " << std::string(12, ' ');

			bFirst = true;
			fjson << "\t\t\"RBACEntityPermissions\":\n";
			fjson << "\t\t[";

			for (	itrEntityPermission  = D3EntityPermission::begin(this);
						itrEntityPermission != D3EntityPermission::end(this);
						itrEntityPermission++, lRecCountCurrent++)
			{
				if (bFirst)
					bFirst = false;
				else
					fjson << ',';

				(*itrEntityPermission)->ExportToJSON(fjson);

				if ((lRecCountCurrent % 100) == 0)
					std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent;
			}

			fjson << "\n\t\t],\n";

			std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent << std::endl;
			lRecCountTotal += lRecCountCurrent;

			// Dump ColumnPermissions
			lRecCountCurrent = 0;
			std::cout << "  RBAC ColumnPermissions.......: " << std::string(12, ' ');

			bFirst = true;
			fjson << "\t\t\"RBACColumnPermissions\":\n";
			fjson << "\t\t[";

			for (	itrColumnPermission  = D3ColumnPermission::begin(this);
						itrColumnPermission != D3ColumnPermission::end(this);
						itrColumnPermission++, lRecCountCurrent++)
			{
				if (bFirst)
					bFirst = false;
				else
					fjson << ',';

				(*itrColumnPermission)->ExportToJSON(fjson);

				if ((lRecCountCurrent % 100) == 0)
					std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent;
			}

			fjson << "\n\t\t]\n";
			fjson << "\t}\n";
			fjson << "}\n";

			std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent << std::endl;
			lRecCountTotal += lRecCountCurrent;

			fjson.close();

			std::cout << "Finished to export RBAC data from database " << m_pMetaDatabase->GetName() << std::endl;
			std::cout << "Records exported..: " << lRecCountTotal << std::endl;
		}
		catch(...)
		{
			GenericExceptionHandler("ExportRBACToJSON(): exception caught. ");
			return -1;
		}

		return lRecCountTotal;
	}



	//! Import RBAC settings from an JSON file
	/*! This method expects strJSONFileName to point to a file that was created with
			the this' ExportRBACToJSON method.

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

	*/
	long Database::ImportRBACFromJSON(MetaDatabaseDefinitionList & listMDDefs, const std::string & strRootName, const std::string & strJSONFileName)
	{
		long														lRecCountTotal=0, lRecCountCurrent;
		Json::Value											jsnRoot, jsnValue;
		MetaKeyPtr											pMK;
		InstanceKeyPtrSet::iterator			itrKey;
		D3MDDB_Tables										aryRBACID[] = { D3MDDB_D3Role,
																										D3MDDB_D3User,
																										D3MDDB_D3HistoricPassword,
																										D3MDDB_D3RoleUser,
																										D3MDDB_D3RowLevelPermission,
																										D3MDDB_D3DatabasePermission,
																										D3MDDB_D3EntityPermission,
																										D3MDDB_D3ColumnPermission
																									};

		try
		{
			// Phase 1: Parse JSON file and preprocess it
			{
				Json::Reader										jsnReader;
				std::string											strJSONDoc;
				std::string											strLine;

				// Yiekes, we need to read entire file into strJSONDoc
				// (maybe an updated JSON lib might support parsing streams)
				std::ifstream ifJSONFile(strJSONFileName.c_str());

				while (std::getline(ifJSONFile, strLine))
					strJSONDoc += strLine;

				ifJSONFile.close();

				// Parse the file
				if (!jsnReader.parse(strJSONDoc, jsnRoot))
				{
					// report to the user the failure and their locations in the document.
					std::cout  << "Failed to parse RBAC file: " << strJSONFileName << std::endl
										 << jsnReader.getFormatedErrorMessages();
					return -1;
				}

				// We're only really interested in the root node
				jsnRoot =	jsnRoot[strRootName];

				if (jsnRoot.type() != Json::objectValue)
				{
					std::cout  << "The top-level JSON structure from file " << strJSONFileName << " does not contain member object \"" << strRootName << "\"\n";
					return -1;
				}

				// The meta dictionary versions must match
				std::string		strActualVersion = m_pMetaDatabase->GetAlias() + ' ' + m_pMetaDatabase->GetVersion();
				std::string		strImportVersion;

				jsnValue = jsnRoot["MetaDictionary"];

				if (jsnValue.type() == Json::stringValue)
					strImportVersion = jsnValue.asString();

				if (strImportVersion > strActualVersion)
				{
					std::cout  << "ERROR: Current MetaDictionary version " << strActualVersion << " is older than " << strImportVersion << " from the RBAC backup.\n";
					std::cout  << "       You can import identical or older RBAC backups only!\n";
					return -1;
				}

				if (strImportVersion < strActualVersion)
				{
					std::cout  << "WARNING: Current MetaDictionary version " << strActualVersion << " is newer than " << strImportVersion << " from the RBAC backup.\n";
					std::cout  << "         You may need to adjust some RBAC settings manually after the import!\n";
				}

				// Display all the version details of custom databases
				jsnValue = jsnRoot["CustomDatabases"];

				std::cout << "Import file contains RBAC settings originating from database(s):\n";

				for (unsigned int	idx = 0 ; idx < jsnValue.size(); idx++)
					std::cout << "  " << jsnValue[idx].asString() << std::endl;
			}

			// Phase 2: Make meta dictionaries resident and mark all RBAC objects
			{
				// Let's make the relevant meta dictionary data resident
				D3MetaDatabase::MakeD3MetaDictionariesResident(this, listMDDefs);

				// Mark all RBAC objects
				for (unsigned int idx = 0; idx < sizeof(aryRBACID)/sizeof(D3MDDB_Tables); idx++)
				{
					pMK = m_pMetaDatabase->GetMetaEntity(aryRBACID[idx])->GetPrimaryMetaKey();

					for ( itrKey =  pMK->GetInstanceKeySet(this)->begin();
								itrKey != pMK->GetInstanceKeySet(this)->end();
								itrKey++)
					{
						(*itrKey)->GetEntity()->MarkMarked();
					}
				}
			}

			// Phase 3: Import the RBAC objects
			{
				EntityPtr			pEntity;
				int						nIgnored, nAdded, nNoChange, nUpdated, nDeleted;
				std::string		strKey;
				enum RBACType
				{
					Users,
					HistoricPasswords,
					Roles,
					RoleUsers,
					RowLevelPermissions,
					DatabasePermissions,
					EntityPermissions,
					ColumnPermissions
				};

				G_bImportingRBAC = true;

				// We import all or nothing
				BeginTransaction();

				// Import RBAC Objects
				for (unsigned int type = 0; type <= ColumnPermissions; type++)
				{
					lRecCountCurrent = nIgnored = nAdded = nNoChange = nUpdated = nDeleted = 0;

					switch (type)
					{
						case Users:
							std::cout << "  RBAC Users...................: " << std::string(12, ' ');
							strKey = "RBACUsers";
							break;
						case HistoricPasswords:
							std::cout << "  RBAC HistoricPasswords.......: " << std::string(12, ' ');
							strKey = "RBACHistoricPasswords";
							break;
						case Roles:
							std::cout << "  RBAC Roles...................: " << std::string(12, ' ');
							strKey = "RBACRoles";
							break;
						case RoleUsers:
							std::cout << "  RBAC RoleUsers...............: " << std::string(12, ' ');
							strKey = "RBACRoleUsers";
							break;
						case RowLevelPermissions:
							std::cout << "  RBAC RowLevelPermissions.....: " << std::string(12, ' ');
							strKey = "RBACRowLevelPermissions";
							break;
						case DatabasePermissions:
							std::cout << "  RBAC DatabasePermissions.....: " << std::string(12, ' ');
							strKey = "RBACDatabasePermissions";
							break;
						case EntityPermissions:
							std::cout << "  RBAC EntityPermissions.......: " << std::string(12, ' ');
							strKey = "RBACEntityPermissions";
							break;
						case ColumnPermissions:
							std::cout << "  RBAC ColumnPermissions.......: " << std::string(12, ' ');
							strKey = "RBACColumnPermissions";
							break;
					}

					jsnValue = jsnRoot[strKey];

					for (unsigned int	idx = 0 ; idx < jsnValue.size(); idx++, lRecCountCurrent++)
					{
						switch (type)
						{
							case Users:
								pEntity = D3User::ImportFromJSON(this, jsnValue[idx]);
								pMK = NULL;
								break;
							case HistoricPasswords:
								pEntity = D3HistoricPassword::ImportFromJSON(this, jsnValue[idx]);
								pMK = NULL;
								break;
							case Roles:
								pEntity = D3Role::ImportFromJSON(this, jsnValue[idx]);
								pMK = NULL;
								break;
							case RoleUsers:
								pEntity = D3RoleUser::ImportFromJSON(this, jsnValue[idx]);
								pMK = NULL;
								break;
							case RowLevelPermissions:
								pEntity = D3RowLevelPermission::ImportFromJSON(this, jsnValue[idx]);
								pMK = m_pMetaDatabase->GetMetaEntity(D3MDDB_D3RowLevelPermission)->GetPrimaryMetaKey();
								break;
							case DatabasePermissions:
								pEntity = D3DatabasePermission::ImportFromJSON(this, jsnValue[idx]);
								pMK = m_pMetaDatabase->GetMetaEntity(D3MDDB_D3DatabasePermission)->GetPrimaryMetaKey();
								break;
							case EntityPermissions:
								pEntity = D3EntityPermission::ImportFromJSON(this, jsnValue[idx]);
								pMK = m_pMetaDatabase->GetMetaEntity(D3MDDB_D3EntityPermission)->GetPrimaryMetaKey();
								break;
							case ColumnPermissions:
								pEntity = D3ColumnPermission::ImportFromJSON(this, jsnValue[idx]);
								pMK = m_pMetaDatabase->GetMetaEntity(D3MDDB_D3ColumnPermission)->GetPrimaryMetaKey();
								break;
							default:
								pEntity = NULL;
						}

						if (pEntity)
						{
							if (!pEntity->IsDirty())
							{
								nNoChange++;
							}
							else
							{
								if (pEntity->IsNew())
									nAdded++;
								else
									nUpdated++;

								pEntity->Update(this);
							}
						}
						else
						{
							nIgnored++;
						}

						if ((lRecCountCurrent % 100) == 0)
							std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent;
					}

					if (pMK)
					{
						// Let's delete all objects which are still marked
						EntityPtrList		listDelete;

						for ( itrKey =  pMK->GetInstanceKeySet(this)->begin();
									itrKey != pMK->GetInstanceKeySet(this)->end();
									itrKey++)
						{
							pEntity = (*itrKey)->GetEntity();

							if (pEntity->IsMarked())
								listDelete.push_back(pEntity);
						}

						nDeleted = listDelete.size();

						while (!listDelete.empty())
						{
							listDelete.front()->Delete(this);
							listDelete.pop_front();
						}
					}

					std::cout << std::string(12, '\b') << std::setw(12) << lRecCountCurrent << "  [Ignored: "		<< std::setw(5) << nIgnored << ", " <<
																																												"NoChange: "	<< std::setw(5) << nNoChange << ", " <<
																																												"Updated: "		<< std::setw(5) << nUpdated <<  ", " <<
																																												"Added: "			<< std::setw(5) << nAdded <<  ", " <<
																																												"Deleted: "		<< std::setw(5) << nDeleted << "]\n";
					lRecCountTotal += lRecCountCurrent;
				}

				// We import all or nothing
				CommitTransaction();
			}

			std::cout << "Finished importing RBAC data into database " << m_pMetaDatabase->GetName() << std::endl;
			std::cout << "Records imported..: " << lRecCountTotal << std::endl;
		}
		catch(...)
		{
			if (HasTransaction())
				RollbackTransaction();

			Exception::GenericExceptionHandler(__FILE__, __LINE__, Exception_error);
			std::cout << "Error occurred importing RBAC data, please check log file for details\n";
		}

		G_bImportingRBAC = false;

		return lRecCountTotal;
	}



	// Filter ExportToXML
	//
	std::string	Database::FilterExportToXML(MetaEntityPtr pMetaEntity, const string & strD3MDDBIDFilter)
	{
		if(!strD3MDDBIDFilter.length())return "";
		if(pMetaEntity && pMetaEntity->GetMetaDatabase()->GetAlias() != "D3MDDB")return "";

		std::string strFilter;

		try
		{
			switch(pMetaEntity->GetEntityIdx())
			{
			case D3::D3MDDB_D3MetaDatabase:
				strFilter = "D3MetaDatabase.ID IN " + strD3MDDBIDFilter;
				break;
			case D3MDDB_D3MetaEntity:
				strFilter = "D3MetaEntity.MetaDatabaseID IN " + strD3MDDBIDFilter;
				break;
			case D3MDDB_D3MetaColumn:
				strFilter = "D3MetaColumn.MetaEntityID IN (SELECT D3MetaEntity.ID FROM D3MetaEntity WHERE D3MetaEntity.MetaDatabaseID IN " + strD3MDDBIDFilter + ")";
				break;
			case D3MDDB_D3MetaColumnChoice:
				strFilter = "D3MetaColumnChoice.MetaColumnID IN (SELECT D3MetaColumn.ID FROM D3MetaColumn WHERE D3MetaColumn.MetaEntityID IN (SELECT D3MetaEntity.ID FROM D3MetaEntity WHERE D3MetaEntity.MetaDatabaseID IN " + strD3MDDBIDFilter + "))";
				break;
			case D3MDDB_D3MetaKey:
				strFilter = "D3MetaKey.MetaEntityID IN (SELECT D3MetaEntity.ID FROM D3MetaEntity WHERE D3MetaEntity.MetaDatabaseID IN " + strD3MDDBIDFilter + ")";
				break;
			case D3MDDB_D3MetaKeyColumn:
				strFilter = "D3MetaKeyColumn.MetaKeyID IN (SELECT D3MetaKey.ID FROM D3MetaKey WHERE D3MetaKey.MetaEntityID IN (SELECT D3MetaEntity.ID FROM D3MetaEntity WHERE D3MetaEntity.MetaDatabaseID IN " + strD3MDDBIDFilter + "))";
				break;
			case D3MDDB_D3MetaRelation:
				strFilter = "D3MetaRelation.ParentKeyID IN (SELECT D3MetaKey.ID FROM D3MetaKey WHERE D3MetaKey.MetaEntityID IN (SELECT D3MetaEntity.ID FROM D3MetaEntity WHERE D3MetaEntity.MetaDatabaseID IN " + strD3MDDBIDFilter + "))";
				break;
			case D3MDDB_D3Role:
				break;
			case D3MDDB_D3User:
				break;
			case D3MDDB_D3RoleUser:
				break;
			case D3MDDB_D3Session:
				break;
			case D3MDDB_D3DatabasePermission:
				break;
			case D3MDDB_D3EntityPermission:
				break;
			case D3MDDB_D3ColumnPermission:
				break;
			case D3MDDB_D3RowLevelPermission:
				break;
			}
		}
		catch(...)
		{
			D3::Exception::GenericExceptionHandler(__FILE__, __LINE__);
			return "";
		}

		if(strFilter.length())
			strFilter = " WHERE " + strFilter + " ";

		return strFilter;
	}









	//===========================================================================
	//
	// DatabaseWorkspace implementation
	//
	D3_CLASS_IMPL(DatabaseWorkspace, Object);

	DatabaseWorkspace::~DatabaseWorkspace()
	{
		DatabasePtrMapItr		itrDatabase;
		DatabasePtr					pDB = NULL;

		while (!m_mapDatabase.empty())
		{
			itrDatabase = m_mapDatabase.begin();
			pDB = itrDatabase->second;
			delete pDB;
		}

		if (m_pSession)
			m_pSession->On_DatabaseWorkspaceDestroyed(this);
	}



	// Retrieve the workspace's Database instance for a MetaDatabase. If the instance does not exist,
	// create a connected instance automatically.
	//
	DatabasePtr DatabaseWorkspace::GetDatabase(MetaDatabasePtr pMetaDatabase)
	{
		if (!pMetaDatabase)
			return NULL;

		DatabasePtr		pDB = FindDatabase(pMetaDatabase);

		if (!pDB)
			pDB = pMetaDatabase->CreateInstance(this);

		return pDB;
	}



	// Retrieve the workspace's Database instance for a MetaDatabase. If the instance does not exist,
	// create a connected instance automatically.
	//
	DatabasePtr DatabaseWorkspace::GetDatabase(const std::string & strName)
	{
		MetaDatabasePtr		pMDB = MetaDatabase::GetMetaDatabase(strName);

		if (!pMDB)
			return NULL;

		return GetDatabase(pMDB);
	}



	// Retrieve the workspace's Database instance for a MetaDatabase. Return NULL if the instance does not exist.
	//
	DatabasePtr DatabaseWorkspace::FindDatabase(MetaDatabasePtr pMetaDatabase)
	{
		if (!pMetaDatabase)
			return NULL;

		DatabasePtrMapItr itrDatabase = m_mapDatabase.find(pMetaDatabase);

		if (itrDatabase == m_mapDatabase.end())
			return NULL;

		return itrDatabase->second;
	}



	// Add the newly created database to our map
	//
	void DatabaseWorkspace::On_DatabaseCreated(DatabasePtr pDatabase)
	{
		assert(pDatabase);
		assert(pDatabase->GetDatabaseWorkspace() == this);
		assert(FindDatabase(pDatabase->GetMetaDatabase()) == NULL);

		m_mapDatabase.insert(DatabasePtrMap::value_type(pDatabase->GetMetaDatabase(), pDatabase));
	}



	// Remove the deleted database from our map
	//
	void DatabaseWorkspace::On_DatabaseDeleted(DatabasePtr pDatabase)
	{
		DatabasePtrMapItr	itrDatabase = m_mapDatabase.find(pDatabase->GetMetaDatabase());

		if (itrDatabase != m_mapDatabase.end())
			m_mapDatabase.erase(itrDatabase);
	}



} // end namespace D3
