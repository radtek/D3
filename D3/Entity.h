#ifndef INC_D3_ENTITY_H
#define INC_D3_ENTITY_H

// MODULE: Entity header
//;
// ===========================================================
// File Info:
//
// $Author: pete $
// $Date: 2014/09/10 04:59:43 $
// $Revision: 1.46 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 24/09/02 - R1 - Hugp
//
// Created Class
//
//
// 12/03/03 - R2 - Hugp
//
// Added ObjectLink support.
// Enable Updates through other DB objects than the containing DB
//
//
// 05/04/03 - R3 - Hugp
//
// - Added HasObjectLink method
//
//
// 08/05/03 - R4 - Hugp
//
// Added SMUCKERS specific code
//
// -----------------------------------------------------------


// @@DatatypeInclude
#include "D3.h"
#include "D3MDDB.h"
#include "D3BitMask.h"
#include "Database.h"
#include "Exception.h"

// Needs JSON
#include <json/json.h>

//#include <odbc++/connection.h>

/*
#define OTL_ORA8 // Compile OTL 4/OCI8
#define OTL_STL // Turn on STL features
#ifndef OTL_ANSI_CPP
#define OTL_ANSI_CPP // Turn on ANSI C++ typecasts
#endif
#include <otlv4.h> // include the OTL 4 header file
*/
// @@End

extern const char szMetaEntityFlags[];					// solely used to ensure type safety (no implementation needed)
extern const char szMetaEntityPermissions[];		// solely used to ensure type safety (no implementation needed)

namespace D3
{
	//! We store all MetaEntity objects in a map keyed by ID
	/*! Note that objects which are not stored in the meta dictionary (e.g. MetaEntity objects
			created at system startup to enable access to the the MetaDictionary database)
			are assigned MetaEntity::M_uNextInternalID which is then decremented by 1. The
			initial value	of MetaEntity::M_uNextInternalID is ENTITY_ID_MAX.
			MetaEntity objects without a physical representation on disk are therefore
			identifiable if GetID() returns a value > MetaEntity::M_uNextInternalID.
	*/
	typedef std::map< EntityID, MetaEntityPtr >				MetaEntityPtrMap;
	typedef MetaEntityPtrMap::iterator								MetaEntityPtrMapItr;

	//! The MetaEntity class keeps track of the intrinsics of a database table.
	/*! MetaEntity objects are part of a MetaDatabase. They maintain the following
			information:

			- Associated MetaColumn objects

			- Associated Primary, Secondary, Foreign and Conceptual MetaKey objetcs

			- Associated Parent MetaRelation objects (a Parent MetaRelation is a

				MetaRelation whos target key si a member of this Entity)

			Instances of this class are created by MetaDatabase::InitMetaDictionary()
			and MetaDatabase::LoadMetaDictionaryData().
	*/
	class D3_API MetaEntity : public Object
	{
		friend class MetaDatabase;
		friend class MetaColumn;
		friend class MetaColumnString;
		friend class MetaColumnChar;
		friend class MetaColumnShort;
		friend class MetaColumnBool;
		friend class MetaColumnInt;
		friend class MetaColumnLong;
		friend class MetaColumnFloat;
		friend class MetaColumnDate;
		friend class MetaColumnBlob;
		friend class MetaKey;
		friend class MetaRelation;
		friend class Entity;
		friend class ODBCDatabase;
		friend class OTLDatabase;

		// Standard D3 stuff
		//
		D3_CLASS_DECL(MetaEntity);

		public:
			//! The Flags bitmask identifies basic characteristics for each type of entity
			struct Flags : public Bits<szMetaEntityFlags>
			{
				BITS_DECL(MetaEntity, Flags);

				//@{ Primitive masks
				static const Mask Hidden;							//!< 0x00000008 - Hide instances of this type from users
				static const Mask Cached;							//!< 0x00000010 - All instances are loaded at program start
				static const Mask ShowCK;							//!< 0x00000020 - If set, the conceptual key will be displayed allong with the title on the detailscreen and along with the arrow in a datatable view
				//@}

				//@{ Combo masks
				static const Mask	Default;						//!< All cleared
				//@}
			};


			//! The Permissions bitmask identifies basic permissions for entity type
			struct Permissions : public Bits<szMetaEntityPermissions>
			{
				BITS_DECL(MetaEntity, Permissions);

				//@{ Primitive masks
				static const Mask Insert;							//!< 0x00000001 - Records can be Inserted
				static const Mask Select;							//!< 0x00000002 - Records can be Selected
				static const Mask Update;							//!< 0x00000004 - Records can be Updated
				static const Mask Delete;							//!< 0x00000008 - Records can be Deleted
				//@}

				//@{ Combo masks
				static const Mask	Default;						//!< Insert | Select | Update | Delete
				//@}
			};


		protected:
			static EntityID					M_uNextInternalID;				//!< If no internal ID is passed to the constructor, use this and decrement afterwards (used for meta dictionary definition meta obejcts)
			static MetaEntityPtrMap	M_mapMetaEntity;					//!< Holds all MetaEntity objects in the system indexed by m_uID

			MetaDatabasePtr					m_pMetaDatabase;					//!< This MetaEntity belongs to this MetaDatabase
			EntityID								m_uID;										//!< This is the index of this so that (MetaEntity::M_mapMetaEntity.find(m_uID)->second == this)
			EntityIndex							m_uEntityIdx;							//!< This is the index of this so that (m_pMetaDatabase->GetMetaEntity(m_uEntityIdx) == this)
			Flags										m_Flags;									//!< See MetaEntity::Flags above
			Permissions							m_Permissions;						//!< See MetaEntity::Permissions above
			std::string							m_strName;								//!< The table name
			std::string							m_strProsaName;						//!< User friendly name
			std::string							m_strDescription;					//!< A description of the entity

			//! The alias for the Entity.
			/*! An alias provides an alternative way of addressing a specific Entity.

					\todo The intention is to use the alias when generating source code. Because we already
					have the InstanceClass name, the alias may not be of much use though.
			*/
			std::string							m_strAlias;

			const Class*						m_pInstanceClass;					//!< pointer to ako Entity::Entity(DatabasePtr pDB, const std::string & strID) function used by the CreateInstance(DatabasePtr) method
			std::string							m_strInstanceInterface;		//!< The name of the interface class that can wrap an instance of this type
			std::string							m_strJSMetaClass;					//!< The name of the JavaScript meta class representing this in the browser
			std::string							m_strJSInstanceClass;			//!< The name of the JavaScript instance class representing this in the browser
			std::string							m_strJSViewClass;					//!< The name of the JavaScript widget that knows how to render instances of this type as a APALUI widget in the browser (full)
			std::string							m_strJSDetailViewClass;		//!< The name of the JavaScript widget that knows how to render instances of this type as a APALUI widget in the browser (detail only)
			MetaColumnPtrVect				m_vectMetaColumn;					//!< A vector containing this' MetaColumnPtr objects
			MetaColumnPtrVect				m_vectMetaColumnFetchOrder;	//!< A vector containing this' MetaColumnPtr objects in the order they must be retrieved from the database
			MetaKeyPtrVect					m_vectMetaKey;						//!< A vector containing this' MetaKeyPtr objects
			MetaRelationPtrVect			m_vectChildMetaRelation;	//!< std::vect of MetaRelation objects where this has the source key
			MetaRelationPtrVect			m_vectParentMetaRelation;	//!< std::vect of MetaRelation objects where this has the target key

			KeyIndex								m_uPrimaryKeyIdx;					//!< The index of the primary MetaKey object in m_vectMetaKey
			KeyIndex								m_uConceptualKeyIdx;			//!< The index of the conceptual MetaKey object in m_vectMetaKey

			short										m_sAssociative;						//!< indicator whether or not this is an associative entity. Innitially -1, but when a call to IsAssociative() is made, will be set to 0 (not associative) or 1 (associative) and future calls to IsAssociative() simply return this value.
			std::string							m_strHSTopicsJSON;				//!< JSON string containing an array of help topics associated with this

			//! ctor() used to instantiate ako MetaEntity objects via the class factory from the meta dictionary entries
			MetaEntity();
			//! The ctor used to initialise meta dictionary accessor MetaEntity objects
			MetaEntity(MetaDatabasePtr pMetaDatabase, const std::string & strName, const std::string & strInstanceClassName, const Flags::Mask & flags = Flags::Default, const Permissions::Mask & permissions = Permissions::Default);

			//! The destructor destroys it's target and source MetaRelation, MetaKey and MetaColumn objects in this order.
			~MetaEntity();

			//! Internal ctor() helper used to instantiate CustomDatabase MetaEntity objects
			virtual void						Init(MetaDatabasePtr pMetaDatabase, EntityID uID, const std::string & strName, const std::string & strInstanceClassName, const Flags::Mask & flags = Flags::Default, const Permissions::Mask & permissions = Permissions::Default);
			//! Internal ctor() helper used to instantiate MetaDictionary MetaEntity objects
			void										Init(const std::string& strInstanceClassName);

		public:

			//! Create an Entity object based on this
			EntityPtr								CreateInstance(DatabasePtr pDB);

