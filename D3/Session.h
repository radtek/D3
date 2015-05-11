#ifndef _D3_Session_H_
#define _D3_Session_H_

// MODULE: Session header
//
// NOTE: This module implements:
//
//					D3::Role
//					D3::User
//					D3::RoleUser
//					D3::Permission
//					D3::Session
//;
// ===========================================================


#include "D3.h"
#include "D3BitMask.h"
#include "Exception.h"
#include "Database.h"

#include "D3Role.h"
#include "D3User.h"
#include "D3RoleUser.h"
#include "D3Session.h"
#include "D3DatabasePermission.h"
#include "D3EntityPermission.h"
#include "D3ColumnPermission.h"
#include "D3RowLevelPermission.h"

// sysadmin details (Role/User/Password): sysadmin/sysadmin/AvLbtW0
#define D3_SYSADMIN_ROLE "sysadmin"
#define D3_SYSADMIN_USER "sysadmin"
#define D3_SYSADMIN_PWD	 "Yi/iJHTxGsCfXCoaVbBRDg=="

// admin details (Role/User/Password): Administrator/admin/akl5120
#define D3_ADMIN_ROLE "Administrator"
#define D3_ADMIN_USER "admin"
#define D3_ADMIN_PWD	"0Ud6Qsc0zK+oA/aRkMl3yw=="

#include <boost/thread/recursive_mutex.hpp>

extern const char szRoleFeatures[];					// solely used to ensure type safety (no implementation needed)
extern const char szRoleIRSSettings[];			// solely used to ensure type safety (no implementation needed)
extern const char szRoleV3ParamSettings[];	// solely used to ensure type safety (no implementation needed)
extern const char szRoleT3ParamSettings[];	// solely used to ensure type safety (no implementation needed)
extern const char szRoleP3ParamSettings[];	// solely used to ensure type safety (no implementation needed)

namespace D3
{
	// During importing of RBAC, updates to RBAC objects are ignored
	extern bool	G_bImportingRBAC;

	//@{

	//! We store type specific permissions in a maps...
	typedef std::map<DatabaseID,	MetaDatabase::Permissions>				MetaDatabasePermissionsMap;
	typedef std::map<EntityID,		MetaEntity::Permissions>					MetaEntityPermissionsMap;
	typedef std::map<ColumnID,		MetaColumn::Permissions>					MetaColumnPermissionsMap;

	//! ...and use these iterators
	typedef MetaDatabasePermissionsMap::iterator			MetaDatabasePermissionsMapItr;
	typedef MetaEntityPermissionsMap::iterator				MetaEntityPermissionsMapItr;
	typedef MetaColumnPermissionsMap::iterator				MetaColumnPermissionsMapItr;
	//@}

	//! We store all Role objects in a map keyed by ID
	/*! Note that objects which are not stored in the meta dictionary (e.g. Role objects
			created at system startup to enable access to the the MetaDictionary database)
			are assigned Role::M_uNextInternalID which is then decremented by 1. The
			initial value	of Role::M_uNextInternalID is ROLE_ID_MAX.
			Role objects without a physical representation on disk are therefore
			identifiable if GetID() returns a value > Role::M_uNextInternalID.
	*/
	typedef std::map< RoleID, RolePtr >				RolePtrMap;
	typedef RolePtrMap::iterator							RolePtrMapItr;

	//! Role objects encapsulate tasks or functions users perform within an organisation.
	/*! Many users can act in the same role.

			The Role is part of Role Based Access Control, other components are:
			User, RoleUser and Permissions (permissions are not separate, they
			are embedded inside this class)

			To initialize RBAC, all you need to do is:

			-# Make all relevant D3Role objects resident
			-# Make all relevant D3User objects resident
			-# Make all relevant D3RoleUser objects resident
			-# Make all relevant D3DatabasePermissions objects resident
			-# Make all relevant D3EntityPermissions objects resident
			-# Make all relevant D3ColumnPermissions objects resident
			-# Create a new D3::Role object for each D3::D3Role you loaded in 1
			-# Create a new D3::User object for each D3::D3User you loaded in 2
			-# Create a new D3::RoleUser object for each D3::D3User you loaded in 3

			See MetaDatabase::LoadMetaDictionary().

			You need to delete all Role and User objects using the provided static methods
			DeleteAll() explicitly before exiting the system. Currently, this is handled
			in MetaDatabase::UnInitialsie().

			If you want to save any active sessions before existing, call either:

			User::SaveAndDeleteRoleUsers();
			Role::SaveAndDeleteRoleUsers();

			You only need to call one (if you called both the second would do nothing).
	*/
	class D3_API Role : public Object
	{
		friend class MetaDatabase;
		friend class RoleUser;
		friend class D3Role;
		friend class D3DatabasePermission;
		friend class D3EntityPermission;
		friend class D3ColumnPermission;

		// Standard D3 stuff
		D3_CLASS_DECL(Role);

		public:
			//! This bitmask describes application features a Role is entiled to utilise
			struct Features : public Bits<szRoleFeatures>
			{
				BITS_DECL(Role, Features);

