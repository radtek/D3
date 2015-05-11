// MODULE: HSTopicAssociationBase.cpp
//;
//; IMPLEMENTATION CLASS: HSTopicAssociationBase
//;

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can regenerate this file.
//

#include "D3Types.h"

#include "HSTopicAssociationBase.h"

// Dependent includes
//
#include "HSTopicBase.h"


namespace D3
{

	// =====================================================================================
	// Class HSTopicAssociationBase Implementation
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(HSTopicAssociationBase, Entity);


	// Get a collection reflecting all currently resident instances of this
	//
	/* static */
	InstanceKeyPtrSetPtr HSTopicAssociationBase::GetAll(DatabasePtr pDB)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3HSDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3HSDB"));

		if (!pDatabase)
			return NULL;

		return pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopicAssociation)->GetPrimaryMetaKey()->GetInstanceKeySet(pDatabase);
	}



	// Make all objects of this type memory resident
	//
	/* static */
	void HSTopicAssociationBase::LoadAll(DatabasePtr pDB, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3HSDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3HSDB"));

		if (!pDatabase)
			return;

		pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopicAssociation)->LoadAll(pDatabase, bRefresh, bLazyFetch);
	}



	// Make a specific instance resident
	//
	/* static */
	HSTopicAssociationPtr HSTopicAssociationBase::Load(DatabasePtr pDB, long lParentTopicID, long lChildTopicID, float fSequenceNo, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3HSDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3HSDB"));

		if (!pDatabase)
			return NULL;

		TemporaryKey	tmpKey(*(pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopicAssociation)->GetPrimaryMetaKey()));

		// Set all key column values
		//
		tmpKey.GetColumn(D3HSDB_HSTopicAssociation_ParentTopicID)->SetValue(lParentTopicID);
		tmpKey.GetColumn(D3HSDB_HSTopicAssociation_ChildTopicID)->SetValue(lChildTopicID);
		tmpKey.GetColumn(D3HSDB_HSTopicAssociation_SequenceNo)->SetValue(fSequenceNo);

		return (HSTopicAssociationPtr) pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSTopicAssociation)->GetPrimaryMetaKey()->LoadObject(&tmpKey, pDatabase, bRefresh, bLazyFetch);
	}



	// Get parent of relation ParentTopic
	//
	HSTopicPtr HSTopicAssociationBase::GetParentTopic()
	{
		RelationPtr		pRelation;

		pRelation = GetParentRelation(D3HSDB_HSTopicAssociation_PR_ParentTopic);

		if (!pRelation)
			return NULL;

		return (HSTopicPtr) pRelation->GetParent();
	}



	// Set parent of relation ParentTopic
	//
	void HSTopicAssociationBase::SetParentTopic(HSTopicPtr pParent)
	{
		SetParent(m_pMetaEntity->GetParentMetaRelation(D3HSDB_HSTopicAssociation_PR_ParentTopic), (EntityPtr) pParent);
	}



	// Get parent of relation ChildTopic
	//
	HSTopicPtr HSTopicAssociationBase::GetChildTopic()
	{
		RelationPtr		pRelation;

		pRelation = GetParentRelation(D3HSDB_HSTopicAssociation_PR_ChildTopic);

		if (!pRelation)
			return NULL;

		return (HSTopicPtr) pRelation->GetParent();
	}



	// Set parent of relation ChildTopic
	//
	void HSTopicAssociationBase::SetChildTopic(HSTopicPtr pParent)
	{
		SetParent(m_pMetaEntity->GetParentMetaRelation(D3HSDB_HSTopicAssociation_PR_ChildTopic), (EntityPtr) pParent);
	}





	// =====================================================================================
	// Class HSTopicAssociationBase::iterator Implementation
	//

	// De-reference operator*()
	//
	HSTopicAssociationPtr HSTopicAssociationBase::iterator::operator*()
	{
		InstanceKeyPtr pKey;
		EntityPtr      pEntity;

		pKey = (InstanceKeyPtr) ((InstanceKeyPtrSetItr*) this)->operator*();
		pEntity = pKey->GetEntity();

		return (HSTopicAssociationPtr) pEntity;
	}



	// Assignment operator=()
	//
	HSTopicAssociationBase::iterator& HSTopicAssociationBase::iterator::operator=(const iterator& itr)
	{
		((InstanceKeyPtrSetItr*) this)->operator=(itr);

		return *this;
	}




} // end namespace D3