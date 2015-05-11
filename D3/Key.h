#ifndef INC_D3_KEY_H
#define INC_D3_KEY_H

// ===========================================================
// File Info:
//
// $Author: daellenj $
// $Date: 2014/08/23 21:47:47 $
// $Revision: 1.32 $
//
// MetaKey: This class describes primary, secondary and
// foreign keys of any of ako entity classes.
//
// Key: This class acts as a common, abstract base for
// InstanceKey and TemporaryKey classes.
//
// InstanceKey: This class holds the details of an actual
// key associated with an Entity type instance.
//
// TemporaryKey: This class is a snapshot of a InstanceKey
// and can be used beyond the lifetime of the InstanceKey
// from which it was created
//
// ===========================================================
// Change History:
// ===========================================================
//
// 27/08/03 - R1 - Hugp
//
// Created class
//
// -----------------------------------------------------------

#include "D3.h"
#include "D3MDDB.h"
#include "D3BitMask.h"
#include <boost/thread/recursive_mutex.hpp>

// Needs JSON
#include <json/json.h>

/** \file Key.h
		Contains Declarations for MetaKey, Key, InstanceKey and TemporaryKey
*/


extern const char szMetaKeyFlags[];					// solely used to ensure type safety (no implementation needed)

namespace D3
{
	//! This template is required to sort keys
	/*! Key objects are typically stored in std::multiset<> collections.
	    The structure can be used as the Less predicate in the mutliset
			template instantiation:

			\code
			std::multiset<InstanceKeyPtr, KeyLessPredicate>		multisetKey;
			\endcode

			The KeyLessPredicate ensures that key comparision is handled
			correctly and thus insertion of new elements in multisets is
			such that they are ordered correctly.
	*/
	struct D3_API KeyLessPredicate : public std::binary_function<KeyPtr, KeyPtr, bool>
	{
		//! This operator is used by the standard template library when comparing two Key*.
		/*! The method simply translates the call to:

		\code
		return ((*x) < (*y));
		\endcode

		*/
		bool operator()(const KeyPtr x, const KeyPtr y) const;
	};





	/** @name Key-Collections
			The following types are needed by MetaKey instances to maintain recordsets of
			its related InstanceKey objects per database.
			MetaKey objects maintain one recordset containing all instances of its related
			InstanceKey objects per database which holds at least one such InstanceKey.
			If the MetaKey is a member of a cached MetaEntity, there will only be one recordset
			in the map, namely that of the global database.
	*/
  //@{
	typedef std::multiset<KeyPtr, KeyLessPredicate>						InstanceKeyPtrSet;
	typedef InstanceKeyPtrSet*																InstanceKeyPtrSetPtr;
	typedef InstanceKeyPtrSet::iterator												InstanceKeyPtrSetItr;
	typedef std::map<DatabasePtr, InstanceKeyPtrSetPtr>				InstanceKeyPtrSetPtrMap;
	typedef InstanceKeyPtrSetPtrMap*													InstanceKeyPtrSetPtrMapPtr;
	typedef InstanceKeyPtrSetPtrMap::iterator									InstanceKeyPtrSetPtrMapItr;
  //@}




	//! We store all MetaKey objects in a map keyed by ID
	/*! Note that objects which are not stored in the meta dictionary (e.g. MetaKey objects
			created at system startup to enable access to the the MetaDictionary database)
			are assigned MetaKey::M_uNextInternalID which is then decremented by 1. The
			initial value	of MetaKey::M_uNextInternalID is KEY_ID_MAX.
			MetaKey objects without a physical representation on disk are therefore
			identifiable if GetID() returns a value > MetaKey::M_uNextInternalID.
	*/
	typedef std::map< KeyID, MetaKeyPtr >				MetaKeyPtrMap;
	typedef MetaKeyPtrMap::iterator							MetaKeyPtrMapItr;

	//! This class provides meta details for a key
	/*! MetaKey objects are owned by MetaEntity objects and created during meta model initialisation.

			A MetaKey object holds information which provide the following features:

			-	Given an Entity object, can create an appropriate InstanceKey

			- Given the MetaKey object is a Primary, Secondary or Foreign key,
			  they maintain a random access, ordered collection of all InstanceKey
				objects based on this

			- Can be used to construct a TemporaryKey object

			- Can take part in MetaRelation objects as source MetaKey and/or target MetaKey

		\todo While it is nice to be able to specify cool flags in the MetaKey ctor, some of
		the flags are extremely dependent by attribute of the key's columns. For example, an
		AutoNum key is an AutoNum key only if it consists of a single, mandatory IDENTITY column.
		There are many other similar constraints, e.g.: a key can't be unique if any of
		its columns is NULL-able. Perhaps the keys should be verified once the entire Dictionary
		has been loaded.

	*/
	class D3_API MetaKey : public Object
	{
		friend class MetaDatabase;
		friend class MetaEntity;
		friend class MetaRelation;
		friend class Database;
		friend class InstanceKey;
		friend class ODBCDatabase;
		friend class OTLDatabase;

		// Standard D3 stuff
		//
		D3_CLASS_DECL(MetaKey);