				//@{ Primitive masks
				static const Mask RunP3;								//!< 0x00000001 - User can RunP3 algorithm.
				static const Mask RunT3;								//!< 0x00000002 - User can RunT3 algorithm.
				static const Mask RunV3;								//!< 0x00000004 - User can RunV3 algorithm.
				static const Mask WHMapView;						//!< 0x00000008 - User can view Warehouse Map using applet
				static const Mask WHMapUpdate;					//!< 0x00000010 - User can modify Warehouse Map using applet
				static const Mask ResolveIssues;				//!< 0x00000080 - User can resolve issues
				//@}
			};

			//! This bitmask describes which features of IRS are available to members of a Role
			struct IRSSettings : public Bits<szRoleIRSSettings>
			{
				BITS_DECL(Role, IRSSettings);

				//@{ Primitive masks
				static const Mask AllowUndo;						//!< 0x00000001 - Allow undoing of recship processing
				static const Mask PullForwardSmooth;		//!< 0x00000002 - Allow pull forward smoothing
				static const Mask PushBackSmooth;				//!< 0x00000004 - Allow push back smoothing
				static const Mask ChangeOIShipFrom;			//!< 0x00000008 - Can move order items to shipments with different ship-from location
				static const Mask ChangeOIShipTo;				//!< 0x00000010 - Can move order items to shipments with different ship-to location
				static const Mask ChangeOITransmode;		//!< 0x00000020 - Can move order items to shipments with different transmode
				static const Mask AllowCutSub;					//!< 0x00000040 - Allow cut sub
				static const Mask AllowRequestSolution;	//!< 0x00000080 - Allow requesting solution
				static const Mask AllowExportOrDownload;//!< 0x00000100 - Allow exporting details or downloading csv files of solved actual shipments
				//@}
			};

			//! This bitmask describes which order optimization parameters can be modified by a Role
			struct V3ParamSettings : public Bits<szRoleV3ParamSettings>
			{
				BITS_DECL(Role, V3ParamSettings);

				//@{ Primitive masks
				static const Mask ModifyWarehouseParam;	//!< 0x00000001 - Modify Ship-from location parameters
				static const Mask ModifyLaneParam;			//!< 0x00000002 - Modify Lane parameters
				static const Mask ModifyShipmentParam;	//!< 0x00000004 - Modify Shipment parameters
				//@}
			};

			//! This bitmask describes which truck loading parameters can be modified by a Role
			struct T3ParamSettings : public Bits<szRoleT3ParamSettings>
			{
				BITS_DECL(Role, T3ParamSettings);

				//@{ Primitive masks
				static const Mask ModifyWarehouseParam;	//!< 0x00000001 - Modify Ship-from location parameters
				static const Mask ModifyLaneParam;			//!< 0x00000002 - Modify Lane parameters
				static const Mask ModifyShipmentParam;	//!< 0x00000004 - Modify Shipment parameters
				//@}
			};

			//! This bitmask describes which case picking parameters can be modified by a Role
			struct P3ParamSettings : public Bits<szRoleP3ParamSettings>
			{
				BITS_DECL(Role, P3ParamSettings);

				//@{ Primitive masks
				static const Mask ModifyWarehouseParam;	//!< 0x00000001 - Modify Ship-from location parameters
				static const Mask ModifyCustomerParam;	//!< 0x00000002 - Modify Customer parameters
				static const Mask ModifyOrderParam;			//!< 0x00000004 - Modify Order parameters
				static const Mask ModifyZoneParam;			//!< 0x00000008 - Modify Zone parameters
				//@}
			};

		public:
			//! Holds the in memory only sysadmin Role
			/*! sysadmin is an in-memory only role that is created automatically at system start.
					It has the most relaxed permissions possible. It can't be configured or restricted
					in any other way than through hard-coding.
			*/
			static RolePtr													M_sysadmin;								//!< Holds the in memory only sysadmin Role

		protected:
			static RoleID														M_uNextInternalID;				//!< If no internal ID is passed to the constructor, use this and decrement afterwards (used for meta dictionary definition meta obejcts)
			static RolePtrMap												M_mapRole;								//!< Holds all Role objects in the system indexed by m_uID

			boost::recursive_mutex									m_mtxExcl;								//!< Protects security setting changes
			RoleID																	m_uID;										//!< The Role's ID
			std::string															m_strName;								//!< The Role's name
			bool																		m_bEnabled;								//!< Must be true for use in Sessions
			Features																m_features;								//!< See Role::Features for details
			IRSSettings															m_irssettings;						//!< See Role::IRSSettings for details
			V3ParamSettings													m_v3paramsettings;				//!< See Role::V3ParamSettings for details
			T3ParamSettings													m_t3paramsettings;				//!< See Role::T3ParamSettings for details
			P3ParamSettings													m_p3paramsettings;				//!< See Role::P3ParamSettings for details
			RoleUserPtrList													m_listRoleUsers;					//!< RoleUsers associated with this

