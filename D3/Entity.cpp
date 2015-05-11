// MODULE: Entity source
//;
//; IMPLEMENTATION CLASS: Entity
//;
// ===========================================================
// File Info:
//
// $Author: pete $
// $Date: 2014/09/10 04:59:42 $
// $Revision: 1.67 $
//
// MetaColumn: This class describes a column of any of
// the ap3/t3 classes.
//
// ===========================================================
// Change History:
// ===========================================================
//
// 09/10/02 - R1 - Hugp
//
// Changes required to accomodate extended attributes as
// implemented by ActivityType and Activity:
//
// - Store column names as strings
//
// -----------------------------------------------------------

// @@DatatypeInclude
#include "D3Types.h"
//#include <project-parameters.h>
// @@End
// @@Includes
#include "Entity.h"
#include "Column.h"
#include "Key.h"
#include "Relation.h"
#include "ResultSet.h"
#include "Session.h"
#include "Codec.h"

#include <sstream>

namespace D3
{
	// ==========================================================================
	// MetaEntity::Flags class implementation
	//

	// Implement Characteristics bitmask
	BITS_IMPL(MetaEntity, Flags);

	// Primitive Characteristis bitmask values
	PRIMITIVEMASK_IMPL(MetaEntity, Flags, Hidden,							0x00000008);
	PRIMITIVEMASK_IMPL(MetaEntity, Flags, Cached,							0x00000010);
	PRIMITIVEMASK_IMPL(MetaEntity, Flags, ShowCK,							0x00000020);

	// Combo Characteristis bitmask values
	COMBOMASK_IMPL(MetaEntity, Flags, Default, 0);


	// Implement Permissions bitmask
	BITS_IMPL(MetaEntity, Permissions);

	// Primitive Permissions bitmask values
	PRIMITIVEMASK_IMPL(MetaEntity, Permissions, Insert,				0x00000001);
	PRIMITIVEMASK_IMPL(MetaEntity, Permissions, Select,				0x00000002);
	PRIMITIVEMASK_IMPL(MetaEntity, Permissions, Update,				0x00000004);
	PRIMITIVEMASK_IMPL(MetaEntity, Permissions, Delete,				0x00000008);

	// Combo Permissions bitmask values
	COMBOMASK_IMPL(MetaEntity, Permissions, Default,	MetaEntity::Permissions::Insert |
																										MetaEntity::Permissions::Select |
																										MetaEntity::Permissions::Update |
																										MetaEntity::Permissions::Delete);

	// ==========================================================================
	// MetaEntity implementation
	//

	// Statics:
	//
	EntityID																MetaEntity::M_uNextInternalID = ENTITY_ID_MAX;
	MetaEntityPtrMap												MetaEntity::M_mapMetaEntity;

	// Standard D3 stuff
	//
	D3_CLASS_IMPL(MetaEntity, Object);


	// This constructor is used by the framework and is followed by the call to init()
	MetaEntity::MetaEntity()
	: m_uID(0),
		m_pMetaDatabase(NULL),
		m_pInstanceClass(NULL),
		m_uEntityIdx(D3_UNDEFINED_ID),
		m_uPrimaryKeyIdx(D3_UNDEFINED_ID),
		m_uConceptualKeyIdx(D3_UNDEFINED_ID),
		m_sAssociative(-1)
	{
	}




	// Constructor needs to ensure all meta columns are created
	MetaEntity::MetaEntity(MetaDatabasePtr pMetaDatabase, const std::string & strName, const std::string & strInstanceClassName, const Flags::Mask & flags, const Permissions::Mask & permissions)
	: m_uID(M_uNextInternalID--),
		m_pMetaDatabase(pMetaDatabase),
		m_strName(strName),
		m_pInstanceClass(NULL),
		m_Flags(flags),
		m_Permissions(permissions),
		m_uEntityIdx(D3_UNDEFINED_ID),
		m_uPrimaryKeyIdx(D3_UNDEFINED_ID),
		m_uConceptualKeyIdx(D3_UNDEFINED_ID),
		m_sAssociative(-1)
	{
		Init(strInstanceClassName);
	}




	// Destructor needs to delete all meta columns
	MetaEntity::~MetaEntity()
	{
		MetaKeyPtr			pMK;
		MetaColumnPtr		pMC;
		MetaRelationPtr	pMR;
		unsigned int		idx;


		// Delete all child MetaRelation objects
		//
		for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
		{
			pMR = m_vectChildMetaRelation[idx];
			delete pMR;
		}

		// Delete all parent MetaRelation objects
		//
		for (idx = 0; idx < m_vectParentMetaRelation.size(); idx++)
		{
			pMR = m_vectParentMetaRelation[idx];
			delete pMR;
		}

		// Delete all MetaKey objects
		//
		for (idx = 0; idx < m_vectMetaKey.size(); idx++)
		{
			pMK = m_vectMetaKey[idx];
			delete pMK;
		}

		// Delete all MetaColumn objects
		//
		for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
		{
			pMC = m_vectMetaColumn[idx];
			delete pMC;
		}

		m_pMetaDatabase->On_MetaEntityDeleted(this);

		M_mapMetaEntity.erase(m_uID);
	}




	// This method is invoked during framework initialisation
	void MetaEntity::Init(MetaDatabasePtr pMetaDatabase, EntityID uID, const std::string & strName, const std::string & strInstanceClassName, const Flags::Mask & flags, const Permissions::Mask & permissions)
	{
		m_uID = uID;
		m_pMetaDatabase = pMetaDatabase;
		m_strName = strName;
		m_Flags = flags;
		m_Permissions = permissions;

		Init(strInstanceClassName);
	}




	void MetaEntity::Init(const std::string& strInstanceClassName)
	{
		std::string			strClassName = strInstanceClassName;


		if (strClassName.empty())
			strClassName = "Entity";

		try
		{
			m_pInstanceClass = &(Class::Of(strClassName));
		}
		catch(Exception & e)
		{
			e.AddMessage("MetaEntity::MetaEntity(): MetaEntity %s is configured to use instance class %s which isn't valid. Using default class 'Entity'.", m_strName.c_str(), strInstanceClassName.c_str());
			e.LogError();
			m_pInstanceClass = &Entity::ClassObject();
		}

		assert(m_pMetaDatabase);
		assert(!m_strName.empty());
		assert(m_pInstanceClass);
		assert(m_pInstanceClass->IsKindOf(Entity::ClassObject()));

		// If this is a duplicate, throw an exception
		if (m_pMetaDatabase->GetMetaEntity(m_strName))
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaEntity::Init(): MetaEntity %s already exists!", GetFullName().c_str());

		m_pMetaDatabase->On_MetaEntityCreated(this);

		assert(M_mapMetaEntity.find(m_uID) == M_mapMetaEntity.end());
		M_mapMetaEntity[m_uID] = this;
	}



	// Construct a MetaEntity object from a D3MetaEntity object (a D3MetaEntity object
	// represents a record from the D3MetaEntity table in the D3 MetaDictionary database)
	//
	/* static */
	void MetaEntity::LoadMetaObject(MetaDatabasePtr pNewMD, D3MetaEntityPtr pD3ME)
	{
		MetaEntityPtr														pNewME;
		std::string															strName;
		std::string															strClassName;
		Flags::Mask															flags;
		Permissions::Mask												permissions;
		D3MetaEntity::D3MetaColumns::iterator		itrD3MC;
		D3MetaColumnPtr													pD3MC;
		D3MetaEntity::D3MetaKeys::iterator			itrD3MK;
		D3MetaKeyPtr														pD3MK;


		assert(pD3ME);

		// Get the name of the object to create
		//
		strName = pD3ME->GetName();

		// Get the instance class name for the MetaEntity
		//
		if (!pD3ME->GetColumn(D3MDDB_D3MetaEntity_InstanceClass)->IsNull())
			strClassName = pD3ME->GetColumn(D3MDDB_D3MetaEntity_InstanceClass)->GetString();
		else
			strClassName = "Entity";

		// Set the construction flags
		//
		if (!pD3ME->GetColumn(D3MDDB_D3MetaEntity_Flags)->IsNull())
			flags = Flags::Mask(pD3ME->GetFlags());

		// Set the construction permissions and adjust if necessary
		//
		if (!pD3ME->GetColumn(D3MDDB_D3MetaEntity_AccessRights)->IsNull())
			permissions = Permissions::Mask(pD3ME->GetAccessRights());

		if (!pNewMD->AllowRead())
			permissions &= ~Permissions::Select;

		if (!pNewMD->AllowWrite())
			permissions &= ~(Permissions::Insert | Permissions::Update | Permissions::Delete);


		// Create an MetaEntity instance via the class factory
		const D3::Class*		pMetaClass = NULL;

		if (!pD3ME->GetColumn(D3MDDB_D3MetaEntity_MetaClass)->IsNull() && !pD3ME->GetMetaClass().empty())
		{
			try
			{
				pMetaClass = &(Class::Of(pD3ME->GetMetaClass()));

				if (!pMetaClass->IsKindOf(MetaEntity::ClassObject()))
					pMetaClass = NULL;
			}
			catch(...)
			{
				pMetaClass = NULL;
			}

			if (!pMetaClass)
				ReportError("MetaEntity::MetaEntity(): MetaEntity '%s' is configured to use meta class '%s' which isn't valid. Using default class MetaEntity", strName.c_str(), pD3ME->GetMetaClass().c_str());
		}

		if (pMetaClass)
			pNewME = (MetaEntityPtr) pMetaClass->CreateInstance();
		else
			pNewME = new MetaEntity();

		pNewME->Init(pNewMD, pD3ME->GetID(), strName, strClassName, flags, permissions);
		pNewME->On_BeforeConstructingMetaEntity();

		// Set all the other attributes
		//
		pNewME->m_strProsaName					= pD3ME->GetProsaName();
		pNewME->m_strDescription				= pD3ME->GetDescription();
		pNewME->m_strAlias							= pD3ME->GetAlias();
		pNewME->m_strInstanceInterface	= pD3ME->GetInstanceInterface();
		pNewME->m_strJSMetaClass				= pD3ME->GetJSMetaClass();
		pNewME->m_strJSInstanceClass		= pD3ME->GetJSInstanceClass();
		pNewME->m_strJSViewClass				= pD3ME->GetJSViewClass();
		pNewME->m_strJSDetailViewClass	= pD3ME->GetJSDetailViewClass();

		// Create MetaColumn objects
		//
		for ( itrD3MC =  pD3ME->GetD3MetaColumns()->begin();
					itrD3MC != pD3ME->GetD3MetaColumns()->end();
					itrD3MC++)
		{
			pD3MC = *itrD3MC;

			MetaColumn::LoadMetaObject(pNewME, pD3MC);
		}

		// Create MetaKey objects
		//
		for ( itrD3MK =  pD3ME->GetD3MetaKeys()->begin();
					itrD3MK != pD3ME->GetD3MetaKeys()->end();
					itrD3MK++)
		{
			pD3MK = *itrD3MK;

			MetaKey::LoadMetaObject(pNewME, pD3MK);
		}

		pNewME->On_AfterConstructingMetaEntity();
	}



	std::string	MetaEntity::GetFullName() const
	{
		std::string			strFullName;

		strFullName  = m_pMetaDatabase->GetFullName();
		strFullName += ".";
		strFullName += m_strName;

		return strFullName;
	}



	EntityPtr MetaEntity::CreateInstance(DatabasePtr pDB)
	{
		EntityPtr						pEntity = NULL;
		DatabasePtr					pDatabase = pDB;


		// Some sanity checks
		//
		assert(m_pInstanceClass);
		assert(pDatabase);

		if (IsCached())
		{
			// Key's from cached entities only maintain one KeySet for the global database
			//
			pDatabase = m_pMetaDatabase->GetGlobalDatabase();
		}
		else
		{
			// The meta database passed in can be different if this request originates
			// from a relation accross databases
			//
			if (pDatabase->GetMetaDatabase() != m_pMetaDatabase)
			{
				pDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(m_pMetaDatabase);
			}
		}

		assert(pDatabase->GetMetaDatabase() == m_pMetaDatabase);

		try
		{
			// Create the object
			//
			pEntity = (EntityPtr) m_pInstanceClass->CreateInstance();
			pEntity->MarkConstructing();
			pEntity->m_pMetaEntity = this;
			pEntity->m_pDatabase = pDatabase;
			pEntity->On_PostCreate();
			pEntity->UnMarkConstructing();
		}
		catch(Exception & e)
		{
			delete pEntity;
			e.AddMessage("MetaEntity::CreateInstance(): failed to create object of %s.", GetFullName().c_str());
			throw;
		}
		catch(...)
		{
			delete pEntity;
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaEntity::CreateInstance(): failed to create object of %s.", GetFullName().c_str());
		}


		return pEntity;
	}



