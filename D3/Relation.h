#ifndef INC_D3_RELATION_H
#define INC_D3_RELATION_H

// MODULE: Relation header
//;
// ===========================================================
// File Info:
//
// $Author: daellenj $
// $Date: 2014/08/23 21:47:47 $
// $Revision: 1.27 $
//
// MetaRelation: This class describes relationships between
// meta keys.
//
// Relation: This class holds the details of an actual
// relation
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
#include "Exception.h"
#include "Entity.h"
#include "Column.h"
#include "Key.h"


extern const char szMetaRelationFlags[];					// solely used to ensure type safety (no implementation needed)

namespace D3
{
	//! We store all MetaRelation objects in a map keyed by ID
	/*! Note that objects which are not stored in the meta dictionary (e.g. MetaRelation objects
			created at system startup to enable access to the the MetaDictionary database)
			are assigned MetaRelation::M_uNextInternalID which is then decremented by 1. The
			initial value	of MetaRelation::M_uNextInternalID is RELATION_ID_MAX.
			MetaRelation objects without a physical representation on disk are therefore
			identifiable if GetID() returns a value > MetaRelation::M_uNextInternalID.
	*/
	typedef std::map< RelationID, MetaRelationPtr >		MetaRelationPtrMap;
	typedef MetaRelationPtrMap::iterator							MetaRelationPtrMapItr;

	//! MetaRelation objects describe relationships between MetaKey objects.
	/*! Objects of this class are typically created during meta model MetaDatabase construction.
			A MetaRelation object has two MetaKey objects, one is the parent of the
			relation and the other the child.

			A MetaRelation manifests the fact that it is possible to navigate from an instance
			of the parent key to the corresponding instance(s) of the child key and vice versa.
			This class can handle relationship from parent to child of 0:1 and 0:M and from
			child to parent of 0:1 and 1:1.

			To illustrate how relations are implemented, consider the compatibility (C) between
			ProductGroups (PG). Each PG can have a C with zero or more PG's when above or
			below each other. This represents an	M:M relationship which we would typically
			implement by creating a PG table and a C table:

			<TT><B>
			CREATE TABLE ProductGroup (ID int IDENTITY (1, 1) NOT NULL,	Name vchar (60) NOT NULL)<BR>
			CREATE TABLE Compatibility (PG_ID_A int NOT NULL, PG_ID_B int NOT NULL, Compatibility smallint NULL)
			</B></TT>

			PG.ID is the unique identifier for PG, let's call this PG.PK. C.PG_ID_A points to the
			PG above and C.PG_ID_B to the PG below.	To express the C when PG x is placed above PG y,
			we add a new row z in C and set z.PG_ID_A to x.ID and z.PG_ID_B to y.ID.

			Since we don't want more than one row in C representing the above/below compatibility
			for the same two products, the unique Key for C is	column PG_ID_A and PG_ID_B, let's
			call it C.PK. So given PG x, we can now find the matching C when PG x is above PG y as follows:

			<TT>
			SELECT * FROM C WHERE PG_ID_A = x.ID AND PG_ID_B = y.ID
			</TT>

			If we wanted to know the below C, we simply swap the WHERE predicate elements x.ID and y.ID.
			If we wanted to know all the above C between PG x and any other PG, we do:

			<TT>
			SELECT * FROM C WHERE PG_ID_A = x.ID
			</TT>

			and C.PK will provide efficient lookup. However, if we wanted to find the below C between
			PG x and any other PG, we do:

			<TT>
			SELECT * FROM C WHERE PG_ID_B = x.ID
			</TT>

			and the RDBMS would have to traverse the entire table instead of an index. Therefore, we'd
			add another indexed key to C consisting of PG_ID_B and call it C.FK.

			We would map this model in D3 in that we created MetaKey objects for PG.PK, C.PK and C.FK.
			We'd create two MetaRelation objects as follows:

			<TT>
			MetaRelation		relPGCAbove("is group above in", "where product group below is", PG.PK, C.PK);<BR>
			MetaRelation		relPGCBelow("is group below in", "where product group above is", PG.PK, C.FK);
			</TT>

			The MetaRelation objects will notify the MetaKey objects involved by posting a
			On_MetaRelationCreated() a message. In turn, the MetaKey objects record the relation
			as a parent or a child relation. In our example, PG.PK will have relPGCAbove and
			relPGCBelow as parent relations, C.PK will have relPGCAbove as a child relation
			and c.FK will have relPGCBelow as a child relation.

			When instances of MetaKey objects are created, D3 will create MetaRelation
			instances for each relation of which the key is a parent. Please refer to the
			Relation class for more details.


			Basic requirements:

			Relations demand that a parent key is Unique. In other words, using a non-NULL
			instance of the taget key to navigate to the parent key which is an instance
			of the parent MetaKey must always yield exactly 1 instance. Relational databases
			do not support M:M relationships and therefore, neither does D3.

			If a child key is optional, the system will treat instances of such MetaRelation
			objects as CascadeClearRef. This means that if the parent key is deleted, the child
			key is set to NULL. You can use the M_uPreventDelete flag to overwrite this
			functionality (see member M_uPreventDelete).

			If a child key is mandatory, the system will treat instances of such MetaRelation
			objects as CascadeDelete. This means that if the parent key is deleted, the child
			key is also deleted. You can use the M_uPreventDelete flag to overwrite this
			functionality (see member M_uPreventDelete).
	*/
	class D3_API MetaRelation : public Object
	{
		friend class MetaDatabase;
		friend class MetaKey;
		friend class MetaEntity;
		friend class Relation;
		friend class ODBCDatabase;
		friend class OTLDatabase;