			//@{
			//! The purpose of the permissions vectors is to enable rapid lookup of permissions relating to a particular meta object
			/*!	At construction time, the Role will initialise these maps with permissions from the  specific
					permission records associated with itself.

					Each of the meta types has a static method which returns a vector of pointers to all instances of its meta type:

					\code
					static const map<MetaDatabase*>&		D3::MetaDatabase::GetMetaDatabases();
					static const map<MetaEntity*>&			D3::MetaEntity::GetMetaEntities();
					static const map<MetaColumn*>&			D3::MetaColumn::GetMetaColumns();
					static const map<MetaKey*>&					D3::MetaKey::GetMetaKeys();
					static const map<MetaRelation*>&		D3::MetaRelation::GetMetaRelations();
					\endcode

					At Role construction time, it is expected that these collections are fully populated. This enables the Role
					constructor to generate permissions maps mapped to the correct elements.

					Each instance of the above meta types has a GetID instance method. The value this method returns is the ordinal
					position of the object inside the vector of all instances of its type. The same value will also be used to
					locate the objects permissions associated with a Role efficiently via these maps.

					It is easiest to explain how the permissions are applied using a small example. Let's assume you'd like to know
					is a specific MetaColumn (pMC) is allowed to be read. Here is what we do:

					-# if (!pMC->AllowRead()) return false;
					-# if (!pMC->GetMetaEntity()->AllowSelect()) return false;
					-# if (!pMC->GetMetaEntity()->GetMetaDatabase()->AllowRead()) return false;
					-# if (m_mapPermissions[pMC->GetID()] && *(m_mapMCPermissions[pMC->GetID()]) & D3::MetaColumn::Permissions::Read == 0) return false;
					-# if (m_mapMEPermissions[pMC->GetMetaEntity()->GetID()] && *(m_mapMEPermissions[pMC->GetMetaEntity()->GetID()]) & D3::MetaEntity::Permissions::Select == 0) return false;
					-# if (m_mapMDPermissions[pMC->GetMetaEntity()->GetMetaDatabase()->GetID()] && *(m_mapMDPermissions[pMC->GetMetaEntity()->GetMetaDatabase()->GetID()]) & D3::MetaDatabase::Permissions::Read == 0) return false;
					-# return true;

					In steps 1-3 we deal with basic meta type permissions which have precedence over any role specific settings.
					In steps 4-6 we check for any explicit denials of the tested permission. This means that the acknoledgement
					of a permission is always implied: if no explicit permission exsist, we assume permission is granted.
			*/
			MetaDatabasePermissionsMap							m_mapMDPermissions;			//!< A map of D3::MetaDatabase::Permissions
			MetaEntityPermissionsMap								m_mapMEPermissions;			//!< A map of D3::MetaEntity::Permissions
			MetaColumnPermissionsMap								m_mapMCPermissions;			//!< A map of D3::MetaColumn::Permissions
			//@}

			//!	Dummy ctor needed for D3 class factory
			Role()	{ assert(false); }

			//!	Construct a Role from its database representation
			Role(D3RolePtr pD3Role);

			//!	This ctor allows construction of role objects to access meta dictionary objects
			Role(const std::string& strName, bool bEnabled, const Features::Mask& features, const IRSSettings::Mask& irssettings, const V3ParamSettings::Mask& v3paramsettings, const T3ParamSettings::Mask& t3paramsettings, const P3ParamSettings::Mask& p3paramsettings);

			//! ctor helper
			void																			Init(D3RolePtr pD3Role = NULL);

			//! Delete all Role objects (does not save active sessions!!!)
			static void																DeleteAll();
			//! Save and delete RoleUser
			static void																SaveAndDeleteRoleUsers();
			//! Save RoleUser
			void																			SaveRoleUsers();
			//! Delete RoleUser
			void																			DeleteRoleUsers();

			//! Sets the role specific permission for the given a MetaDatabase object
			virtual void															SetPermissions(D3DatabasePermissionPtr	pD3DatabasePermissions);
			//! Sets the role specific permission for the given a MetaEntity object
			virtual void															SetPermissions(D3EntityPermissionPtr		pD3EntityPermissions)			{ m_mapMEPermissions[pD3EntityPermissions->GetMetaEntityID()] = MetaEntity::Permissions::Mask(pD3EntityPermissions->GetAccessRights()); }
			//! Sets the role specific permission for the given a MetaColumn object
			virtual void															SetPermissions(D3ColumnPermissionPtr		pD3ColumnPermissions)			{ m_mapMCPermissions[pD3ColumnPermissions->GetMetaColumnID()] = MetaColumn::Permissions::Mask(pD3ColumnPermissions->GetAccessRights()); }

			//! Sets the role specific permission for the given a MetaDatabase object
			virtual void															SetPermissions(MetaDatabasePtr pMD, MetaDatabase::Permissions::Mask mask);
			//! Sets the role specific permission for the given a MetaEntity object
			virtual void															SetPermissions(MetaEntityPtr pME, MetaEntity::Permissions::Mask mask)				{ m_mapMEPermissions[pME->GetID()] = mask; }
			//! Sets the role specific permission for the given a MetaColumn object
			virtual void															SetPermissions(MetaColumnPtr pMC, MetaColumn::Permissions::Mask mask)				{ m_mapMCPermissions[pMC->GetID()] = mask; }

			//! Deletes the role specific permission for the given a MetaDatabase object
			virtual void															DeletePermissions(D3DatabasePermissionPtr	pD3DatabasePermissions);
			//! Deletes the role specific permission for the given a MetaEntity object
			virtual void															DeletePermissions(D3EntityPermissionPtr		pD3EntityPermissions);
			//! Deletes the role specific permission for the given a MetaColumn object
			virtual void															DeletePermissions(D3ColumnPermissionPtr		pD3ColumnPermissions);

			//! Deletes the role specific permission for the given a MetaColumn object
			virtual void															AddDefaultPermissions();

		public:
			//!	The destructor deletes all attached RoleUsers
			virtual ~Role();

