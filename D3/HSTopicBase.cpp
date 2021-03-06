// MODULE: HSTopicBase.cpp
//;
//; IMPLEMENTATION CLASS: HSTopicBase
//;

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can regenerate this file.
//

#include "D3Types.h"

#include "HSTopicBase.h"

// Dependent includes
//
#include "HSMetaColumnTopicBase.h"
#include "HSMetaDatabaseTopicBase.h"
#include "HSMetaEntityTopicBase.h"
#include "HSMetaKeyTopicBase.h"
#include "HSMetaRelationTopicBase.h"
#include "HSResourceUsageBase.h"
#include "HSTopicAssociationBase.h"
#include "HSTopicLinkBase.h"


namespace D3
{

	// =====================================================================================
	// Class HSTopicBase Implementation
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(HSTopicBase, Entity);


	// Get a collection reflecting all currently resident instances of this
	//
	/* static */
	InstanceKeyPtrSetPtr HSTopicBase::GetAll(DatabasePtr pDB)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3HSDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3HSDB"));

		if (!pDatabase)
			return NULL;

		return pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetPrimaryMetaKey()->GetInstanceKeySet(pDatabase);
	}



	// Make all objects of this type memory resident
	//
	/* static */
	void HSTopicBase::LoadAll(DatabasePtr pDB, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3HSDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3HSDB"));

		if (!pDatabase)
			return;

		pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->LoadAll(pDatabase, bRefresh, bLazyFetch);
	}



	// Make a specific instance resident
	//
	/* static */
	HSTopicPtr HSTopicBase::Load(DatabasePtr pDB, long lID, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3HSDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3HSDB"));

		if (!pDatabase)
			return NULL;

		TemporaryKey	tmpKey(*(pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetPrimaryMetaKey()));

		// Set all key column values
		//
		tmpKey.GetColumn(D3HSDB_HSTopic_ID)->SetValue(lID);

		return (HSTopicPtr) pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetPrimaryMetaKey()->LoadObject(&tmpKey, pDatabase, bRefresh, bLazyFetch);
	}



	// Load all instances of relation ChildTopics
	//
	void HSTopicBase::LoadAllChildTopics(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_ChildTopics));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation ChildTopics
	//
	HSTopicBase::ChildTopics* HSTopicBase::GetChildTopics()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_ChildTopics));

		return (HSTopicBase::ChildTopics*) pRelation;
	}



	// Load all instances of relation ParentTopics
	//
	void HSTopicBase::LoadAllParentTopics(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_ParentTopics));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation ParentTopics
	//
	HSTopicBase::ParentTopics* HSTopicBase::GetParentTopics()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_ParentTopics));

		return (HSTopicBase::ParentTopics*) pRelation;
	}



	// Load all instances of relation LinksToTopics
	//
	void HSTopicBase::LoadAllLinksToTopics(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_LinksToTopics));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation LinksToTopics
	//
	HSTopicBase::LinksToTopics* HSTopicBase::GetLinksToTopics()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_LinksToTopics));

		return (HSTopicBase::LinksToTopics*) pRelation;
	}



	// Load all instances of relation LinkedFromTopics
	//
	void HSTopicBase::LoadAllLinkedFromTopics(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_LinkedFromTopics));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation LinkedFromTopics
	//
	HSTopicBase::LinkedFromTopics* HSTopicBase::GetLinkedFromTopics()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_LinkedFromTopics));

		return (HSTopicBase::LinkedFromTopics*) pRelation;
	}



	// Load all instances of relation ResourceUsages
	//
	void HSTopicBase::LoadAllResourceUsages(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_ResourceUsages));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation ResourceUsages
	//
	HSTopicBase::ResourceUsages* HSTopicBase::GetResourceUsages()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_ResourceUsages));

		return (HSTopicBase::ResourceUsages*) pRelation;
	}



	// Load all instances of relation MetaDatabases
	//
	void HSTopicBase::LoadAllMetaDatabases(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_MetaDatabases));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation MetaDatabases
	//
	HSTopicBase::MetaDatabases* HSTopicBase::GetMetaDatabases()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_MetaDatabases));

		return (HSTopicBase::MetaDatabases*) pRelation;
	}



	// Load all instances of relation MetaEntities
	//
	void HSTopicBase::LoadAllMetaEntities(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_MetaEntities));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation MetaEntities
	//
	HSTopicBase::MetaEntities* HSTopicBase::GetMetaEntities()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_MetaEntities));

		return (HSTopicBase::MetaEntities*) pRelation;
	}



	// Load all instances of relation MetaColumns
	//
	void HSTopicBase::LoadAllMetaColumns(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_MetaColumns));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation MetaColumns
	//
	HSTopicBase::MetaColumns* HSTopicBase::GetMetaColumns()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_MetaColumns));

		return (HSTopicBase::MetaColumns*) pRelation;
	}



	// Load all instances of relation MetaKeys
	//
	void HSTopicBase::LoadAllMetaKeys(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_MetaKeys));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation MetaKeys
	//
	HSTopicBase::MetaKeys* HSTopicBase::GetMetaKeys()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_MetaKeys));

		return (HSTopicBase::MetaKeys*) pRelation;
	}



	// Load all instances of relation MetaRelations
	//
	void HSTopicBase::LoadAllMetaRelations(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_MetaRelations));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation MetaRelations
	//
	HSTopicBase::MetaRelations* HSTopicBase::GetMetaRelations()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopic)->GetChildMetaRelation(D3HSDB_HSTopic_CR_MetaRelations));

		return (HSTopicBase::MetaRelations*) pRelation;
	}





	// =====================================================================================
	// Class HSTopicBase::iterator Implementation
	//

	// De-reference operator*()
	//
	HSTopicPtr HSTopicBase::iterator::operator*()
	{
		InstanceKeyPtr pKey;
		EntityPtr      pEntity;

		pKey = (InstanceKeyPtr) ((InstanceKeyPtrSetItr*) this)->operator*();
		pEntity = pKey->GetEntity();

		return (HSTopicPtr) pEntity;
	}



	// Assignment operator=()
	//
	HSTopicBase::iterator& HSTopicBase::iterator::operator=(const iterator& itr)
	{
		((InstanceKeyPtrSetItr*) this)->operator=(itr);

		return *this;
	}





	// =====================================================================================
	// Class HSTopicBase::ChildTopics
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(HSTopicBase::ChildTopics, Relation);

	// De-reference operator*()
	//
	HSTopicAssociationPtr HSTopicBase::ChildTopics::iterator::operator*()
	{
		return (HSTopicAssociationPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	HSTopicBase::ChildTopics::iterator& HSTopicBase::ChildTopics::iterator::operator=(const iterator& itr)
	{
		return (HSTopicBase::ChildTopics::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	HSTopicAssociationPtr HSTopicBase::ChildTopics::front()
	{
		return (HSTopicAssociationPtr) Relation::front();
	}



	// back() method
	//
	HSTopicAssociationPtr HSTopicBase::ChildTopics::back()
	{
		return (HSTopicAssociationPtr) Relation::back();
	}





	// =====================================================================================
	// Class HSTopicBase::ParentTopics
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(HSTopicBase::ParentTopics, Relation);

	// De-reference operator*()
	//
	HSTopicAssociationPtr HSTopicBase::ParentTopics::iterator::operator*()
	{
		return (HSTopicAssociationPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	HSTopicBase::ParentTopics::iterator& HSTopicBase::ParentTopics::iterator::operator=(const iterator& itr)
	{
		return (HSTopicBase::ParentTopics::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	HSTopicAssociationPtr HSTopicBase::ParentTopics::front()
	{
		return (HSTopicAssociationPtr) Relation::front();
	}



	// back() method
	//
	HSTopicAssociationPtr HSTopicBase::ParentTopics::back()
	{
		return (HSTopicAssociationPtr) Relation::back();
	}





	// =====================================================================================
	// Class HSTopicBase::LinksToTopics
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(HSTopicBase::LinksToTopics, Relation);

	// De-reference operator*()
	//
	HSTopicLinkPtr HSTopicBase::LinksToTopics::iterator::operator*()
	{
		return (HSTopicLinkPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	HSTopicBase::LinksToTopics::iterator& HSTopicBase::LinksToTopics::iterator::operator=(const iterator& itr)
	{
		return (HSTopicBase::LinksToTopics::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	HSTopicLinkPtr HSTopicBase::LinksToTopics::front()
	{
		return (HSTopicLinkPtr) Relation::front();
	}



	// back() method
	//
	HSTopicLinkPtr HSTopicBase::LinksToTopics::back()
	{
		return (HSTopicLinkPtr) Relation::back();
	}





	// =====================================================================================
	// Class HSTopicBase::LinkedFromTopics
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(HSTopicBase::LinkedFromTopics, Relation);

	// De-reference operator*()
	//
	HSTopicLinkPtr HSTopicBase::LinkedFromTopics::iterator::operator*()
	{
		return (HSTopicLinkPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	HSTopicBase::LinkedFromTopics::iterator& HSTopicBase::LinkedFromTopics::iterator::operator=(const iterator& itr)
	{
		return (HSTopicBase::LinkedFromTopics::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	HSTopicLinkPtr HSTopicBase::LinkedFromTopics::front()
	{
		return (HSTopicLinkPtr) Relation::front();
	}



	// back() method
	//
	HSTopicLinkPtr HSTopicBase::LinkedFromTopics::back()
	{
		return (HSTopicLinkPtr) Relation::back();
	}





	// =====================================================================================
	// Class HSTopicBase::ResourceUsages
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(HSTopicBase::ResourceUsages, Relation);

	// De-reference operator*()
	//
	HSResourceUsagePtr HSTopicBase::ResourceUsages::iterator::operator*()
	{
		return (HSResourceUsagePtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	HSTopicBase::ResourceUsages::iterator& HSTopicBase::ResourceUsages::iterator::operator=(const iterator& itr)
	{
		return (HSTopicBase::ResourceUsages::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	HSResourceUsagePtr HSTopicBase::ResourceUsages::front()
	{
		return (HSResourceUsagePtr) Relation::front();
	}



	// back() method
	//
	HSResourceUsagePtr HSTopicBase::ResourceUsages::back()
	{
		return (HSResourceUsagePtr) Relation::back();
	}





	// =====================================================================================
	// Class HSTopicBase::MetaDatabases
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(HSTopicBase::MetaDatabases, Relation);

	// De-reference operator*()
	//
	HSMetaDatabaseTopicPtr HSTopicBase::MetaDatabases::iterator::operator*()
	{
		return (HSMetaDatabaseTopicPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	HSTopicBase::MetaDatabases::iterator& HSTopicBase::MetaDatabases::iterator::operator=(const iterator& itr)
	{
		return (HSTopicBase::MetaDatabases::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	HSMetaDatabaseTopicPtr HSTopicBase::MetaDatabases::front()
	{
		return (HSMetaDatabaseTopicPtr) Relation::front();
	}



	// back() method
	//
	HSMetaDatabaseTopicPtr HSTopicBase::MetaDatabases::back()
	{
		return (HSMetaDatabaseTopicPtr) Relation::back();
	}





	// =====================================================================================
	// Class HSTopicBase::MetaEntities
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(HSTopicBase::MetaEntities, Relation);

	// De-reference operator*()
	//
	HSMetaEntityTopicPtr HSTopicBase::MetaEntities::iterator::operator*()
	{
		return (HSMetaEntityTopicPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	HSTopicBase::MetaEntities::iterator& HSTopicBase::MetaEntities::iterator::operator=(const iterator& itr)
	{
		return (HSTopicBase::MetaEntities::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	HSMetaEntityTopicPtr HSTopicBase::MetaEntities::front()
	{
		return (HSMetaEntityTopicPtr) Relation::front();
	}



	// back() method
	//
	HSMetaEntityTopicPtr HSTopicBase::MetaEntities::back()
	{
		return (HSMetaEntityTopicPtr) Relation::back();
	}





	// =====================================================================================
	// Class HSTopicBase::MetaColumns
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(HSTopicBase::MetaColumns, Relation);

	// De-reference operator*()
	//
	HSMetaColumnTopicPtr HSTopicBase::MetaColumns::iterator::operator*()
	{
		return (HSMetaColumnTopicPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	HSTopicBase::MetaColumns::iterator& HSTopicBase::MetaColumns::iterator::operator=(const iterator& itr)
	{
		return (HSTopicBase::MetaColumns::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	HSMetaColumnTopicPtr HSTopicBase::MetaColumns::front()
	{
		return (HSMetaColumnTopicPtr) Relation::front();
	}



	// back() method
	//
	HSMetaColumnTopicPtr HSTopicBase::MetaColumns::back()
	{
		return (HSMetaColumnTopicPtr) Relation::back();
	}





	// =====================================================================================
	// Class HSTopicBase::MetaKeys
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(HSTopicBase::MetaKeys, Relation);

	// De-reference operator*()
	//
	HSMetaKeyTopicPtr HSTopicBase::MetaKeys::iterator::operator*()
	{
		return (HSMetaKeyTopicPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	HSTopicBase::MetaKeys::iterator& HSTopicBase::MetaKeys::iterator::operator=(const iterator& itr)
	{
		return (HSTopicBase::MetaKeys::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	HSMetaKeyTopicPtr HSTopicBase::MetaKeys::front()
	{
		return (HSMetaKeyTopicPtr) Relation::front();
	}



	// back() method
	//
	HSMetaKeyTopicPtr HSTopicBase::MetaKeys::back()
	{
		return (HSMetaKeyTopicPtr) Relation::back();
	}





	// =====================================================================================
	// Class HSTopicBase::MetaRelations
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(HSTopicBase::MetaRelations, Relation);

	// De-reference operator*()
	//
	HSMetaRelationTopicPtr HSTopicBase::MetaRelations::iterator::operator*()
	{
		return (HSMetaRelationTopicPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	HSTopicBase::MetaRelations::iterator& HSTopicBase::MetaRelations::iterator::operator=(const iterator& itr)
	{
		return (HSTopicBase::MetaRelations::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	HSMetaRelationTopicPtr HSTopicBase::MetaRelations::front()
	{
		return (HSMetaRelationTopicPtr) Relation::front();
	}



	// back() method
	//
	HSMetaRelationTopicPtr HSTopicBase::MetaRelations::back()
	{
		return (HSMetaRelationTopicPtr) Relation::back();
	}




} // end namespace D3