		public:
			//! The Flags bitmask identifies basic characteristics for each type of key
			struct Flags : public Bits<szMetaKeyFlags>
			{
				BITS_DECL(MetaKey, Flags);

				//@{ Primitive masks
				static const Mask Mandatory;											//!< 0x00000001 - The key cannot be NULL
				static const Mask Unique;													//!< 0x00000002 - The key must be UNIQUE
				static const Mask AutoNum;												//!< 0x00000004 - The key is assigned by the RDBMS at insertion time (typically the PRIMARY key)
				static const Mask Primary;												//!< 0x00000010 - The key is the PRIMARY key for an Entity (it must also be MANDATORY and UNIQUE)
				static const Mask Secondary;											//!< 0x00000020 - A secondary key providing alternate direct access
				static const Mask Foreign;												//!< 0x00000040 - A foreign key pointing to a UNIQUE
				static const Mask Conceptual;											//!< 0x00000080 - A unmaintained key that succinctly represents the associated entity
				static const Mask HiddenOnQuickAccess;						//!< 0x00000008 - If true, this key will not be present in quick access views
				static const Mask DefaultQuickAccess;							//!< 0x00000100 - If true and HiddenOnQuickAccess is false, this will be the default tab in a Quick Access dialog for this entity type
				static const Mask AutocompleteQuickAccess;				//!< 0x00000200 - If true and HiddenOnQuickAccess is false, the first time the Quick Access tab for this key is activated it is submitted automatically (with no key values) so that a result should show on activation
				//@}

				//@{ Combo masks
				static const Mask PrimaryKey;											//!< Same as: (Primary | Unique | Mandatory)
				static const Mask PrimaryAutoNumKey;							//!< Same as: (Primary | Unique | Mandatory | AutoNum)

				static const Mask OptionalSecondaryKey;						//!< Same as: (Secondary)
				static const Mask MandatorySecondaryKey;					//!< Same as: (Secondary | Mandatory)
				static const Mask UniqueSecondaryKey;							//!< Same as: (Secondary | Mandatory | Unique)

				static const Mask OptionalForeignKey;							//!< Same as: (Foreign)
				static const Mask MandatoryForeignKey;						//!< Same as: (Foreign | Mandatory)

				static const Mask ConceptualKey;									//!< Same as: (Conceptual | Mandatory)
				static const Mask UniqueConceptualKey;						//!< Same as: (Conceptual | Mandatory | Unique)

				static const Mask Searchable;											//!< Used internally, Same as: (Primary | Secondary | Foreign)

				static const Mask Default;												//!< Nothing set
				//@}
			};

		protected:
			static KeyID							M_uNextInternalID;				//!< If no internal ID is passed to the constructor, use this and decrement afterwards (used for meta dictionary definition meta obejcts)
			static MetaKeyPtrMap			M_mapMetaKey;							//!< Holds all MetaKey objects in the system indexed by m_uID

			MetaEntityPtr							m_pMetaEntity;						//!< The MetaEntity of which this is a MetaKey
			KeyID											m_uID;										//!< This is the index of this so that (MetaKey::M_mapMetaKey.find(m_uID)->second == this)
			KeyIndex									m_uKeyIdx;								//!< This is the index of this so that (m_pMetaEntity->GetMetaKey(m_uKeyIdx) == this)
			std::string								m_strName;								//!< The MetaKey's name
			std::string								m_strProsaName;						//!< A user friendly name for this
			std::string								m_strDescription;					//!< Infor that describes this in more details to the users
			Flags											m_Flags;									//!< See MetaKey::Flags above
			const Class*							m_pInstanceClass;					//!< The Class object which knows how to create instances
			std::string								m_strInstanceInterface;		//!< The name of the interface class that can wrap an instance of this type
			std::string								m_strJSMetaClass;					//!< The name of the JavaScript meta class representing this in the browser
			std::string								m_strJSInstanceClass;			//!< The name of the JavaScript instance class representing this in the browser
			std::string								m_strJSViewClass;					//!< The name of the JavaScript widget that knows how to render instances of this type as a APALUI widget in the browser (full)
			MetaColumnPtrList					m_listColumn;							//!< A list holding MetaColumnPtr objects making up the key
			MetaRelationPtrVect				m_vectMetaRelation;				//!< This vector holds all meta relation objects this has. If this is a foreign key, these should be parent relations.
			InstanceKeyPtrSetPtrMap		m_mapInstanceKeySet;			//!< Holds a map keyed by DatabasePtr containing a multiset of InstanceKeyPtr objects
			MetaKeyPtrList						m_listOverlappedKeys;			//!< Holds all MetaKey objects belonging to m_pMetaEntity which share one or more MetaColumn objects with this
  		boost::recursive_mutex		m_mtxExclusive;						//!< This Mutex is used to serialise modifications to m_mapInstanceKeySet
			std::string								m_strHSTopicsJSON;				//!< JSON string containing an array of help topics associated with this

			//! Unused ctor() - only here for D3 Class stuff
			/*! This constructor is solely here because of D3::Class.
			    If - at some later point in time - we decide to archive
					meta dictionaries, we will be using this ctor().
			*/
			MetaKey();