	MetaRelationPtr MetaEntity::GetChildMetaRelation(const std::string & strName)
	{
		for (unsigned int idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
			if (m_vectChildMetaRelation[idx] && m_vectChildMetaRelation[idx]->GetName() == strName)
				return m_vectChildMetaRelation[idx];

		return NULL;
	}



	MetaRelationPtr MetaEntity::GetParentMetaRelation(const std::string & strName)
	{
		for (unsigned int idx = 0; idx < m_vectParentMetaRelation.size(); idx++)
			if (m_vectParentMetaRelation[idx] && m_vectParentMetaRelation[idx]->GetReverseName() == strName)
				return m_vectParentMetaRelation[idx];

		return NULL;
	}



 	MetaColumnPtr MetaEntity::GetAutoNumMetaColumn()
  {
 		for (unsigned int idx = 0; idx < m_vectMetaColumn.size(); idx++)
 			if (m_vectMetaColumn[idx] && m_vectMetaColumn[idx]->IsAutoNum())
    		return m_vectMetaColumn[idx];

    return NULL;
	}



	MetaColumnPtr MetaEntity::GetMetaColumn(const std::string & strColumnName)
	{
		for (unsigned int idx = 0; idx < m_vectMetaColumn.size(); idx++)
			if (strColumnName == m_vectMetaColumn[idx]->GetName())
				return m_vectMetaColumn[idx];

		return NULL;
	}



	MetaKeyPtr MetaEntity::GetMetaKey(const std::string & strKeyName)
	{
		for (unsigned int idx = 0; idx < m_vectMetaKey.size(); idx++)
			if (strKeyName == m_vectMetaKey[idx]->GetName())
				return m_vectMetaKey[idx];

		return NULL;
	}



	bool MetaEntity::IsAssociative()
	{
		// If this test has been perforemed already, the value is 1 or 0
		switch (m_sAssociative)
		{
			case 1: return true;
			case 0: return false;
		}

		// This has not yet been evaluated yet.
		m_sAssociative = 0;

		// To be associative, this must have no child relations
		if (GetChildMetaRelationCount() == 0)
		{
			MetaColumnPtrVect								vectMC(m_vectMetaColumn);
			MetaColumnPtrVect::iterator			itr;
			MetaRelationPtrVect::iterator		itrMR;
			MetaRelationPtr									pMR;
			MetaColumnPtrList::iterator			itrMC;
			MetaColumnPtr										pMC;
			unsigned int										i;

			// Let's start by assuming this is associative
			m_sAssociative = 1;

			// To be associative, all columns must be referencing a parent
			for ( itrMR =  m_vectParentMetaRelation.begin();
						itrMR != m_vectParentMetaRelation.end();
						itrMR++)
			{
				pMR = *itrMR;

				for ( itrMC =  pMR->GetChildMetaKey()->GetMetaColumns()->begin(), i = 0;
							itrMC != pMR->GetChildMetaKey()->GetMetaColumns()->end() &&	i < pMR->GetParentMetaKey()->GetColumnCount();
							itrMC++, i++)
				{
					pMC = *itrMC;
					vectMC[pMC->GetColumnIdx()] = NULL;
				}
			}

			// Now check if all columns in our temporary vector have been set to NULL
			for ( itr =  vectMC.begin();
						itr != vectMC.end();
						itr++)
			{
				if (*itr != NULL)
				{
					m_sAssociative = 0;
					break;
				}
			}
		}

		return m_sAssociative == 1;
	}



	long MetaEntity::LoadAll(DatabasePtr pDB, bool bRefresh, bool bLazyFetch)
	{
		assert(pDB);

		return pDB->LoadObjects(this, bRefresh, bLazyFetch);
	}



	void MetaEntity::DeleteAllObjects(DatabasePtr pDatabase)
	{
		GetPrimaryMetaKey()->DeleteAllObjects(pDatabase);
	}



	//! Debug aid: The method dumps this and all its objects to cout
	void MetaEntity::Dump(int nIndentSize, bool bDeep)
	{
		unsigned int	idx;

		// Dump this
		//
		std::cout << std::setw(nIndentSize) << "";
		std::cout << "MetaEntity " << m_strName << std::endl;
		std::cout << std::setw(nIndentSize) << "";
		std::cout << "{" << std::endl;

		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Name              " << m_strName << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Alias             " << m_strAlias << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "InstanceClass     " << m_pInstanceClass->Name() << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "InstanceInterface " << m_strInstanceInterface << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "ProsaName         " << m_strProsaName << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Description       " << m_strDescription << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Flags             " << m_Flags.AsString() << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Permissions       " << m_Permissions.AsString() << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Associative       " << (IsAssociative() ? "true" : "false") << std::endl;

		// Dump all related meta objects
		//
		if (bDeep)
		{
			std::cout << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "Columns:" << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "{" << std::endl;

			// Dump all MetaColumn objects
			//
			for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
			{
				if (idx > 0)
					std::cout << std::endl;

				m_vectMetaColumn[idx]->Dump(nIndentSize+4, bDeep);
			}

			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "}" << std::endl;

			std::cout << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "Keys:" << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "{" << std::endl;

			// Dump all MetaKey objects
			//
			for (idx = 0; idx < m_vectMetaKey.size(); idx++)
			{
				if (idx > 0)
					std::cout << std::endl;

				m_vectMetaKey[idx]->Dump(nIndentSize+4, bDeep);
			}

			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "}" << std::endl;
		}

		std::cout << std::setw(nIndentSize) << "";
		std::cout << "}" << std::endl;
	}



	bool MetaEntity::AddToCPPHeader(std::ofstream & fout)
	{
		std::string			strTablePfx;
		unsigned int		idx;
		MetaColumnPtr		pMC;
		MetaKeyPtr			pMK;
		MetaRelationPtr	pMR;


		// Lets create an enum for the columns
		//
		strTablePfx  = m_pMetaDatabase->GetAlias();
		strTablePfx += "_";
		strTablePfx += m_strName;

		fout << std::endl;
		fout << "\t//-----------------------------------------------------------------------------" << std::endl;
		fout << "\t// Table: " << m_pMetaDatabase->GetAlias() << '.' << GetName() << std::endl;
		fout << "\t//" << std::endl;

		// Forward decls and typedefs for specialised classes
		//
		if (m_pInstanceClass && m_pInstanceClass != &Entity::ClassObject())
		{
			fout << std::endl;
			fout << "\t// Specialisation: " << std::endl;
			fout << "\t//" << std::endl;
			fout << "\tclass " << m_pInstanceClass->Name() << ";" << std::endl;
			fout << "\ttypedef " << m_pInstanceClass->Name() << "*\t\t" << m_pInstanceClass->Name() << "Ptr;" << std::endl;
		}

		fout << std::endl;
		fout << "\t// Columns: " << std::endl;
		fout << "\t//" << std::endl;
		fout << "\tenum " << strTablePfx << "_Columns" << std::endl;
		fout << "\t{" << std::endl;

		for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
		{
			pMC = m_vectMetaColumn[idx];

			if (idx > 0)
				fout << "," << std::endl;

			fout << "\t\t" << strTablePfx << "_" << pMC->GetName();
		}

		fout << std::endl;
		fout << "\t};" << std::endl;
		fout << std::endl;


		// Lets create an enum for the keys
		//
		if (!m_vectMetaKey.empty())
		{
			fout << std::endl;
			fout << "\t// Keys: " << std::endl;
			fout << "\t//" << std::endl;
			fout << "\tenum " << strTablePfx << "_Keys" << std::endl;
			fout << "\t{" << std::endl;

			for (idx = 0; idx < m_vectMetaKey.size(); idx++)
			{
				pMK = m_vectMetaKey[idx];

				if (idx > 0)
					fout << "," << std::endl;

				fout << "\t\t" << strTablePfx << "_" << pMK->GetName();
			}

			fout << std::endl;
			fout << "\t};" << std::endl;
			fout << std::endl;
		}


		// Lets create an enum for parent relations
		//
		if (!m_vectParentMetaRelation.empty())
		{
			fout << std::endl;
			fout << "\t// Parent relations: " << std::endl;
			fout << "\t//" << std::endl;
			fout << "\tenum " << strTablePfx << "_ParentRelations" << std::endl;
			fout << "\t{" << std::endl;

			for (idx = 0; idx < m_vectParentMetaRelation.size(); idx++)
			{
				pMR = m_vectParentMetaRelation[idx];

				if (idx > 0)
					fout << "," << std::endl;

				fout << "\t\t" << strTablePfx << "_" << "PR_" << pMR->GetUniqueReverseName();
			}

			fout << std::endl;
			fout << "\t};" << std::endl;
			fout << std::endl;
		}


		// Lets create an enum for child relations
		//
		if (!m_vectChildMetaRelation.empty())
		{
			fout << std::endl;
			fout << "\t// Child relations: " << std::endl;
			fout << "\t//" << std::endl;
			fout << "\tenum " << strTablePfx << "_ChildRelations" << std::endl;
			fout << "\t{" << std::endl;

			for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
			{
				pMR = m_vectChildMetaRelation[idx];

				if (idx > 0)
					fout << "," << std::endl;

				fout << "\t\t" << strTablePfx << "_" << "CR_" << pMR->GetUniqueName();
			}

			fout << std::endl;
			fout << "\t};" << std::endl;
			fout << std::endl;
		}


		return true;
	}



	bool MetaEntity::CreateSpecialisedCPPHeader()
	{
		std::ofstream						fout;
		unsigned int						idx, idx1;
		std::string							strFileName;
		std::string							strClassName;
		std::string							strBaseClassName;
		std::string							strChildClass;
		std::string							strChildName;
		MetaDatabasePtrMapItr		itr;
		MetaDatabasePtr					pMDB;
		MetaColumnPtr						pMC;
		MetaKeyPtr							pMK;
		MetaRelationPtr					pMR;
		MetaColumnPtrListItr		itrKeyCol;
		int											iWidth;
		bool										bNeedDatabase;


		// Create the names we need
		//
		if (m_pInstanceClass != &(Entity::ClassObject()))
			strClassName			= m_pInstanceClass->Name();
		else
			strClassName			= GetName();

		strBaseClassName	= strClassName + "Base";
		strFileName				= strBaseClassName + ".h";

		// Open the Header file
		//
		fout.open((std::string("Generated/") + strFileName).c_str(), std::ios::out | std::ios::trunc);

		if (fout.fail())
		{
			ReportError("MetaEntity::CreateSpecialisedCPPHeader(): Failed to open/create header file %s.", strFileName.c_str());
			return false;
		}

		// Generate include blockers and standard includes
		//
		fout << "#ifndef INC_" << strBaseClassName << "_H" << std::endl;
		fout << "#define INC_" << strBaseClassName << "_H" << std::endl;
		fout << std::endl;
		fout << "// WARNING: This file has been generated by D3. You should NEVER change this file." << std::endl;
		fout << "//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()" << std::endl;
		fout << "//          which explains in detail how you can gegenerate this file." << std::endl;
		fout << "//" << std::endl;
		fout << "// The file declares methods which simplify client interactions with objects of type " << std::endl;
		fout << "// " << strClassName << " representing instances of " << GetMetaDatabase()->GetAlias() << "." << GetName() << "." << std::endl;
		fout << "//" << std::endl;
		fout << "//       For D3 to work correctly, you must implement your own class as follows:" << std::endl;
		fout << "//" << std::endl;
		fout << "//       #include \"" << strFileName << "\"" << std::endl;
		fout << "//" << std::endl;
		fout << "//       namespace D3" << std::endl;
		fout << "//       {" << std::endl;
		fout << "//         class " << strClassName << " : public " << strClassName << "Base" << std::endl;
		fout << "//         {" << std::endl;
		fout << "//           D3_CLASS_DECL(" << strClassName << ");" << std::endl;
		fout << "//" << std::endl;
		fout << "//           protected:" << std::endl;
		fout << "//             " << strClassName << "() {}" << std::endl;
		fout << "//" << std::endl;
		fout << "//           public:" << std::endl;
		fout << "//             ~" << strClassName << "() {}" << std::endl;
		fout << "//" << std::endl;
		fout << "//             // Insert your specialised member functions here..." << std::endl;
		fout << "//" << std::endl;
		fout << "//         };" << std::endl;
		fout << "//       } // end namespace D3" << std::endl;
		fout << "//" << std::endl;
		fout << std::endl;
		fout << "#include \"Entity.h\""		<< std::endl;
		fout << "#include \"Column.h\""		<< std::endl;
		fout << "#include \"Key.h\""			<< std::endl;
		fout << "#include \"Relation.h\"" << std::endl;

		if (m_pMetaDatabase->IsMetaDictionary())
		{
			fout << "#include \"" << m_pMetaDatabase->GetAlias() << ".h\"" << std::endl;
		}
		else
		{
			// Firstly, include the header for the meta database this belongs to
			fout << "#include \"" << GetMetaDatabase()->GetAlias() << ".h\"" << std::endl;

			// We need to check for cross database relations and include such headers also
			std::set<MetaDatabasePtr>							setOtherDBs;
			std::set<MetaDatabasePtr>::iterator		itrOtherDBs;

			for (idx = 0; idx < m_vectParentMetaRelation.size(); idx++)
			{
				pMR = m_vectParentMetaRelation[idx];
				pMDB = pMR->GetParentMetaKey()->GetMetaEntity()->GetMetaDatabase();

				if (pMDB != GetMetaDatabase())
				{
					if (setOtherDBs.find(pMDB) == setOtherDBs.end())
						setOtherDBs.insert(pMDB);
				}
			}

			for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
			{
				pMR = m_vectChildMetaRelation[idx];
				pMDB = pMR->GetChildMetaKey()->GetMetaEntity()->GetMetaDatabase();

				if (pMDB != GetMetaDatabase())
				{
					if (setOtherDBs.find(pMDB) == setOtherDBs.end())
						setOtherDBs.insert(pMDB);
				}
			}

			for ( itrOtherDBs =  setOtherDBs.begin();
						itrOtherDBs != setOtherDBs.end();
						itrOtherDBs++)
			{
				pMDB = *itrOtherDBs;
				fout << "#include \"" << pMDB->GetAlias() << ".h\"" << std::endl;
			}
		}

		// Everything is D3 namespaced
		//
		fout << std::endl;
		fout << "namespace D3" << std::endl;
		fout << "{" << std::endl;
		fout << std::endl;

		// Create easy column accessor enum
		//
		fout << "\t" << "//! Use these enums to access columns through " << strClassName << "::Column() method" << std::endl;
		fout << "\tenum " << strClassName << "_Fields" << std::endl;
		fout << "\t{" << std::endl;

		for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
		{
			pMC = m_vectMetaColumn[idx];
			if (idx > 0) fout << "," << std::endl;
			fout << "\t\t" << strClassName << "_" << pMC->GetName();
		}

		if (idx > 0)
			fout << std::endl;

		fout << "\t};" << std::endl;
		fout << std::endl;
		fout << std::endl;

		// Now write the class
		//
		fout << "\t//! " << strBaseClassName << " is a base class that \\b MUST be subclassed through a class called \\a " << strClassName << "." << std::endl;
		fout << "\t/*! " << "The purpose of this class to provide more natural access to related objects as well as this' columns." << std::endl;

		fout << "\t\t\t" << "This class is only usefull if it is subclassed by a class called " << strClassName << std::endl;
		fout << "\t\t\t" << "Equally important is that the meta dictionary knows of the existence of your subclass as well as" << std::endl;
		fout << "\t\t\t" << "specialised Relation classes implemented herein. Only once these details have been added to the" << std::endl;
		fout << "\t\t\t" << "dictionary will D3 instantiate objects of type \\a " << strClassName << " representing rows from the table \\a " << GetMetaDatabase()->GetAlias() << "." << GetName() << "." << std::endl;
		fout << "\t*/" << std::endl;
		fout << "\tclass D3_API " << strBaseClassName << ": public Entity" << std::endl;
		fout << "\t{" << std::endl;
		fout << "\t\tD3_CLASS_DECL(" << strBaseClassName << ");" << std::endl;
		fout << std::endl;
		fout << "\t\tpublic:" << std::endl;

		// Write static iterator support
		//
		fout << "\t\t\t//! Enable iterating over all instances of this" << std::endl;
		fout << "\t\t\tclass D3_API iterator : public InstanceKeyPtrSetItr" << std::endl;
		fout << "\t\t\t{" << std::endl;
		fout << "\t\t\t\tpublic:" << std::endl;
		fout << "\t\t\t\t\titerator() {}" << std::endl;
		fout << "\t\t\t\t\titerator(const InstanceKeyPtrSetItr& itr) : InstanceKeyPtrSetItr(itr) {}" << std::endl;
		fout << std::endl;

		fout << "\t\t\t\t\t//! De-reference operator*()" << std::endl;
		fout << "\t\t\t\t\tvirtual " << strClassName << "Ptr";
		iWidth = 32 - (strClassName.size() + 11);
		if (iWidth > 0) fout << std::setw(iWidth) << ""; else fout << " ";
		fout << "operator*();" << std::endl;

		fout << "\t\t\t\t\t//! Assignment operator=()" << std::endl;
		fout << "\t\t\t\t\t" << "virtual iterator&";
		fout << std::setw(15) << "";
		fout << "operator=(const iterator& itr);" << std::endl;
		fout << "\t\t\t};" << std::endl;
		fout << std::endl;

		fout << "\t\t\tstatic unsigned int";
		fout << std::setw(17) << "";
		fout << "size(DatabasePtr pDB)";
		fout << std::setw(9) << "";
		fout << "{ return GetAll(pDB)->size(); }" << std::endl;

		fout << "\t\t\tstatic bool";
		fout << std::setw(25) << "";
		fout << "empty(DatabasePtr pDB)";
		fout << std::setw(8) << "";
		fout << "{ return GetAll(pDB)->empty(); }" << std::endl;

		fout << "\t\t\tstatic iterator";
		fout << std::setw(21) << "";
		fout << "begin(DatabasePtr pDB)";
		fout << std::setw(8) << "";
		fout << "{ return iterator(GetAll(pDB)->begin()); }" << std::endl;

		fout << "\t\t\tstatic iterator";
		fout << std::setw(21) << "";
		fout << "end(DatabasePtr pDB)";
		fout << std::setw(10) << "";
		fout << "{ return iterator(GetAll(pDB)->end()); }" << std::endl;

		fout << std::endl;

		// Write an iterator class for each child relation
		//
		for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
		{
			pMR = m_vectChildMetaRelation[idx];

			// Only generate generate this for the first child relation with this name
			//
			for (idx1 = 0; idx1 < m_vectChildMetaRelation.size(); idx1++)
			{
				if (m_vectChildMetaRelation[idx1]->GetName() == pMR->GetName())
					break;
			}

			if (idx1 < m_vectChildMetaRelation.size() && m_vectChildMetaRelation[idx1] != pMR)
				continue;

			if (pMR->GetChildMetaKey()->GetMetaEntity()->m_pInstanceClass != &(Entity::ClassObject()))
				strChildClass = pMR->GetChildMetaKey()->GetMetaEntity()->m_pInstanceClass->Name();
			else
				strChildClass = pMR->GetChildMetaKey()->GetMetaEntity()->GetName();

			strChildName = pMR->GetChildMetaKey()->GetMetaEntity()->GetName();

			fout << "\t\t\t//! Enable iterating the relation " << pMR->GetName() << " to access related " << strChildName << " objects" << std::endl;
			fout << "\t\t\tclass D3_API " << pMR->GetName() << " : public Relation" << std::endl;
			fout << "\t\t\t{" << std::endl;
			fout << "\t\t\t\tD3_CLASS_DECL(" << pMR->GetName() << ");" << std::endl;
			fout << std::endl;
			fout << "\t\t\t\tprotected:" << std::endl;
			fout << "\t\t\t\t\t" << pMR->GetName() << "() {}" << std::endl;
			fout << std::endl;
			fout << "\t\t\t\tpublic:" << std::endl;
			fout << "\t\t\t\t\tclass D3_API iterator : public Relation::iterator" << std::endl;
			fout << "\t\t\t\t\t{" << std::endl;
			fout << "\t\t\t\t\t\tpublic:" << std::endl;
			fout << "\t\t\t\t\t\t\titerator() {}" << std::endl;
			fout << "\t\t\t\t\t\t\titerator(const InstanceKeyPtrSetItr& itr) : Relation::iterator(itr) {}" << std::endl;
			fout << std::endl;

			iWidth = 28 - (strChildClass.size() + 11);
			fout << "\t\t\t\t\t\t\t//! De-reference operator*()" << std::endl;
			fout << "\t\t\t\t\t\t\tvirtual " << strChildClass << "Ptr";
			if (iWidth > 0) fout << std::setw(iWidth) << ""; else fout << " ";
			fout << "operator*();" << std::endl;

			fout << "\t\t\t\t\t\t\t//! Assignment operator=()" << std::endl;
			fout << "\t\t\t\t\t\t\tvirtual iterator&";
			fout << std::setw(11) << "";
			fout << "operator=(const iterator& itr);" << std::endl;
			fout << "\t\t\t\t\t};" << std::endl;
			fout << std::endl;

			iWidth = 32 - (strChildClass.size() + 11);
			fout << "\t\t\t\t\t//! front() method" << std::endl;
			fout << "\t\t\t\t\tvirtual " << strChildClass << "Ptr";
			if (iWidth > 0) fout << std::setw(iWidth) << ""; else fout << " ";
			fout << "front();" << std::endl;

			fout << "\t\t\t\t\t//! back() method" << std::endl;
			fout << "\t\t\t\t\tvirtual " << strChildClass << "Ptr";
			if (iWidth > 0) fout << std::setw(iWidth) << ""; else fout << " ";
			fout << "back();" << std::endl;

			fout << "\t\t\t};" << std::endl;
			fout << std::endl;
		}

		fout << std::endl;
		fout << std::endl;

		// Declare the actual class
		//
		fout << "\t\tprotected:" << std::endl;
		fout << "\t\t\t" << strBaseClassName << "() {}" << std::endl;
		fout << std::endl;

		fout << "\t\tpublic:" << std::endl;
		fout << "\t\t\t~" << strBaseClassName << "() {}" << std::endl;
		fout << std::endl;

		fout << "\t\t\t//! Create a new " << strClassName << std::endl;
		fout << "\t\t\tstatic " << strClassName << "Ptr";
		iWidth = 36 - (10 + strClassName.size());
		if (iWidth > 0) fout << std::setw(iWidth) << ""; else fout << " ";
		fout << "Create" << strClassName << "(DatabasePtr pDB)\t\t{ return (" << strClassName << "Ptr) pDB->GetMetaDatabase()->GetMetaEntity(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << ")->CreateInstance(pDB); }" << std::endl;
		fout << std::endl;

		fout << "\t\t\t//! Return a collection of all instances of this" << std::endl;
		fout << "\t\t\tstatic InstanceKeyPtrSetPtr\t\t\t\t\tGetAll(DatabasePtr pDB);" << std::endl;
		fout << std::endl;

		fout << "\t\t\t//! Load all instances of this" << std::endl;
		fout << "\t\t\tstatic void\t\t\t\t\t\t\t\t\t\t\t\t\tLoadAll(DatabasePtr pDB, bool bRefresh = false, bool bLazyFetch = true);" << std::endl;
		fout << std::endl;

		fout << "\t\t\t//! Load a particular instance of this" << std::endl;
		fout << "\t\t\tstatic " << strClassName << "Ptr";
		iWidth = 36 - (10 + strClassName.size());
		if (iWidth > 0) fout << std::setw(iWidth) << ""; else fout << " ";
		fout << "Load(DatabasePtr pDB";

		pMK = GetPrimaryMetaKey();