			//! Returns the physical name of this entity matching the name of tyhe physical database table.
			const std::string	&			GetName() const											{ return m_strName; }
			//! Returns a name meaningful to the user.
			const std::string	&			GetProsaName() const								{ return m_strProsaName; }
			//! Returns a description giving the user more details about the entity.
			const std::string	&			GetDescription() const							{ return m_strDescription; }
			//! Builds and returns a string consisting of the physical database name, a full stop and the physical name of this.
			std::string							GetFullName() const;
			//! Returns the alias for the database.
			const std::string &			GetAlias() const										{ if (!m_strAlias.empty()) return m_strAlias; return GetName(); }
			//! Returns the meta class name this uses
			const std::string &			GetMetaClassName() const						{ return IAm().Name(); }
			//! Returns the instance class name this uses
			const std::string &			GetInstanceClassName() const				{ return m_pInstanceClass->Name(); }
			//! Returns the name of the interface that is expected to wrap an instance of this type
			const std::string &			GetInstanceInterface()							{ return  m_strInstanceInterface;}
			//! Returns the JavaScript MetaClass to use on the browser side
			const std::string &			GetJSMetaClassName() const					{ return m_strJSMetaClass; }
			//! Returns the JavaScript InstanceClass to use on the browser side
			const std::string &			GetJSInstanceClassName() const			{ return m_strJSInstanceClass; }
			//! Returns the JavaScript ViewClass to use on the browser side
			const std::string &			GetJSViewClassName() const					{ return m_strJSViewClass; }
			//! Returns the JavaScript DetailViewClass to use on the browser side
			const std::string &			GetJSDetailViewClassName() const		{ return m_strJSDetailViewClass; }

			//! Returns the MetaDatabase object of which this is a member
			MetaDatabasePtr					GetMetaDatabase()										{ return m_pMetaDatabase; }

			/** @name Meta Data Accessors
			*/
			//@{
			//! Returns the MetaColumn object which is an autonum (aka IDENTITY column) or NULL if there is none.
			MetaColumnPtr						GetAutoNumMetaColumn();
			//! Returns the MetaColumn object with the specified name.
			MetaColumnPtr						GetMetaColumn(const std::string & strColumnName);
			//! Returns the MetaColumn object with the specified index.
			MetaColumnPtr						GetMetaColumn(ColumnIndex idx)			{ return (idx < m_vectMetaColumn.size() ? m_vectMetaColumn[idx] : NULL); }
			//! Returns a vector of this' MetaColumn objects.
			MetaColumnPtrVectPtr		GetMetaColumns()										{ return &m_vectMetaColumn; }
			//! Returns a vector of this' MetaColumn objects in the order they should be fecthed from the database.
			/*! The reason for this is because streamed columns must always be the last columns to fetch
			*/
			MetaColumnPtrVectPtr		GetMetaColumnsInFetchOrder()				{ return &m_vectMetaColumnFetchOrder; }
			//! Returns the number of MetaColumn objects this has.
			unsigned int						GetMetaColumnCount()								{ return m_vectMetaColumn.size(); }


			//! Returns the MetaKey object with the specified name.
			MetaKeyPtr							GetMetaKey(const std::string & strKeyName);
			//! Returns the MetaKey object with the specified index.
			MetaKeyPtr							GetMetaKey(unsigned int idx)				{ if (idx >= m_vectMetaKey.size()) return NULL; return m_vectMetaKey[idx]; }
			//! Returns a vector of this' MetaKey objects.
			MetaKeyPtrVectPtr				GetMetaKeys()												{ return &m_vectMetaKey; }
			//! Returns the number of MetaKey objects this has.
			unsigned int						GetMetaKeyCount()										{ return m_vectMetaKey.size(); }

			//! Returns the Primary MetaKey object.
			MetaKeyPtr							GetPrimaryMetaKey()									{ return GetMetaKey(m_uPrimaryKeyIdx); }
			//! Returns the Conceptual MetaKey object.
			MetaKeyPtr							GetConceptualMetaKey()							{ return GetMetaKey(GetConceptualKeyIdx()); }


			//! Returns the child MetaRelation object with the specified name.
			MetaRelationPtr					GetChildMetaRelation(const std::string & strColumnName);
			//! Returns the child MetaRelation object with the specified index.
			MetaRelationPtr					GetChildMetaRelation(unsigned int idx)		{ if (idx >= m_vectChildMetaRelation.size()) return NULL; return m_vectChildMetaRelation[idx]; }
			//! Returns a vector of this' child MetaRelation objects.
			MetaRelationPtrVectPtr	GetChildMetaRelations()										{ return &m_vectChildMetaRelation; }
			//! Returns the number of child MetaRelation objects this has.
			unsigned int						GetChildMetaRelationCount()								{ return m_vectChildMetaRelation.size(); }


			//! Returns the parent MetaRelation object with the specified name.
			MetaRelationPtr					GetParentMetaRelation(const std::string & strColumnName);
			//! Returns the parent MetaRelation object with the specified index.
			MetaRelationPtr					GetParentMetaRelation(unsigned int idx)		{ if (idx >= m_vectParentMetaRelation.size()) return NULL; return m_vectParentMetaRelation[idx]; }
			//! Returns a vector of this' parent MetaRelation objects.
			MetaRelationPtrVectPtr	GetParentMetaRelations()									{ return &m_vectParentMetaRelation; }
			//! Returns the number of parent MetaRelation objects this has.
			unsigned int						GetParentMetaRelationCount()							{ return m_vectParentMetaRelation.size(); }
			//@}

			//! GetID returns a unique identifier which can be used to retrieve the same object using GetMetaEntityByID(unsigned int)
			EntityID								GetID() const															{ return m_uID; }
			//! Return the meta relation where GetID() == uID
			static MetaEntityPtr		GetMetaEntityByID(EntityID uID)
			{
				MetaEntityPtrMapItr	itr = M_mapMetaEntity.find(uID);
				return (itr == M_mapMetaEntity.end() ? NULL : itr->second);
			}

			//! Returns a list of all MetaEntities in the system.
			static MetaEntityPtrMap&	GetMetaEntities()												{ return M_mapMetaEntity; };


			//! Returns the ID which uniquely identifies the MetaEntity within its MetaDatabase.
			EntityIndex							GetEntityIdx() const											{ return m_uEntityIdx; }

			//! Get the unique identifier of the primary MetaKey within this' MetaKey stl vector.
			/*! The method returns an index such that the following expression:

					pME->GetMetaKey(pME->GetPrimaryKeyIdx()) == pME->GetPrimaryMetaKey();

					evaluates to true.
			*/
			KeyIndex								GetPrimaryKeyIdx()									{ return m_uPrimaryKeyIdx; }
			//! Get the index to the conceptual MetaKey within this' MetaKey stl vector.
			/*! The method returns an index such that the following expression:

					pME->GetMetaKey(pME->GetConceptualKeyIdx()) == pME->GetConceptualMetaKey();

					evaluates to true.
			*/
			KeyIndex								GetConceptualKeyIdx()								{ return m_uConceptualKeyIdx == D3_UNDEFINED_ID ? m_uPrimaryKeyIdx : m_uConceptualKeyIdx; }

			//@{ Permissions
			//! Returns the Permissions bitmask
			const Permissions &			GetPermissions() const							{ return m_Permissions; }

			//! Returns true if new instances can be inserted into the database.
			bool										AllowInsert() const									{ return (m_Permissions & Permissions::Insert); }
			//! Returns true if new instances can be selected from the database.
			bool										AllowSelect() const									{ return (m_Permissions & Permissions::Select); }
			//! Returns true if existing instances can be updated to the database.
			bool										AllowUpdate() const									{ return (m_Permissions & Permissions::Update); }
			//! Returns true if existing instances can be deleted from the database.
			bool										AllowDelete() const									{ return (m_Permissions & Permissions::Delete); }

			//! Sets the Insert permission to on or off
			void 										AllowInsert(bool bEnable)						{ bEnable ? m_Permissions |= Permissions::Insert : m_Permissions &= ~Permissions::Insert; }
			//! Sets the Select permission to on or off
			void 										AllowSelect(bool bEnable)						{ bEnable ? m_Permissions |= Permissions::Select : m_Permissions &= ~Permissions::Select; }
			//! Sets the Update permission to on or off
			void 										AllowUpdate(bool bEnable)						{ bEnable ? m_Permissions |= Permissions::Update : m_Permissions &= ~Permissions::Update; }
			//! Sets the Delete permission to on or off
			void 										AllowDelete(bool bEnable)						{ bEnable ? m_Permissions |= Permissions::Delete : m_Permissions &= ~Permissions::Delete; }
			//@}


			//@{ Characteristics
			//! returns the characteristics bitmask
			const Flags &						GetFlags() const										{ return m_Flags; }

			//! Returns true if instances are to be hidden from users.
			bool										IsHidden() const										{ return (m_Flags & Flags::Hidden); }
			//! Returns true if instances are cached.
			bool										IsCached() const										{ return (m_Flags & Flags::Cached); }
			//! Returns true if instances are to be displayed with the conceptual key
			bool										IsShowCK() const										{ return (m_Flags & Flags::ShowCK); }

			//! Sets the Hidden to on or off
			void 										Hidden(bool bEnable)								{ bEnable ? m_Flags |= Flags::Hidden : m_Flags &= ~Flags::Hidden; }
			//! Sets the Cached characteristic to on or off
			void 										Cached(bool bEnable)								{ bEnable ? m_Flags |= Flags::Cached : m_Flags &= ~Flags::Cached; }
			//@}