			//! Get RoleUsers returns the list of RoleUser obejcts assigned to this.
			virtual RoleUserPtrList&									GetRoleUsers()				{ return m_listRoleUsers; }

			//! Given a MetaDatabase object, return a MetaDatabase::Permissions object that applies to this Role
			virtual MetaDatabase::Permissions					GetPermissions(MetaDatabasePtr pMD);
			//! Given a MetaEntity object, return a MetaEntity::Permissions object that applies to this Role
			virtual MetaEntity::Permissions						GetPermissions(MetaEntityPtr pME);
			//! Given a MetaColumn object, return a MetaColumn::Permissions object that applies to this Role
			virtual MetaColumn::Permissions						GetPermissions(MetaColumnPtr pMC);

			//@{ Derived permissions
			//! Given a MetaKey object, return true if the role has read access to all MetaColumns in the key
			virtual bool															CanRead(MetaKeyPtr pMK);
			//! Given a MetaKey object, return true if the role has write access to all MetaColumns in the key
			virtual bool															CanWrite(MetaKeyPtr pMK);
			//! Given a MetaRelation object, return true if the role has read access to the parent key in the relation
			virtual bool															CanReadParent(MetaRelationPtr pMR);
			//! Given a MetaRelation object, return true if the role has write access to the parent key in the relation
			virtual bool															CanChangeParent(MetaRelationPtr pMR);
			//! Given a MetaRelation object, return true if the role has read access to the child key in the relation
			virtual bool															CanReadChildren(MetaRelationPtr pMR)					{ return CanRead(pMR->GetParentMetaKey()) && CanRead(pMR->GetChildMetaKey()); }
			//! Given a MetaRelation object, return true if the role has write access to the child key in the relation
			virtual bool															CanAddRemoveChildren(MetaRelationPtr pMR)			{ return CanRead(pMR->GetParentMetaKey()) && CanWrite(pMR->GetChildMetaKey()); }
			//@}

			//@{ Attributes accessors
			//! Return this users ID
			virtual RoleID														GetID() const														{ return m_uID; }
			//! Returns the name of this Role
			virtual const std::string&								GetName() const													{ return m_strName; }
			//! Changes the name of the Role
			virtual void															SetName(const std::string& strName)			{ m_strName = strName; }
			//! Returns true if this Role is enabled (must be enabled to establish a Session)
			virtual bool															IsEnabled()															{ return m_bEnabled; }

			//! Enables this role
			virtual void															Enable()																{ m_bEnabled = true; }
			//! Enables this role
			virtual void															Disable()																{ m_bEnabled = false; }
			//@}


			//! Return the list of all roles in the system
			static RolePtrMap&												GetRoles()						{ return M_mapRole; }
			//! Givem a D3Role ID, find the matching Role
			static RolePtr														GetRoleByID(RoleID uID)
			{
				RolePtrMapItr	itr = M_mapRole.find(uID);
				return (itr == M_mapRole.end() ? NULL : itr->second);
			}

			//!	Update this from the values of the passed in object
			virtual void															Refresh(D3RolePtr pD3Role);
			//! Loads the corresponding D3Role record into the passed in DatabaseWorkspace and returns it
			virtual D3RolePtr													GetD3Role(DatabaseWorkspace& dbWS);

			//! Inserts this into the passed-in stream as a JSON object.
			std::ostream &														AsJSON(std::ostream & ostrm);
			//! Inserts all Roles currently resident into the passed-in stream as a JSON objects.
			static std::ostream &											RolesAsJSON(std::ostream & ostrm);

			//@{ Special rights accessors
			const Features &													GetFeatures() const							{ return m_features; }
			const IRSSettings &												GetIRSSettings() const					{ return m_irssettings; }
			const V3ParamSettings &										GetV3ParamSettings() const			{ return m_v3paramsettings; }
			const T3ParamSettings &										GetT3ParamSettings() const			{ return m_t3paramsettings; }
			const P3ParamSettings &										GetP3ParamSettings() const			{ return m_p3paramsettings; }
			//@}

		protected:
			//! Notification that a RoleUser has been added
			virtual void															On_RoleUserCreated(RoleUserPtr	pRoleUser);
			//! Notification that a RoleUser has been deleted
			virtual void															On_RoleUserDeleted(RoleUserPtr	pRoleUser);
	};





	//! We store all User objects in a map keyed by ID
	/*! Note that objects which are not stored in the meta dictionary (e.g. User objects
			created at system startup to enable access to the the MetaDictionary database)
			are assigned User::M_uNextInternalID which is then decremented by 1. The
			initial value	of User::M_uNextInternalID is ROLE_ID_MAX.
			User objects without a physical representation on disk are therefore
			identifiable if GetID() returns a value > User::M_uNextInternalID.
	*/
	typedef std::map< UserID, UserPtr >				UserPtrMap;
	typedef UserPtrMap::iterator							UserPtrMapItr;

	//! User objects specify people who have a right to use D3
	class D3_API User : public Object
	{
		friend class MetaDatabase;
		friend class Role;
		friend class RoleUser;
		friend class D3User;
		friend class D3RowLevelPermission;

		// Standard D3 stuff
		D3_CLASS_DECL(User);

		//! Users access to records can be restricted on a row-level basis. We store such details in a RowAccessRestrictions structure.
		struct RowAccessRestrictions
		{
			enum RowAccessRestrictionsType
			{
				Undefined,
				Allow,
				Deny
			};

