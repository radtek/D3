// MODULE: HSMetaDatabaseTopicBase.cpp
//;
//; IMPLEMENTATION CLASS: HSMetaDatabaseTopicBase
//;

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can regenerate this file.
//

#include "D3Types.h"

#include "HSMetaDatabaseTopicBase.h"

// Dependent includes
//
#include "HSTopicBase.h"


namespace D3
{

	// =====================================================================================
	// Class HSMetaDatabaseTopicBase Implementation
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(HSMetaDatabaseTopicBase, Entity);


	// Get a collection reflecting all currently resident instances of this
	//
	/* static */
	InstanceKeyPtrSetPtr HSMetaDatabaseTopicBase::GetAll(DatabasePtr pDB)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3HSDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3HSDB"));

		if (!pDatabase)
			return NULL;

		return pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSMetaDatabaseTopic)->GetPrimaryMetaKey()->GetInstanceKeySet(pDatabase);
	}



	// Make all objects of this type memory resident
	//
	/* static */
	void HSMetaDatabaseTopicBase::LoadAll(DatabasePtr pDB, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3HSDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3HSDB"));

		if (!pDatabase)
			return;

		pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSMetaDatabaseTopic)->LoadAll(pDatabase, bRefresh, bLazyFetch);
	}



	// Make a specific instance resident
	//
	/* static */
	HSMetaDatabaseTopicPtr HSMetaDatabaseTopicBase::Load(DatabasePtr pDB, const std::string& strMetaDatabaseAlias, long lTopicID, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3HSDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3HSDB"));

		if (!pDatabase)
			return NULL;

		TemporaryKey	tmpKey(*(pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSMetaDatabaseTopic)->GetPrimaryMetaKey()));

		// Set all key column values
		//
		tmpKey.GetColumn(D3HSDB_HSMetaDatabaseTopic_MetaDatabaseAlias)->SetValue(strMetaDatabaseAlias);
		tmpKey.GetColumn(D3HSDB_HSMetaDatabaseTopic_TopicID)->SetValue(lTopicID);

		return (HSMetaDatabaseTopicPtr) pDatabase->GetMetaDatabase()->GetMetaEntity(D3HSDB_HSMetaDatabaseTopic)->GetPrimaryMetaKey()->LoadObject(&tmpKey, pDatabase, bRefresh, bLazyFetch);
	}



	// Get parent of relation Topic
	//
	HSTopicPtr HSMetaDatabaseTopicBase::GetTopic()
	{
		RelationPtr		pRelation;

		pRelation = GetParentRelation(D3HSDB_HSMetaDatabaseTopic_PR_Topic);

		if (!pRelation)
			return NULL;

		return (HSTopicPtr) pRelation->GetParent();
	}



	// Set parent of relation Topic
	//
	void HSMetaDatabaseTopicBase::SetTopic(HSTopicPtr pParent)
	{
		SetParent(m_pMetaEntity->GetParentMetaRelation(D3HSDB_HSMetaDatabaseTopic_PR_Topic), (EntityPtr) pParent);
	}





	// =====================================================================================
	// Class HSMetaDatabaseTopicBase::iterator Implementation
	//

	// De-reference operator*()
	//
	HSMetaDatabaseTopicPtr HSMetaDatabaseTopicBase::iterator::operator*()
	{
		InstanceKeyPtr pKey;
		EntityPtr      pEntity;

		pKey = (InstanceKeyPtr) ((InstanceKeyPtrSetItr*) this)->operator*();
		pEntity = pKey->GetEntity();

		return (HSMetaDatabaseTopicPtr) pEntity;
	}



	// Assignment operator=()
	//
	HSMetaDatabaseTopicBase::iterator& HSMetaDatabaseTopicBase::iterator::operator=(const iterator& itr)
	{
		((InstanceKeyPtrSetItr*) this)->operator=(itr);

		return *this;
	}




} // end namespace D3