			//! Returns true if this is a purely associative entity or false otherwise
			bool										IsAssociative();

			//! Retrieve all objects of this type from the physical store and associate these with database passed in.
			//* \todo This method needs to be implemented */
			long										LoadAll(DatabasePtr pDB, bool bRefresh = false, bool bLazyFetch = true);

			//! Deletes all Entity objects of this type belonging to the specified database.
			/*! A database object should send this message to each MetaEntity
					before it is being destroyed to ensure that all	D3 resources associated
					with the database are released correctly.
			*/
			void										DeleteAllObjects(DatabasePtr pDatabase);

			//! Debug aid: The method dumps this and (if bDeep is true) all its objects to cout
			void										Dump(int nIndentSize = 0, bool bDeep = true);

			//! Returns all known and accessible instances of D3::MetaEntity as a JSON stream
			static std::ostream &		AllAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm);

			//! Inserts this' accessible components into the passed-in stream as a JSON object. If bBrief is true, it only outputs details about this but not about this' columns, keys and relations
			virtual std::ostream &	AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, bool* pFirstChild = NULL, bool bShallow = false);

			//! Returns this' attributes as an SQL select-list, e.g. attr-1,attr-2,...,attr-n
			/*! The list is populated in the exact same order as the columns appear in the vector returned by
					GetMetaColumnsInFetchOrder().

					If bLazyFetch is true, the data of any column of type D3::MetaColumnBlob will not be loaded.
					Instead, the select list will contain a place holder

					If strPrefix is specified, it will be used to prefix each column name (e.g., if you specified
					"tbl1" as strAlias, the string returned might look like this: "tbl1.attr-1,tbl1.attr-2,...,tbl1.attr-n"
			*/
			std::string							AsSQLSelectList(bool bLazyFetch = true, const std::string & strPrefix = "");

			//! This static method expects a json value that represents the ID for this
			/*! The JSON ID must be a Jason value of type array and have exactly two elements. The first element must be a
					valid ID of a MetaKey and this MetaKey must be the primary key of the MetaEntity to which it belongs. The
					second element is another array which must have exactly as many element as the MetaKey has MetaColumns. Each
					element in the array represents a value for the corresponding key column.
			*/
			static EntityPtr				GetInstanceEntity(Json::Value & jsonID, DatabaseWorkspace& dbWS);

			//! Populates an object from a json structure
			/*! The JSON structure must be in the form {"Name":val_0,"Name":val_1,...,"Name":val_n}
					The method:
						- verifies that all mandatory columns have none null values supplied
						- verifies that the values supplied for the unique keys are unique
						- returns a new object
					This method can throw std::string exceptions with info on the problem
			*/
			EntityPtr								PopulateObject(RoleUserPtr pRoleUser, Json::Value & jsonValues, DatabaseWorkspace& dbWS);

			//! Return true if this is pMetaEntity or pMetaEntity's direct or indirect ancestor
			/*! A cyclic dependencies exist where an object type is directly or indirectly dependent on itself.
					An example would be a hierarchy where a given instance of type x could have many children of
					type x as well as a parent of type x.
			*/
			bool										HasCyclicDependency(MetaEntityPtr pMetaEntity, bool bIgnoreCrossDatabaseRelations = true);

			//! Get all instance resident in thge database
			/*! @param pDB (database) The database to check
					@param el (input) All instances of this meta entity resident in pDB will be stored in this list (see notes below)
					Note: el will be emptied before anything is stored
			*/
			void										CollectAllInstances(DatabasePtr pDB, EntityPtrList & el)			{ GetPrimaryMetaKey()->CollectAllInstances(pDB, el); }

		protected:
			//! AsJSON helper dumping meta columns
			virtual void						MetaColumnsAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm);
			//! AsJSON helper dumping meta keys
			virtual void						MetaKeysAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm);
			//! AsJSON helper dumping child meta relations
			virtual void						ChildMetaRelationsAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm);
			//! AsJSON helper dumping child meta relations
			virtual void						ParentMetaRelationsAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm);

			//! Called by #MetaDatabase::LoadMetaObject().
			/*!	The method expects that the object passed in is an instance of the D3MetaEntity
					MetaEntity and constructs a new MetaEntity object based on pD3ME which acts as
					a template.
					The method propagates the construction process to MetaColumn and MetaKey.
			*/
			static void							LoadMetaObject(MetaDatabasePtr pMetaDatabase, D3MetaEntityPtr pD3ME);

			/*!	@name C++ Source File Generation
					The following methods are invoked by MetaDatabase::CreateCPPFiles().
			*/
			//@{
			//! Add details to a C++ header file which simplyfies the referencing of database elements in C++.
			/*! This method adds enumerators to the header file x.h (where h is the alias of the MetaDatabase
			    of which this is a member) which allow efficient indexed access to this' elements as well as
					elements of this' instances. If this is a specialised class (i.e. instances are not of class
					Entity but of a class derived from Entity) it will also create class x forward declarations as
					well as a xPtr typedef (where x is the name of the instance class).
			*/
			bool										AddToCPPHeader(std::ofstream & fout);
			//! This method creates a CPP header file if this specifies an instance class other than Entity.
			/*! This method generates a specialised C++ header and source file for an entity which uses
					a non-generic instance class (i.e. a class for its instances which is derived from class Entity).

					The meta dictionary contains instance class information for each MetaEntity. If the instance class
					name is empty, D3 will create generic Entity objects to represent instances of the MetaEntity. If
					a name is specified, a class based on Entity with the given name must exist.

					The first time you generate this file, you're faced with the "chicken and egg" issue. The dictionary
					must load correctly before you can generate the header and source file. For the dictionary to load
					correctly, class factories for all specialised Entity classes must exist. The easiest way to bypass
					this problem is build a starter class (which you'll modify later) as follows:

					MySpecialClass.h:
					\code
					#include "D3.h"
					#include "Entity.h"

					namespace D3
					{
						class MySpecialClass : public Entity
						{
							D3_CLASS_DECL(MySpecialClass);

							protected:
								MySpecialClass() {}

							public:
								~MySpecialClass() {}
						};
					}

					\endcode

					MySpecialClass.cpp:
					\code
					#include "MySpecialClass.h"

					namespace D3
					{
						D3_CLASS_IMPL(MySpecialClass, Entity);
					}
					\endcode

					Once you have implemented the above class, you can adjust the dictionary and set the instance
					class name for the appropriate MetaEntity to "MySpecialClass". Once you've done that, you can
					generate the files MySpecialClassBase.h and MySpecialClassBase.cpp using the following code
					fragment:

					\code
					int main(int argc, char* argv[])
					{
						unsigned int						idx;
						MetaDatabasePtr					pMetaDatabase;


						try
						{
							if (InitialiseMetaDictionary())			// calls MetaDatabase::Initialise()
							{
								// Load the meta dictionary
								//
								if (!MetaDatabase::LoadMetaDictionary())
								{
									std::cout << "MetaDatabase::LoadMetaDictionary() failed" << std::endl;
									return false;
								}

								// Create source files for each database
								//
								for ( idx = 0; idx < MetaDatabase::GetMetaDatabases().size(); idx++)
								{
									pMetaDatabase = MetaDatabase::GetMetaDatabases()[idx];

									if (!pMetaDatabase->CreateCPPFiles())
									{
										std::cout << "Failed to create CPP files for database " << pMetaDatabase->GetName().c_str() << "!" << std::endl;
										bSuccess = false;
									}
								}
							}


							UnInitialiseMetaDictionary();			// calls MetaDatabase::UnInitialise()
						}
						catch (Exception & e)
						{
							e.LogError();
							return false;
						}

						return 0;
					}
					\endcode

					In order to keep these generated files in synch with the actual database structure, you'll need
					to run the above code again each time you modify your meta dictionary contents for a a database.

					Going back to the MySpecialClass example, once you have successfully generated the files
					MySpecialClassBase.h and MySpecialClassBase.cpp, you'll want to modify MySpecialClass.h and
					MySpecialClass.cpp as	follows:

					MySpecialClass.h:
					\code
					#include "D3.h"
					#include "MySpecialClassBase.h"

					namespace D3
					{
						class MySpecialClass : public MySpecialClassBase
						{
							D3_CLASS_DECL(MySpecialClass);

							protected:
								MySpecialClass() {}

							public:
								~MySpecialClass() {}
						};
					}
					\endcode

					MySpecialClass.cpp:
					\code
					#include "MySpecialClass.h"

					namespace D3
					{
						D3_CLASS_IMPL(MySpecialClass, MySpecialClassBase);
					}
					\endcode

					You can now enhance this class with any specific methods that you require.

					The method generates the header xBase.h which defines	the class xBase (where x is the name of the
					instance class). xBase provides friendly Column and Relation accessors as well as methods to
					iterate over all instances in a database. xBase is an incomplete class. Once you have generated
					an xBase.h file, you must implement a new class x based on xBase for D3 to function correctly.

					Here is an example of what xBase.h may contain:
					\code
					#ifndef INC_AP3ProductBase_H
					#define INC_AP3ProductBase_H

					#include "Entity.h"
					#include "Column.h"
					#include "Key.h"
					#include "Relation.h"
					#include "P3T3TestDB.h"

					namespace D3
					{

						enum AP3Product_Fields
						{
							AP3Product_ID,
							AP3Product_Type,
							AP3Product_Name,
							AP3Product_StackingRestriction,
							AP3Product_StackingVisibility,
							AP3Product_Height,
							AP3Product_Width,
							AP3Product_Length,
							AP3Product_Weight,
							AP3Product_Strength,
							AP3Product_ItemsCase
						};


						class AP3ProductBase : public Entity
						{
							D3_CLASS_DECL(AP3ProductBase);

							public:
								// Enable iterating over all instances of this
								//
								class iterator : public InstanceKeyPtrSetItr
								{
									public:
										iterator()	{}
										iterator(const InstanceKeyPtrSetItr& itr) : InstanceKeyPtrSetItr(itr) {}

										AP3ProductPtr								operator*();
										iterator&										operator=(const iterator& itr);
								};

								static unsigned int							size(DatabasePtr pDB)				{ return GetAll(pDB)->size(); }
								static bool											empty(DatabasePtr pDB)			{ return GetAll(pDB)->empty(); }
								static iterator									begin(DatabasePtr pDB)			{ return iterator(GetAll(pDB)->begin()); }
								static iterator									end(DatabasePtr pDB)				{ return iterator(GetAll(pDB)->end()); }

								// Navigate through related AP3ProductGroupProducts
								//
								class AP3ProductGroupProducts : public Relation
								{
									D3_CLASS_DECL(AP3ProductGroupProducts);

									protected:
										AP3ProductGroupProducts() {}

									public:
										class iterator : public Relation::iterator
										{
											public:
												iterator()	{}
												iterator(const InstanceKeyPtrSetItr& itr) : Relation::iterator(itr) {}

												AP3ProductGroupProductPtr		operator*();
												iterator&										operator=(const iterator& itr);
										};

										AP3ProductGroupProductPtr		front();
										AP3ProductGroupProductPtr		back();
								};


							protected:
								AP3ProductBase () {}

							public:
								~AP3ProductBase () {}

								// Create a new AP3Product
								//
								static AP3ProductPtr							CreateAP3Product(DatabasePtr pDB)		{ return (AP3ProductPtr) pDB->GetMetaDatabase()->GetMetaEntity(P3T3TestDB_AP3Product)->CreateInstance(pDB); }

								// Collection of all instances of this
								//
								static InstanceKeyPtrSetPtr				GetAll(DatabasePtr pDB);

								// Load product instances
								//
								static AP3ProductPtr							Load(DatabasePtr pDB, const std::string & strID, bool bRefresh = false);
								static void												LoadAll(DatabasePtr pDB, bool bRefresh = false);

								// Relation accessors
								//
								EntityPtr													GetRTCISProduct();
								bool															SetRTCISProduct(EntityPtr pRTCISProduct);


								void															LoadAllAP3ProductGroupProducts(bool bRefresh = false);
								AP3ProductGroupProducts*					GetAP3ProductGroupProducts();

								// Member accessors
								bool                              SetID(const std::string & val)			{ return Column(AP3Product_ID)->SetValue(val); }
								bool                              SetType(char val)										{ return Column(AP3Product_Type)->SetValue(val); }
								bool                              SetName(const std::string & val)		{ return Column(AP3Product_Name)->SetValue(val); }
								bool                              SetStackingRestriction(short val)		{ return Column(AP3Product_StackingRestriction)->SetValue(val); }
								bool                              SetStackingVisibility(char val)			{ return Column(AP3Product_StackingVisibility)->SetValue(val); }
								bool                              SetLength(float val)								{ return Column(AP3Product_Length)->SetValue(val); }
								bool                              SetWidth(float val)									{ return Column(AP3Product_Width)->SetValue(val); }
								bool                              SetHeight(float val)								{ return Column(AP3Product_Height)->SetValue(val); }
								bool                              SetWeight(float val)								{ return Column(AP3Product_Weight)->SetValue(val); }
								bool                              SetStrength(long val)								{ return Column(AP3Product_Strength)->SetValue(val); }
								bool                              SetItemsCase(short val)							{ return Column(AP3Product_ItemsCase)->SetValue(val); }

								const std::string &								GetID()															{ return Column(AP3Product_ID)->GetString(); }
								char															GetType()														{ return Column(AP3Product_Type)->GetChar(); }
								const std::string &								GetName()														{ return Column(AP3Product_Name)->GetString(); }
								short															GetStackingRestriction ()		        { return Column(AP3Product_StackingRestriction)->GetShort(); }
								char															GetStackingVisibility ()		        { return Column(AP3Product_StackingVisibility)->GetChar(); }
								float															GetLength()									        { return Column(AP3Product_Length)->GetFloat(); }
								float															GetWidth()									        { return Column(AP3Product_Width)->GetFloat(); }
								float															GetHeight()									        { return Column(AP3Product_Height)->GetFloat(); }
								float															GetWeight()									        { return Column(AP3Product_Weight)->GetFloat(); }
								long															GetStrength()								        { return Column(AP3Product_Strength)->GetLong(); }
								short															GetItemsCase()							        { return Column(AP3Product_ItemsCase)->GetShort(); }

								ColumnPtr													Column(AP3Product_Fields eCol)			{ return GetColumn((unsigned int) eCol); }
						};



					} // end namespace D3



					#endif
					\endcode

					As stated, you must subclass this class as follows:
					\code
					// A lightweight class based on the generated specialisation
					//
					class AP3Product : public AP3ProductBase
					{
						D3_CLASS_DECL(AP3Product);

						protected:
							AP3Product () {}

						public:
							~AP3Product () {}

					};
					\endcode
					The above class is the minimum requirement for a class based on xBase (or in this case
					AP3ProductBase). You also must implement D3 class support for this class through the
					macro D3_CLASS_IMPL(AP3Product, AP3ProductBase) in your class implementation.

					The main purpose of specialised classes is to provide a simple mechanism to expand the
					functionality of entity objects. For example, you may wish to add a method to AP3Product
					which returns true if the product is a member of a specific group.

					Even if a class does not require specialisation, it may still be advantageous to generate
					and use such classes. If nothing else, it greatly simplifies accessing related objects
					and navigation of related objects.

					Here are a few examples that show how specialisation eases typing. The mehods are based
					on the AP3ProductBase class above:
					\code
					// --------------------------------------------------------
					// Load Product "ABC123" (assumes pDB is a valid Database*)
					//
					//
					// Without AP3ProductBase
					//
					EntityPtr			pEntity;
					TemporaryKey	keyProduct(*(pDB->GetMetaDatabase()->GetMetaEntity(P3T3TestDB_AP3Product)->GetPrimaryMetaKey()));

					keyProduct.GetColumn(P3T3TestDB_AP3Product_ID)->SetValue("ABC123");
					pEntity = pDB->GetMetaDatabase()->GetMetaEntity(P3T3TestDB_AP3Product)->GetPrimaryMetaKey()->LoadObject(&keyProduct, pDB, bRefresh);


					// With AP3ProductBase
					//
					AP3ProductPtr	pProduct = AP3Product::Load(pDB, "ABC123");


					// --------------------------------------------------------
					// Load All Products (assumes pDB is a valid Database*)
					//
					//
					// Without AP3ProductBase
					//
					pDB->GetMetaDatabase()->GetMetaEntity(P3T3TestDB_AP3Product)->LoadAll(pDB);


					// With AP3ProductBase
					//
					AP3Product::LoadAll(pDB);



					// --------------------------------------------------------
					// Iterate through all AP3Product instances (assumes pDB is a valid Database*)
					//
					//
					// Without AP3ProductBase
					//
					InstanceKeyPtrSetItr		itrKey;
					KeyPtr									pKey;
					EntityPtr								pEntity;

					for ( itrKey =  pDB->GetMetaEntity(P3T3TestDB_AP3Product)->GetPrimaryMetaKey()->GetInstanceKeySet(pDB)->begin();
								itrKey != pDB->GetMetaEntity(P3T3TestDB_AP3Product)->GetPrimaryMetaKey()->GetInstanceKeySet(pDB)->end();
								itrKey++)
					{
						pKey = *itrKey;
						pEntity = pKey->GetEntity();
					}


					// With AP3ProductBase
					//
					AP3Product::iterator		itrAll;
					AP3ProductPtr						pProduct;

					AP3Product::LoadAll(pDB);

					for ( itrAll =  AP3Product::begin(pDB);
								itrAll != AP3Product::end(pDB);
								itrAll++)
					{
						pProduct = *itrAll;
						// now work with pProduct
					}



					// --------------------------------------------------------
					// Load and iterate through all related AP3ProductGroupProduct instances (assumes pDB is a valid Database*)
					//
					//
					// Without AP3ProductBase
					//
					RelationPtr							pRel;
					Relation::iterator			itrRel;
					EntityPtr								pEntity;


					pRel = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(P3T3TestDB_AP3Product)->GetChildMetaRelation(P3T3TestDB_AP3Product_CR_ProductGroupProducts));
					pRel->LoadAll(bRefresh);

					for ( itrRel =  pRel->begin();
								itrRel != pRel->end();
								itrRel++)
					{
						pEntity = *itrRel;
					}


					// With AP3ProductBase
					//
					AP3Product::AP3ProductGroupProducts*					pPGPRel;
					AP3Product::AP3ProductGroupProducts::iterator	itrPGPRel;
					AP3ProductGroupProductPtr											pPGP;

					pProduct->LoadAllAP3ProductGroupProducts();
					pPGPRel = pProduct->GetAP3ProductGroupProducts();

					for ( itrPGPRel =  pPGPRel->begin();
								itrPGPRel != pPGPRel->end();
								itrPGPRel++)
					{
						pPGP = *itrPGPRel;
						// now work with pPGP
					}
					\endcode
			*/
			bool										CreateSpecialisedCPPHeader();
			//! This method creates the implementation of xBase.cpp (please refer to #CreateSpecialisedCPPHeader() for more details).
			bool										CreateSpecialisedCPPSource();
			//@}
