// MODULE: Session source
//;
//; IMPLEMENTATION CLASS: Database
//;
// ===========================================================

// @@DatatypeInclude
#include "D3Types.h"
// @@End
// @@Includes
#include <time.h>
#include <sstream>

#include "Session.h"
#include "Exception.h"
#include "D3Funcs.h"
#include "Database.h"
#include "Entity.h"
#include "Column.h"
#include "Key.h"
#include "Relation.h"
#include "ResultSet.h"

#include "MonitorFunctions.h"

#include <boost/thread/xtime.hpp>
#include <json/json.h>

using namespace wofuncs;


namespace D3
{
	// During importing of RBAC, updates to RBAC objects are ignored
	bool	G_bImportingRBAC = false;


	// ==========================================================================
	// Role::Features class implementation
	//

	// Implement Features bitmask
	BITS_IMPL(Role, Features);

	// Primitive Flags bitmask values
	PRIMITIVEMASK_IMPL(Role, Features, RunP3,					0x00000001);
	PRIMITIVEMASK_IMPL(Role, Features, RunT3,					0x00000002);
	PRIMITIVEMASK_IMPL(Role, Features, RunV3,					0x00000004);
	PRIMITIVEMASK_IMPL(Role, Features, WHMapView,			0x00000008);
	PRIMITIVEMASK_IMPL(Role, Features, WHMapUpdate,		0x00000010);
	PRIMITIVEMASK_IMPL(Role, Features, ResolveIssues,	0x00000080);


	// ==========================================================================
	// Role::IRSSettings class implementation
	//

	// Implement IRSSettings bitmask
	BITS_IMPL(Role, IRSSettings);

	// Primitive Flags bitmask values
	PRIMITIVEMASK_IMPL(Role, IRSSettings, AllowUndo,						0x00000001);
	PRIMITIVEMASK_IMPL(Role, IRSSettings, PullForwardSmooth,		0x00000002);
	PRIMITIVEMASK_IMPL(Role, IRSSettings, PushBackSmooth,				0x00000004);
	PRIMITIVEMASK_IMPL(Role, IRSSettings, ChangeOIShipFrom,			0x00000008);
	PRIMITIVEMASK_IMPL(Role, IRSSettings, ChangeOIShipTo,				0x00000010);
	PRIMITIVEMASK_IMPL(Role, IRSSettings, ChangeOITransmode,		0x00000020);
	PRIMITIVEMASK_IMPL(Role, IRSSettings, AllowCutSub,					0x00000040);
	PRIMITIVEMASK_IMPL(Role, IRSSettings, AllowRequestSolution,	0x00000080);
	PRIMITIVEMASK_IMPL(Role, IRSSettings, AllowExportOrDownload,0x00000100);


	// ==========================================================================
	// Role::V3ParamSettings class implementation
	//

	// Implement V3ParamSettings bitmask
	BITS_IMPL(Role, V3ParamSettings);

	// Primitive Flags bitmask values
	PRIMITIVEMASK_IMPL(Role, V3ParamSettings, ModifyWarehouseParam,	0x00000001);
	PRIMITIVEMASK_IMPL(Role, V3ParamSettings, ModifyLaneParam,			0x00000002);
	PRIMITIVEMASK_IMPL(Role, V3ParamSettings, ModifyShipmentParam,	0x00000004);


	// ==========================================================================
	// Role::T3ParamSettings class implementation
	//

	// Implement T3ParamSettings bitmask
	BITS_IMPL(Role, T3ParamSettings);

	// Primitive Flags bitmask values
	PRIMITIVEMASK_IMPL(Role, T3ParamSettings, ModifyWarehouseParam,	0x00000001);
	PRIMITIVEMASK_IMPL(Role, T3ParamSettings, ModifyLaneParam,			0x00000002);
	PRIMITIVEMASK_IMPL(Role, T3ParamSettings, ModifyShipmentParam,	0x00000004);


	// ==========================================================================
	// Role::P3ParamSettings class implementation
	//

	// Implement P3ParamSettings bitmask
	BITS_IMPL(Role, P3ParamSettings);

	// Primitive Flags bitmask values
	PRIMITIVEMASK_IMPL(Role, P3ParamSettings, ModifyWarehouseParam,	0x00000001);
	PRIMITIVEMASK_IMPL(Role, P3ParamSettings, ModifyCustomerParam,	0x00000002);
	PRIMITIVEMASK_IMPL(Role, P3ParamSettings, ModifyOrderParam,			0x00000004);
	PRIMITIVEMASK_IMPL(Role, P3ParamSettings, ModifyZoneParam,			0x00000008);






	//===========================================================================
	//
	// Role implementation
	//


	// Statics initialisation
	//
	RolePtr									Role::M_sysadmin = NULL;											//!< The memory only sysadmin role
	RoleID									Role::M_uNextInternalID = ROLE_ID_MAX;				//!< If no internal ID is passed to the constructor, use this and decrement afterwards (used for meta dictionary definition meta obejcts)
	RolePtrMap							Role::M_mapRole;															//!< Holds all Role objects in the system indexed by m_uID

	// Standard D3 Stuff
	//
	D3_CLASS_IMPL(Role, Object);



	Role::Role(const std::string& strName, bool bEnabled, const Features::Mask& features, const IRSSettings::Mask& irssettings, const V3ParamSettings::Mask& v3paramsettings, const T3ParamSettings::Mask& t3paramsettings, const P3ParamSettings::Mask& p3paramsettings)
	: m_uID(M_uNextInternalID--),
		m_strName(strName),
		m_bEnabled(bEnabled),
		m_features(features),
		m_irssettings(irssettings),
		m_v3paramsettings(v3paramsettings),
		m_t3paramsettings(t3paramsettings),
		m_p3paramsettings(p3paramsettings)
	{
		Init();
	}



	Role::Role(D3RolePtr pD3Role)
	: m_uID(pD3Role->GetID()),
		m_strName(pD3Role->GetName()),
		m_bEnabled(pD3Role->GetEnabled()),
		m_features(Features::Mask(pD3Role->GetFeatures())),
		m_irssettings(IRSSettings::Mask(pD3Role->GetIRSSettings())),
		m_v3paramsettings(V3ParamSettings::Mask(pD3Role->GetV3ParamSettings())),
		m_t3paramsettings(T3ParamSettings::Mask(pD3Role->GetT3ParamSettings())),
		m_p3paramsettings(P3ParamSettings::Mask(pD3Role->GetP3ParamSettings()))
	{
		Init(pD3Role);
	}



	void Role::Init(D3RolePtr pD3Role)
	{
		if (m_strName == D3_SYSADMIN_ROLE)
		{
			if (M_sysadmin)
				throw std::runtime_error("Role::Init(): " D3_SYSADMIN_ROLE " is not a valid role name. ");

			M_sysadmin = this;
		}

		if (pD3Role)
		{
			D3Role::D3DatabasePermissions::iterator		itrD3DatabasePermissions;
			D3Role::D3EntityPermissions::iterator			itrD3EntityPermissions;
			D3Role::D3ColumnPermissions::iterator			itrD3ColumnPermissions;


			// Load all DatabasePermissions for this role
			for ( itrD3DatabasePermissions =  pD3Role->GetD3DatabasePermissions()->begin();
						itrD3DatabasePermissions != pD3Role->GetD3DatabasePermissions()->end();
						itrD3DatabasePermissions++)
			{
				this->SetPermissions(*itrD3DatabasePermissions);
			}

			// Load all EntityPermissions for this role
			for ( itrD3EntityPermissions =  pD3Role->GetD3EntityPermissions()->begin();
						itrD3EntityPermissions != pD3Role->GetD3EntityPermissions()->end();
						itrD3EntityPermissions++)
			{
				this->SetPermissions(*itrD3EntityPermissions);
			}

			// Load all ColumnPermissions for this role
			for ( itrD3ColumnPermissions =  pD3Role->GetD3ColumnPermissions()->begin();
						itrD3ColumnPermissions != pD3Role->GetD3ColumnPermissions()->end();
						itrD3ColumnPermissions++)
			{
				this->SetPermissions(*itrD3ColumnPermissions);
			}
		}

		if (this != M_sysadmin)
		{
			// restrict access to D3HSDB
			MetaDatabasePtr		pHSDB = MetaDatabase::GetMetaDatabase("D3HSDB");

			if (pHSDB)
				SetPermissions(pHSDB, MetaDatabase::Permissions::Read);
		}

		assert(M_mapRole.find(m_uID) == M_mapRole.end());
		M_mapRole[m_uID] = this;
	}



	// The destructor deletes all attached RoleUsers and Permissions
	Role::~Role()
	{
		RoleUserPtr				pRoleUser;

		M_mapRole.erase(m_uID);

		while (!m_listRoleUsers.empty())
		{
			pRoleUser = m_listRoleUsers.front();
			delete pRoleUser;
		}

		if (M_sysadmin == this)
			M_sysadmin = NULL;
	}