		for ( itrKeyCol =  pMK->GetMetaColumns()->begin();
					itrKeyCol != pMK->GetMetaColumns()->end();
					itrKeyCol++)
		{
			pMC = *itrKeyCol;

			fout << ", ";

			switch (pMC->GetType())
			{
				case MetaColumn::dbfString:
				{
					fout << "const std::string& str" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfChar:
				{
					fout << "char c" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfShort:

				{
					fout << "short s" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfBool:
				{
					fout << "bool b" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfInt:
				{
					fout << "int i" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfLong:
				{
					fout << "long l" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfFloat:
				{
					fout << "float f" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfDate:
				{
					fout << "const D3Date& d" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfBlob:
				{
					fout << "const std::stringstream& strm" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfBinary:
				{
					fout << "const Data& b" << pMC->GetName();
					break;
				}
			}
		}

		fout << ", bool bRefresh = false, bool bLazyFetch = true);" << std::endl;
		fout << std::endl;



		// Write parent relation accessors
		//
		for (idx = 0; idx < m_vectParentMetaRelation.size(); idx++)
		{
			pMR = m_vectParentMetaRelation[idx];

			// Only generate generate this for the first child relation with this name
			//
			for (idx1 = 0; idx1 < m_vectParentMetaRelation.size(); idx1++)
			{
				if (m_vectParentMetaRelation[idx1]->GetReverseName() == pMR->GetReverseName())
					break;
			}

			if (idx1 < m_vectParentMetaRelation.size() && m_vectParentMetaRelation[idx1] != pMR)
				continue;

			// Determine if this relation requires a database parameter to access the childs parent
			//
			if (!pMR->GetParentMetaKey()->GetMetaEntity()->IsCached() &&
					 pMR->GetChildMetaKey()->GetMetaEntity()->IsCached())
			{
				bNeedDatabase = true;
			}
			else
			{
				bNeedDatabase = false;
			}

			if (pMR->GetParentMetaKey()->GetMetaEntity()->m_pInstanceClass != &(Entity::ClassObject()))
				strChildClass = pMR->GetParentMetaKey()->GetMetaEntity()->m_pInstanceClass->Name();
			else
				strChildClass = pMR->GetParentMetaKey()->GetMetaEntity()->GetName();

			strChildName = pMR->GetParentMetaKey()->GetMetaEntity()->GetName();

			fout << "\t\t\t//! Get related parent " << strChildClass << " object" << std::endl;
			fout << "\t\t\tvirtual " << strChildClass << "Ptr";
			iWidth = 36 - (strChildClass.size() + 11);
			if (iWidth > 0) fout << std::setw(iWidth) << ""; else fout << " ";

			if (bNeedDatabase)
				fout << "Get" << pMR->GetReverseName() << "(DatabasePtr pDB);" << std::endl;
			else
				fout << "Get" << pMR->GetReverseName() << "();" << std::endl;

			fout << "\t\t\t//! Set related parent " << strChildClass << " object" << std::endl;
			fout << "\t\t\tvirtual void\t\t\t\t\t\t\t\t\t\t\t\tSet" << pMR->GetReverseName() << "(" << strChildClass << "Ptr p" << strChildName << ");" << std::endl;
		}

		fout << std::endl;

		// Write child relation accessors
		//
		for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
		{
			pMR = m_vectChildMetaRelation[idx];

			// Only generate generate this for the first child relation with this name
			//
			for (idx1 = 0; idx1 < m_vectChildMetaRelation.size(); idx1++)
			{
				if (m_vectChildMetaRelation[idx1]->GetName() == pMR->GetName())
					break;
			}

			if (idx1 < m_vectChildMetaRelation.size() && m_vectChildMetaRelation[idx1] != pMR)
				continue;

			// Determine if this relation requires a database parameter to access the childs parent
			//
			if ( pMR->GetParentMetaKey()->GetMetaEntity()->IsCached() &&
					!pMR->GetChildMetaKey()->GetMetaEntity()->IsCached())
			{
				bNeedDatabase = true;
			}
			else
			{
				bNeedDatabase = false;
			}

			if (pMR->GetChildMetaKey()->GetMetaEntity()->m_pInstanceClass != &(Entity::ClassObject()))
				strChildClass = pMR->GetChildMetaKey()->GetMetaEntity()->m_pInstanceClass->Name();
			else
				strChildClass = pMR->GetChildMetaKey()->GetMetaEntity()->GetName();

			strChildName = pMR->GetChildMetaKey()->GetMetaEntity()->GetName();

			// Create a LoadAll relations method
			//
			fout << "\t\t\t//! Load all " << pMR->GetName() << " objects. The objects loaded are of type " << strChildClass << "." << std::endl;

			if (bNeedDatabase)
				fout << "\t\t\tvirtual void\t\t\t\t\t\t\t\t\t\t\t\tLoadAll" << pMR->GetName() << "(DatabasePtr pDB, bool bRefresh = false, bool bLazyFetch = true);" << std::endl;
			else
				fout << "\t\t\tvirtual void\t\t\t\t\t\t\t\t\t\t\t\tLoadAll" << pMR->GetName() << "(bool bRefresh = false, bool bLazyFetch = true);" << std::endl;

			// Create code that returns a specific relation
			//
			fout << "\t\t\t//! Get the relation " << pMR->GetName() << " collection which contains objects of type " << strChildClass << "." << std::endl;

			// PH: The true block looks useless so I added "false && "
			if (false && pMR->GetChildMetaKey()->GetMetaEntity()->m_pInstanceClass == &(Entity::ClassObject()))
			{
				if (bNeedDatabase)
					fout << "\t\t\tvirtual RelationPtr\t\t\t\t\t\t\t\tGet" << pMR->GetName() << "(DatabasePtr pDB);" << std::endl;
				else
					fout << "\t\t\tvirtual RelationPtr\t\t\t\t\t\t\t\tGet" << pMR->GetName() << "();" << std::endl;
			}
			else
			{
				fout << "\t\t\tvirtual " << pMR->GetName() << "*";
				iWidth = 36 - (pMR->GetName().size() + 9);
				if (iWidth > 0) fout << std::setw(iWidth) << ""; else fout << " ";


				if (bNeedDatabase)
				{
					fout << "Get" << pMR->GetName() << "(DatabasePtr pDB);" << std::endl;
				}
				else
				{
					fout << "Get" << pMR->GetName() << "();" << std::endl;
				}
			}
		}

		fout << std::endl;

		// Generate a simplified GetValue method for each column
		//
		fout << "\t\t\t/*! @name Get Column Values" << std::endl;
		fout << "\t\t\t    \\note These accessors do not throw even if the column's value is NULL." <<  std::endl;
		fout << "\t\t\t           Therefore, you should use these methods only if you're sure the" <<  std::endl;
		fout << "\t\t\t           column's value is NOT NULL before using these." <<  std::endl;
		fout << "\t\t\t*/" <<  std::endl;
		fout << "\t\t\t//@{" <<  std::endl;

		for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
		{
			pMC = m_vectMetaColumn[idx];

			// Write a little comment
			//
			fout << "\t\t\t//! " << pMC->GetName() << std::endl;

			switch (pMC->GetType())
			{
				case MetaColumn::dbfString:
				{
					fout << "\t\t\tvirtual const std::string&";
					fout << std::setw(10) << "";
					break;
				}
				case MetaColumn::dbfChar:
				{
					fout << "\t\t\tvirtual char";
					fout << std::setw(24) << "";
					break;
				}
				case MetaColumn::dbfShort:
				{
					fout << "\t\t\tvirtual short";
					fout << std::setw(23) << "";
					break;
				}
				case MetaColumn::dbfBool:
				{
					fout << "\t\t\tvirtual bool";
					fout << std::setw(24) << "";
					break;
				}
				case MetaColumn::dbfInt:
				{
					fout << "\t\t\tvirtual int";
					fout << std::setw(25) << "";
					break;
				}
				case MetaColumn::dbfLong:
				{
					fout << "\t\t\tvirtual long";
					fout << std::setw(24) << "";
					break;
				}
				case MetaColumn::dbfFloat:
				{
					fout << "\t\t\tvirtual float";
					fout << std::setw(23) << "";
					break;
				}
				case MetaColumn::dbfDate:
				{
					fout << "\t\t\tvirtual const D3Date&";
					fout << std::setw(15) << "";
					break;
				}
				case MetaColumn::dbfBlob:
				{
					fout << "\t\t\tvirtual const std::stringstream&";
					fout << std::setw(4) << "";
					break;
				}
				case MetaColumn::dbfBinary:
				{
					fout << "\t\t\tvirtual const Data&";
					fout << std::setw(17) << "";
					break;
				}
			}

			fout << "Get" << pMC->GetName() << "()";
			iWidth = 31 - (pMC->GetName().size() + 5);
			if (iWidth > 0) fout << std::setw(iWidth) << "";

			fout << "{ return Column(" << GetName() << "_" << pMC->GetName() << ")->Get";

			switch (pMC->GetType())
			{
				case MetaColumn::dbfString:
				{
					fout << "String(); }" << std::endl;
					break;
				}
				case MetaColumn::dbfChar:
				{
					fout << "Char(); }" << std::endl;
					break;
				}
				case MetaColumn::dbfShort:
				{
					fout << "Short(); }" << std::endl;
					break;
				}
				case MetaColumn::dbfBool:
				{
					fout << "Bool(); }" << std::endl;
					break;
				}
				case MetaColumn::dbfInt:
				{
					fout << "Int(); }" << std::endl;
					break;
				}
				case MetaColumn::dbfLong:
				{
					fout << "Long(); }" << std::endl;
					break;
				}
				case MetaColumn::dbfFloat:
				{
					fout << "Float(); }" << std::endl;

					break;
				}
				case MetaColumn::dbfDate:
				{
					fout << "Date(); }" << std::endl;
					break;
				}
				case MetaColumn::dbfBlob:
				{
					fout << "Blob(); }" << std::endl;
					break;
				}
				case MetaColumn::dbfBinary:
				{
					fout << "Data(); }" << std::endl;
					break;
				}
			}
		}

		fout << "\t\t\t//@}" <<  std::endl;
		fout << std::endl;


		// Generate a simplified SetValue method for each column
		//
		fout << "\t\t\t/*! @name Set Column Values" << std::endl;
		fout << "\t\t\t*/" <<  std::endl;
		fout << "\t\t\t//@{" <<  std::endl;

		for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
		{
			pMC = m_vectMetaColumn[idx];

			// Write a little comment
			//
			fout << "\t\t\t//! Set " << pMC->GetName() << std::endl;


			fout << "\t\t\tvirtual bool\t\t\t\t\t\t\t\t\t\t\t\tSet" << pMC->GetName() << "(";

			iWidth = 31 - (pMC->GetName().size() + 4);

			switch (pMC->GetType())
			{
				case MetaColumn::dbfString:
				{
					fout << "const std::string& val)";
					iWidth -= 23;
					if (iWidth > 0) fout << std::setw(iWidth) << "";
					break;
				}
				case MetaColumn::dbfChar:
				{
					fout << "char val)";
					iWidth -= 9;
					if (iWidth > 0) fout << std::setw(iWidth) << "";
					break;
				}
				case MetaColumn::dbfShort:
				{
					fout << "short val)";
					iWidth -= 10;
					if (iWidth > 0) fout << std::setw(iWidth) << "";
					break;
				}
				case MetaColumn::dbfBool:
				{
					fout << "bool val)";
					iWidth -= 9;
					if (iWidth > 0) fout << std::setw(iWidth) << "";
					break;
				}
				case MetaColumn::dbfInt:
				{
					fout << "int val)";
					iWidth -= 8;
					if (iWidth > 0) fout << std::setw(iWidth) << "";
					break;
				}
				case MetaColumn::dbfLong:
				{
					fout << "long val)";
					iWidth -= 9;
					if (iWidth > 0) fout << std::setw(iWidth) << "";
					break;
				}
				case MetaColumn::dbfFloat:
				{
					fout << "float val)";
					iWidth -= 10;
					if (iWidth > 0) fout << std::setw(iWidth) << "";
					break;
				}
				case MetaColumn::dbfDate:
				{
					fout << "const D3Date& val)";
					iWidth -= 18;
					if (iWidth > 0) fout << std::setw(iWidth) << "";
					break;
				}
				case MetaColumn::dbfBlob:
				{
					fout << "const std::stringstream& val)";
					iWidth -= 29;
					if (iWidth > 0) fout << std::setw(iWidth) << "";
					break;
				}
				case MetaColumn::dbfBinary:
				{
					fout << "const Data& val)";
					iWidth -= 16;
					if (iWidth > 0) fout << std::setw(iWidth) << "";
					break;
				}
			}

			fout << "{ return Column(" << GetName() << "_" << pMC->GetName() << ")->SetValue(val); }" << std::endl;
		}

		fout << "\t\t\t//@}" <<  std::endl;
		fout << std::endl;


		// Create simpliefied Column object accessor
		//
		fout << "\t\t\t//! A column accessor provided mainly for backwards compatibility" << std::endl;
		fout << "\t\t\tvirtual ColumnPtr\t\t\t\t\t\t\t\t\t\tColumn(" << GetName() << "_Fields eCol)";
		iWidth = 36 - (GetName().size() + 20);
		if (iWidth > 0) fout << std::setw(iWidth);
		fout << "{ return Entity::GetColumn((unsigned int) eCol); }" << std::endl;

		fout << "\t};" << std::endl;

		fout << std::endl;
		fout << std::endl;
		fout << std::endl;
		fout << "} // end namespace D3" << std::endl;

		fout << std::endl;
		fout << std::endl;
		fout << "#endif /* INC_" << strBaseClassName << "_H */" << std::endl;

		fout.close();


		return true;
	}



	bool MetaEntity::CreateSpecialisedCPPSource()
	{
		std::ofstream										fout;
		unsigned int										idx;
		std::string											strFileName;
		std::string											strClassName;
		std::string											strBaseClassName;
		std::string											strChildClass;
		std::string											strChildName;
		MetaColumnPtr										pMC;
		MetaKeyPtr											pMK;
		MetaRelationPtr									pMR;
		MetaColumnPtrListItr						itrKeyCol;
		std::set<std::string>						setInclude;
		std::set<std::string>::iterator	itrInclude;
		bool														bNeedDatabase;
		MetaRelationPtrList							listSynonymMR;
		MetaRelationPtrListItr					itrSynonymMR;
		MetaRelationPtr									pSynonymMR;


		// Create the names we need
		//
		if (m_pInstanceClass != &(Entity::ClassObject()))
			strClassName			= m_pInstanceClass->Name();
		else
			strClassName			= GetName();

		strBaseClassName	= strClassName + "Base";
		strFileName				= strBaseClassName + ".cpp";

		// Open the Header file
		//
		fout.open((std::string("Generated/") + strFileName).c_str(), std::ios::out | std::ios::trunc);

		if (fout.fail())
		{
			ReportError("MetaEntity::CreateSpecialisedCPPSource(): Failed to open/create source file %s.", strFileName.c_str());
			return false;
		}

		fout << "// MODULE: " << strFileName << std::endl;
		fout << "//;" << std::endl;
		fout << "//; IMPLEMENTATION CLASS: " << strBaseClassName << std::endl;
		fout << "//;" << std::endl;
		fout << std::endl;
		fout << "// WARNING: This file has been generated by D3. You should NEVER change this file." << std::endl;
		fout << "//          Please refer to the documentation of MetaEntity::CreateSpecialisedCPPHeader()" << std::endl;
		fout << "//          which explains in detail how you can regenerate this file." << std::endl;
		fout << "//" << std::endl;
		fout << std::endl;
		fout << "#include \"D3Types.h\"" << std::endl;
		fout << std::endl;
		fout << "#include \"" << strBaseClassName << ".h\"" << std::endl;
		fout << std::endl;

		// Determine additional includes needed by this
		//
		for (idx = 0; idx < m_vectParentMetaRelation.size(); idx++)
		{
			pMR = m_vectParentMetaRelation[idx];

			if (pMR->GetParentMetaKey()->GetMetaEntity()->m_pInstanceClass != &(Entity::ClassObject()))
				strChildClass = pMR->GetParentMetaKey()->GetMetaEntity()->m_pInstanceClass->Name();
			else
				strChildClass = pMR->GetParentMetaKey()->GetMetaEntity()->GetName();

			if (strChildClass == "Entity")
				continue;

			if (setInclude.find(strChildClass) == setInclude.end())
				setInclude.insert(strChildClass);
		}

		for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
		{
			pMR = m_vectChildMetaRelation[idx];

			if (pMR->GetChildMetaKey()->GetMetaEntity()->m_pInstanceClass != &(Entity::ClassObject()))
				strChildClass = pMR->GetChildMetaKey()->GetMetaEntity()->m_pInstanceClass->Name();
			else
				strChildClass = pMR->GetChildMetaKey()->GetMetaEntity()->GetName();

			if (strChildClass == "Entity")
				continue;

			if (setInclude.find(strChildClass) == setInclude.end())
				setInclude.insert(strChildClass);
		}

		if (!setInclude.empty())
		{
			fout << "// Dependent includes" << std::endl;
			fout << "//" << std::endl;

			for ( itrInclude =  setInclude.begin();
						itrInclude != setInclude.end();
						itrInclude++)
			{
				fout << "#include \"" << *itrInclude << "Base.h\"" << std::endl;
			}

			fout << std::endl;
		}

		fout << std::endl;
		fout << "namespace D3" << std::endl;
		fout << "{" << std::endl;
		fout << std::endl;
		fout << "\t// =====================================================================================" << std::endl;
		fout << "\t// Class " << strBaseClassName << " Implementation" << std::endl;
		fout << "\t//" << std::endl;
		fout << std::endl;
		fout << "\t// D3 class factory support for this class" << std::endl;
		fout << "\t//" << std::endl;
		fout << "\tD3_CLASS_IMPL(" << strBaseClassName << ", Entity);" << std::endl;
		fout << std::endl;
		fout << std::endl;

		// Write GetAll() method
		//
		fout << "\t// Get a collection reflecting all currently resident instances of this" << std::endl;
		fout << "\t//" << std::endl;
		fout << "\t/* static */" << std::endl;
		fout << "\tInstanceKeyPtrSetPtr " << strBaseClassName << "::GetAll(DatabasePtr pDB)" << std::endl;
		fout << "\t{" << std::endl;
		fout << "\t\tDatabasePtr\t\tpDatabase = pDB;" << std::endl;
		fout << std::endl;
		fout << std::endl;
		fout << "\t\tif (!pDatabase)" << std::endl;
		fout << "\t\t\treturn NULL;" << std::endl;
		fout << std::endl;
		fout << "\t\tif (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase(\"" << m_pMetaDatabase->GetAlias() << "\"))" << std::endl;
		fout << "\t\t\tpDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase(\"" << m_pMetaDatabase->GetAlias() << "\"));" << std::endl;
		fout << std::endl;
		fout << "\t\tif (!pDatabase)" << std::endl;
		fout << "\t\t\treturn NULL;" << std::endl;
		fout << std::endl;
		fout << "\t\treturn pDatabase->GetMetaDatabase()->GetMetaEntity(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << ")->GetPrimaryMetaKey()->GetInstanceKeySet(pDatabase);" << std::endl;
		fout << "\t}" << std::endl;
		fout << std::endl;
		fout << std::endl;
		fout << std::endl;

		// Write LoadAll() method
		//
		fout << "\t// Make all objects of this type memory resident" << std::endl;
		fout << "\t//" << std::endl;
		fout << "\t/* static */" << std::endl;
		fout << "\tvoid " << strBaseClassName << "::LoadAll(DatabasePtr pDB, bool bRefresh, bool bLazyFetch)" << std::endl;
		fout << "\t{" << std::endl;
		fout << "\t\tDatabasePtr\t\tpDatabase = pDB;" << std::endl;
		fout << std::endl;
		fout << std::endl;
		fout << "\t\tif (!pDatabase)" << std::endl;
		fout << "\t\t\treturn;" << std::endl;
		fout << std::endl;
		fout << "\t\tif (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase(\"" << m_pMetaDatabase->GetAlias() << "\"))" << std::endl;
		fout << "\t\t\tpDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase(\"" << m_pMetaDatabase->GetAlias() << "\"));" << std::endl;
		fout << std::endl;
		fout << "\t\tif (!pDatabase)" << std::endl;
		fout << "\t\t\treturn;" << std::endl;
		fout << std::endl;
		fout << "\t\tpDatabase->GetMetaDatabase()->GetMetaEntity(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << ")->LoadAll(pDatabase, bRefresh, bLazyFetch);" << std::endl;
		fout << "\t}" << std::endl;
		fout << std::endl;
		fout << std::endl;
		fout << std::endl;

		// Write Load() method
		//
		fout << "\t// Make a specific instance resident" << std::endl;
		fout << "\t//" << std::endl;
		fout << "\t/* static */" << std::endl;
		fout << "\t" << strClassName << "Ptr " << strBaseClassName << "::Load(DatabasePtr pDB";

		pMK = GetPrimaryMetaKey();

		for ( itrKeyCol =  pMK->GetMetaColumns()->begin();
					itrKeyCol != pMK->GetMetaColumns()->end();
					itrKeyCol++)
		{
			pMC = *itrKeyCol;

			fout << ", ";

			switch (pMC->GetType())

			{
				case MetaColumn::dbfString:
				{
					fout << "const std::string& str" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfChar:
				{
					fout << "char c" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfShort:
				{
					fout << "short s" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfBool:
				{
					fout << "bool b" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfInt:
				{
					fout << "int i" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfLong:
				{
					fout << "long l" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfFloat:
				{

					fout << "float f" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfDate:
				{
					fout << "const D3Date& d" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfBinary:
				{
					fout << "const Data& b" << pMC->GetName();
					break;
				}
			}
		}

		fout << ", bool bRefresh, bool bLazyFetch)" << std::endl;
		fout << "\t{" << std::endl;
		fout << "\t\tDatabasePtr\t\tpDatabase = pDB;" << std::endl;
		fout << std::endl;
		fout << std::endl;
		fout << "\t\tif (!pDatabase)" << std::endl;
		fout << "\t\t\treturn NULL;" << std::endl;
		fout << std::endl;
		fout << "\t\tif (pDatabase->GetMetaDatabase() != MetaDatabase::GetMetaDatabase(\"" << m_pMetaDatabase->GetAlias() << "\"))" << std::endl;
		fout << "\t\t\tpDatabase = pDatabase->GetDatabaseWorkspace()->GetDatabase(MetaDatabase::GetMetaDatabase(\"" << m_pMetaDatabase->GetAlias() << "\"));" << std::endl;
		fout << std::endl;
		fout << "\t\tif (!pDatabase)" << std::endl;
		fout << "\t\t\treturn NULL;" << std::endl;
		fout << std::endl;
		fout << "\t\tTemporaryKey\ttmpKey(*(pDatabase->GetMetaDatabase()->GetMetaEntity(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << ")->GetPrimaryMetaKey()));" << std::endl;
		fout << std::endl;
		fout << "\t\t// Set all key column values" << std::endl;
		fout << "\t\t//" << std::endl;

		for ( itrKeyCol =  pMK->GetMetaColumns()->begin();
					itrKeyCol != pMK->GetMetaColumns()->end();
					itrKeyCol++)
		{
			pMC = *itrKeyCol;

			fout << "\t\ttmpKey.GetColumn(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_" << pMC->GetName() << ")->SetValue(";

			switch (pMC->GetType())
			{
				case MetaColumn::dbfString:
				{
					fout << "str" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfChar:
				{
					fout << "c" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfShort:
				{
					fout << "s" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfBool:
				{
					fout << "b" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfInt:
				{
					fout << "i" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfLong:
				{
					fout << "l" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfFloat:
				{
					fout << "f" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfDate:
				{
					fout << "d" << pMC->GetName();
					break;
				}
				case MetaColumn::dbfBinary:
				{
					fout << "b" << pMC->GetName();
					break;
				}
			}

			fout << ");" << std::endl;
		}

		fout << std::endl;

		fout << "\t\treturn (" << strClassName << "Ptr) pDatabase->GetMetaDatabase()->GetMetaEntity(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << ")->GetPrimaryMetaKey()->LoadObject(&tmpKey, pDatabase, bRefresh, bLazyFetch);" << std::endl;
		fout << "\t}" << std::endl;
		fout << std::endl;
		fout << std::endl;
		fout << std::endl;

		// Write GetParent and SetParent method
		//
		for (idx = 0; idx < m_vectParentMetaRelation.size(); idx++)
		{
			pMR = m_vectParentMetaRelation[idx];

			// Only generate generate this for the first parent relation with this name
			//
			GetSiblingSwitchedParentRelations(pMR, listSynonymMR);

			if (!listSynonymMR.empty() && listSynonymMR.front() != pMR)
				continue;

			// Determine if this relation requires a database parameter to access the childs parent
			//
			if (!pMR->GetParentMetaKey()->GetMetaEntity()->IsCached() &&
					 pMR->GetChildMetaKey()->GetMetaEntity()->IsCached())
			{
				bNeedDatabase = true;
			}
			else
			{
				bNeedDatabase = false;
			}

			if (pMR->GetParentMetaKey()->GetMetaEntity()->m_pInstanceClass != &(Entity::ClassObject()))
				strChildClass = pMR->GetParentMetaKey()->GetMetaEntity()->m_pInstanceClass->Name();
			else
				strChildClass = pMR->GetParentMetaKey()->GetMetaEntity()->GetName();
			
			strChildName = pMR->GetParentMetaKey()->GetMetaEntity()->GetName();

			// Write GetParent method
			//
			fout << "\t// Get parent of relation " << pMR->GetReverseName() << std::endl;
			fout << "\t//" << std::endl;

			if (bNeedDatabase)
				fout << "\t" << strChildClass << "Ptr " << strBaseClassName << "::Get" << pMR->GetReverseName() << "(DatabasePtr pDB)" << std::endl;
			else
				fout << "\t" << strChildClass << "Ptr " << strBaseClassName << "::Get" << pMR->GetReverseName() << "()" << std::endl;

			fout << "\t{" << std::endl;
			fout << "\t\tRelationPtr\t\tpRelation;" << std::endl;
			fout << std::endl;


			// Build code which retrieves the first non-NULL relation of all feasible relations
			//
			if (listSynonymMR.size() > 1)
			{
				fout << "\t\t// Deal with multiple choices" << std::endl;
				fout << "\t\t//" << std::endl;
				fout << "\t\twhile(true)" << std::endl;
				fout << "\t\t{" << std::endl;

				for ( itrSynonymMR =  listSynonymMR.begin();
							itrSynonymMR != listSynonymMR.end();
							itrSynonymMR++)
				{
					pSynonymMR = *itrSynonymMR;

					fout << "\t\t\t// Try relation " << pSynonymMR->GetName() << "'s " << pSynonymMR->GetReverseName() << std::endl;
					fout << "\t\t\t//" << std::endl;

					// Determine if this relation requires a database parameter to access the childs parent
					//
					if (!pSynonymMR->GetParentMetaKey()->GetMetaEntity()->IsCached() &&
							 pSynonymMR->GetChildMetaKey()->GetMetaEntity()->IsCached())
					{
						fout << "\t\t\tpRelation = GetParentRelation(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_PR_" << pSynonymMR->GetUniqueReverseName() << ", pDB);" << std::endl;
					}
					else
					{
						fout << "\t\t\tpRelation = GetParentRelation(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_PR_" << pSynonymMR->GetUniqueReverseName() << ");" << std::endl;
					}

					fout << std::endl;
					fout << "\t\t\tif (pRelation)" << std::endl;
					fout << "\t\t\t\tbreak;" << std::endl;
					fout << std::endl;
				}

				fout << "\t\t\tbreak;" << std::endl;
				fout << "\t\t}" << std::endl;
			}
			else
			{
				if (bNeedDatabase)
					fout << "\t\tpRelation = GetParentRelation(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_PR_" << pMR->GetReverseName() << ", pDB);" << std::endl;
				else
					fout << "\t\tpRelation = GetParentRelation(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_PR_" << pMR->GetReverseName() << ");" << std::endl;
			}

			fout << std::endl;
			fout << "\t\tif (!pRelation)" << std::endl;
			fout << "\t\t\treturn NULL;" << std::endl;
			fout << std::endl;

			if (strChildClass == "Entity")
				fout << "\t\treturn pRelation->GetParent();" << std::endl;
			else
				fout << "\t\treturn (" << strChildClass << "Ptr) pRelation->GetParent();" << std::endl;

			fout << "\t}" << std::endl;
			fout << std::endl;
			fout << std::endl;
			fout << std::endl;

			// Write SetParent method
			//
			fout << "\t// Set parent of relation " << pMR->GetReverseName() << std::endl;
			fout << "\t//" << std::endl;

			fout << "\tvoid " << strBaseClassName << "::Set" << pMR->GetReverseName() << "(" << strChildClass << "Ptr pParent)" << std::endl;
			fout << "\t{" << std::endl;

			// Build code which set's the parent of the most appropriate relation
			//
			if (listSynonymMR.size() > 1)
			{
				MetaRelationPtr		pBestMR = NULL;

				// Let' see if we have a parent without switched children
				//
				for ( itrSynonymMR =  listSynonymMR.begin();
							itrSynonymMR != listSynonymMR.end();
							itrSynonymMR++)
				{
					pSynonymMR = *itrSynonymMR;

					if (!pSynonymMR->IsChildSwitch())
					{
						pBestMR = pSynonymMR;
						break;
					}
				}

				if (pBestMR)
				{
					fout << "\t\tSetParent(m_pMetaEntity->GetParentMetaRelation(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_PR_" << pBestMR->GetReverseName() << "), (EntityPtr) pParent);" << std::endl;
				}
				else
				{
					fout << "\t\tMetaRelationPtr\t\tpMR;" << std::endl;
					fout << std::endl;
					fout << std::endl;
					fout << "\t\t// Set the parent of the most appropriate relation" << std::endl;
					fout << "\t\t//" << std::endl;
					fout << "\t\twhile(true)" << std::endl;
					fout << "\t\t{" << std::endl;

					for ( itrSynonymMR =  listSynonymMR.begin();
								itrSynonymMR != listSynonymMR.end();
								itrSynonymMR++)
					{
						pSynonymMR = *itrSynonymMR;

						fout << "\t\t\t// Try relation " << pSynonymMR->GetName() << "'s " << pSynonymMR->GetReverseName() << std::endl;
						fout << "\t\t\t//" << std::endl;

						fout << "\t\t\tpMR = m_pMetaEntity->GetParentMetaRelation(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_PR_" << pSynonymMR->GetUniqueReverseName() << ");" << std::endl;

						fout << std::endl;

						fout << "\t\t\tif (*(GetColumn(" << m_pMetaDatabase->GetAlias() << "_" << m_strName << "_" << pSynonymMR->GetSwitchColumn()->GetMetaColumn()->GetName() << ")) == *(pMR->GetSwitchColumn()))" << std::endl;
						fout << "\t\t\t{" << std::endl;
						fout << "\t\t\t\tSetParent(pMR, (EntityPtr) pParent);" << std::endl;
						fout << "\t\t\t\tbreak;" << std::endl;
						fout << "\t\t\t}" << std::endl;

						if (itrSynonymMR != listSynonymMR.end())
							fout << std::endl;
					}

					fout << "\t\t\tthrow Exception(__FILE__, __LINE__, Exception_error, \"" << strBaseClassName << "::Set" << pMR->GetReverseName() << "(): failed to set parent because value '%s' is invalid for switch column '%s'.\", GetColumn(" << m_pMetaDatabase->GetAlias() << "_" << m_strName << "_" << pSynonymMR->GetSwitchColumn()->GetMetaColumn()->GetName() << ")->AsString().c_str(), pMR->GetSwitchColumn()->GetMetaColumn()->GetFullName().c_str());" << std::endl;
					fout << "\t\t}" << std::endl;
				}
			}
			else
			{
				fout << "\t\tSetParent(m_pMetaEntity->GetParentMetaRelation(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_PR_" << pMR->GetReverseName() << "), (EntityPtr) pParent);" << std::endl;
			}

			fout << "\t}" << std::endl;
			fout << std::endl;
			fout << std::endl;
			fout << std::endl;
		}

		// Write LoadAllChildren GetAllChildren methods

		//
		for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
		{
			pMR = m_vectChildMetaRelation[idx];

			// Only generate generate this for the first parent relation with this name
			//
			GetSiblingSwitchedChildRelations(pMR, listSynonymMR);

			if (!listSynonymMR.empty() && listSynonymMR.front() != pMR)
				continue;

			// Determine if this relation requires a database parameter to access the childs parent
			//
			if ( pMR->GetParentMetaKey()->GetMetaEntity()->IsCached() &&
					!pMR->GetChildMetaKey()->GetMetaEntity()->IsCached())
			{
				bNeedDatabase = true;
			}
			else
			{
				bNeedDatabase = false;
			}

			if (pMR->GetChildMetaKey()->GetMetaEntity()->m_pInstanceClass != &(Entity::ClassObject()))
				strChildClass = pMR->GetChildMetaKey()->GetMetaEntity()->m_pInstanceClass->Name();
			else
				strChildClass = pMR->GetChildMetaKey()->GetMetaEntity()->GetName();

			strChildName = pMR->GetChildMetaKey()->GetMetaEntity()->GetName();

			// Write LoadAll method
			//
			fout << "\t// Load all instances of relation " << pMR->GetName() << std::endl;
			fout << "\t//" << std::endl;

			if (bNeedDatabase)
				fout << "\tvoid " << strBaseClassName << "::LoadAll" << pMR->GetName() << "(DatabasePtr pDB, bool bRefresh, bool bLazyFetch)" << std::endl;
			else
				fout << "\tvoid " << strBaseClassName << "::LoadAll" << pMR->GetName() << "(bool bRefresh, bool bLazyFetch)" << std::endl;

			fout << "\t{" << std::endl;
			fout << "\t\tRelationPtr\t\tpRelation = NULL;" << std::endl;
			fout << std::endl;

			// Build code which retrieves loads the correct relation
			//
			if (listSynonymMR.size() > 1)
			{
				fout << "\t\t// Load members of correct relation" << std::endl;
				fout << "\t\t//" << std::endl;
				fout << "\t\twhile(true)" << std::endl;
				fout << "\t\t{" << std::endl;
				fout << "\t\t\tMetaRelationPtr\t\tpMR;" << std::endl;
				fout << std::endl;

				for ( itrSynonymMR =  listSynonymMR.begin();
							itrSynonymMR != listSynonymMR.end();
							itrSynonymMR++)
				{
					pSynonymMR = *itrSynonymMR;

					fout << "\t\t\t// Try relation " << pSynonymMR->GetReverseName() << "'s " << pSynonymMR->GetName() << std::endl;
					fout << "\t\t\t//" << std::endl;

					fout << "\t\t\tpMR = m_pDatabase->GetMetaDatabase()->GetMetaEntity(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << ")->GetChildMetaRelation(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_CR_" << pSynonymMR->GetUniqueName() << ");\n";
					fout << std::endl;

					fout << "\t\t\tif (*(GetColumn(" << m_pMetaDatabase->GetAlias() << "_" << m_strName << "_" << pSynonymMR->GetSwitchColumn()->GetMetaColumn()->GetName() << ")) == *(pMR->GetSwitchColumn()))" << std::endl;
					fout << "\t\t\t{" << std::endl;

					if ( pSynonymMR->GetParentMetaKey()->GetMetaEntity()->IsCached() &&
							!pSynonymMR->GetChildMetaKey()->GetMetaEntity()->IsCached())
					{
						fout << "\t\t\t\tpRelation = GetChildRelation(pMR, pDB);" << std::endl;
					}
					else
					{
						fout << "\t\t\t\tpRelation = GetChildRelation(pMR);" << std::endl;
					}

					fout << "\t\t\t\tbreak;" << std::endl;
					fout << "\t\t\t}" << std::endl;

					if (itrSynonymMR != listSynonymMR.end())
						fout << std::endl;
				}

				fout << "\t\t\tbreak;" << std::endl;
				fout << "\t\t}" << std::endl;
			}
			else
			{
				if (bNeedDatabase)
					fout << "\t\tpRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << ")->GetChildMetaRelation(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_CR_" << pMR->GetName() << "), pDB);" << std::endl;
				else
					fout << "\t\tpRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << ")->GetChildMetaRelation(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_CR_" << pMR->GetName() << "));" << std::endl;
			}

			fout << std::endl;
			fout << "\t\tif (pRelation)" << std::endl;
			fout << "\t\t\tpRelation->LoadAll(bRefresh, bLazyFetch);" << std::endl;
			fout << "\t}" << std::endl;
			fout << std::endl;
			fout << std::endl;
			fout << std::endl;

			// Write GetAllChildren method
			//
			fout << "\t// Get the relation " << pMR->GetName() << std::endl;
			fout << "\t//" << std::endl;

			if (strChildClass == "Entity")
			{
				if (bNeedDatabase)
					fout << "\tRelationPtr " << strBaseClassName << "::Get" << pMR->GetName() << "(DatabasePtr pDB)" << std::endl;
				else
					fout << "\tRelationPtr " << strBaseClassName << "::Get" << pMR->GetName() << "()" << std::endl;
			}
			else
			{
				if (bNeedDatabase)
					fout << "\t" << strBaseClassName << "::" << pMR->GetName() << "* " << strBaseClassName << "::Get" << pMR->GetName() << "(DatabasePtr pDB)" << std::endl;
				else
					fout << "\t" << strBaseClassName << "::" << pMR->GetName() << "* " << strBaseClassName << "::Get" << pMR->GetName() << "()" << std::endl;
			}

			fout << "\t{" << std::endl;
			fout << "\t\tRelationPtr\t\tpRelation;" << std::endl;
			fout << std::endl;

			if (listSynonymMR.size() > 1)
			{
				fout << "\t\t// Get correct relation" << std::endl;
				fout << "\t\t//" << std::endl;
				fout << "\t\twhile(true)" << std::endl;
				fout << "\t\t{" << std::endl;
				fout << "\t\t\tMetaRelationPtr\t\tpMR;" << std::endl;
				fout << std::endl;

				for ( itrSynonymMR =  listSynonymMR.begin();
							itrSynonymMR != listSynonymMR.end();
							itrSynonymMR++)
				{

					pSynonymMR = *itrSynonymMR;

					fout << "\t\t\t// Try relation " << pSynonymMR->GetReverseName() << "'s " << pSynonymMR->GetName() << std::endl;
					fout << "\t\t\t//" << std::endl;

					fout << "\t\t\tpMR = m_pDatabase->GetMetaDatabase()->GetMetaEntity(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << ")->GetChildMetaRelation(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_CR_" << pSynonymMR->GetUniqueName() << ");\n";
					fout << std::endl;

					fout << "\t\t\tif (*(GetColumn(" << m_pMetaDatabase->GetAlias() << "_" << m_strName << "_" << pSynonymMR->GetSwitchColumn()->GetMetaColumn()->GetName() << ")) == *(pMR->GetSwitchColumn()))" << std::endl;
					fout << "\t\t\t{" << std::endl;



					if ( pSynonymMR->GetParentMetaKey()->GetMetaEntity()->IsCached() &&
							!pSynonymMR->GetChildMetaKey()->GetMetaEntity()->IsCached())
					{
						fout << "\t\t\t\tpRelation = GetChildRelation(pMR, pDB);" << std::endl;
					}
					else
					{
						fout << "\t\t\t\tpRelation = GetChildRelation(pMR);" << std::endl;
					}

					fout << "\t\t\t\tbreak;" << std::endl;
					fout << "\t\t\t}" << std::endl;

					if (itrSynonymMR != listSynonymMR.end())
						fout << std::endl;
				}


				fout << "\t\t\tbreak;" << std::endl;
				fout << "\t\t}" << std::endl;
			}
			else
			{
				if (bNeedDatabase)
					fout << "\t\tpRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << ")->GetChildMetaRelation(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_CR_" << pMR->GetName() << "), pDB);" << std::endl;
				else
					fout << "\t\tpRelation = GetChildRelation(m_pDatabase->GetMetaDatabase()->GetMetaEntity(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << ")->GetChildMetaRelation(" << m_pMetaDatabase->GetAlias() << "_" << GetName() << "_CR_" << pMR->GetName() << "));" << std::endl;
			}

			fout << std::endl;

			if (strChildClass == "Entity")
				fout << "\t\treturn pRelation;" << std::endl;
			else
				fout << "\t\treturn (" << strBaseClassName << "::" << pMR->GetName() << "*) pRelation;" << std::endl;

			fout << "\t}" << std::endl;
			fout << std::endl;
			fout << std::endl;
			fout << std::endl;
		}


		fout << std::endl;
		fout << std::endl;

		// Implement ALL iterator support
		//
		fout << "\t// =====================================================================================" << std::endl;
		fout << "\t// Class " << strBaseClassName << "::iterator Implementation" << std::endl;
		fout << "\t//" << std::endl;
		fout << std::endl;

		fout << "\t// De-reference operator*()" << std::endl;
		fout << "\t//" << std::endl;
		fout << "\t" << strClassName << "Ptr " << strBaseClassName << "::iterator::operator*()" << std::endl;
		fout << "\t{" << std::endl;
		fout << "\t\tInstanceKeyPtr pKey;" << std::endl;
		fout << "\t\tEntityPtr      pEntity;" << std::endl;
		fout << std::endl;
		fout << "\t\tpKey = (InstanceKeyPtr) ((InstanceKeyPtrSetItr*) this)->operator*();" << std::endl;
		fout << "\t\tpEntity = pKey->GetEntity();" << std::endl;
		fout << std::endl;
		fout << "\t\treturn (" << strClassName << "Ptr) pEntity;" << std::endl;
		fout << "\t}" << std::endl;
		fout << std::endl;
		fout << std::endl;
		fout << std::endl;

		fout << "\t// Assignment operator=()" << std::endl;
		fout << "\t//" << std::endl;
		fout << "\t" << strBaseClassName << "::iterator& " << strBaseClassName << "::iterator::operator=(const iterator& itr)" << std::endl;
		fout << "\t{" << std::endl;
		fout << "\t\t((InstanceKeyPtrSetItr*) this)->operator=(itr);" << std::endl;
		fout << std::endl;
		fout << "\t\treturn *this;" << std::endl;
		fout << "\t}" << std::endl;
		fout << std::endl;
		fout << std::endl;
		fout << std::endl;

		// Implement nested Child relation classes
		//
		for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
		{
			pMR = m_vectChildMetaRelation[idx];

			// Only generate generate this for the first child relation with this name
			//
			GetSiblingSwitchedChildRelations(pMR, listSynonymMR);

			if (!listSynonymMR.empty() && listSynonymMR.front() != pMR)
				continue;

			// Don't bother generating this if the target is a generic type
			if (pMR->GetChildMetaKey()->GetMetaEntity()->m_pInstanceClass != &(Entity::ClassObject()))
				strChildClass = pMR->GetChildMetaKey()->GetMetaEntity()->m_pInstanceClass->Name();
			else
				strChildClass = pMR->GetChildMetaKey()->GetMetaEntity()->GetName();

			strChildName = pMR->GetChildMetaKey()->GetMetaEntity()->GetName();

			fout << std::endl;
			fout << std::endl;

			fout << "\t// =====================================================================================" << std::endl;
			fout << "\t// Class " << strBaseClassName << "::" << pMR->GetName() << std::endl;
			fout << "\t//" << std::endl;
			fout << std::endl;
			fout << "\t// D3 class factory support for this class" << std::endl;
			fout << "\t//" << std::endl;
			fout << "\tD3_CLASS_IMPL(" << strBaseClassName << "::" << pMR->GetName() << ", Relation);" << std::endl;
			fout << std::endl;

			fout << "\t// De-reference operator*()" << std::endl;
			fout << "\t//" << std::endl;
			fout << "\t" << strChildClass << "Ptr " << strBaseClassName << "::" << pMR->GetName() << "::iterator::operator*()" << std::endl;

			fout << "\t{" << std::endl;
			fout << "\t\treturn (" << strChildClass << "Ptr) ((Relation::iterator*) this)->operator*();" << std::endl;
			fout << "\t}" << std::endl;
			fout << std::endl;
			fout << std::endl;
			fout << std::endl;

			fout << "\t// Assignment operator=()" << std::endl;
			fout << "\t//" << std::endl;
			fout << "\t" << strBaseClassName <<  "::" << pMR->GetName() << "::iterator& " << strBaseClassName << "::" << pMR->GetName() << "::iterator::operator=(const iterator& itr)" << std::endl;
			fout << "\t{" << std::endl;
			fout << "\t\treturn (" << strBaseClassName << "::" << pMR->GetName() << "::iterator&) ((Relation::iterator*) this)->operator=(itr);" << std::endl;
			fout << "\t}" << std::endl;
			fout << std::endl;
			fout << std::endl;
			fout << std::endl;

			fout << "\t// front() method" << std::endl;
			fout << "\t//" << std::endl;
			fout << "\t" << strChildClass << "Ptr " << strBaseClassName << "::" << pMR->GetName() << "::front()" << std::endl;
			fout << "\t{" << std::endl;
			fout << "\t\treturn (" << strChildClass << "Ptr) Relation::front();" << std::endl;
			fout << "\t}" << std::endl;
			fout << std::endl;
			fout << std::endl;
			fout << std::endl;

			fout << "\t// back() method" << std::endl;
			fout << "\t//" << std::endl;
			fout << "\t" << strChildClass << "Ptr " << strBaseClassName << "::" << pMR->GetName() << "::back()" << std::endl;
			fout << "\t{" << std::endl;
			fout << "\t\treturn (" << strChildClass << "Ptr) Relation::back();" << std::endl;
			fout << "\t}" << std::endl;
			fout << std::endl;
			fout << std::endl;
			fout << std::endl;
		}


		fout << std::endl;
		fout << "} // end namespace D3" << std::endl;

		return true;
	}



	std::string MetaEntity::AsCreateTableSQL(TargetRDBMS eTarget)
	{
		std::string			strSQL;
		unsigned int		idx;

		switch (eTarget)
		{
			case SQLServer:
			{
				strSQL  = "CREATE TABLE [";
				strSQL += m_strName;
				strSQL += "] (";

				for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
				{
					if (idx > 0)
						strSQL += ", ";

					strSQL += m_vectMetaColumn[idx]->AsCreateColumnSQL(eTarget);
				}

				strSQL += ") ";
				break;
			}
			case Oracle:
			{
				strSQL  = "CREATE TABLE ";
				strSQL += m_strName;
				strSQL += " (";

				for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
				{
					if (idx > 0)
						strSQL += ", ";

					strSQL += m_vectMetaColumn[idx]->AsCreateColumnSQL(eTarget);
				}

				strSQL += ") TABLESPACE ";

				strSQL += m_pMetaDatabase->m_strName;
				break;
			}
		}
		return strSQL;
	}

/*

	void MetaEntity::CreateTable(TargetRDBMS eTarget, odbc::Connection* pCon)
	{
		odbc::Statement*	pStmt = NULL;

		try
		{
			pStmt=pCon->createStatement();
			pStmt->executeUpdate(AsCreateTableSQL(eTarget));
			delete pStmt;
			pStmt = NULL;
		}
		catch(...)
		{
			delete pStmt;
			throw;
		}
	}

	void MetaEntity::CreateTable(TargetRDBMS eTarget, otl_connect& pCon)
	{
		try
		{
			otl_cursor::direct_exec(pCon,AsCreateTableSQL(eTarget).c_str());
		}
		catch(...)
		{
		}
	}
	#endif
*/
	std::string MetaEntity::AsCreateSequenceSQL(TargetRDBMS eTarget)
	{
		std::string			strSQL;
		unsigned int		idx;


		switch (eTarget)
		{
			case SQLServer:
			{
				break;
			}

			case Oracle:
			{
				for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
				{
					if (m_vectMetaColumn[idx]->IsAutoNum())
					{
					  std::string strName;

					  strName  = "seq_";
						strName += m_strName;
						strName += "_";
						strName += m_vectMetaColumn[idx]->GetName();

						strSQL  = "CREATE SEQUENCE ";
						strSQL += strName.substr(0,30);
						strSQL += " START WITH 1";

						ReportDiagnostic("%s", strSQL.c_str());

						break;		// D3 does not support multiple auto num columns per entity
					}
				}
			}
		}



		return strSQL;
	}



	std::string MetaEntity::AsDropSequenceSQL(TargetRDBMS eTarget)
	{
		std::string			strSQL;
		unsigned int		idx;


		switch (eTarget)
		{
			case SQLServer:
			{
				break;
			}

			case Oracle:
			{
				for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
				{
					if (m_vectMetaColumn[idx]->IsAutoNum())
					{
					  std::string strName;

					  strName  = "seq_";
						strName += m_strName;
						strName += "_";
						strName += m_vectMetaColumn[idx]->GetName();

						strSQL  = "DROP SEQUENCE ";
						strSQL += strName.substr(0,30);

						ReportDiagnostic("%s", strSQL.c_str());

						break;		// D3 does not support multiple auto num columns per entity
					}
				}

				break;
			}
		}


		return strSQL;
	}



	std::string MetaEntity::AsCreateSequenceTriggerSQL(TargetRDBMS eTarget)
	{
		std::string			strSQL;
		unsigned int		idx;


		switch (eTarget)
		{
			case SQLServer:
			{
				break;
			}

			case Oracle:
			{
				for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
				{
					if (m_vectMetaColumn[idx]->IsAutoNum())
					{
					  std::string strName;

						strName  = m_strName;
						strName += "_";
						strName += m_vectMetaColumn[idx]->GetName();

						strSQL  = "CREATE TRIGGER TRY_";
						strSQL += strName.substr(0,26);
						strSQL += "\n";
						strSQL += "BEFORE INSERT ON ";
						strSQL += m_strName;
						strSQL += "\n";
						strSQL += "FOR EACH ROW\n";
						strSQL += "BEGIN\n";
						strSQL += "SELECT seq_";
						strSQL += strName.substr(0,26);
						strSQL += ".NEXTVAL INTO :NEW.";
						strSQL += m_vectMetaColumn[idx]->GetName();
						strSQL += " FROM DUAL;\n";
						strSQL += "END;\n";

						ReportDiagnostic("%s", strSQL.c_str());
						break;
					}
				}

				break;
			}
		}

		return strSQL;
	}
/*


	void MetaEntity::CreateSequence(TargetRDBMS eTarget, odbc::Connection* pCon)
	{
		std::string			strSQL;
		odbc::Statement*	pStmt = NULL;

		try
		{
			strSQL=AsCreateSequenceSQL(eTarget);

			if (!strSQL.empty())
			{
				pStmt=pCon->createStatement();
				pStmt->executeUpdate(strSQL);
				delete pStmt;
				pStmt = NULL;
			}
		}
		catch(...)
		{
			delete pStmt;
			throw;
		}
	}
	void MetaEntity::CreateSequence(TargetRDBMS eTarget, otl_connect& pCon)
	{
		std::string			strSQL;
		try
		{
			strSQL=AsCreateSequenceSQL(eTarget);

			if (!strSQL.empty())
			{
				otl_cursor::direct_exec(pCon,strSQL.c_str());
			}
		}
		catch(...)
		{
			throw;
		}
	}
	void MetaEntity::DropSequence(TargetRDBMS eTarget, odbc::Connection* pCon)
	{
		std::string				strSQL;
		odbc::Statement*	pStmt = NULL;

		try
		{
			strSQL = AsDropSequenceSQL(eTarget);

			if (!strSQL.empty())
			{
				pStmt=pCon->createStatement();
				pStmt->executeUpdate(strSQL);
				delete pStmt;
				pStmt = NULL;
			}
		}
		catch(odbc::SQLException& e)
		{
			delete pStmt;
			ReportError("MetaEntity::DropSequence(): odbc++ error: %s", e.getMessage().c_str());
		}
		catch(...)
		{
			delete pStmt;
			ReportError("MetaEntity::DropSequence(): An unknow error occurred executing SQL: %s", strSQL.c_str());
		}
	}
	void MetaEntity::DropSequence(TargetRDBMS eTarget, otl_connect& pCon)
	{
		std::string				strSQL;
		try
		{
			strSQL = AsDropSequenceSQL(eTarget);
			if (!strSQL.empty())
			{
				  otl_cursor::direct_exec(pCon,strSQL.c_str());
			}

		}
		catch(otl_exception& p){ // intercept OTL exceptions
			std::cerr<<p.msg<<std::endl; // print out error message
  			std::cerr<<p.stm_text<<std::endl; // print out SQL that caused the error
			std::cerr<<p.var_info<<std::endl; // print out the variable that caused the error
			ReportError("MetaEntity::DropSequence(): OTL Error occurred executing SQL: %s", p.msg);
 		}
		catch(...)
		{
			ReportError("MetaEntity::DropSequence(): An unknow error occurred executing SQL: %s", strSQL.c_str());
		}
	}
	void MetaEntity::CreateSequenceTrigger(TargetRDBMS eTarget, odbc::Connection* pCon)
	{
		std::string			strSQL;
		odbc::Statement*	pStmt = NULL;

		try
		{
			strSQL=AsCreateSequenceTriggerSQL(eTarget);

			if (!strSQL.empty())
			{
				pStmt=pCon->createStatement();
				pStmt->executeUpdate(strSQL);
				delete pStmt;
				pStmt = NULL;
			}
		}
		catch(...)
		{
			delete pStmt;
			throw;
		}
	}
	void MetaEntity::CreateSequenceTrigger(TargetRDBMS eTarget, otl_connect& pCon)
	{
		std::string			strSQL;
		try
		{
			strSQL=AsCreateSequenceTriggerSQL(eTarget);

			if (!strSQL.empty())
			{
				otl_cursor::direct_exec(pCon,strSQL.c_str());
			}
		}
		catch(...)
		{
			throw;
		}
	}
	void MetaEntity::CreateIndexes(TargetRDBMS eTarget, odbc::Connection* pCon)
	{
		odbc::Statement*	pStmt = NULL;
		std::string				strSQL;
		unsigned int			idx;

		try
		{
			for (idx = 0; idx < m_vectMetaKey.size(); idx++)
			{
				strSQL = m_vectMetaKey[idx]->AsCreateIndexSQL(eTarget);

				if (!strSQL.empty())
				{
					pStmt=pCon->createStatement();
					pStmt->executeUpdate(m_vectMetaKey[idx]->AsCreateIndexSQL(eTarget));
					delete pStmt;
					pStmt = NULL;
				}
			}
		}
		catch(...)
		{
			delete pStmt;
			throw;
		}
	}
	void MetaEntity::CreateIndexes(TargetRDBMS eTarget, otl_connect& pCon)
	{
		std::string				strSQL;
		unsigned int			idx;
		try
		{
			for (idx = 0; idx < m_vectMetaKey.size(); idx++)
			{
				strSQL = m_vectMetaKey[idx]->AsCreateIndexSQL(eTarget);

				if (!strSQL.empty())
				{
					otl_cursor::direct_exec(pCon,m_vectMetaKey[idx]->AsCreateIndexSQL(eTarget).c_str());
				}
			}
		}
		catch(...)
		{
			throw;
		}
	}
	void MetaEntity::CreateTriggers(TargetRDBMS eTargetDB, odbc::Connection* pCon)
	{
		odbc::Statement*	pStmt = NULL;
		std::string				strSQL;
		unsigned int			idx;


		try
		{
			// Create CheckInsertUpdate triggers
			//
			for (idx = 0; idx < m_vectParentMetaRelation.size(); idx++)
			{
				strSQL = m_vectParentMetaRelation[idx]->AsCreateCheckInsertUpdateTrigger(eTargetDB);

				if (!strSQL.empty())
				{
					pStmt=pCon->createStatement();
					pStmt->executeUpdate(strSQL);
					delete pStmt;
					pStmt = NULL;
				}
			}

			// Create CascadeDelete triggers
			//
			for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
			{
				strSQL = m_vectChildMetaRelation[idx]->AsCreateCascadeDeleteTrigger(eTargetDB);

				if (!strSQL.empty())
				{
					pStmt=pCon->createStatement();
					pStmt->executeUpdate(strSQL);
					delete pStmt;
					pStmt = NULL;
				}
			}

			// Create CascadeClearRef triggers
			//
			for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
			{
				strSQL = m_vectChildMetaRelation[idx]->AsCreateCascadeClearRefTrigger(eTargetDB);

				if (!strSQL.empty())
				{
					pStmt=pCon->createStatement();
					pStmt->executeUpdate(strSQL);
					delete pStmt;
					pStmt = NULL;
				}
			}

			// Create VerifyDelete triggers
			//
			for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
			{
				strSQL = m_vectChildMetaRelation[idx]->AsCreateVerifyDeleteTrigger(eTargetDB);

				if (!strSQL.empty())
				{
					pStmt=pCon->createStatement();

					pStmt->executeUpdate(strSQL);
					delete pStmt;
					pStmt = NULL;
				}
			}

			// Create UpdateForeignRef triggers
			//
			for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
			{
				strSQL = m_vectChildMetaRelation[idx]->AsUpdateForeignRefTrigger(eTargetDB);

				if (!strSQL.empty())
				{
					pStmt=pCon->createStatement();
					pStmt->executeUpdate(strSQL);
					delete pStmt;
					pStmt = NULL;
				}
			}
		}
		catch(...)
		{
			delete pStmt;
			throw;
		}
	}
	void MetaEntity::CreateTriggers(TargetRDBMS eTargetDB, otl_connect& pCon)
	{
		std::string				strSQL;
		unsigned int			idx;

		try
		{
			// Create CheckInsertUpdate triggers
			//
			for (idx = 0; idx < m_vectParentMetaRelation.size(); idx++)
			{
				strSQL = m_vectParentMetaRelation[idx]->AsCreateCheckInsertUpdateTrigger(eTargetDB);

				if (!strSQL.empty())
				{
					otl_cursor::direct_exec(pCon,strSQL.c_str());
				}
			}

			// Create CascadeDelete triggers
			//
			for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
			{
				strSQL = m_vectChildMetaRelation[idx]->AsCreateCascadeDeleteTrigger(eTargetDB);

				if (!strSQL.empty())
				{
					otl_cursor::direct_exec(pCon,strSQL.c_str());
				}
			}

			// Create CascadeClearRef triggers
			//
			for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
			{
				strSQL = m_vectChildMetaRelation[idx]->AsCreateCascadeClearRefTrigger(eTargetDB);

				if (!strSQL.empty())
				{
					otl_cursor::direct_exec(pCon,strSQL.c_str());
				}
			}

			// Create VerifyDelete triggers
			//
			for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
			{
				strSQL = m_vectChildMetaRelation[idx]->AsCreateVerifyDeleteTrigger(eTargetDB);

				if (!strSQL.empty())
				{
					otl_cursor::direct_exec(pCon,strSQL.c_str());
				}
			}

			// Create UpdateForeignRef triggers
			//
			for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
			{
				strSQL = m_vectChildMetaRelation[idx]->AsUpdateForeignRefTrigger(eTargetDB);

				if (!strSQL.empty())
				{
					otl_cursor::direct_exec(pCon,strSQL.c_str());
				}
			}
		}
		catch(otl_exception& p){ // intercept OTL exceptions
			if(strlen((char*)p.var_info)!=0){
				std::cerr<<p.msg<<std::endl; // print out error message
				std::cerr<<p.stm_text<<std::endl; // print out SQL that caused the error
				std::cerr<<p.var_info<<std::endl; // print out the variable that caused the error
				ReportError("MetaDatabase::CreateTriggers(): OTL error: %s", p.msg);
				throw;
			}
 		}
		catch(...)
		{
			throw;
		}
	}

*/


	// We intercept this message to build a meta column vector that contains the columns
	// in the order they must be fetched from the database. BLOB like columns must be fetched
	// last!
	void MetaEntity::On_AfterConstructingMetaEntity()
	{
		unsigned int		idx;
		MetaColumnPtr		pCol;

		// Add all non streamed columns to the vector
		for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
		{
			pCol = m_vectMetaColumn[idx];

			if (!pCol->IsDerived() && !pCol->IsStreamed())
				m_vectMetaColumnFetchOrder.push_back(pCol);
		}

		// Now add all streamed columns to the vector
		for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
		{
			pCol = m_vectMetaColumn[idx];

			if (!pCol->IsDerived() && pCol->IsStreamed())
				m_vectMetaColumnFetchOrder.push_back(pCol);
		}

		// Build default helptext for columns without help text (usefull for single and multichoice columns only)
		for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
		{
			m_vectMetaColumn[idx]->SetDefaultDescription();
		}
	}


	// Add a MetaColumn to this
	//
	void MetaEntity::On_MetaColumnCreated(MetaColumnPtr pMetaColumn)
	{
		unsigned int		idx = m_vectMetaColumn.size();

		assert(pMetaColumn);
		assert(pMetaColumn->GetMetaEntity() == this);
		m_vectMetaColumn.push_back(pMetaColumn);
		pMetaColumn->m_uColumnIdx = idx;
	}


	void MetaEntity::On_MetaColumnDeleted(MetaColumnPtr pMetaColumn)
	{
		assert(pMetaColumn);
		assert(pMetaColumn->GetMetaEntity() == this);
		m_vectMetaColumn[pMetaColumn->m_uColumnIdx] = NULL;
	}


	// Add a MetaKey to this
	//
	void MetaEntity::On_MetaKeyCreated(MetaKeyPtr pMetaKey)
	{
		unsigned int		idx = m_vectMetaKey.size();

		assert(pMetaKey);
		assert(pMetaKey->GetMetaEntity() == this);
		m_vectMetaKey.push_back(pMetaKey);
		pMetaKey->m_uKeyIdx = idx;

		if (pMetaKey->IsPrimary())
			m_uPrimaryKeyIdx = idx;

		if (pMetaKey->IsConceptual())
			m_uConceptualKeyIdx = idx;
	}


	void MetaEntity::On_MetaKeyDeleted(MetaKeyPtr pMetaKey)
	{
		assert(pMetaKey);
		assert(pMetaKey->GetMetaEntity() == this);
		m_vectMetaKey[pMetaKey->m_uKeyIdx] = NULL;
	}



	// Notification that a relation with children has been added (this is the parent)
	//
	void MetaEntity::On_ChildMetaRelationCreated(MetaRelationPtr pMetaRelation)
	{
#ifdef _DEBUG
		unsigned int		idx;


		assert(pMetaRelation);
		assert(pMetaRelation->GetParentMetaKey());
		assert(pMetaRelation->GetParentMetaKey()->GetMetaEntity() == this);
		assert(pMetaRelation->GetParentIdx() == D3_UNDEFINED_ID);

		// Make sure we don't know this already
		//
		for (idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
		{
			// Now that would be odd...
			assert(m_vectChildMetaRelation[idx] != pMetaRelation);
		}
#endif

		// Resize vector and add MetaRelation
		//
		pMetaRelation->m_uParentIdx = m_vectChildMetaRelation.size();
		m_vectChildMetaRelation.push_back(pMetaRelation);
	}



	// Notification that a relation with a parent has been added (this is a child)
	//
	void MetaEntity::On_ParentMetaRelationCreated(MetaRelationPtr pMetaRelation)
	{
#ifdef _DEBUG
		unsigned int		idx;


		assert(pMetaRelation);
		assert(pMetaRelation->GetChildMetaKey());
		assert(pMetaRelation->GetChildMetaKey()->GetMetaEntity() == this);
		assert(pMetaRelation->GetChildIdx() == D3_UNDEFINED_ID);

		// Make sure we don't know this already
		//
		for (idx = 0; idx < m_vectParentMetaRelation.size(); idx++)
		{
			// Now that would be odd...
			assert(m_vectParentMetaRelation[idx] != pMetaRelation);
		}
#endif

		// Resize vector and add MetaRelation
		//
		pMetaRelation->m_uChildIdx = m_vectParentMetaRelation.size();
		m_vectParentMetaRelation.push_back(pMetaRelation);
	}



	void MetaEntity::On_ChildMetaRelationDeleted(MetaRelationPtr pMetaRelation)
	{
		assert(pMetaRelation);
		assert(m_vectChildMetaRelation[pMetaRelation->m_uParentIdx] == pMetaRelation);
		m_vectChildMetaRelation[pMetaRelation->m_uParentIdx] = NULL;
	}



	void MetaEntity::On_ParentMetaRelationDeleted(MetaRelationPtr pMetaRelation)
	{
		assert(pMetaRelation);
		assert(m_vectParentMetaRelation[pMetaRelation->m_uChildIdx] == pMetaRelation);
		m_vectParentMetaRelation[pMetaRelation->m_uChildIdx] = NULL;
	}



	void MetaEntity::On_DatabaseCreated(DatabasePtr pDatabase)
	{
		// Pass the message on to all our MetaKey objects
		//
		for (unsigned int idx = 0; idx < m_vectMetaKey.size(); idx++)
			m_vectMetaKey[idx]->On_DatabaseCreated(pDatabase);
	}



	void MetaEntity::On_DatabaseDeleted(DatabasePtr pDatabase)
	{
		// The primary key must destroy all instances for this
		//
		GetPrimaryMetaKey()->DeleteAllObjects(pDatabase);

		// Pass the message on to all our MetaKey objects
		//
		for (unsigned int idx = 0; idx < m_vectMetaKey.size(); idx++)
			m_vectMetaKey[idx]->On_DatabaseDeleted(pDatabase);
	}



	// Verify this' validity
	//
	bool MetaEntity::VerifyMetaData()
	{
		unsigned int		idx1, idx2;
		MetaColumnPtr		pAutoNumCol = NULL;
		MetaColumnPtr		pMC;
		MetaKeyPtr			pMK;
		bool						bHasRealCols = false;
		bool						bResult = true;


		// Must have at least one column
		//
		if (m_vectMetaColumn.empty())
		{
			ReportError("MetaEntity::VerifyMetaData(): MetaEntity %s has no columns.", GetFullName().c_str());
			bResult = false;
		}

		// Must have a class factory
		//
		if (!m_pInstanceClass)
		{
			ReportError("MetaEntity::VerifyMetaData(): MetaEntity %s has no class factory.", GetFullName().c_str());
			bResult = false;
		}

		// Can't have more that one AutoNum column
		//
		for (idx1 = 0; idx1 < m_vectMetaColumn.size(); idx1++)
		{
			if (m_vectMetaColumn[idx1] == NULL)
			{
				ReportError("MetaEntity::VerifyMetaData(): MetaColumn with index %i in MetaEntity %s is NULL.", idx1, GetFullName().c_str());
				bResult = false;
			}
			else
			{
				// Column must have a class factory
				//
				if (!m_vectMetaColumn[idx1]->m_pInstanceClass)
				{
					ReportError("MetaEntity::VerifyMetaData(): MetaColumn %s in MetaEntity %s has no class factory.", m_vectMetaColumn[idx1]->GetName().c_str(), GetFullName().c_str());
					bResult = false;
				}

				if (m_vectMetaColumn[idx1]->IsDerived())
					continue;

				bHasRealCols = true;

				if (m_vectMetaColumn[idx1]->IsAutoNum())
				{
					// Only one AutoNum column allowed
					//
					if (pAutoNumCol)
					{
						ReportError("MetaEntity::VerifyMetaData(): MetaColumn %s in MetaEntity %s is marked as AutoNum, but MetaColumn %s is already marked as AutoNum. Only one column can be AutoNum.", m_vectMetaColumn[idx1]->GetName().c_str(), GetFullName().c_str(), pAutoNumCol->GetName().c_str());
						bResult = false;
					}
					else
					{
						pAutoNumCol = m_vectMetaColumn[idx1];

					}
				}
			}
		}

		// Must have at least one key
		//
		pMK = GetPrimaryMetaKey();

		if (!pMK)
		{
			ReportError("MetaEntity::VerifyMetaData(): MetaEntity %s has no primary key. Every MetaEntity must have a primary key!", GetFullName().c_str());
			bResult = false;
		}
		else
		{
			if (!pMK->IsUnique())
			{
				ReportError("MetaEntity::VerifyMetaData(): Key %s is a primary key but not marked as UNIQUE!", pMK->GetFullName().c_str());
				bResult = false;
			}

			if (pAutoNumCol)
			{
				// We have an AutoColumn so the primary key must be AutoNum
				//
				if (!pMK->IsAutoNum())
				{
					ReportError("MetaEntity::VerifyMetaData(): MetaEntity %s has AutoNum column %s, but primary key %s is not marked AutoNum.", GetFullName().c_str(), pAutoNumCol->GetName().c_str(), pMK->GetName().c_str());
					bResult = false;
				}

				if (pMK->GetColumnCount())
				{
					pMC = pMK->GetMetaColumns()->front();

					// We have an AutoColumn so the primary key's first Column must be the AutoNum column
					//
					if (pMC != pAutoNumCol)
					{
						ReportError("MetaEntity::VerifyMetaData(): MetaEntity %s has AutoNum column %s, but primary key %s is not using this column.", GetFullName().c_str(), pAutoNumCol->GetName().c_str(), pMK->GetName().c_str());
						bResult = false;
					}

					// We have an AutoColumn so the primary key can only have one column
					//
					if (pMK->GetColumnCount() > 1)
					{

						ReportError("MetaEntity::VerifyMetaData(): MetaEntity %s has a an AutoNum column %s which should be the only column in the primary key %s but more columns exist.", GetFullName().c_str(), pAutoNumCol->GetName().c_str(), pMK->GetName().c_str());
						bResult = false;
					}
				}
			}
		}

		// Check keys
		//
		for (idx1 = 0; idx1 < m_vectMetaKey.size(); idx1++)
		{
			pMK = m_vectMetaKey[idx1];

			// Check if this key is overlapping with any of the existing keys and if it is send message
			//
			for (idx2 = 0; idx2 < idx1; idx2++)
			{
				if (pMK && m_vectMetaKey[idx2] && m_vectMetaKey[idx2]->IsOverlappingKey(pMK))
				{
					m_vectMetaKey[idx2]->AddOverlappingKey(pMK);
					pMK->AddOverlappingKey(m_vectMetaKey[idx2]);
				}
			}

			// We may not have a NULL MetaKey
			//
			if (pMK == NULL)
			{
				ReportError("MetaEntity::VerifyMetaData(): MetaKey with index %i in MetaEntity %s is NULL.", idx1, GetFullName().c_str());
				bResult = false;
			}
			else
			{
				// Don't bother checking conceptual keys
				//
				if (pMK->IsConceptual())
					continue;

				// We can't have an AutoNum key without an AutoNum colummn
				//
				if (pMK->IsAutoNum() && pAutoNumCol == NULL)
				{
					ReportError("MetaEntity::VerifyMetaData(): MetaKey %s of MetaEntity %s is marked AutoNum but this entity has no AutoNum column.", pMK->GetName().c_str(), GetFullName().c_str());
					bResult = false;
				}

				// Each MetaKey must at least have one column
				//
				if (pMK->GetColumnCount() < 1)
				{
					ReportError("MetaEntity::VerifyMetaData(): MetaKey %s of MetaEntity %s has no columns.", pMK->GetName().c_str(), GetFullName().c_str());
					bResult = false;
				}
				else
				{
					MetaColumnPtrListItr		itrKeyMC;
					MetaColumnPtr						pKeyMC;
					bool										bColsMandatory = false;

					for ( itrKeyMC =  pMK->GetMetaColumns()->begin();
								itrKeyMC != pMK->GetMetaColumns()->end();
								itrKeyMC++)
					{
						pKeyMC = *itrKeyMC;

						// The MetaKey may not have a NULL column in it's list
						//
						if (!pKeyMC)
						{
							ReportError("MetaEntity::VerifyMetaData(): MetaKey %s of MetaEntity %s has a NULL column.", pMK->GetName().c_str(), GetFullName().c_str());
							bResult = false;
						}
						else
						{
							if (pKeyMC->IsMandatory())
								bColsMandatory = true;

							// A MetaKey's columns must be KeyMember columns
							//
							if (!pKeyMC->IsKeyMember())
							{
								ReportError("MetaEntity::VerifyMetaData(): MetaKey %s of MetaEntity %s has MetaColumn %s which is not marked as key column.", pMK->GetName().c_str(), GetFullName().c_str(), pKeyMC->GetName().c_str());
								bResult = false;
							}
						}
					}

					// At least one column from a mandatory key must be mandatory.
					// Note that this is important because of differences between Oracle and SQL Server.
					// Let's assume a unique key consisting of to int, both optional. In Oracle this statement:
					//    INSERT INTO MyTable (x, y) VALUES (NULL, NULL)
					// can be run many times successfully. In SQL Server, it will fail the second time.
					// This is because in ORACLE: WHERE NULL=NULL is always false whereas in SQLServer
					// it is always true. However, if x was mandatory, this SQL:
					//    INSERT INTO MyTable (x, y) VALUES (1, NULL)
					// would only succeed once in both SQL Server and Oracle.
					if (!pMK->IsPrimary() && pMK->IsMandatory() && !bColsMandatory)
					{
						ReportError("MetaEntity::VerifyMetaData(): MetaKey %s of MetaEntity %s is marked mandatory but all columns are optional.", pMK->GetName().c_str(), GetFullName().c_str());
						bResult = false;
					}
				}
			}
		}


		return bResult;
	}



	void MetaEntity::GetSiblingSwitchedParentRelations(MetaRelationPtr pRel, MetaRelationPtrList & listMR)
	{
		MetaRelationPtr			pCandiateRel;


		listMR.clear();

		for (unsigned int idx = 0; idx < m_vectParentMetaRelation.size(); idx++)
		{
			pCandiateRel = m_vectParentMetaRelation[idx];

			if (pCandiateRel->GetReverseName() == pRel->GetReverseName())
				listMR.push_back(pCandiateRel);
		}
	}



	void MetaEntity::GetSiblingSwitchedChildRelations(MetaRelationPtr pRel, MetaRelationPtrList & listMR)
	{
		MetaRelationPtr			pCandiateRel;


		listMR.clear();

		for (unsigned int idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
		{
			pCandiateRel = m_vectChildMetaRelation[idx];

			if (pCandiateRel->GetName() == pRel->GetName())
				listMR.push_back(pCandiateRel);
		}
	}



	/* static */
	std::ostream & MetaEntity::AllAsJSON(RoleUserPtr pRoleUser, std::ostream & ojson)
	{
		D3::RolePtr									pRole = pRoleUser ? pRoleUser->GetRole() : NULL;
		D3::MetaDatabasePtr					pMD;
		D3::MetaDatabasePtrMapItr		itrMD;
		D3::MetaEntityPtr						pME;
		D3::MetaEntityPtrVectItr		itrME;
		bool												bFirst = true;
		bool												bShallow = true;

		ojson << '[';

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

				if (pRole->GetPermissions(pME) & D3::MetaEntity::Permissions::Select)
				{
					bFirst ? bFirst = false : ojson << ',';
					ojson << "\n\t";
					pME->AsJSON(pRoleUser, ojson, NULL, bShallow);
				}
			}
		}

		ojson << "\n]";


		return ojson;
	}



	std::ostream & MetaEntity::AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, bool* pFirstSibling, bool bShallow)
	{
		RolePtr										pRole = pRoleUser ? pRoleUser->GetRole() : NULL;
		MetaEntity::Permissions		permissions = pRole ? pRole->GetPermissions(this) : m_Permissions;

		if (permissions & MetaEntity::Permissions::Select)
		{
			if (pFirstSibling)
			{
				if (*pFirstSibling)
					*pFirstSibling = false;
				else
					ostrm << ',';
			}

			ostrm << '{';
			ostrm << "\"ID\":"									<< GetID() << ',';
			ostrm << "\"Idx\":"									<< GetEntityIdx() << ',';
			ostrm << "\"MetaDatabaseID\":"			<< m_pMetaDatabase->GetID() << ',';
			ostrm << "\"Name\":\""							<< JSONEncode(GetName()) << "\",";
			ostrm << "\"Alias\":\""							<< JSONEncode(GetAlias()) << "\",";
			ostrm << "\"ProsaName\":\""					<< JSONEncode(GetProsaName()) << "\",";
/*
			std::string				strBase64Desc;
			APALUtil::base64_encode(strBase64Desc, (const unsigned char *) GetDescription().c_str(), GetDescription().size());
			ostrm << "\"Description\":\""				<< strBase64Desc << "\",";
*/
			ostrm << "\"PrimaryKeyIdx\":"				<< m_uPrimaryKeyIdx << ',';

			ostrm << "\"ConceptualKeyIdx\":";
			if (GetConceptualMetaKey())
				ostrm << GetConceptualKeyIdx() << ',';
			else
				ostrm << "null,";

			ostrm << "\"MetaClass\":\""					<< JSONEncode(GetMetaClassName()) << "\",";
			ostrm << "\"InstanceClass\":\""			<< JSONEncode(GetInstanceClassName()) << "\",";
			ostrm << "\"InstanceInterface\":\""	<< JSONEncode(GetInstanceInterface()) << "\",";
			ostrm << "\"JSMetaClass\":\""				<< JSONEncode(GetJSMetaClassName()) << "\",";
			ostrm << "\"JSInstanceClass\":\""		<< JSONEncode(GetJSInstanceClassName()) << "\",";
			ostrm << "\"JSViewClass\":\""				<< JSONEncode(GetJSViewClassName()) << "\",";
			ostrm << "\"JSDetailViewClass\":\""	<< JSONEncode(GetJSDetailViewClassName()) << "\",";

			ostrm << "\"Hidden\":"			<< (IsHidden()		? "true" : "false") << ',';
			ostrm << "\"Cached\":"			<< (IsCached()		? "true" : "false") << ',';
			ostrm << "\"ShowCK\":"			<< (IsShowCK()		? "true" : "false") << ',';

			ostrm << "\"Associative\":"	<< (IsAssociative() ? "true" : "false") << ',';

			ostrm << "\"AllowInsert\":"	<< (permissions & MetaEntity::Permissions::Insert	? "true" : "false") << ',';
			ostrm << "\"AllowSelect\":"	<< (permissions & MetaEntity::Permissions::Select	? "true" : "false") << ',';
			ostrm << "\"AllowUpdate\":"	<< (permissions & MetaEntity::Permissions::Update	? "true" : "false") << ',';
			ostrm << "\"AllowDelete\":"	<< (permissions & MetaEntity::Permissions::Delete	? "true" : "false") << ',';
			ostrm << "\"HSTopics\":"		<< (m_strHSTopicsJSON.empty() ? "null" : m_strHSTopicsJSON);

			if (!bShallow)
			{
				ostrm << ",\"MetaColumns\":[";
				MetaColumnsAsJSON(pRoleUser, ostrm);
				ostrm << "],\"MetaKeys\":[";
				MetaKeysAsJSON(pRoleUser, ostrm);
				ostrm << "],\"ChildMetaRelations\":[";
				ChildMetaRelationsAsJSON(pRoleUser, ostrm);
				ostrm << "],\"ParentMetaRelations\":[";
				ParentMetaRelationsAsJSON(pRoleUser, ostrm);
				ostrm << ']';
			}

			ostrm << '}';
		}

		return ostrm;
	}



	void MetaEntity::MetaColumnsAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm)
	{
		MetaColumnPtr			pMC;
		bool							bFirstChild = true;


		for (unsigned int idx = 0; idx < m_vectMetaColumn.size(); idx++)
		{
			pMC = m_vectMetaColumn[idx];
			pMC->AsJSON(pRoleUser, ostrm, &bFirstChild);
		}
	}



	void MetaEntity::MetaKeysAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm)
	{
		MetaKeyPtr				pMK;
		bool							bFirstChild = true;


		for (unsigned int idx = 0; idx < m_vectMetaKey.size(); idx++)
		{
			pMK = m_vectMetaKey[idx];
			pMK->AsJSON(pRoleUser, ostrm, this, &bFirstChild);
		}
	}



	void MetaEntity::ChildMetaRelationsAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm)
	{
		RolePtr						pRole = pRoleUser ? pRoleUser->GetRole() : NULL;
		MetaRelationPtr		pMR;
		bool							bFirstChild = true;


		for (unsigned int idx = 0; idx < m_vectChildMetaRelation.size(); idx++)
		{
			pMR = m_vectChildMetaRelation[idx];
			if (!pRole || pRole->CanReadChildren(pMR))
				pMR->AsJSON(pRoleUser, ostrm, this, &bFirstChild);
		}
	}



	void MetaEntity::ParentMetaRelationsAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm)
	{
		RolePtr						pRole = pRoleUser ? pRoleUser->GetRole() : NULL;
		MetaRelationPtr		pMR;
		bool							bFirstChild = true;


		for (unsigned int idx = 0; idx < m_vectParentMetaRelation.size(); idx++)
		{
			pMR = m_vectParentMetaRelation[idx];
			if (!pRole || pRole->CanReadParent(pMR))
				pMR->AsJSON(pRoleUser, ostrm, this, &bFirstChild);
		}
	}



	std::string MetaEntity::AsSQLSelectList(bool bLazyFetch, const std::string & strPrefix)
	{
		unsigned int			idx;
		MetaColumnPtr			pMC;
		bool							bFirst = true;
		std::string				strSQL;


		for (idx = 0; idx < m_vectMetaColumnFetchOrder.size(); idx++)
		{
			pMC = m_vectMetaColumnFetchOrder[idx];

			if (bLazyFetch && pMC->IsLazyFetch())
			{
				// We still want a NULL indicator
				if (bFirst)
					bFirst = false;
				else
					strSQL += ',';

				switch (GetMetaDatabase()->GetTargetRDBMS())
				{
					case SQLServer:
					{
						strSQL += "CASE WHEN ";

						if (!strPrefix.empty())
						{
							strSQL += strPrefix;
							strSQL += '.';
						}

						strSQL += pMC->GetName();
						strSQL += " IS NULL THEN CONVERT(bit, 0) ELSE CONVERT(bit, 1) END AS Has";
						strSQL += pMC->GetName();
						break;
					}
					case Oracle:
					{
						if (pMC->IsStreamed())
						{
							// Do this:
							//  CASE WHEN dbms_lob.getLength(BLOB_COLUMN_NAME) > 0 THEN 1 ELSE 0 END AS HasBLOB_COLUMN_NAME
							strSQL += "CASE WHEN dbms_lob.getLength(";

							if (!strPrefix.empty())
							{
								strSQL += strPrefix;
								strSQL += '.';
							}

							strSQL += pMC->GetName();
							strSQL += ") > 0 THEN 1 ELSE 0 END AS Has";
							strSQL += pMC->GetName();
						}
						else
						{
							// Do this:
							//  NVL2(BLOB_COLUMN_NAME, 1, 0) AS HasBLOB_COLUMN_NAME
							strSQL += "NVL2(";

							if (!strPrefix.empty())
							{
								strSQL += strPrefix;
								strSQL += '.';
							}

							strSQL += pMC->GetName();
							strSQL += ", 1, 0) AS Has";
							strSQL += pMC->GetName();
						}
						break;
					}
				}
			}
			else
			{
				if (bFirst)
					bFirst = false;
				else
					strSQL += ',';

				if (!strPrefix.empty())
				{
					strSQL += strPrefix;
					strSQL += '.';
				}

				strSQL += pMC->GetName();
			}
		}

		return strSQL;
	}





	/* static */
	EntityPtr MetaEntity::GetInstanceEntity(Json::Value & jsonID, DatabaseWorkspace& dbWS)
	{
		D3::TemporaryKey		tempKey(jsonID);
		D3::MetaKeyPtr			pMK = tempKey.GetMetaKey();
		D3::DatabasePtr			pDB;


		if (pMK != tempKey.GetMetaKey()->GetMetaEntity()->GetPrimaryMetaKey())
			throw std::string("KeyID does not specify a primary key");

		pDB = dbWS.GetDatabase(pMK->GetMetaEntity()->GetMetaDatabase());

		if (pDB)
			return pMK->LoadObject(&tempKey, pDB);

		return NULL;
	}





	bool MetaEntity::HasCyclicDependency(MetaEntityPtr pMetaEntity, bool bIgnoreCrossDatabaseRelations)
	{
		std::set<MetaEntityPtr>		setProcessed;

		return HasCyclicDependency(pMetaEntity, setProcessed, bIgnoreCrossDatabaseRelations);
	}





	bool MetaEntity::HasCyclicDependency(MetaEntityPtr pMetaEntity, std::set<MetaEntityPtr>& setProcessed, bool bIgnoreCrossDatabaseRelations)
	{
		if (this != pMetaEntity)
		{
			MetaRelationPtrVectItr		itrPMR;
			MetaEntityPtr							pMEParent;

			setProcessed.insert(pMetaEntity);

			for ( itrPMR =  pMetaEntity->GetParentMetaRelations()->begin();
						itrPMR != pMetaEntity->GetParentMetaRelations()->end();
						itrPMR++)
			{
				pMEParent = (*itrPMR)->GetParentMetaKey()->GetMetaEntity();

				if (bIgnoreCrossDatabaseRelations && pMetaEntity->GetMetaDatabase() != pMEParent->GetMetaDatabase())
					continue;

				if (setProcessed.find(pMEParent) != setProcessed.end())
					continue;

				if (HasCyclicDependency(pMEParent, setProcessed, bIgnoreCrossDatabaseRelations))
					return true;
			}

			return false;
		}

		return true;
	}





	EntityPtr MetaEntity::PopulateObject(RoleUserPtr pRoleUser, Json::Value & jsonValues, DatabaseWorkspace& dbWS)
	{
		D3::DatabasePtr						pDB = NULL;
		D3::EntityPtr							pEntity = NULL;


		try
		{
			// Create a new instance and populate it
			pDB = dbWS.GetDatabase(m_pMetaDatabase);

			pEntity = CreateInstance(pDB);
			pEntity->On_BeforePopulatingObject();
			pEntity->SetValues(pRoleUser, jsonValues);
			pEntity->On_AfterPopulatingObject();
			pEntity->MarkNew();
		}
		catch (...)
		{
			delete pEntity;
			throw;
		}

		return pEntity;
/*
		D3::MetaKeyPtr						pMK;
		D3::MetaColumnPtr					pMC;
		D3::DatabasePtr						pDB = NULL;
		D3::EntityPtr							pEntity = NULL;
		D3::ColumnPtr							pIC;
		unsigned int							idx;


		try
		{
			pDB = dbWS.GetDatabase(m_pMetaDatabase);

			// Verify that all mandatory columns have none null values supplied
			for (idx = 0; idx < m_vectMetaColumn.size(); idx++)
			{
				pMC = m_vectMetaColumn[idx];

				if (pMC->IsMandatory() && !pMC->IsAutoNum())
				{
					if (jsonValues[pMC->GetName()] == Json::nullValue)
						throw std::string("MetaEntity::PopulateObject(): You must supply a non null value for column ") + pMC->GetName();
				}
			}

			// Verify that the values supplied for the unique keys are unique
			for (idx = 0; idx < m_vectMetaKey.size(); idx++)
			{
				pMK = m_vectMetaKey[idx];

				if (pMK->IsUnique() && !pMK->IsAutoNum())
				{
					D3::TemporaryKey			tempKey(*pMK);
					D3::ColumnPtrListItr	itr;

					for (	itr =  tempKey.GetColumns().begin();
								itr != tempKey.GetColumns().end();
								itr++)
					{
						pIC = *itr;
						pIC->SetValue(jsonValues[pIC->GetMetaColumn()->GetName()]);
					}

					if (pMK->FindInstanceKey(&tempKey, pDB))
						throw std::string("MetaEntity::PopulateObject(): Another object with matching unique key ") + pMK->GetName() + " already exists in this workspace";
				}
			}

			// Create a new instance and populate it
			pEntity = CreateInstance(pDB);

			pEntity->On_BeforePopulatingObject();

			for (idx = 0; idx < pEntity->GetColumnCount(); idx++)
			{
				pIC = pEntity->GetColumn(idx);

				if (!pIC->GetMetaColumn()->IsAutoNum())
					pIC->SetValue(jsonValues[pIC->GetMetaColumn()->GetName()]);
			}

			pEntity->On_AfterPopulatingObject();
			pEntity->MarkNew();
		}
		catch (...)
		{
			delete pEntity;
			throw;
		}

		return pEntity;
*/
	}











	// ==========================================================================
	// Entity implementation
	//

	static boost::recursive_mutex			G_mtxEntity;


	// Standard D3 stuff
	//
	D3_CLASS_IMPL(Entity, Object);



	Entity::~Entity()
	{
#ifdef D3_OBJECT_LINKS
		// 24/09/02 - R1 - Hugp - TMS support stuff
		// Delete all ObjectLinks
		//
		if (m_pObjectLinks)
		{
			while (!m_pObjectLinks->empty())
				delete m_pObjectLinks->front();

			delete m_pObjectLinks;
		}
#endif
		ColumnPtr						pCol;
		InstanceKeyPtr			pKey;
		unsigned int				idx;
		RelationPtrMapPtr		pRelationMap;
		RelationPtrMapItr		itrRelationMap;
		RelationPtr					pRelation;


		if (m_uFlags & D3_ENTITY_TRACE_DELETE)
		{
			// Debugging aid, dump info about this
			ReportInfo("Entity::~Entity(): Start deleting %s (" PRINTF_POINTER_MASK "). Key Column Values follow:", m_pMetaEntity->GetName().c_str(), this);

			for (idx = 0; idx < m_vectColumn.size(); idx++)
			{
				pCol = m_vectColumn[idx];

				if (pCol->IsKeyMember())
					ReportInfo("Entity::~Entity():       %s => %s", pCol->GetMetaColumn()->GetName().c_str(), pCol->AsString().c_str());
			}
		}

		MarkDestroying();

		// Delete the original primary key
		delete m_pOriginalKey;
		m_pOriginalKey = NULL;

		// If this is a member of one or more result sets, notify them before deleting the collection
		if (m_pListResultSet)
		{
			ResultSetPtr	pResultSet;

			while (!m_pListResultSet->empty())
			{
				pResultSet = m_pListResultSet->front();
				pResultSet->On_EntityDeleted(this);
				m_pListResultSet->pop_front();
			}

			delete m_pListResultSet;
		}

		// Delete all parent relation maps.
		// Note: This must be done BEFORE we delete child relations as otherwise cyclic relations will not
		//       be cleaned up correctly. Consider entity A is a parent of Entity B and vs. if A deletes B
		//       before it relinquishes it's association as a child of B, it will try to notify the parent
		//       relation with B to remove itself after the relation has been destroyed with B.
		//
		for (idx = 0; idx < m_vectParentRelation.size(); idx++)
		{
			pRelationMap = m_vectParentRelation[idx];

			while (pRelationMap && !pRelationMap->empty())
			{
				itrRelationMap = pRelationMap->begin();
				pRelation = itrRelationMap->second;
				pRelation->RemoveChild(this);
			}

			delete m_vectParentRelation[idx];
			m_vectParentRelation[idx] = NULL;
		}

		// Delete all child relations and child relation maps
		//
		for (idx = 0; idx < m_vectChildRelation.size(); idx++)
		{
			pRelationMap = m_vectChildRelation[idx];


			if (pRelationMap && !pRelationMap->empty())
			{
				// In order to ensure correct destruction of objects even if cyclic relations are in place
				// we copy the relation map and iterate over it. We then just use the key (the DatabasePtr)
				// and try and access it from the actual map and if it still exists, we'll delete the
				// relation.
				//
				RelationPtrMap			mapRelation(*pRelationMap);
				RelationPtrMapItr		itrRelationMap, itrRelationMapCopy;

				for ( itrRelationMapCopy =  mapRelation.begin();
							itrRelationMapCopy != mapRelation.end();
							itrRelationMapCopy++)
				{
					itrRelationMap = pRelationMap->find(itrRelationMapCopy->first);

					if (itrRelationMap != pRelationMap->end())
					{
						pRelation = itrRelationMap->second;

						if (pRelation && !pRelation->IsDestroying())
							delete pRelation;
					}
				}
			}

			delete m_vectChildRelation[idx];
			m_vectChildRelation[idx] = NULL;
		}

		// Delete all InstanceKey objects
		//
		for (idx = 0; idx < m_vectInstanceKey.size(); idx++)
		{
			pKey = m_vectInstanceKey[idx];
			delete pKey;
		}

		// Delete all Column objects
		//
		for (idx = 0; idx < m_vectColumn.size(); idx++)
		{
			pCol = m_vectColumn[idx];
			delete pCol;
		}

		m_pMetaEntity->On_InstanceDeleted(this);

		if (m_uFlags & D3_ENTITY_TRACE_DELETE)
			ReportInfo("Entity::~Entity(): Finished deleting %s (" PRINTF_POINTER_MASK ").", m_pMetaEntity->GetName().c_str(), this);
	}





	KeyPtr Entity::GetOriginalPrimaryKey()
	{
		if (m_pOriginalKey)
			return m_pOriginalKey;

		return GetInstanceKey(m_pMetaEntity->GetPrimaryKeyIdx());
	}






	Entity::UpdateType Entity::GetUpdateType()
	{
		if (IsDeleted())
			return SQL_Delete;

		if (IsNew())
			return SQL_Insert;

		if (IsDirty())
			return SQL_Update;

		return SQL_None;
	}




	bool Entity::Delete(DatabasePtr pDB)
	{
		// New records are simple destroyed
		if (IsNew())
		{
			delete this;
			return true;
		}

		MarkDelete();

		return Update(pDB);
	}





	void Entity::TraceDelete(bool bDeep)
	{
		RelationPtrMapPtr			pRelationMap;
		RelationPtrMapItr			itrRelationMap;
		RelationPtr						pRelation;
		InstanceKeyPtrSetItr	itrKeySet;
		InstanceKeyPtr				pKey;


		// Set this' D3_ENTITY_TRACE_DELETE flag
		m_uFlags |= D3_ENTITY_TRACE_DELETE;

		if (bDeep)
		{
			// Propagate method to children
			for (std::size_t idx = 0; idx < m_vectChildRelation.size(); idx++)
			{
				pRelationMap = m_vectChildRelation[idx];

				if (pRelationMap && !pRelationMap->empty())
				{
					for ( itrRelationMap =  pRelationMap->begin();
								itrRelationMap != pRelationMap->end();
								itrRelationMap++)
					{
						pRelation = itrRelationMap->second;

						for ( itrKeySet =  pRelation->GetChildKeys().begin();
									itrKeySet != pRelation->GetChildKeys().end();
									itrKeySet++)
						{
							pKey = (InstanceKeyPtr) *itrKeySet;
							pKey->GetEntity()->TraceDelete(bDeep);
						}
					}
				}
			}
		}
	}





	EntityPtr Entity::LoadObject(DatabasePtr pDB, bool bRefresh, bool bLazyFetch)
	{
		DatabasePtr				pDatabase = pDB;


		if (IsNew())
			return NULL;

		if (!pDatabase)
		{
			pDatabase = m_pDatabase;
		}
		else
		{
			if (m_pMetaEntity->IsCached())
			{
				pDatabase = m_pMetaEntity->GetMetaDatabase()->GetGlobalDatabase();
			}
			else
			{
				if (pDatabase->GetMetaDatabase() != m_pMetaEntity->GetMetaDatabase())
				{
					pDatabase = pDB->GetDatabaseWorkspace()->GetDatabase(m_pMetaEntity->GetMetaDatabase());
				}
			}
		}

		assert (pDatabase);


		return GetPrimaryKey()->GetMetaKey()->LoadObject(GetPrimaryKey(), m_pDatabase, bRefresh, bLazyFetch);
	}





	bool Entity::Update(DatabasePtr pDB, bool bDeepUpdate)
	{
		unsigned int				idx;
		bool								bSuccess = true;
		RelationPtr					pRelation;
		UpdateType					eUT = GetUpdateType();
		EntityPtr						pEntity;
		EntityPtrList				listChildren;
		EntityPtrListItr		itrChildren;
		InstanceKeyPtr			pKey;


#ifdef D3_OBJECT_LINKS
		ObjectLinkPtr										pOL;
		ObjectLinkListItr								itrOL;
#endif


		// depending on the update type, we may have to let the dependants do their work first
		//
		switch (eUT)
		{
			case SQL_Delete :
				if (!On_BeforeUpdate(eUT))
					return false;

				// If this is not IsNew(), delete the object from the database
				//
				if (!IsNew())
					bSuccess = pDB->UpdateObject(this);

				return bSuccess;

			case SQL_Insert :
			case SQL_Update :
				// When inserting/updating objects, we must ensure that all dirty parent objects are updated first
				//
				for (idx = 0; idx < GetParentRelationCount(); idx++)
				{
					pRelation = GetParentRelation(idx);

					if (!pRelation)
					{
						MetaRelationPtr	pMR;

						pMR = GetMetaEntity()->GetParentMetaRelation(idx);

						assert(pMR);

						// Is the missing relation mandatory
						//
						if (pMR->GetChildMetaKey()->IsMandatory())
						{
							// Relations which are switched based on the value of a specific column from this
							// can be NULL if the switch column's value doesn't match the meta relations switch
							if (pMR->IsChildSwitch())
							{
								if (*(pMR->GetSwitchColumn()) != *(GetColumn(pMR->GetSwitchColumn()->GetMetaColumn())))
									continue;
							}

							pKey = GetInstanceKey(pMR->GetChildMetaKey());

							// If the missing mandatory relationship is a cross database relation
							// issue a warning only else issue error and fail
							//
							if (pMR->GetParentMetaKey()->GetMetaEntity()->GetMetaDatabase() !=
									pMR->GetChildMetaKey()->GetMetaEntity()->GetMetaDatabase())
							{
								ReportWarning("Entity::Update(): Inserting Entity %s with mandatory foreign key %s (%s) pointing to no valid parent from another database.", m_pMetaEntity->GetName().c_str(), pMR->GetChildMetaKey()->GetName().c_str(), pKey->AsString().c_str());
							}
							else
							{
								throw Exception(__FILE__, __LINE__, Exception_error, "Entity::Update(): Can't insert/update Entity %s with mandatory foreign key %s (%s) pointing to no valid parent.", m_pMetaEntity->GetName().c_str(), pMR->GetChildMetaKey()->GetName().c_str(), pKey->AsString().c_str());
							}
						}

						continue;
					}

					pEntity = pRelation->GetParent();

					if (pEntity && pEntity->GetUpdateType() == SQL_Insert)
						if (!pEntity->Update(pDB))
							return false;
				}

				if (!On_BeforeUpdate(eUT)			||
						!pDB->UpdateObject(this)	||
						!On_AfterUpdate(eUT))
					return false;

#ifdef D3_OBJECT_LINKS
				// 24/09/02 - R1 - Hugp - TMS support stuff
				// Update ObjectLink objects
				//
				if (m_pObjectLinks)
				{
					ObjectLinkList	listOL(*m_pObjectLinks);

					for (itrOL = listOL.begin(); itrOL != listOL.end(); itrOL++)
					{
						pOL = *itrOL;
						if (!pOL->Update(pDB))
						{
							bSuccess = false;
							break;
						}
					}
				}
#endif
				break;

			case SQL_None :
				// we may still have to update dirty kids
				break;

			default :
				return false;
		}


		if (bSuccess && bDeepUpdate)
		{
			// Update all child relationship objects
			//
			for (idx = 0; idx < GetChildRelationCount(); idx++)
			{
				pRelation = GetChildRelation(idx, pDB);

				if (pRelation)
				{
					// We must take a snapshot of the children. During updates, relation navigation is
					// not reliable as the update can cause objects in relations to be repositioned.
					// For this reason, we take a snapshot
					//
					listChildren.clear();
					pRelation->GetChildrenSnapshot(listChildren);

					for ( itrChildren =  listChildren.begin();
								itrChildren != listChildren.end();
								itrChildren++)
					{

						pEntity = *itrChildren;
						bSuccess = pEntity->Update(pDB, bDeepUpdate);

						if (!bSuccess)
							break;
					}
				}

				if (!bSuccess)
					break;
			}
		}

		// Notify the keys of update
		//
		if (bSuccess)
		{
			for (idx = 0; idx < m_vectInstanceKey.size(); idx++)
			{
				pKey = m_vectInstanceKey[idx];

				if (pKey)
					pKey->On_AfterUpdateEntity();
			}
		}


		return bSuccess;
	}



	bool Entity::IsValid()
	{
		return true;
	}



	void Entity::Dump(int nIndentSize, bool bDeep)
	{
		unsigned int				idx;
		ColumnPtr						pCol;
		InstanceKeyPtr			pKey;
		unsigned int				iMaxColWidth = 0;
		RelationPtr					pRel;
		Relation::iterator	itrRel;



		std::cout << std::setw(nIndentSize) << "";
		std::cout << m_pMetaEntity->GetName().c_str() << " (Database " << m_pDatabase << "):" << std::endl;
		std::cout << std::setw(nIndentSize) << "";
		std::cout << "{" << std::endl;

		switch (GetUpdateType())
		{
			case SQL_Delete:
				std::cout << std::setw(nIndentSize+2) << "";
				std::cout << "(marked DELETE)" << std::endl;
				break;

			case SQL_Insert:
				std::cout << std::setw(nIndentSize+2) << "";
				std::cout << "(marked INSERT)" << std::endl;
				break;

			case SQL_Update:
				std::cout << std::setw(nIndentSize+2) << "";
				std::cout << "(marked UPDATE)" << std::endl;
				break;

			default:
				std::cout << std::setw(nIndentSize+2) << "";
				std::cout << "(marked CLEAN)" << std::endl;
		}

		// Find the longest label
		//
		for (idx = 0; idx < m_vectColumn.size(); idx++)
		{
			pCol = m_vectColumn[idx];

			if (pCol)
				if (pCol->GetMetaColumn()->GetName().size() > iMaxColWidth)
					iMaxColWidth = pCol->GetMetaColumn()->GetName().size();
		}

		// Dump columns regardless of deep or not deep
		//
		std::cout << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "Columns:" << std::endl;
		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "{" << std::endl;

		for (idx = 0; idx < m_vectColumn.size(); idx++)
		{
			pCol = m_vectColumn[idx];

			if (pCol)
				pCol->Dump(nIndentSize+4, iMaxColWidth);
		}

		std::cout << std::setw(nIndentSize+2) << "";
		std::cout << "}" << std::endl;


		if (bDeep)
		{
			// Dump keys

			//
/*
			std::cout << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "Keys:" << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "{" << std::endl;

			for (idx = 0; idx < m_pMetaEntity->GetMetaKeyCount(); idx++)
			{
				pKey = m_vectInstanceKey[idx];

				if (!pKey)
				{
					std::cout << std::setw(nIndentSize) << "";
					std::cout << m_pMetaEntity->GetMetaKey(idx)->GetName().c_str() << ": [entity has no instance key for this]" << std::endl;

				}
				else
					pKey->Dump(nIndentSize+4);
			}

			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "}" << std::endl;
*/
			// Dump parent relations
			//
			std::cout << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "ParentRelations:" << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "{" << std::endl;

			for (idx = 0; idx < GetParentRelationCount(); idx++)
			{
				pRel = GetParentRelation(idx, m_pDatabase);

				if (pRel)
				{
					std::cout << std::setw(nIndentSize+4) << "";
					std::cout << pRel->GetMetaRelation()->GetName() << " " << pRel->GetMetaRelation()->GetReverseName() << ": ";
					pKey = pRel->GetParent()->GetPrimaryKey();
					std::cout << pKey->AsString() << std::endl;
				}
/*
				if (pRel)
				{
					std::cout << std::setw(nIndentSize+4) << "";
					std::cout << pRel->GetMetaRelation()->GetName() << " " << pRel->GetMetaRelation()->GetReverseName() << ":" << std::endl;
					std::cout << std::setw(nIndentSize+4) << "";
					std::cout << "{" << std::endl;

					pRel->GetParent()->Dump(nIndentSize+6);

					std::cout << std::setw(nIndentSize+4) << "";
					std::cout << "}" << std::endl;
				}
*/
			}

			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "}" << std::endl;

			// Dump child relations
			//
			std::cout << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "ChildRelations:" << std::endl;
			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "{" << std::endl;

			for (idx = 0; idx < GetChildRelationCount(); idx++)
			{
				pRel = GetChildRelation(idx, m_pDatabase);

				if (pRel)
				{
					std::cout << std::setw(nIndentSize+4) << "";
					std::cout << pRel->GetMetaRelation()->GetName().c_str() << ":" << std::endl;
					std::cout << std::setw(nIndentSize+4) << "";
					std::cout << "{" << std::endl;

					for ( itrRel =  pRel->begin();
								itrRel != pRel->end();
								itrRel++)
					{
						(*itrRel)->Dump(nIndentSize+6, bDeep);
					}

					std::cout << std::setw(nIndentSize+4) << "";
					std::cout << "}" << std::endl;
				}
			}

			std::cout << std::setw(nIndentSize+2) << "";
			std::cout << "}" << std::endl;
		}

		std::cout << std::setw(nIndentSize) << "";
		std::cout << "}" << std::endl;
	}



	// Returns a string containing DatabaseName.EntityName[CKCol1][CKCol2]...[CKColn] (CK is the conceptual key),
	// or [Col1/Col2/.../Coln] if AllColumns is true (with flags at start is entity is Marked and/or New)
	std::string Entity::AsString (bool bAllColumns)
	{
		if (!bAllColumns)
			return GetMetaEntity()->GetFullName() + GetConceptualKey()->AsString();

		std::string					strValue;
		unsigned int				idx;
		ColumnPtr						pCol;

		if (IsNew())
			strValue += '^';
		if (IsMarked())
			strValue += '*';
		strValue += "[";
		for (idx = 0; idx < m_vectColumn.size(); idx++)
		{
			pCol = m_vectColumn[idx];

			if (idx != 0)
				strValue += "/";

			if (pCol)
				strValue += pCol->AsSQLString();
			else
				strValue += "?";
		}
		strValue += "]";

		return strValue;
	}



	// Return an instance Column by name
	//
	ColumnPtr Entity::GetColumn(const std::string & strColumnName)
	{
		for (unsigned int idx = 0; idx < m_vectColumn.size(); idx++)
		{
			if (strColumnName == m_vectColumn[idx]->GetMetaColumn()->GetName())
			{
				return m_vectColumn[idx];
			}
		}
		return NULL;
	}


	// Return an instance Column by MetaColumn
	//
	ColumnPtr Entity::GetColumn(MetaColumnPtr pMetaColumn)
	{
		return GetColumn(pMetaColumn->GetColumnIdx());
	}


	// Return an instance Key by name
	//
	InstanceKeyPtr Entity::GetInstanceKey(const std::string & strKeyName)
	{
		for (unsigned int idx = 0; idx < m_vectInstanceKey.size(); idx++)
			if (strKeyName == m_vectInstanceKey[idx]->GetMetaKey()->GetName())
				return m_vectInstanceKey[idx];

		return NULL;
	}


	// Return an instance Key by MetaKey
	//
	InstanceKeyPtr Entity::GetInstanceKey(MetaKeyPtr pMetaKey)
	{
		return GetInstanceKey(pMetaKey->GetMetaKeyIndex());
	}


	bool Entity::IsDirty()
	{
		ColumnPtr			pCol;
		unsigned int	idx;


		if (m_uFlags & D3_ENTITY_DIRTY)
			return true;

		for (idx = 0; idx < m_vectColumn.size(); idx++)
		{
			pCol = m_vectColumn[idx];

			if (pCol->IsDirty())
			{
				MarkDirty();
				return true;
			}
		}

		return false;
	}


	void Entity::UnMarkDirty()
	{
		ColumnPtr			pCol;
		unsigned int	idx;

		for (idx = 0; idx < m_vectColumn.size(); idx++)
		{

			pCol = m_vectColumn[idx];
			pCol->MarkClean();
		}

		m_uFlags &= ~D3_ENTITY_DIRTY;
	}


	void Entity::MarkClean()
	{
		ColumnPtr			pCol;
		unsigned int	idx;

		for (idx = 0; idx < m_vectColumn.size(); idx++)
		{
			pCol = m_vectColumn[idx];
			pCol->MarkClean();
		}

		m_uFlags = 0;
	}


	// traverses ancestor chains and returns the name of the locking user of the first that doesn't return NULL
	std::string Entity::LockedBy(RoleUserPtr pRoleUser, EntityPtr pChild)
	{
		std::string		strLockedBy;
		unsigned int	idx;
		EntityPtr			pParent;
		RelationPtr		pRelation;

		for (idx = 0; idx < GetParentRelationCount(); idx++)
		{
			pRelation = GetParentRelation(idx);

			if (pRelation)
			{
				pParent = pRelation->GetParent();

				if (pParent)
				{
					strLockedBy = pParent->LockedBy(pRoleUser, this);

					if (!strLockedBy.empty())
						return strLockedBy;
				}
			}
		}

		return strLockedBy;
	}


	// This notification is sent by the associated MetaEntity's CreateInstance() method.
	//
	// The method resizes its column and key vectors end request MetaColumn and MetaKey
	// associated with this' MetaEntity to construct instances.
	//
	void Entity::On_PostCreate()
	{
		MetaColumnPtr				pMC;
		MetaKeyPtr					pMK;
		unsigned int				idx;


		// Initialize Column vector
		//
		m_vectColumn.reserve(m_pMetaEntity->GetMetaColumnCount());

//		for (idx = 0; idx < m_vectColumn.size(); idx++)
//			m_vectColumn.push_back(NULL);

		// Initialize Key vector
		//
		m_vectInstanceKey.reserve(m_pMetaEntity->GetMetaKeyCount());

//		for (idx = 0; idx < m_vectInstanceKey.size(); idx++)
//			m_vectInstanceKey.push_back(NULL);

		// Initialize parent Relation vector
		//
		if (m_pMetaEntity->GetParentMetaRelationCount() > 0)
		{
			m_vectParentRelation.reserve(m_pMetaEntity->GetParentMetaRelationCount());

			for (idx = 0; idx < m_pMetaEntity->GetParentMetaRelationCount(); idx++)
				m_vectParentRelation.push_back(new RelationPtrMap());
		}

		// Initialize child Relation vector
		//
		if (m_pMetaEntity->GetChildMetaRelationCount() > 0)
		{
			m_vectChildRelation.reserve(m_pMetaEntity->GetChildMetaRelationCount());

			for (idx = 0; idx < m_pMetaEntity->GetChildMetaRelationCount(); idx++)
				m_vectChildRelation.push_back(new RelationPtrMap());
		}

		// Create Column instances
		//
		for (idx = 0; idx < m_pMetaEntity->GetMetaColumnCount(); idx++)
		{
			pMC = m_pMetaEntity->GetMetaColumn(idx);
			assert(pMC);
			pMC->CreateInstance(this);
		}

		// Create Key instances
		//
		for (idx = 0; idx < m_pMetaEntity->GetMetaKeyCount(); idx++)
		{
			pMK = m_pMetaEntity->GetMetaKey(idx);
			assert(pMK);
			pMK->CreateInstance(this);
		}

		m_pMetaEntity->On_InstanceCreated(this);
	}



	// A column belonging to this has been constructed, add it to the vector
	//
	void Entity::On_ColumnCreated(ColumnPtr pColumn)
	{
		assert(pColumn);
		assert(pColumn->GetEntity() == this);
		assert(pColumn->GetMetaColumn()->GetColumnIdx() == m_vectColumn.size());
//		assert(m_vectColumn[pColumn->GetMetaColumn()->GetColumnIdx()] == NULL);

		m_vectColumn.push_back(pColumn);
	}



	void Entity::On_ColumnDeleted(ColumnPtr pColumn)
	{

		assert(pColumn);
		assert(pColumn->GetEntity() == this);
		assert(pColumn->GetMetaColumn()->GetColumnIdx() < m_vectColumn.size());
		assert(m_vectColumn[pColumn->GetMetaColumn()->GetColumnIdx()] == pColumn);

		m_vectColumn[pColumn->GetMetaColumn()->GetColumnIdx()] = NULL;
	}



	void Entity::On_InstanceKeyCreated(InstanceKeyPtr pInstanceKey)
	{
		assert(pInstanceKey);
		assert(pInstanceKey->GetEntity() == this);
		assert(pInstanceKey->GetMetaKey()->GetMetaKeyIndex() == m_vectInstanceKey.size());
//		assert(m_vectInstanceKey[pInstanceKey->GetMetaKey()->GetMetaKeyIndex()] == NULL);

		m_vectInstanceKey.push_back(pInstanceKey);
	}



	void Entity::On_InstanceKeyDeleted(InstanceKeyPtr pInstanceKey)
	{
		assert(pInstanceKey);
		assert(pInstanceKey->GetEntity() == this);
		assert(pInstanceKey->GetMetaKey()->GetMetaKeyIndex() < m_vectInstanceKey.size());
		assert(m_vectInstanceKey[pInstanceKey->GetMetaKey()->GetMetaKeyIndex()] != NULL);

		m_vectInstanceKey[pInstanceKey->GetMetaKey()->GetMetaKeyIndex()] = NULL;
	}



	void Entity::On_ParentRelationCreated(RelationPtr pRelation)
	{
		RelationPtrMapPtr		pRelationMap;
		RelationPtrMapItr		itrRelationMap;
		std::auto_ptr<boost::recursive_mutex::scoped_lock>		lk;


		assert(pRelation);
		assert(pRelation->GetParentDatabase());
		assert(pRelation->GetMetaRelation()->GetChildIdx() < m_vectParentRelation.size());

		pRelationMap = m_vectParentRelation[pRelation->GetMetaRelation()->GetChildIdx()];

		assert(pRelationMap);

		if (pRelation->GetParentKey()->GetEntity()->GetMetaEntity()->IsCached())
			lk = std::auto_ptr<boost::recursive_mutex::scoped_lock>(new boost::recursive_mutex::scoped_lock(G_mtxEntity));

		itrRelationMap = pRelationMap->find(pRelation->GetParentDatabase());

		assert(itrRelationMap == pRelationMap->end());

		pRelationMap->insert(RelationPtrMap::value_type(pRelation->GetParentDatabase(), pRelation));
	}



	void Entity::On_ParentRelationDeleted(RelationPtr pRelation)
	{
		RelationPtrMapPtr		pRelationMap;
		RelationPtrMapItr		itrRelationMap;
		std::auto_ptr<boost::recursive_mutex::scoped_lock>		lk;


		assert(pRelation);
		assert(pRelation->GetParentDatabase());
		assert(pRelation->GetMetaRelation()->GetChildIdx() < m_vectParentRelation.size());

		pRelationMap = m_vectParentRelation[pRelation->GetMetaRelation()->GetChildIdx()];

		assert(pRelationMap);

		if (pRelation->GetParentKey()->GetEntity()->GetMetaEntity()->IsCached())
			lk = std::auto_ptr<boost::recursive_mutex::scoped_lock>(new boost::recursive_mutex::scoped_lock(G_mtxEntity));

		itrRelationMap = pRelationMap->find(pRelation->GetParentDatabase());

		assert(itrRelationMap != pRelationMap->end());

		pRelationMap->erase(itrRelationMap);
	}



	void Entity::On_ChildRelationCreated(RelationPtr pRelation)
	{
		RelationPtrMapPtr		pRelationMap;
		RelationPtrMapItr		itrRelationMap;
		std::auto_ptr<boost::recursive_mutex::scoped_lock>		lk;


		assert(pRelation);
		assert(pRelation->GetChildDatabase());
		assert(pRelation->GetMetaRelation()->GetParentIdx() < m_vectChildRelation.size());

		pRelationMap = m_vectChildRelation[pRelation->GetMetaRelation()->GetParentIdx()];

		assert(pRelationMap);

		if (m_pMetaEntity->IsCached())
			lk = std::auto_ptr<boost::recursive_mutex::scoped_lock>(new boost::recursive_mutex::scoped_lock(G_mtxEntity));

		itrRelationMap = pRelationMap->find(pRelation->GetChildDatabase());

		assert(itrRelationMap == pRelationMap->end());

		pRelationMap->insert(RelationPtrMap::value_type(pRelation->GetChildDatabase(), pRelation));
	}



	void Entity::On_ChildRelationDeleted(RelationPtr pRelation)
	{
		RelationPtrMapPtr		pRelationMap;
		RelationPtrMapItr		itrRelationMap;
		std::auto_ptr<boost::recursive_mutex::scoped_lock>		lk;


		assert(pRelation);
		assert(pRelation->GetChildDatabase());
		assert(pRelation->GetMetaRelation()->GetParentIdx() < m_vectChildRelation.size());

		pRelationMap = m_vectChildRelation[pRelation->GetMetaRelation()->GetParentIdx()];

		assert(pRelationMap);

		if (m_pMetaEntity->IsCached())
			lk = std::auto_ptr<boost::recursive_mutex::scoped_lock>(new boost::recursive_mutex::scoped_lock(G_mtxEntity));

		itrRelationMap = pRelationMap->find(pRelation->GetChildDatabase());

		assert(itrRelationMap != pRelationMap->end());

		pRelationMap->erase(itrRelationMap);
	}




	bool Entity::On_BeforeUpdateColumn(ColumnPtr pCol)
	{
		// If this is a primary key column, make sure we have the original key
		if (!m_pOriginalKey && pCol->IsPrimaryKeyMember())
			m_pOriginalKey = new TemporaryKey(*(GetPrimaryKey()));

		return true;
	}





	bool Entity::On_AfterUpdate(UpdateType eUT)
	{
		delete m_pOriginalKey;
		m_pOriginalKey = NULL;

		return true;
	}





	void Entity::SetParent(MetaRelationPtr pMetaRelation, EntityPtr pEntity)
	{
		RelationPtr		pRelation;


		assert(pMetaRelation);
		assert(pMetaRelation->GetChildMetaKey()->GetMetaEntity() == m_pMetaEntity);

		if (!IsPopulating())
		{
			pRelation = GetParentRelation(pMetaRelation);

			if (pRelation)
			{
				// No work if parent has not changed
				if (pRelation->GetParentKey()->GetEntity() == pEntity)
					return;

				pRelation->RemoveChild(this);
			}

			if (pEntity)
			{
				assert(pMetaRelation->GetParentMetaKey()->GetMetaEntity() == pEntity->GetMetaEntity());
				pRelation = pEntity->GetChildRelation(pMetaRelation, m_pDatabase);

				assert(pRelation);

				pRelation->AddChild(this);
			}
			else
			{
				InstanceKeyPtr	pKey = GetInstanceKey(pMetaRelation->GetChildMetaKey()->GetMetaKeyIndex());

				assert(pKey);
				pKey->On_BeforeUpdate();
				pKey->SetNull();
				pKey->On_AfterUpdate();
			}
		}
		else
		{
			InstanceKeyPtr		pFK, pPK;
			assert(pEntity);

			// While populating, simply ensure that the foreign key columns are updated
			//
			pFK = GetInstanceKey(pMetaRelation->GetChildMetaKey());
			pPK = pEntity->GetInstanceKey(pMetaRelation->GetParentMetaKey());

			assert(pFK);
			assert(pPK);

			*pFK = *pPK;
		}
	}



	// Return the instance relation object which manifests the meta relation passed in. An instance
	// key belonging to this will be the parent in the relation object returned.
	//
	RelationPtr Entity::GetChildRelation(MetaRelationPtr pMetaRelation, DatabasePtr	pChildDB)
	{
		RelationPtrMapPtr		pRelationMap;
		RelationPtrMapItr		itrRelation;
		RelationPtr					pRelation = NULL;
		DatabasePtr					pDB = pChildDB;


		// Sanity checks
		//
		assert(pMetaRelation);
		assert(pMetaRelation->GetParentMetaKey()->GetMetaEntity() == m_pMetaEntity);

		// Assume this' database if none was passed in
		//
		if (!pDB)
			pDB = m_pDatabase;

		// Special treatment for cached entities
		//
		if (pMetaRelation->GetChildMetaKey()->GetMetaEntity()->IsCached())
		{
			pDB = pMetaRelation->GetChildMetaKey()->GetMetaEntity()->GetMetaDatabase()->GetGlobalDatabase();
		}
		else
		{
			// Handle cross database relations
			//
			if (pDB->GetMetaDatabase() != pMetaRelation->GetChildMetaKey()->GetMetaEntity()->GetMetaDatabase())
			{
				pDB = pDB->GetDatabaseWorkspace()->GetDatabase(pMetaRelation->GetChildMetaKey()->GetMetaEntity()->GetMetaDatabase());
			}
		}

		// Get the correct RelationPtrMap
		//
		pRelationMap = m_vectChildRelation[pMetaRelation->GetParentIdx()];
		assert(pRelationMap);

		// Find the RelationPtr
		//
		itrRelation = pRelationMap->find(pDB);

		if (itrRelation == pRelationMap->end())
		{
			// Create a new Relation and add it to the map
			//
			pRelation = pMetaRelation->CreateInstance(this, pDB);
		}
		else
		{
			pRelation = itrRelation->second;
		}

		return pRelation;
	}


	// Return the instance Relation matching the MetaRelation passed in. In the relation returned,
	// an instance key of this will be a child in the relation.
	//
	RelationPtr Entity::GetParentRelation(MetaRelationPtr pMetaRelation, DatabasePtr pParentDB)
	{
		RelationPtrMapPtr		pRelationMap;
		RelationPtrMapItr		itrRelation;
		RelationPtr					pRelation = NULL;
		DatabasePtr					pDB = pParentDB;


		// Sanity checks
		//
		assert(pMetaRelation);
		assert(pMetaRelation->GetChildMetaKey()->GetMetaEntity() == m_pMetaEntity);

		// Assume this' database if none was passed in
		//
		if (!pDB)
			pDB = m_pDatabase;

		// Special treatment for cached entities
		//
		if (pMetaRelation->GetParentMetaKey()->GetMetaEntity()->IsCached())
		{
			pDB = pMetaRelation->GetParentMetaKey()->GetMetaEntity()->GetMetaDatabase()->GetGlobalDatabase();
		}
		else
		{
			// Handle cross database relations
			//
			if (pDB->GetMetaDatabase() != pMetaRelation->GetParentMetaKey()->GetMetaEntity()->GetMetaDatabase())
			{
				pDB = pDB->GetDatabaseWorkspace()->GetDatabase(pMetaRelation->GetParentMetaKey()->GetMetaEntity()->GetMetaDatabase());
			}

		}

		// Get the correct RelationPtrMap
		//
		pRelationMap = m_vectParentRelation[pMetaRelation->GetChildIdx()];
		assert(pRelationMap);

		// Find the RelationPtr
		//
		itrRelation = pRelationMap->find(pDB);

		if (itrRelation == pRelationMap->end())
		{
			InstanceKeyPtr	pChildKey, pParentKey;

			// Search the matching parent key
			//
			pChildKey = GetInstanceKey(pMetaRelation->GetChildMetaKey()->GetMetaKeyIndex());

			// The parent key part of this foreign key must not be NULL
			//
			if (!pChildKey->IsKeyPartNull(pMetaRelation->GetParentMetaKey()))
			{
				pParentKey = pMetaRelation->GetParentMetaKey()->FindInstanceKey(pChildKey);

				// Attempt to add this to it
				//
				if (pParentKey)
				{
					pRelation = pParentKey->GetEntity()->GetChildRelation(pMetaRelation, m_pDatabase);
					pRelation->AddChild(this);
					itrRelation = pRelationMap->find(pDB);
				}
			}
		}

		if (itrRelation == pRelationMap->end())
			return NULL;

		pRelation = itrRelation->second;


		return pRelation;
	}



	void Entity::On_BeforePopulatingObject()
	{
		MarkPopulating();

		for (unsigned int idx = 0; idx < m_vectInstanceKey.size(); idx++)
			m_vectInstanceKey[idx]->On_BeforeUpdate();
	}




	void Entity::On_AfterPopulatingObject(bool bMarkClean)
	{
		for (unsigned int idx = 0; idx < m_vectInstanceKey.size(); idx++)
			m_vectInstanceKey[idx]->On_AfterUpdate();

		UnMarkPopulating();

		if (bMarkClean)
			MarkClean();
	}




	void Entity::On_AddedToResultSet(ResultSetPtr pResultSet)
	{
		if (!m_pListResultSet)
			m_pListResultSet = new ResultSetPtrList();

		m_pListResultSet->push_back(pResultSet);
	}




	void Entity::On_RemovedFromResultSet(ResultSetPtr pResultSet)
	{
		assert(m_pListResultSet);
		m_pListResultSet->remove(pResultSet);
	}




	std::ostream & Entity::AsSQL(std::ostream & ostrm)
	{
		unsigned int	idx;

		// extract columns
		ostrm << "INSERT INTO " << GetMetaEntity()->GetName() << "(";

		std::string strColumnNames, strColumnValues;

		for (idx = 0; idx < m_vectColumn.size(); idx++)
		{
			if(idx)
			{
				strColumnValues += ",";
				strColumnNames += ",";
			}

			strColumnNames += m_vectColumn[idx]->GetMetaColumn()->GetName();
			strColumnValues += m_vectColumn[idx]->AsSQLString();
		}

		ostrm << strColumnNames << ") VALUES(" << strColumnValues << ")";

		return ostrm;
	}




	std::ostream & Entity::AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, bool * pFirstSibling)
	{
		RolePtr										pRole = pRoleUser ? pRoleUser->GetRole() : NULL;
		MetaEntity::Permissions		permissions = pRole ? pRole->GetPermissions(m_pMetaEntity) : m_pMetaEntity->GetPermissions();
		ColumnPtrVectItr					itr;
		ColumnPtr									pCol;
		bool											bFirst = true;


		if (permissions & MetaEntity::Permissions::Select)
		{
			if (pFirstSibling)
			{
				if (*pFirstSibling)
					*pFirstSibling = false;
				else
					ostrm << ',';
			}

			ostrm << '{';

#if defined (USING_AVLB)
			std::string strLockedBy = LockedBy(pRoleUser);

			if (!strLockedBy.empty())
				ostrm << "\"lockedBy\":\"" << strLockedBy << "\",";
			else
				ostrm << "\"lockedBy\":null,";
#endif

			// We supply extended meta columns if there are any
			for ( itr =  m_vectColumn.begin();
						itr != m_vectColumn.end();
						itr++)
			{
				pCol = *itr;

				if (pCol->GetMetaColumn()->IsDerived())
				{
					if (bFirst)
					{
						ostrm << "\"ExtendedMetaColumns\":[";
						bFirst = false;
					}
					else
						ostrm << ',';

					pCol->GetMetaColumn()->AsJSON(pRoleUser, ostrm);
				}
			}

			if (!bFirst) ostrm << "],";

			ostrm << "\"Columns\":[";
			ColumnsAsJSON(pRoleUser, ostrm);
			ostrm << "],";

			ostrm << "\"ConceptualKey\":\"";
			ostrm << APALUtil::base64_encode(this->GetConceptualKey()->AsString());
			ostrm << "\",";

			ostrm << "\"ParentRelations\":[";
			ParentRelationsAsJSON(pRoleUser, ostrm);
			ostrm << "]}";
		}

		return ostrm;
	}



	void Entity::ColumnsAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm)
	{
		ColumnPtr				pC;
		bool						bFirstChild = true;


		for (unsigned int idx = 0; idx < m_vectColumn.size(); idx++)
		{
			pC = m_vectColumn[idx];
			pC->AsJSON(pRoleUser, ostrm, &bFirstChild);
		}
	}