/*

			//! Create the Table in the physical database
			void										CreateTable(TargetRDBMS eTarget, odbc::Connection* pCon);
			//! Create Sequences for the Table in the physical database
			void										CreateSequence(TargetRDBMS eTarget, odbc::Connection* pCon);
			//! Create Trigger for Sequences for the Table in the physical database
			void										CreateSequenceTrigger(TargetRDBMS eTarget, odbc::Connection* pCon);
			//! Create Indexes for the Table in the physical database
			void										CreateIndexes(TargetRDBMS eTarget, odbc::Connection* pCon);
			//! Create referential integrity triggers for the Table in the physical database
			void										CreateTriggers(TargetRDBMS eTarget, odbc::Connection* pCon);
			//! Create Sequences for the Table in the physical database
			void										DropSequence(TargetRDBMS eTarget, odbc::Connection* pCon);

			//! Create the Table in the physical database
			void										CreateTable(TargetRDBMS eTarget, otl_connect& pCon);

			//! Create Sequences for the Table in the physical database
			void										CreateSequence(TargetRDBMS eTarget, otl_connect& pCon);
			//! Create Trigger for Sequences for the Table in the physical database
			void										CreateSequenceTrigger(TargetRDBMS eTarget,otl_connect& pCon);
			//! Create Indexes for the Table in the physical database
			void										CreateIndexes(TargetRDBMS eTarget, otl_connect& pCon);
			//! Create referential integrity triggers for the Table in the physical database
			void										CreateTriggers(TargetRDBMS eTarget, otl_connect& pCon);

			//! Create Sequences for the Table in the physical database
			void										DropSequence(TargetRDBMS eTarget, otl_connect& pCon);
*/
			/*!	@name Notifications
					Notifications are sent by dependents of this.
			*/
			//@{
			//! A notification sent by manufacturers of MetaEntity instances after a new instance of this has been created
			void										On_BeforeConstructingMetaEntity()	{};
			//! A notification sent by manufacturers of MetaEntity instances after a new instance of this has been fully populated
			/*! We intercept this message to build a meta column vector that contains the columns
					in the order they must be fetched from the database. BLOB like columns must be fetched
					last!
			*/
			void										On_AfterConstructingMetaEntity();


			//! An MetaColumn has been created. The MetaColumn receives its unique ID and is added to the vector of MetaColumn objects this maintains.
			void										On_MetaColumnCreated(MetaColumnPtr pMetaColumn);
			//! An MetaColumn has been deleted. The MetaColumn is deleted from the vector of MetaColumn objects this maintains.
			void										On_MetaColumnDeleted(MetaColumnPtr pMetaColumn);


			//! An MetaKey has been created. The MetaKey receives its unique ID and is added to the vector of MetaKey objects this maintains.
			void										On_MetaKeyCreated(MetaKeyPtr pMetaKey);
			//! An MetaKey has been deleted. The MetaKey is deleted from the vector of MetaKey objects this maintains.
			void										On_MetaKeyDeleted(MetaKeyPtr pMetaKey);


			//! An parent MetaRelation has been created. The MetaRelation receives its unique ID and is added to the vector of parent MetaRelation objects this maintains.
			void										On_ParentMetaRelationCreated(MetaRelationPtr pMetaRelation);
			//! An parent MetaRelation has been deleted. The MetaRelation is deleted from the vector of parent MetaRelation objects this maintains.
			void										On_ParentMetaRelationDeleted(MetaRelationPtr pMetaRelation);


			//! An child MetaRelation has been created. The MetaRelation receives its unique ID and is added to the vector of child MetaRelation objects this maintains.
			void										On_ChildMetaRelationCreated(MetaRelationPtr pMetaRelation);
			//! An child MetaRelation has been deleted. The MetaRelation is deleted from the vector of child MetaRelation objects this maintains.
			void										On_ChildMetaRelationDeleted(MetaRelationPtr pMetaRelation);


			//! An instance of this has been created (ignored).
			void										On_InstanceCreated(EntityPtr pEntity)		{}
			//! An instance of this has been destroyed (ignored).
			void										On_InstanceDeleted(EntityPtr pEntity)		{}

			//! Notification that a database has been created.
			/*! MetaKey objects maintain collections of InstanceKey objects on a per database
					basis. This will pass this message on to each of it's MetaKey objects which will
					take care accordingly.
			*/
			void										On_DatabaseCreated(DatabasePtr pDatabase);

			//! Notification that a database has been destroyed.
			/*! MetaKey objects maintain collections of InstanceKey objects on a per database
					basis. This will pass the message on to each of it's MetaKey objects which will
					take care accordingly.
			*/
			void										On_DatabaseDeleted(DatabasePtr pDatabase);
			//@}

			//! Method is called by MetaDatabase::VerifyMetaData().
			/*! This method verifies that this is correclty configured.

					For this to return true, it must meet the following requirements:

					- Must have at least one cobHasIdentityColumnlumn

					- Can't have more that one AutoNum column

					- If it has an AutoNum Column, the column must be
					  the only column of the primary key

					- Must have one and only one primary key

					- The primary key must be unique and mandatory

					- Any key must have at least one column

					- Any mandatory key must only consist of mandatory columns

					\todo Verify relations also
			*/
			bool										VerifyMetaData();

			//! This method is needed for code generation.
			/*! Given a switched MetaRelation, the method searches for all parent relations which specify
					a relation between the same two keys using the same switch and filles the list passed in accordingly.
			*/
			void										GetSiblingSwitchedParentRelations(MetaRelationPtr pRel, MetaRelationPtrList & listMR);

			//! This method is needed for code generation.
			/*! Given a switched MetaRelation, the method searches for all child relations which specify
					a relation between the same two keys using the same switch and filles the list passed in accordingly.
			*/
			void										GetSiblingSwitchedChildRelations(MetaRelationPtr pRel, MetaRelationPtrList & listMR);

			public:
			//! Return this as a CREATE TABLE ... sql string
			std::string							AsCreateTableSQL(TargetRDBMS eTarget);
			//! Return this as a CREATE sequence ... sql string
			std::string							AsCreateSequenceSQL(TargetRDBMS eTarget);
			//! Return this as a DROP SEQUENCE... sql string
			/*! \note Sequences are not destroyed when an Oracle tablespace is deleted. In Oracle, the ODBCDatabase
								will delete all the know sequences associated with the tablespace that was destroyed.
			*/
			std::string							AsDropSequenceSQL(TargetRDBMS eTarget);

			//! Return this as a CREATE Trigger for sequence ... sql string
			std::string							AsCreateSequenceTriggerSQL(TargetRDBMS eTarget);

			//! Return true if this is pMetaEntity or pMetaEntity's direct or indirect ancestor (the workhores helper)
			bool										HasCyclicDependency(MetaEntityPtr pMetaEntity, std::set<MetaEntityPtr>& setProcessed, bool bIgnoreCrossDatabaseRelations);

			//! Sets the JSON HSTopic array
			void										SetHSTopics(const std::string & strHSTopicsJSON) { m_strHSTopicsJSON = strHSTopicsJSON; }

			//! If your specialised meta entity requires a custom update trigger, overload this method and return the SQL that builds the trigger
			virtual std::string			GetCustomUpdateTrigger(TargetRDBMS eTarget)					{ return ""; }
			//! If your specialised meta entity requires a custom insert trigger, overload this method and return the SQL that builds the trigger
			virtual std::string			GetCustomInsertTrigger(TargetRDBMS eTarget)					{ return ""; }
			//! If your specialised meta entity requires a custom delete trigger, overload this method and return the SQL that builds the trigger
			virtual std::string			GetCustomDeleteTrigger(TargetRDBMS eTarget)					{ return ""; }
	};





	// A map that is required by the Clone() method
	typedef std::map<EntityPtr, EntityPtr>		EntityCloneMap;
	typedef EntityCloneMap::iterator					EntityCloneMapItr;

	//! An instance of the CloningController class deals with issues which may arise while cloning an object
	/*! Some of the issues that could arise are:

			# a cloned object may violate some constraint (like uniqueness)
			# if cloning should be deep, the descendents that should be cloned too must be made resident first
			# not all resident descendents may be desired to be cloned

			The purpose of instances of this class is to deal with these issues.

			When you send a clone() message to an object, one of the first things the method does is to send
			the object a CreateCloningController message. CreateCloningController is a virtual method which can
			be overloaded by subclasses of D3::Entity. If you need to clone an object in a peculiar way, you
			would create a subclass based on CloningController which would overload some of the virtual methods
			this exposes to ensure the object is cloned as shoud be.

			Your D3::Entity subclass would then overload the CreateCloningController and return a new instance
			of your specialised CloningController.

			The default cloning controller only clones the object which is the recipient of the D3::Entity::Clone
			message but no descendents.
	*/
	class CloningController
	{
		friend class D3::Entity;

		protected:
			EntityPtr									m_pOrigRoot;		//!< The object that received the clone message
			SessionPtr								m_pSession;			//!< The session that is used to clone
			DatabaseWorkspacePtr			m_pDBWS;				//!< If new objects need to be made resident, this is the datababase workspace that ought to be used
			Json::Value*							m_pJSONChanges;	//!< Attributes which were supplied for the clone of pOrigRoot
			EntityCloneMap						m_mapClones;		//!< Map the original Entity* with the cloned Entity*

		public:
			//! Simply sets instance attributes
			CloningController(EntityPtr pOrigRoot, SessionPtr pSession, DatabaseWorkspacePtr pDBWS, Json::Value* pJSONChanges)
				: m_pOrigRoot(pOrigRoot),
					m_pSession(pSession),
					m_pDBWS(pDBWS),
					m_pJSONChanges(pJSONChanges)
			{}

			//! Virtual destructor to ensure the Entity::Clone can correctly delete the instance even when a subclass is in use
			virtual ~CloningController() {}

			//! Return true if the children in this relation should be cloned
			virtual	bool							WantCloned(MetaRelationPtr pMetaRelation)					{ return false; }
			//! Return true if the passed in object should be cloned
			virtual	bool							WantCloned(EntityPtr pOrigCandidate)							{ return (m_pOrigRoot == pOrigCandidate); }

			//! Overload this in subclasses if you need to load additional objects to clone into m_pDBWS
			virtual	void							On_BeforeCloning()	{}

			//! This method is sent just before an object is finalised. This is the message you intercept if you need to manually adjust the object (typically to ensure uniqueness requirements are met)
			virtual	void							On_BeforeFinalisingClone(EntityPtr pOriginal, EntityPtr pClone)				{}
	};





	//! The Entity class is an instance of a MetaEntity and provides the basic features required by all instances of a MetaEntity.
	/*! You never create an instance of this nor any of its subclasses explicitly.
			Instead, you create an Entity object through its MetaEntity object's CreateInstance()
			method. This construction process guarantees that all the necessary D3 objects
			are created in tandem.

			You can create your own subclass
			of this class. To ensure that D3 creates an instance of your subclass when
			MetaEntity::CreateInstance() is called on the corresponding MetaEntity object, you set
			the InstanceClass column in the meta data database for your MetaEntity
			accordingly. For this to work correctly, your class must use the D3 Class
			macros D3_CLASS_DECL() and D3_CLASS_IMPL(). If you need to perform any additional initialisation after the
			database has been created, intercept the On_PostCreate() message sent by
			the MetaEntity object but be sure to chain to the ancestores On_PostCreate()
			once you have performed your object specific initialisation actions.
	*/
	class D3_API Entity : public Object
	{
		friend class MetaEntity;
		friend class Database;
		friend class ODBCDatabase;
		friend class OTLDatabase;
		friend class Column;
		friend class ColumnString;
		friend class ColumnChar;
		friend class ColumnShort;
		friend class ColumnBool;
		friend class ColumnInt;
		friend class ColumnLong;
		friend class ColumnFloat;
		friend class ColumnDate;
		friend class InstanceKey;
		friend class Relation;
		friend class ResultSet;

		// Standard D3 stuff
		//
		D3_CLASS_DECL(Entity);

		// 12/03/03 - R2 - Hugp
		public:
			enum UpdateType
			{
				SQL_None,
				SQL_Update,
				SQL_Delete,
				SQL_Insert
			};

		protected:
			#define D3_ENTITY_DIRTY					0x01
			#define D3_ENTITY_NEW						0x02
			#define D3_ENTITY_DELETE				0x04
			#define D3_ENTITY_POPULATING		0x08			// 12/03/03 - R2 - Hugp
			#define D3_ENTITY_DESTROYING		0x10
			#define D3_ENTITY_MARKED				0x20
			#define D3_ENTITY_CONSTRUCTING	0x40			// 06/11/08 - R2 - Hugp
			#define D3_ENTITY_TRACE_DELETE	0x80

		protected:
			MetaEntityPtr						m_pMetaEntity;				//!< The meta entity of which this is an instance
			DatabasePtr							m_pDatabase;					//!< This object belongs to this database
			unsigned short					m_uFlags;							//!< Status indicator (see #defines above)
			ColumnPtrVect						m_vectColumn;					//!< A vector holding ako ColumnPtr objects
			InstanceKeyPtrVect			m_vectInstanceKey;		//!< A vector holding ako InstanceKeyPtr objects
			DatabaseRelationVect		m_vectParentRelation;	//!< Relations where this is the parent key (this owns the relations in this list)
			DatabaseRelationVect		m_vectChildRelation;	//!< Relations where this is the child key (this does NOT own the relations in this list)
			ResultSetPtrListPtr			m_pListResultSet;			//!< If this is a member of a ResultSet this list contains all the result sets
			TemporaryKeyPtr					m_pOriginalKey;				//!< Normally NULL, but will be set to hold the original primary key if a primary key column was modified

			//! Invoked by MetaEntity::CreateInstance(DatabasePtr)
			Entity ()	: m_pMetaEntity(NULL), m_pDatabase(NULL), m_uFlags(D3_ENTITY_NEW), m_pListResultSet(NULL), m_pOriginalKey(NULL) {}

		public:
			//! Ensures related objects are removed from memory
			virtual ~Entity();

			//! Return MetaEntity of which this is an instance
			MetaEntityPtr						GetMetaEntity()									{ return m_pMetaEntity; }

			//! Return Database of which this is an instance
			DatabasePtr							GetDatabase()										{ return m_pDatabase; }


			/** @name  Data Accessors
			*/
			//@{
			//! Returns this' instance Column object which matches the MetaColumn passed in
			ColumnPtr								GetColumn(MetaColumnPtr pMC);
			//! Returns the Column object with the specified name.
			ColumnPtr								GetColumn(const std::string & strColumnName);
			//! Returns the Column object with the specified index.
			ColumnPtr								GetColumn(unsigned int idx)			{ if (idx >= m_vectColumn.size()) return NULL; return m_vectColumn[idx]; }
			//! Returns a vector of this' Column objects.
			ColumnPtrVectPtr				GetColumns()										{ return &m_vectColumn; }
			//! Returns the number of Column objects this has.
			unsigned int						GetColumnCount()								{ return m_vectColumn.size(); }

			//! Returns this' instance InstanceKey object which matches the MetaKey passed in
			InstanceKeyPtr					GetInstanceKey(MetaKeyPtr pMK);
			//! Returns the InstanceKey object with the specified name.
			InstanceKeyPtr					GetInstanceKey(const std::string & strInstanceKeyName);
			//! Returns the InstanceKey object with the specified index.
			InstanceKeyPtr					GetInstanceKey(unsigned int idx){ if (idx >= m_vectInstanceKey.size()) return NULL; return m_vectInstanceKey[idx]; }
			//! Returns a vector of this' InstanceKey objects.
			InstanceKeyPtrVectPtr		GetInstanceKeys()								{ return &m_vectInstanceKey; }
			//! Returns the number of InstanceKey objects this has.
			unsigned int						GetInstanceKeyCount()						{ return m_vectInstanceKey.size(); }

			//! Returns the Primary InstanceKey object.
			InstanceKeyPtr					GetPrimaryKey()									{ return GetInstanceKey(m_pMetaEntity->GetPrimaryKeyIdx()); }
			//! Returns the original primary key
			/*! Returns the original primary key. The original primary key is different from the current
					primary key if any of the primary key columns have changed.
			*/
			KeyPtr									GetOriginalPrimaryKey();

			//! Returns the Conceptual InstanceKey object.
			InstanceKeyPtr					GetConceptualKey()							{ return GetInstanceKey(m_pMetaEntity->GetConceptualKeyIdx()); }

			//! Returns the parent Relation object with the specified name.
			RelationPtr							GetParentRelation(const std::string & strName, DatabasePtr pDB = NULL){ return GetParentRelation(m_pMetaEntity->GetParentMetaRelation(strName), pDB); }
			//! Returns the parent Relation object with the specified index.
			RelationPtr							GetParentRelation(unsigned int idx, DatabasePtr pDB = NULL)						{ return GetParentRelation(m_pMetaEntity->GetParentMetaRelation(idx), pDB); }

			//! Returns the instance parent Relation matching the MetaRelation.
			/*! @param pMR	The MetaRelation which must be a member of this' MetaEntity's
											ParentRelation collection.
					@param pDB	The database to which the parent is expected to belong. There
											is really only one case where it is important you pass the correct
											database pointer in: if the child is a cached entity and this is not.
											In this case, the relationship is typed by the parents m_pDatabase.
											See also note below.
					@return			returns a Relation object
					\note It is very bad practise to mark entities as cached which have relations
					with non cached entities. Resolving such relations requires a significant
					overhead quite possibly nullifying the benefits caching brings.			*/
			RelationPtr							GetParentRelation(MetaRelationPtr pMR, DatabasePtr pDB = NULL);
			//! Returns the number of parent Relation objects this has.
			unsigned int						GetParentRelationCount()				{ return m_vectParentRelation.size(); }

			//! Returns the child Relation object with the specified name.
			RelationPtr							GetChildRelation(const std::string & strName, DatabasePtr pDB = NULL)	{ return GetChildRelation(m_pMetaEntity->GetChildMetaRelation(strName), pDB); }
			//! Returns the child Relation object with the specified index.
			RelationPtr							GetChildRelation(unsigned int idx, DatabasePtr pDB = NULL)						{ return GetChildRelation(m_pMetaEntity->GetChildMetaRelation(idx), pDB); }

			//! Set's the parent of a relation
			void										SetParent(MetaRelationPtr pMetaRelation, EntityPtr pEntity);


			//! Returns the instance child Relation matching the MetaRelation.
			/*! @param pMR	The MetaRelation which must be a member of this' MetaEntity's
											Child Relation collection.
					@param pDB	The database to which the child is expected to belong. There
											is really only one case where it is important you pass the correct
											database pointer in: if this is a cached entity and the child is not.
											In this case, the relationship is typed by the parents m_pDatabase.
					@return			returns a Relation object			*/
			RelationPtr							GetChildRelation(MetaRelationPtr pMR, DatabasePtr pDB = NULL);
			//! Returns the number of child Relation objects this has.
			unsigned int						GetChildRelationCount()										{ return m_vectChildRelation.size(); }
			//@}

			//! Exception context accessor
			ExceptionContextPtr			GetExceptionContext()											{ return m_pDatabase->GetDatabaseWorkspace()->GetExceptionContext(); }

			//! Debug helper
			virtual void						Dump (int nIndentSize = 0, bool bDeep = false);
			//! Returns a string containing DatabaseName.EntityName[CKCol1][CKCol2]...[CKColn] (CK is the conceptual key), or [Col1/Col2/.../Coln] if AllColumns
			virtual std::string			AsString (bool bAllColumns = false);

			//! Returns true if this requires updating.
			bool										NeedsUpdate()							{ return GetUpdateType() != SQL_None; }

			//! Returns true if this has been tempered with.
			bool										IsDirty();
			//! Returns true if this is a new object (i.e. has no known counterpart in the database)
			bool										IsNew()									  { return m_uFlags & D3_ENTITY_NEW ? true : false; }
			//! Returns true if this has been marked for deletion.
			bool										IsDeleted()								{ return m_uFlags & D3_ENTITY_DELETE ? true : false; }
			//! Returns true if this is currently being populated
			bool										IsPopulating()						{ return m_uFlags & D3_ENTITY_POPULATING ? true : false; }
			//! Returns true if this is currently being constructed
			bool										IsConstructing()					{ return m_uFlags & D3_ENTITY_CONSTRUCTING ? true : false; }
			//! Returns true if this is currently in the process of being destroyed
			bool										IsDestroying()						{ return m_uFlags & D3_ENTITY_DESTROYING ? true : false; }
			//! Returns true if this is marked
			bool										IsMarked()								{ return m_uFlags & D3_ENTITY_MARKED ? true : false; }
			//! Returns the name of the user that holds a lock or an empty string if no lock is held
			/*! Notes:
					- This method returns an empty string if USING_AVLB is not defined
					- Otherwise, the method traverses the ancestor tree and returns the first non-empty value encountered or an empty string if none are found
			*/
			virtual std::string			LockedBy(RoleUserPtr pRoleUser, EntityPtr pChild = NULL);
			//! Call UnMarkMarked() to un mark an entity (after calling UnMarkMarked(), IsMarked() returns false)
			void										MarkMarked()							{ m_uFlags |= D3_ENTITY_MARKED; }
			//! Call UnMarkMarked() to un mark an entity (after calling UnMarkMarked(), IsMarked() returns false)
			void										UnMarkMarked()						{ m_uFlags &= ~D3_ENTITY_MARKED; }
			//! Debugging Helper. TraceDelete will ensure that this logs messages when its destructor starts and finishes (if bDeep is true, all its offspring is traced too)
			void										TraceDelete(bool bDeep = true);

			//! Load this into the database specified and return a pointer to the loaded object (this can't be new).
			virtual EntityPtr				LoadObject(DatabasePtr pDB, bool bRefresh = false, bool bLazyFetch = true);
			//! Refresh this by getting the latest version from the database (this can't be new).
			virtual bool						Reload()	{ return LoadObject(m_pDatabase, true) != NULL; }
			//! Update this and if (bCascade is true also all it's children). pDB->HasTransaction() must be true!
			virtual bool 						Update(DatabasePtr pDB, bool bCascade = false);
			//! Update this and if (bCascade is true also all it's children). this->m_pDatabase->HasTransaction() must be true!
			virtual bool 						Update(bool bCascade = false)	{ return Update(m_pDatabase, bCascade); }
			//! Delete this from the database and from memory. pDB->HasTransaction() must be true!
			virtual bool 						Delete(DatabasePtr pDB);
			//! Delete this from the database and from memory. this->m_pDatabase->HasTransaction() must be true!
			virtual bool 						Delete()											{ return Delete(m_pDatabase); }
			//! Return true if this is valid. Does some basic checks like columns marked as mandatory may not be NULL, etc.
			virtual bool 						IsValid();

			//! The IsRelationChild method is called by this' parent relations marked as "Switched"
			/*! @param pRelation	The switched Relation which sends the message
					@param pKey       One of this' instance key
					@return bool      Return true if the key is a valid member of the relation, false otherwise.

					\note This method determines whether or not the passed in \a pKey meets the requirements
					as a memeber of the switched relationship.

	        To understand this method, you need to understand the concept of switched relations
					(also commonly refered to as \a typed \a relations). Switched relations are best explained
					using an example. Consider the Table Street and House and the two relations \a Street \a has
					\a Houses \a on \a the \a left and \a Street \a has \a Houses \a on \a the \a right. The
					Table House countains a foreign key to the table Street. This foreign key alone only
					allows you to implement the relation \a Street \a has \a Houses.

					D3 allows you to implement the other two relations by marking both relations as switched.

					For the switching to work as expected, you must implement your own subclass of Entity
					handling House instances. The subclass will need to implement it's own version of this
					method. Now consider that Houses have street numbers and that the rule is that
					all houses with odd street numbers are on the left. In your House class you'd overload
					this method as follows:

					\code
					bool House::IsRelationChild(RelationPtr pRelation, InstanceKeyPtr pKey)
					{
						// Preconditions
						//
						assert(pRelation);
						assert(pRelation->GetMetaRelation()->GetChildMetaKey()->GetMetaEntity() == m_pMetaEntity);
						assert(pKey->GetEntity() == this);

						MetaRelationPtr pHousesOnLeft = // set to meta relation "Street has Houses on the left"
						MetaRelationPtr pHousesOnRight = // set to meta relation "Street has Houses on the right"

						if (pRelation->GetMetaRelation() == pHousesOnLeft)
						{
							if ((GetStreetNumber() % 2) == 0)
							  return false;		// this house is on the right
							else
							  return true;		// this house is on the left
						}

						if (pRelation->GetMetaRelation() == pHousesOnRight)
						{
							if ((GetStreetNumber() % 2) == 0)		// we have an even house number
							  return true;		// this house is on the right
							else
							  return false;		// this house is on the left
						}

						throw 0; // Throw an exception, we should never reach this point

						return false;

					}
					\endcode

					At this level, the method always returns false!
			*/
			virtual bool						IsRelationChild(RelationPtr pRelation, InstanceKeyPtr pKey)		{ return false; }

			//! Returns the UpdateType. If you send an Update() message, D3 would perform the type of update this method returns.
			UpdateType							GetUpdateType();

			//! Notification that this is about to be populated manually
			/*! This will send a message to all it's InstanceKey objects notifying them
					that this is beeing populated. In effect, all keys must be removed from
					the MetaKey's multiset.
			*/
			virtual void						On_BeforeManualPopulatingObject()			{ On_BeforePopulatingObject(); }
			//! Notification that this has been fully populated
			/*! This method will resolve relations, but in contrast to the internal
					On_AfterPopulatingObject(), this will not be clean if any changes were
					made.
			*/
			virtual void						On_AfterManualPopulatingObject()			{ On_AfterPopulatingObject(false); }

			//! Writes the complete contents of this as SQL insert statement to the out stream
			virtual std::ostream &	AsSQL(std::ostream & ostrm);

			//! Writes the accessible contents of this as JSON to the out stream
			virtual std::ostream &	AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, bool* pFirstChild = NULL);

			//! This method creates a new object that has identical attributes as the original and then overrides those attributes for which values have been supplied in pJSONChanges
			/*! This method calls other protected virtual methods which do the actual work:
						CreateCloningController (returns an instance of D3::CloningController)
						CreateClone (does the actual cloning recursively)
						On_AfterCloned	(called for each cloned object)

			*/
			virtual EntityPtr				Clone(SessionPtr pSession, DatabaseWorkspacePtr pDBWS, Json::Value* pJSONChanges = NULL);

			//! Copies all the columns values from aObj to this (this method is a synonym for Assign(aObj))
			/*!
					If aObj is not ako D3::Entity object or aObj is an instance of a different
					MetaEntity than this the method will throw and exception.

					If this and aObj belong to the same D3::Database, simply calling this method
					is most likely going to violate key constraints and fail unless you
					send this an On_BeforeManualPopulatingObject message before calling
					Copy or Assign. In this case, you set the values of the columns
					after calling this methed and when uniqueness is guaranteed, send this
					the On_AfterManualPopulatingObject message.
			*/
			virtual	bool						Copy(ObjectPtr aObj);

		protected:
			//! Overload this in subclasses so that it can construct and return your own specific CloningController
			virtual CloningController*	CreateCloningController(SessionPtr pSession, DatabaseWorkspacePtr pDBWS, Json::Value* pJSONChanges)	{ return new CloningController(this, pSession, pDBWS, pJSONChanges); }
			//! This
			virtual	EntityPtr				CreateClone(CloningController* pController, EntityPtr pClonedParent = NULL, MetaRelationPtr pParentMR = NULL);

		public:

			//! Updates the values of this columns from a json structure
			/*! The JSON structure must be in the form {"Name":val_0,"Name":val_1,...,"Name":val_n}
					The method:
						- iterates over all columns and looks if a value for a given column was supplied
						- if a value was supplied, it sets the value (might be null)
						- columns supplied which are not known are ignored
					This method can throw std::string exceptions with info on the problem
			*/
			virtual void						SetValues(RoleUserPtr pRoleUser, Json::Value & jsonValues);

			//! DumpTree returns a string containing basic info about this and all its descendents
			/*! The string returned is properly formated so that it can be dumped to a stl ostream
			*/
			std::string							DumpTree();

		protected:
			//! AsJSON helper dumping columns
			virtual void						ColumnsAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm);
			//! AsJSON helper dumping keys
			virtual void						KeysAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm);
			//! AsJSON helper dumping child relations
			virtual void						ParentRelationsAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm);

			//! DumpTree helper
			void										DumpTree(std::ostringstream & ostrm, std::set<EntityPtr> & setProcessed, int ilevel);

			/*! @name State modifiers
					These modifiers are used internally to keep track of an objects state.
			*/
			void										MarkClean();																										//!< Mark this as clean (typically set after the object is populated from the database)

			void										MarkDirty()							{ m_uFlags |= D3_ENTITY_DIRTY; }				//!< Mark this as dirty (needs UPDATE)
			void										MarkNew()								{ m_uFlags |= D3_ENTITY_NEW; }					//!< Mark this as new (needs INSERT)
			void										MarkDelete()						{ m_uFlags |= D3_ENTITY_DELETE; }				//!< Mark this as deleted (needs DELETE)
			void										MarkPopulating()				{ m_uFlags |= D3_ENTITY_POPULATING; }		//!< Mark this as populating (data has been obtained from DB and is now being writting to this)
			void										MarkDestroying()				{ m_uFlags |= D3_ENTITY_DESTROYING; }		//!< Mark this as being in the process of destroying the object
			void										MarkConstructing()			{ m_uFlags |= D3_ENTITY_CONSTRUCTING; }	//!< Mark this as constructing (this is currently set/unset by MetaEntity::CreateInstance())

			void										UnMarkDirty();																									//!< Clear dirty flag
			void										UnMarkNew()							{ m_uFlags &= ~D3_ENTITY_NEW; }					//!< Clear new flag
			void										UnMarkDelete()					{ m_uFlags &= ~D3_ENTITY_DELETE; }			//!< Clear delete flag
			void										UnMarkPopulating()			{ m_uFlags &= ~D3_ENTITY_POPULATING; }	//!< Clear populating flag
			void										UnMarkDestroying()			{ m_uFlags &= ~D3_ENTITY_DESTROYING; }	//!< Clear destroying flag
			void										UnMarkConstructing()		{ m_uFlags &= ~D3_ENTITY_CONSTRUCTING; }//!< Clear constructing flag

			/** @name Notifications
			*/
			//@{
			//! Notification sent by the associated MetaEntity's CreateInstance() method.
			/*!	If you subclass this class and properly register your class such that
					MetaEntity::CreateInstance() creates an instance of your class, you may want
					to intercept this message in order to perform other initialisation tasks.
					However, you must make sure that you chain to this method when you're done
					with your initialisation.
			*/
			virtual void						On_PostCreate();

			//! A Column has been created, add it to m_vectColumn
			void										On_ColumnCreated(ColumnPtr pColumn);
			//! A Column has been deleted, remove it from m_vectColumn
			void										On_ColumnDeleted(ColumnPtr pColumn);


			//! An InstanceKey has been created, add it to m_vectInstanceKey
			void										On_InstanceKeyCreated(InstanceKeyPtr pKey);
			//! An InstanceKey has been deleted, remove it from m_vectInstanceKey
			void										On_InstanceKeyDeleted(InstanceKeyPtr pKey);


			//! A parent Relation has been created, add it to m_vectRelation
			void										On_ParentRelationCreated(RelationPtr pRelation);
			//! A parent Relation has been deleted, remove it from m_vectRelation
			void										On_ParentRelationDeleted(RelationPtr pRelation);


			//! A child Relation has been created, add it to m_vectRelation
			void										On_ChildRelationCreated(RelationPtr pRelation);
			//! A child Relation has been deleted, remove it from m_vectRelation
			void										On_ChildRelationDeleted(RelationPtr pRelation);

			//! Notification that the specified column is about to change. Trap this and return false to stop the change.
			virtual bool						On_BeforeUpdateColumn(ColumnPtr pCol);
			//! Notification that the specified column has changed. Trap this and return false to stop the change.
			virtual bool						On_AfterUpdateColumn(ColumnPtr pCol)			{ return true; }


			//! Notification that this is about to be saved to the database. Trap this and return false to stop the update.
			virtual bool						On_BeforeUpdate(UpdateType eUT)						{ return true; }
			//! Notification that this has been saved to the database. Trap this and return false to reverse the update.
			virtual bool						On_AfterUpdate(UpdateType eUT);


			//! Notification that this is about to be populated from data from the actual database.
			/*! This will send a message to all it's InstanceKey objects notifying them
					that this is beeing populated from the database. In effect, all keys must be
					removed from the <MetaKey's multiset.
			*/
			virtual void						On_BeforePopulatingObject();
			//! Notification that this has been fully populated from data from the actual database.
			virtual void						On_AfterPopulatingObject(bool bMarkClean = true);

			//@}

			//! Notification that this has been added to a ResultSet
			virtual void						On_AddedToResultSet(ResultSetPtr pResultSet);
			//! Notification that this has been removed from a ResultSet
			virtual void						On_RemovedFromResultSet(ResultSetPtr pResultSet);

