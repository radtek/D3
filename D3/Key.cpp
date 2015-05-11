// MODULE: Key source
//;
//; IMPLEMENTATION CLASS: MetaKey, Key, InstanceKey, TemporaryKey
//;
// ===========================================================
// File Info:
//
// $Author: daellenj $
// $Date: 2014/11/07 05:25:15 $
// $Revision: 1.45 $
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

// @@DatatypeInclude
#include "D3Types.h"

#include <time.h>
#include <stdio.h>
#include <sstream>

#include "Key.h"
#include "Entity.h"
#include "Column.h"
#include "Relation.h"
#include "D3Funcs.h"
#include "Session.h"

#include <Codec.h>

namespace D3
{


	// ==========================================================================
	// MetaKey::Flags class implementation
	//

	// Flags Implementation
	BITS_IMPL(MetaKey, Flags);

	// Primitive mask values
	PRIMITIVEMASK_IMPL(MetaKey, Flags, Mandatory,							0x0001);
	PRIMITIVEMASK_IMPL(MetaKey, Flags, Unique,								0x0002);
	PRIMITIVEMASK_IMPL(MetaKey, Flags, AutoNum,								0x0004);
	PRIMITIVEMASK_IMPL(MetaKey, Flags, Primary,								0x0010);
	PRIMITIVEMASK_IMPL(MetaKey, Flags, Secondary,							0x0020);
	PRIMITIVEMASK_IMPL(MetaKey, Flags, Foreign,								0x0040);
	PRIMITIVEMASK_IMPL(MetaKey, Flags, Conceptual,						0x0080);
	PRIMITIVEMASK_IMPL(MetaKey, Flags, HiddenOnQuickAccess,		0x0008);
	PRIMITIVEMASK_IMPL(MetaKey, Flags, DefaultQuickAccess,		0x0100);
	PRIMITIVEMASK_IMPL(MetaKey, Flags, AutocompleteQuickAccess,0x0200);

	// Combo mask values
	COMBOMASK_IMPL(MetaKey, Flags, PrimaryKey,								(MetaKey::Flags::Primary		| MetaKey::Flags::Unique		| MetaKey::Flags::Mandatory));
	COMBOMASK_IMPL(MetaKey, Flags, PrimaryAutoNumKey,					(MetaKey::Flags::Primary		| MetaKey::Flags::Unique		| MetaKey::Flags::Mandatory | MetaKey::Flags::AutoNum));
	COMBOMASK_IMPL(MetaKey, Flags, OptionalSecondaryKey,			(MetaKey::Flags::Secondary));
	COMBOMASK_IMPL(MetaKey, Flags, MandatorySecondaryKey,			(MetaKey::Flags::Secondary	| MetaKey::Flags::Mandatory));
	COMBOMASK_IMPL(MetaKey, Flags, UniqueSecondaryKey,				(MetaKey::Flags::Secondary	| MetaKey::Flags::Mandatory | MetaKey::Flags::Unique));
	COMBOMASK_IMPL(MetaKey, Flags, OptionalForeignKey,				(MetaKey::Flags::Foreign));
	COMBOMASK_IMPL(MetaKey, Flags, MandatoryForeignKey,				(MetaKey::Flags::Foreign		| MetaKey::Flags::Mandatory));
	COMBOMASK_IMPL(MetaKey, Flags, ConceptualKey,							(MetaKey::Flags::Conceptual | MetaKey::Flags::Mandatory));
	COMBOMASK_IMPL(MetaKey, Flags, UniqueConceptualKey,				(MetaKey::Flags::Conceptual | MetaKey::Flags::Mandatory | MetaKey::Flags::Unique));
	COMBOMASK_IMPL(MetaKey, Flags, Searchable,								(MetaKey::Flags::Primary		| MetaKey::Flags::Secondary | MetaKey::Flags::Foreign));
	COMBOMASK_IMPL(MetaKey, Flags, Default,										0x0000);


	// ===========================================================
	// MetaKey class implementation
	//

	// Statics:
	//
	KeyID																MetaKey::M_uNextInternalID = KEY_ID_MAX;
	MetaKeyPtrMap												MetaKey::M_mapMetaKey;

	// Standard D3 Stuff
	//
	D3_CLASS_IMPL(MetaKey, Object);


	// ctor
	//
	MetaKey::MetaKey()
	{
		// This constructor isn't prepared for use yet
		assert(false);
	}




	MetaKey::MetaKey(MetaEntityPtr pMetaEntity, const std::string & strName, const Flags::Mask & flags)
	: m_pMetaEntity(pMetaEntity),
		m_uID(M_uNextInternalID--),
		m_strName(strName),
		m_Flags(flags),
		m_pInstanceClass(NULL),
		m_uKeyIdx(D3_UNDEFINED_ID)
	{
		Init("InstanceKey");
	}



	MetaKey::MetaKey(MetaEntityPtr pMetaEntity, KeyID uID, const std::string & strName, const std::string & strClassName, const Flags::Mask & flags)
	: m_pMetaEntity(pMetaEntity),
		m_uID(uID),
		m_strName(strName),
		m_Flags(flags),
		m_pInstanceClass(NULL),
		m_uKeyIdx(D3_UNDEFINED_ID)
	{
		Init(strClassName);
	}



	void MetaKey::Init(const std::string & strInstanceClassName)
	{
		MetaKeyPtr			pMK;
		std::string			strClassName = strInstanceClassName;

		assert(m_pMetaEntity);

		if (strClassName.empty())
			strClassName = "InstanceKey";

		try
		{
			m_pInstanceClass = &(Class::Of(strClassName));
		}
		catch(Exception & e)
		{
			e.AddMessage("MetaKey::Initialise(): Error creating MetaKey %s, the instance class name %s is invalid, using default 'InstanceKey' class.", m_strName.c_str(), strInstanceClassName.c_str());
			e.LogError();
			m_pInstanceClass = (ClassPtr) &InstanceKey::ClassObject();
		}

		// If a MetaKey with the same name already exists, we must throw an Exception (but not just yet)
		pMK = m_pMetaEntity->GetMetaKey(m_strName);

		// If this is a duplicate, throw an exception
		if (pMK)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaKey::MetaKey(): Key %s already exists!", GetFullName().c_str());

		// Notify the MetaEntity object regardless of whether or not this is a duplicate
		m_pMetaEntity->On_MetaKeyCreated(this);

		assert(M_mapMetaKey.find(m_uID) == M_mapMetaKey.end());
		M_mapMetaKey[m_uID] = this;
	}



	MetaKey::~MetaKey()
	{
		MetaColumnPtrListItr	itrColumn;
		MetaColumnPtr					pMC;


		// Notify all this' columns that it has been deleted
		//
		for ( itrColumn =  m_listColumn.begin();
					itrColumn != m_listColumn.end();
					itrColumn++)
		{
			pMC = *itrColumn;
			pMC->On_RemovedFromMetaKey(this);
		}


		// At this stage we should have no instances!!!
		//
		assert(m_mapInstanceKeySet.empty());

		m_pMetaEntity->On_MetaKeyDeleted(this);

		M_mapMetaKey.erase(m_uID);
	}



	// Construct a MetaKey object from a D3MetaKey object (a D3MetaKey object
	// represents a record from the D3MetaKey table in the D3 MetaDictionary database)
	//
	/* static */
	void MetaKey::LoadMetaObject(MetaEntityPtr pNewME, D3MetaKeyPtr pD3MK)
	{
		MetaKeyPtr															pNewMK;
		MetaColumnPtr														pMC;
		D3MetaKey::D3MetaKeyColumns::iterator		itrMKC;
		D3MetaKeyColumnPtr											pMKC;
		std::string															strName;
		std::string															strClassName;
		Flags::Mask															flags;


		assert(pD3MK);

		// Get the name of the object to create
		//
		strName = pD3MK->GetName();

		// Get the instance class name for the MetaKey
		//
		if (!pD3MK->GetColumn(D3MDDB_D3MetaKey_InstanceClass)->IsNull())
			strClassName = pD3MK->GetInstanceClass();
		else
			strClassName = "InstanceKey";

		// Set the construction flags
		//
		if (!pD3MK->GetColumn(D3MDDB_D3MetaKey_Flags)->IsNull())
			flags = Flags::Mask(pD3MK->GetFlags());

		// Create matching MetaKey object
		//
		pNewMK = new MetaKey(pNewME, pD3MK->GetID(), strName, strClassName, flags);

		pNewMK->m_strProsaName = pD3MK->GetProsaName();
		pNewMK->m_strDescription = pD3MK->GetDescription();

		// Set all the other attributes
		//
		pNewMK->m_strInstanceInterface	= pD3MK->GetInstanceInterface();
		pNewMK->m_strJSMetaClass				= pD3MK->GetJSMetaClass();
		pNewMK->m_strJSInstanceClass		= pD3MK->GetJSInstanceClass();
		pNewMK->m_strJSViewClass				= pD3MK->GetJSViewClass();

		// Add MetaColumn objects
		//
		for ( itrMKC =  pD3MK->GetD3MetaKeyColumns()->begin();
					itrMKC != pD3MK->GetD3MetaKeyColumns()->end();
					itrMKC++)
		{
			pMKC = *itrMKC;

			pMC = MetaColumn::GetMetaColumnByID(pMKC->GetMetaColumnID());
			assert(pMC);
			assert(pMC->GetMetaEntity() == pNewME);
			pNewMK->AddMetaColumn(pMC);
		}
	}