		// Standard D3 stuff
		//
		D3_CLASS_DECL(MetaRelation);

		public:
			//! The Flags bitmask identifies basic characteristics for each type of key
			struct Flags : public Bits<szMetaRelationFlags>
			{
				BITS_DECL(MetaRelation, Flags);

				//@{ Primitive masks
				static const Mask CascadeDelete;			//!< 0x00000001 - Indicates that if the parent key of the relationship is deleted, all the child keys must be deleted also.
				static const Mask CascadeClearRef;		//!< 0x00000002 - Indicates that if the parent key of the relationship is deleted, all the child keys must be set to NULL.
				static const Mask PreventDelete;			//!< 0x00000004 - Indicates that the parent key can't be deleted if there are any related child keys associated.
				static const Mask ParentSwitch;				//!< 0x00000008 - Indicates that child key instances are restricted to take part in the relation based on the parent
				static const Mask ChildSwitch;				//!< 0x00000010 - Indicates that child key instances are restricted to take part in the relation based on the child
				static const Mask HiddenParent;				//!< 0x00000020 - When viewing a child, user can't see parent
				static const Mask HiddenChildren;			//!< 0x00000040 - When viewing a parent, user can't see the children
				//@}

				//@{ Combo masks
				static const Mask Default;						//!< Same as CascadeDelete
				static const Mask Hidden;							//!< Same as (HiddenParent | HiddenChildren)
				//@}
			};


		protected:
			static RelationID						M_uNextInternalID;	//!< If no internal ID is passed to the constructor, use this and decrement afterwards (used for meta dictionary definition meta obejcts)
			static MetaRelationPtrMap		M_mapMetaRelation;	//!< Holds all MetaRelation objects in the system indexed by m_uID

			RelationID									m_uID;							//!< This is the ID of this so that (MetaRelation::M_mapMetaRelation.find(m_uID)->second == this)