			RowAccessRestrictionsType				m_type;
			TemporaryKeyPtrList							m_listTempKeys;

			RowAccessRestrictions() : m_type(Undefined) {}
			RowAccessRestrictions(MetaEntityPtr pMetaEntity, D3RowLevelPermissionPtr pRowLevelPermission);
			RowAccessRestrictions(RowAccessRestrictionsType type)	: m_type(type) {};

			~RowAccessRestrictions()
			{
				while (!m_listTempKeys.empty())
				{
					delete m_listTempKeys.front();
					m_listTempKeys.pop_front();
				}
			}

			void AddKey(TemporaryKeyPtr pTempKey)
			{
				m_listTempKeys.push_back(pTempKey);
			}
		};

		typedef std::map< MetaEntityPtr, RowAccessRestrictions* >	RowLevelAccessPtrMap;
		typedef RowLevelAccessPtrMap::iterator										RowLevelAccessPtrMapItr;

		typedef std::map< MetaEntityPtr, std::string >						RLSPredicateMap;
		typedef RLSPredicateMap::iterator													RLSPredicateMapItr;

		public:
			//! Holds the in memory only sysadmin User
			/*! sysadmin is an in-memory only user that is created automatically at system start.
					It has the most relaxed pewrmissions possible. It can't be configured or restricted
					nor can the password be changed in any other way than by making programmatic changes.
			*/
			static UserPtr														M_sysadmin;

		protected:
			static UserID															M_uNextInternalID;						//!< If no internal ID is passed to the constructor, use this and decrement afterwards (used for meta dictionary definition meta obejcts)
			static UserPtrMap													M_mapUser;										//!< Holds all User objects in the system indexed by m_uID

			boost::recursive_mutex										m_mtxExcl;										//!< Protects security setting changes
			UserID																		m_uID;												//!< The Users ID
			std::string																m_strName;										//!< The Users name
			Data																			m_Password;										//!< The Users Password
			bool																			m_bEnabled;										//!< Must be true for use in Sessions
			RoleUserPtrList														m_listRoleUsers;							//!< RoleUsers associated with this
			std::string																m_strWarehouseAreaAccessList; //!< Semicolon seperated list of WarehouseArea.ID to which a user has access (if empty, user can access all warehouses)
			std::string																m_strCustomerAccessList;			//!< Semicolon seperated list of Customer.ID to which a user has access (if empty, user can access all warehouses)
			std::string																m_strTransmodeAccessList;			//!< Semicolon seperated list of Transmode.ID to which a user has access (if empty, user can access all warehouses)
			int																				m_iPWDAttempts;								//!< Number of times user has incorrectly entered password
			D3Date																		m_dtPWDExpires;								//!< Date when password expires
			bool																			m_bTemporary;									//!< If set, the password is temporary and must be reset on first logging		
			std::string																m_strLanguage;								//!< The Users language
			RowLevelAccessPtrMap											m_mapRowAccessRestrictions;		//!< A map keyed by MetaEntityPtr that (if row level access restrictions exist at all) stores RowAccessRestrictions for each restricted meta entity.
			RLSPredicateMap														m_mapRLSPredicates;						//!< A map keyed by MetaEntityPtr that stores the WHERE predicate enforcing RLS.

			//!	Dummy ctor needed for D3 class factory
			User()	{ assert(false); }

			//!	Construct a User from its database representation
			User(D3UserPtr pD3User);

			//!	This ctor allows construction of role objects to access meta dictionary objects
			User(const std::string& strName, const Data& password, bool m_bEnabled);

			//! ctor helper
			void																			Init(D3UserPtr pD3User = NULL);

			//! Delete all User objects
			static void																DeleteAll();

			//! Save RoleUser
			void																			SaveRoleUsers();
			//! Delete RoleUser
			void																			DeleteRoleUsers();
			//! Save and delete RoleUser
			static void																SaveAndDeleteRoleUsers();

			//! Save the users pending sessions
			void																			SaveSessions()						{ SaveRoleUsers(); }
			//! Delete the users pending sessions
			void																			DeleteSessions();
			//! Save and delete the users pending sessions
			void																			SaveAndDeleteSessions()		{ SaveSessions(); DeleteSessions(); }

		public:
			//!	The destructor deletes all attached RoleUsers
			~User();

			//! Return the list of all users in the system
			static UserPtrMap&												GetUsers()						{ return M_mapUser; }
			//! Given a D3User ID, find the matching User
			static UserPtr														GetUserByID(UserID uID)
			{
				UserPtrMapItr	itr = M_mapUser.find(uID);
				return (itr == M_mapUser.end() ? NULL : itr->second);
			}
			//! Given a user name and password, returns the matching User object (if it exists)
			static UserPtr														GetUser(const std::string& strName, const std::string& strBase64PWD);
			//! Given a user name and password, fetches the matching User object (if it exists), updates its password and returns it
			static UserPtr														UpdateUserPassword(const std::string& strName, const std::string& strCrntBase64PWD, const std::string& strNewBase64PWD);

			//! Get RoleUsers returns the list of RoleUser obejcts assigned to this.
			virtual RoleUserPtrList&									GetRoleUsers()				{ return m_listRoleUsers; }