	InstanceKeyPtr MetaKey::CreateInstance(EntityPtr pEntity)
	{
		InstanceKeyPtr	pKey = NULL;

		if (!m_pInstanceClass)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaKey::CreateInstance(): Can't create instance of key %s because class factory is missing!", GetFullName().c_str());

		pKey = (InstanceKeyPtr) m_pInstanceClass->CreateInstance();

		if (!pKey)
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaKey::CreateInstance(): Class factory %s failed to create instance of key %s!", m_pInstanceClass->Name().c_str(), GetFullName().c_str());

		pKey->m_pMetaKey = this;
		pKey->m_pEntity = pEntity;
		pKey->On_PostCreate();

		return pKey;
	}




	std::string MetaKey::GetFullName() const
	{
		std::string strFullName;

		strFullName = m_pMetaEntity->GetFullName();
		strFullName += ".";
		strFullName += m_strName;

		return strFullName;
	}




	// Return the Entity who's key matches the key passed in
	//
	InstanceKeyPtr MetaKey::FindInstanceKey(KeyPtr pKey, DatabasePtr pDatabase)
	{
		InstanceKeyPtrSetPtr				pKeySet;
		InstanceKeyPtrSetItr				itrKeySet;
		DatabasePtr									pDB = pDatabase;;


		// Sanity checks
		//
		assert(pKey);

		if (!pDB)
		{
			assert(pKey->GetEntity());
			pDB = pKey->GetEntity()->GetDatabase();
		}

		assert(pDB);

		// Locate the correct recordset
		//
		pKeySet = GetInstanceKeySet(pDB);

		if (!pKeySet)
			return NULL;

		// Locate the key
		//
		itrKeySet = pKeySet->find(pKey);

		if (itrKeySet == pKeySet->end())
			return NULL;


		return (InstanceKeyPtr) *itrKeySet;
	}






	// FindObject calls FindInstanceKey but instead of returning the key it returns the object that onws the key (method may return NULL)
	EntityPtr MetaKey::FindObject(KeyPtr pKey, DatabasePtr pDatabase)
	{
		InstanceKeyPtr pIK;


		pIK = FindInstanceKey(pKey, pDatabase);

		if (!pIK)
			return NULL;

		return pIK->GetEntity();
	}




	// Return the Entity who's key matches the key passed in
	//
	EntityPtr MetaKey::LoadObject(KeyPtr pKey, DatabasePtr pDatabase, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr			pDB = pDatabase;
		InstanceKeyPtr	pIK;


		assert(pKey);

		if (!pDB)
		{
			assert(pKey->IsKindOf(InstanceKey::ClassObject()));
			pDB = pKey->GetEntity()->GetDatabase();
		}

		assert(pDB);

		// Make sure we use the correct database in case the search key is from an object
		// blonging to another meta database than this
		//
		if (pDB->GetMetaDatabase() != m_pMetaEntity->GetMetaDatabase())
		{
			pDB = pDB->GetDatabaseWorkspace()->GetDatabase(m_pMetaEntity->GetMetaDatabase());
		}

		// To load a specific object through it's key, the key must be searchable and unique
		//
		if (!IsSearchable() || !IsUnique())
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaKey::LoadObject(): Attempt to load single object through non-unique key %s.", GetFullName().c_str());

		// Cached objects are always loaded into the Global database
		//
		if (m_pMetaEntity->IsCached())
			pDB = m_pMetaEntity->GetMetaDatabase()->GetGlobalDatabase();

		// Lookup the cache if no refresh is required
		//
		if (!bRefresh)
		{
			pIK = FindInstanceKey(pKey, pDB);

			if (pIK)
				return pIK->GetEntity();
		}

		// The object needs to be refreshed or wasn't found in the cache so get it
		//
		if (pDB->LoadObjects(this, pKey, bRefresh, bLazyFetch) == 1)
		{
			pIK = FindInstanceKey(pKey, pDB);

			if (pIK)
				return pIK->GetEntity();
		}

		return NULL;
	}




	// appends objects matching the current key to the listEntity
	void MetaKey::LoadObjects(KeyPtr pKey, EntityPtrList& listEntity, DatabasePtr pDatabase, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr			pDB = pDatabase;

		assert(pKey);

		if (!pDB)
		{
			assert(pKey->IsKindOf(InstanceKey::ClassObject()));
			pDB = pKey->GetEntity()->GetDatabase();
		}

		assert(pDB);

		// Make sure we use the correct database in case the search key is from an object
		// blonging to another meta database than this
		//
		if (pDB->GetMetaDatabase() != m_pMetaEntity->GetMetaDatabase())
		{
			pDB = pDB->GetDatabaseWorkspace()->GetDatabase(m_pMetaEntity->GetMetaDatabase());
		}

		// Cached objects are always loaded into the Global database
		//
		if (m_pMetaEntity->IsCached())
			pDB = m_pMetaEntity->GetMetaDatabase()->GetGlobalDatabase();

		if (pDB->LoadObjects(this, pKey, bRefresh, bLazyFetch) > 0)
		{
			InstanceKeyPtrSetPtr	pIKSet = GetInstanceKeySet(pDB);
			InstanceKeyPtrSetItr	itrIKSet;
			KeyPtr								pIK;

			if (!pIKSet)
				return;

			// Locate the key
			for ( itrIKSet =  pIKSet->find(pKey);
						itrIKSet != pIKSet->end();
						itrIKSet++
						)
			{
				pIK = *itrIKSet;

				if (pIK->Compare(pKey) != 0)
					break;

				listEntity.push_back(pIK->GetEntity());
			}
		}
	}