	// Delete all Role objects (does not save active sessions)
	/* static */
	void Role::DeleteAll()
	{
		RolePtr		pRole;

		while(!M_mapRole.empty())
		{
			pRole = M_mapRole.begin()->second;
			pRole->DeleteRoleUsers();
			delete pRole;
		}
	}



	/* static */
	void Role::SaveAndDeleteRoleUsers()
	{
		RolePtr		pRole;

		while(!M_mapRole.empty())
		{
			pRole = M_mapRole.begin()->second;
			pRole->SaveRoleUsers();
			pRole->DeleteRoleUsers();
			delete pRole;
		}
	}



	// we delete the sessions
	void Role::SaveRoleUsers()
	{
		RoleUserPtrListItr		itrRU;
		RoleUserPtr						pRU;

		for (itrRU =  m_listRoleUsers.begin();
		     itrRU != m_listRoleUsers.end();
		     itrRU++)
		{
			pRU = *itrRU;
			pRU->SaveSessions();
		}
	}




	void Role::DeleteRoleUsers()
	{
		RoleUserPtr						pRU;

		while(!m_listRoleUsers.empty())
		{
			pRU = m_listRoleUsers.front();
			pRU->DeleteSessions();
			delete pRU;
		}
	}



	//! Makes sure D3MDDB is read only for none sysadmin roles
	void Role::SetPermissions(D3DatabasePermissionPtr	pD3DatabasePermissions)
	{
		if (pD3DatabasePermissions->GetD3MetaDatabase()->GetAlias() == "D3HSDB" && m_strName != D3_SYSADMIN_ROLE)
			m_mapMDPermissions[pD3DatabasePermissions->GetMetaDatabaseID()] = MetaDatabase::Permissions::Read;
		else
			m_mapMDPermissions[pD3DatabasePermissions->GetMetaDatabaseID()] = MetaDatabase::Permissions::Mask(pD3DatabasePermissions->GetAccessRights());
	}



	//! Makes sure D3MDDB is read only for none sysadmin roles
	void Role::SetPermissions(MetaDatabasePtr pMD, MetaDatabase::Permissions::Mask mask)
	{
		if (pMD->GetAlias() == "D3HSDB" && m_strName != D3_SYSADMIN_ROLE)
			m_mapMDPermissions[pMD->GetID()] = MetaDatabase::Permissions::Read;
		else
			m_mapMDPermissions[pMD->GetID()] = mask;
	}



	void Role::DeletePermissions(D3DatabasePermissionPtr pD3DatabasePermissions)
	{
		MonitoredLocker								lk(m_mtxExcl, "Role::DeletePermissions()");

		if (pD3DatabasePermissions->GetD3MetaDatabase()->GetAlias() != "D3HSDB")
		{
			MetaDatabasePermissionsMapItr itr = m_mapMDPermissions.find(pD3DatabasePermissions->GetMetaDatabaseID());

			if (itr != m_mapMDPermissions.end())
				m_mapMDPermissions.erase(itr);
		}
	}



	void Role::DeletePermissions(D3EntityPermissionPtr pD3EntityPermissions)
	{
		MonitoredLocker							lk(m_mtxExcl, "Role::DeletePermissions()");
		MetaEntityPermissionsMapItr itr = m_mapMEPermissions.find(pD3EntityPermissions->GetMetaEntityID());

		if (itr != m_mapMEPermissions.end())
			m_mapMEPermissions.erase(itr);
	}



	void Role::DeletePermissions(D3ColumnPermissionPtr pD3ColumnPermissions)
	{
		MonitoredLocker							lk(m_mtxExcl, "Role::DeletePermissions()");
		MetaColumnPermissionsMapItr itr = m_mapMCPermissions.find(pD3ColumnPermissions->GetMetaColumnID());

		if (itr != m_mapMCPermissions.end())
			m_mapMCPermissions.erase(itr);
	}



	void Role::AddDefaultPermissions()
	{
		if (this != M_sysadmin && this->m_strName != D3_ADMIN_ROLE)
			SetPermissions(MetaDatabase::GetMetaDatabase("D3MDDB"), MetaDatabase::Permissions::Read);
	}



	MetaDatabase::Permissions Role::GetPermissions(MetaDatabasePtr pMD)
	{
		MonitoredLocker								lk(m_mtxExcl, "Role::GetPermissions()");
		MetaDatabase::Permissions			objDatabaseRights(MetaDatabase::Permissions::Mask(0));
		MetaDatabasePermissionsMapItr itr = m_mapMDPermissions.find(pMD->GetID());


		if (itr != m_mapMDPermissions.end())
			objDatabaseRights = itr->second;

		return objDatabaseRights & pMD->GetPermissions();
	}



	MetaEntity::Permissions Role::GetPermissions(MetaEntityPtr pME)
	{
		MonitoredLocker								lk(m_mtxExcl, "Role::GetPermissions()");
		MetaDatabase::Permissions			objDatabaseRights(GetPermissions(pME->GetMetaDatabase()));
		MetaEntity::Permissions				objEntityRights(MetaEntity::Permissions::Mask(0xFFFF));
		MetaEntityPermissionsMapItr		itr = m_mapMEPermissions.find(pME->GetID());


		if (itr != m_mapMEPermissions.end())
			objEntityRights = itr->second;

		// No database Read permission?
		if (!(objDatabaseRights & MetaDatabase::Permissions::Read))
			objEntityRights &= ~MetaEntity::Permissions::Select;

		// No database Write permission?
		if (!(objDatabaseRights & MetaDatabase::Permissions::Write))
			objEntityRights &= ~(MetaEntity::Permissions::Insert | MetaEntity::Permissions::Update | MetaEntity::Permissions::Delete);

		// TMS stuff is read only for non-sysadmin
		static MetaDatabasePtr			s_pAPAL3MDB = MetaDatabase::GetMetaDatabase("APAL3DB");
		static MetaEntityPtr				s_pMETaskType = s_pAPAL3MDB ? s_pAPAL3MDB->GetMetaEntity("APAL3TaskType") : NULL;
		static MetaEntityPtr				s_pMETaskActivityType = s_pAPAL3MDB ? s_pAPAL3MDB->GetMetaEntity("APAL3TaskActivityType") : NULL;
		static MetaEntityPtr				s_pMEActivityType = s_pAPAL3MDB ? s_pAPAL3MDB->GetMetaEntity("APAL3ActivityType") : NULL;
		static MetaEntityPtr				s_pMEVLBUsage = s_pAPAL3MDB ? s_pAPAL3MDB->GetMetaEntity("VLBUsage") : NULL;

		// Generic Importer stuff is read only for non-sysadmin
		static MetaEntityPtr				s_pMEImporter = s_pAPAL3MDB ? s_pAPAL3MDB->GetMetaEntity("AP3Importer") : NULL;
		static MetaEntityPtr				s_pMEImporterColumnDetail = s_pAPAL3MDB ? s_pAPAL3MDB->GetMetaEntity("AP3ImporterColumnDetail") : NULL;

		if (this != M_sysadmin)
		{
			if (pME == s_pMEVLBUsage)
			{
				// non-sysadmin can't even see VLBUsage
				objEntityRights &= MetaEntity::Permissions::Mask(0);
			}
			else
			{
				if (pME == s_pMETaskType || pME == s_pMETaskActivityType || pME == s_pMEActivityType || pME == s_pMEImporter || pME == s_pMEImporterColumnDetail)
					objEntityRights &= MetaEntity::Permissions::Select;
			}
		}

		return objEntityRights & pME->GetPermissions();
	}



	MetaColumn::Permissions Role::GetPermissions(MetaColumnPtr pMC)
	{
		if (pMC->IsDerived())
			return pMC->GetPermissions();

		MonitoredLocker								lk(m_mtxExcl, "Role::GetPermissions()");
		MetaEntity::Permissions				objEntityRights(GetPermissions(pMC->GetMetaEntity()));
		MetaColumn::Permissions				objColumnRights(MetaColumn::Permissions::Mask(0xFFFF));
		MetaColumnPermissionsMapItr		itr = m_mapMCPermissions.find(pMC->GetID());


		if (itr != m_mapMCPermissions.end())
			objColumnRights = itr->second;

		// No Entity Select permission?
		if (!(objEntityRights & MetaEntity::Permissions::Select))
			objColumnRights &= ~MetaColumn::Permissions::Read;

		// No Entity Update permission?
		if (!(objEntityRights & MetaEntity::Permissions::Update))
			objColumnRights &= ~MetaColumn::Permissions::Write;

		return objColumnRights & pMC->GetPermissions();
	}



	bool Role::CanRead(MetaKeyPtr pMK)
	{
		MonitoredLocker					lk(m_mtxExcl, "Role::CanRead()");
		MetaColumnPtrListItr		itr;


		for ( itr =  pMK->GetMetaColumns()->begin();
					itr != pMK->GetMetaColumns()->end();
					itr++)
		{
			if (!(GetPermissions(*itr) & MetaColumn::Permissions::Read))
				return false;
		}

		return true;
	}