			//@{ Attributes accessors
			//! Return this users ID
			virtual UserID														GetID() const					{ return m_uID; }
			//! Returns the name of this User
			virtual const std::string&								GetName() const				{ return m_strName; }
			//! Returns the language of this User
			virtual const std::string&								GetLanguage() const		{ return m_strLanguage; }
			//! Returns the password of this User
			virtual const Data&												GetPassword() const		{ return m_Password; }
			//! Returns true if this Role is enabled (must be enabled to establish a Session)
			virtual bool															IsEnabled() const			{ return m_bEnabled; }
			//! Return the semicolon seperated list of WarehouseArea.ID to which a user has access (if empty, user can access all warehouses)
			virtual const std::string&								GetWarehouseAreaAccessList() const	{ return m_strWarehouseAreaAccessList; }
			//! Return the semicolon seperated list of Customer.ID to which a user has access (if empty, user can access all warehouses)
			virtual const std::string&								GetCustomerAccessList() const	{ return m_strCustomerAccessList; }
			//! Return the semicolon seperated list of Transmode.ID to which a user has access (if empty, user can access all warehouses)
			virtual const std::string&								GetTransmodeAccessList() const	{ return m_strTransmodeAccessList; }
			//! Returns the number of times the user has consecutively entered the wrong password
			virtual int																GetPWDAttempts()			{ return m_iPWDAttempts; }
			//! Returns the date the password expires
			virtual D3Date														GetPWDExpires()				{ return m_dtPWDExpires; }
			//! Returns true if the password is a temporary password that must be reset on first log-in
			virtual bool															GetTemporary()				{ return m_bTemporary; }


			//! Enables this role
			virtual void															Enable()							{ m_bEnabled = true; }
			//! Enables this role
			virtual void															Disable()							{ m_bEnabled = false; }
			//@}

			//!	Update this from the values of the passed in object
			virtual void															Refresh(D3UserPtr pD3User);
			//! Loads the corresponding D3User record into the passed in DatabaseWorkspace and returns it
			virtual D3UserPtr													GetD3User(DatabaseWorkspace& dbWS);

			//! Inserts this into the passed-in stream as a JSON object.
			std::ostream &														AsJSON(std::ostream & ostrm);
			//! Inserts all Users currently resident into the passed-in stream as a JSON objects.
			static std::ostream &											UsersAsJSON(std::ostream & ostrm);

			//! Sets the role specific permission for RowLevel access to a specific MetaEntity
			virtual void															SetPermissions(D3RowLevelPermissionPtr	pD3RowLevelPermissions);
			//! Deletes the role specific permission for RowLevel access to a specific MetaEntity
			virtual void															DeletePermissions(D3RowLevelPermissionPtr	pD3RowLevelPermissions);

			//! Helper to verify filters are as expected
			void																			DumpAllRLSFilters();
			//! Given an Entity object, returns a string containing WHERE predicate that ensures records are filtered such that row level security restrictions are observed
			virtual std::string&											GetRLSPredicate(MetaEntityPtr pEntity);
			//! Given an Entity object, return false if a user is blocked from accessing the object via row level security
			virtual bool															HasAccess(EntityPtr pEntity);

		protected:
			//! Notification that a RoleUser has been added
			virtual void															On_RoleUserCreated(RoleUserPtr	pRoleUser);
			//! Notification that a RoleUser has been deleted
			virtual void															On_RoleUserDeleted(RoleUserPtr	pRoleUser);
			//! Removes any row level security settings a user may have
			void																			DropRowLevelSecurity();
			//! Establishes row level security from related D3RowLevelPermission objects associated with this user
			void																			SetRowLevelSecurity(D3UserPtr pD3User);

			//! Create all composite RLS Predictaes for all meta databases and meta entities
			void																			CreateRLSPredicates();
			//! Create all primitive RLS Predictaes for all meta databases and meta entities
			void																			CreatePrimitiveRLSPredicates(RLSPredicateMap & mapPrimitiveRLSPredicates);
			//! Given an Entity, returns true if the current user can access it (does not look at parents)
			virtual bool															HasExplicitAccess(EntityPtr pEntity);
	};





	//! We store all RoleUser objects in a map keyed by ID
	/*! Note that objects which are not stored in the meta dictionary (e.g. RoleUser objects
			created at system startup to enable access to the the MetaDictionary database)
			are assigned RoleUser::M_uNextInternalID which is then decremented by 1. The
			initial value	of RoleUser::M_uNextInternalID is ROLEUSER_ID_MAX.
			RoleUser objects without a physical representation on disk are therefore
			identifiable if GetID() returns a value > RoleUser::M_uNextInternalID.
	*/
	typedef std::map< RoleUserID, RoleUserPtr >		RoleUserPtrMap;
	typedef RoleUserPtrMap::iterator							RoleUserPtrMapItr;

	//! RoleUser objects specify the roles users can assume to access the system
	class D3_API RoleUser : public Object
	{
		friend class MetaDatabase;
		friend class Role;
		friend class User;
		friend class Session;
		friend class D3RoleUser;

		// Standard D3 stuff
		D3_CLASS_DECL(RoleUser);

		public:
			//! Holds the in memory only sysadmin RoleUser
			/*! sysadmin is an in-memory only role-user that is created automatically at system start.
			*/
			static RoleUserPtr												M_sysadmin;