#ifdef D3_OBJECT_LINKS
		// Friend Class ObjectLink
		friend class ObjectLink;					// 24/09/02 - R1 - Hugp - TMS support stuff

		protected:
			// 24/09/02 - R1 - Hugp - TMS support stuff
			ObjectLinkPtrListPtr		m_pObjectLinks;	// A pointer to a list of objects linked with this object through the ObjectLink table

		public:
			// 24/09/02 - R1 - Hugp - TMS support stuff
			// Support arbitrary linked objects
			ObjectLinkPtrListPtr		GetNativeObjectLinks()	{ return m_pObjectLinks; }
			bool										CopyObjectLinks(Entity & anotherEntity);


			/* This method returns a list of all linked objects meeting the criteria passed in.

					@param  strToObjectType If specified, the returned collection will only
																	contain objects of this type, if any.

					@param  nLinkType  If not 0, only objects linked to this with the
														 specified link type will be returned. If 0,
														 objects returned can have any link type.

				 \note Clients MUST delete the returned collection when no longer required.
			*/

			EntityPtrListPtr				GetToObjectLinks		(const std::string & strToObjectType = std::string(""), short nLinkType = 0);


			/* This method returns a list of all linked objects meeting the criteria passed in.

					@param  strFromObjectType If specified, the returned collection will only
																		contain objects of this type, if any.

					@param	nLinkType  If not 0, only objects linked to this with the
														 specified link type will be returned. If 0,
		                         objects returned can have any link type.

				 \note Clients MUST delete the returned collection when no longer required.
			*/

			EntityPtrListPtr				GetFromObjectLinks	(const std::string & strFromObjectType = std::string(""), short nLinkType = 0);
			ObjectLinkPtr						GetToObjectLink(EntityPtr pToObject, short nLinkType = 0);
			ObjectLinkPtr						GetFromObjectLink(EntityPtr pFromObject, short nLinkType = 0);
			ObjectLinkPtr 					AddObjectLink(EntityPtr pToObject, short nLinkType = 0);
			bool										RemoveObjectLink(EntityPtr pToObject, short nLinkType = 0);
			bool										RemoveAllObjectLinks(const std::string & strToObjectType = std::string(""), short nLinkType = 0);

		protected:
			// 24/09/02 - R1 - Hugp - TMS support stuff
			bool										AddObjectLink(ObjectLinkPtr pOL);
			bool										RemoveObjectLink(ObjectLinkPtr pOL);
			// 05/04/03 - R3 - Hugp
			bool										HasObjectLink(ObjectLinkPtr pOL);
#endif /* D3_OBJECT_LINKS */

	};


} // end namespace D3

#endif /* INC_D3_ENTITY_H */