	bool Role::CanWrite(MetaKeyPtr pMK)
	{
		MonitoredLocker					lk(m_mtxExcl, "Role::CanWrite()");
		MetaColumnPtrListItr		itr;


		for ( itr =  pMK->GetMetaColumns()->begin();
					itr != pMK->GetMetaColumns()->end();
					itr++)
		{
			if (!(GetPermissions(*itr) & MetaColumn::Permissions::Write))
				return false;
		}

		return true;
	}



	bool Role::CanReadParent(MetaRelationPtr pMR)
	{
		// Must be able to read the parent key
		if (CanRead(pMR->GetParentMetaKey()))
		{
			MetaKeyPtr						pMK = pMR->GetChildMetaKey();
			unsigned int					iSharedCols = pMR->GetParentMetaKey()->GetColumnCount();
			MetaColumnPtrListItr	itr;

			// Must be able to read the columns in the child key that overlap with the parent key
			for ( itr =  pMK->GetMetaColumns()->begin();
						itr != pMK->GetMetaColumns()->end() &&	iSharedCols;
						itr++,																	iSharedCols--)
			{
				if (!(GetPermissions(*itr) & MetaColumn::Permissions::Read))
					return false;
			}

			return true;
		}

		return false;
	}



	bool Role::CanChangeParent(MetaRelationPtr pMR)
	{
		// Must be able to read the parent key
		if (CanRead(pMR->GetParentMetaKey()))
		{
			MetaKeyPtr						pMK = pMR->GetChildMetaKey();
			unsigned int					iSharedCols = pMR->GetParentMetaKey()->GetColumnCount();
			MetaColumnPtrListItr	itr;

			// Must be able to write at least one of the columns in the child key that overlap with the parent key
			for ( itr =  pMK->GetMetaColumns()->begin();
						itr != pMK->GetMetaColumns()->end() &&	iSharedCols;
						itr++,																	iSharedCols--)
			{
				if (GetPermissions(*itr) & MetaColumn::Permissions::Write)
					return true;
			}

			return false;
		}

		return false;
	}



	void Role::Refresh(D3RolePtr pD3Role)
	{
		assert(m_uID == pD3Role->GetID());
		m_strName = pD3Role->GetName();
		m_bEnabled = pD3Role->GetEnabled();
		m_features = Features::Mask(pD3Role->GetFeatures());
		m_irssettings = IRSSettings::Mask(pD3Role->GetIRSSettings());
		m_v3paramsettings = V3ParamSettings::Mask(pD3Role->GetV3ParamSettings());
		m_t3paramsettings = T3ParamSettings::Mask(pD3Role->GetT3ParamSettings());
		m_p3paramsettings = P3ParamSettings::Mask(pD3Role->GetP3ParamSettings());
	}



	D3RolePtr Role::GetD3Role(DatabaseWorkspace& dbWS)
	{
		DatabasePtr		pDB = dbWS.GetDatabase("D3MDDB");
		return D3Role::Load(pDB, m_uID);
	}


	void Role::On_RoleUserCreated(RoleUserPtr	pRoleUser)
	{
		assert(pRoleUser->GetRole() == this);
		m_listRoleUsers.push_back(pRoleUser);
	}


	void Role::On_RoleUserDeleted(RoleUserPtr	pRoleUser)
	{
		assert(pRoleUser->GetRole() == this);
		m_listRoleUsers.remove(pRoleUser);
	}


	std::ostream & Role::AsJSON(std::ostream & ostrm)
	{
		ostrm << '{';
		ostrm << "\"ID\":"							<< m_uID																		<< ",";
		ostrm << "\"Name\":\""					<< m_strName																<< "\",";
		ostrm << "\"Enabled\":"					<< (m_bEnabled ? "true" : "false")					<< ",";
		ostrm << "\"Features\":"				<< this->m_features.m_bitset.m_value				<< ",";
		ostrm << "\"IRSSettings\":"			<< this->m_irssettings.m_bitset.m_value			<< ",";
		ostrm << "\"V3ParamSettings\":"	<< this->m_v3paramsettings.m_bitset.m_value	<< ",";
		ostrm << "\"T3ParamSettings\":"	<< this->m_t3paramsettings.m_bitset.m_value	<< ",";
		ostrm << "\"P3ParamSettings\":"	<< this->m_p3paramsettings.m_bitset.m_value;
		ostrm << '}';

		return ostrm;
	}


	/* static */
	std::ostream & Role::RolesAsJSON(std::ostream & ostrm)
	{
		RolePtrMapItr			itr;
		bool							bFirst = true;

		ostrm << '[';

		for ( itr =  M_mapRole.begin();
					itr != M_mapRole.end();
					itr++)
		{
			if (bFirst)
				bFirst = false;
			else
				ostrm << ',';

			itr->second->AsJSON(ostrm);
		}

		ostrm << ']';

		return ostrm;
	}





	//===========================================================================
	//
	// User implementation
	//


	// Statics initialisation
	//
	UserPtr									User::M_sysadmin = NULL;											//!< Holds all User objects in the system indexed by m_uID
	UserID									User::M_uNextInternalID = USER_ID_MAX;				//!< If no internal ID is passed to the constructor, use this and decrement afterwards (used for meta dictionary definition meta obejcts)
	UserPtrMap							User::M_mapUser;															//!< Holds all User objects in the system indexed by m_uID

	// Standard D3 Stuff
	//
	D3_CLASS_IMPL(User, Object);



	User::RowAccessRestrictions::RowAccessRestrictions(MetaEntityPtr pMetaEntity, D3RowLevelPermissionPtr pRowLevelPermission)
	{
		Json::Reader						jsonReader;
		Json::Value							nRoot;

		switch (pRowLevelPermission->GetAccessRights())
		{
			case 1:
				m_type = Allow;
				break;

			case 2:
				m_type = Deny;
				break;

			default:
				throw Exception(__FILE__, __LINE__, Exception_error, "User::RowAccessRestrictions::RowAccessRestrictions(): RowLevelPermission (MetaEntityID=%i/UserID=%i) specifies incorrect AccessRights, settings ignored!", pRowLevelPermission->GetMetaEntityID(), pRowLevelPermission->GetUserID());
		}

		if (!pRowLevelPermission->Column(D3RowLevelPermission_PKValueList)->IsNull() && !pRowLevelPermission->GetPKValueList().empty())
		{
			if (!jsonReader.parse(pRowLevelPermission->GetPKValueList(), nRoot))
				throw Exception(__FILE__, __LINE__, Exception_error, "User::RowAccessRestrictions::RowAccessRestrictions(): Parse error parsing RowLevelPermission (MetaEntityID=%i/UserID=%i)\nPKValueList:\n%s\n\nParser message:\n%s", pRowLevelPermission->GetMetaEntityID(), pRowLevelPermission->GetUserID(), pRowLevelPermission->GetPKValueList().c_str(), jsonReader.getFormatedErrorMessages().c_str());

			if (!nRoot.isArray())
				throw Exception(__FILE__, __LINE__, Exception_error, "User::RowAccessRestrictions::RowAccessRestrictions(): RowLevelPermission (MetaEntityID=%i/UserID=%i) does not hold a Json Array in PKValueList, settings ignored!", pRowLevelPermission->GetMetaEntityID(), pRowLevelPermission->GetUserID());
			{
				for (Json::Value::UInt iKeys=0; iKeys < nRoot.size(); iKeys++)
				{
					Json::Value&					keyValues(nRoot[iKeys]);
					Json::Value::UInt			iColumns = 0;
					ColumnPtrListItr			itr;
					TemporaryKeyPtr				pTempKey = new TemporaryKey(*(pMetaEntity->GetPrimaryMetaKey()));

					// keyValues must be an array
					if (!keyValues.isArray())
						throw Exception(__FILE__, __LINE__, Exception_error, "User::RowAccessRestrictions::RowAccessRestrictions(): RowLevelPermission (MetaEntityID=%i/UserID=%i) the Json Array in PKValueList has a value that is not an array, settings ignored!", pRowLevelPermission->GetMetaEntityID(), pRowLevelPermission->GetUserID());

					// ... and have as many elements as our pMetaEntity's primary key has columns
					if (keyValues.size() != pMetaEntity->GetPrimaryMetaKey()->GetColumnCount())
						throw Exception(__FILE__, __LINE__, Exception_error, "User::RowAccessRestrictions::RowAccessRestrictions(): RowLevelPermission (MetaEntityID=%i/UserID=%i) the Json Array in PKValueList has key value array with too many or too few values, settings ignored!", pRowLevelPermission->GetMetaEntityID(), pRowLevelPermission->GetUserID());

					for (	itr  = pTempKey->GetColumns().begin();
								itr != pTempKey->GetColumns().end();
								itr++)
					{
						ColumnPtr			pCol = *itr;
						Json::Value&	columnValue(keyValues[iColumns]);
						if (!pCol->SetValue(columnValue))
						{
							delete pTempKey;
							pTempKey = NULL;
							ReportError("User::RowAccessRestrictions::ctor(): PKValueList for D3MetaEntity '%s' and user '%s' has incorrect key at index [%i] (PKValueList='%s'). Key ignored.", pMetaEntity->GetName().c_str(), pRowLevelPermission->GetD3User()->GetName().c_str(), iKeys, pRowLevelPermission->GetPKValueList().c_str());
							break;
						}
						iColumns++;
					}

					if (pTempKey)
						m_listTempKeys.push_back(pTempKey);
				}
			}
		}
	}