			//! The \a Parent to \a Child MetaRelation name (as given at construction time).
			/*! The name should reflect the relation that it represents well.
					Ideally, when concatenating the parent MetaKey objects MetaEntity name
					with this name and the child MetaKey objects MetaEntity name, we should
					end up with some meaningful phrase, e.g.:

					Order consists of OrderItem	       (m_strName = "consists of")<BR>
					Container is utilised by Shipment  (m_strName = "is utilised by")

					The methods GetFullName() and GetFullReverseName() build and return strings
					concatenated in this way.
			*/
			std::string							m_strName;
			std::string							m_strProsaName;
			std::string							m_strDescription;
			//! The \a Child to \a Parent MetaRelation name (as given at construction time).
			/*! The same rules apply as m_strName but parent and child are reversed.
			*/
			std::string							m_strReverseName;
			std::string							m_strReverseProsaName;
			std::string							m_strReverseDescription;
			MetaKeyPtr							m_pParentKey;							//!< The MetaKey object that represents the parent key for this relation
			MetaKeyPtr							m_pChildKey;							//!< The MetaKey object that represents the child key for this relation
			const Class*						m_pInstanceClass;					//!< The Class object that knows how to construct a Relation object (typically the Relation::ClassObject())
			std::string							m_strInstanceInterface;		//!< The name of the interface class that can wrap an instance of this type
			std::string							m_strJSMetaClass;					//!< The name of the JavaScript meta class representing this in the browser
			std::string							m_strJSInstanceClass;			//!< The name of the JavaScript instance class representing this in the browser
			std::string							m_strJSParentViewClass;		//!< The name of the JavaScript widget that knows how to render the parent of instances of this type as a APALUI widget in the browser (full)
			std::string							m_strJSChildViewClass;		//!< The name of the JavaScript widget that knows how to render the children of instances of this type as a APALUI widget in the browser (full)
			RelationIndex						m_uParentIdx;							//!< The index that meets the criteria: m_pChildKey->GetMetaEntity()->GetParentRelation(idx) == this
			RelationIndex						m_uChildIdx;							//!< The index that meets the criteria: m_pParentKey->GetMetaEntity()->GetChildRelation(idx) == this
			ColumnPtr								m_pSwitchColumn;					//!< If not NULL, this member is a switch
			Flags										m_Flags;									//!< See MetaRelation::Flags above
			std::string							m_strHSTopicsJSON;				//!< JSON string containing an array of help topics associated with this

			//! Unused ctor() - only here for D3 Class stuff
			MetaRelation();

			//! The ctor used to construct objects from their phisical database representation
			/*! This ctor allows the meta model initialisation process to create a MetaRelation object
					which will create a custom Relation type when needed. This can be useful if
					non standard work needs to be performed in addition to what D3 does.

					@param uID						The unique ID this' persistent representation has in the meta dictionary database
																for details.
					@param strName				The Parent to Child relation name (see member #m_strName)
																for details.
					@param strReverseName	The Child to Parent relation name (see member #m_strReverseName)
																for details.
					@param pParentKey			A pointer to the MetaKey object which is the parent for this relation
					@param pChildKey			A pointer to the MetaKey object which is the child for this relation
					@param strClassName		The name of the class which is used to create instances of this MetaRelation.
																A D3::Class object with the name you specify must exist and be
																a kind of an Relation class. CreateInstance() will create, initialise and
																return an object of the specified class.
					@param flags					See MetaKey::Flags (the default makes this a CascadeDelete relation)
					@param pSwitchColumn	The column who's value determines whether or not this is part of the relation
					@param strSwitchColumnValue	If pSwitchColumn contains this value the relationship applies
			*/
			MetaRelation(RelationID uID, const std::string & strName, const std::string & strReverseName, MetaKeyPtr pParentKey, MetaKeyPtr pChildKey, const std::string & strClassName, const Flags::Mask & flags = Flags::Default, MetaColumnPtr pSwitchColumn = NULL, const std::string & strSwitchColumnValue = "");

			//! The ctor used to construct objects to access the meta dictionary database
			MetaRelation(const std::string & strName, const std::string & strReverseName, MetaKeyPtr pParentKey, MetaKeyPtr pChildKey, const std::string & strClassName, const Flags::Mask & flags = Flags::Default, MetaColumnPtr pSwitchColumn = NULL, const std::string & strSwitchColumnValue = "");

			//! The destructor notifies the MetaKey objects involved that it is dying by posting On_MetaRelationDeleted(this) messages.
			~MetaRelation();

			//! Internal helper that initialises this after construction
			void										Init(const std::string & strInstanceClassName, MetaColumnPtr pSwitchColumn, const std::string & strSwitchColumnValue);

		public:

			const std::string	&			GetName() const												{ return m_strName; }					//!< Returns the name of the relation (see #m_strName for details)
			const std::string	&			GetProsaName() const									{ return m_strProsaName; }		//!< Returns the prosa name of the relation (see #m_strProsaName for details)
			const std::string	&			GetDescription() const								{ return m_strDescription; }	//!< Returns the description of the relation (see #m_strDescription for details)
			const std::string	&			GetReverseName() const								{ return m_strReverseName; }	//!< Returns the reverse name of the relation (see #m_strReverseName for details)
			const std::string	&			GetReverseProsaName() const						{ return m_strReverseProsaName; }		//!< Returns the reverse prosa name of the relation (see #m_strReverseProsaName for details)
			const std::string	&			GetReverseDescription() const					{ return m_strReverseDescription; }	//!< Returns the reverse description of the relation (see #m_strReverseDescription for details)
			std::string							GetFullName() const;																								//!< Returns the full name of the relation (see #m_strName for details)
			std::string							GetFullReverseName() const;																					//!< Returns the full reverse name of the relation (see #m_strName for details)
			MetaKeyPtr							GetParentMetaKey() const							{ return m_pParentKey; }			//!< Returns the MetaKey object which is the parent of this relationship
			MetaKeyPtr							GetChildMetaKey() const								{ return m_pChildKey; }				//!< Returns the MetaKey object which is the child of this relationship
			RelationIndex						GetParentIdx() const									{ return m_uParentIdx; }			//!< Returns the index with which this relation can be located in the vector of parent relations the child entity holds
			RelationIndex						GetChildIdx() const										{ return m_uChildIdx; }				//!< Returns the index with which this relation can be located in the vector of child relations the parent entity holds
			//! Returns the instance class name this uses
			const std::string &			GetInstanceClassName() const					{ return m_pInstanceClass->Name(); }
			//! Returns the name of the interface that is expected to wrap an instance of this type
			const std::string &			GetInstanceInterface()								{ return  m_strInstanceInterface;}
			//! Returns the JavaScript MetaClass to use on the browser side
			const std::string &			GetJSMetaClassName() const						{ return m_strJSMetaClass; }
			//! Returns the JavaScript InstanceClass to use on the browser side
			const std::string &			GetJSInstanceClassName() const				{ return m_strJSInstanceClass; }
			//! Returns the JavaScript ParentRelationView widget to use on the browser side
			const std::string &			GetJSParentViewClassName() const			{ return m_strJSParentViewClass; }
			//! Returns the JavaScript ChildRelationView widget to use on the browser side
			const std::string &			GetJSChildViewClassName() const			{ return m_strJSChildViewClass; }

			//! GetID returns a unique identifier which can be used to retrieve the same object using GetMetaRelationByID(unsigned int)
			RelationID							GetID() const													{ return m_uID; }
			//! Return the meta relation where GetID() == uID
			static MetaRelationPtr	GetMetaRelationByID(RelationID uID)
			{
				MetaRelationPtrMapItr		itr = M_mapMetaRelation.find(uID);
				return (itr == M_mapMetaRelation.end() ? NULL : itr->second);
			}

			//! Returns a list of all MetaRelations in the system.
			static MetaRelationPtrMap&	GetMetaRelations()								{ return M_mapMetaRelation; };

			//! Returns a name that is guaranteed to be unique for all child relations of this' parent entity
			/*! This methods purpose is to support code generation which requires unique names
			*/
			std::string							GetUniqueName() const;

			//! Returns a name that is guaranteed to be unique for all parent relations of this' child entity
			/*! This methods purpose is to support code generation which requires unique names
			*/
			std::string							GetUniqueReverseName() const;

			//! Create an instance of this MetaRelation
			/*! @param pEntity				 This is the owner of the relation. The rule is that
																 the parent key of this is a unique key of the MetaEntity
																 of which pEntity is an instance of.

					@param pChildDatabase  All child InstanceKey objects stored in the Relation
																 are from this and only this database.

					@return The method returns a correctly wired Relation object. If a special class
									factory was provided during construction of the MetaRelation, the object
									returned may by of a class type other than Relation, but based on Relation.
			*/
			RelationPtr							CreateInstance(EntityPtr pEntity, DatabasePtr pChildDatabase);