			//! The typical ctor used during MetaDatabase::InitialiseDictionary()
			/*! @param pMetaEntity	A pointer to the MetaEntity of which this is a MetaKey
					@param strName			The name of the key. You can use this name when searching
					                    a particular MetaKey through it's MetaEntity, e.g.:
															pMetaEntity->GetMetaKey(strName);
					@param flags				See MetaKey::Flags above
			*/
			MetaKey(MetaEntityPtr pMetaEntity, const std::string & strName, const Flags::Mask & flags);

			//! A special ctor used during MetaDatabase::InitialiseDictionary()
			/*! This ctor allows the meta model initialisation process to create a MetaKey object
					which will create a custom InstanceKey type when needed. This can be useful if
					non standard work needs to be performed in addition to what D3 does.

					@param pMetaEntity	A pointer to the MetaEntity of which this is a MetaKey
					@param strName			The name of the key. You can use this name when searching
					                    a particular MetaKey through it's MetaEntity, e.g.:
															pMetaEntity->GetMetaKey(strName);
					@param strClassName	The name of the class which represents instances of this MetaKey.
															A D3::Class object with the name you specify must exist and be
															a kind of an InstanceKey class.
					@param flags				See MetaKey::Flags above
			*/
			MetaKey(MetaEntityPtr pMetaEntity, KeyID uID, const std::string & strName, const std::string & strClassName, const Flags::Mask & flags);

			//! Deletes all members in m_listSourceMetaRelation
			/*! The c-tor should ever only be invoked by D3 itself.
			*/
			~MetaKey();

			//! ctor helper
			void											Init(const std::string & strInstanceClassName);

		public:
			//! Creates an InstanceKey object associated with the Entity object passed in
			/*! Note: The type of object created may not necessarily be an InstanceKey object.
			*/
			InstanceKeyPtr						CreateInstance(EntityPtr pEntity);

			/** @name Meta Data Accessors
			*/
			//@{
			//! Returns the MetaEntity object of which this is a MetaKey
			MetaEntityPtr							GetMetaEntity()											{ return m_pMetaEntity; }
			//! Returns a pointer to an ordered std::list<MetaColumnPtr> objects making up the key
			MetaColumnPtrListPtr			GetMetaColumns()										{ return &m_listColumn; }
			//! Returns a reference to a std::vect<MetaRelationPtr> containing all relations where this is either the parent or child key.
			MetaRelationPtrVect&			GetMetaRelations()									{ return m_vectMetaRelation; }
			//! Returns a reference to a std::vect<MetaRelationPtr> containing all relations where this is either the parent or child key.
			MetaRelationPtr						GetMetaRelation(unsigned int idx)		{ if (idx < m_vectMetaRelation.size()) return m_vectMetaRelation[idx]; return NULL; }
			//@}

			/** @name Property accessors
			*/
			//@{
			//! Returns the number of columns making up the MetaKey
			unsigned int							GetColumnCount()										{ return m_listColumn.size(); }
			//! The Index of the MetaKey. The Index can be used for efficient Key lookup through Entity and MetaEntity objects
			KeyIndex									GetMetaKeyIndex()	const							{ return m_uKeyIdx; }
			//! The name as passed in during construction of the MetaKey
			const std::string &				GetName() const											{ return m_strName; }
			//! The name that is meaningful to the users
			const std::string &				GetProsaName() const								{ return m_strProsaName; }
			//! Text explaining this in more detail to the users
			const std::string &				GetDescription() const							{ return m_strDescription; }
			//! Constructs and returns a string representing the key's full name in the form \a TableName.KeyName
			std::string								GetFullName() const;
			//! Returns the instance class name this uses
			const std::string &				GetInstanceClassName() const				{ return m_pInstanceClass->Name(); }
			//! Returns the name of the interface that is expected to wrap an instance of this type
			const std::string &				GetInstanceInterface()							{ return  m_strInstanceInterface;}
			//! Returns the JavaScript MetaClass to use on the browser side
			const std::string &				GetJSMetaClassName() const					{ return m_strJSMetaClass; }
			//! Returns the JavaScript InstanceClass to use on the browser side
			const std::string &				GetJSInstanceClassName() const			{ return m_strJSInstanceClass; }
			//! Returns the JavaScript ViewClass to use on the browser side
			const std::string &				GetJSViewClassName() const					{ return m_strJSViewClass; }
			//@}

			//! GetID returns a unique identifier which can be used to retrieve the same object using GetMetaKeyByID(unsigned int)
			KeyID											GetID() const												{ return m_uID; }
			//! Return the meta relation where GetID() == uID
			static MetaKeyPtr					GetMetaKeyByID(KeyID uID)
			{
				MetaKeyPtrMapItr	itr = M_mapMetaKey.find(uID);
				return (itr == M_mapMetaKey.end() ? NULL : itr->second);
			}
			//! Returns a list of all MetaKeys in the system.
			static MetaKeyPtrMap&			GetMetaKeys()												{ return M_mapMetaKey; };

			//@{ Characteristics
			//! Return this' characteristics bitmask
			const Flags &							GetFlags() const										{ return m_Flags; }