	User::User(const std::string& strName, const Data& password, bool bEnabled)
	:	m_uID(M_uNextInternalID--),
		m_strName(strName),
		m_Password(password),
		m_bEnabled(bEnabled),
		m_iPWDAttempts(0),
		m_dtPWDExpires(D3Date(D3Date::MaxISO8601DateTime)),
		m_bTemporary(false)
	{
		Init();
	}



	User::User(D3UserPtr pD3User)
	:	m_uID(pD3User->GetID()),
		m_strName(pD3User->GetName()),
		m_Password(pD3User->GetPassword()),
		m_bEnabled(pD3User->GetEnabled()),
		m_strWarehouseAreaAccessList(pD3User->GetWarehouseAreaAccessList()),
		m_strCustomerAccessList(pD3User->GetCustomerAccessList()),
		m_strTransmodeAccessList(pD3User->GetTransmodeAccessList()),
		m_iPWDAttempts(pD3User->GetPWDAttempts()),
		m_dtPWDExpires(pD3User->GetPWDExpires()),
		m_bTemporary(pD3User->GetTemporary()),
		m_strLanguage(pD3User->GetLanguage())
	{
		Init(pD3User);
	}



	void User::Init(D3UserPtr pD3User)
	{
		if (m_strName.find("SYSTEM") == 0)
			throw std::runtime_error("User::Init(): User name SYSTEM or a user names staring with SYSTEM are reserved.");

		if (m_strName == D3_SYSADMIN_USER)
		{
			if (M_sysadmin)
				throw std::runtime_error("User::Init(): " D3_SYSADMIN_USER " is not a valid user name. ");

			M_sysadmin = this;
		}

		// make sure
		if (m_dtPWDExpires.is_special())
			m_dtPWDExpires = D3Date(D3Date::MaxISO8601DateTime);

		assert(M_mapUser.find(m_uID) == M_mapUser.end());
		M_mapUser[m_uID] = this;
	}



	// The destructor deletes all attached RoleUsers
	User::~User()
	{
		RoleUserPtr								pRoleUser;

		M_mapUser.erase(m_uID);

		while (!m_listRoleUsers.empty())
		{
			pRoleUser = m_listRoleUsers.front();
			delete pRoleUser;
		}

		DropRowLevelSecurity();

		if (M_sysadmin == this)
			M_sysadmin = NULL;
	}



	// Delete all User objects
	/* static */
	void User::DeleteAll()
	{
		UserPtr		pUser;

		while(!M_mapUser.empty())
		{
			pUser = M_mapUser.begin()->second;
			pUser->DeleteRoleUsers();
			delete pUser;
		}
	}



	/* static */
	void User::SaveAndDeleteRoleUsers()
	{
		UserPtr		pUser;

		while(!M_mapUser.empty())
		{
			pUser = M_mapUser.begin()->second;
			pUser->SaveRoleUsers();
			pUser->DeleteRoleUsers();
			delete pUser;
		}
	}



	// we delete the sessions
	void User::SaveRoleUsers()
	{
		RoleUserPtrListItr		itrRU;
		RoleUserPtr						pRU;

		for (itrRU =  m_listRoleUsers.begin();
		     itrRU != m_listRoleUsers.end();
		     itrRU++)
		{
			pRU = *itrRU;
			pRU->SaveSessions();
		}
	}




	void User::DeleteRoleUsers()
	{
		RoleUserPtr						pRU;

		while(!m_listRoleUsers.empty())
		{
			pRU = m_listRoleUsers.front();
			pRU->DeleteSessions();
			delete pRU;
		}
	}



	//! Delete the users pending sessions
	void User::DeleteSessions()
	{
		RoleUserPtrListItr		itrRU;
		RoleUserPtr						pRU;
		
		for (itrRU =  m_listRoleUsers.begin();
		     itrRU != m_listRoleUsers.end();
		     itrRU++)
		{
			pRU = *itrRU;
			pRU->DeleteSessions();
		}
	}



	/* static */
	UserPtr User::GetUser(const std::string& strName, const std::string& strBase64PWD)
	{
		UserPtr					pUser = NULL;
		UserPtrMapItr		itr;
		Data						password;
		bool						bDisableUser = false;


		password.base64_decode(strBase64PWD);

		for ( itr =  M_mapUser.begin();
					itr != M_mapUser.end();
					itr++)
		{
			pUser = itr->second;

			if (pUser->GetName() == strName)
			{
				if (strName == D3_SYSADMIN_USER)
				{
					if (pUser->GetPassword() == password)
						return pUser;

					pUser = NULL;
					break;
				}
				else
				{
					DatabaseWorkspace		dbWS;
					DatabasePtr					pDB = dbWS.GetDatabase("D3MDDB");
					D3UserPtr						pD3User = D3User::LoadByName(pDB, strName);
					int									maxRetries = Settings::Singleton().MaxPasswordRetries();

					// too many attempts?
					if (maxRetries > 0 && maxRetries <= pD3User->GetPWDAttempts())
						throw std::runtime_error("too many failed login attempts");

					// we need to set the users PWDAttempts and update user
					if (pUser->GetPassword() == password)
					{
						// password expired?
						if (pD3User->GetPWDExpires() < D3Date())
							throw std::runtime_error("password expired");

						// locked?
						if (!pD3User->GetEnabled())
							throw std::runtime_error("account locked");

						pD3User->SetPWDAttempts(0);
					}
					else
					{
						pD3User->SetPWDAttempts(pD3User->GetPWDAttempts() + 1);

						if (maxRetries > 0 && maxRetries <= pD3User->GetPWDAttempts())
							bDisableUser = true;
						else
							pUser = NULL;
					}

					try
					{
						pDB->BeginTransaction();
						pD3User->Update(pDB);
						pDB->CommitTransaction();
					}
					catch (...)
					{
						if (pDB->HasTransaction())
							pDB->RollbackTransaction();

						std::string		strMsg = D3::ShortGenericExceptionHandler();
						throw std::runtime_error(strMsg);
					}

					break;
				}
			}

			pUser = NULL;
		}

		if (bDisableUser)
		{
			// we need to destroy all current sessions
			pUser->SaveAndDeleteSessions();
			throw std::runtime_error("too many failed login attempts");
		}

		if (!pUser)
			throw std::runtime_error("invalid user or password");

		return pUser;
	}






	/* static */
	UserPtr User::UpdateUserPassword(const std::string& strName, const std::string& strCrntBase64PWD, const std::string& strNewBase64PWD)
	{
		UserPtr		pUser = GetUser(strName, strCrntBase64PWD);

		if (pUser->GetName() == D3_SYSADMIN_USER)
		{
			// let's not give away the fact that we have a sysadmin user
			throw std::runtime_error("insufficient privileges to change password");
		}
		else
		{
			// This block is executed even if the old and new passwords are the same
			DatabaseWorkspace		dbWS;
			DatabasePtr					pDB = dbWS.GetDatabase("D3MDDB");

			if (strNewBase64PWD.empty())
				throw std::runtime_error("new password must not be empty");

			try
			{
				D3UserPtr						pD3User = D3User::LoadByName(pDB, strName);
				Json::Value					jsonValues;
				Json::Reader				jsonReader;
				std::ostringstream	strm;

				// build json
				strm << "{\"Password\":\"" << strNewBase64PWD << "\"}";

				// Parse json
				jsonReader.parse(strm.str(), jsonValues);

				// Set password then update the user
				pDB->BeginTransaction();
				pD3User->SetValues(NULL, jsonValues);
				pD3User->Update(pDB);
				pDB->CommitTransaction();
			}
			catch (...)
			{
				if (pDB->HasTransaction())
					pDB->RollbackTransaction();

				std::string		strMsg = D3::ShortGenericExceptionHandler();
				throw std::runtime_error(strMsg);
			}
		}

		return pUser;
	}