	// Return the instance key set for the database object passed in
	//
	InstanceKeyPtrSetPtr MetaKey::GetInstanceKeySet(DatabasePtr pDatabase)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxExclusive);

		InstanceKeyPtrSetPtrMapItr		itrKeySetMap;
		InstanceKeyPtrSetPtr					pKeySet;
		DatabasePtr										pDB = pDatabase;


		// Sanity checks
		//
		assert(pDatabase);

		// The meta database passed in can be different if this request originates
		// from a relation accross databases
		//
		if (pDatabase->GetMetaDatabase() != GetMetaEntity()->GetMetaDatabase())
		{
			pDB = pDatabase->GetDatabaseWorkspace()->GetDatabase(GetMetaEntity()->GetMetaDatabase());
			assert(pDB);
		}

		// Key's from cached entities only maintain one KeySet for the global database
		//
		if (m_pMetaEntity->IsCached())
			pDB = m_pMetaEntity->GetMetaDatabase()->GetGlobalDatabase();

		// Locate the correct recordset
		//
		itrKeySetMap = m_mapInstanceKeySet.find(pDB);

		if (itrKeySetMap == m_mapInstanceKeySet.end())
			return NULL;

		pKeySet = itrKeySetMap->second;

		assert(pKeySet);


		return pKeySet;
	}



	void MetaKey::CollectAllInstances(DatabasePtr pDatabase, EntityPtrList& el)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxExclusive);

		InstanceKeyPtrSetPtr		pKeySet = GetInstanceKeySet(pDatabase);
		InstanceKeyPtrSetItr		itrKey;
		KeyPtr									pKey;

		// empty list
		el.clear();

		if (pKeySet)
		{
			// Collect all entities
			for (	itrKey =  pKeySet->begin();
						itrKey !=  pKeySet->end();
						itrKey++)
			{
				pKey = *itrKey;
				el.push_back(pKey->GetEntity());
			}
		}
	}



	//! Debug aid: The method dumps this and all its objects to cout
	void MetaKey::Dump(int nIndentSize, bool bDeep)
	{
		MetaRelationPtr							pMetaRelation;
		MetaColumnPtrListItr				itrColumn;
		MetaColumnPtr								pColumn;
		unsigned int								idx;


		// Dump this
		//
		std::cout << std::setw(nIndentSize) << "";
		std::cout << "MetaKey " << m_strName << std::endl;
		std::cout << std::setw(nIndentSize) << "";
		std::cout << "{" << std::endl;

		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Name              " << m_strName << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "ProsaName         " << m_strProsaName << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Description       " << m_strDescription << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "InstanceClass     " << m_pInstanceClass->Name() << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "InstanceInterface " << m_strInstanceInterface << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Flags             " << m_Flags.AsString() << std::endl;

		if (bDeep)
		{
			std::cout << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "KeyColumns:" << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "{" << std::endl;

			// Dump key columns
			//
			for ( itrColumn =  m_listColumn.begin();
						itrColumn != m_listColumn.end();
						itrColumn++)
			{
				pColumn = *itrColumn;

				if (itrColumn != m_listColumn.begin())
					std::cout << std::endl;

				pColumn->Dump(nIndentSize+4, bDeep);
			}

			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "}" << std::endl;

			std::cout << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "Relations:" << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "{" << std::endl;


			// Dump related Parent keys
			//
			for (idx = 0; idx < m_vectMetaRelation.size(); idx++)
			{
				pMetaRelation = m_vectMetaRelation[idx];

				if (this == pMetaRelation->GetChildMetaKey())
				{
					std::cout << std::setw(nIndentSize+4) << "";
					std::cout << "Related Parent Key    " << pMetaRelation->GetParentMetaKey()->GetFullName() << std::endl;
				}
			}

			// Dump related Child keys
			//
			for (idx = 0; idx < m_vectMetaRelation.size(); idx++)
			{
				pMetaRelation = m_vectMetaRelation[idx];

				if (this == pMetaRelation->GetParentMetaKey())
				{
					std::cout << std::setw(nIndentSize+4) << "";
					std::cout << "Related Child Key     " << pMetaRelation->GetChildMetaKey()->GetFullName() << std::endl;
				}
			}

			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "}" << std::endl;
		}

		std::cout << std::setw(nIndentSize) << "";
		std::cout << "}" << std::endl;
	}



	void MetaKey::DeleteAllObjects(DatabasePtr pDatabase)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxExclusive);

		InstanceKeyPtrSetPtrMapItr	itrKeySet;
		InstanceKeyPtrSetPtr				pKeySet;
		InstanceKeyPtrSetItr				itrKey;


		// This message is only being permisible on primary keys
		//
		assert(IsPrimary());

		// Delete all primary keys
		//
		itrKeySet = m_mapInstanceKeySet.find(pDatabase);

		if (itrKeySet != m_mapInstanceKeySet.end())
		{
			pKeySet = itrKeySet->second;

			while (pKeySet && !pKeySet->empty())
			{
				itrKey = pKeySet->begin();
				delete (*itrKey)->GetEntity();
			}
		}
	}




	std::string MetaKey::AsCreateIndexSQL(TargetRDBMS eTarget)
	{
		std::string						strSQL;
		MetaColumnPtrListItr	itrColumn;


		switch (eTarget)
		{
			case SQLServer:
			{
				if			(IsPrimary())
				{
					// Do this for primary keys:
					//
					// CREATE UNIQUE CLUSTERED INDEX [keyname] ON [tblname]
					// (
					//   [KeyCol1],
					// 		[KeyCol1],
					// 		[KeyCol2],
					//		...
					// 		[KeyColn]
					// 	)  ON [PRIMARY]
					//
					strSQL  = "CREATE UNIQUE CLUSTERED INDEX [";

					strSQL += m_strName;
					strSQL += "] ON [";
					strSQL += m_pMetaEntity->GetName();
					strSQL += "] (";

					for (	itrColumn  = m_listColumn.begin();
								itrColumn != m_listColumn.end();
								itrColumn++)
					{
						if (itrColumn != m_listColumn.begin())
							strSQL += ", ";

						strSQL += "[";
						strSQL += (*itrColumn)->GetName();
						strSQL += "]";
					}

					strSQL += ") ON [PRIMARY]";
				}
				else if (IsUnique())
				{
					// Create this for unique secondary keys:
					//
					// ALTER TABLE [tblname] WITH NOCHECK ADD
					// 	CONSTRAINT [keyname] UNIQUE NONCLUSTERED
					// 	(
					// 		[KeyCol1],
					// 		[KeyCol2],
					//		...
					// 		[KeyColn]
					// 	)
					strSQL  = "ALTER TABLE [";
					strSQL += m_pMetaEntity->GetName();
					strSQL += "] WITH NOCHECK ADD CONSTRAINT [";
					strSQL += m_strName;
					strSQL += "] UNIQUE NONCLUSTERED ( ";

					for (	itrColumn  = m_listColumn.begin();
								itrColumn != m_listColumn.end();
								itrColumn++)
					{
						if (itrColumn != m_listColumn.begin())
							strSQL += ", ";

						strSQL += "[";
						strSQL += (*itrColumn)->GetName();
						strSQL += "]";
					}

					strSQL += " )";
				}
				else if (IsSearchable())
				{
					// Create this for unique secondary keys:
					//
					// CREATE INDEX [keyname] ON [tblname]
					// 	(
					// 		[KeyCol1],
					// 		[KeyCol2],
					//		...
					// 		[KeyColn]
					// 	)
					strSQL  = "CREATE INDEX [";
					strSQL += m_strName;
					strSQL += "] ON [";
					strSQL += m_pMetaEntity->GetName();
					strSQL += "] ( ";

					for (	itrColumn  = m_listColumn.begin();
								itrColumn != m_listColumn.end();
								itrColumn++)
					{
						if (itrColumn != m_listColumn.begin())
							strSQL += ", ";

						strSQL += "[";
						strSQL += (*itrColumn)->GetName();
						strSQL += "]";
					}

					strSQL += " )";
				}

				break;
			}

			case Oracle:
			{
				std::string		m_strNameShort(m_strName,0,30);


				if (IsPrimary())
				{
					bool									bHasOptionalColumns = false;
					MetaColumnPtrListItr	itrMC;
					MetaColumnPtr					pMC;


					// Has this key got optional columns?
					//
					for ( itrMC  = m_listColumn.begin();
								itrMC != m_listColumn.end();
								itrMC++)
					{
						pMC = *itrMC;

						if (!pMC->IsMandatory())
						{
							bHasOptionalColumns = true;
							break;
						}
					}

					// Create this for primary keys:
					//
					// ALTER TABLE [tblname] ADD
					// 	CONSTRAINT [keyname] { PRIMARY | UNIQUE } KEY
					// 	(
					// 		KeyCol1,
					// 		KeyCol2,
					//		...
					// 		KeyColn
					// 	)
					//  ENABLE NOVALIDATE
					//
					// We'll create a primary key constraint only if the key has
					// no optional columsn. Otherwise we'll create a UNIQUE key
					// copnstraint
					//
					strSQL  = "ALTER TABLE ";
					strSQL += m_pMetaEntity->GetName();
					strSQL += " ADD CONSTRAINT ";
					strSQL += m_strNameShort;

					if (bHasOptionalColumns)
						strSQL += " UNIQUE ( ";
					else
						strSQL += " PRIMARY KEY ( ";

					for (	itrColumn  = m_listColumn.begin();
								itrColumn != m_listColumn.end();
								itrColumn++)
					{
						if (itrColumn != m_listColumn.begin())
							strSQL += ", ";

						strSQL += "";
						strSQL += (*itrColumn)->GetName();
						strSQL += "";
					}

					strSQL += " )";
					strSQL += " ENABLE NOVALIDATE";
				}
				else if (IsUnique())
				{
					// Create this for unique secondary keys:
					//
					// ALTER TABLE [tblname] ADD
					// 	CONSTRAINT [keyname] UNIQUE
					// 	(
					// 		KeyCol1,
					// 		KeyCol2,
					//		...
					// 		KeyColn
					// 	)
					//  ENABLE NOVALIDATE

					strSQL  = "ALTER TABLE ";
					strSQL += m_pMetaEntity->GetName();
					strSQL += " ADD CONSTRAINT ";
					strSQL += m_strNameShort;
					strSQL += " UNIQUE ( ";

					for (	itrColumn  = m_listColumn.begin();
								itrColumn != m_listColumn.end();
								itrColumn++)
					{
						if (itrColumn != m_listColumn.begin())
							strSQL += ", ";

						strSQL += "";
						strSQL += (*itrColumn)->GetName();
						strSQL += "";
					}

					strSQL += " )";
					strSQL += " ENABLE NOVALIDATE";
				}
				else if (IsSearchable())
				{
					// Create this for unique secondary keys:
					//
					// CREATE INDEX [keyname] ON [tblname]
					// 	(
					// 		KeyCol1,
					// 		KeyCol2,
					//		...
					// 		KeyColn
					// 	)
					strSQL  = "CREATE INDEX ";
					strSQL += m_strNameShort;
					strSQL += " ON ";
					strSQL += m_pMetaEntity->GetName();
					strSQL += " ( ";

					for (	itrColumn  = m_listColumn.begin();
								itrColumn != m_listColumn.end();
								itrColumn++)
					{
						if (itrColumn != m_listColumn.begin())
							strSQL += ", ";

						strSQL += "";
						strSQL += (*itrColumn)->GetName();
						strSQL += "";
					}

					strSQL += " )";
				}

				break;
			}
		}

		return strSQL;
	}



	void MetaKey::AddMetaColumn(MetaColumnPtr	pMC)
	{
		assert(pMC);

		m_listColumn.push_back(pMC);
		pMC->On_AddedToMetaKey(this);
	}



	void MetaKey::On_InstanceCreated(InstanceKeyPtr pKey)
	{
		DatabasePtr									pDB;
		InstanceKeyPtrSetPtr				pKeySet;


		// Don't bother with non searchable keys
		//
		if (!IsSearchable())
			return;

		// Some sanity checks
		//
		assert(pKey);
		assert(pKey->GetEntity());
		pDB = pKey->GetEntity()->GetDatabase();
		assert(pDB);

		// Key's from cached entities only maintain one KeySet for the global database
		//
		if (m_pMetaEntity->IsCached())
			pDB = m_pMetaEntity->GetMetaDatabase()->GetGlobalDatabase();

		// Add the key (which will be a NULL key) to the instance key multiset
		//
		pKeySet = GetInstanceKeySet(pDB);
		assert(pKeySet);
		pKeySet->insert(pKey);
	}



	void MetaKey::On_InstanceDeleted(InstanceKeyPtr pKey)
	{
		InstanceKeyPtrSetPtr				pKeySet;
		InstanceKeyPtrSetItr				itrKeySet;
		DatabasePtr									pDB;
		InstanceKeyPtr							pExistingKey = NULL;


		// Don't bother with non searchable or NULL keys
		//
		if (!IsSearchable())
			return;

		// Some sanity checks
		//
		assert(pKey);
		assert(pKey->GetMetaKey() == this);
		assert(pKey->GetEntity());
		pDB = pKey->GetEntity()->GetDatabase();
		assert(pDB);

		// Find the key in our multiset
		//
		pKeySet = GetInstanceKeySet(pDB);
		assert(pKeySet);

		for (	itrKeySet =  pKeySet->find(pKey);
					itrKeySet != pKeySet->end();
					itrKeySet++)
		{
			pExistingKey = (InstanceKeyPtr) *itrKeySet;

			if (pKey == pExistingKey)
				break;

			if (*pKey != *pExistingKey)
			{
				itrKeySet = pKeySet->end();
				break;
			}
		}

		if (pExistingKey != pKey)
		{
#ifdef _DEBUG
			ReportError("MetaKey::On_InstanceDeleted(): Searching key %s (Value: %s) forcefully.", GetFullName().c_str(), pKey->AsString().c_str());

			for (	itrKeySet =  pKeySet->begin();
						itrKeySet != pKeySet->end();
						itrKeySet++)
			{
				pExistingKey = (InstanceKeyPtr) *itrKeySet;

				ReportError("MetaKey::On_InstanceDeleted(): Key value found (Value: %s)", pExistingKey->AsString().c_str());

				if (pExistingKey == pKey)
					break;
			}

			if (pExistingKey == pKey)
			{
				ReportError("MetaKey::On_InstanceDeleted(): Only forceful search of key %s (Value: %s) succeeded!", GetName().c_str(), pKey->AsString().c_str());
			}
			else
			{
				ReportError("MetaKey::On_InstanceDeleted(): Could not find key %s (Value: %s)", GetName().c_str(), pKey->AsString().c_str());
				return;
			}

#else
			ReportError("MetaKey::On_InstanceDeleted(): Could not find key %s (Value: %s)", GetName().c_str(), pKey->AsString().c_str());
			return;
#endif
		}

		pKeySet->erase(itrKeySet);
	}



	// This method removes the key that is about to change
	//
	void MetaKey::On_BeforeUpdateInstance(InstanceKeyPtr pKey)
	{
		InstanceKeyPtr							pExistingKey = NULL;
		InstanceKeyPtrSetPtr				pKeySet;
		InstanceKeyPtrSetItr				itrKeySet;
		DatabasePtr									pDB;


		// Don't bother with non searchable or NULL keys
		//
		if (!IsSearchable())
			return;

		// Some sanity checks
		//
		assert(pKey);
		assert(pKey->GetEntity());
		pDB = pKey->GetEntity()->GetDatabase();
		assert(pDB);

		// Key's from cached entities only maintain one KeySet for the global database
		//
		if (m_pMetaEntity->IsCached())
			pDB = m_pMetaEntity->GetMetaDatabase()->GetGlobalDatabase();

		// Locate the correct recordset
		//
		pKeySet = GetInstanceKeySet(pDB);
		assert(pKeySet);

		// Find the key using the original value
		//
		for ( itrKeySet =  pKeySet->find(pKey);
					itrKeySet != pKeySet->end();
					itrKeySet++)
		{
			pExistingKey = (InstanceKeyPtr) *itrKeySet;

			if (pExistingKey == pKey)
				break;

			if (*pExistingKey != *pKey)
				break;
		}

		if (pExistingKey != pKey)
		{
#ifdef _DEBUG
			// A forceful search
			//
			ReportError("MetaKey::On_BeforeUpdateInstance(): Searching key %s (Value: %s) forcefully.", GetFullName().c_str(), pKey->AsString().c_str());

			for ( itrKeySet =  pKeySet->begin();
						itrKeySet != pKeySet->end();
						itrKeySet++)
			{
				pExistingKey = (InstanceKeyPtr) *itrKeySet;

				ReportError("MetaKey::On_BeforeUpdateInstance(): Key value found (Value: %s)", pExistingKey->AsString().c_str());

				if (pExistingKey == pKey)
					break;
			}

			if (pExistingKey == pKey)
			{
				ReportError("MetaKey::On_BeforeUpdateInstance(): Key %s (Value: %s) only found through forceful search!", GetFullName().c_str(), pKey->AsString().c_str());
			}
			else
			{
				ReportError("MetaKey::On_BeforeUpdateInstance(): Key %s (Value: %s) not found!", GetFullName().c_str(), pKey->AsString().c_str());
				return;
			}
#else
			ReportError("MetaKey::On_BeforeUpdateInstance(): Key %s (Value: %s) not found!", GetFullName().c_str(), pKey->AsString().c_str());
			return;
#endif
		}

		// Let's remove the original key and reinsert the new key
		//
		pKeySet->erase(itrKeySet);
	}



	void MetaKey::On_AfterUpdateInstance(InstanceKeyPtr pKey)
	{
		InstanceKeyPtrSetPtr				pKeySet;
		DatabasePtr									pDB;


		// Some sanity checks
		//
		assert(pKey);
		assert(pKey->GetEntity());

		// Don't bother with non searchable keys
		//
		if (!IsSearchable())
			return;

		// Determine the keyset to which the instance must be added.
		// Key's from cached entities only maintain one KeySet for the global database
		//
		pDB = pKey->GetEntity()->GetDatabase();
		assert(pDB);

		if (m_pMetaEntity->IsCached())
			pDB = m_pMetaEntity->GetMetaDatabase()->GetGlobalDatabase();

		// Now get the correct Key multiset and insert the key if it is not already a member
		//
		pKeySet = GetInstanceKeySet(pDB);
		assert(pKeySet);

		if (IsUnique() && !pKey->IsNull())
		{
			// Let's see if we already have a key with the new value
			//
			if (pKeySet->find(pKey) != pKeySet->end())
			{
				// A key with the same value already exists so throw an error
				//
				throw Exception(__FILE__, __LINE__, Exception_error, "MetaKey::On_AfterUpdateInstance(): Can't insert duplicate key %s (Value: %s).", GetFullName().c_str(), pKey->AsString().c_str());
			}
		}

		pKeySet->insert(pKey);
	}



	void MetaKey::On_ParentMetaRelationCreated(MetaRelationPtr pRel)
	{
		assert(pRel->GetChildMetaKey() == this);

		m_vectMetaRelation.push_back(pRel);
	}



	void MetaKey::On_ParentMetaRelationDeleted(MetaRelationPtr pRel)
	{
		unsigned int		idx;

		assert(pRel->GetChildMetaKey() == this);

		idx = FindRelation(pRel);

		if (idx < m_vectMetaRelation.size())
			m_vectMetaRelation[idx] = NULL;
	}



	void MetaKey::On_ChildMetaRelationCreated(MetaRelationPtr pRel)
	{
		assert(pRel->GetParentMetaKey() == this);

		m_vectMetaRelation.push_back(pRel);
	}



	void MetaKey::On_ChildMetaRelationDeleted(MetaRelationPtr pRel)
	{
		unsigned int		idx;

		assert(pRel->GetParentMetaKey() == this);

		idx = FindRelation(pRel);

		if (idx < m_vectMetaRelation.size())
			m_vectMetaRelation[idx] = NULL;
	}



	unsigned int MetaKey::FindRelation(MetaRelationPtr pRel)
	{
		unsigned int		idx;

		for ( idx = 0; idx < m_vectMetaRelation.size(); idx++)
		{
			if (pRel == m_vectMetaRelation[idx])
				break;;
		}

		return idx;
	}



	bool MetaKey::IsOverlappingKey(MetaKeyPtr pMK)
	{
		if (!pMK || pMK == this || pMK->GetMetaEntity() != m_pMetaEntity)
			return false;

		MetaColumnPtrListItr		itrThisColumn, itrOtherColumn;

		for ( itrThisColumn =  GetMetaColumns()->begin();
					itrThisColumn != GetMetaColumns()->end();
					itrThisColumn++)
		{
			for ( itrOtherColumn =  pMK->GetMetaColumns()->begin();
						itrOtherColumn != pMK->GetMetaColumns()->end();
						itrOtherColumn++)
			{
				if (*itrThisColumn == *itrOtherColumn)
					return true;
			}
		}

		return false;
	}



	void MetaKey::AddOverlappingKey(MetaKeyPtr pMK)
	{
		m_listOverlappedKeys.push_back(pMK);
		ReportDiagnostic("MetaKey::AddOverlappingKey(): Key %s overlaps with key %s", GetFullName().c_str(), pMK->GetFullName().c_str());
	}



	void MetaKey::RemoveOverlappingKey(MetaKeyPtr pMK)
	{
		m_listOverlappedKeys.remove(pMK);
	}



	void MetaKey::On_DatabaseCreated(DatabasePtr pDatabase)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxExclusive);

		InstanceKeyPtrSetPtrMapItr						itrKeySetMap;
		InstanceKeyPtrSetPtr									pKeySet = NULL;
		DatabasePtr														pDB = pDatabase;


		// Some sanity checks
		//
		assert(pDB);

		// Don't add non searchable keys
		//
		if (!IsSearchable())
			return;

		// Key's from cached entities only maintain one KeySet for the global database
		//
		if (m_pMetaEntity->IsCached())
		{
			pDB = m_pMetaEntity->GetMetaDatabase()->GetGlobalDatabase();

			// We may already have this set
			//
			if ((itrKeySetMap = m_mapInstanceKeySet.find(pDB)) != m_mapInstanceKeySet.end())
				pKeySet = itrKeySetMap->second;
		}

		if (!pKeySet)
		{
			pKeySet = new InstanceKeyPtrSet();
			m_mapInstanceKeySet.insert(InstanceKeyPtrSetPtrMap::value_type(pDB, pKeySet));
		}
	}



	void MetaKey::On_DatabaseDeleted(DatabasePtr pDatabase)
	{
		boost::recursive_mutex::scoped_lock		lk(m_mtxExclusive);

		InstanceKeyPtrSetPtrMapItr						itrKeySetMap;
		InstanceKeyPtrSetPtr									pKeySet;


		// Some sanity checks
		//
		assert(pDatabase);

		// No work for non searchable keys
		//
		if (!IsSearchable())
			return;

		// Delete the multiset we maintained for the dying database
		//
		itrKeySetMap = m_mapInstanceKeySet.find(pDatabase);

		if (itrKeySetMap != m_mapInstanceKeySet.end())
		{
			pKeySet = itrKeySetMap->second;
			assert(pKeySet);
			assert (pKeySet->empty());
			delete pKeySet;
			m_mapInstanceKeySet.erase(itrKeySetMap);
		}
	}



	/* static */
	std::ostream & MetaKey::AllAsJSON(RoleUserPtr pRoleUser, std::ostream & ojson)
	{
		D3::RolePtr									pRole = pRoleUser ? pRoleUser->GetRole() : NULL;
		D3::MetaDatabasePtr					pMD;
		D3::MetaDatabasePtrMapItr		itrMD;
		D3::MetaEntityPtr						pME;
		D3::MetaEntityPtrVectItr		itrME;
		D3::MetaKeyPtr							pMK;
		D3::MetaKeyPtrVectItr				itrMK;
		bool												bFirst = true;

		ojson << "[\n\t";

		for ( itrMD =	 MetaDatabase::GetMetaDatabases().begin();
					itrMD != MetaDatabase::GetMetaDatabases().end();
					itrMD++)
		{
			pMD = itrMD->second;

			for ( itrME =  pMD->GetMetaEntities()->begin();
						itrME != pMD->GetMetaEntities()->end();
						itrME++)
			{
				pME = *itrME;

				for ( itrMK =  pME->GetMetaKeys()->begin();
							itrMK != pME->GetMetaKeys()->end();
							itrMK++)
				{
					pMK = *itrMK;

					if (pRole->CanRead(pMK))
					{
						bFirst ? bFirst = false : ojson << ',';
						ojson << "\n\t";
						pMK->AsJSON(pRoleUser, ojson, NULL, NULL, true);
					}
				}
			}
		}

		ojson << "\n]";


		return ojson;
	}



	std::ostream & MetaKey::AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, MetaEntityPtr pContext, bool * pFirstSibling, bool bShallow)
	{
		RolePtr			pRole = pRoleUser ? pRoleUser->GetRole() : NULL;

		if (!pRole || pRole->CanRead(this))
		{
			MetaColumnPtrListItr	itrColumn;
			MetaColumnPtr					pMC;
			bool									bFirst = true;


			if (pFirstSibling)
			{
				if (*pFirstSibling)
					*pFirstSibling = false;
				else
					ostrm << ',';
			}

			ostrm << "{";
			ostrm << "\"ID\":"									<< GetID() << ',';
			ostrm << "\"MetaEntityID\":"				<< GetMetaEntity()->GetID() << ',';
			ostrm << "\"Idx\":"									<< GetMetaKeyIndex() << ',';
			ostrm << "\"Name\":\""							<< JSONEncode(GetName()) << "\",";
			ostrm << "\"ProsaName\":\""					<< JSONEncode(GetProsaName()) << "\",";
/*
			std::string						strBase64Desc;
			APALUtil::base64_encode(strBase64Desc, (const unsigned char *) GetDescription().c_str(), GetDescription().size());
			ostrm << "\"Description\":\""				<< strBase64Desc << "\",";
*/
			ostrm << "\"InstanceClass\":\""			<< JSONEncode(GetInstanceClassName()) << "\",";
			ostrm << "\"InstanceInterface\":\""	<< JSONEncode(GetInstanceInterface()) << "\",";
			ostrm << "\"JSMetaClass\":\""				<< JSONEncode(GetJSMetaClassName()) << "\",";
			ostrm << "\"JSInstanceClass\":\""		<< JSONEncode(GetJSInstanceClassName()) << "\",";
			ostrm << "\"JSViewClass\":\""				<< JSONEncode(GetJSViewClassName()) << "\",";

			ostrm << "\"Mandatory\":"						<< (IsMandatory()							? "true" : "false") << ",";
			ostrm << "\"Unique\":"							<< (IsUnique()								? "true" : "false") << ",";
			ostrm << "\"Primary\":"							<< (IsPrimary()								? "true" : "false") << ",";
			ostrm << "\"Secondary\":"						<< (IsSecondary()							? "true" : "false") << ",";
			ostrm << "\"Foreign\":"							<< (IsForeign()								? "true" : "false") << ",";
			ostrm << "\"Conceptual\":"					<< (IsConceptual()						? "true" : "false") << ",";
			ostrm << "\"HiddenOnQuickAccess\":"	<< (IsHiddenOnQuickAccess()		? "true" : "false") << ",";
			ostrm << "\"DefaultQuickAccess\":"	<< (IsDefaultQuickAccess()		? "true" : "false") << ",";
			ostrm << "\"AutocompleteQuickAccess\":"	<< (IsAutocompleteQuickAccess()		? "true" : "false") << ",";

			ostrm << "\"HSTopics\":"						<< (m_strHSTopicsJSON.empty() ? "null" : m_strHSTopicsJSON) << ',';

			if (!bShallow && pContext != GetMetaEntity())
			{
				ostrm << "\"TargetMetaEntity\":";

				GetMetaEntity()->AsJSON(pRoleUser, ostrm, NULL, true);

				ostrm << ",\"Columns\":[";

				for ( itrColumn =  m_listColumn.begin();
							itrColumn != m_listColumn.end();
							itrColumn++)
				{
					pMC = *itrColumn;
					pMC->AsJSON(NULL, ostrm, &bFirst);
				}

				ostrm << "]";
			}
			else
			{
				ostrm << "\"ColumnIDs\":[";

				for ( itrColumn =  m_listColumn.begin();
							itrColumn != m_listColumn.end();
							itrColumn++)
				{
					pMC = *itrColumn;

					if (bFirst)
						bFirst = false;
					else
						ostrm << ",";

					ostrm << pMC->GetID();
				}

				ostrm << "]";
			}


			ostrm << "}";
		}

		return ostrm;
	}









	// ===========================================================
	// Key abstract class: This class provides a common base for InstanceKey and TemporaryKey
	//

	// Standard D3 Stuff
	//
	D3_CLASS_IMPL_PV(Key, Object);


	int Key::Compare(ObjectPtr pObj)
	{
		ColumnPtrListItr	itrSrceColumn;
		ColumnPtrListItr	itrTrgtColumn;
		ColumnPtr					pSrce;
		ColumnPtr					pTrgt;
		KeyPtr						pKey = (KeyPtr) pObj;


		assert(pObj);
		assert(pObj->IsKindOf(Key::ClassObject()));

		for ( itrSrceColumn =  m_listColumn.begin(),    itrTrgtColumn =  pKey->m_listColumn.begin();
					itrSrceColumn != m_listColumn.end()    && itrTrgtColumn != pKey->m_listColumn.end();
					itrSrceColumn++, itrTrgtColumn++)
		{
			pSrce = *itrSrceColumn;
			pTrgt = *itrTrgtColumn;

			if (pSrce->IsUndefined() || pTrgt->IsUndefined())
				break;

			if (pSrce->IsNull() && pTrgt->IsNull())
				continue;

			if (!pSrce->IsNull() && pTrgt->IsNull())
				return 1;

			if (pSrce->IsNull() && !pTrgt->IsNull())
				return -1;

			if (*pSrce > *pTrgt)
				return 1;

			if (*pSrce < *pTrgt)
				return -1;

			// So far equal, continue
		}

		return 0;
	}



	bool Key::IsNull()
	{
		ColumnPtrListItr	itrSrceColumn;
		ColumnPtr					pSrce;
		bool							bAllNull = true;


		// A key is NULL if it has no columns
		//
		if (m_listColumn.empty())
			return true;

		for ( itrSrceColumn =  m_listColumn.begin();
					itrSrceColumn != m_listColumn.end();
					itrSrceColumn++)
		{
			pSrce = *itrSrceColumn;


			// A key is NULL if a mandatory column is NULL
			//
			if (pSrce->IsNull())
			{
				if (pSrce->GetMetaColumn()->IsMandatory())
					return true;
			}
			else
			{
				// Remember that we have a column that is NOT null
				//
				bAllNull = false;
			}
		}


		return bAllNull;
	}



	bool Key::SetNull()
	{
		ColumnPtrListItr	itrSrceColumn;
		ColumnPtr					pSrce;


		for ( itrSrceColumn =  m_listColumn.begin();
					itrSrceColumn != m_listColumn.end();
					itrSrceColumn++)
		{
			pSrce = *itrSrceColumn;

			if (!pSrce->SetNull())
				return false;
		}


		return true;
	}



	// Check whether a key is NULL based on another key.
	// This method is useful to check related keys for nullness. Consider Entity A and
	// Entity B. A has a primary key PK and B has a foreign key FK. B.FK is a unique key
	// consisting of A.PK and say SequenceNo. If the A.PK part of B.FK (B.FK.APK) is not
	// NULL, we have a valid relation even if SequenceNo is NULL.
	// This means that technically B.FK is incomplete and therefore B.FK->IsNull() returs
	// true. If you used B.FK->IsKeyPartNull(A.PK->GetMetaKey()) it will return false,
	// indicating that the A.PK part of B.FK is not NULL.
	//
	bool Key::IsKeyPartNull(MetaKeyPtr pMK)
	{
		unsigned int					idx;
		ColumnPtrListItr			itrSrceColumn;
		ColumnPtr							pSrce;


		for ( itrSrceColumn =  m_listColumn.begin(), idx = 0;
					itrSrceColumn != m_listColumn.end() && idx < pMK->GetColumnCount();
					itrSrceColumn++,                       idx++)
		{
			pSrce = *itrSrceColumn;

			if (!pSrce->IsNull())
				return false;
		}

		return true;
	}



	Key & Key::Assign(Key & aKey)
	{
		ColumnPtrListItr	itrTrgtColumn;
		ColumnPtrListItr	itrSrceColumn;
		ColumnPtr					pTrgt;
		ColumnPtr					pSrce;


		// Get the column values from the source
		//
		for ( itrTrgtColumn =  m_listColumn.begin(),    itrSrceColumn =  aKey.m_listColumn.begin();
					itrTrgtColumn != m_listColumn.end()    && itrSrceColumn != aKey.m_listColumn.end();
					itrTrgtColumn++,													itrSrceColumn++)
		{
			pTrgt = *itrTrgtColumn;
			pSrce = *itrSrceColumn;
			pTrgt->Assign(*pSrce);
		}


		return *this;
	}



	ColumnPtr Key::GetColumn(MetaColumnPtr pMC)
	{
		ColumnPtrListItr	itrColumn;
		ColumnPtr					pColumn;


		// Set all our columns to NULL
		//
		for ( itrColumn =  m_listColumn.begin();
					itrColumn != m_listColumn.end();
					itrColumn++)
		{
			pColumn = *itrColumn;

			if (pColumn->GetMetaColumn() == pMC)
				return pColumn;
		}

		return NULL;
	}


	ColumnPtr Key::GetColumn(ColumnIndex idx)
	{
		return GetColumn(m_pMetaKey->GetMetaEntity()->GetMetaColumn(idx));
	}



	ColumnPtr Key::GetColumn(const std::string & strName)
	{
		return GetColumn(m_pMetaKey->GetMetaEntity()->GetMetaColumn(strName));
	}



	void Key::Dump(int nIndentSize)
	{
		ColumnPtrListItr	itrCol;
		ColumnPtr					pCol;
		unsigned int			iLabelWidth = 0;


		std::cout << std::setw(nIndentSize) << "";
		std::cout << m_pMetaKey->GetName() << std::endl;
		std::cout << std::setw(nIndentSize) << "";
		std::cout << "{" << std::endl;

		// Determine label width
		//
		for ( itrCol =  m_listColumn.begin();
					itrCol != m_listColumn.end();
					itrCol++)
		{
			pCol = *itrCol;

			if (pCol && pCol->GetMetaColumn()->GetName().size() > iLabelWidth)
				iLabelWidth = pCol->GetMetaColumn()->GetName().size();
		}

		// Dump Columns
		//
		std::cout << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Columns:" << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "{" << std::endl;

		for ( itrCol =  m_listColumn.begin();
					itrCol != m_listColumn.end();
					itrCol++)
		{
			pCol = *itrCol;
			pCol->Dump(nIndentSize+4, iLabelWidth);
		}

		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "}" << std::endl;

		std::cout << std::setw(nIndentSize) << "";
		std::cout << "}" << std::endl;

	}



	std::string Key::AsString()
	{
		std::string				strValue;
		ColumnPtrListItr	itrCol;
		ColumnPtr					pCol;


		for ( itrCol =  m_listColumn.begin();
					itrCol != m_listColumn.end();
					itrCol++)
		{
			pCol = *itrCol;

			strValue += "[";
			strValue += pCol->AsString();
			strValue += "]";
		}


		return strValue;
	}



	std::ostream & Key::AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, bool * pFirstSibling)
	{
		RolePtr	pRole = pRoleUser ? pRoleUser->GetRole() : NULL;

		if (!pRole || pRole->CanRead(m_pMetaKey))
		{
			ColumnPtrListItr	itr;
			ColumnPtr					pCol;
			bool							bFirst = true;


			if (pFirstSibling)
			{
				if (*pFirstSibling)
					*pFirstSibling = false;
				else
					ostrm << ',';
			}

			ostrm << "[";

			if (pRole && GetMetaKey()->IsConceptual())
			{
				ostrm << '"';
				ostrm << JSONEncode(AsString());
				ostrm << '"';
			}
			else
			{
				for ( itr  = m_listColumn.begin();
							itr != m_listColumn.end();
							itr++)
				{
					pCol = *itr;
					pCol->AsJSON(NULL, ostrm, &bFirst);
				}
			}

			ostrm << "]";
		}

		return ostrm;
	}





	std::string Key::AsJSON()
	{
		std::ostringstream	ostrm;
		AsJSON(NULL, ostrm);
		return ostrm.str();
	}





	std::string Key::AsJSONKey()
	{
		std::ostringstream	ostrm;

		ostrm << "{\"KeyID\":" << m_pMetaKey->GetID() << ",\"Columns\":";
		AsJSON(NULL, ostrm);
		ostrm << '}';

		return ostrm.str();
	}







	// ===========================================================
	// InstanceKey class: Instances of this class are associated with an entity object
	//

	// Standard D3 Stuff
	//
	D3_CLASS_IMPL(InstanceKey, Key);


	InstanceKey::~InstanceKey()
	{
		ColumnPtrListItr		itrColumn;
		ColumnPtr						pCol;


		for (	itrColumn =  m_listColumn.begin();
					itrColumn != m_listColumn.end();
					itrColumn++)
		{
			pCol = *itrColumn;
			pCol->On_RemovedFromKey(this);
		}

		m_pMetaKey->On_InstanceDeleted(this);
		m_pEntity->On_InstanceKeyDeleted(this);

		delete m_pOriginalKey;
		m_pOriginalKey = NULL;
	}



	KeyPtr InstanceKey::GetOriginalKey()
	{
		if (m_pOriginalKey)
			return m_pOriginalKey;

		return this;
	}



	void InstanceKey::On_PostCreate()
	{
		MetaColumnPtrListItr		itrMC;
		MetaColumnPtr						pMC;
		ColumnPtr								pCol;


		// Some basic sanity checks
		//
		assert(m_pMetaKey);
		assert(m_pEntity);

		// Add the key columns from the entity
		//
		for (	itrMC =  m_pMetaKey->GetMetaColumns()->begin();
					itrMC != m_pMetaKey->GetMetaColumns()->end();
					itrMC++)
		{
			pMC = *itrMC;
			pCol = m_pEntity->GetColumn(pMC);

			assert(pCol->IsKeyMember());

			pCol->On_AddedToKey(this);
			m_listColumn.push_back(pCol);
		}

		m_pEntity->On_InstanceKeyCreated(this);
		m_pMetaKey->On_InstanceCreated(this);
	}


	void InstanceKey::On_ChildRelationCreated(RelationPtr pRel)
	{
	}



	void InstanceKey::On_ChildRelationDeleted(RelationPtr pRel)
	{
	}



	void InstanceKey::On_ParentRelationCreated(RelationPtr pRel)
	{
	}



	void InstanceKey::On_ParentRelationDeleted(RelationPtr pRel)
	{
	}



	bool InstanceKey::On_BeforeUpdateColumn(ColumnPtr pCol)
	{
		if (!m_bUpdating)
		{
			assert (!m_pOriginalKey);
			m_pOriginalKey = new TemporaryKey(*this);

			m_pMetaKey->On_BeforeUpdateInstance(this);
			return NotifyRelationsBeforeUpdate();
		}

		return true;
	}



	bool InstanceKey::On_AfterUpdateColumn(ColumnPtr pCol)
	{
		bool			bSuccess = true;


		if (!m_bUpdating)
		{
			m_pMetaKey->On_AfterUpdateInstance(this);
			bSuccess = NotifyRelationsAfterUpdate();
			delete m_pOriginalKey;
			m_pOriginalKey = NULL;
		}

		return bSuccess;
	}


	bool InstanceKey::On_BeforeUpdate()
	{
		InstanceKeyPtr			pOverlappingKey;
		MetaKeyPtrListItr		itrMK;
		MetaKeyPtr					pMK;


		m_bUpdating = true;

		if (!m_pEntity->IsPopulating())
		{
			if (!m_pOriginalKey)
				m_pOriginalKey = new TemporaryKey(*this);
			else
				*m_pOriginalKey = *this;
		}

		m_pMetaKey->On_BeforeUpdateInstance(this);

		if (!NotifyRelationsBeforeUpdate())
			return false;

		if (!m_pEntity->IsPopulating())
		{
			// Do the same for overlapping keys
			//
			for ( itrMK =  m_pMetaKey->m_listOverlappedKeys.begin();
						itrMK != m_pMetaKey->m_listOverlappedKeys.end();
						itrMK++)
			{
				pMK = *itrMK;

				pOverlappingKey = m_pEntity->GetInstanceKey(pMK);

				pOverlappingKey->m_bUpdating = true;

				if (!pOverlappingKey->m_pEntity->IsPopulating())
				{
					if (!pOverlappingKey->m_pOriginalKey)
						pOverlappingKey->m_pOriginalKey = new TemporaryKey(*pOverlappingKey);
				}

				pOverlappingKey->m_pMetaKey->On_BeforeUpdateInstance(pOverlappingKey);

				if (!pOverlappingKey->NotifyRelationsBeforeUpdate())
					return false;
			}
		}

		return true;
	}



	bool InstanceKey::On_AfterUpdate()
	{
		InstanceKeyPtr			pOverlappingKey;
		MetaKeyPtrListItr		itrMK;
		MetaKeyPtr					pMK;
		bool								bSuccess;


		m_pMetaKey->On_AfterUpdateInstance(this);
		bSuccess = NotifyRelationsAfterUpdate();
		m_bUpdating = false;
		delete m_pOriginalKey;
		m_pOriginalKey = NULL;

		if (!bSuccess)
			return false;

		if (!m_pEntity->IsPopulating())
		{
			// Do the same for overlapping keys
			//
			for ( itrMK =  m_pMetaKey->m_listOverlappedKeys.begin();
						itrMK != m_pMetaKey->m_listOverlappedKeys.end();
						itrMK++)
			{
				pMK = *itrMK;

				pOverlappingKey = m_pEntity->GetInstanceKey(pMK);

				pOverlappingKey->m_pMetaKey->On_AfterUpdateInstance(pOverlappingKey);
				bSuccess = pOverlappingKey->NotifyRelationsAfterUpdate();
				pOverlappingKey->m_bUpdating = false;
				delete pOverlappingKey->m_pOriginalKey;
				pOverlappingKey->m_pOriginalKey = NULL;

				if (!bSuccess)
					return false;
			}
		}

		return true;
	}



	void InstanceKey::On_AfterUpdateEntity()
	{
		delete m_pOriginalKey;
		m_pOriginalKey = NULL;
	}



	// Method is called after changes to the key where made
	//
	bool InstanceKey::NotifyRelationsBeforeUpdate()
	{
		unsigned int				idx;
		RelationPtrMapPtr		pRelationMap;
		RelationPtrMapItr		itrRelationMap;
		RelationPtr					pRelation;
		RelationPtrMap			mapTempRelation;


		// Deal with relations where this' entity is the child
		//
		for (idx = 0; idx < m_pEntity->GetMetaEntity()->GetParentMetaRelationCount(); idx++)
		{
			// Only bother if this' meta key is the same as the child meta key of the relation
			//
			if (m_pEntity->GetMetaEntity()->GetParentMetaRelation(idx)->GetChildMetaKey() != m_pMetaKey)
				continue;

			// Take a copy of the existing parent relation map (the purpose of the map is to
			// enable multiple parent relations if this' entity is cached but the parent is not)
			//
			pRelationMap = m_pEntity->m_vectParentRelation[idx];

			mapTempRelation.clear();
			mapTempRelation = *pRelationMap;

			// Now use the copy of the collection to send each relation the On_BeforeUpdateChildKey()
			// notification. The copy of the map is essential as the original map can change during
			// this operation
			//
			for ( itrRelationMap =  mapTempRelation.begin();
						itrRelationMap != mapTempRelation.end();
						itrRelationMap++)
			{
				pRelation = itrRelationMap->second;

				if (pRelation)
					pRelation->On_BeforeUpdateChildKey(this);
			}
		}

		// Deal with relations where this' entity is the parent
		//
		for (idx = 0; idx < m_pEntity->GetMetaEntity()->GetChildMetaRelationCount(); idx++)
		{
			// Only bother if this' meta key is the same as the parent meta key of the relation
			//
			if (m_pEntity->GetMetaEntity()->GetChildMetaRelation(idx)->GetParentMetaKey() != m_pMetaKey)
				continue;

			// Get the appropriate relation map (the purpose of the map is to enable multiple
			// relations if this' entity is cached but the child entity is not)
			//
			pRelationMap = m_pEntity->m_vectChildRelation[idx];

			// Now send each relation the On_BeforeUpdateParentKey() notification.
			//
			for ( itrRelationMap =  pRelationMap->begin();
						itrRelationMap != pRelationMap->end();
						itrRelationMap++)
			{
				pRelation = itrRelationMap->second;

				if (pRelation)
					pRelation->On_BeforeUpdateParentKey(this);
			}
		}


		return true;
	}



	// Method is called after changes to the key where made
	//
	bool InstanceKey::NotifyRelationsAfterUpdate()
	{
		unsigned int				idx;
		RelationPtrMapPtr		pRelationMap;
		RelationPtrMapItr		itrRelationMap;
		MetaRelationPtr			pMetaRelation;
		RelationPtr					pRelation;


		// Deal with relations where this' entity is the child
		//
		for (idx = 0; idx < m_pEntity->GetMetaEntity()->GetParentMetaRelationCount(); idx++)
		{
			// Only bother if this' meta key is the same as the child meta key of the relation
			//
			pMetaRelation = m_pEntity->GetMetaEntity()->GetParentMetaRelation(idx);

			if (pMetaRelation->GetChildMetaKey() != m_pMetaKey)
				continue;

			// Get the appropriate relation map (the purpose of the map is to enable multiple
			// relations if this' entity is cached but the parent entity is not)
			//
			pRelationMap = m_pEntity->m_vectParentRelation[idx];

			// Now send each relation the On_AfterUpdateChildKey() notification.
			//
			for ( itrRelationMap =  pRelationMap->begin();
						itrRelationMap != pRelationMap->end();
						itrRelationMap++)
			{
				pRelation = itrRelationMap->second;

				if (pRelation)
					pRelation->On_AfterUpdateChildKey(this);
			}

			// Ensure parents are resolved
			//
			if (pRelationMap->find(m_pEntity->GetDatabase()) == pRelationMap->end())
			{
				if (!IsKeyPartNull(pMetaRelation->GetParentMetaKey()))
				{
					EntityPtr					pParentEntity;


					pParentEntity = pMetaRelation->GetParentMetaKey()->LoadObject(this);

					if (!pParentEntity)
					{
						if (m_pEntity->IsPopulating())
						{
							// If the parent is from a different database, we must allow for referential
							// integrity errors
							//
							if (pMetaRelation->GetParentMetaKey()->GetMetaEntity()->GetMetaDatabase() !=
									pMetaRelation->GetChildMetaKey()->GetMetaEntity()->GetMetaDatabase())
							{
								if (m_pMetaKey->IsMandatory())
									ReportWarning("InstanceKey::UpdateDependentRelations(): Integrity warning, failed to resolve cross database relation between foreign key %s %s and unique key %s.", m_pMetaKey->GetFullName().c_str(), AsString().c_str(), pMetaRelation->GetParentMetaKey()->GetFullName().c_str());
							}
							else
							{
								// A fairly disasterous situation
								//
								throw Exception(__FILE__, __LINE__, Exception_error, "InstanceKey::UpdateDependentRelations(): Failed to resolve relation between foreign key %s %s and unique key %s.", m_pMetaKey->GetFullName().c_str(), AsString().c_str(), pMetaRelation->GetParentMetaKey()->GetFullName().c_str());
							}
						}
						else
						{
							if (pMetaRelation->GetParentMetaKey()->GetColumnCount() == 1)
							{
								// A fairly disasterous situation
								//
								throw Exception(__FILE__, __LINE__, Exception_error, "InstanceKey::UpdateDependentRelations(): Failed to resolve relation between foreign key %s %s and unique key %s.", m_pMetaKey->GetFullName().c_str(), AsString().c_str(), pMetaRelation->GetParentMetaKey()->GetFullName().c_str());
							}
							else
							{
								if (m_pMetaKey->IsMandatory())
								{
									// This could be caused by manually changing compound keys so just produce a warning
									// (if the key remains as is, the update/insert will fail and the error will be caught and reported)
									ReportWarning("InstanceKey::UpdateDependentRelations(): Integrity warning, failed to resolve relation between foreign key %s %s and unique key %s.", m_pMetaKey->GetFullName().c_str(), AsString().c_str(), pMetaRelation->GetParentMetaKey()->GetFullName().c_str());
								}
							}
						}
					}
					else
					{
						pRelation = pParentEntity->GetChildRelation(pMetaRelation, m_pEntity->GetDatabase());
						pRelation->AddChild(m_pEntity);
					}
				}
			}

		}

		for (idx = 0; idx < m_pEntity->GetMetaEntity()->GetChildMetaRelationCount(); idx++)
		{
			pMetaRelation = m_pEntity->GetMetaEntity()->GetChildMetaRelation(idx);

			if (pMetaRelation->GetParentMetaKey() != m_pMetaKey)
				continue;

			pRelationMap = m_pEntity->m_vectChildRelation[idx];

			for ( itrRelationMap =  pRelationMap->begin();
						itrRelationMap != pRelationMap->end();
						itrRelationMap++)
			{
				pRelation = itrRelationMap->second;

				if (pRelation)
					pRelation->On_AfterUpdateParentKey(this);
			}
		}


		return true;
	}







	// ===========================================================
	// TemporaryKey class implementation
	//

	// Standard D3 Stuff
	//
	D3_CLASS_IMPL(TemporaryKey, Key);


	TemporaryKey::TemporaryKey(Key & aKey)
	 : Key(aKey.GetMetaKey())
	{
		CreateColumns();
		Assign(aKey);
	}



	TemporaryKey::TemporaryKey(MetaKey & aMetaKey)
	 : Key(&aMetaKey)
	{
		CreateColumns();
	}



	TemporaryKey::TemporaryKey(MetaKey & aMetaKey, Key & aKey)
	 : Key(&aMetaKey)
	{
		CreateColumns();
		Key::Assign(aKey);
	}



	TemporaryKey::TemporaryKey(Json::Value & jsonID)
	{
		InitializeFromJSON(jsonID);
	}



	TemporaryKey::~TemporaryKey()
	{
		DeleteColumns();
	}