		protected:
			static RoleUserID													M_uNextInternalID;				//!< If no internal ID is passed to the constructor, use this and decrement afterwards (used for meta dictionary definition meta obejcts)
			static RoleUserPtrMap											M_mapRoleUser;						//!< Holds all RoleUser objects in the system indexed by m_uID
			static RoleUserPtr												M_twoRoleUser;						//!< Holds the in memory two only two RoleUser

			RoleUserID																m_uID;										//!< The RoleUsers ID
			RolePtr																		m_pRole;									//!< The Role this belongs to
			UserPtr																		m_pUser;									//!< The User this belongs to
			SessionPtrList														m_listSessions;						//!< Currently active sessions for this RoleUser

			//!	Dummy ctor needed for D3 class factory
			RoleUser()	{ assert(false); }

			//!	Construct a RoleUser from its database representation
			RoleUser(D3RoleUserPtr pD3RoleUser);

			//!	This ctor allows construction of role objects to access meta dictionary objects
			RoleUser(RolePtr pRole, UserPtr pUser);

			//!	Simply notify parents that this is dead
			~RoleUser();

			//! ctor helper
			void																			Init();

			//! Save Sessions
			void																			SaveSessions();
			//! Delete Sessions
			void																			DeleteSessions();
			//! Save and delete
			void																			SaveAndDeleteSessions()													{ SaveSessions(); DeleteSessions(); }

		public:

			//! Returns true if this RoleUser is enabled (both Role and User must be enabled for this to return true)
			bool																			IsEnabled()																			{ return m_pRole->IsEnabled() && m_pUser->IsEnabled(); }

			//@{ Attributes accessors
			//! Return this RoleUsers ID
			virtual RoleUserID												GetID() const																		{ return m_uID; }
			//! Return this' Role
			virtual RolePtr														GetRole()																				{ return m_pRole; }
			//! Return this' User
			virtual UserPtr														GetUser()																				{ return m_pUser; }
			//@}

			//! Return the list of all RoleUsers in the system
			static RoleUserPtrMap&										GetRoleUsers()																	{ return M_mapRoleUser; }
			//! Givem a D3RoleUser ID, find the matching RoleUser
			static RoleUserPtr												GetRoleUserByID(RoleUserID uID)
			{
				RoleUserPtrMapItr	itr = M_mapRoleUser.find(uID);
				return (itr == M_mapRoleUser.end() ? NULL : itr->second);
			}

			//!	Update this from the values of the passed in object
			virtual void															Refresh(D3RoleUserPtr pD3RoleUser);
			//! Loads the corresponding D3RoleUser record into the passed in DatabaseWorkspace and returns it
			virtual D3RoleUserPtr											GetD3RoleUser(DatabaseWorkspace& dbWS);

			//! Inserts this into the passed-in stream as a JSON object.
			std::ostream &														AsJSON(std::ostream & ostrm);
			//! Inserts all RoleUsers currently resident into the passed-in stream as a JSON objects.
			static std::ostream &											RoleUsersAsJSON(std::ostream & ostrm);

		protected:
			//! Notification that a Session has been added
			virtual void															On_SessionCreated(SessionPtr	pSession);
			//! Notification that a Session has been deleted
			virtual void															On_SessionDeleted(SessionPtr	pSession);
	};





	//! We store all Session objects in a map keyed by ID.
	/*! All D3::Session objects also have a physical representation in the form of a D3::D3Session
			object with an identical ID. This collection allows quick navigation between the two.

			\note One day we'll get rid of this duplication
	*/
	typedef std::map< SessionID, SessionPtr >		SessionPtrMap;
	typedef SessionPtrMap::iterator							SessionPtrMapItr;

	// Need this forward declaration for class Session
	class LockedSessionPtr;

	//! Session objects are assigned to external clients.
	/*! The purpose of the session class is to provide functionality to external clients to communicate
			with D3.

			When a session is created initially, a database workspace is created and attached to it. This
			database workspace becomes the default workspace and unless it is deleted, it is the one that
			will be returned each time you call GetDefaultDatabaseWorkspace(). Clients can create additional
			database workspaces by calling CreateDatabaseWorkspace(). All these workspaces will be destroyed
			when the session is deleted.
	*/
	class D3_API Session
	{
		friend class RoleUser;
		friend class DatabaseWorkspace;
		friend class LockedSessionPtr;

		protected:
			static SessionPtrMap													M_mapSession;							//!< Holds all Session objects in the system indexed by m_uID
			static boost::recursive_mutex									M_mtxExclusive;						//!< Mutex serialising update access to M_mapSession

			boost::recursive_try_mutex										m_mtxExcl;								//!< Access to a session is mutually exclusive
			ExceptionContext															m_excptCtx;								//!< Each session has its own exception context
			DatabaseWorkspacePtrList											m_listDBWS;								//!< Each session can have one or more database workspaces attached
			SessionID																			m_uID;										//!< each session has a unique ID
			D3Date																				m_dtSignedIn; 						//!< Date/time this session was started
			RoleUserPtr																		m_pRoleUser;							//!< RoleUser that onws the session
			time_t																				m_tLastAccessed;					//!< Number of seconds elapsed since midnight (00:00:00), January 1, 1970 when this was last accessed (set by SessionI::Touch())

		public:
			//! Protected ctor()'s (this class creates instances by itself)
			/*! You will never create instances of this class manually. Use SessionManagerI::CreateSession
					to create an instance.
			*/
			Session(RoleUserPtr & pRoleUser);