	void User::Refresh(D3UserPtr pD3User)
	{
		assert(m_uID == pD3User->GetID());

		m_strName = pD3User->GetName();
		m_Password = pD3User->GetPassword();
		m_bEnabled = pD3User->GetEnabled();
		m_strWarehouseAreaAccessList = pD3User->GetWarehouseAreaAccessList();
		m_strCustomerAccessList = pD3User->GetCustomerAccessList();
		m_strTransmodeAccessList = pD3User->GetTransmodeAccessList();
		m_iPWDAttempts = pD3User->GetPWDAttempts();
		m_dtPWDExpires = pD3User->GetPWDExpires();
		m_bTemporary = pD3User->GetTemporary();
		m_strLanguage = pD3User->GetLanguage();

		if (m_dtPWDExpires.is_special())
			m_dtPWDExpires = D3Date(D3Date::MaxISO8601DateTime);

		m_bTemporary = pD3User->GetTemporary();
	}



	D3UserPtr User::GetD3User(DatabaseWorkspace& dbWS)
	{
		DatabasePtr		pDB = dbWS.GetDatabase("D3MDDB");
		return D3User::Load(pDB, m_uID);
	}


	void User::On_RoleUserCreated(RoleUserPtr	pRoleUser)
	{
		assert(pRoleUser->GetUser() == this);
		m_listRoleUsers.push_back(pRoleUser);
	}



	void User::On_RoleUserDeleted(RoleUserPtr	pRoleUser)
	{
		assert(pRoleUser->GetUser() == this);
		m_listRoleUsers.remove(pRoleUser);
	}



	std::ostream & User::AsJSON(std::ostream & ostrm)
	{
		ostrm << '{';
		ostrm << "\"ID\":"												<< m_uID														<< ',';
		ostrm << "\"Name\":\""										<< m_strName												<< "\",";
		ostrm << "\"Password\":\""								<< m_Password.base64_encode()				<< "\",";
		ostrm << "\"Enabled\":"										<< (m_bEnabled ? "true" : "false")	<< ',';
		ostrm << "\"WarehouseAreaAccessList\":\""	<< m_strWarehouseAreaAccessList			<< "\",";
		ostrm << "\"CustomerAccessList\":\""			<< m_strCustomerAccessList					<< "\",";
		ostrm << "\"TransmodeAccessList\":\""			<< m_strTransmodeAccessList					<< "\",";
		ostrm << "\"PWDAttempts\":"								<< m_iPWDAttempts										<< ',';

		if (m_dtPWDExpires.is_special())
			ostrm << "\"PWDExpires\":null,";
		else
			ostrm << "\"PWDExpires\":\""						<< m_dtPWDExpires.AsISOString()			<< "\",";

		ostrm << "\"Temporary\":"									<< (m_bTemporary ? "true" : "false")<< ',';
		ostrm << "\"Language\":\""								<< m_strLanguage										<< '"';
		ostrm << '}';

		return ostrm;
	}



	/* static */
	std::ostream & User::UsersAsJSON(std::ostream & ostrm)
	{
		UserPtrMapItr			itr;
		bool							bFirst = true;

		ostrm << '[';

		for ( itr =  M_mapUser.begin();
					itr != M_mapUser.end();
					itr++)
		{
			if (bFirst)
				bFirst = false;
			else
				ostrm << ',';

			itr->second->AsJSON(ostrm);
		}

		ostrm << ']';

		return ostrm;
	}



	void User::SetPermissions(D3RowLevelPermissionPtr	pD3RowLevelPermissions)
	{
		MonitoredLocker						lk(m_mtxExcl, "User::SetPermissions()");
		MetaEntityPtr							pME;
		RowLevelAccessPtrMapItr		itr;

		m_mapRLSPredicates.clear();

		pME = MetaEntity::GetMetaEntityByID(pD3RowLevelPermissions->GetMetaEntityID());

		// If this is a row level security added for a meta dictionary object that has been added
		// during the session, we can't (nor should we) action on it
		if (!pME)
			return;

		itr = m_mapRowAccessRestrictions.find(pME);

		if (itr != m_mapRowAccessRestrictions.end())
		{
			delete itr->second;
			m_mapRowAccessRestrictions.erase(itr);
		}

		// A new one has been added
		m_mapRowAccessRestrictions[pME] = new RowAccessRestrictions(pME, pD3RowLevelPermissions);
	}



	void User::DeletePermissions(D3RowLevelPermissionPtr	pD3RowLevelPermissions)
	{
		MonitoredLocker						lk(m_mtxExcl, "User::DeletePermissions()");
		MetaEntityPtr							pME;
		RowLevelAccessPtrMapItr		itr;

		m_mapRLSPredicates.clear();

		pME = MetaEntity::GetMetaEntityByID(pD3RowLevelPermissions->GetMetaEntityID());
		itr = m_mapRowAccessRestrictions.find(pME);

		if (itr != m_mapRowAccessRestrictions.end())
		{
			// An existing one has been deleted
			delete itr->second;
			m_mapRowAccessRestrictions.erase(itr);
		}
	}



#	define SQL_COLUMN_SEPARATOR_STRING "/#/"

	std::string& User::GetRLSPredicate(MetaEntityPtr pMetaEntity)
	{
		MonitoredLocker						lk(m_mtxExcl, "User::GetRLSPredicate()");

		if (m_mapRLSPredicates.empty())
			CreateRLSPredicates();

		return m_mapRLSPredicates[pMetaEntity];
	}



	void User::CreateRLSPredicates()
	{
		RLSPredicateMap						mapPrimitiveRLSPredicates;
		MetaDatabasePtrMapItr			itrMD;
		MetaDatabasePtr						pMD;
		MetaEntityPtrListItr			itrME;
		MetaEntityPtr							pME;
		MetaRelationPtrVectItr		itrPMR;
		MetaRelationPtr						pPMR;
		MetaEntityPtr							pMEParent;
		MetaColumnPtrListItr			itrPMC, itrCMC;
		MetaColumnPtr							pPMC, pCMC;
		RowLevelAccessPtrMapItr		itrRLS;
		std::string								strPredicate;
		bool											bFirst;


		assert(m_mapRLSPredicates.empty());
		CreatePrimitiveRLSPredicates(mapPrimitiveRLSPredicates);

		for ( itrMD =  MetaDatabase::GetMetaDatabases().begin();
					itrMD != MetaDatabase::GetMetaDatabases().end();
					itrMD++)
		{
			pMD = itrMD->second;
			MetaEntityPtrList&	listME = pMD->GetDependencyOrderedMetaEntities();

			for ( itrME =  listME.begin();
						itrME != listME.end();
						itrME++)
			{
				pME = *itrME;

				strPredicate.clear();
				bFirst = true;

				// Let's build the predicate
				for ( itrPMR =  pME->GetParentMetaRelations()->begin();
							itrPMR != pME->GetParentMetaRelations()->end();
							itrPMR++)
				{
					pPMR = *itrPMR;
					pMEParent = pPMR->GetParentMetaKey()->GetMetaEntity();

					if (m_mapRLSPredicates.find(pMEParent) != m_mapRLSPredicates.end() && !m_mapRLSPredicates[pMEParent].empty())
					{
						std::string			strThisKeyNull;
						std::string			strParentSelect;

						if (bFirst)
							bFirst = false;
						else
							strPredicate += " AND ";

						// We need to iterate the key columns
						for ( itrPMC =  pPMR->GetParentMetaKey()->GetMetaColumns()->begin(),	itrCMC =  pPMR->GetChildMetaKey()->GetMetaColumns()->begin();
									itrPMC != pPMR->GetParentMetaKey()->GetMetaColumns()->end();
									itrPMC++, itrCMC++)
						{
							pPMC = *itrPMC;
							pCMC = *itrCMC;

							if (itrPMC != pPMR->GetParentMetaKey()->GetMetaColumns()->begin())
							{
								strParentSelect += "+'" SQL_COLUMN_SEPARATOR_STRING "'+";
								strPredicate += "+'" SQL_COLUMN_SEPARATOR_STRING "'+";
								strThisKeyNull += " AND ";
							}
							else
							{
								strPredicate += "(";
							}

							strThisKeyNull += pCMC->GetName();
							strThisKeyNull += " IS NULL";

							if (pPMR->GetParentMetaKey()->GetColumnCount() == 1 && pPMC->GetType() == pCMC->GetType())
							{
								strParentSelect += pPMC->GetName();
								strPredicate += pCMC->GetName();
							}
							else
							{
								strParentSelect += "CAST(";
								strParentSelect += pPMC->GetName();
								strParentSelect += " AS varchar(20))";
								strPredicate += "CAST(";
								strPredicate += pCMC->GetName();
								strPredicate += " AS varchar(20))";
							}
						}

						// Complete parent select
						strPredicate += " IN (SELECT ";
						strPredicate += strParentSelect;
						strPredicate += " FROM ";
						strPredicate += pMEParent->GetName();
						strPredicate += " WHERE ";
						strPredicate += m_mapRLSPredicates[pMEParent];

						if (pPMR->GetParentMetaKey()->GetColumnCount() == 1)
						{
							strPredicate += ") OR ";
							strPredicate += strThisKeyNull;
							strPredicate += ")";
						}
						else
						{
							strPredicate += ") OR (";
							strPredicate += strThisKeyNull;
							strPredicate += "))";
						}
					}
				}

				// if this meta entity has a primitive predicate, add this also
				if (mapPrimitiveRLSPredicates.find(pME) != mapPrimitiveRLSPredicates.end())
				{
					if (strPredicate.empty())
						strPredicate = mapPrimitiveRLSPredicates[pME];
					else
						strPredicate = mapPrimitiveRLSPredicates[pME] + " AND " + strPredicate;
				}

				m_mapRLSPredicates[pME] = strPredicate;
/*
				// for debugging
				std::cout << "RLS Predicate:\n";
				std::cout << "  User.......: " << GetName() << std::endl;
				std::cout << "  MetaEntity.: " << pME->GetName() << std::endl;
				std::cout << "  Preciate...: " << strPredicate << std::endl;
*/
			}
		}
	}