			//! Returns true if the MetaKey is mandatory
			bool											IsMandatory() const									{ return (m_Flags & Flags::Mandatory); }
			//! Returns true if the MetaKey is unique
			bool											IsUnique() const										{ return (m_Flags & Flags::Unique); }
			//! Returns true if the MetaKey is an IDENTITY column
			bool											IsAutoNum() const										{ return (m_Flags & Flags::AutoNum); }
			//! Returns true if the MetaKey is a primary key
			bool											IsPrimary() const										{ return (m_Flags & Flags::Primary); }
			//! Returns true if the MetaKey is a secondary key
			bool											IsSecondary() const									{ return (m_Flags & Flags::Secondary); }
			//! Returns true if the MetaKey is a foreign key
			bool											IsForeign() const										{ return (m_Flags & Flags::Foreign); }
			//! Returns true if the MetaKey is a conceptual key
			bool											IsConceptual() const								{ return (m_Flags & Flags::Conceptual); }

			bool											IsHiddenOnQuickAccess() const				{ return (m_Flags & Flags::HiddenOnQuickAccess); }
			//! If true and DefaultQuickAccess is false, this will be the default tab in a Quick Access dialog for this entity type
			bool											IsDefaultQuickAccess() const				{ return (m_Flags & Flags::DefaultQuickAccess); }
			//! If true and AutocompleteQuickAccess is false, the first time the Quick Access tab for this key is activated it is submitted automatically (with no key values) so that a result should show on activation
			bool											IsAutocompleteQuickAccess() const		{ return (m_Flags & Flags::AutocompleteQuickAccess); }
			//@}


			/** @name Access to collected InstanceKey objects
			*/
			//@{
			//! Searches the requested key in this' instance cache.
			/*! This method does not search the database, it only checks the cache.
					@param pKey				The key who's columns hold values which this method will search for. This can be
														a TemporaryKey or ako InstanceKey.
					@param pDatabase	Optional parameter specifying the database within which we search the key. If this
					                  parameter is omitted, the method will attempt to determine the database from pKey
														or in case the key to be searched for belongs to a cached entity, use the global database.
														If pKey is a TemporaryKey and the key to be searched is not a key from a cached entity,
														the method will not be able to determine the database and return NULL.
					@return If successful, returns an InstanceKey based on this which matches the key passed in, else NULL is returned.
			*/
			InstanceKeyPtr						FindInstanceKey(KeyPtr pKey, DatabasePtr pDatabase = 0);

			//! FindObject calls FindInstanceKey but instead of returning the key it returns the object that onws the key (method may return NULL)
			EntityPtr									FindObject(KeyPtr pKey, DatabasePtr pDatabase = 0);

			//! Searches the requested key in the database.
			/*! This method checks the cache first and if found, returns the cached key.
			/*! @param pKey				The key who's columns hold values which this method will search for. This can be
														a TemporaryKey or ako InstanceKey.
					@param pDatabase	Optional parameter specifying the database within which we search the key. If this
					                  parameter is omitted, the method will attempt to determine the database from pKey
														or in case the key to be searched for belongs to a cached entity, use the global database.
														If pKey is a TemporaryKey and the key to be searched is not a key from a cached entity,
														the method will not be able to determine the database and return NULL.
					@param bRefresh		Optional parameter which is relevant only if the object is already resident in memory. If true,
					                  the resident object will be updated before it is returned, if false the resident object will be
														returned "as-is". If you need an up-to-date copy of the object, set this to true.
					@param bLazyFetch	Optional parameter. If true, none of the columns marked as "lazy fetch" will the loaded. This
														means that lazy-fetch columns will be loaded when first accessed. If you know you need to access
														lazy-fetch columns, you should set this parameter to false.
					@return If successful, returns an EntityPtr of the object matching the key passed in, otherwise, returns NULL
			*/
			EntityPtr									LoadObject(KeyPtr pKey, DatabasePtr pDatabase = 0, bool bRefresh = false, bool bLazyFetch = true);

			/*! Searches instances matching the requested key in the database.
			/*! @param listEntity An std::list holding EntityPtr. Objects found will be appended to the end of the list.
					@param pKey				The key who's columns hold values which this method will search for. This can be
														a TemporaryKey or ako InstanceKey.
					@param pDatabase	Optional parameter specifying the database within which we search the key. If this
					                  parameter is omitted, the method will attempt to determine the database from pKey
														or in case the key to be searched for belongs to a cached entity, use the global database.
														If pKey is a TemporaryKey and the key to be searched is not a key from a cached entity,
														the method will not be able to determine the database and return NULL.
					@param bRefresh		Optional parameter which is relevant only if an object is already resident in memory. If true,
					                  the resident object will be updated before it is returned, if false the resident object will be
														returned "as-is". If you need an up-to-date copy of the object, set this to true.
					@param bLazyFetch	Optional parameter. If true, none of the columns marked as "lazy fetch" will the loaded. This
														means that lazy-fetch columns will be loaded when first accessed. If you know you need to access
														lazy-fetch columns, you should set this parameter to false.
			*/
			void											LoadObjects(KeyPtr pKey, EntityPtrList& listEntity, DatabasePtr pDatabase = 0, bool bRefresh = false, bool bLazyFetch = true);

			//! Returns a pointer to set containig all InstanceKey objects of this MetaKey type
			InstanceKeyPtrSetPtr			GetInstanceKeySet(DatabasePtr pDatabase);
			//@}

