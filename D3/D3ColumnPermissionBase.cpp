// MODULE: D3ColumnPermissionBase.cpp
//;
//; IMPLEMENTATION CLASS: D3ColumnPermissionBase
//;

// WARNING: This file has been generated by D3. You should NEVER change this file.
//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()
//          which explains in detail how you can regenerate this file.
//

#include "D3Types.h"

#include "D3ColumnPermissionBase.h"

// Dependent includes
//
#include "D3MetaColumnBase.h"
#include "D3RoleBase.h"


namespace D3
{

	// =====================================================================================
	// Class D3ColumnPermissionBase Implementation
	//

	// D3 class factory support for this class
	//
	D3_CLASS_IMPL(D3ColumnPermissionBase, Entity);


	// Get a collection reflecting all currently resident instances of this
	//
	/* static */
	InstanceKeyPtrSetPtr D3ColumnPermissionBase::GetAll(DatabasePtr pDB)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3MDDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3MDDB"));

		if (!pDatabase)
			return NULL;

		return pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3ColumnPermission)->GetPrimaryMetaKey()->GetInstanceKeySet(pDatabase);
	}



	// Make all objects of this type memory resident
	//
	/* static */
	void D3ColumnPermissionBase::LoadAll(DatabasePtr pDB, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3MDDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3MDDB"));

		if (!pDatabase)
			return;

		pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3ColumnPermission)->LoadAll(pDatabase, bRefresh, bLazyFetch);
	}



	// Make a specific instance resident
	//
	/* static */
	D3ColumnPermissionPtr D3ColumnPermissionBase::Load(DatabasePtr pDB, long lRoleID, long lMetaColumnID, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr		pDatabase = pDB;


		if (!pDatabase)
			return NULL;

		if (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase("D3MDDB"))
			pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase("D3MDDB"));

		if (!pDatabase)
			return NULL;

		TemporaryKey	tmpKey(*(pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3ColumnPermission)->GetPrimaryMetaKey()));

		// Set all key column values
		//
		tmpKey.GetColumn(D3MDDB_D3ColumnPermission_RoleID)->SetValue(lRoleID);
		tmpKey.GetColumn(D3MDDB_D3ColumnPermission_MetaColumnID)->SetValue(lMetaColumnID);

		return (D3ColumnPermissionPtr) pDatabase->GetMetaDatabase()->GetMetaEntity(D3MDDB_D3ColumnPermission)->GetPrimaryMetaKey()->LoadObject(&tmpKey, pDatabase, bRefresh, bLazyFetch);
	}



	// Get parent of relation D3Role
	//
	D3RolePtr D3ColumnPermissionBase::GetD3Role()
	{
		RelationPtr		pRelation;

		pRelation = GetParentRelation(D3MDDB_D3ColumnPermission_PR_D3Role);

		if (!pRelation)
			return NULL;

		return (D3RolePtr) pRelation->GetParent();
	}



	// Set parent of relation D3Role
	//
	void D3ColumnPermissionBase::SetD3Role(D3RolePtr pParent)
	{
		SetParent(m_pMetaEntity->GetParentMetaRelation(D3MDDB_D3ColumnPermission_PR_D3Role), (EntityPtr) pParent);
	}



	// Get parent of relation D3MetaColumn
	//
	D3MetaColumnPtr D3ColumnPermissionBase::GetD3MetaColumn()
	{
		RelationPtr		pRelation;

		pRelation = GetParentRelation(D3MDDB_D3ColumnPermission_PR_D3MetaColumn);

		if (!pRelation)
			return NULL;

		return (D3MetaColumnPtr) pRelation->GetParent();
	}



	// Set parent of relation D3MetaColumn
	//
	void D3ColumnPermissionBase::SetD3MetaColumn(D3MetaColumnPtr pParent)
	{
		SetParent(m_pMetaEntity->GetParentMetaRelation(D3MDDB_D3ColumnPermission_PR_D3MetaColumn), (EntityPtr) pParent);
	}





	// =====================================================================================
	// Class D3ColumnPermissionBase::iterator Implementation
	//

	// De-reference operator*()
	//
	D3ColumnPermissionPtr D3ColumnPermissionBase::iterator::operator*()
	{
		InstanceKeyPtr pKey;
		EntityPtr      pEntity;

		pKey = (InstanceKeyPtr) ((InstanceKeyPtrSetItr*) this)->operator*();
		pEntity = pKey->GetEntity();

		return (D3ColumnPermissionPtr) pEntity;
	}



	// Assignment operator=()
	//
	D3ColumnPermissionBase::iterator& D3ColumnPermissionBase::iterator::operator=(const iterator& itr)
	{
		((InstanceKeyPtrSetItr*) this)->operator=(itr);

		return *this;
	}




} // end namespace D3