	void Entity::KeysAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm)
	{
		KeyPtr					pK;
		bool						bFirstChild = true;


		for (unsigned int idx = 0; idx < m_vectInstanceKey.size(); idx++)
		{
			pK = m_vectInstanceKey[idx];
			pK->AsJSON(pRoleUser, ostrm, &bFirstChild);
		}
	}



	void Entity::ParentRelationsAsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm)
	{
		RolePtr					pRole = pRoleUser ? pRoleUser->GetRole() : NULL;
		MetaRelationPtr	pMR;
		RelationPtr			pR;
		bool						bFirstChild = true;


		for (unsigned int idx = 0; idx < m_pMetaEntity->GetParentMetaRelationCount(); idx++)
		{
			pMR = m_pMetaEntity->GetParentMetaRelation(idx);

			// Only include if Role has access
			if (!pRole || pRole->CanReadParent(pMR))
			{
				pR = GetParentRelation(pMR);

				// Parent relations can be NULL
				if (pR)
				{
					pR->AsJSON(NULL, ostrm, &bFirstChild);
				}
				else
				{
					if (bFirstChild)
						bFirstChild = false;
					else
						ostrm << ',';

					ostrm << "null";
				}
			}
		}
	}



	//! Mini dump returns a string containing basic info about this and all its descendents
	/*! The string returned is properly formated so that it can be dumped to a stl ostream
	*/
	std::string Entity::DumpTree()
	{
		std::ostringstream		ostrm;
		std::set<EntityPtr>		setProcessed;

		DumpTree(ostrm, setProcessed, 0);

		return ostrm.str();
	}



	//! DoMiniDump helper
	void Entity::DumpTree(std::ostringstream & ostrm, std::set<EntityPtr> & setProcessed, int iLevel)
	{
		if (setProcessed.find(this) == setProcessed.end())
		{
			unsigned int				idx;
			RelationPtr					pRel;
			Relation::iterator	itr;

			setProcessed.insert(this);

			ostrm << "[db=0x" << m_pDatabase << ", this=0x" << this << ", state=";

			if (IsNew())
				ostrm << "new";
			else if (IsDirty())
				ostrm << "mod";
			else
				ostrm << "cln";

			ostrm << "] " << std::string(iLevel*2, ' ') << m_pMetaEntity->GetName() << "(pk=" << this->GetPrimaryKey()->AsString() << ")\n";

			for (idx = 0; idx < GetChildRelationCount(); idx++)
			{
				pRel = GetChildRelation(idx, m_pDatabase);

				for ( itr =  pRel->begin();
							itr != pRel->end();
							itr++)
				{
					(*itr)->DumpTree(ostrm, setProcessed, iLevel+1);
				}
			}
		}
	}



	EntityPtr Entity::Clone(SessionPtr pSession, DatabaseWorkspacePtr pDBWS, Json::Value* pJSONChanges)
	{
		CloningController*				pController = NULL;
		MetaEntityPtrList					listME = m_pDatabase->GetMetaDatabase()->GetDependencyOrderedMetaEntities();
		MetaEntityPtrListItr			itrME;
		MetaRelationPtr						pMR;
		RelationPtr								pIR;
		unsigned int							idx;
		EntityPtr									pClone = NULL, pClonedEntity, pOrigEntity, pClonedParent;
		EntityCloneMap::iterator	itrClones;
		DatabasePtr								pDB = pDBWS->GetDatabase(m_pDatabase->GetMetaDatabase());

		try
		{
			// Create the cloning controller
			pController = CreateCloningController(pSession, pDBWS, pJSONChanges);

			// Give subclasses a chance to load additional objects
			pController->On_BeforeCloning();

			// Mark this and all loaded descendents as cloning
			pClone = CreateClone(pController);

			if (pClone)
			{
				// Send an On_AfterPopulatingObject message to each cloned object but do this in the right order
				for (itrME = listME.begin(); itrME != listME.end(); itrME++)
				{
					for (itrClones = pController->m_mapClones.begin(); itrClones != pController->m_mapClones.end(); itrClones++)
					{
						pOrigEntity = itrClones->first;
						pClonedEntity = itrClones->second;

						if (*itrME == pClonedEntity->m_pMetaEntity)
						{
							pController->On_BeforeFinalisingClone(pOrigEntity, pClonedEntity);
							pClonedEntity->On_AfterPopulatingObject(false);
						}
					}
				}

				// Fix parents (we need the new parent)
				for (itrME = listME.begin(); itrME != listME.end(); itrME++)
				{
					for (itrClones = pController->m_mapClones.begin(); itrClones != pController->m_mapClones.end(); itrClones++)
					{
						pOrigEntity = itrClones->first;
						pClonedEntity = itrClones->second;

						if (*itrME == pOrigEntity->m_pMetaEntity)
						{
							// Check if any of this' parents was cloned
							for (idx = 0; idx < pOrigEntity->m_pMetaEntity->GetParentMetaRelationCount(); idx++)
							{
								pMR = pOrigEntity->m_pMetaEntity->GetParentMetaRelation(idx);
								pIR = pOrigEntity->GetParentRelation(pMR, pDB);

								if (pIR)
								{
									pClonedParent = pController->m_mapClones[pIR->GetParent()];

									if (pClonedParent)
										pClonedEntity->SetParent(pMR, pClonedParent);
								}
							}
						}
					}
				}

				// Let's update it all
				pDB->TraceAll();
				pDB->BeginTransaction();
				pClone->Update(pDB, true);
				pDB->CommitTransaction();
				pDB->TraceNone();
			}
		}
		catch(...)
		{
			try
			{
				if (m_pDatabase->HasTransaction())
					m_pDatabase->RollbackTransaction();

				delete pClone;
				pClone = NULL;
			}
			catch(...)
			{
				pClone = NULL;
			}

			GenericExceptionHandler("Entity::Clone(): Operation failed. ");
		}

		return pClone;
	}



	EntityPtr Entity::CreateClone(CloningController* pController, EntityPtr pClonedParent, MetaRelationPtr pParentMR)
	{
		EntityPtr									pClone = NULL;
		DatabasePtr								pDB = pController->m_pDBWS->GetDatabase(m_pDatabase->GetMetaDatabase());
		RoleUserPtr								pRoleUser = pController->m_pSession ? pController->m_pSession->GetRoleUser() : NULL;
		MetaRelationPtr						pMR;
		RelationPtr								pIR;
		EntityPtr									pOrigChild;
		unsigned int							idx;
		InstanceKeyPtrSetItr			itr;


		try
		{
			// If user is not entitled to insert this or RLS prevents access, don't clone this
			if (	pController->m_pSession &&
					(	!(pController->m_pSession->GetRoleUser()->GetRole()->GetPermissions(m_pMetaEntity) & D3::MetaEntity::Permissions::Insert) ||
						!pController->m_pSession->GetRoleUser()->GetUser()->HasAccess(this)))
				return NULL;

			// Have we cloned this already?
			pClone = pController->m_mapClones[this];

			if (!pClone)
			{
				if (!pController->WantCloned(this))
					return NULL;

				// Let's create a new instance
				pController->m_mapClones[this] = pClone = m_pMetaEntity->CreateInstance(pDB);

				// Mark it populating
				pClone->On_BeforePopulatingObject();

				// Copy values
				for (idx = 0; idx < pClone->m_vectColumn.size(); idx++)
				{
					if (!pClone->m_vectColumn[idx]->GetMetaColumn()->IsAutoNum())
						pClone->m_vectColumn[idx]->Assign(*(m_vectColumn[idx]));
				}

				// If we received passed in JSON values and this is the root of the object hierarchy to clone, assign the passed in values
				if (pController->m_pOrigRoot == this && pController->m_pJSONChanges)
					pClone->SetValues(pRoleUser, *(pController->m_pJSONChanges));
			}

			// Set the parent
			if (pClonedParent && pParentMR)
				pClone->SetParent(pParentMR, pClonedParent);

			// Clone each child
			for (idx = 0; idx < m_pMetaEntity->GetChildMetaRelationCount(); idx++)
			{
				pMR = m_pMetaEntity->GetChildMetaRelation(idx);

				if (pController->WantCloned(pMR))
				{
					pIR = GetChildRelation(pMR, pDB);

					for ( itr =  pIR->GetChildKeys().begin();
								itr != pIR->GetChildKeys().end();
								itr++)
					{
						pOrigChild = (*itr)->GetEntity();
						pOrigChild->CreateClone(pController, pClone, pMR);
					}
				}
			}
		}
		catch(...)
		{
			delete pClone;
			throw;
		}

		return pClone;
	}



	bool Entity::Copy(ObjectPtr pObj)
	{
		unsigned int	idx;
		EntityPtr			pRHS;


		if (!pObj || !pObj->IsKindOf(Entity::ClassObject()))
			throw std::runtime_error("Entity::Copy(rhs): rhs is not a valid object. ");

		pRHS = (EntityPtr) pObj;

		if (GetMetaEntity() != pRHS->GetMetaEntity())
			throw std::runtime_error("Entity::Copy(rhs): rhs must be an instance of the same meta entity as this. ");

		for (idx = 0; idx < m_vectColumn.size(); idx++)
		{
			Column&		colLHS(*(m_vectColumn[idx]));
			Column&		colRHS(*(pRHS->m_vectColumn[idx]));

			colLHS.Assign(colRHS);
//			m_vectColumn[idx]->SetValue(pRHS->m_vectColumn[idx]);
		}

		return true;
	}



	void Entity::SetValues(RoleUserPtr pRoleUser, Json::Value & jsonChanges)
	{
		unsigned int	idx;
		ColumnPtr			pIC;


		for (idx = 0; idx < m_vectColumn.size(); idx++)
		{
			pIC = m_vectColumn[idx];

			if (jsonChanges.isMember(pIC->GetMetaColumn()->GetName()))
				pIC->SetValue(jsonChanges[pIC->GetMetaColumn()->GetName()]);
		}
	}