			//! The destructor closes all instances of this and deletes all it's associated meta objects.
			/*!	While the destructor attempts to do a proper clean up, you should not rely on this...
					\todo document this... (there needs to be a session monitor thread that periodically
					deletes timed-out sessions
			*/
			~Session();

			//! Return the list of all Sessions in the system
			static SessionPtrMap&											GetSessions()																		{ return M_mapSession; }

			//! This static method simply destroys all sessions which have been inactive for uiSessionTimeout seconds
			static void																KillTimedOutSessions(unsigned int uiSessionTimeout);

			//! Touch simply renews the sessions m_tLastAccessed time
			void Touch();

			//! Return this Sessions ID
			virtual SessionID													GetID() const																		{ return m_uID; }

			//! Return the sessions RoleUser
			RoleUserPtr																GetRoleUser()																		{ return m_pRoleUser; }

			//! Return last Signed-In Date
			const D3Date&															GetSignedInDate() const													{ return m_dtSignedIn; }

			//! Creates a new database workspace
			/*! The new workspace returned is owned by the session. In other words, if the session is destroyed,
					so is the database workspace. It is save to delete the workspace that this method returns, since
					the workspace will notify the Session when it is destroyed so that the Session can take care of
					necessary houskeeping.
			*/
			DatabaseWorkspacePtr											CreateDatabaseWorkspace();

			//! Returns the default database workspace
			DatabaseWorkspacePtr											GetDefaultDatabaseWorkspace()
			{
				return m_listDBWS.empty() ? NULL : m_listDBWS.front();
			}

			//! Returns a list of database workspaces currently assigned to this session (the forst member in the list is the default database workspace).
			DatabaseWorkspacePtrList&									GetDatabaseWorkspaces()
			{
				return m_listDBWS;
			}

			//! Expects an ObjectID JSON structure as parameter and tries to fetch and return the matching object from m_dbWS
			D3::EntityPtr															GetEntity(const std::string& strJSONObjectID, bool bRefresh);

			//! Returns the last error message for this session
			std::string																GetLastError()
			{
				return m_excptCtx.GetLastError();
			}

			//! Allows you to set a new message that will be returned by the next call to GetLastError()
			void																			SetLastError(const std::string& strLastError)
			{
				m_excptCtx.SetLastError(strLastError);
			}

			//! Loads the corresponding D3Session record into the passed in DatabaseWorkspace and returns it
			virtual D3SessionPtr											GetD3Session(DatabaseWorkspace& dbWS);

		protected:
			//! Notification sent by a DatabaseWorkspace owned by this that it has been destroyed
			void																			On_DatabaseWorkspaceDestroyed(DatabaseWorkspacePtr pDBWS);

			//! Given a D3Session ID, find the matching Session
			/*! The method tries to find the session with the speciofied ID.

					It then sets the passed in pLockedSession using the LockedSessionPtr::Assign(pSession, true)
					method. pSession passed to Assign can be NULL if the session doesn't exist (typically expired).
			*/
			static void																GetSessionByID(SessionID uID, LockedSessionPtr* pLockedSession);

			//! Save the session assumes the session has ended and persists the changes in the database
			void																			Save();
	};




	//! External clients use this class to get access to a session. Sessions obtained in this way are thread-safe.
	class D3_API LockedSessionPtr
	{
		friend class Session;

		protected:
			SessionPtr																		m_pSession;
			boost::recursive_try_mutex::scoped_try_lock*	m_pLock;

			LockedSessionPtr(SessionPtr pSession);

		public:
			LockedSessionPtr() : m_pSession(NULL), m_pLock(NULL) {}
			LockedSessionPtr(LockedSessionPtr& rhs);
			~LockedSessionPtr();

																				operator bool ()														{ return m_pSession ? true : false; }
			SessionPtr												operator -> ()															{ return m_pSession; }
																				operator Session* ()												{ return m_pSession; }
			bool															operator == (const LockedSessionPtr& rhs)		{ return m_pSession == rhs.m_pSession; }
			bool															operator != (const LockedSessionPtr& rhs)		{ return m_pSession != rhs.m_pSession; }

			//! This method allows you to Delete the actual session attached to this
			void															Delete();

			//! This method allows you to create a new session
			/*! Clients should make sure that the returned object is short lived as no other
			    threads can access the session until the returned object is deleted.

					If the session could not be created, pSession.operator bool() returns false upon return
			*/
			static void												CreateSession(LockedSessionPtr & pSession, RoleUserPtr & pRoleUser);

			//! This method is the main access point to existing sessions for external clients
			/*! Clients should make sure that the returned object is short lived as no other
			    threads can access the session until the returned object is deleted

					If the session could not be retrieved or locked, pSession.operator bool() returns false upon return
			*/
			static void												GetSessionByID(LockedSessionPtr & pSession, SessionID uID);

		protected:
			//! Helper allocating resources
			/*! The method calls Release() before doing anything.

					If the session passed in is not NULL, the method will block and wait until the
					Session can be locked if bTryLock is false or it will try to set a lock without
					blocking.
			*/
			void															Assign(SessionPtr pSession, bool bTryLock = false);

			//! Helper freeing allocated resources
			void															Release();
			//!

			LockedSessionPtr&									operator=(LockedSessionPtr& rhs);
	};

}	// end namespace D3

#endif