			//{@ Characteristics access
			//! Return the relations characteristics bitmask
			const Flags&						GetFlags() const										{ return m_Flags; }

			//! If true, the relationship is a CascadeDelete relationship.
			/*! CascadeDelete relationships ensure that all childs of the relation are also
					deleted if the parent is deleted.
			*/
			bool										IsCascadeDelete() const							{ return (m_Flags & Flags::CascadeDelete); }
			//! If true, the relationship is a CascadeClearRef relationship.
			/*! CascadeClearRef relationships ensure that all child keys of the relation
					are set to NULL if the parent is deleted.
			*/
			bool										IsCascadeClearRef() const						{ return (m_Flags & Flags::CascadeClearRef); }
			//! If true, the relationship is a PreventDelete relationship.
			/*! PreventDelete relationships ensure that you can't delete a
					parent key if any child key is referencing it.
			*/
			bool										IsPreventDelete() const							{ return (m_Flags & Flags::PreventDelete); }
			//! If true, the relationship is a switched relation
			bool										IsSwitched() const									{ return IsParentSwitch() || IsChildSwitch(); }
			//! If true, the relation membership is dependent on parent characteristic.
			bool										IsParentSwitch() const							{ return (m_Flags & Flags::ParentSwitch); }
			//! If true, the relation membership is dependent on child characteristic.
			bool										IsChildSwitch() const								{ return (m_Flags & Flags::ChildSwitch); }
			//! If true, the parent is hidden from the user when looking at a child
			bool										IsHiddenParent() const							{ return (m_Flags & Flags::HiddenParent); }
			//! If true, children are hidden from the user when looking at the parent
			bool										IsHiddenChildren() const						{ return (m_Flags & Flags::HiddenChildren); }
			//@}


			//! Returns a column that contains the switch value
			ColumnPtr								GetSwitchColumn()											{ return m_pSwitchColumn; }

			//! Returns all known and accessible instances of D3::MetaRelation as a JSON stream
			static std::ostream &			AllAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm);