#ifdef D3_OBJECT_LINKS
	// Tis method returns a list of all linked objects meeting the criteria passed in.
	//
	// Parameters:
	//
	// strToObjectType: If specified, the returned collection will only
	//                  contain objects of this type, if any.
	//
	// nLinkType:       If not 0, only objects linked to this with the
	//                  specified link type will be returned. If 0,
	//                  objects returned can have any link type.
	//
	// Note: Clients MUST delete the returned collection when no longer required.
	//
	EntityListPtr Entity::GetToObjectLinks(const std::string & strToObjectType, short nLinkType)
	{
		EntityListPtr				pEL = new EntityList();
		EntityPtr						pTo;
		ObjectLinkPtr			pOL;
		ObjectLinkListItr	itrOL;


		if (!pEL)
			return NULL;

		if (!m_pObjectLinks)
			return pEL;

		// Browse all ObjectLink objects
		//
		for (itrOL = m_pObjectLinks->begin(); itrOL != m_pObjectLinks->end(); itrOL++)
		{
			pOL = *itrOL;

			pTo = pOL->GetToObject();

			// Only add the object to the list if it exists, is not marked for deletion
			// and matches the requested criteria
			//
			if (pTo && !pOL->IsDeleted() &&
				 (strToObjectType.empty() || pTo->GetTableName() == strToObjectType) &&
				 (nLinkType == 0          || pOL->GetLinkType()  == nLinkType))
				pEL->push_back(pTo);
		}

		return pEL;
	}


	// This method returns a list of all linked objects meeting the criteria passed in.
	//
	// Parameters:
	//
	// strFromObjectType: If specified, the returned collection will only
	//										contain objects of this type, if any.
	//
	// nLinkType:       If not 0, only objects linked to this with the
	//                  specified link type will be returned. If 0,
	//                  objects returned can have any link type.
	//
	// Note: Clients MUST delete the returned collection when no longer required.
	//
	EntityListPtr Entity::GetFromObjectLinks(const std::string & strFromObjectType, short nLinkType)
	{
		EntityListPtr				pEL = new EntityList();
		EntityPtr						pFrom, pTo;
		ObjectLinkPtr			pOL;
		ObjectLinkMapItr		itrOL;


		if (!pEL)
			return NULL;

		// Browse all ObjectLink objects
		//
		for ( itrOL  = m_pDatabase->GetObjectLinks().begin();
					itrOL != m_pDatabase->GetObjectLinks().end();
					itrOL++)
		{
			pOL = (*itrOL).second;

			pTo = pOL->GetToObject();

			if (pTo == this)
			{
				pFrom = pOL->GetFromObject();

				// Only add the object to the list if it exists, is not marked for deletion
				// and matches the requested criteria
				//
				if ( pFrom &&
						!pOL->IsDeleted() &&
					 (strFromObjectType.empty() || pFrom->GetTableName()	== strFromObjectType) &&
					 (nLinkType == 0						|| pOL->GetLinkType()			== nLinkType))
					pEL->push_back(pFrom);
			}
		}

		return pEL;
	}


	ObjectLinkPtr Entity::GetToObjectLink(EntityPtr pToObject, short nLinkType)
	{
		EntityPtr						pTo;
		ObjectLinkPtr			pOL;
		ObjectLinkListItr	itrOL;


		if (!m_pObjectLinks)
			return NULL;

		// Search in this' ObjectLink list
		//
		for ( itrOL = m_pObjectLinks->begin();
					itrOL != m_pObjectLinks->end();
					itrOL++)
		{
			pOL = *itrOL;

			pTo = pOL->GetToObject();

			if (pTo &&
					pTo->GetTableName()	== pToObject->GetTableName()	&&
					pTo->GetID()				== pToObject->GetID()					&&
					pOL->GetLinkType()	== nLinkType)
				return pOL;
		}

		return NULL;
	}


	ObjectLinkPtr Entity::GetFromObjectLink(EntityPtr pFromObject, short nLinkType)
	{
		EntityPtr						pFrom;
		EntityPtr						pTo;
		ObjectLinkPtr			pOL;
		ObjectLinkMapItr		itrOL;


		// Search in the databases ObjectLink map
		//
		for ( itrOL  = m_pDatabase->GetObjectLinks().begin();
					itrOL != m_pDatabase->GetObjectLinks().end();
					itrOL++)
		{
			pOL = (*itrOL).second;

			pTo = pOL->GetToObject();

			if (pTo == this)
			{
				pFrom = pOL->GetFromObject();

				if (pFrom &&
						pFrom->GetTableName()	== pFromObject->GetTableName()	&&
						pFrom->GetID()				== pFromObject->GetID()					&&
						pOL->GetLinkType()		== nLinkType)
					return pOL;
			}
		}

		return NULL;
	}


	ObjectLinkPtr Entity::AddObjectLink(EntityPtr pToObject, short nLinkType)
	{
		ObjectLinkPtr			pOL;


		pOL = GetToObjectLink(pToObject, nLinkType);

		if (!pOL)
		{
			pOL = new ObjectLink(m_pDatabase, this, pToObject);

			if (pOL)
			{
				pOL->SetLinkType(nLinkType);
				MarkDirty();
			}
		}

		// If this object link is marked deleted, unmark it
		//
		if (pOL->IsDeleted())
		{
			((EntityPtr) pOL)->m_uFlags &= ~APP3ENTITY_DELETE;
			MarkDirty();
		}

		return pOL;
	}


	bool Entity::RemoveObjectLink(EntityPtr pToObject, short nLinkType)
	{
		ObjectLinkPtr			pOL;


		pOL = GetToObjectLink(pToObject, nLinkType);

		if (!pOL)
			return true;

		// Marked the link deleted
		//
		pOL->MarkDelete();
		MarkDirty();

		return true;
	}


	bool Entity::AddObjectLink(ObjectLinkPtr pOL)
	{
		if (HasObjectLink(pOL))
			return true;

		if (!m_pObjectLinks)
			m_pObjectLinks = new ObjectLinkList();

		if (!m_pObjectLinks)
			return false;

		m_pObjectLinks->push_back(pOL);

		return true;
	}


	bool Entity::RemoveObjectLink(ObjectLinkPtr pOL)
	{
		if (!m_pObjectLinks)
			return false;

		m_pObjectLinks->remove(pOL);

		return true;
	}

	bool Entity::HasObjectLink(ObjectLinkPtr pNewOL)
	{
		ObjectLinkPtr			pOL;
		ObjectLinkListItr	itrOL;


		if (!m_pObjectLinks || m_pObjectLinks->empty())
			return false;

		// Browse all ObjectLink objects
		//
		for (	itrOL  = m_pObjectLinks->begin();
					itrOL != m_pObjectLinks->end();
					itrOL++)
		{
			pOL = *itrOL;

			if (pOL == pNewOL)
				return true;
		}


		return false;
	}

	bool Entity::RemoveAllObjectLinks(const std::string & strToObjectType, short nLinkType)
	{
		ObjectLinkPtr			pOL;
		ObjectLinkListItr	itrOL;


		if (!m_pObjectLinks)
			return true;

		// If this link already exists, return true
		//
		for (itrOL = m_pObjectLinks->begin(); itrOL != m_pObjectLinks->end(); itrOL++)
		{
			pOL = *itrOL;

			if ((strToObjectType.empty() || pOL->GetToObjectType() == strToObjectType)	&&
					(nLinkType == 0          || pOL->GetLinkType()     == nLinkType))
			{
				pOL->MarkDelete();
				MarkDirty();
			}
		}

		return true;
	}

	bool Entity::CopyObjectLinks(Entity & anotherEntity)
	{
		ObjectLinkPtr			pOL, pNewOL;
		ObjectLinkListItr	itrOL;


		if (!RemoveAllObjectLinks())
			return false;

		if (!anotherEntity.m_pObjectLinks)
		{
			delete m_pObjectLinks;
			return true;
		}


		if (!m_pObjectLinks)
		{
			m_pObjectLinks = new ObjectLinkList();

			if (!m_pObjectLinks)
				return false;
		}

		// If this link already exists, return true
		//
		for (itrOL = anotherEntity.m_pObjectLinks->begin(); itrOL != anotherEntity.m_pObjectLinks->end(); itrOL++)
		{
			pOL = *itrOL;

			pNewOL = new ObjectLink(m_pDatabase, this, pOL);

			if (!pNewOL)
				return false;

			MarkDirty();
		}

		return true;
	}


#endif

} // end namespace D3