	void User::CreatePrimitiveRLSPredicates(RLSPredicateMap & mapPrimitiveRLSPredicates)
	{
		RowLevelAccessPtrMapItr		itrRLS;
		TemporaryKeyPtr						pTmpKey;
		TemporaryKeyPtrListItr		itrTmpKey;
		std::string								strThisPred;
		ColumnPtrListItr					itrCol;
		ColumnPtr									pCol;
		MetaEntityPtr							pMetaEntity;
		bool											bQuotedValue = false;

		// If this has row level security build this' filter
		for ( itrRLS =  m_mapRowAccessRestrictions.begin();
					itrRLS != m_mapRowAccessRestrictions.end();
					itrRLS++)
		{
			pMetaEntity = itrRLS->first;

			strThisPred.clear();

			if (itrRLS->second->m_listTempKeys.empty())
			{
				// Build dummy predicate if we don't have any keys defined
				switch (itrRLS->second->m_type)
				{
					case RowAccessRestrictions::Allow:
						strThisPred = "1=2";	// do not allow any
						break;

					case RowAccessRestrictions::Deny:
						strThisPred = "1=1";	// allow any
						break;
				}
			}
			else
			{
				for ( itrTmpKey =  itrRLS->second->m_listTempKeys.begin();
							itrTmpKey != itrRLS->second->m_listTempKeys.end();
							itrTmpKey++)
				{
					pTmpKey = *itrTmpKey;

					// the first time add lhs for boolean expression
					if (itrTmpKey == itrRLS->second->m_listTempKeys.begin())
					{
						for (	itrCol  = pTmpKey->GetColumns().begin();
									itrCol != pTmpKey->GetColumns().end();
									itrCol++)
						{
							pCol = *itrCol;

							if (itrCol != pTmpKey->GetColumns().begin())
								strThisPred +=  "+'" SQL_COLUMN_SEPARATOR_STRING "'+";

							if (pCol->GetMetaColumn()->GetType() == MetaColumn::dbfString || pTmpKey->GetColumnCount() == 1)
							{
								strThisPred += pCol->GetMetaColumn()->GetName();
							}
							else
							{
								bQuotedValue = true;
								strThisPred += "CAST(";
								strThisPred += pCol->GetMetaColumn()->GetName();
								strThisPred += " AS varchar(20))";
							}
						}

						switch (itrRLS->second->m_type)
						{
							case RowAccessRestrictions::Allow:
								strThisPred += " IN (";
								break;

							case RowAccessRestrictions::Deny:
								strThisPred += " NOT IN (";
								break;
						}
					}
					else
					{
						strThisPred += ',';
					}

					if (pTmpKey->GetColumnCount() > 1)
						strThisPred += '\'';

					for (	itrCol  = pTmpKey->GetColumns().begin();
								itrCol != pTmpKey->GetColumns().end();
								itrCol++)
					{
						pCol = *itrCol;

						if (itrCol != pTmpKey->GetColumns().begin())
							strThisPred += SQL_COLUMN_SEPARATOR_STRING;

						if (pCol->GetMetaColumn()->GetType() == MetaColumn::dbfString && pTmpKey->GetColumnCount() == 1)
						{
							strThisPred += '\'';
							strThisPred += pCol->AsString();
							strThisPred += '\'';
						}
						else
						{
							strThisPred += pCol->AsString();
						}
					}

					if (pTmpKey->GetColumnCount() > 1)
						strThisPred += '\'';
				}

				strThisPred += ')';
			}
/*
			// for debugging
			std::cout << "Primitive predicate:\n";
			std::cout << "  User.......: " << GetName() << std::endl;
			std::cout << "  MetaEntity.: " << pMetaEntity->GetName() << std::endl;
			std::cout << "  Preciate...: " << strThisPred << std::endl;
*/
			mapPrimitiveRLSPredicates[pMetaEntity] = strThisPred;
		}
	}



	void User::DumpAllRLSFilters()
	{
		MetaDatabasePtr				pMD = MetaDatabase::GetMetaDatabase("APAL3DB");
		MetaEntityPtrVectItr	itrME;
		MetaEntityPtr					pME;


		for (	itrME  = pMD->GetMetaEntities()->begin();
					itrME != pMD->GetMetaEntities()->end();
					itrME++)
		{
			pME = *itrME;
			std::cout << pME->GetName() << std::endl;
			std::string&	strPred = GetRLSPredicate(pME);
			if (!strPred.empty())
			{
				std::cout << "{\n   SELECT * FROM " << pME->GetName() << " WHERE ";
				std::cout << strPred;
				std::cout << "\n}\n";
			}
			std::cout << std::endl;
		}
	}



	bool User::HasAccess(EntityPtr pEntity)
	{
		MonitoredLocker						lk(m_mtxExcl, "User::HasAccess()");
		unsigned int							idx;
		RelationPtr								pRel;

		// Have we got access to pEntity
		if (!HasExplicitAccess(pEntity))
			return false;

		for (idx = 0; idx < pEntity->GetParentRelationCount(); idx++)
		{
			pRel = pEntity->GetParentRelation(idx);

			if (pRel && !HasAccess(pRel->GetParent()))
				return false;
		}

		return true;
	}



	bool User::HasExplicitAccess(EntityPtr pEntity)
	{
		RowLevelAccessPtrMapItr		itrRLS;

		itrRLS = m_mapRowAccessRestrictions.find(pEntity->GetMetaEntity());

		if (itrRLS != m_mapRowAccessRestrictions.end())
		{
			TemporaryKeyPtrListItr		itrTmpKey;

			switch (itrRLS->second->m_type)
			{
				case RowAccessRestrictions::Allow:
				{
					for ( itrTmpKey =  itrRLS->second->m_listTempKeys.begin();
								itrTmpKey != itrRLS->second->m_listTempKeys.end();
								itrTmpKey++)
					{
						if (*(*itrTmpKey) == *(pEntity->GetPrimaryKey()))
							return true;
					}

					return false;
				}

				case RowAccessRestrictions::Deny:
				{
					for ( itrTmpKey =  itrRLS->second->m_listTempKeys.begin();
								itrTmpKey != itrRLS->second->m_listTempKeys.end();
								itrTmpKey++)
					{
						if (*(*itrTmpKey) == *(pEntity->GetPrimaryKey()))
							return false;
					}

					return true;
				}
			}
		}

		return true;
	}



	void User::SetRowLevelSecurity(D3UserPtr pD3User)
	{
		MonitoredLocker									lk(m_mtxExcl, "User::SetRowLevelSecurity()");
		D3User::D3RowLevelPermissions*	pListRLS = pD3User->GetD3RowLevelPermissions();

		// Check if RowLevelPermissions exists and initialise these
		if (!pListRLS->empty())
		{
			D3RowLevelPermissionPtr										pRLS;
			D3User::D3RowLevelPermissions::iterator		itrRLS;
			MetaEntityPtr															pME;

			for ( itrRLS =  pListRLS->begin();
						itrRLS != pListRLS->end();
						itrRLS++)
			{
				pRLS = *itrRLS;
				pME = MetaEntity::GetMetaEntityByID(pRLS->GetMetaEntityID());
				m_mapRowAccessRestrictions[pME] = new RowAccessRestrictions(pME, pRLS);
			}
		}

		// Allow access to their own D3User record only
		if (m_strName != D3_SYSADMIN_USER && m_strName != D3_ADMIN_USER)
		{
			RowAccessRestrictions*	pRLS = new RowAccessRestrictions(RowAccessRestrictions::Allow);
			TemporaryKeyPtr					pKey = new TemporaryKey(*(pD3User->GetPrimaryKey()));

			pRLS->AddKey(pKey);
			m_mapRowAccessRestrictions[pD3User->GetMetaEntity()] = pRLS;
		}
	}



