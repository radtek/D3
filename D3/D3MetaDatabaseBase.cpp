// MODULE: D3MetaDatabaseBase.cpp
//;
//; IMPLEMENTATION CLASS: D3MetaDatabaseBase
//;

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can regenerate this file.
//

#include "D3Types.h"

#include "D3MetaDatabaseBase.h"

// Dependent includes
//
#include "D3DatabasePermissionBase.h"
#include "D3MetaEntityBase.h"


namespace D3
{

	// =====================================================================================
	// Class D3MetaDatabaseBase Implementation
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(D3MetaDatabaseBase, Entity);


	// Get a collection reflecting all currently resident instances of this
	//
	/* static */
	InstanceKeyPtrSetPtr D3MetaDatabaseBase::GetAll(DatabasePtr pDB)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3MDDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3MDDB"));

		if (!pDatabase)
			return NULL;

		return pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaDatabase)->GetPrimaryMetaKey()->GetInstanceKeySet(pDatabase);
	}



	// Make all objects of this type memory resident
	//
	/* static */
	void D3MetaDatabaseBase::LoadAll(DatabasePtr pDB, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3MDDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3MDDB"));

		if (!pDatabase)
			return;

		pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaDatabase)->LoadAll(pDatabase, bRefresh, bLazyFetch);
	}



	// Make a specific instance resident
	//
	/* static */
	D3MetaDatabasePtr D3MetaDatabaseBase::Load(DatabasePtr pDB, long lID, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3MDDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3MDDB"));

		if (!pDatabase)
			return NULL;

		TemporaryKey	tmpKey(*(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaDatabase)->GetPrimaryMetaKey()));

		// Set all key column values
		//
		tmpKey.GetColumn(D3MDDB_D3MetaDatabase_ID)->SetValue(lID);

		return (D3MetaDatabasePtr) pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaDatabase)->GetPrimaryMetaKey()->LoadObject(&tmpKey, pDatabase, bRefresh, bLazyFetch);
	}



	// Load all instances of relation D3MetaEntities
	//
	void D3MetaDatabaseBase::LoadAllD3MetaEntities(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaDatabase)->GetChildMetaRelation(D3MDDB_D3MetaDatabase_CR_D3MetaEntities));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation D3MetaEntities
	//
	D3MetaDatabaseBase::D3MetaEntities* D3MetaDatabaseBase::GetD3MetaEntities()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaDatabase)->GetChildMetaRelation(D3MDDB_D3MetaDatabase_CR_D3MetaEntities));

		return (D3MetaDatabaseBase::D3MetaEntities*) pRelation;
	}



	// Load all instances of relation D3RolePermissions
	//
	void D3MetaDatabaseBase::LoadAllD3RolePermissions(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaDatabase)->GetChildMetaRelation(D3MDDB_D3MetaDatabase_CR_D3RolePermissions));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation D3RolePermissions
	//
	D3MetaDatabaseBase::D3RolePermissions* D3MetaDatabaseBase::GetD3RolePermissions()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaDatabase)->GetChildMetaRelation(D3MDDB_D3MetaDatabase_CR_D3RolePermissions));

		return (D3MetaDatabaseBase::D3RolePermissions*) pRelation;
	}





	// =====================================================================================
	// Class D3MetaDatabaseBase::iterator Implementation
	//

	// De-reference operator*()
	//
	D3MetaDatabasePtr D3MetaDatabaseBase::iterator::operator*()
	{
		InstanceKeyPtr pKey;
		EntityPtr      pEntity;

		pKey = (InstanceKeyPtr) ((InstanceKeyPtrSetItr*) this)->operator*();
		pEntity = pKey->GetEntity();

		return (D3MetaDatabasePtr) pEntity;
	}



	// Assignment operator=()
	//
	D3MetaDatabaseBase::iterator& D3MetaDatabaseBase::iterator::operator=(const iterator& itr)
	{
		((InstanceKeyPtrSetItr*) this)->operator=(itr);

		return *this;
	}





	// =====================================================================================
	// Class D3MetaDatabaseBase::D3MetaEntities
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(D3MetaDatabaseBase::D3MetaEntities, Relation);

	// De-reference operator*()
	//
	D3MetaEntityPtr D3MetaDatabaseBase::D3MetaEntities::iterator::operator*()
	{
		return (D3MetaEntityPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	D3MetaDatabaseBase::D3MetaEntities::iterator& D3MetaDatabaseBase::D3MetaEntities::iterator::operator=(const iterator& itr)
	{
		return (D3MetaDatabaseBase::D3MetaEntities::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	D3MetaEntityPtr D3MetaDatabaseBase::D3MetaEntities::front()
	{
		return (D3MetaEntityPtr) Relation::front();
	}



	// back() method
	//
	D3MetaEntityPtr D3MetaDatabaseBase::D3MetaEntities::back()
	{
		return (D3MetaEntityPtr) Relation::back();
	}





	// =====================================================================================
	// Class D3MetaDatabaseBase::D3RolePermissions
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(D3MetaDatabaseBase::D3RolePermissions, Relation);

	// De-reference operator*()
	//
	D3DatabasePermissionPtr D3MetaDatabaseBase::D3RolePermissions::iterator::operator*()
	{
		return (D3DatabasePermissionPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	D3MetaDatabaseBase::D3RolePermissions::iterator& D3MetaDatabaseBase::D3RolePermissions::iterator::operator=(const iterator& itr)
	{
		return (D3MetaDatabaseBase::D3RolePermissions::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	D3DatabasePermissionPtr D3MetaDatabaseBase::D3RolePermissions::front()
	{
		return (D3DatabasePermissionPtr) Relation::front();
	}



	// back() method
	//
	D3DatabasePermissionPtr D3MetaDatabaseBase::D3RolePermissions::back()
	{
		return (D3DatabasePermissionPtr) Relation::back();
	}




} // end namespace D3
