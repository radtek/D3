// MODULE: D3MetaColumnBase.cpp
//;
//; IMPLEMENTATION CLASS: D3MetaColumnBase
//;

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can regenerate this file.
//

#include "D3Types.h"

#include "D3MetaColumnBase.h"

// Dependent includes
//
#include "D3ColumnPermissionBase.h"
#include "D3MetaColumnChoiceBase.h"
#include "D3MetaEntityBase.h"
#include "D3MetaKeyColumnBase.h"
#include "D3MetaRelationBase.h"


namespace D3
{

	// =====================================================================================
	// Class D3MetaColumnBase Implementation
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(D3MetaColumnBase, Entity);


	// Get a collection reflecting all currently resident instances of this
	//
	/* static */
	InstanceKeyPtrSetPtr D3MetaColumnBase::GetAll(DatabasePtr pDB)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3MDDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3MDDB"));

		if (!pDatabase)
			return NULL;

		return pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->GetPrimaryMetaKey()->GetInstanceKeySet(pDatabase);
	}



	// Make all objects of this type memory resident
	//
	/* static */
	void D3MetaColumnBase::LoadAll(DatabasePtr pDB, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3MDDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3MDDB"));

		if (!pDatabase)
			return;

		pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->LoadAll(pDatabase, bRefresh, bLazyFetch);
	}



	// Make a specific instance resident
	//
	/* static */
	D3MetaColumnPtr D3MetaColumnBase::Load(DatabasePtr pDB, long lID, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3MDDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3MDDB"));

		if (!pDatabase)
			return NULL;

		TemporaryKey	tmpKey(*(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->GetPrimaryMetaKey()));

		// Set all key column values
		//
		tmpKey.GetColumn(D3MDDB_D3MetaColumn_ID)->SetValue(lID);

		return (D3MetaColumnPtr) pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->GetPrimaryMetaKey()->LoadObject(&tmpKey, pDatabase, bRefresh, bLazyFetch);
	}



	// Get parent of relation D3MetaEntity
	//
	D3MetaEntityPtr D3MetaColumnBase::GetD3MetaEntity()
	{
		RelationPtr		pRelation;

		pRelation = GetParentRelation(D3MDDB_D3MetaColumn_PR_D3MetaEntity);

		if (!pRelation)
			return NULL;

		return (D3MetaEntityPtr) pRelation->GetParent();
	}



	// Set parent of relation D3MetaEntity
	//
	void D3MetaColumnBase::SetD3MetaEntity(D3MetaEntityPtr pParent)
	{
		SetParent(m_pMetaEntity->GetParentMetaRelation(D3MDDB_D3MetaColumn_PR_D3MetaEntity), (EntityPtr) pParent);
	}



	// Load all instances of relation D3MetaKeyColumns
	//
	void D3MetaColumnBase::LoadAllD3MetaKeyColumns(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->GetChildMetaRelation(D3MDDB_D3MetaColumn_CR_D3MetaKeyColumns));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation D3MetaKeyColumns
	//
	D3MetaColumnBase::D3MetaKeyColumns* D3MetaColumnBase::GetD3MetaKeyColumns()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->GetChildMetaRelation(D3MDDB_D3MetaColumn_CR_D3MetaKeyColumns));

		return (D3MetaColumnBase::D3MetaKeyColumns*) pRelation;
	}



	// Load all instances of relation D3MetaColumnChoices
	//
	void D3MetaColumnBase::LoadAllD3MetaColumnChoices(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->GetChildMetaRelation(D3MDDB_D3MetaColumn_CR_D3MetaColumnChoices));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation D3MetaColumnChoices
	//
	D3MetaColumnBase::D3MetaColumnChoices* D3MetaColumnBase::GetD3MetaColumnChoices()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->GetChildMetaRelation(D3MDDB_D3MetaColumn_CR_D3MetaColumnChoices));

		return (D3MetaColumnBase::D3MetaColumnChoices*) pRelation;
	}



	// Load all instances of relation D3MetaRelationSwitches
	//
	void D3MetaColumnBase::LoadAllD3MetaRelationSwitches(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->GetChildMetaRelation(D3MDDB_D3MetaColumn_CR_D3MetaRelationSwitches));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation D3MetaRelationSwitches
	//
	D3MetaColumnBase::D3MetaRelationSwitches* D3MetaColumnBase::GetD3MetaRelationSwitches()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->GetChildMetaRelation(D3MDDB_D3MetaColumn_CR_D3MetaRelationSwitches));

		return (D3MetaColumnBase::D3MetaRelationSwitches*) pRelation;
	}



	// Load all instances of relation D3RolePermissions
	//
	void D3MetaColumnBase::LoadAllD3RolePermissions(bool bRefresh, bool bLazyFetch)
	{
		RelationPtr		pRelation = NULL;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->GetChildMetaRelation(D3MDDB_D3MetaColumn_CR_D3RolePermissions));

		if (pRelation)
			pRelation->LoadAll(bRefresh, bLazyFetch);
	}



	// Get the relation D3RolePermissions
	//
	D3MetaColumnBase::D3RolePermissions* D3MetaColumnBase::GetD3RolePermissions()
	{
		RelationPtr		pRelation;

		pRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3MetaColumn)->GetChildMetaRelation(D3MDDB_D3MetaColumn_CR_D3RolePermissions));

		return (D3MetaColumnBase::D3RolePermissions*) pRelation;
	}





	// =====================================================================================
	// Class D3MetaColumnBase::iterator Implementation
	//

	// De-reference operator*()
	//
	D3MetaColumnPtr D3MetaColumnBase::iterator::operator*()
	{
		InstanceKeyPtr pKey;
		EntityPtr      pEntity;

		pKey = (InstanceKeyPtr) ((InstanceKeyPtrSetItr*) this)->operator*();
		pEntity = pKey->GetEntity();

		return (D3MetaColumnPtr) pEntity;
	}



	// Assignment operator=()
	//
	D3MetaColumnBase::iterator& D3MetaColumnBase::iterator::operator=(const iterator& itr)
	{
		((InstanceKeyPtrSetItr*) this)->operator=(itr);

		return *this;
	}





	// =====================================================================================
	// Class D3MetaColumnBase::D3MetaKeyColumns
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(D3MetaColumnBase::D3MetaKeyColumns, Relation);

	// De-reference operator*()
	//
	D3MetaKeyColumnPtr D3MetaColumnBase::D3MetaKeyColumns::iterator::operator*()
	{
		return (D3MetaKeyColumnPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	D3MetaColumnBase::D3MetaKeyColumns::iterator& D3MetaColumnBase::D3MetaKeyColumns::iterator::operator=(const iterator& itr)
	{
		return (D3MetaColumnBase::D3MetaKeyColumns::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	D3MetaKeyColumnPtr D3MetaColumnBase::D3MetaKeyColumns::front()
	{
		return (D3MetaKeyColumnPtr) Relation::front();
	}



	// back() method
	//
	D3MetaKeyColumnPtr D3MetaColumnBase::D3MetaKeyColumns::back()
	{
		return (D3MetaKeyColumnPtr) Relation::back();
	}





	// =====================================================================================
	// Class D3MetaColumnBase::D3MetaColumnChoices
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(D3MetaColumnBase::D3MetaColumnChoices, Relation);

	// De-reference operator*()
	//
	D3MetaColumnChoicePtr D3MetaColumnBase::D3MetaColumnChoices::iterator::operator*()
	{
		return (D3MetaColumnChoicePtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	D3MetaColumnBase::D3MetaColumnChoices::iterator& D3MetaColumnBase::D3MetaColumnChoices::iterator::operator=(const iterator& itr)
	{
		return (D3MetaColumnBase::D3MetaColumnChoices::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	D3MetaColumnChoicePtr D3MetaColumnBase::D3MetaColumnChoices::front()
	{
		return (D3MetaColumnChoicePtr) Relation::front();
	}



	// back() method
	//
	D3MetaColumnChoicePtr D3MetaColumnBase::D3MetaColumnChoices::back()
	{
		return (D3MetaColumnChoicePtr) Relation::back();
	}





	// =====================================================================================
	// Class D3MetaColumnBase::D3MetaRelationSwitches
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(D3MetaColumnBase::D3MetaRelationSwitches, Relation);

	// De-reference operator*()
	//
	D3MetaRelationPtr D3MetaColumnBase::D3MetaRelationSwitches::iterator::operator*()
	{
		return (D3MetaRelationPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	D3MetaColumnBase::D3MetaRelationSwitches::iterator& D3MetaColumnBase::D3MetaRelationSwitches::iterator::operator=(const iterator& itr)
	{
		return (D3MetaColumnBase::D3MetaRelationSwitches::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	D3MetaRelationPtr D3MetaColumnBase::D3MetaRelationSwitches::front()
	{
		return (D3MetaRelationPtr) Relation::front();
	}



	// back() method
	//
	D3MetaRelationPtr D3MetaColumnBase::D3MetaRelationSwitches::back()
	{
		return (D3MetaRelationPtr) Relation::back();
	}





	// =====================================================================================
	// Class D3MetaColumnBase::D3RolePermissions
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(D3MetaColumnBase::D3RolePermissions, Relation);

	// De-reference operator*()
	//
	D3ColumnPermissionPtr D3MetaColumnBase::D3RolePermissions::iterator::operator*()
	{
		return (D3ColumnPermissionPtr) ((Relation::iterator*) this)->operator*();
	}



	// Assignment operator=()
	//
	D3MetaColumnBase::D3RolePermissions::iterator& D3MetaColumnBase::D3RolePermissions::iterator::operator=(const iterator& itr)
	{
		return (D3MetaColumnBase::D3RolePermissions::iterator&) ((Relation::iterator*) this)->operator=(itr);
	}



	// front() method
	//
	D3ColumnPermissionPtr D3MetaColumnBase::D3RolePermissions::front()
	{
		return (D3ColumnPermissionPtr) Relation::front();
	}



	// back() method
	//
	D3ColumnPermissionPtr D3MetaColumnBase::D3RolePermissions::back()
	{
		return (D3ColumnPermissionPtr) Relation::back();
	}




} // end namespace D3