	void User::DropRowLevelSecurity()
	{
		MonitoredLocker						lk(m_mtxExcl, "User::DropRowLevelSecurity()");
		RowLevelAccessPtrMapItr		itr;

		for ( itr  = m_mapRowAccessRestrictions.begin();
					itr != m_mapRowAccessRestrictions.end();
					itr++)
		{
			delete itr->second;
		}

		m_mapRowAccessRestrictions.clear();
		m_mapRLSPredicates.clear();
	}






	// ==========================================================================
	// RoleUser class implementation
	//


	// Statics initialisation
	//
	RoleUserPtr							RoleUser::M_sysadmin = NULL;											//!< The memory only sysadmin role-user
	RoleUserID							RoleUser::M_uNextInternalID = ROLEUSER_ID_MAX;		//!< If no internal ID is passed to the constructor, use this and decrement afterwards (used for meta dictionary definition meta obejcts)
	RoleUserPtrMap					RoleUser::M_mapRoleUser;													//!< Holds all RoleUser objects in the system indexed by m_uID

	// Standard D3 Stuff
	//
	D3_CLASS_IMPL(RoleUser, Object);


	RoleUser::RoleUser(D3RoleUserPtr pD3RoleUser)
	:	m_uID(pD3RoleUser->GetID()),
		m_pRole(NULL),
		m_pUser(NULL)
	{
		m_pRole = Role::GetRoleByID(pD3RoleUser->GetRoleID());
		m_pUser = User::GetUserByID(pD3RoleUser->GetUserID());

		Init();
	}



	RoleUser::RoleUser(RolePtr pRole, UserPtr pUser)
	: m_uID(M_uNextInternalID--),
		m_pRole(pRole),
		m_pUser(pUser)
	{
		Init();
	}



	RoleUser::~RoleUser()
	{
		SessionPtr	pSession;

		if (m_pRole) m_pRole->On_RoleUserDeleted(this);
		if (m_pUser) m_pUser->On_RoleUserDeleted(this);

		M_mapRoleUser.erase(m_uID);

		while (!m_listSessions.empty())
		{
			pSession = m_listSessions.front();
			delete pSession;
		}

		if (M_sysadmin == this)
			M_sysadmin = NULL;
	}



	void RoleUser::Init()
	{
		assert(m_pRole);
		assert(m_pUser);

		if (m_pRole == Role::M_sysadmin && m_pUser == User::M_sysadmin)
		{
			if (M_sysadmin)
				throw std::runtime_error("RoleUser::Init(): role-user already exists.");

			M_sysadmin = this;
		}

		assert(M_mapRoleUser.find(m_uID) == M_mapRoleUser.end());
		M_mapRoleUser[m_uID] = this;

		// Enable access to all D3User objects if the created role user belongs to the Administrator role
		if (m_pRole->GetName() == D3_ADMIN_ROLE && m_pUser->GetName() != D3_ADMIN_USER)
		{
			MetaEntityPtr											pME = D3::MetaDatabase::GetMetaDatabase("D3MDDB")->GetMetaEntity(D3MDDB_D3User);
			User::RowLevelAccessPtrMapItr			itr1 = m_pUser->m_mapRowAccessRestrictions.find(pME);
			User::RLSPredicateMapItr					itr2 = m_pUser->m_mapRLSPredicates.find(pME);

			if (itr1 != m_pUser->m_mapRowAccessRestrictions.end())
			{
				delete itr1->second;
				m_pUser->m_mapRowAccessRestrictions.erase(itr1);
			}

			if (itr2 != m_pUser->m_mapRLSPredicates.end())
				m_pUser->m_mapRLSPredicates.erase(itr2);
		}

		m_pRole->On_RoleUserCreated(this);
		m_pUser->On_RoleUserCreated(this);
	}



	//! Save Sessions
	void RoleUser::SaveSessions()
	{
		if (m_pRole != Role::M_sysadmin)
		{
			SessionPtr					pSession;
			SessionPtrListItr		itrSessions;
			

			for (itrSessions =  m_listSessions.begin();
					 itrSessions != m_listSessions.end();
					 itrSessions++)
			{
				pSession = *itrSessions;
				pSession->Save();
			}
		}
	}



	//! Delete Sessions
	void RoleUser::DeleteSessions()
	{
		SessionPtr	pSession;

		while (!m_listSessions.empty())
		{
			pSession = m_listSessions.front();
			delete pSession;
		}
	}



	void RoleUser::Refresh(D3RoleUserPtr pD3RoleUser)
	{
		assert(m_uID == pD3RoleUser->GetID());

		if (m_pRole->GetID() != pD3RoleUser->GetID())
			m_pRole = Role::GetRoleByID(pD3RoleUser->GetID());

		if (m_pUser->GetID() != pD3RoleUser->GetID())
			m_pUser = User::GetUserByID(pD3RoleUser->GetID());
	}



	D3RoleUserPtr RoleUser::GetD3RoleUser(DatabaseWorkspace& dbWS)
	{
		DatabasePtr		pDB = dbWS.GetDatabase("D3MDDB");

		// preload these
		GetRole()->GetD3Role(dbWS);
		GetUser()->GetD3User(dbWS);

		return D3RoleUser::Load(pDB, m_uID);
	}


	void RoleUser::On_SessionCreated(SessionPtr	pSession)
	{
		assert(pSession->GetRoleUser() == this);
		m_listSessions.push_back(pSession);
	}



	void RoleUser::On_SessionDeleted(SessionPtr	pSession)
	{
		assert(pSession->GetRoleUser() == this);
		m_listSessions.remove(pSession);
	}


	std::ostream & RoleUser::AsJSON(std::ostream & ostrm)
	{
		ostrm << '{';
		ostrm << "\"ID\":"					<< m_uID													<< ",";
		ostrm << "\"RoleID\":"			<< m_pRole->GetID()								<< ",";
		ostrm << "\"UserID\":"			<< m_pUser->GetID();
		ostrm << '}';

		return ostrm;
	}


	/* static */
	std::ostream & RoleUser::RoleUsersAsJSON(std::ostream & ostrm)
	{
		RoleUserPtrMapItr			itr;
		bool							bFirst = true;

		ostrm << '[';

		for ( itr =  M_mapRoleUser.begin();
					itr != M_mapRoleUser.end();
					itr++)
		{
			if (bFirst)
				bFirst = false;
			else
				ostrm << ',';

			itr->second->AsJSON(ostrm);
		}

		ostrm << ']';

		return ostrm;
	}




	//===========================================================================
	//
	// Session implementation
	//


	// Statics initialisation
	//
	SessionPtrMap						Session::M_mapSession;
	boost::recursive_mutex	Session::M_mtxExclusive;



	Session::Session(RoleUserPtr & pRoleUser)
	: m_pRoleUser(pRoleUser)
	{
		MonitoredLocker						lk(M_mtxExclusive, "Session::Session()");

		DatabaseWorkspace					dbWS;
		DatabasePtr								pDB = dbWS.GetDatabase("D3MDDB");
		D3SessionPtr							pD3Session;
		D3RoleUserPtr							pD3RoleUser;


		m_dtSignedIn = D3Date();
		assert(m_pRoleUser->GetRole()->IsEnabled());
		assert(m_pRoleUser->GetUser()->IsEnabled());

		if (m_pRoleUser != RoleUser::M_sysadmin)
		{
			pD3RoleUser = m_pRoleUser->GetD3RoleUser(dbWS);
			assert(pD3RoleUser);

			// Create a D3Session in the database
			pD3Session = D3Session::CreateD3Session(pDB);
			pD3Session->SetD3RoleUser(pD3RoleUser);
			pD3Session->SetSignedIn(m_dtSignedIn);
			pD3Session->GetColumn(D3Session_SignedOut)->SetNull();

			pDB->BeginTransaction();
			pD3Session->Update(pDB);
			pDB->CommitTransaction();

			m_uID = pD3Session->GetID();
		}
		else
		{
			m_uID = (unsigned long) this;
		}

		M_mapSession[m_uID] = this;

		// Create the default DatabaseWorkspace
		CreateDatabaseWorkspace();
		Touch();

		D3::ReportDiagnostic("Session %i created", m_uID);

		if (m_pRoleUser)
			m_pRoleUser->On_SessionCreated(this);
	}



	Session::~Session()
	{
		MonitoredLocker						lk(M_mtxExclusive, "Session::~Session()");

		try
		{
			if (m_pRoleUser)
				m_pRoleUser->On_SessionDeleted(this);

			M_mapSession.erase(m_uID);

			if (m_pRoleUser != RoleUser::M_sysadmin)
			{
				// Remove all DatabaseWorkspace objects this owns
				while (!m_listDBWS.empty())
					delete  m_listDBWS.front();
			}

			D3::ReportDiagnostic("Session %i closed", m_uID);
		}
		catch (...)
		{
			GenericExceptionHandler("Session::~Session(): Session %i closed with errors", m_uID);
		}
	}



