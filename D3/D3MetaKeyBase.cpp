// MODULE: D3MetaKeyBase.cpp
//;
//; IMPLEMENTATION CLASS: D3MetaKeyBase
//;

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can regenerate this file.
//

#include "D3Types.h"

#include "D3MetaKeyBase.h"

// Dependent includes
//
#include "D3MetaEntityBase.h"
#include "D3MetaKeyColumnBase.h"
#include "D3MetaRelationBase.h"


namespace D3
{

	// =====================================================================================
	// Class D3MetaKeyBase Implementation
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(D3MetaKeyBase, Entity);


	// Get a collection reflecting all currently resident instances of this
	//
	/* static */
	InstanceKeyPtrSetPtr D3MetaKeyBase::GetAll(DatabasePtr pDB)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3MDDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3MDDB"));

		if (!pDatabase)
			return NULL;

		return pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKey)->GetPrimaryMetaKey()->GetInstanceKeySet(pDatabase);
	}



	// Make all objects of this type memory resident
	//
	/* static */
	void D3MetaKeyBase::LoadAll(DatabasePtr pDB, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3MDDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3MDDB"));

		if (!pDatabase)
			return;

		pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKey)->LoadAll(pDatabase, bRefresh, bLazyFetch);
	}



	// Make a specific instance resident
	//
	/* static */
	D3MetaKeyPtr D3MetaKeyBase::Load(DatabasePtr pDB, long lID, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3MDDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3MDDB"));

		if (!pDatabase)
			return NULL;

		TemporaryKey	tmpKey(*(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKey)->GetPrimaryMetaKey()));

		// Set all key column values
		//
		tmpKey.GetColumn(D3MDDB_D3MetaKey_ID)->SetValue(lID);

		return (D3MetaKeyPtr) pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKey)->GetPrimaryMetaKey()->LoadObject(&tmpKey, pDatabase, bRefresh, bLazyFetch);
	}



	// Get parent of relation D3MetaEntity
	//
	D3MetaEntityPtr D3MetaKeyBase::GetD3MetaEntity()
	{
		RelationPtr		pRelation;

		pRelation = GetParentRelation(D3MDDB_D3MetaKey_PR_D3MetaEntity);

		if (!pRelation)
			return NULL;

		return (D3MetaEntityPtr) pRelation->GetParent();
	}



	// Set parent of relation D3MetaEntity
	//
	void D3MetaKeyBase::SetD3MetaEntity(D3MetaEntityPtr pParent)
	{
		SetParent(m_pMetaEntity->GetParentMetaRelation(D3MDDB_D3MetaKey_PR_D3MetaEntity), (EntityPtr) pParent);
	}



	// Load all instances of relation D3MetaKeyColumns
	//
	void D3MetaKeyBase::LoadAllD3MetaKeyColumns(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKey)->GetChildMetaRelation(D3MDDB_D3MetaKey_CR_D3MetaKeyColumns));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation D3MetaKeyColumns
	//
	D3MetaKeyBase::D3MetaKeyColumns* D3MetaKeyBase::GetD3MetaKeyColumns()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKey)->GetChildMetaRelation(D3MDDB_D3MetaKey_CR_D3MetaKeyColumns));

		return (D3MetaKeyBase::D3MetaKeyColumns*) pRelation;
	}



	// Load all instances of relation ChildD3MetaRelations
	//
	void D3MetaKeyBase::LoadAllChildD3MetaRelations(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKey)->GetChildMetaRelation(D3MDDB_D3MetaKey_CR_ChildD3MetaRelations));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation ChildD3MetaRelations
	//
	D3MetaKeyBase::ChildD3MetaRelations* D3MetaKeyBase::GetChildD3MetaRelations()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKey)->GetChildMetaRelation(D3MDDB_D3MetaKey_CR_ChildD3MetaRelations));

		return (D3MetaKeyBase::ChildD3MetaRelations*) pRelation;
	}



	// Load all instances of relation ParentD3MetaRelations
	//
	void D3MetaKeyBase::LoadAllParentD3MetaRelations(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKey)->GetChildMetaRelation(D3MDDB_D3MetaKey_CR_ParentD3MetaRelations));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation ParentD3MetaRelations
	//
	D3MetaKeyBase::ParentD3MetaRelations* D3MetaKeyBase::GetParentD3MetaRelations()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaKey)->GetChildMetaRelation(D3MDDB_D3MetaKey_CR_ParentD3MetaRelations));

		return (D3MetaKeyBase::ParentD3MetaRelations*) pRelation;
	}





	// =====================================================================================
	// Class D3MetaKeyBase::iterator Implementation
	//

	// De-reference operator*()
	//
	D3MetaKeyPtr D3MetaKeyBase::iterator::operator*()
	{
		InstanceKeyPtr pKey;
		EntityPtr      pEntity;

		pKey = (InstanceKeyPtr) ((InstanceKeyPtrSetItr*) this)->operator*();
		pEntity = pKey->GetEntity();

		return (D3MetaKeyPtr) pEntity;
	}



	// Assignment operator=()
	//
	D3MetaKeyBase::iterator& D3MetaKeyBase::iterator::operator=(const iterator& itr)
	{
		((InstanceKeyPtrSetItr*) this)->operator=(itr);

		return *this;
	}





	// =====================================================================================
	// Class D3MetaKeyBase::D3MetaKeyColumns
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(D3MetaKeyBase::D3MetaKeyColumns, Relation);

	// De-reference operator*()
	//
	D3MetaKeyColumnPtr D3MetaKeyBase::D3MetaKeyColumns::iterator::operator*()
	{
		return (D3MetaKeyColumnPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	D3MetaKeyBase::D3MetaKeyColumns::iterator& D3MetaKeyBase::D3MetaKeyColumns::iterator::operator=(const iterator& itr)
	{
		return (D3MetaKeyBase::D3MetaKeyColumns::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	D3MetaKeyColumnPtr D3MetaKeyBase::D3MetaKeyColumns::front()
	{
		return (D3MetaKeyColumnPtr) Relation::front();
	}



	// back() method
	//
	D3MetaKeyColumnPtr D3MetaKeyBase::D3MetaKeyColumns::back()
	{
		return (D3MetaKeyColumnPtr) Relation::back();
	}





	// =====================================================================================
	// Class D3MetaKeyBase::ChildD3MetaRelations
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(D3MetaKeyBase::ChildD3MetaRelations, Relation);

	// De-reference operator*()
	//
	D3MetaRelationPtr D3MetaKeyBase::ChildD3MetaRelations::iterator::operator*()
	{
		return (D3MetaRelationPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	D3MetaKeyBase::ChildD3MetaRelations::iterator& D3MetaKeyBase::ChildD3MetaRelations::iterator::operator=(const iterator& itr)
	{
		return (D3MetaKeyBase::ChildD3MetaRelations::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	D3MetaRelationPtr D3MetaKeyBase::ChildD3MetaRelations::front()
	{
		return (D3MetaRelationPtr) Relation::front();
	}



	// back() method
	//
	D3MetaRelationPtr D3MetaKeyBase::ChildD3MetaRelations::back()
	{
		return (D3MetaRelationPtr) Relation::back();
	}





	// =====================================================================================
	// Class D3MetaKeyBase::ParentD3MetaRelations
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(D3MetaKeyBase::ParentD3MetaRelations, Relation);

	// De-reference operator*()
	//
	D3MetaRelationPtr D3MetaKeyBase::ParentD3MetaRelations::iterator::operator*()
	{
		return (D3MetaRelationPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	D3MetaKeyBase::ParentD3MetaRelations::iterator& D3MetaKeyBase::ParentD3MetaRelations::iterator::operator=(const iterator& itr)
	{
		return (D3MetaKeyBase::ParentD3MetaRelations::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	D3MetaRelationPtr D3MetaKeyBase::ParentD3MetaRelations::front()
	{
		return (D3MetaRelationPtr) Relation::front();
	}



	// back() method
	//
	D3MetaRelationPtr D3MetaKeyBase::ParentD3MetaRelations::back()
	{
		return (D3MetaRelationPtr) Relation::back();
	}




} // end namespace D3