/*
	Key & TemporaryKey::Assign(Key & aKey)
	{
		assert(m_pMetaKey);
		assert(aKey.GetMetaKey());

		// If this has a different MetaKey than aKey, the collection of columns is
		// destroyed and entirely rebuilt before the values from aKey are copied.
		//
		if (m_pMetaKey != aKey.GetMetaKey())
		{
			DeleteColumns();
			m_pMetaKey = aKey.GetMetaKey();
			CreateColumns();
		}

		Key::Assign(aKey);

		return *this;
	}
*/



	EntityPtr TemporaryKey::GetEntity(DatabasePtr pDB)
	{
		if (!pDB)
			ReportWarning("TemporaryKey::GetEntity(): Missing database pointer.");

		InstanceKeyPtr pIK = m_pMetaKey->FindInstanceKey(this, pDB);

		if (pIK)
			return pIK->GetEntity();

		return NULL;
	}



	void TemporaryKey::CreateColumns()
	{
		MetaColumnPtrListItr	itrSrceColumn;
		MetaColumnPtr					pSrce;
		ColumnPtr							pTrgt;


		// Create a copy of each of the columns of the input key
		// and add each to this' list of columns
		//
		for ( itrSrceColumn =  m_pMetaKey->GetMetaColumns()->begin();
					itrSrceColumn != m_pMetaKey->GetMetaColumns()->end();
					itrSrceColumn++)
		{
			pSrce = *itrSrceColumn;
			pTrgt = pSrce->CreateInstance();
			pTrgt->MarkUndefined();
			m_listColumn.push_back(pTrgt);
		}
	}


	void TemporaryKey::DeleteColumns()
	{
		ColumnPtr					pCol;

		while (!m_listColumn.empty())
		{
			pCol = m_listColumn.front();
			m_listColumn.pop_front();
			delete pCol;
		}
	}



	// This method expects a well formed Json KeyID structure as input
	//
	// Here is a well formed JSON example:
	//
	// {"KeyID":123,"Columns":[1,'abc',false]}
	//
	// The first element in the Key array is the systemwide unique ID of the MetaKey and the
	// second element contains the array that holds the actual values for the key.
	// The following requirements must be met for this method to succeed:
	//
	// 1. The MetaKey with the ID specified must exists
	// 2. The Number of MetaColumns in the MetaKey found must equal Key[1].size()
	//
	void TemporaryKey::InitializeFromJSON(const std::string & strJSONObjectID)
	{
		Json::Reader						jsonReader;
		Json::Value							nRoot;


		if (!jsonReader.parse(strJSONObjectID, nRoot))
			throw Exception(__FILE__, __LINE__, Exception_error,
											"Parse error. Input:\n%s\n\nParser message:\n%s",
											strJSONObjectID.c_str(),
											jsonReader.getFormatedErrorMessages().c_str());

		InitializeFromJSON(nRoot);
	}



	// This method expects a Json value that is of an array type and has exactly
	// two elements.
	//
	// The first element in the Key array is the systemwide unique ID of the MetaKey and the
	// second element contains the array that holds the actual values for the key.
	// The following requirements must be met for this method to succeed:
	//
	// 1. The MeatKey with the ID specified must exists
	// 2. The Number of MetaColumns in the MetaKey found must equal Key[1].size()
	//
	void TemporaryKey::InitializeFromJSON(Json::Value & jsonID)
	{
		Json::Value&						nMetaKeyID	= jsonID["KeyID"];
		Json::Value&						nKeyColumns	= jsonID["Columns"];
		ColumnPtr								pCol;
		ColumnPtrListItr				itrKeyColumn;
		int											idx;


		// Get the MetaKey
		switch (nMetaKeyID.type())
		{
			case Json::nullValue:
				throw std::string("KeyID not found in Json key ID structure");

			case Json::intValue:
			case Json::uintValue:
				m_pMetaKey = MetaKey::GetMetaKeyByID(nMetaKeyID.asInt());

				if (!m_pMetaKey)
					throw std::string("KeyID does not identify a known MetaKey");

				break;

			default:
				throw std::string("KeyID is not a valid value");
		}


		switch (nKeyColumns.type())
		{
			case Json::arrayValue:
				// This key references an existing entity and the Columns member
				// is an array of values for key columns
				CreateColumns();

				if (GetColumnCount() == nKeyColumns.size())
				{
					for (	itrKeyColumn =  GetColumns().begin(), idx=0;
								itrKeyColumn != GetColumns().end();
								itrKeyColumn++, idx++)
					{
						Json::Value&	nColumn = nKeyColumns[idx];
						pCol = *itrKeyColumn;

						switch (nColumn.type())
						{
							case Json::intValue:
								pCol->SetValue(nColumn.asInt());
								break;
							case Json::uintValue:
								pCol->SetValue((int) nColumn.asUInt());
								break;
							case Json::realValue:
								{
								float f = (float) nColumn.asDouble();
								pCol->SetValue(f);
								}
								break;
							case Json::stringValue:
								pCol->SetValue(nColumn.asString());
								break;
							case Json::booleanValue:
								pCol->SetValue(nColumn.asBool());
								break;
							case Json::nullValue:
								pCol->SetNull();
								break;
							default:
								throw std::string("Bad JSON Key.");
						}
					}
				}
				else
				{
					throw std::string("Columns array has incorrect number of column values.");
				}

				break;

			default:
				throw std::string("Columns is not an array of values");
		}

	}


} // end namespace D3