	D3::DatabaseWorkspacePtr Session::CreateDatabaseWorkspace()
	{
		DatabaseWorkspacePtr			pDBWS;

		try
		{
			pDBWS = new DatabaseWorkspace(&m_excptCtx, this);
			m_listDBWS.push_back(pDBWS);
		}
		catch (...)
		{
			pDBWS = NULL;
			ReportError("Session::CreateDatabaseWorkspace(): Failed to create DatabaseWorkspace");
		}

		return pDBWS;
	}



	void Session::KillTimedOutSessions(unsigned int uiSessionTimeout)
	{
		MonitoredLocker				lk(M_mtxExclusive, "Session::KillTimedOutSessions");

		SessionPtr						pSession;
		SessionPtrMapItr			itrSession;
		SessionPtrMap					mapSessions(M_mapSession);		// work with copy
		time_t								tCurrentTime = time(NULL);
		bool									bLockable;


		for ( itrSession =  mapSessions.begin();
					itrSession != mapSessions.end();
					itrSession++)
		{
			pSession = itrSession->second;
			bLockable = false;

			// This try block attempts a non-blocking lock which it releases immediately
			try
			{
				boost::recursive_try_mutex::scoped_try_lock			lk(pSession->m_mtxExcl);
				bLockable = lk.owns_lock();
			}
			catch(boost::lock_error)
			{
			}

			if (bLockable && (unsigned int) (tCurrentTime - pSession->m_tLastAccessed) > uiSessionTimeout)
			{
				pSession->Save();
				delete pSession;
			}
		}
	}



	void Session::Touch()
	{
		m_tLastAccessed = time(NULL);
	}



	void Session::GetSessionByID(SessionID uID, LockedSessionPtr* pLockedSession)
	{
		MonitoredLocker				lk(M_mtxExclusive, "Session::GetSessionByID");

		SessionPtrMapItr			itr;
		SessionPtr						pSession = NULL;


		itr = M_mapSession.find(uID);

		if (itr != M_mapSession.end())
			pSession = itr->second;
		else
			D3::ReportDiagnostic("Session %i does not exist", uID);

		pLockedSession->Assign(pSession, true);
	}



	// Update the related D3Session object
	void Session::Save()
	{
		MonitoredLocker						lk(M_mtxExclusive, "Session::Save()");

		DatabaseWorkspace					dbWS;
		DatabasePtr								pDB = NULL;
		D3SessionPtr							pD3Session;

		try
		{
			if (m_pRoleUser != RoleUser::M_sysadmin)
			{
				pD3Session = GetD3Session(dbWS);

				if (pD3Session)
				{
					pD3Session->SetSignedOut(D3Date());
					pDB = pD3Session->GetDatabase();
					pDB->BeginTransaction();
					pD3Session->Update();
					pDB->CommitTransaction();
				}
			}

			D3::ReportDiagnostic("Session %i saved", m_uID);
		}
		catch (...)
		{
			GenericExceptionHandler("Session::~Session(): Session %i closed with errors", m_uID);
		}

		if (pDB && pDB->HasTransaction())
			pDB->RollbackTransaction();
	}



	D3::EntityPtr Session::GetEntity(const std::string & strJSONObjectID, bool bRefresh)
	{
		Json::Reader								jsonReader;
		Json::Value									nRoot;
		Json::Value*								pNode;
		D3::DatabaseWorkspacePtr		pDBWS;
		D3::DatabasePtr							pDB;
		D3::MetaKeyPtr							pMK;


		// Parse the JSON input
		if (!jsonReader.parse(strJSONObjectID, nRoot))
			throw Exception(__FILE__, __LINE__, Exception_error,
											"Parse error. Input:\n%s\n\nParser message:\n",
											strJSONObjectID.c_str(),
											jsonReader.getFormatedErrorMessages().c_str());

		// Get the database workspace
		pNode	= &(nRoot["DBWSID"]);

		if (pNode->type() != Json::nullValue)
			pDBWS = (DatabaseWorkspacePtr) (pNode->asUInt());
		else
			throw std::string("DBWSID not found in Json key ID structure");

		// Create the new temporary key (the constructor will set all the keys values)
		D3::TemporaryKey						tempKey(nRoot);

		pMK = tempKey.GetMetaKey();
		pDB = pDBWS->GetDatabase(pMK->GetMetaEntity()->GetMetaDatabase());

		if (pDB)
			return pMK->LoadObject(&tempKey, pDB, bRefresh);

		throw std::runtime_error(strJSONObjectID + ": object not found!");


		return NULL;
	}



	D3SessionPtr Session::GetD3Session(DatabaseWorkspace& dbWS)
	{
		DatabasePtr		pDB = dbWS.GetDatabase("D3MDDB");

		GetRoleUser()->GetD3RoleUser(dbWS);

		return D3Session::Load(pDB, m_uID);
	}



	// Notification received by a DatabaseWorkspace owned by this that it has been detroyed,
	// so simply remove it from the collection of DatabaseWorkspaces this owns
	void Session::On_DatabaseWorkspaceDestroyed(DatabaseWorkspacePtr pDeletedDBWS)
	{
		DatabaseWorkspacePtr				pDBWS;
		DatabaseWorkspacePtrListItr	itrDBWS;


		for ( itrDBWS =  m_listDBWS.begin();
					itrDBWS != m_listDBWS.end();
					itrDBWS++)
		{
			pDBWS = *itrDBWS;

			if (pDBWS == pDeletedDBWS)
			{
				m_listDBWS.erase(itrDBWS);
				break;
			}
		}
	}






	//===========================================================================
	//
	// LockedSessionPtr implementation
	//

	LockedSessionPtr::LockedSessionPtr(SessionPtr pSession)
		: m_pSession(pSession),
			m_pLock(NULL)
	{
		Assign(pSession);
	}



	LockedSessionPtr::LockedSessionPtr(LockedSessionPtr& rhs) : m_pSession(NULL), m_pLock(NULL)
	{
		m_pSession = rhs.m_pSession;
		m_pLock = rhs.m_pLock;
		rhs.m_pSession = NULL;
		rhs.m_pLock = NULL;
	}



	LockedSessionPtr::~LockedSessionPtr()
	{
		Release();
	}



	LockedSessionPtr& LockedSessionPtr::operator=(LockedSessionPtr& rhs)
	{
		Release();

		m_pSession = rhs.m_pSession;
		m_pLock = rhs.m_pLock;
		rhs.m_pSession = NULL;
		rhs.m_pLock = NULL;

		return *this;
	}



	void LockedSessionPtr::Assign(SessionPtr pSession, bool bTryLock)
	{
		Release();

		m_pSession = pSession;

		if (m_pSession)
		{
			try
			{
				if (bTryLock)
					m_pLock = new boost::recursive_try_mutex::scoped_try_lock(m_pSession->m_mtxExcl, boost::try_to_lock);
				else
					m_pLock = new boost::recursive_try_mutex::scoped_try_lock(m_pSession->m_mtxExcl);

				if (!m_pLock->owns_lock())
					throw 0;
			}
			catch (...)
			{
				delete m_pLock;
				m_pLock = NULL;
			}
		}
	}



	void LockedSessionPtr::Release()
	{
		delete m_pLock;
		m_pLock = NULL;
		m_pSession = NULL;
	}



	//! This method allows you to Delete the actual session attached to this
	void LockedSessionPtr::Delete()
	{
		delete m_pLock;
		m_pLock = NULL;
		m_pSession->Save();
		delete m_pSession;
		m_pSession = NULL;
	}



	/* static */
	void LockedSessionPtr::CreateSession(LockedSessionPtr & pLockedSession, RoleUserPtr & pRoleUser)
	{
		pLockedSession.Release();
		pLockedSession.Assign(new Session(pRoleUser));
		pLockedSession.m_pSession->Touch();
	}



	/* static */
	void LockedSessionPtr::GetSessionByID(LockedSessionPtr & pLockedSession, SessionID uID)
	{
    boost::xtime			xt;
		int								iWaitMax = 60; // Wait at most for 60 seconds


		pLockedSession.Release();

		// We may have to do this a few times
		while (true)
		{
			Session::GetSessionByID(uID, &pLockedSession);

			if (pLockedSession.m_pSession && (pLockedSession.m_pLock == NULL || !pLockedSession.m_pLock->owns_lock()))
			{
				if (iWaitMax <= 0)
				{
					ReportWarning("LockedSessionPtr::GetSessionByID(): Timed-out waiting for session %i to unlock.", uID);
					pLockedSession.Assign(NULL);
					break;
				}

				// Session is locked, so wait 1 sec and then try again
				boost::xtime_get(&xt, boost::TIME_UTC_);
				xt.sec += 1;
				iWaitMax--;
		    boost::thread::sleep(xt);
				continue;
			}

			break;
		}

		if (pLockedSession.m_pSession && pLockedSession.m_pLock && pLockedSession.m_pLock->owns_lock())
			pLockedSession.m_pSession->Touch();
		else
			pLockedSession.Release();
	}
}