			//! Inserts this meta relation into the passed-in JSON stream
			/*! If a relation is inserted in the context of a meta entity, then output to the stream is
					reduced since it is assumed that any keys involved in this relation wich belong to the
					meta entity in context have already been stream so all you get is that keys ID.
			*/
			std::ostream &					AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, MetaEntityPtr pContext, bool* pFirstChild = NULL, bool bShallow = false);

		protected:
			//! Called by #MetaDatabase::LoadMetaObject().
			/*!	The method expects that the object passed in is an instance of the D3MetaRelation
					MetaEntity and constructs a new MetaRelation object based on pD3MR which acts as
					a template.
			*/
			static void							LoadMetaObject(D3MetaRelationPtr pD3MR);


			//!< Notification from an Relation object based on this after it has been created successfully.
			/*! Notification is ignored. */
			void										On_InstanceCreated(RelationPtr pRelation)		{}

			//!< Notification from an Relation object based on this before it is being deleted.
			/*! Notification is ignored. */
			void										On_InstanceDeleted(RelationPtr pRelation)		{}

			//! Create a root meta relation which only has a child but no parent
			/*! @todo Root relations are neither documented nor implemented yet.
			*/


			//! Helper which returns true if the keys are compatible
			/*!	To be considered compatible the two key's must meet the following criteria

					- The parent key must have no more columns than the child key

					- All parent key columns must match the columns of the child key
					  in the same order. Matching columns are of the same type and if
						strings, of the same length.

					@return Returns true if the parent and child keys are compatible,
					false otherwise.
			*/
			bool										VerifyKeyCompatibility();

			//! This method is needed for trigger code generation.
			/*! The mothod populates the list with MetaRelation objects which are between the
					same two keys. Relations between the same two keys is only appropriate if
					the relations are switched (typed).
			*/
			void										GetSynonymRelations(MetaRelationPtrList & listMR);
			public:
			//! Build a string that can be used to create a Check Insert/Update trigger (ciu).
			/*! Insert/Update check triggers guarantee that any row added to the child table
					have a matching key in the parent table.
			*/
			std::string							AsCreateCheckInsertUpdateTrigger(TargetRDBMS eTargetDB);

			std::string							AsCreateCascadePackage(TargetRDBMS eTargetDB);
			//! Build a string that can be used to create a CascadeDelete trigger (cd).
			/*! CascadeDelete triggers will be generated for relations marked as CascadeDelete.
					The trigger ensures that all child's are deleted also when the parent is deleted.
			*/
			std::string							AsCreateCascadeDeleteTrigger(TargetRDBMS eTargetDB);

			//! Build a string that can be used to create a CascadeClearRef trigger (ccr).
			/*! CascadeClearRef triggers will be generated for relations marked as CascadeClearRef.
					The trigger ensures that all child keys are set to NULL when the parent is deleted.
			*/
			std::string							AsCreateCascadeClearRefTrigger(TargetRDBMS eTargetDB);

			//! Build a string that can be used to create a VerifyDelete trigger (vd).
			/*! VerifyDelete triggers will be generated for relations marked as PreventDelete.
					The trigger raises an SQL Error if the parent is deleted when childs exist
					an cause such an update transaction to be Rollback'ed.
			*/
			std::string							AsCreateVerifyDeleteTrigger(TargetRDBMS eTargetDB);

			//! Build a string that can be used to create a UpdateForeignReference trigger (ufr).
			/*! VerifyDelete triggers will be generated for relations marked as PreventDelete.
					The trigger raises an SQL Error if the parent is deleted when childs exist
					an cause such an update transaction to be Rollback'ed.
			*/
			std::string							AsUpdateForeignRefTrigger(TargetRDBMS eTargetDB);

			//! Sets the child JSON HSTopic array
			void										SetHSTopics(const std::string & strHSTopicsJSON) { m_strHSTopicsJSON = strHSTopicsJSON; }
	};






	//============================================================================================

	//! Relation objects provide a mechanism to navigate between related keys.
	/*! Relation objects are instances of MetaRelation objects. They are owned by the
			parent key.

			You use Relation objects by obtaining the MetaRelation object through the parent
			or child Key object's MetaKey. You then ask the key for the instance Relation
			object representing this MetaRelation object.

			Relation object allow traversal of relations. Based on the ProductGroup and Compatibility
			example presented in the documentation for the MetaRelation class, let's assume we have
			a PG.PK instance pPGPrimaryKey and we wanted to iterate through all related below C, we
			would do this as follows:

\todo Implement an efficient mechanism to get the correct Relation quickly. For example, if I have
      a ProductGroup \a x and want to traverse all Compatibility objects where ProductGroup \a x
			is the group above, I need the instance of the MetaRelation "ProductGroup is group above in
			Compatibility". To get this MetaRelation, I firstly need to know that it is a parent MetaRelation
			of the product group's MetaKey \a PG.PK, but secondly, I also need to know the name and the child
			key of the relation. This seems a bit heavy.

\code
// Find the best compatibility value when pPGPrimaryKey is above another PG
//
short FindBestCompatibilityWithGroupBelow(InstanceKeyPtr pPGPrimaryKey)
{
	MetaRelationPtr			pMR;
	RelationPtr					pIR;
	KeyPtrMultiSetItr		itrChildKey;
	KeyPtr							pChildKey;
	ColumnPtr						pCompatibility;
	short								iBestCompatibility = SINT_MAX;

	pMR = pPGPrimaryKey->GetMetaKey()->GetParentMetaRelation("ProductGroup is group above in Compatibility");
	pIR = pPGPrimaryKey->GetParentRelation(pMR);

	for ( itrChildKey =	pIR->GetChildKeys()->begin();
				itrChildKey != pIR->GetChildKeys()->end();
				itrChildKey++)
	{
		pChildKey = *itrChildKey;
		pCompatibility = pChildKey->GetEntity()->GetColumn("Compatibility");

		if (iBestCompatibility > *pCompatibility)
			iBestCompatibility = (short) *pCompatibility;
	}
}
\endcode

			Relation objects are created by the parent Key for each MetaRelation object that figures in it's
			MetaKey object's MetaKey::GetParentMetaRelation() collection.

			The Relation object does no navigation at creation time. Navigation occurs only when the Relation
			has not already navigated and a client requests the child Key objects through Relation::GetChildKeys().
	*/
	class D3_API Relation : public Object
	{
		friend class MetaRelation;		//!< These objects create instances of this class.
		friend class InstanceKey;			//!< Expects notifications
		friend class Entity;					//!< These objects assume ownership.

		D3_CLASS_DECL(Relation);

		protected:
			#define D3_RELATION_UPDATING	  0x01						//<! flag indicating whether or not this is in the process of being updated
			#define D3_RELATION_NAVIAGETED	0x02						//<! flag indicating whether or not this has been naviageted
			#define D3_RELATION_DESTROYING	0x04						//<! flag indicating that this is in the process of being destroyed


			MetaRelationPtr							m_pMetaRelation;		//!< The MetaRelation object of which this is an instance
			InstanceKeyPtr							m_pParentKey;				//!< The InstanceKey objecrt which is the parent of this realtion
			DatabasePtr									m_pChildDatabase;		//!< Holds relations with children from this database
			InstanceKeyPtrSet						m_setChildKey;			//!< Holds InstanceKey objects which are the child of this relation
			EntityPtr										m_pPendingChild;		//!< Set when an operation on a child commences and set to NULL when operation is complete (AddChild() and RemoveChild()).
			unsigned short							m_uFlags;						//!< Holds flags as defined by the #defines above.


			//! This constructor is used by MetaEntity::CreateInstance()
			Relation() : m_pMetaRelation(NULL), m_pParentKey(NULL), m_pChildDatabase(NULL), m_pPendingChild(NULL), m_uFlags(0) {}
			//! The destructor posts messages to the parent and all child keys as well as the MetaEntity.
			~Relation();

		public:
			/** @name Iteration
					This class supports iteration. This means that you can treat a Relation
					object similar to a stl multiset.
			*/
			//@{
			//! Relation specific iterator
			class D3_API iterator : public InstanceKeyPtrSetItr
			{
				public:
					iterator()	{};
					iterator(const InstanceKeyPtrSetItr& itr) : InstanceKeyPtrSetItr(itr) {}

					EntityPtr		operator*();
					iterator&		operator=(const iterator& itr);
			};

			//! Returns the number of children in this Relation
			unsigned int								size()				{ return m_setChildKey.size(); }

			//! Returns true if this has no children
			bool												empty()				{ return m_setChildKey.empty(); }

			//! Return the first child in this' relation or NULL if there is none
			EntityPtr										front();

			//! Return the last child in this' relation or NULL if there is none
			EntityPtr										back();

			//! Return a Relation::iterator pointing to the first child
			iterator										begin()				{ return iterator(m_setChildKey.begin()); }

			//! Return a Relation::iterator pointing one beyond the last child
			iterator										end()					{ return iterator(m_setChildKey.end()); }
			//@}

			//! Removes a child
			void												RemoveChild(EntityPtr pChild);
			//! Removes a child
			void												AddChild(EntityPtr pChild);
			//! Removes all children
			void												RemoveAllChildren();


			//! Returns the MetaRelation object of which this is an instance
			MetaRelationPtr							GetMetaRelation() const		{ return m_pMetaRelation; }

			//! Returns the MetaRelation object of which this is an instance
			InstanceKeyPtr							GetParentKey()						{ return m_pParentKey; }
			//! Return the InstanceKey object which is the child of this relation
			InstanceKeyPtr							GetChildKey();
			//! Returns a stl multiset of all related child InstanceKey objects
			InstanceKeyPtrSet&					GetChildKeys()						{ return m_setChildKey; }

			//! Returns the EntityPtr which is the parent of this relation
			EntityPtr										GetParent()								{ if (!m_pParentKey) return NULL; return m_pParentKey->GetEntity(); }
			//! Return the EntityPtr to the child of this relation (only if 0:1 or 1:1 relation)
			EntityPtr										GetChild()								{ if (m_setChildKey.size() != 1) return NULL; return front(); }
			//! Returns the database of the entity which owns the parent key

			DatabasePtr									GetParentDatabase();
			//! Returns the database of the entity(s) which own the child key(s)
			DatabasePtr									GetChildDatabase()				{ return m_pChildDatabase; }

			//! Populates the Relation
			void												LoadAll(bool bRefresh = false, bool bLazyFetch = true);

			//! Writes the complete contents of this as XML to the out stream
			virtual std::ostream &			AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, bool* pFirstChild = NULL);

			//! Appends the child entities of this relation to the passed-in EntityPtrList
			/*! Use this method in cases where you iterate over the children in a relation and make changes
					one or more of the children. If you make changes to columns	which are key members, this may
					result in the relation you're iterating to be modified and this would case a GPF in the next
					iteration.
			*/
			void												GetSnapshot(EntityPtrListPtr pEL);

		protected:
			//! This method returns true if this is not a switched relation or if the child meets the switches criteria.
			virtual bool								IsValidChild(EntityPtr pKey);


			bool												IsUpdating()							{ return m_uFlags & D3_RELATION_UPDATING   ? true : false; };		//!< Return true if this is currently in the process of updating
			bool												IsNavigated()							{ return m_uFlags & D3_RELATION_NAVIAGETED ? true : false; };		//!< Return true if this has been navigated
			bool												IsDestroying()						{ return m_uFlags & D3_RELATION_DESTROYING ? true : false; };		//!< Return true if this is currently being destroyed

			void												MarkUpdating()						{ m_uFlags |= D3_RELATION_UPDATING; };		//!< Mark this as being in the process of updating
			void												MarkNavigated()						{ m_uFlags |= D3_RELATION_NAVIAGETED; };	//!< Mark this as having been navigated
			void												MarkDestroying()					{ m_uFlags |= D3_RELATION_DESTROYING; };	//!< Mark this as being in the process of destroying the object

			void												UnMarkUpdating()					{ m_uFlags &= ~D3_RELATION_UPDATING; };		//!< Clear updating flag
			void												UnMarkNavigated()					{ m_uFlags &= ~D3_RELATION_NAVIAGETED; };	//!< Clear navigated flag
			void												UnMarkDestroying()				{ m_uFlags &= ~D3_RELATION_DESTROYING; };	//!< Clear destroying flag



			//! Populates the list passed in with all of this' children
			void												GetChildrenSnapshot(EntityPtrList & listEntity);

			//! Searches the matching key from pChild in m_setChildKey list and returns an iterator
			InstanceKeyPtrSetItr				FindChild(InstanceKeyPtr pChild, bool bShouldExist = false);

			//! This message is sent by MetaRelation::CreateInstance()
			/*! The method notifies it's parent InstanceKey object that it has sprung into life
			*/
			virtual void								On_PostCreate();

			//! Message sent by the parent key of this before it is being updated
			void												On_BeforeUpdateParentKey(InstanceKeyPtr pParentKey) { MarkUpdating(); }
			//! Message sent by the parent key of this after it has been updated
			void												On_AfterUpdateParentKey(InstanceKeyPtr pParentKey);

			//! Message sent by a child key of this before it is being updated
			void												On_BeforeUpdateChildKey(InstanceKeyPtr pParentKey);
			//! Message sent by a child key of this after it has been updated
			/*! Under normal circumstances, the On_BeforeUpdateRelation() notification received prior
					to this notification will have removed the child and subsequent changes to the former
					childs key would have resulted in the child having been inserted into the correct relation.
					However, consider that the foreign key of entity y is a reference to an autonum column in
					entity x. If you create a new instance of entity x and y and relate y with x, x has a NULL
					foreign reference key. Therefore, On_BeforeUpdateRelation will not remove NULL keys.
			*/
			void												On_AfterUpdateChildKey(InstanceKeyPtr pParentKey);
	};






} // end namespace D3

#endif /* INC_D3_RELATION_H */