			//! Debug aid: The method dumps this and (if bDeep is true) all its objects to cout
			void											Dump(int nIndentSize = 0, bool bDeep = true);

			//! Returns a string containing an SQL command which when executed creates the index for the key.
			/*! The returned string depends on the target RDBMS specified with the parameter.
			*/
			std::string								AsCreateIndexSQL(TargetRDBMS eTarget);

			//! Returns all known and accessible instances of D3::MetaKey as a JSON stream
			static std::ostream &			AllAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm);

			//! Inserts this meta key into the passed-in JSON stream
			/*! If a key is inserted in the context of a meta entity, then output to the stream is
					reduced since it is assumed that any columns belonging to this meta key are
					already contained in the stream so all you get is that columns ID's.
			*/
			std::ostream &						AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, MetaEntityPtr pContext, bool * pFirstChild = NULL, bool bShallow = false);

			//! Get all instance resident in thge database
			/*! @param pDB (database) The database to check
					@param el (input) All instances of this meta entity resident in pDB will be stored in this list (see notes below)
					Note: el will be emptied before anything is stored
			*/
			void											CollectAllInstances(DatabasePtr pDB, EntityPtrList & el);

		protected:
			//! Called by #MetaEntity::LoadMetaObject().
			/*!	The method expects that the object passed in is an instance of the D3MetaKey
					MetaEntity and constructs a new MetaKey object based on pD3MK which acts as
					a template.
			*/
			static void								LoadMetaObject(MetaEntityPtr pMetaEntity, D3MetaKeyPtr pD3MK);

			//! Deletes all Entity objects of this type belonging to the specified database.
			/*! This method is called by MetaEntity::DeleteAllObjects(pDatabase);
			*/
			void											DeleteAllObjects(DatabasePtr pDatabase);

			//! Internal helper that is called by the constructors to complete object initialisation.
			void											Initialise(const std::string & strClassName);

			//! Returns true if this collects InstanceKey objects (true for Primary, Secondary and Foreign keys).
			bool											IsSearchable() const								{ return (m_Flags & Flags::Searchable); }

			//! Used during meta model initialisation
			void											AddMetaColumn(MetaColumnPtr	pMC);

			/** @name Notifications
					The following methods are handlers for notifications sent by relations of this.
			*/
			//@{
			//! A new InstanceKey object of this MetaKey type has been created
			void											On_InstanceCreated(InstanceKeyPtr pInstanceKey);
			//! A InstanceKey object of this MetaKey type is being destroyed
			void											On_InstanceDeleted(InstanceKeyPtr pInstanceKey);

			//! A new InstanceKey object of this MetaKey type is about to change
			void											On_BeforeUpdateInstance(InstanceKeyPtr pInstanceKey);
			//! A new InstanceKey object of this MetaKey type has been changed
			/*! \note In the current implementation I have solved a problem very inefficiently.
					Theoretically, this notification should ever only be posted if the key passed in
					is not already a member in the MetaKey's InstanceKey set. However, this rule is
					violated if a key is a child key in a relationship which IsChildSwitch() is true.
					The problem can easily be demonstrated with QuickTest03() in D3Test/D3Test.cpp.
					The method set's a Person's father and mother \a before setting the childs Sex.
					The foreign keys are obviously inserted into their respective meta entity's
					set of instance keys at the time the childs parents are set. However, when the
					childs Sex is set, the instance foreign keys do not receive On_BeforeUpdate()
					notifications and hence, the keys are duplicated in the MetaKey's collections.
					To overcome this problem, I have for now simply implemented code to ensure the
					instance key is only inserted if it does not already exist. This comes with
					a performance penalty for non-unique keys: whenever a key is inserted and another
					matching key is found, the method walks all keys which match the new keys value
					and to see whether or not the key is already a member. This is of course extremely
					costly if instance key collections exist with a large number of matching keys.
			*/
			void											On_AfterUpdateInstance(InstanceKeyPtr pInstanceKey);

			//! Notification than a MetaRelation which has this as its parent Key has been created (ignored)
			void											On_ParentMetaRelationCreated(MetaRelationPtr pRel);
			//! Notification than a MetaRelation which has this as its parent Key has been deleted (ignored)
			void											On_ParentMetaRelationDeleted(MetaRelationPtr pRel);

			//! Notification than a MetaRelation which has this as its child Key has been created (ignored)
			void											On_ChildMetaRelationCreated(MetaRelationPtr pRel);
			//! Notification than a MetaRelation which has this as its child Key has been deleted (ignored)
			void											On_ChildMetaRelationDeleted(MetaRelationPtr pRel);

			//! Notification that a database has been created.
			/*! If this is a searchable key, it will create this' InstanceKey multiset for the database.

					\note If this belongs to a MetaEntity marked as cached, only a single key mutiset will
					will be maintained for the global database.
			*/
			void											On_DatabaseCreated(DatabasePtr pDatabase);

			//! Notification that a database has been destroyed.
			/*! If this is a searchable key, it will delete this' InstanceKey multiset for the database.

					\note If this belongs to a cached MetaEntity and the pDatabase parameter passed
					in is not the global database, no action is taken.
			*/
			void											On_DatabaseDeleted(DatabasePtr pDatabase);
			//@}

			//! Returns the index of the MetaRelationPtr requested.
			/*!	Returns an index \a idx which meets the condition:

					(m_vectMetaRelation[idx] == pRel || idx >= m_vectMetaRelation[idx].size())
			*/
			unsigned int							FindRelation(MetaRelationPtr pRel);

			//! This helper returns true if this overlaps with the MetaKey passed in
			/*! Overlapping keys are keys from the same entity sharing one or more columns.
					In these cases, updating one key implicitly changes the other key.
			*/
			bool											IsOverlappingKey(MetaKeyPtr pMK);

			//! This helper adds the key passed in to it's collection of overlapping keys
			/*! Overlapping keys are keys from the same entity sharing one or more columns.
					In these cases, updating one key implicitly changes the other key.
			*/
			void											AddOverlappingKey(MetaKeyPtr pMK);

			//! This helper removes the key passed in from it's collection of overlapping keys
			/*! Overlapping keys are keys from the same entity sharing one or more columns.
					In these cases, updating one key implicitly changes the other key.
			*/
			void											RemoveOverlappingKey(MetaKeyPtr pMK);

			//! Sets the JSON HSTopic array
			void											SetHSTopics(const std::string & strHSTopicsJSON) { m_strHSTopicsJSON = strHSTopicsJSON; }
	};










	//! This class provides a common base for InstanceKey and TemporaryKey.
	/** The Key class implements basics features common to any sub-class based on Key.
			Such common features are:

			- Binary comparison (based on class Object)

			- Assignement (implements Assign())

			- Meta Data Access (access the MetaKey)

			You can't create an instance of this class, only sub-classes of this class are
			publically constructable.
	*/
	class D3_API Key : public Object
	{
		friend class MetaKey;
		friend class Relation;

		D3_CLASS_DECL_PV(Key);

		protected:
			MetaKeyPtr							m_pMetaKey;				//!< The meta key
			ColumnPtrList						m_listColumn;			//!< List of columns making up the key, most important column is in the front, least important is at the back

			//@{
			//! Ctor indirectly invoked through sub-class ctor's
			Key() : m_pMetaKey(NULL) {};
			Key(MetaKeyPtr pMetaKey) : m_pMetaKey(pMetaKey) {};
			//@}

		public:
			//! Provides access to the MetaKey of which this is an instance
			MetaKeyPtr							GetMetaKey() const							{ return m_pMetaKey; }

			//! Returns true if all columns associated with this have a NULL value
			/*! A key is considered NULL, if any of the following is true:

					- The key has no columns

					- A mandatory column is NULL or

					- All columns are NULL

			*/
			virtual bool						IsNull();

			//! Set this key to NULL
			/*! The method sends each column the SetNull() message and returns true if
			    all succeed, else false is returned.
			*/
			virtual bool						SetNull();

			//! Check whether a key is NULL based on another key.
			/*! This method is useful to check related keys for nullness. Consider Entity A and
					Entity B. A has a primary key PK and B has a foreign key FK. B.FK is a unique key
					consisting of A.PK and say SequenceNo. If the A.PK part of B.FK (B.FK.APK) is not
					NULL, we have a valid relation even if SequenceNo is NULL.
					This means that technically B.FK is incomplete and therefore B.FK->IsNull() returs
					true. If you used B.FK->IsKeyPartNull(A.PK->GetMetaKey()) it will return false,
					indicating that the A.PK part of B.FK is not NULL.
			*/
			virtual bool						IsKeyPartNull(MetaKeyPtr pMK);

			//! Assigns the values from another key to this.
			/** @param aKey The Key object who's values are assigned to this.

					@return			A reference to this

					@note				A bit of care is justified when using key assignement. If columns in the
											two keys are of different types, conversion is attempted but can fail
											and throw a D3::Exception.
											The method firstly sets all this' column values to NULL. The method then
											assigns min(this->GetColumnCount(), pObj->GetColumnCount()) columns from front
											to back from aKey to this.
			*/
			virtual Key &						Assign(Key & aKey);

			//! Returns the Entity of which this key is a member.
			/** @return			A pointer to the Entity object which is the owner of this key.

					@note				At this level, NULL is returned. InstanceKey objects will return
											appropriate values while TemporaryKey objects always return NULL.
			*/
			virtual EntityPtr				GetEntity()											{ return NULL; }

			//! Return the number of columns which make up this key
			unsigned int						GetColumnCount() 								{ return m_listColumn.size(); };

			//! Return the stl list containing the key columns
			ColumnPtrList&					GetColumns() 										{ return m_listColumn; };

			//@{
			//! Column accessor
			ColumnPtr								GetColumn(MetaColumnPtr pMC);
			ColumnPtr								GetColumn(ColumnIndex idx);
			ColumnPtr								GetColumn(const std::string & strName);
			//@}

			//@{
			/** Notifications from columns that they have changed (ignored at this level) */
			virtual bool						On_BeforeUpdateColumn(ColumnPtr pCol)	{ return true; }
			virtual bool						On_AfterUpdateColumn(ColumnPtr pCol)	{ return true; }
			//@}

			//! Debug helper dumps Key to std::cout
			virtual void						Dump (int nIndentSize = 0);

			//! Returns a string reflecting the key's values in a displayable format
			virtual std::string			AsString();

			//! Writes the complete contents of this as JSON to the out stream
			virtual std::ostream &	AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, bool * pFirstChild = NULL);

			//! Writes this' column values as a stringified JSON Array in the form [val1,val2,..valn]
			virtual std::string 		AsJSON();

			//! Writes this' column values as a stringified JSON Array in the form {"KeyID": 123, "Columns": [val1,val2,..valn]}
			virtual std::string 		AsJSONKey();

			//! Searches the requested key in the database.
			/*! @param pDatabase	Optional parameter specifying the database within which we search the key. If this
					                  parameter is omitted, the method will attempt to determine the database from pKey
														or in case the key to be searched for belongs to a cached entity, use the global database.
														If pKey is a TemporaryKey and the key to be searched is not a key from a cached entity,
														the method will not be able to determine the database and return NULL.
					@param bRefresh		Optional parameter which is relevant only if the object is already resident in memory. If true,
					                  the resident object will be updated before it is returned, if false the resident object will be
														returned "as-is". If you need an up-to-date copy of the object, set this to true.
					@param bLazyFetch	Optional parameter. If true, none of the columns marked as "lazy fetch" will the loaded. This
														means that lazy-fetch columns will be loaded when first accessed. If you know you need to access
														lazy-fetch columns, you should set this parameter to false.
					@return If successful, returns an EntityPtr of the object matching the key passed in, otherwise, returns NULL
			*/
			EntityPtr								LoadObject(DatabasePtr pDatabase = 0, bool bRefresh = false, bool bLazyFetch = true)	{ return m_pMetaKey->LoadObject(this, pDatabase, bRefresh, bLazyFetch); }

			//! Searches instances matching the requested key in the database.
			/*! @param listEntity An std::list holding EntityPtr. Objects found will be appended to the end of the list.
					@param pDatabase	Optional parameter specifying the database within which we search the key. If this
					                  parameter is omitted, the method will attempt to determine the database from pKey
														or in case the key to be searched for belongs to a cached entity, use the global database.
														If pKey is a TemporaryKey and the key to be searched is not a key from a cached entity,
														the method will not be able to determine the database and return NULL.
					@param bRefresh		Optional parameter which is relevant only if the object is already resident in memory. If true,
					                  the resident object will be updated before it is returned, if false the resident object will be
														returned "as-is". If you need an up-to-date copy of the object, set this to true.
					@param bLazyFetch	Optional parameter. If true, none of the columns marked as "lazy fetch" will the loaded. This
														means that lazy-fetch columns will be loaded when first accessed. If you know you need to access
														lazy-fetch columns, you should set this parameter to false.
			*/
			void										LoadObjects(EntityPtrList& listEntity, DatabasePtr pDatabase = 0, bool bRefresh = false, bool bLazyFetch = true)	{ return m_pMetaKey->LoadObjects(this, listEntity, pDatabase, bRefresh, bLazyFetch); }

		protected:
			//! Compare this with another Object
			/** @param pObj This must be a valid pointer to an Object which meets the criteria
											pObj->IsKindOf(Key::ClassObject()) == true.

					@return			-1 : This is less than pObj<BR>
					            0  : This is equal pObj<BR>
											1  : This is greater than pObj

					@note				Comparing keys is comparing their columns from front to back.
											If the columns at a given index are of different types, conversion is
											attempted but can fail and throw a D3::Exception.
											The method only compares the smaller number of columns in the two keys.
											Hence, if one of the keys has no columns, the method will report these
											as identical.
											Similarly, as soon as an undefined column is found in either key,
											comparison stops with success.
			*/
			int											Compare(ObjectPtr pObj);

	};





	//! Instances of this class are not associated with an entity object.
	/*! These objects are typically created from an InstanceKey or a MetaKey object. If an
			instance key is supplied during construction, the new TemporaryKey represents a
			detached and independent snapshot of the InstanceKey's values. If a MetaKey is
			supplied, the key's columns can the be assigned specific values before it is
			used to navigate inside D3.
	*/
	class D3_API TemporaryKey : public Key
	{
		friend class MetaKey;
		friend class Key;
		friend class InstanceKey;

		D3_CLASS_DECL(TemporaryKey);

		protected:
			TemporaryKey() {}

		public:
			TemporaryKey(Key & aKey);
			TemporaryKey(MetaKey & aMetaKey);

			//! Create a new temporary key and populate its values from the contents passed in the JSON value
			/*! The Json::Value passed in is expected to be an array with at least two elements, the first element contains
					the unique ID for the MetaKey and the second element is another array element containing a value for each
					column in the key
			*/
			TemporaryKey(Json::Value & jsonKey);

			//! Create a new temporary key and populate its values from the contents passed in JSON
			/*! The JSON structure is expected to adhere to this format:

					{"KeyID":*a,"Columns":[*b]}

					*a: This is the ID of the MetaKey who's values are supplied in the Columns array.
							The ID is the same as is returned by MetaKey::GetID().

					*b: The value of columns separated by commas. You must not supply more values than
					    the number of columns included by MetaKey. The columns must be of the appropriate
							type but can be null provided the corresponding MetaColumn allows NULL's.

					Note: This ctor throws an D3::Exception if:
					 . Input is badly formed
					 . A MetaKey with a* is not known
					 . The Columns array specifies more value than the MetaKey has columns
					 . If the datatype of one or more columns in the Columns array is not compatible with
						 the type of the MetaKey's corresponding MetaColumn

			*/
			TemporaryKey(const std::string & strJSON)	{ InitializeFromJSON(strJSON); }

			//! Ctor 3
			/*! Creates a new temporary key which is a type of aMetaKey and is
					initialised with the values from aKey.
			*/
			TemporaryKey(MetaKey & aMetaKey, Key & aKey);

			~TemporaryKey();

			//! Assignment allows you to assign another TemporaryKey or ako InstanceKey object to this.
			/*! If this has a different MetaKey than aKey, the collection of columns is
					destroyed and entirely rebuilt before the values from aKey are copied.
			*/
			TemporaryKey &				operator=(Key & aKey)		{ return (TemporaryKey &) Assign(aKey); }

			//! Search for the Entity matching this key
			virtual EntityPtr			GetEntity(DatabasePtr pDB = NULL);

		protected:
			//! Creates the columns for this based on the MetaKey associated with this.
			void									CreateColumns();

			//! Deletes the columns.
			void									DeleteColumns();

			//! Assign() is overloaded such that this can be appropriately adjusted if this has a different MetaKey than aKey.
			//Key &									Assign(Key & aKey);

			//! Helper for ctor() that allows initialisation from XML or JSON
			void									InitializeFromJSON(const std::string & strJSONObjectID);
			//! Helper for ctor() that allows initialisation from XML or JSON
			void									InitializeFromJSON(Json::Value & jsonKeyID);
	};





	//! Instances of this class represent a Key from a particular Entity
	/*!
	*/
	class D3_API InstanceKey : public Key
	{
		friend class MetaDatabase;
		friend class MetaKey;
		friend class Entity;
		friend class Column;
		friend class Relation;
		friend class ODBCDatabase;
		friend class OTLDatabase;

		D3_CLASS_DECL(InstanceKey);

		protected:
			//! The entity of which this is a key
			EntityPtr							m_pEntity;

			//! Member is set to true on BeforeUpdate() and false on AfterUpdate()
			bool									m_bUpdating;

			//! Member is set to true on BeforeUpdate() and false on AfterUpdate()
			TemporaryKeyPtr				m_pOriginalKey;

			//! The common ctor used by MetaKey::CreateInstance()
			InstanceKey() : m_pEntity(NULL), m_bUpdating(false), m_pOriginalKey(NULL) {}

			//! The destructor deletes all source relations
			~InstanceKey();

		public:

			//! Entity Accessor
			virtual EntityPtr			GetEntity()														{ return m_pEntity; }

			//! Assignment
			InstanceKey &					operator=(InstanceKey & aKey)					{ return (InstanceKey&) Assign(aKey); }

			//! The original key value
			/*! If a key is modified it will
					create a temporary key holding it's original values while processing the
					On_BeforeUpdate() notification. If a temporary key already exists at this
					time, no action is taken.

					The original key value is of particular importance for primary keys. The
					primary key is typically used to address physical objects during an
					update/delete process on a physical store. The physical store will have to
					get the primary key's original values to reference the correct object in
					the store through this method.

					This key remains in place until an this
					receives an On_AfterUpdateEntity() message.
			*/
			KeyPtr								GetOriginalKey();

		protected:
			virtual void					On_PostCreate();

			//! Notification than a Relation which has this as its parent Key has been created (ignored)
			void									On_ParentRelationCreated(RelationPtr pRel);
			//! Notification than a Relation which has this as its parent Key has been deleted (ignored)
			void									On_ParentRelationDeleted(RelationPtr pRel);

			//! Notification than a Relation which has this as its child Key has been created (ignored)
			void									On_ChildRelationCreated(RelationPtr pRel);
			//! Notification than a Relation which has this as its child Key has been deleted (ignored)
			void									On_ChildRelationDeleted(RelationPtr pRel);

			//! Notification than a column belonging to this is about to change
			bool									On_BeforeUpdateColumn(ColumnPtr pCol);
			//! Notification than a column belonging to this has been changed
			bool									On_AfterUpdateColumn(ColumnPtr pCol);

			//! Notification that this is about to be updated.
			bool									On_BeforeUpdate();
			//! Notification that this has bee updated.
			bool									On_AfterUpdate();

			//! Notification that the entity has been upated (sent to the primary key only)
			void									On_AfterUpdateEntity();

			//! This method ensures that if this changes, all dependent relations are notified of the change.
			bool									NotifyRelationsBeforeUpdate();
			//! This method ensures that if this changes, all dependent relations are notified of the change.
			bool									NotifyRelationsAfterUpdate();
	};




	inline bool KeyLessPredicate::operator()(const KeyPtr x, const KeyPtr y) const
	{
		return ((*x) < (*y));
	}


} // end namespace D3



#endif /* INC_D3_KEY_H */
