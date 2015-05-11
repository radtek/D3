// MODULE: Relation source
//;
// ===========================================================
// File Info:
//
// $Author: daellenj $
// $Date: 2014/05/26 08:47:17 $
// $Revision: 1.38 $
//
// MetaRelation: This class describes relationships between
// meta keys.
//
// BaseRelation: This class holds the details of an actual
// relation
//
// Relation<T srce, T trgt>: This class holds the details
// of an actual relation
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
// @@End
// @@Includes
#include "Entity.h"
#include "Column.h"
#include "Key.h"
#include "Relation.h"
#include "D3Funcs.h"
#include "Session.h"

#include <Codec.h>

namespace D3
{
	// ==========================================================================
	// MetaRelation::Flags class implementation
	//

	// Implement Flags
	BITS_IMPL(MetaRelation, Flags);

	// Primitive mask values
	PRIMITIVEMASK_IMPL(MetaRelation, Flags, CascadeDelete,		0x00000001);
	PRIMITIVEMASK_IMPL(MetaRelation, Flags, CascadeClearRef,	0x00000002);
	PRIMITIVEMASK_IMPL(MetaRelation, Flags, PreventDelete,		0x00000004);
	PRIMITIVEMASK_IMPL(MetaRelation, Flags, ParentSwitch,			0x00000008);
	PRIMITIVEMASK_IMPL(MetaRelation, Flags, ChildSwitch,			0x00000010);
	PRIMITIVEMASK_IMPL(MetaRelation, Flags, HiddenParent,			0x00000020);
	PRIMITIVEMASK_IMPL(MetaRelation, Flags, HiddenChildren,		0x00000040);

	// Combo mask values
	COMBOMASK_IMPL(MetaRelation, Flags, Default, MetaRelation::Flags::CascadeDelete);
	COMBOMASK_IMPL(MetaRelation, Flags, Hidden,  MetaRelation::Flags::HiddenParent | MetaRelation::Flags::HiddenChildren);


	// ===========================================================
	// MetaRelation class implementation
	//

	// Statics:
	//
	RelationID																MetaRelation::M_uNextInternalID = RELATION_ID_MAX;
	MetaRelationPtrMap												MetaRelation::M_mapMetaRelation;

	// Standard D3 Stuff
	//
	D3_CLASS_IMPL(MetaRelation, Object);

	MetaRelation::MetaRelation()
	{
		assert(false);
	}



	MetaRelation::MetaRelation(RelationID uID, const std::string & strName, const std::string & strReverseName, MetaKeyPtr pParentKey, MetaKeyPtr pChildKey, const std::string & strInstanceClassName, const Flags::Mask & flags, MetaColumnPtr pSwitchColumn, const std::string & strSwitchColumnValue)
		: m_pInstanceClass(NULL),
			m_uID(uID),
			m_strName(strName),
			m_strReverseName(strReverseName),
			m_pParentKey(pParentKey),
			m_pChildKey(pChildKey),
			m_uParentIdx(D3_UNDEFINED_ID),
			m_uChildIdx(D3_UNDEFINED_ID),
			m_Flags(flags),
			m_pSwitchColumn(NULL)
	{
		Init(strInstanceClassName, pSwitchColumn, strSwitchColumnValue);
	}



	MetaRelation::MetaRelation(const std::string & strName, const std::string & strReverseName, MetaKeyPtr pParentKey, MetaKeyPtr pChildKey, const std::string & strInstanceClassName, const Flags::Mask & flags, MetaColumnPtr pSwitchColumn, const std::string & strSwitchColumnValue)
		: m_pInstanceClass(NULL),
			m_uID(M_uNextInternalID--),
			m_strName(strName),
			m_strReverseName(strReverseName),
			m_pParentKey(pParentKey),
			m_pChildKey(pChildKey),
			m_uParentIdx(D3_UNDEFINED_ID),
			m_uChildIdx(D3_UNDEFINED_ID),
			m_Flags(flags),
			m_pSwitchColumn(NULL)
	{
		Init(strInstanceClassName, pSwitchColumn, strSwitchColumnValue);
	}



	// Root relation ctor (not yet implemented)
	//
	MetaRelation::~MetaRelation()
	{
		if (m_pParentKey)
			m_pParentKey->On_ChildMetaRelationDeleted(this);

		if (m_pParentKey->GetMetaEntity())
			m_pParentKey->GetMetaEntity()->On_ChildMetaRelationDeleted(this);

		if (m_pChildKey)
			m_pChildKey->On_ParentMetaRelationDeleted(this);

		if (m_pChildKey->GetMetaEntity())
			m_pChildKey->GetMetaEntity()->On_ParentMetaRelationDeleted(this);

		if (m_pSwitchColumn)
			m_pSwitchColumn->GetMetaColumn()->On_MetaRelationDeleted(this);

		delete m_pSwitchColumn;

		M_mapMetaRelation.erase(m_uID);
	}



	void MetaRelation::Init(const std::string & strInstanceClassName, MetaColumnPtr pSwitchColumn, const std::string & strSwitchColumnValue)
	{
		unsigned int			idx;
		MetaRelationPtr		pMR;


		std::string			strClassName = strInstanceClassName;


		if (strClassName.empty())
			strClassName = "Relation";

		try
		{
			m_pInstanceClass = &(Class::Of(strClassName));
		}
		catch(Exception & e)
		{
			e.AddMessage("MetaRelation::MetaRelation(): Error creating MetaRelation from key %s to key %s, the instance class name %s is not valid, using default class 'Relation'.", m_pParentKey->GetName().c_str(), m_pChildKey->GetName().c_str(), strClassName.c_str());
			e.LogError();
			m_pInstanceClass = (ClassPtr) &Relation::ClassObject();
		}

		// Create a switch if necessary
		if (IsSwitched())
		{
			if (!pSwitchColumn || strSwitchColumnValue.empty())
			{
				ReportError("MetaRelation::MetaRelation(): Relation %s is marked as switched but no switch column is specified.", GetFullName().c_str());
			}
			else
			{
				if (IsParentSwitch())
				{
					if (pSwitchColumn->GetMetaEntity() != m_pParentKey->GetMetaEntity())
					{
						ReportError("MetaRelation::MetaRelation(): Relation %s is marked as parent switch but the switch column specified is not from the parent.", GetFullName().c_str());
					}
					else
					{
						m_pSwitchColumn = pSwitchColumn->CreateInstance();
						m_pSwitchColumn->SetValue(strSwitchColumnValue);
						pSwitchColumn->On_MetaRelationCreated(this);
					}
				}
				else
				{
					if (pSwitchColumn->GetMetaEntity() != m_pChildKey->GetMetaEntity())
					{
						ReportError("MetaRelation::MetaRelation(): Relation %s is marked as child switch but the switch column specified is not from the child.", GetFullName().c_str());
					}
					else
					{
						m_pSwitchColumn = pSwitchColumn->CreateInstance();
						m_pSwitchColumn->SetValue(strSwitchColumnValue);
						pSwitchColumn->On_MetaRelationCreated(this);
					}
				}
			}
		}

		if (!m_pParentKey)
		{
			// This is a root relation (implement later)
			//
			assert(false);
		}
		else
		{
			// This is a normal relation
			//

			// Ensure the relation type is only one of CascadeDelete, CascadeClearRef and PreventDelete
			//
			idx = 0;
			idx += IsCascadeDelete() ? 1 : 0;
			idx += IsCascadeClearRef() ? 1 : 0;
			idx += IsPreventDelete() ? 1 : 0;

			if (idx != 1)
				ReportError("MetaRelation::MetaRelation(): MetaRelation %s.%s either specifies none or more than one of CascadeDelete, CascadeClearRef and PreventDelete!", m_pChildKey->GetMetaEntity()->GetName().c_str(), m_strName.c_str());

			// some sanity checks
			//
			assert(m_pParentKey);
			assert(m_pChildKey);

			// The parent key must be unique to resolve relations
			//
			if (!m_pParentKey->IsUnique())
			{
				ReportError("MetaRelation::MetaRelation(): Can't use key %s as source key in a relationship. Parent keys must be unique!", m_pParentKey->GetFullName().c_str());
			}

			// Make sure the two keys are compatible
			//
			if (!VerifyKeyCompatibility())
				ReportError("MetaRelation::MetaRelation(): ParentKey %s is incompatible with ChildKey %s!", m_pParentKey->GetFullName().c_str(), m_pChildKey->GetFullName().c_str());

			// Make sure that we have the correct flag
			//
			if (!m_pParentKey)
			{
				m_Flags = Flags::Mask();
			}
			else
			{
				if (m_pChildKey->IsMandatory() && IsCascadeClearRef())
				{
					ReportWarning("MetaRelation::MetaRelation(): Relation '%s' cannot be set to CascadeClearRef. Changing to CascadeDelete.", GetFullName().c_str());
					m_Flags &= ~Flags::CascadeClearRef;
					m_Flags |= Flags::CascadeDelete;
				}

				if (!IsCascadeDelete() && !IsPreventDelete() && !IsCascadeClearRef())
				{
					if (m_pChildKey->IsMandatory())
					{
						ReportWarning("MetaRelation::MetaRelation(): Relation '%s' is not marked as CascadeDelete, PreventDelete or CascadeClearRef. Assuming CascadeDelete.", GetFullName().c_str());
						m_Flags |= Flags::CascadeDelete;
					}
					else
					{
						ReportWarning("MetaRelation::MetaRelation(): Relation '%s' is not marked as CascadeDelete, PreventDelete or CascadeClearRef. Assuming CascadeClearRef.", GetFullName().c_str());
						m_Flags |= Flags::CascadeClearRef;
					}
				}
			}

			// Check if this is a relation from a non cached entity to a cached entity
			//
			if (!m_pParentKey)
			{
				if (!m_pParentKey->GetMetaEntity()->IsCached() && m_pChildKey->GetMetaEntity()->IsCached())
					ReportWarning("MetaRelation::MetaRelation(): Relation '%s' is from a non-cached parent to a cached child. You should avoid such configurations.", GetFullName().c_str());
			}

			// Check if any MetaRelation already exists between the same two keys and
			// Make sure relation name is unique
			//
			for (idx = 0; idx < m_pParentKey->GetMetaEntity()->GetChildMetaRelations()->size(); idx++)
			{
				pMR = m_pParentKey->GetMetaEntity()->GetChildMetaRelation(idx);

				if (pMR->GetParentMetaKey() == m_pParentKey &&
						pMR->GetChildMetaKey()  == m_pChildKey)
				{
					// This is OK only for switched relations
					//
					if (!IsSwitched() || !pMR->IsSwitched())
						ReportError("MetaRelation::MetaRelation(): Relation %s and %s are relations between the same two keys but at least one is not switched. More than one relations between the same keys only makes sense if they are switched.", GetFullName().c_str(), pMR->GetFullName().c_str());
				}

				// Make sure the relation name is unambiguous
				//
				if (pMR->GetName() == m_strName)
				{
					// Ok only if both relations are switched
					//
					if (!IsSwitched() || !pMR->IsSwitched())
						ReportError("MetaRelation::MetaRelation(): Relation %s and %s have the same name but at least one of the relations is not switched. This causes ambiguities.", GetFullName().c_str(), pMR->GetFullName().c_str());
				}
			}

			// Make sure relation reverse name is unique
			//
			for (idx = 0; idx < m_pChildKey->GetMetaEntity()->GetParentMetaRelations()->size(); idx++)
			{
				pMR = m_pChildKey->GetMetaEntity()->GetParentMetaRelation(idx);

				if (pMR->GetReverseName() == m_strReverseName)
				{
					// Ok only if both relations are switched
					//
					if (!IsSwitched() || !pMR->IsSwitched())
						ReportError("MetaRelation::MetaRelation(): Relation %s and %s have the same reverse name but at least one of the relations is not switched. This causes ambiguities.", GetFullName().c_str(), pMR->GetFullName().c_str());
				}
			}

			// Notify MetaKey
			//
			if (m_pParentKey)
			{
				m_pParentKey->On_ChildMetaRelationCreated(this);
				m_pChildKey->On_ParentMetaRelationCreated(this);

				m_pParentKey->GetMetaEntity()->On_ChildMetaRelationCreated(this);
				m_pChildKey->GetMetaEntity()->On_ParentMetaRelationCreated(this);
			}
		}

		assert(M_mapMetaRelation.find(m_uID) == M_mapMetaRelation.end());
		M_mapMetaRelation[m_uID] =  this;
	}



	// Construct a MetaRelation object from a D3MetaRelation object (a D3MetaRelation object
	// represents a record from the D3MetaRelation table in the D3 MetaDictionary database)
	//
	/* static */
	void MetaRelation::LoadMetaObject(D3MetaRelationPtr pD3MR)
	{
		MetaKeyPtr					pParentMK;
		MetaKeyPtr					pChildMK;
		MetaColumnPtr				pSwitchMC = NULL;
		D3MetaColumnPtr			pD3MCSwitch;
		MetaRelationPtr			pNewMR;
		std::string					strName;
		std::string					strProsaName;
		std::string					strDescription;
		std::string					strReverseName;
		std::string					strReverseProsaName;
		std::string					strReverseDescription;
		std::string					strClassName;
		std::string					strSwitchColumnValue;
		Flags::Mask					flags;


		assert(pD3MR);

		// Get the name of the object to create
		//
		strName					= pD3MR->GetName();
		strReverseName	= pD3MR->GetReverseName();

		// Get the instance class name for the MetaRelation
		//
		if (!pD3MR->GetColumn(D3MDDB_D3MetaRelation_InstanceClass)->IsNull())
			strClassName = pD3MR->GetInstanceClass();
		else
			strClassName = "Relation";

		// Set the construction flags
		//
		if (!pD3MR->GetColumn(D3MDDB_D3MetaRelation_Flags)->IsNull())
			flags = Flags::Mask(pD3MR->GetFlags());

		// Get the switch column value
		//
		if (!pD3MR->GetColumn(D3MDDB_D3MetaRelation_SwitchColumnValue)->IsNull())
			strSwitchColumnValue = pD3MR->GetSwitchColumnValue();

		// Find the correct keys (navigate Meta objects top down)
		pParentMK	= MetaKey::GetMetaKeyByID(pD3MR->GetParentKeyID());
		pChildMK	= MetaKey::GetMetaKeyByID(pD3MR->GetChildKeyID());

		// Get the switched meta column
		pD3MCSwitch	= pD3MR->GetSwitchD3MetaColumn();

		if (( ((flags & Flags::ParentSwitch) || (flags & Flags::ChildSwitch)) && !pD3MCSwitch) ||
			  (!((flags & Flags::ParentSwitch) || (flags & Flags::ChildSwitch)) && pD3MCSwitch))
		{
			flags &= ~Flags::ParentSwitch;
			flags &= ~Flags::ChildSwitch;
			pSwitchMC = NULL;
			strSwitchColumnValue.clear();
			ReportError("MetaRelation::LoadMetaObject(): Relation %s.%s is either marked switched or specifies a switch column but not both. Ignoring switch.", pParentMK->GetMetaEntity()->GetName().c_str(), strName.c_str());
		}
		else
		{
			if (pD3MCSwitch)
			{
				pSwitchMC = MetaColumn::GetMetaColumnByID(pD3MCSwitch->GetID());

				if (!pSwitchMC)
				{
					flags &= ~Flags::ParentSwitch;
					flags &= ~Flags::ChildSwitch;
					strSwitchColumnValue.clear();
					ReportError("MetaRelation::LoadMetaObject(): Relation %s.%s specifies a switch column that doesn't exist (check if parent switch specifies a child column or vs). Ignoring switch.", pParentMK->GetMetaEntity()->GetName().c_str(), strName.c_str());
				}
			}
		}

		assert(pParentMK && pChildMK);

		// Create matching MetaRelation object
		//
		pNewMR = new MetaRelation(pD3MR->GetID(), strName, strReverseName, pParentMK, pChildMK, strClassName, flags, pSwitchMC, strSwitchColumnValue);

		pNewMR->m_strProsaName					= pD3MR->GetProsaName();
		pNewMR->m_strDescription				= pD3MR->GetDescription();
		pNewMR->m_strReverseProsaName		= pD3MR->GetReverseProsaName();
		pNewMR->m_strReverseDescription	= pD3MR->GetReverseDescription();

		// Set all the other attributes
		//
		pNewMR->m_strInstanceInterface	= pD3MR->GetInstanceInterface();
		pNewMR->m_strJSMetaClass				= pD3MR->GetJSMetaClass();
		pNewMR->m_strJSInstanceClass		= pD3MR->GetJSInstanceClass();
		pNewMR->m_strJSParentViewClass	= pD3MR->GetJSParentViewClass();
		pNewMR->m_strJSChildViewClass		= pD3MR->GetJSChildViewClass();
	}



	std::string MetaRelation::AsCreateCheckInsertUpdateTrigger(TargetRDBMS eTargetDB)
	{
		std::string							strSQL;
		MetaRelationPtr					pMR;
		MetaRelationPtrList			listMR;
		MetaRelationPtrListItr	itrMR;


		// Do not create cross database triggers
		//
		if (!m_pParentKey                                     ||  !m_pChildKey ||
			   m_pParentKey->GetMetaEntity()->GetMetaDatabase() != m_pChildKey->GetMetaEntity()->GetMetaDatabase())
		{
			return strSQL;
		}

		// No need to create root relation triggers (root relations are not [and probably never will be] implemented)
		//
		if (!m_pParentKey)
			return strSQL;

		// Make sure we create one trigger only for multiple relations between the same two keys
		//
		GetSynonymRelations(listMR);

		if (listMR.front() != this)
			return strSQL;

		switch (eTargetDB)
		{
			case SQLServer:
			{
				std::string						strTriggerName;
				unsigned int					idx;
				MetaColumnPtrListItr	itrTrgtMetaColumn, itrSrceMetaColumn;

				// Build something along these lines
				//
				// CREATE TRIGGER ciu_trgttblname_col1_col2_coln ON trgttblname
				// FOR INSERT, UPDATE AS
				// DECLARE @trgtCol1 int
				// DECLARE @trgtCol2 int
				// DECLARE @trgtColn int
				// DECLARE ciu_trgttblname_col1_col2_coln_cursor CURSOR LOCAL FOR SELECT trgtCol1, trgtCol2, trgtColn FROM inserted
				// OPEN ciu_trgttblname_col1_col2_coln_cursor
				// FETCH NEXT FROM ciu_trgttblname_col1_col2_coln_cursor INTO @trgtCol1, @trgtCol2, @trgtColn
				// WHILE @@FETCH_STATUS = 0
				// BEGIN
				// *********** If target key is mandatory do this:
				//   IF NOT EXISTS (SELECT * FROM srcetblname WHERE srceCol1 = @trgtCol1 AND srceCol2 = @trgtCol2 AND srceColn = @trgtColn)
				// *********** If target key is optional do this:
				//   IF (NOT @trgtCol1 IS NULL OR NOT @trgtCol2 IS NULL OR NOT @trgtColn IS NULL) AND NOT	EXISTS (SELECT * FROM srcetblname WHERE srceCol1 = @trgtCol1 AND srceCol2 = @trgtCol2 AND srceColn = @trgtColn)
				// ***********
				//   BEGIN
				//     RAISERROR ('Unable to insert trgttblname because related srcetblname does not exist.', 16, 1) WITH SETERROR
				//     ROLLBACK TRANSACTION
				//     BREAK
				//   END
				//   FETCH NEXT FROM ciu_trgttblname_col1_col2_coln_cursor INTO @trgtCol1, @trgtCol2, @trgtColn
				// END
				// CLOSE ciu_trgttblname_col1_col2_coln_cursor
				// DEALLOCATE ciu_trgttblname_col1_col2_coln_cursor
				//

				// Create trigger name
				//
				strTriggerName  = "ciu_";
				strTriggerName += m_pChildKey->GetMetaEntity()->GetName();

				for ( itrTrgtMetaColumn =  m_pChildKey->GetMetaColumns()->begin(), idx = 0;
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && idx < m_pParentKey->GetColumnCount();
							itrTrgtMetaColumn++,																					idx++)
				{
					strTriggerName += "_";
					strTriggerName += (*itrTrgtMetaColumn)->GetName();
				}

				// CREATE TRIGGER ciu_trgttblname_col1_col2_coln ON trgttblname
				// FOR INSERT, UPDATE AS
				//
				strSQL  = "CREATE TRIGGER ";
				strSQL += strTriggerName;
				strSQL += " ON ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += "\n";
				strSQL += "FOR INSERT, UPDATE AS\n";

				// DECLARE @trgtCol1 int
				// DECLARE @trgtCol2 int
				// DECLARE @trgtColn int
				//
				for ( itrTrgtMetaColumn =  m_pChildKey->GetMetaColumns()->begin(), idx = 0;
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && idx < m_pParentKey->GetColumnCount();
							itrTrgtMetaColumn++,																					idx++)
				{
					strSQL += "DECLARE @v";
					strSQL += (*itrTrgtMetaColumn)->GetName();
					strSQL += " ";
					strSQL += (*itrTrgtMetaColumn)->GetTypeAsSQLString(eTargetDB);
					strSQL += "\n";
				}

				// DECLARE ciu_trgttblname_col1_col2_coln_cursor CURSOR LOCAL FOR SELECT trgtCol1, trgtCol2, trgtColn FROM inserted
				//
				strSQL += "DECLARE ";
				strSQL += strTriggerName;
				strSQL += "_cursor CURSOR LOCAL FOR SELECT ";

				for ( itrTrgtMetaColumn =  m_pChildKey->GetMetaColumns()->begin(), idx = 0;
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && idx < m_pParentKey->GetColumnCount();
							itrTrgtMetaColumn++,																					idx++)
				{
					if (idx > 0)
						strSQL += ", ";

					strSQL += (*itrTrgtMetaColumn)->GetName();
				}

				strSQL += " FROM inserted\n";

				// OPEN ciu_trgttblname_col1_col2_coln_cursor
				//
				strSQL += "OPEN ";
				strSQL += strTriggerName;
				strSQL += "_cursor\n";

				// FETCH NEXT FROM ciu_trgttblname_col1_col2_coln_cursor INTO @trgtCol1, @trgtCol2, @trgtColn
				//
				strSQL += "FETCH NEXT FROM ";
				strSQL += strTriggerName;
				strSQL += "_cursor INTO ";

				for ( itrTrgtMetaColumn =  m_pChildKey->GetMetaColumns()->begin(), idx = 0;
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && idx < m_pParentKey->GetColumnCount();
							itrTrgtMetaColumn++,																					idx++)
				{
					if (idx > 0)
						strSQL += ", ";

					strSQL += "@v";
					strSQL += (*itrTrgtMetaColumn)->GetName();
				}

				strSQL += "\n";

				// WHILE @@FETCH_STATUS = 0
				// BEGIN
				//
				strSQL += "WHILE @@FETCH_STATUS = 0\n";
				strSQL += "BEGIN\n";

				//   IF (NOT @trgtCol1 IS NULL OR NOT @trgtCol2 IS NULL OR NOT @trgtColn IS NULL) AND NOT	EXISTS (SELECT * FROM srcetblname WHERE srceCol1 = @trgtCol1 AND srceCol2 = @trgtCol2 AND srceColn = @trgtColn)
				//
				strSQL += "  IF ";

				if (!m_pChildKey->IsMandatory())
				{
					strSQL += " (";

					for ( itrTrgtMetaColumn =  m_pChildKey->GetMetaColumns()->begin(), idx = 0;
								itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && idx < m_pParentKey->GetColumnCount();
								itrTrgtMetaColumn++,																					idx++)
					{
						if (idx > 0)
							strSQL += " OR ";

						strSQL += "NOT @v";
						strSQL += (*itrTrgtMetaColumn)->GetName();
						strSQL += " IS NULL";
					}

					strSQL += ") AND ";
				}

				strSQL += "NOT EXISTS (SELECT * FROM ";
				strSQL += m_pParentKey->GetMetaEntity()->GetName();
				strSQL += " WHERE ";

				for ( itrTrgtMetaColumn  = m_pChildKey->GetMetaColumns()->begin(), itrSrceMetaColumn  = m_pParentKey->GetMetaColumns()->begin();
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end();
							itrTrgtMetaColumn++,																					itrSrceMetaColumn++)
				{
					if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
						strSQL += " AND ";

					strSQL += (*itrSrceMetaColumn)->GetName();
					strSQL += " = @v";
					strSQL += (*itrTrgtMetaColumn)->GetName();
				}

				// If we have multiple relations between the same two keys, take parent switches into account
				//
				if (listMR.size() > 1)
				{
					for ( itrMR =  listMR.begin();
								itrMR != listMR.end();
								itrMR++)
					{
						pMR = *itrMR;

						if (pMR->IsParentSwitch())
						{
							strSQL += " AND ";
							strSQL += pMR->GetSwitchColumn()->GetMetaColumn()->GetName();
							strSQL += " = ";
							strSQL += pMR->GetSwitchColumn()->AsSQLString();
						}
					}
				}


				strSQL += ")\n";

				//   BEGIN
				//     RAISERROR ('Unable to insert trgttblname because related srcetblname does not exist.', 16, 1) WITH SETERROR
				//     ROLLBACK TRANSACTION
				//     BREAK
				//   END
				//
				strSQL += "  BEGIN\n";
				strSQL += "    RAISERROR ('Unable to insert ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += " because related ";
				strSQL += GetReverseName();
				strSQL += " does not exist.', 16, 1) WITH SETERROR\n";
				strSQL += "    ROLLBACK TRANSACTION\n";
				strSQL += "    BREAK\n";
				strSQL += "  END\n";

				//   FETCH NEXT FROM ciu_trgttblname_col1_col2_coln_cursor INTO @trgtCol1, @trgtCol2, @trgtColn
				//
				strSQL += "  FETCH NEXT FROM ";
				strSQL += strTriggerName;
				strSQL += "_cursor INTO ";

				for ( itrTrgtMetaColumn =  m_pChildKey->GetMetaColumns()->begin(), idx = 0;
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && idx < m_pParentKey->GetColumnCount();
							itrTrgtMetaColumn++,																					idx++)
				{
					if (idx > 0)
						strSQL += ", ";

					strSQL += "@v";
					strSQL += (*itrTrgtMetaColumn)->GetName();
				}

				strSQL += "\n";

				// END
				// CLOSE ciu_trgttblname_col1_col2_coln_cursor
				// DEALLOCATE ciu_trgttblname_col1_col2_coln_cursor
				//
				strSQL += "END\n";
				strSQL += "CLOSE ";
				strSQL += strTriggerName;
				strSQL += "_cursor\n";
				strSQL += "DEALLOCATE ";
				strSQL += strTriggerName;
				strSQL += "_cursor\n";


				break;
			}

			case Oracle:
			{
				std::string						strTriggerName;
				std::string						strPackageName;
				char									szPackageBuffer[20];
				char									szBuffer[20];
				unsigned int					idx;
				MetaColumnPtrListItr	itrTrgtMetaColumn, itrSrceMetaColumn;


				// Build something along these lines
				//
				//	CREATE TRIGGER CIU_RULE
				//	BEFORE INSERT OR UPDATE OF PID1, PID2 ON CHILD
				//	FOR EACH ROW
				//	DECLARE
				//	ISOK INTEGER;
				//	BEGIN
				//	ISOK := 0;
				//	SELECT COUNT(*) INTO ISOK FROM PARENT WHERE ID1 = :NEW.PID1 AND ID2=:NEW.PID2;
				// *********** If target key is mandatory do this:
				//	IF ISOK = 0 THEN
				// *********** If target key is optional do this:
				//	IF (:NEW.PID1 IS NOT NULL OR :NEW.PID2 IS NOT NULL) AND ISOK = 0 THEN
				// ***********
				//	RAISE_APPLICATION_ERROR(-20001,'CANNOT INSERT OR UPDATE NO MATCHING PARENT ROW FOUND',TRUE);
				//	END IF;
				//	END;

				// Create Package name
				//
				sprintf(szPackageBuffer, "E%02i_E%02iK%02i", m_pParentKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaKeyIndex());

				strPackageName  = "pk_";
				strPackageName += m_pParentKey->GetMetaEntity()->GetMetaDatabase()->GetAlias();
				strPackageName += "_";
				strPackageName += szPackageBuffer;

				if (strPackageName.size() > 30)
				{
					ReportWarning("MetaRelation::AsCreateCascadePackage(): Name %s of generated %s  %s exceedes maximum length of 30 characters. Using truncated name.", m_pChildKey->GetName().c_str(), m_pChildKey->GetName().c_str(), strPackageName.c_str());
					strPackageName = strPackageName.substr(0, 30);
				}

				std::string strPackage(strPackageName,0,30);

				// Create trigger name (in Oracle there is a 30char name limit so just use id's)
				//
				sprintf(szBuffer, "E%02i_E%02iK%02i", m_pChildKey->GetMetaEntity()->GetEntityIdx(), m_pParentKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaKeyIndex());

				strTriggerName  = "ciu_";
				strTriggerName += m_pChildKey->GetMetaEntity()->GetMetaDatabase()->GetAlias();
				strTriggerName += "_";
				strTriggerName += szBuffer;

				if (strTriggerName.size() > 30)
				{
					ReportWarning("MetaRelation::AsCreateCheckInsertUpdateTrigger(): Name %s of generated check insert update trigger for relation from key %s to key %s exceedes maximum length of 30 characters. Using truncated name.", m_pChildKey->GetName().c_str(), m_pChildKey->GetName().c_str(), strTriggerName.c_str());
					strTriggerName = strTriggerName.substr(0, 30);
				}

				if (m_pParentKey->GetMetaEntity()->GetName() == m_pChildKey->GetMetaEntity()->GetName())
				{
					strSQL  = "ALTER TABLE ";
					strSQL += m_pChildKey->GetMetaEntity()->GetName();

					strSQL += " ADD CONSTRAINT ";
					strSQL += strTriggerName;
					strSQL += " FOREIGN KEY ( ";

					for ( itrTrgtMetaColumn  = m_pChildKey->GetMetaColumns()->begin(), itrSrceMetaColumn  = m_pParentKey->GetMetaColumns()->begin();
								itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end();
								itrTrgtMetaColumn++,																					itrSrceMetaColumn++)
					{
						if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
							strSQL += ",";

						strSQL += (*itrTrgtMetaColumn)->GetName();
					}

					strSQL += " ) ";
					strSQL += "REFERENCES ";

					strSQL += m_pParentKey->GetMetaEntity()->GetName();
					strSQL += " ( ";

					for ( itrTrgtMetaColumn  = m_pChildKey->GetMetaColumns()->begin(), itrSrceMetaColumn  = m_pParentKey->GetMetaColumns()->begin();
								itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end();
								itrTrgtMetaColumn++,																					itrSrceMetaColumn++)
					{
						if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
							strSQL += " , ";

						strSQL += (*itrSrceMetaColumn)->GetName();
					}

					strSQL += " ) ";
					strSQL += " ON DELETE ";
					if (IsCascadeClearRef())
						strSQL += "SET NULL";
					else
						strSQL += "CASCADE";
				}
				else
				{
					strSQL  = "CREATE TRIGGER ";
					strSQL += strTriggerName;
					strSQL += "\nBEFORE INSERT OR UPDATE OF ";

					for ( itrTrgtMetaColumn  = m_pChildKey->GetMetaColumns()->begin(), itrSrceMetaColumn  = m_pParentKey->GetMetaColumns()->begin();
								itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end();
								itrTrgtMetaColumn++,																					itrSrceMetaColumn++)
					{
						if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
							strSQL += ",";

						strSQL += (*itrTrgtMetaColumn)->GetName();
					}

					strSQL += " ON ";
					strSQL += m_pChildKey->GetMetaEntity()->GetName();
					strSQL += "\n";
					strSQL += "FOR EACH ROW\n";
					strSQL += "DECLARE\n";
					strSQL += "l_fire_trigger varchar2(5);\n";
					strSQL += "ISOK INTEGER;\n";
					strSQL += "BEGIN\n";
					strSQL += "l_fire_trigger := ";
					strSQL += strPackage;
					strSQL += ".fire_trigger;\n";

					strSQL += "ISOK := 0;\n";
					strSQL += "IF l_fire_trigger = 'YES' THEN\n";
					strSQL += "SELECT COUNT(*) INTO ISOK FROM ";

					strSQL += m_pParentKey->GetMetaEntity()->GetName();
					strSQL += " WHERE ";

					for ( itrTrgtMetaColumn  = m_pChildKey->GetMetaColumns()->begin(), itrSrceMetaColumn  = m_pParentKey->GetMetaColumns()->begin();
								itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end();
								itrTrgtMetaColumn++,																					itrSrceMetaColumn++)
					{
						if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
							strSQL += " AND ";

						strSQL += (*itrSrceMetaColumn)->GetName();
						strSQL += " = :NEW.";
						strSQL += (*itrTrgtMetaColumn)->GetName();
					}

					// If we have multiple relations between the same two keys, take parent switches into account
					//
					if (listMR.size() > 1)
					{
						for ( itrMR =  listMR.begin();
									itrMR != listMR.end();
									itrMR++)
						{
							pMR = *itrMR;

							if (pMR->IsParentSwitch())
							{
								strSQL += " AND ";
								strSQL += pMR->GetSwitchColumn()->GetMetaColumn()->GetName();
								strSQL += " = ";
								strSQL += pMR->GetSwitchColumn()->AsSQLString();
							}
						}
					}

					strSQL += ";\n";

					//	IF (:NEW.PID1 IS NOT NULL OR :NEW.PID2 IS NOT NULL) AND ISOK = 0 THEN
					//
					strSQL += "IF";

					if (!m_pChildKey->IsMandatory())
					{
						strSQL += " (";

						for ( itrTrgtMetaColumn =  m_pChildKey->GetMetaColumns()->begin(), idx = 0;
									itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && idx < m_pParentKey->GetColumnCount();
									itrTrgtMetaColumn++,																					idx++)
						{
							if (idx > 0)
								strSQL += " OR ";

							strSQL += " :NEW.";
							strSQL += (*itrTrgtMetaColumn)->GetName();
							strSQL += " IS NOT NULL";
						}

						strSQL += ") AND ";
					}
						strSQL += " ISOK=0 THEN\n";

					//   BEGIN
					//     RAISERROR ('Unable to insert trgttblname because related srcetblname does not exist.', 16, 1) WITH SETERROR
					//     ROLLBACK TRANSACTION
					//     BREAK
					//   END
					//
					strSQL += "    RAISE_APPLICATION_ERROR (-20001,'Unable to insert ";
					strSQL += m_pChildKey->GetMetaEntity()->GetName();
					strSQL += " because related ";
					strSQL += m_pParentKey->GetMetaEntity()->GetName();
					strSQL += " does not exist.', TRUE);\n";
					strSQL += "END IF;\n";
					strSQL += "END IF;\n";
					strSQL += "END;\n";

					//   FETCH NEXT FROM ciu_trgttblname_col1_col2_coln_cursor INTO @trgtCol1, @trgtCol2, @trgtColn
					//
					strSQL += "\n";
				}
				break;
			}
		}
		return strSQL;
	}



	std::string MetaRelation::AsCreateCascadeDeleteTrigger(TargetRDBMS eTargetDB)
	{
		std::string							strSQL;
		MetaRelationPtr					pMR;
		MetaRelationPtrList			listMR;
		MetaRelationPtrListItr	itrMR;


		// Do not create cross database triggers
		//
		if (!m_pParentKey                                     ||  !m_pChildKey ||
			   m_pParentKey->GetMetaEntity()->GetMetaDatabase() != m_pChildKey->GetMetaEntity()->GetMetaDatabase())
		{
			return strSQL;
		}

		// Make sure we create one trigger only for multiple relations between the same two keys
		//
		GetSynonymRelations(listMR);

		if (listMR.front() != this)
			return strSQL;

		switch (eTargetDB)
		{
			case SQLServer:
			{
				std::string						strTriggerName, strWhereClause;
				MetaColumnPtrListItr	itrTrgtMetaColumn, itrSrceMetaColumn;
				bool									bFirst;

				// Build something along these lines
				//
				// create trigger cd_trgttblname_trgtCol1_trgtCol2_trgtColn on srcetblname
				// for delete
				// as
				// if @@rowcount = 0
				//   return
				// delete trgttblname
				//   from
				//     trgttblname, deleted
				//   where
				//     trgttblname.trgtCol1=deleted.srceCol1 and
				//     trgttblname.trgtCol2=deleted.srceCol2 and
				//     trgttblname.trgtColn=deleted.srceColn
				//

				// No need to create a trigger for root relations
				//
				if (!m_pParentKey || !IsCascadeDelete())
					return strSQL;

				// Create trigger name and the where clause
				//
				bFirst = true;

				for ( itrSrceMetaColumn =  m_pParentKey->GetMetaColumns()->begin(),
							itrTrgtMetaColumn =  m_pChildKey-> GetMetaColumns()->begin();
							itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end() &&
							itrTrgtMetaColumn != m_pChildKey-> GetMetaColumns()->end();
							itrSrceMetaColumn++,
							itrTrgtMetaColumn++)
				{
					if (bFirst)
					{
						bFirst = false;

						strTriggerName  = "cd_";
						strTriggerName += m_pChildKey->GetMetaEntity()->GetName();

						strWhereClause	= "  where\n";
					}
					else
					{
						strWhereClause += " and\n";
					}

					// Add column name to trigger name suffix
					//
					strTriggerName += "_";
					strTriggerName += (*itrTrgtMetaColumn)->GetName();

					// generate where clause part
					//
					strWhereClause += "    ";
					strWhereClause += m_pChildKey->GetMetaEntity()->GetName();
					strWhereClause += ".";
					strWhereClause += (*itrTrgtMetaColumn)->GetName();
					strWhereClause += "=deleted.";
					strWhereClause += (*itrSrceMetaColumn)->GetName();
				}

				// Generate this:
				//
				// create trigger cd_trgttblname_trgtCol1_trgtCol2_trgtColn on srcetblname
				// for delete
				// as
				// if @@rowcount = 0
				//   return
				// delete trgttblname
				//   from
				//     trgttblname, deleted
				//   where
				//     trgttblname.trgtCol1=deleted.srceCol1 and
				//     trgttblname.trgtCol2=deleted.srceCol2 and
				//     trgttblname.trgtColn=deleted.srceColn
				//
				strSQL  = "CREATE TRIGGER ";
				strSQL += strTriggerName;
				strSQL += " on ";
				strSQL += m_pParentKey->GetMetaEntity()->GetName();
				strSQL += "\nfor delete\nas\nif @@rowcount=0\n  return\ndelete ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += "\n  from\n    ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += ", deleted\n";
				strSQL += strWhereClause;
				strSQL += "\n";

				break;
			}

			case Oracle:
			{
				std::string						strTriggerName;
				std::string						strPackageName;
				char									szPackageBuffer[20];
				char									szBuffer[20];
				MetaColumnPtrListItr	itrTrgtMetaColumn, itrSrceMetaColumn;

				// CREATE OR REPLACE TRIGGER CD_RULE
				// BEFORE DELETE ON PARENT_TBL
				// FOR EACH ROW
				// BEGIN
				//   DELETE FROM CHILD_TBL WHERE PID = :OLD.ID;
				// END;
				//
				// todo

				if (!m_pParentKey || !IsCascadeDelete())
					return strSQL;

				// For Oracle, these recursive relationships are handled by the CIU constraints on the table
				if (m_pParentKey->GetMetaEntity()->GetName() == m_pChildKey->GetMetaEntity()->GetName())
					return strSQL;

				// Create Package name
				//
				sprintf(szPackageBuffer, "E%02i_E%02iK%02i", m_pParentKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaKeyIndex());

				strPackageName  = "pk_";
				strPackageName += m_pParentKey->GetMetaEntity()->GetMetaDatabase()->GetAlias();
				strPackageName += "_";
				strPackageName += szPackageBuffer;

				if (strPackageName.size() > 30)
				{
					ReportWarning("MetaRelation::AsCreateCascadePackage(): Name %s of generated %s  %s exceedes maximum length of 30 characters. Using truncated name.", m_pChildKey->GetName().c_str(), m_pChildKey->GetName().c_str(), strPackageName.c_str());
					strPackageName = strPackageName.substr(0, 30);
				}

				std::string strPackage(strPackageName,0,30);

				// Create trigger name
				//
				sprintf(szBuffer, "E%02i_E%02iK%02i", m_pParentKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaKeyIndex());

				strTriggerName  = "cd_";
				strTriggerName += m_pParentKey->GetMetaEntity()->GetMetaDatabase()->GetAlias();
				strTriggerName += "_";
				strTriggerName += szBuffer;

				if (strTriggerName.size() > 30)
				{
					ReportWarning("MetaRelation::AsCreateCascadeDeleteTrigger(): Name %s of generated cascade delete trigger for relation from key %s to key %s exceedes maximum length of 30 characters. Using truncated name.", m_pChildKey->GetName().c_str(), m_pChildKey->GetName().c_str(), strTriggerName.c_str());
					strTriggerName = strTriggerName.substr(0, 30);
				}

				// CREATE TRIGGER cd_trgttblname_trgtCol1_trgtCol2_trgtColn ON trgttblname
				// FOR DELETE AS
				//
				std::string strTrigger(strTriggerName,0,30);

				strSQL  = "CREATE TRIGGER ";
				strSQL += strTrigger;
				strSQL += "\nBEFORE DELETE ON ";
				strSQL += m_pParentKey->GetMetaEntity()->GetName();
				strSQL += "\n";
				strSQL += "FOR EACH ROW\n";
				strSQL += "BEGIN\n";
				strSQL += strPackage;
				strSQL += ".fire_trigger := 'NO';\n";

				//  DELETE FROM trgttblname WHERE trgtCol1 = @vsrceCol1 AND trgtCol2 = @vsrceCol2 AND trgtColn = @vsrceColn
				//
				strSQL += "  DELETE FROM ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += " WHERE ";

				for ( itrTrgtMetaColumn  = m_pChildKey->GetMetaColumns()->begin(), itrSrceMetaColumn  = m_pParentKey->GetMetaColumns()->begin();
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end();
							itrTrgtMetaColumn++,																					itrSrceMetaColumn++)
				{
					if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
						strSQL += " AND ";

					strSQL += (*itrTrgtMetaColumn)->GetName();
					strSQL += " = :OLD.";
					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				// If we have multiple relations between the same two keys, take child switches into account
				//
				if (listMR.size() > 1)
				{
					std::string		strSwitch;

					// Find one that is not a switched child
					//
					for ( itrMR =  listMR.begin();
								itrMR != listMR.end();
								itrMR++)
					{
						pMR = *itrMR;

						if (!pMR->IsChildSwitch())
							break;
					}

					// Only do this if all children are switched
					//
					if (itrMR == listMR.end())
					{
						for ( itrMR =  listMR.begin();
									itrMR != listMR.end();
									itrMR++)
						{
							pMR = *itrMR;

							if (!strSwitch.empty())
								strSwitch += " OR ";

							strSwitch += pMR->GetSwitchColumn()->GetMetaColumn()->GetName();
							strSwitch += " = ";
							strSwitch += pMR->GetSwitchColumn()->AsSQLString();
						}
					}

					if (!strSwitch.empty())
					{
						strSQL += " AND (";
						strSQL += strSwitch;
						strSQL += ")";
					}
				}

				strSQL += ";\n";
				strSQL += strPackage;
				strSQL += ".fire_trigger := 'YES';\n";
				strSQL += "END;\n";

				break;
			}
		}
		return strSQL;
	}

	std::string MetaRelation::AsCreateCascadePackage(TargetRDBMS eTargetDB)
	{
		std::string							strSQL;
		MetaRelationPtrList			listMR;


		// Do not create cross database triggers
		//
		if (!m_pParentKey                                     ||  !m_pChildKey ||
			   m_pParentKey->GetMetaEntity()->GetMetaDatabase() != m_pChildKey->GetMetaEntity()->GetMetaDatabase())
		{
			return strSQL;
		}

		// Make sure we create one trigger only for multiple relations between the same two keys
		//
		GetSynonymRelations(listMR);

		if (listMR.front() != this)
			return strSQL;

		switch (eTargetDB)
		{
			case SQLServer:
			{
				return strSQL;
				break;
			}

			case Oracle:
			{
				std::string						strPackageName;
				char									szBuffer[20];


				// CREATE OR REPLACE TRIGGER CD_RULE
				// BEFORE DELETE ON PARENT_TBL
				// FOR EACH ROW
				// BEGIN
				//   DELETE FROM CHILD_TBL WHERE PID = :OLD.ID;
				// END;
				//

				// For Oracle, these recursive relationships are handled by the CIU constraints on the table
				if (m_pParentKey->GetMetaEntity()->GetName() == m_pChildKey->GetMetaEntity()->GetName())
					return strSQL;

				// Create trigger name
				//
				sprintf(szBuffer, "E%02i_E%02iK%02i", m_pParentKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaKeyIndex());

				strPackageName  = "pk_";
				strPackageName += m_pParentKey->GetMetaEntity()->GetMetaDatabase()->GetAlias();
				strPackageName += "_";
				strPackageName += szBuffer;

				if (strPackageName.size() > 30)
				{
					ReportWarning("MetaRelation::AsCreateCascadePackage(): Name %s of generated %s  %s exceedes maximum length of 30 characters. Using truncated name.", m_pChildKey->GetName().c_str(), m_pChildKey->GetName().c_str(), strPackageName.c_str());
					strPackageName = strPackageName.substr(0, 30);
				}

				std::string strPackage(strPackageName,0,30);

				// CREATE TRIGGER cd_trgttblname_trgtCol1_trgtCol2_trgtColn ON trgttblname
				// FOR DELETE AS
				//
				strSQL  = "CREATE or replace PACKAGE ";
				strSQL += strPackage;
				strSQL += "\nAS fire_trigger varchar2(5) := 'YES';\n";
				strSQL += "END ";
				strSQL += strPackage;
				strSQL += ";\n";

				break;
			}
		}
		return strSQL;
	}


	std::string MetaRelation::AsCreateCascadeClearRefTrigger(TargetRDBMS eTargetDB)
	{
		std::string							strSQL;
		MetaRelationPtr					pMR;
		MetaRelationPtrList			listMR;
		MetaRelationPtrListItr	itrMR;


		// Do not create cross database triggers
		//
		if (!m_pParentKey || !m_pChildKey ||
			   m_pParentKey->GetMetaEntity()->GetMetaDatabase() != m_pChildKey->GetMetaEntity()->GetMetaDatabase())
		{
			return strSQL;
		}

		// Make sure we create one trigger only for multiple relations between the same two keys
		//
		GetSynonymRelations(listMR);

		if (listMR.front() != this)
			return strSQL;

		switch (eTargetDB)
		{
			case SQLServer:
			{
				std::string						strTriggerName, strSetClause, strWhereClause;
				MetaColumnPtrListItr	itrTrgtMetaColumn, itrSrceMetaColumn;
				bool									bFirst;

				// Build something along these lines
				//
				// create trigger ccr_trgttblname_trgtCol1_trgtCol2_trgtColn on srcetblname
				// for delete
				// as
				// if @@rowcount = 0
				//   return
				// update trgttblname from trgttblname, deleted
				//   set
				//     trgttblname.trgtCol1=NULL,
				//     trgttblname.trgtCol2=NULL,
				//     trgttblname.trgtColn=NULL
				//   where
				//     trgttblname.trgtCol1=deleted.srceCol1 and
				//     trgttblname.trgtCol2=deleted.srceCol2 and
				//     trgttblname.trgtColn=deleted.srceColn
				//

				// No need to create a trigger for root relations
				//
				if (!m_pParentKey || !IsCascadeClearRef())
					return strSQL;

				// Create full trigger name, set clause and where clause
				//
				bFirst = true;

				for ( itrSrceMetaColumn =  m_pParentKey->GetMetaColumns()->begin(),
							itrTrgtMetaColumn =  m_pChildKey-> GetMetaColumns()->begin();
							itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end() &&
							itrTrgtMetaColumn != m_pChildKey-> GetMetaColumns()->end();
							itrSrceMetaColumn++,
							itrTrgtMetaColumn++)
				{
					if (bFirst)
					{
						bFirst = false;

						strTriggerName	= "ccr_";
						strTriggerName	+= m_pChildKey->GetMetaEntity()->GetName();

						strSetClause		= "  set\n";

						strWhereClause	= "  where\n";
					}
					else
					{
						strSetClause	 += ",\n";
						strWhereClause += " and\n";
					}

					// Add column name to trigger name suffix
					//
					strTriggerName += "_";
					strTriggerName += (*itrTrgtMetaColumn)->GetName();

					// generate set clause part
					//
					strSetClause += "    ";
					strSetClause += m_pChildKey->GetMetaEntity()->GetName();
					strSetClause += ".";
					strSetClause += (*itrTrgtMetaColumn)->GetName();
					strSetClause += "=NULL";

					// generate where clause part
					//
					strWhereClause += "    ";
					strWhereClause += m_pChildKey->GetMetaEntity()->GetName();
					strWhereClause += ".";
					strWhereClause += (*itrTrgtMetaColumn)->GetName();
					strWhereClause += "=deleted.";
					strWhereClause += (*itrSrceMetaColumn)->GetName();
				}

				// Generate this:
				//
				// create trigger ccr_trgttblname_trgtCol1_trgtCol2_trgtColn on srcetblname
				// for delete
				// as
				// if @@rowcount=0
				//   return
				// update trgttblname
				//   set
				//     trgttblname.trgtCol1=NULL,
				//     trgttblname.trgtCol2=NULL,
				//     trgttblname.trgtColn=NULL
				//   from
				//     trgttblname, deleted
				//   where
				//     trgttblname.trgtCol1=deleted.srceCol1 and
				//     trgttblname.trgtCol2=deleted.srceCol2 and
				//     trgttblname.trgtColn=deleted.srceColn
				//
				strSQL  = "CREATE TRIGGER ";
				strSQL += strTriggerName;
				strSQL += " on ";
				strSQL += m_pParentKey->GetMetaEntity()->GetName();
				strSQL += "\nfor delete\nas\nif @@rowcount=0\n  return\nupdate ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += "\n";
				strSQL += strSetClause;
				strSQL += "\n";
				strSQL += "  from\n    ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += ", deleted\n";
				strSQL += strWhereClause;

				strSQL += "\n";

				break;
			}

			case Oracle:
			{

				std::string						strTriggerName;
				std::string						strPackageName;
				char									szPackageBuffer[20];
				char									szBuffer[20];
				unsigned int					idx;
				MetaColumnPtrListItr	itrTrgtMetaColumn, itrSrceMetaColumn;

				//			CREATE OR REPLACE TRIGGER CN_RULE
				//			AFTER DELETE ON PARENT_TBL
				//			FOR EACH ROW
				//			BEGIN
				//				UPDATE CHILD_TBL SET PID = NULL WHERE PID = :OLD.ID;
				//			END;

				if (!m_pParentKey || !IsCascadeClearRef())
					return strSQL;

				// For Oracle, these recursive relationships are handled by the CIU constraints on the table
				if (m_pParentKey->GetMetaEntity()->GetName() == m_pChildKey->GetMetaEntity()->GetName())
					return strSQL;

				// Create Package name
				//
				sprintf(szPackageBuffer, "E%02i_E%02iK%02i", m_pParentKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaKeyIndex());

				strPackageName  = "pk_";
				strPackageName += m_pParentKey->GetMetaEntity()->GetMetaDatabase()->GetAlias();
				strPackageName += "_";
				strPackageName += szPackageBuffer;

				if (strPackageName.size() > 30)
				{
					ReportWarning("MetaRelation::AsCreateCascadePackage(): Name %s of generated %s  %s exceedes maximum length of 30 characters. Using truncated name.", m_pChildKey->GetName().c_str(), m_pChildKey->GetName().c_str(), strPackageName.c_str());
					strPackageName = strPackageName.substr(0, 30);
				}

				std::string strPackage(strPackageName,0,30);

				// Create trigger name
				//
				sprintf(szBuffer, "E%02i_E%02iK%02i", m_pParentKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaKeyIndex());

				strTriggerName  = "ccr_";
				strTriggerName += m_pParentKey->GetMetaEntity()->GetMetaDatabase()->GetAlias();
				strTriggerName += "_";
				strTriggerName += szBuffer;

				if (strTriggerName.size() > 30)
				{
					ReportWarning("MetaRelation::AsCreateCascadeClearRefTrigger(): Name %s of generated cascade clear reference trigger for relation from key %s to key %s exceedes maximum length of 30 characters. Using truncated name.", m_pChildKey->GetName().c_str(), m_pChildKey->GetName().c_str(), strTriggerName.c_str());
					strTriggerName = strTriggerName.substr(0, 30);
				}

				// CREATE TRIGGER cd_trgttblname_trgtCol1_trgtCol2_trgtColn ON trgttblname
				// FOR DELETE AS
				//
				std::string strTrigger(strTriggerName,0,30);

				strSQL  = "CREATE TRIGGER ";
				strSQL += strTrigger;
				strSQL += " AFTER DELETE ON ";
				strSQL += m_pParentKey->GetMetaEntity()->GetName();
				strSQL += "\n";
				strSQL += "FOR EACH ROW\n";
				strSQL += "BEGIN\n";
				strSQL += strPackage;
				strSQL += ".fire_trigger := 'NO';\n";

				strSQL += "    UPDATE ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += " SET ";

				for ( itrTrgtMetaColumn =  m_pChildKey->GetMetaColumns()->begin(), idx = 0;
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && idx < m_pParentKey->GetColumnCount();
							itrTrgtMetaColumn++,																					idx++)
				{
					if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
						strSQL += ", ";

					strSQL += (*itrTrgtMetaColumn)->GetName();
					strSQL += " = NULL";
				}

				strSQL += " WHERE ";

				for ( itrTrgtMetaColumn  = m_pChildKey->GetMetaColumns()->begin(), itrSrceMetaColumn  = m_pParentKey->GetMetaColumns()->begin();
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end();
							itrTrgtMetaColumn++,																					itrSrceMetaColumn++)
				{
					if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
						strSQL += " AND ";

					strSQL += (*itrTrgtMetaColumn)->GetName();
					strSQL += " = :OLD.";
					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				// If we have multiple relations between the same two keys, take child switches into account
				//
				if (listMR.size() > 1)
				{
					std::string		strSwitch;

					// Find one that is not a switched child
					//
					for ( itrMR =  listMR.begin();
								itrMR != listMR.end();
								itrMR++)
					{
						pMR = *itrMR;

						if (!pMR->IsChildSwitch())
							break;
					}

					// Only do this if all children are switched
					//
					if (itrMR == listMR.end())
					{
						for ( itrMR =  listMR.begin();
									itrMR != listMR.end();
									itrMR++)
						{
							pMR = *itrMR;

							if (!strSwitch.empty())
								strSwitch += " OR ";

							strSwitch += pMR->GetSwitchColumn()->GetMetaColumn()->GetName();
							strSwitch += " = ";
							strSwitch += pMR->GetSwitchColumn()->AsSQLString();
						}

					}

					if (!strSwitch.empty())
					{
						strSQL += " AND (";
						strSQL += strSwitch;
						strSQL += ")";
					}
				}

				strSQL += ";\n";
				strSQL += strPackage;
				strSQL += ".fire_trigger := 'YES';\n";

				strSQL += "END;\n";

				break;
			}
		}
		return strSQL;
	}



	std::string MetaRelation::AsCreateVerifyDeleteTrigger(TargetRDBMS eTargetDB)
	{
		std::string							strSQL;
		MetaRelationPtr					pMR;
		MetaRelationPtrList			listMR;
		MetaRelationPtrListItr	itrMR;


		// Do not create cross database triggers
		//
		if (!m_pParentKey                                     ||  !m_pChildKey ||
			   m_pParentKey->GetMetaEntity()->GetMetaDatabase() != m_pChildKey->GetMetaEntity()->GetMetaDatabase())
		{
			return strSQL;
		}

		// Make sure we create one trigger only for multiple relations between the same two keys
		//
		GetSynonymRelations(listMR);

		if (listMR.front() != this)
			return strSQL;

		switch (eTargetDB)
		{
			case SQLServer:
			{
				std::string						strTriggerName, strWhereClause;
				MetaColumnPtrListItr	itrTrgtMetaColumn, itrSrceMetaColumn;
				bool									bFirst;

				// Build something along these lines
				//
				// create trigger vd_trgttblname_trgtCol1_trgtCol2_trgtColn on srcetblname
				// for delete
				// as
				// if @@rowcount = 0
				//   return
				// if exists(select trgttblname.* from trgttblname, deleted
				//             where
				//               trgttblname.trgtCol1=deleted.srceCol1 and
				//               trgttblname.trgtCol2=deleted.srceCol2 and
				//               trgttblname.trgtColn=deleted.srceColn)
				// begin
				//   raiserror('Unable to delete row in srcetblname because related records exist in table trgttblname!', 16, 1) with seterror
				//   rollback transaction
				// end
				//

				// No need to create a trigger for root relations
				//
				if (!m_pParentKey || !IsPreventDelete())
					return strSQL;

				// Create full trigger name, set clause and where clause
				//
				bFirst = true;

				for ( itrSrceMetaColumn =  m_pParentKey->GetMetaColumns()->begin(),
							itrTrgtMetaColumn =  m_pChildKey-> GetMetaColumns()->begin();
							itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end() &&
							itrTrgtMetaColumn != m_pChildKey-> GetMetaColumns()->end();
							itrSrceMetaColumn++,
							itrTrgtMetaColumn++)
				{
					if (bFirst)
					{
						bFirst = false;

						strTriggerName	= "vd_";
						strTriggerName	+= m_pChildKey->GetMetaEntity()->GetName();

						strWhereClause	= "            where\n";
					}
					else
					{
						strWhereClause += " and\n";
					}

					// Add column name to trigger name suffix
					//
					strTriggerName += "_";
					strTriggerName += (*itrTrgtMetaColumn)->GetName();

					// generate where clause part
					//
					strWhereClause += "              ";
					strWhereClause += m_pChildKey->GetMetaEntity()->GetName();
					strWhereClause += ".";
					strWhereClause += (*itrTrgtMetaColumn)->GetName();
					strWhereClause += "=deleted.";
					strWhereClause += (*itrSrceMetaColumn)->GetName();
				}

				// Generate this:
				//
				// create trigger vd_trgttblname_trgtCol1_trgtCol2_trgtColn on srcetblname
				// for delete
				// as
				// if @@rowcount = 0
				//   return
				// if exists(select trgttblname.*
				//             from
				//               trgttblname, deleted
				//             where
				//               trgttblname.trgtCol1=deleted.srceCol1 and
				//               trgttblname.trgtCol2=deleted.srceCol2 and
				//               trgttblname.trgtColn=deleted.srceColn)
				// begin
				//   raiserror('Unable to delete row in srcetblname because related records exist in table trgttblname!', 16, 1) with seterror
				//   rollback transaction
				// end
				//
				strSQL  = "CREATE TRIGGER ";
				strSQL += strTriggerName;
				strSQL += " on ";
				strSQL += m_pParentKey->GetMetaEntity()->GetName();
				strSQL += "\nfor delete\nas\nif @@rowcount=0\n  return\nif exists(select ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += ".*\n            from\n              ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += ", deleted\n";
				strSQL += strWhereClause;
				strSQL += ")\nbegin\n";
				strSQL += "  raiserror('Unable to delete row in ";
				strSQL += m_pParentKey->GetMetaEntity()->GetName();
				strSQL += " because related records exist in table ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += "!', 16, 1) with seterror\n  rollback transaction\nend\n";

				break;
			}

			case Oracle:
			{
				std::string						strTriggerName;
				char									szBuffer[20];
				MetaColumnPtrListItr	itrTrgtMetaColumn, itrSrceMetaColumn;

				// CREATE OR REPLACE TRIGGER VD_RULE
				// BEFORE DELETE ON PARENT_TBL
				// FOR EACH ROW
				// DECLARE
				// VD_RULE_FAILED	INTEGER;
				// BEGIN
				// VD_RULE_FAILED := 0;
				// SELECT COUNT(PID) INTO VD_RULE_FAILED FROM CHILD_TBL WHERE
				// PID = :OLD.ID; IF VD_RULE_FAILED != 0 THEN
				//   RAISE_APPLICATION_ERROR(-20000,'CANNOT DELETE BECAUSE
				// MATCHING CHILD TABLE
				// ROWS EXIST',TRUE);
				// END IF;
				// END;
				//

				// No need to create a trigger for root relations
				//
				if (!m_pParentKey || !IsPreventDelete())
					return strSQL;

				// Create trigger name
				//
				sprintf(szBuffer, "E%02i_E%02iK%02i", m_pParentKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaKeyIndex());

				strTriggerName  = "vd_";
				strTriggerName += m_pParentKey->GetMetaEntity()->GetMetaDatabase()->GetAlias();
				strTriggerName += "_";
				strTriggerName += szBuffer;

				if (strTriggerName.size() > 30)
				{
					ReportWarning("MetaRelation::AsCreateVerifyDeleteTrigger(): Name %s of generated verify delete trigger for relation from key %s to key %s exceedes maximum length of 30 characters. Using truncated name.", m_pChildKey->GetName().c_str(), m_pChildKey->GetName().c_str(), strTriggerName.c_str());
					strTriggerName = strTriggerName.substr(0, 30);
				}

				// CREATE TRIGGER ciu_trgttblname_col1_col2_coln ON trgttblname
				// FOR INSERT, UPDATE AS
				//
				std::string strTrigger(strTriggerName,0,30);

				strSQL  = "CREATE TRIGGER ";
				strSQL += strTrigger;
				strSQL += " BEFORE DELETE ON ";
				strSQL += m_pParentKey->GetMetaEntity()->GetName();
				strSQL += "\n";
				strSQL += "FOR EACH ROW\n";
				strSQL += "DECLARE\n";
				strSQL += "VD_RULE_FAILED	INTEGER;\n";
				strSQL += "BEGIN\n";
				strSQL += "VD_RULE_FAILED := 0;\n";
				//
				strSQL += "SELECT COUNT( ";


				for ( itrTrgtMetaColumn  = m_pChildKey->GetMetaColumns()->begin(), itrSrceMetaColumn  = m_pParentKey->GetMetaColumns()->begin();
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end();
							itrTrgtMetaColumn++,																					itrSrceMetaColumn++)
				{
					if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
					{
						strSQL += " AND ";
					}
					else
					{
						strSQL += (*itrTrgtMetaColumn)->GetName();
						strSQL +=") INTO VD_RULE_FAILED FROM ";
						strSQL += m_pChildKey->GetMetaEntity()->GetName();
						strSQL += " WHERE ";
					}

					strSQL += (*itrTrgtMetaColumn)->GetName();
					strSQL += " = :OLD.";
					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				// If we have multiple relations between the same two keys, take child switches into account
				//
				if (listMR.size() > 1)
				{
					std::string		strSwitch;


					// Find one that is not a switched child
					//

					for ( itrMR =  listMR.begin();
								itrMR != listMR.end();
								itrMR++)
					{
						pMR = *itrMR;

						if (!pMR->IsChildSwitch())
							break;
					}

					// Only do this if all children are switched
					//
					if (itrMR == listMR.end())
					{
						for ( itrMR =  listMR.begin();
									itrMR != listMR.end();
									itrMR++)
						{
							pMR = *itrMR;

							if (!strSwitch.empty())
								strSwitch += " OR ";

							strSwitch += pMR->GetSwitchColumn()->GetMetaColumn()->GetName();
							strSwitch += " = ";
							strSwitch += pMR->GetSwitchColumn()->AsSQLString();
						}
					}

					if (!strSwitch.empty())
					{
						strSQL += " AND (";
						strSQL += strSwitch;
						strSQL += ")";
					}
				}

				strSQL += ";\n";

				strSQL += "IF VD_RULE_FAILED != 0 THEN\n";
				strSQL += "    RAISE_APPLICATION_ERROR(-20000,'Unable to delete row in ";
				strSQL += m_pParentKey->GetMetaEntity()->GetName();
				strSQL += " because related records in table ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += " exist!', TRUE);\n";
				strSQL += "END IF;\n";
				strSQL += "END;\n";

				break;
			}
		}
		return strSQL;
	}



	std::string MetaRelation::AsUpdateForeignRefTrigger(TargetRDBMS eTargetDB)
	{
		std::string							strSQL;
		MetaRelationPtr					pMR;
		MetaRelationPtrList			listMR;
		MetaRelationPtrListItr	itrMR;


		// Do not create cross database triggers
		//
		if (!m_pParentKey																			|| !m_pChildKey ||
				 m_pParentKey->GetMetaEntity()->GetMetaDatabase() != m_pChildKey->GetMetaEntity()->GetMetaDatabase())
		{
			return strSQL;
		}

		// Make sure we create one trigger only for multiple relations between the same two keys
		//
		GetSynonymRelations(listMR);

		if (listMR.front() != this)
			return strSQL;

		switch (eTargetDB)
		{
			case SQLServer:
			{
				std::string						strTriggerName;
				unsigned int					idx;
				MetaColumnPtrListItr	itrTrgtMetaColumn, itrSrceMetaColumn;

				// Build something along these lines
				//
				// CREATE TRIGGER ufr_trgttblname_trgtCol1_trgtColn ON srcetblname
				// FOR UPDATE AS
				// DECLARE @vOldsrceCol1 varchar(12)
				// DECLARE @vOldsrceColn int
				// DECLARE @vNewsrceCol1 varchar(12)
				// DECLARE @vNewsrceColn int
				// DECLARE cu_trgttblname_trgtCol1_trgtColn_oldcursor CURSOR LOCAL FOR SELECT srceCol1 FROM deleted
				// DECLARE cu_trgttblname_trgtCol1_trgtColn_newcursor CURSOR LOCAL FOR SELECT srceCol1 FROM inserted
				// OPEN cu_trgttblname_trgtCol1_trgtColn_oldcursor
				// OPEN cu_trgttblname_trgtCol1_trgtColn_newcursor
				// FETCH NEXT FROM cu_trgttblname_trgtCol1_trgtColn_oldcursor INTO @vOldsrceCol1, @vOldsrceColn
				// FETCH NEXT FROM cu_trgttblname_trgtCol1_trgtColn_newcursor INTO @vNewsrceCol1, @vNewsrceColn
				// WHILE @@FETCH_STATUS = 0
				// BEGIN
				//   IF (@vNewsrceCol1 <> @vOldsrceCol1 OR @vNewsrceColn <> @vOldsrceColn)
				//   BEGIN
				//     UPDATE trgttblname SET trgtCol1 = @vNewsrceCol1, trgtColn = @vNewsrceColn WHERE trgtCol1 = @vOldsrceCol1 AND trgtColn = @vOldsrceColn
				//   END
				//   FETCH NEXT FROM cu_trgttblname_trgtCol1_trgtColn_oldcursor INTO @vOldsrceCol1, @vOldsrceColn
				//   FETCH NEXT FROM cu_trgttblname_trgtCol1_trgtColn_newcursor INTO @vNewsrceCol1, @vNewsrceColn
				// END
				// CLOSE cu_trgttblname_trgtCol1_trgtColn_oldcursor
				// CLOSE cu_trgttblname_trgtCol1_trgtColn_newcursor
				// DEALLOCATE cu_trgttblname_trgtCol1_trgtColn_oldcursor
				// DEALLOCATE cu_trgttblname_trgtCol1_trgtColn_newcursor
				//

				// No need to create a trigger for root relations
				//
				if (!m_pParentKey)
					return strSQL;

				// No need to create a trigger if the source key is an AutoNum
				//
				if (m_pParentKey->IsAutoNum())
					return strSQL;

				// Create trigger name
				//
				strTriggerName  = "ufr_";
				strTriggerName += m_pChildKey->GetMetaEntity()->GetName();

				for ( itrTrgtMetaColumn =  m_pChildKey->GetMetaColumns()->begin(), idx = 0;
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && idx < m_pParentKey->GetColumnCount();
							itrTrgtMetaColumn++,																					idx++)
				{
					strTriggerName += "_";
					strTriggerName += (*itrTrgtMetaColumn)->GetName();
				}

				// CREATE TRIGGER ccr_trgttblname_trgtCol1_trgtColn ON srcetblname
				// FOR UPDATE AS
				//
				strSQL  = "CREATE TRIGGER ";
				strSQL += strTriggerName;
				strSQL += " ON ";
				strSQL += m_pParentKey->GetMetaEntity()->GetName();
				strSQL += "\n";
				strSQL += "FOR UPDATE AS\n";

				// DECLARE @vOldsrceCol1 varchar(12)
				// DECLARE @vNewsrceCol1 varchar(12)
				// DECLARE @vOldsrceColn int
				// DECLARE @vNewsrceColn int
				//
				for ( itrSrceMetaColumn =  m_pParentKey->GetMetaColumns()->begin(), idx = 0;
							itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end() && idx < m_pChildKey->GetColumnCount();
							itrSrceMetaColumn++,																					idx++)
				{
					strSQL += "DECLARE @vOld";
					strSQL += (*itrSrceMetaColumn)->GetName();
					strSQL += " ";
					strSQL += (*itrSrceMetaColumn)->GetTypeAsSQLString(eTargetDB);
					strSQL += "\n";
					strSQL += "DECLARE @vNew";
					strSQL += (*itrSrceMetaColumn)->GetName();
					strSQL += " ";
					strSQL += (*itrSrceMetaColumn)->GetTypeAsSQLString(eTargetDB);
					strSQL += "\n";
				}

				// DECLARE cu_trgttblname_trgtCol1_trgtColn_oldcursor CURSOR LOCAL FOR SELECT srceCol1 FROM deleted

				// DECLARE cu_trgttblname_trgtCol1_trgtColn_newcursor CURSOR LOCAL FOR SELECT srceCol1 FROM inserted
				//
				strSQL += "DECLARE ";
				strSQL += strTriggerName;
				strSQL += "_oldcursor CURSOR LOCAL FOR SELECT ";

				for ( itrSrceMetaColumn =  m_pParentKey->GetMetaColumns()->begin(), idx = 0;
							itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end() && idx < m_pChildKey->GetColumnCount();
							itrSrceMetaColumn++,																					idx++)
				{
					if (idx > 0)
						strSQL += ", ";

					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				strSQL += " FROM deleted\n";


				strSQL += "DECLARE ";
				strSQL += strTriggerName;
				strSQL += "_newcursor CURSOR LOCAL FOR SELECT ";

				for ( itrSrceMetaColumn =  m_pParentKey->GetMetaColumns()->begin(), idx = 0;
							itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end() && idx < m_pChildKey->GetColumnCount();
							itrSrceMetaColumn++,																					idx++)
				{
					if (idx > 0)
						strSQL += ", ";

					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				strSQL += " FROM inserted\n";

				// OPEN cu_trgttblname_trgtCol1_trgtColn_oldcursor
				// OPEN cu_trgttblname_trgtCol1_trgtColn_newcursor
				//
				strSQL += "OPEN ";
				strSQL += strTriggerName;
				strSQL += "_oldcursor\n";

				strSQL += "OPEN ";
				strSQL += strTriggerName;
				strSQL += "_newcursor\n";

				// FETCH NEXT FROM cu_trgttblname_trgtCol1_trgtColn_oldcursor INTO @vOldsrceCol1, @vOldsrceColn
				// FETCH NEXT FROM cu_trgttblname_trgtCol1_trgtColn_newcursor INTO @vNewsrceCol1, @vNewsrceColn
				//
				strSQL += "FETCH NEXT FROM ";
				strSQL += strTriggerName;
				strSQL += "_oldcursor INTO ";

				for ( itrSrceMetaColumn =  m_pParentKey->GetMetaColumns()->begin(), idx = 0;
							itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end() && idx < m_pChildKey->GetColumnCount();
							itrSrceMetaColumn++,																					idx++)
				{
					if (idx > 0)
						strSQL += ", ";

					strSQL += "@vOld";
					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				strSQL += "\n";

				strSQL += "FETCH NEXT FROM ";
				strSQL += strTriggerName;
				strSQL += "_newcursor INTO ";

				for ( itrSrceMetaColumn =  m_pParentKey->GetMetaColumns()->begin(), idx = 0;
							itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end() && idx < m_pChildKey->GetColumnCount();
							itrSrceMetaColumn++,																					idx++)
				{
					if (idx > 0)
						strSQL += ", ";

					strSQL += "@vNew";

					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				strSQL += "\n";

				// WHILE @@FETCH_STATUS = 0
				// BEGIN
				//
				strSQL += "WHILE @@FETCH_STATUS = 0\n";
				strSQL += "BEGIN\n";

				//   IF (@vNewsrceCol1 <> @vOldsrceCol1 OR @vNewsrceColn <> @vOldsrceColn)
				//
				strSQL += "  IF (";

				for ( itrSrceMetaColumn =  m_pParentKey->GetMetaColumns()->begin(), idx = 0;
							itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end() && idx < m_pChildKey->GetColumnCount();
							itrSrceMetaColumn++,																					idx++)
				{
					if (idx > 0)
						strSQL += " OR ";

					strSQL += "@vOld";
					strSQL += (*itrSrceMetaColumn)->GetName();
					strSQL += " <> ";
					strSQL += "@vNew";
					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				strSQL += ")\n";

				//   BEGIN
				//     UPDATE trgttblname SET trgtCol1 = @vNewsrceCol1, trgtColn = @vNewsrceColn WHERE trgtCol1 = @vOldsrceCol1 AND trgtColn = @vOldsrceColn
				//   END
				//
				strSQL += "  BEGIN\n";
				strSQL += "    UPDATE ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += " SET ";

				for ( itrTrgtMetaColumn  = m_pChildKey->GetMetaColumns()->begin(),	itrSrceMetaColumn  = m_pParentKey->GetMetaColumns()->begin();
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() &&	itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end();
							itrTrgtMetaColumn++,																					itrSrceMetaColumn++)
				{
					if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
						strSQL += ", ";

					strSQL += (*itrTrgtMetaColumn)->GetName();
					strSQL += " = @vNew";
					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				strSQL += " WHERE ";

				for ( itrTrgtMetaColumn  = m_pChildKey->GetMetaColumns()->begin(),	itrSrceMetaColumn  = m_pParentKey->GetMetaColumns()->begin();
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() &&	itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end();
							itrTrgtMetaColumn++,																					itrSrceMetaColumn++)
				{
					if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
						strSQL += " AND ";

					strSQL += (*itrTrgtMetaColumn)->GetName();
					strSQL += " = @vOld";
					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				// If we have multiple relations between the same two keys, take child switches into account
				//
				if (listMR.size() > 1)
				{
					std::string		strSwitch;


					// Find one that is not a switched child
					//
					for ( itrMR =  listMR.begin();
								itrMR != listMR.end();
								itrMR++)
					{
						pMR = *itrMR;

						if (!pMR->IsChildSwitch())
							break;
					}

					// Only do this if all children are switched
					//
					if (itrMR == listMR.end())
					{
						for ( itrMR =  listMR.begin();
									itrMR != listMR.end();
									itrMR++)
						{
							pMR = *itrMR;

							if (!strSwitch.empty())
								strSwitch += " OR ";

							strSwitch += pMR->GetSwitchColumn()->GetMetaColumn()->GetName();
							strSwitch += " = ";
							strSwitch += pMR->GetSwitchColumn()->AsSQLString();
						}
					}

					if (!strSwitch.empty())
					{
						strSQL += " AND (";
						strSQL += strSwitch;
						strSQL += ")";
					}
				}

				strSQL += "\n";
				strSQL += "  END\n";

				// FETCH NEXT FROM cu_trgttblname_trgtCol1_trgtColn_oldcursor INTO @vOldsrceCol1, @vOldsrceColn
				// FETCH NEXT FROM cu_trgttblname_trgtCol1_trgtColn_newcursor INTO @vNewsrceCol1, @vNewsrceColn
				//
				strSQL += "  FETCH NEXT FROM ";
				strSQL += strTriggerName;
				strSQL += "_oldcursor INTO ";

				for ( itrSrceMetaColumn =  m_pParentKey->GetMetaColumns()->begin(), idx = 0;
							itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end() && idx < m_pChildKey->GetColumnCount();
							itrSrceMetaColumn++,																					idx++)
				{
					if (idx > 0)
						strSQL += ", ";

					strSQL += "@vOld";
					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				strSQL += "\n";


				strSQL += "  FETCH NEXT FROM ";
				strSQL += strTriggerName;
				strSQL += "_newcursor INTO ";

				for ( itrSrceMetaColumn =  m_pParentKey->GetMetaColumns()->begin(), idx = 0;
							itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end() && idx < m_pChildKey->GetColumnCount();
							itrSrceMetaColumn++,																					idx++)
				{
					if (idx > 0)
						strSQL += ", ";

					strSQL += "@vNew";
					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				strSQL += "\n";

				// END
				// CLOSE cu_trgttblname_trgtCol1_trgtColn_oldcursor
				// CLOSE cu_trgttblname_trgtCol1_trgtColn_newcursor
				// DEALLOCATE cu_trgttblname_trgtCol1_trgtColn_oldcursor
				// DEALLOCATE cu_trgttblname_trgtCol1_trgtColn_newcursor
				//
				strSQL += "END\n";
				strSQL += "CLOSE ";
				strSQL += strTriggerName;
				strSQL += "_oldcursor\n";
				strSQL += "CLOSE ";
				strSQL += strTriggerName;
				strSQL += "_newcursor\n";
				strSQL += "DEALLOCATE ";
				strSQL += strTriggerName;
				strSQL += "_oldcursor\n";
				strSQL += "DEALLOCATE ";
				strSQL += strTriggerName;
				strSQL += "_newcursor\n";

				break;
			}

			case Oracle:
			{
				std::string						strTriggerName;
				std::string						strPackageName;
				char									szBuffer[20];
				char									szPackageBuffer[20];
				MetaColumnPtrListItr	itrTrgtMetaColumn, itrSrceMetaColumn;

				// CREATE OR REPLACE TRIGGER UFR_RULE
				// AFTER UPDATE OF ID ON PARENT_TBL
				// FOR EACH ROW
				// BEGIN
				// UPDATE CHILD_TBL SET PID = :NEW.ID WHERE PID = :OLD.ID;
				// END;



				// No need to create a trigger for root relations
				//
				if (!m_pParentKey)
					return strSQL;

				// No need to create a trigger if the source key is an AutoNum
				//
				if (m_pParentKey->IsAutoNum())
					return strSQL;

				if (m_pParentKey->GetMetaEntity()->GetName() == m_pChildKey->GetMetaEntity()->GetName())
				{
					strSQL  = "BEGIN ";
					strSQL += "update_cascade.on_table('";
					strSQL += m_pParentKey->GetMetaEntity()->GetName();
					strSQL += "');";
					strSQL += " END;";
					return strSQL;
				}

				// Create Package name
				//
				sprintf(szPackageBuffer, "E%02i_E%02iK%02i", m_pParentKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaKeyIndex());

				strPackageName  = "pk_";
				strPackageName += m_pParentKey->GetMetaEntity()->GetMetaDatabase()->GetAlias();
				strPackageName += "_";
				strPackageName += szPackageBuffer;

				if (strPackageName.size() > 30)
				{
					ReportWarning("MetaRelation::AsCreateCascadePackage(): Name %s of generated %s  %s exceedes maximum length of 30 characters. Using truncated name.", m_pChildKey->GetName().c_str(), m_pChildKey->GetName().c_str(), strPackageName.c_str());
					strPackageName = strPackageName.substr(0, 30);
				}

				std::string strPackage(strPackageName,0,30);

				// Create trigger name
				//
				sprintf(szBuffer, "E%02i_E%02iK%02i", m_pParentKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaEntity()->GetEntityIdx(), m_pChildKey->GetMetaKeyIndex());

				strTriggerName  = "ufr_";
				strTriggerName += m_pParentKey->GetMetaEntity()->GetMetaDatabase()->GetAlias();
				strTriggerName += "_";
				strTriggerName += szBuffer;

				if (strTriggerName.size() > 30)
				{
					ReportWarning("MetaRelation::AsUpdateForeignRefTrigger(): Name %s of generated update foreign reference trigger for relation from key %s to key %s exceedes maximum length of 30 characters. Using truncated name.", m_pChildKey->GetName().c_str(), m_pChildKey->GetName().c_str(), strTriggerName.c_str());
					strTriggerName = strTriggerName.substr(0, 30);
				}

				std::string strTrigger(strTriggerName,0,30);

				// CREATE TRIGGER ccr_trgttblname_trgtCol1_trgtColn ON srcetblname
				// FOR UPDATE AS
				//
				strSQL  = "CREATE TRIGGER ";
				strSQL += strTrigger;
				strSQL += " AFTER UPDATE OF ";

				for ( itrTrgtMetaColumn  = m_pChildKey->GetMetaColumns()->begin(), itrSrceMetaColumn  = m_pParentKey->GetMetaColumns()->begin();
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end();
							itrTrgtMetaColumn++,																					itrSrceMetaColumn++)
				{
					if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
						strSQL += ",";

					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				strSQL += " ON ";
				strSQL += m_pParentKey->GetMetaEntity()->GetName();
				strSQL += "\n";
				strSQL += "FOR EACH ROW\n";
				strSQL += "BEGIN\n";
				strSQL += strPackage;
				strSQL += ".fire_trigger := 'NO';\n";

				strSQL += "    UPDATE ";
				strSQL += m_pChildKey->GetMetaEntity()->GetName();
				strSQL += " SET ";

				for ( itrTrgtMetaColumn  = m_pChildKey->GetMetaColumns()->begin(), itrSrceMetaColumn  = m_pParentKey->GetMetaColumns()->begin();
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end();
							itrTrgtMetaColumn++,																					itrSrceMetaColumn++)
				{
					if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
						strSQL += ", ";

					strSQL += (*itrTrgtMetaColumn)->GetName();
					strSQL += " = :NEW.";
					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				strSQL += " WHERE ";

				for ( itrTrgtMetaColumn  = m_pChildKey->GetMetaColumns()->begin(), itrSrceMetaColumn  = m_pParentKey->GetMetaColumns()->begin();
							itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->end() && itrSrceMetaColumn != m_pParentKey->GetMetaColumns()->end();
							itrTrgtMetaColumn++,																					itrSrceMetaColumn++)
				{
					if (itrTrgtMetaColumn != m_pChildKey->GetMetaColumns()->begin())
						strSQL += " AND ";

					strSQL += (*itrTrgtMetaColumn)->GetName();
					strSQL += " = :OLD.";
					strSQL += (*itrSrceMetaColumn)->GetName();
				}

				// If we have multiple relations between the same two keys, take child switches into account
				//
				if (listMR.size() > 1)
				{
					std::string		strSwitch;

					// Find one that is not a switched child
					//
					for ( itrMR =  listMR.begin();
								itrMR != listMR.end();
								itrMR++)
					{
						pMR = *itrMR;

						if (!pMR->IsChildSwitch())
							break;
					}

					// Only do this if all children are switched
					//
					if (itrMR == listMR.end())
					{
						for ( itrMR =  listMR.begin();
									itrMR != listMR.end();
									itrMR++)
						{
							pMR = *itrMR;

							if (!strSwitch.empty())
								strSwitch += " OR ";

							strSwitch += pMR->GetSwitchColumn()->GetMetaColumn()->GetName();
							strSwitch += " = ";
							strSwitch += pMR->GetSwitchColumn()->AsSQLString();
						}
					}

					if (!strSwitch.empty())
					{
						strSQL += " AND (";
						strSQL += strSwitch;
						strSQL += ")";
					}
				}

				strSQL += ";\n";
				strSQL += strPackage;
				strSQL += ".fire_trigger := 'YES';\n";
				strSQL += "END;\n";

				break;
			}
		}
		return strSQL;
	}



	bool MetaRelation::VerifyKeyCompatibility()
	{
		MetaColumnPtrListItr		itrParentColumn, itrChildColumn;
		MetaColumnPtr						pParentColumn, pChildColumn;


		if (m_pParentKey->GetColumnCount() > m_pChildKey->GetColumnCount())
			throw Exception(__FILE__, __LINE__, Exception_error, "MetaRelation::MetaRelation(): ParentKey %s has more columns than ChildKey %s!", m_pParentKey->GetFullName().c_str(), m_pChildKey->GetFullName().c_str());

		for ( itrParentColumn =  m_pParentKey->GetMetaColumns()->begin(),
					itrChildColumn =  m_pChildKey->GetMetaColumns()->begin();
					itrParentColumn != m_pParentKey->GetMetaColumns()->end();
					itrParentColumn++,
					itrChildColumn++)
		{
			pParentColumn = *itrParentColumn;
			pChildColumn = *itrChildColumn;

			if (*pParentColumn != *pChildColumn)
				return false;
		}

		return true;
	}



	void MetaRelation::GetSynonymRelations(MetaRelationPtrList & listMR)
	{
		MetaEntityPtr				pParentME;
		MetaRelationPtr			pCandiateRel;



		listMR.clear();

		pParentME = GetParentMetaKey()->GetMetaEntity();

		for (unsigned int idx = 0; idx < pParentME->GetChildMetaRelationCount(); idx++)
		{
			pCandiateRel = pParentME->GetChildMetaRelation(idx);

			if (pCandiateRel->m_pParentKey == m_pParentKey &&
					pCandiateRel->m_pChildKey  == m_pChildKey)
				listMR.push_back(pCandiateRel);
		}
	}



	RelationPtr MetaRelation::CreateInstance(EntityPtr pEntity, DatabasePtr pChildDatabase)
	{
		RelationPtr			pRelation;

		// Some sanity checks
		//
		assert(pEntity);
		assert(pChildDatabase);
		assert(pEntity->GetMetaEntity() == m_pParentKey->GetMetaEntity());
		assert(m_pInstanceClass);

		// Create and initialise the object
		//
		pRelation = (RelationPtr) m_pInstanceClass->CreateInstance();

		pRelation->m_pMetaRelation = this;
		pRelation->m_pParentKey = pEntity->GetInstanceKey(m_pParentKey->GetMetaKeyIndex());
		pRelation->m_pChildDatabase = pChildDatabase;

		// Post the On_PostCreate() message which allows the object
		// itself to carry out further initialisation
		//
		pRelation->On_PostCreate();

		return pRelation;
	}



	std::string	MetaRelation::GetFullName() const
	{
		std::string		strFullName;


		if (m_pParentKey)
		{
			strFullName  = m_pParentKey->GetMetaEntity()->GetFullName();
			strFullName += " ";
			strFullName += m_strName;
			strFullName += " ";
			strFullName += m_pChildKey->GetMetaEntity()->GetFullName();
		}
		else
		{
			strFullName  = m_pChildKey->GetMetaEntity()->GetFullName();
			strFullName += " ";
			strFullName += m_strName;
		}

		return strFullName;
	}



	std::string	MetaRelation::GetFullReverseName() const

	{
		std::string		strFullName;


		assert(m_pParentKey);

		strFullName  = m_pChildKey->GetMetaEntity()->GetFullName();
		strFullName += " ";
		strFullName += m_strReverseName;
		strFullName += " ";
		strFullName += m_pParentKey->GetMetaEntity()->GetFullName();

		return strFullName;
	}



	std::string	MetaRelation::GetUniqueName() const
	{
		unsigned int			idx;
		MetaEntityPtr			pParentME;
		MetaRelationPtr		pMR;
		int								iSynonyms = 0;
		std::string				strUniqueName = GetName();


		if (!m_pParentKey)
			return strUniqueName;

		pParentME = m_pParentKey->GetMetaEntity();


		for (idx = 0; idx < pParentME->GetChildMetaRelationCount(); idx++)

		{
			pMR = pParentME->GetChildMetaRelation(idx);

			if (pMR == this)
				break;

			if (pMR->GetName() == m_strName)
				iSynonyms++;
		}

		if (iSynonyms > 0)
		{
			std::string			strSuffix;

			Convert(strSuffix, iSynonyms);
			strUniqueName += "_";
			strUniqueName += strSuffix;
		}

		return strUniqueName;
	}



	std::string	MetaRelation::GetUniqueReverseName() const
	{
		unsigned int			idx;
		MetaEntityPtr			pChildME;
		MetaRelationPtr		pMR;
		int								iSynonyms = 0;

		std::string				strUniqueReverseName = GetReverseName();


		if (!m_pChildKey)
			return strUniqueReverseName;

		pChildME = m_pChildKey->GetMetaEntity();

		for (idx = 0; idx < pChildME->GetParentMetaRelationCount(); idx++)
		{
			pMR = pChildME->GetParentMetaRelation(idx);

			if (pMR == this)
				break;

			if (pMR->GetReverseName()   == m_strReverseName  &&
					pMR->GetParentMetaKey() == m_pParentKey)
				iSynonyms++;
		}

		if (iSynonyms > 0)
		{
			std::string			strSuffix;

			Convert(strSuffix, iSynonyms);
			strUniqueReverseName += "_";
			strUniqueReverseName += strSuffix;
		}

		return strUniqueReverseName;
	}



	/* static */
	std::ostream & MetaRelation::AllAsJSON(RoleUserPtr pRoleUser, std::ostream & ojson)
	{
		D3::RolePtr									pRole = pRoleUser ? pRoleUser->GetRole() : NULL;
		D3::MetaDatabasePtr					pMD;
		D3::MetaDatabasePtrMapItr		itrMD;
		D3::MetaEntityPtr						pME;
		D3::MetaEntityPtrVectItr		itrME;
		D3::MetaRelationPtr					pMR;
		D3::MetaRelationPtrVectItr	itrMR;
		bool												bFirst = true;

		ojson << "{\n\t\"CR\":\n\t[";

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

				for ( itrMR =  pME->GetChildMetaRelations()->begin();
							itrMR != pME->GetChildMetaRelations()->end();
							itrMR++)
				{
					pMR = *itrMR;

					if (pRole->CanReadParent(pMR) || pRole->CanReadChildren(pMR))
					{
						bFirst ? bFirst = false : ojson << ',';
						ojson << "\n\t\t";
						pMR->AsJSON(pRoleUser, ojson, NULL, NULL, true);
					}
				}
			}
		}

		ojson << "\n\t],\n\t\"PR\":\n\t[";

		bFirst = true;

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

				for ( itrMR =  pME->GetParentMetaRelations()->begin();
							itrMR != pME->GetParentMetaRelations()->end();
							itrMR++)
				{
					pMR = *itrMR;

					if (pRole->CanReadParent(pMR) || pRole->CanReadChildren(pMR))
					{
						bFirst ? bFirst = false : ojson << ',';
						ojson << "\n\t\t";
						ojson << "{\"ME\":" << pMR->GetChildMetaKey()->GetMetaEntity()->GetID() << ",\"MR\":" << pMR->GetID() << '}';
					}
				}
			}
		}

		ojson << "\n\t]\n}";

		return ojson;
	}



	std::ostream & MetaRelation::AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, MetaEntityPtr pContext, bool* pFirstSibling, bool bShallow)
	{
		RolePtr										pRole = pRoleUser ? pRoleUser->GetRole() : NULL;

		if (!pRole || pRole->CanReadParent(this) || pRole->CanReadChildren(this))
		{
			if (pFirstSibling)
			{
				if (*pFirstSibling)
					*pFirstSibling = false;
				else
					ostrm << ',';
			}
/*
			std::string		strBase64Desc;
			std::string		strBase64RevDesc;

			APALUtil::base64_encode(strBase64Desc, (const unsigned char *) GetDescription().c_str(), GetDescription().size());
			APALUtil::base64_encode(strBase64RevDesc, (const unsigned char *) GetReverseDescription().c_str(), GetReverseDescription().size());
*/
			ostrm << "{";
			ostrm << "\"ID\":"										<< GetID() << ',';
			ostrm << "\"ParentIdx\":"							<< GetParentIdx() << ',';
			ostrm << "\"ChildIdx\":"							<< GetChildIdx() << ',';

			ostrm << "\"Name\":\""								<< JSONEncode(GetName()) << "\",";
			ostrm << "\"ProsaName\":\""						<< JSONEncode(GetProsaName()) << "\",";
//			ostrm << "\"Description\":\""					<< strBase64Desc << "\",";
			ostrm << "\"ReverseName\":\""					<< JSONEncode(GetReverseName()) << "\",";
			ostrm << "\"ReverseProsaName\":\""		<< JSONEncode(GetReverseProsaName()) << "\",";
//			ostrm << "\"ReverseDescription\":\""	<< strBase64RevDesc << "\",";

			ostrm << "\"InstanceClass\":\""				<< JSONEncode(GetInstanceClassName()) << "\",";
			ostrm << "\"InstanceInterface\":\""		<< JSONEncode(GetInstanceInterface()) << "\",";
			ostrm << "\"JSMetaClass\":\""					<< JSONEncode(GetJSMetaClassName()) << "\",";
			ostrm << "\"JSInstanceClass\":\""			<< JSONEncode(GetJSInstanceClassName()) << "\",";
			ostrm << "\"JSParentViewClass\":\""		<< JSONEncode(GetJSParentViewClassName()) << "\",";
			ostrm << "\"JSChildViewClass\":\""		<< JSONEncode(GetJSChildViewClassName()) << "\",";

			ostrm << "\"CascadeDelete\":"					<< (IsCascadeDelete()			? "true" : "false") << ",";
			ostrm << "\"CascadeClearRef\":"				<< (IsCascadeClearRef()		? "true" : "false") << ",";
			ostrm << "\"PreventDelete\":"					<< (IsPreventDelete()			? "true" : "false") << ",";

			ostrm << "\"HiddenParent\":"					<< (IsHiddenParent()			? "true" : "false") << ",";
			ostrm << "\"HiddenChildren\":"				<< (IsHiddenChildren()		? "true" : "false") << ",";

			ostrm << "\"ReadParent\":"						<< (!pRole || pRole->CanReadParent(this)				? "true" : "false") << ",";
			ostrm << "\"ReadChildren\":"					<< (!pRole || pRole->CanReadChildren(this)			? "true" : "false") << ",";
			ostrm << "\"ChangeParent\":"					<< (!pRole || pRole->CanChangeParent(this)			? "true" : "false") << ",";
			ostrm << "\"AddRemoveChildren\":"			<< (!pRole || pRole->CanAddRemoveChildren(this)	? "true" : "false") << ",";

			ostrm << "\"HSTopics\":"							<< (m_strHSTopicsJSON.empty() ? "null" : m_strHSTopicsJSON) << ",";

			if (!bShallow && pContext != GetParentMetaKey()->GetMetaEntity())
			{
				ostrm << "\"ParentKey\":";
				GetParentMetaKey()->AsJSON(NULL, ostrm, NULL);
				ostrm << ",";
			}
			else
			{
				ostrm << "\"ParentKeyID\":"								<< GetParentMetaKey()->GetID() << ",";
			}

			if (!bShallow && pContext != GetChildMetaKey()->GetMetaEntity())
			{
				ostrm << "\"ChildKey\":";
				GetChildMetaKey()->AsJSON(NULL, ostrm, NULL);
				ostrm << ",";
			}
			else
			{
				ostrm << "\"ChildKeyID\":"								<< GetChildMetaKey()->GetID() << ",";
			}

			ostrm << "\"Switch\":";
			if (IsParentSwitch() || IsChildSwitch())
			{
				ostrm << "{";
				ostrm << "\"ParentSwitched\":"	<< (IsParentSwitch()		? "true" : "false") << ",";
				ostrm << "\"ColumnID\":"				<< m_pSwitchColumn->GetMetaColumn()->GetID() << ',';

				switch (m_pSwitchColumn->GetMetaColumn()->GetType())
				{
					case MetaColumn::dbfBool:
						ostrm << "\"Value\":\""	<< m_pSwitchColumn->AsBool();
						break;
					case MetaColumn::dbfString:
					case MetaColumn::dbfChar:
						ostrm << "\"Value\":\""	<< JSONEncode(m_pSwitchColumn->AsString(false))	<< "\"";
						break;
					case MetaColumn::dbfShort:
					case MetaColumn::dbfInt:
					case MetaColumn::dbfLong:
						ostrm << "\"Value\":"	<< m_pSwitchColumn->AsString(false);
						break;

					default:
						ostrm << "\"Value\":\"ERROR: Switch column type not supported as switch column\"";
				}

				ostrm << "}";
			}
			else
			{
				ostrm << "null";
			}
			ostrm << "}";
		}

		return ostrm;
	}








	// ===========================================================
	// Relation class implementation
	//

	// Standard D3 Stuff
	//
	D3_CLASS_IMPL(Relation, Object);


	Relation::~Relation()
	{
		MarkDestroying();

		RemoveAllChildren();

		if (m_pParentKey)
			m_pParentKey->On_ChildRelationDeleted(this);

		if (m_pParentKey->GetEntity())
			m_pParentKey->GetEntity()->On_ChildRelationDeleted(this);

		if (m_pMetaRelation)
			m_pMetaRelation->On_InstanceDeleted(this);
	}



	void Relation::RemoveAllChildren()
	{
		InstanceKeyPtrSetItr			itrChildKey;
		InstanceKeyPtr						pKey;
		EntityPtr									pEntity;
		bool											bDone = false;


		// Note: Since deleting children could potentially also delete other children in this relation, we restart this loop after deleting a child
		while (!bDone)
		{
			bDone = true;

			for ( itrChildKey =  m_setChildKey.begin();
						itrChildKey != m_setChildKey.end();
						itrChildKey++)
			{
				pKey = (InstanceKeyPtr) *itrChildKey;
				pEntity = pKey->GetEntity();

				if (!pEntity->IsDestroying())
				{
					delete pEntity;
					bDone = false;
					break;
				}
			}
		}
	}



	EntityPtr Relation::iterator::operator*()
	{
		InstanceKeyPtr	pKey;
		EntityPtr				pEntity;

		pKey = (InstanceKeyPtr) ((InstanceKeyPtrSetItr*) this)->operator*();
		pEntity = pKey->GetEntity();

		return pEntity;
	}



	Relation::iterator& Relation::iterator::operator=(const iterator & itr)
	{
		((InstanceKeyPtrSetItr*) this)->operator=(itr);

		return *this;
	}


	// Getting "the" target key is fine only if the target key is UNIQUE and we have
	// exactly one member in the list
	//
	InstanceKeyPtr Relation::GetChildKey()
	{
		if (m_pMetaRelation->GetChildMetaKey()->IsUnique())
		{
			if (m_setChildKey.size() == 1)
				return (InstanceKeyPtr) *(m_setChildKey.begin());
		}

		return NULL;
	}



	// Return the database of the entity which owns the source key
	//
	DatabasePtr Relation::GetParentDatabase()
	{
		if (!m_pParentKey)
			return NULL;

		return m_pParentKey->GetEntity()->GetDatabase();
	}



	EntityPtr Relation::front()
	{
		KeyPtr		pKey;


		if (m_setChildKey.empty())
			return NULL;


		pKey = *(m_setChildKey.begin());

		if (!pKey)
			return NULL;

		return pKey->GetEntity();
	}



	EntityPtr Relation::back()
	{
		KeyPtr								pKey;
		InstanceKeyPtrSetItr	itrChildKey;


		if (m_setChildKey.empty())
			return NULL;

		itrChildKey = m_setChildKey.end();
		itrChildKey--;
		pKey = *itrChildKey;

		if (!pKey)
			return NULL;

		return pKey->GetEntity();
	}



	// Remove a child
	//
	void Relation::RemoveChild(EntityPtr pChildEntity)
	{
		InstanceKeyPtr						pChildKey;
		InstanceKeyPtrSetItr			itrChildKey;


		assert (pChildEntity);
		assert (pChildEntity->GetDatabase() == m_pChildDatabase);

		if (m_pPendingChild)
		{
			assert(m_pPendingChild == pChildEntity);
			return;
		}

		m_pPendingChild = pChildEntity;

		pChildKey = pChildEntity->GetInstanceKey(m_pMetaRelation->GetChildMetaKey()->GetMetaKeyIndex());
		itrChildKey = FindChild(pChildKey, true);

		if (itrChildKey != m_setChildKey.end())
		{
			assert(pChildKey == *itrChildKey);

			m_setChildKey.erase(itrChildKey);

			pChildKey->On_ParentRelationDeleted(this);
			pChildEntity->On_ParentRelationDeleted(this);
		}

		// Delete this if relation is empty AND it is between a parent object
		// belonging to another database than the children
		if (m_setChildKey.empty() && !IsDestroying() && m_pChildDatabase != m_pParentKey->GetEntity()->GetDatabase())
		{
			delete this;
			return;
		}

		m_pPendingChild = NULL;
	}


	// Add a child
	//
	void Relation::AddChild(EntityPtr pChildEntity)
	{
		InstanceKeyPtr						pChildKey;


		assert (pChildEntity);
		assert (pChildEntity->GetDatabase() == m_pChildDatabase);

		if (m_pPendingChild)
		{
			assert(m_pPendingChild == pChildEntity);
			return;
		}

		pChildKey = pChildEntity->GetInstanceKey(m_pMetaRelation->GetChildMetaKey()->GetMetaKeyIndex());

		assert(FindChild(pChildKey) == m_setChildKey.end());

		if (!IsValidChild(pChildEntity))
			return;

		m_pPendingChild = pChildEntity;

		if (*pChildKey != *m_pParentKey)
		{
			pChildKey->On_BeforeUpdate();
			*pChildKey = *m_pParentKey;
			pChildKey->On_AfterUpdate();
		}

		m_setChildKey.insert(pChildKey);


		pChildKey->On_ParentRelationCreated(this);
		pChildEntity->On_ParentRelationCreated(this);

		m_pPendingChild = NULL;
	}



	// Populates the Relation
	void Relation::LoadAll(bool bRefresh, bool bLazyFetch)
	{
		if (!m_pChildDatabase || !m_pParentKey || m_pParentKey->IsNull())
			return;

		m_pChildDatabase->LoadObjects(this, bRefresh, bLazyFetch);
	}



	void Relation::GetChildrenSnapshot(EntityPtrList & listEntity)
	{
		InstanceKeyPtrSetItr			itrChildKey;
		InstanceKeyPtr						pChildKey;


		for ( itrChildKey =  m_setChildKey.begin();
					itrChildKey != m_setChildKey.end();
					itrChildKey++)
		{
			pChildKey = (InstanceKeyPtr) *itrChildKey;

			listEntity.push_back(pChildKey->GetEntity());
		}
	}



	// Searches the child in our multiset of child keys
	//
	InstanceKeyPtrSetItr Relation::FindChild(InstanceKeyPtr pChildKey, bool bShouldExist)
	{
		InstanceKeyPtrSetItr			itrChildKey;
		InstanceKeyPtr						pCurrentChildKey;


		for ( itrChildKey = m_setChildKey.find(pChildKey);
					itrChildKey != m_setChildKey.end();
					itrChildKey++)
		{
			pCurrentChildKey = (InstanceKeyPtr) *itrChildKey;

			if (pChildKey == pCurrentChildKey)
				return itrChildKey;

			if (*pCurrentChildKey != *pChildKey)
				break;
		}

		if (bShouldExist && m_pMetaRelation->GetParentMetaKey()->IsAutoNum())
		{
			// The brute-force search is needed if a foreign key consists of an
			// AutoNum column which is updated after a record was inserted. In
			// these cases, relationships do not receive notifications and hence
			// although the key is a related member it will not be found using
			// the normal approach.
			//
			for ( itrChildKey = m_setChildKey.begin();
						itrChildKey != m_setChildKey.end();
						itrChildKey++)
			{
				pCurrentChildKey = (InstanceKeyPtr) *itrChildKey;

				if (pChildKey == pCurrentChildKey)
					return itrChildKey;
			}
		}



		return m_setChildKey.end();
	}


	void Relation::On_PostCreate()
	{
		assert(m_pMetaRelation);
		assert(m_pParentKey);
		assert(m_pChildDatabase);
		assert(m_pParentKey->GetMetaKey() == m_pMetaRelation->GetParentMetaKey());

		// If the target is a target from a cached entity, we use the global database as target
		//
		if (m_pMetaRelation->GetChildMetaKey()->GetMetaEntity()->IsCached())
		{
			m_pChildDatabase = m_pMetaRelation->GetChildMetaKey()->GetMetaEntity()->GetMetaDatabase()->GetGlobalDatabase();
		}
		else
		{
			// Make sure the target database is of the appropriate type (if the relation is between keys from
			// entities belonging to different databases, it is possible that the target database is in fact
			// an instance from the source instead the target. In this case, we get the correct DB from the
			// DatabaseWorkspace
			//
			if (m_pChildDatabase->GetMetaDatabase() != m_pMetaRelation->GetChildMetaKey()->GetMetaEntity()->GetMetaDatabase())
				m_pChildDatabase = m_pChildDatabase->GetDatabaseWorkspace()->GetDatabase(m_pMetaRelation->GetChildMetaKey()->GetMetaEntity()->GetMetaDatabase());
		}


		m_pMetaRelation->On_InstanceCreated(this);

		m_pParentKey->On_ChildRelationCreated(this);
		m_pParentKey->GetEntity()->On_ChildRelationCreated(this);
	}




	void Relation::On_AfterUpdateParentKey(InstanceKeyPtr pKey)
	{
		InstanceKeyPtrSetItr	itrChildKey;
		InstanceKeyPtr				pChildKey;


		assert(pKey);
		assert(m_pParentKey == pKey);

		// If parent key is null, remove all children from this relation
		//
		if (pKey->IsNull())
		{
			while (!m_setChildKey.empty())
			{
				pChildKey = (InstanceKeyPtr) *(m_setChildKey.begin());
				RemoveChild(pChildKey->GetEntity());
			}

			UnMarkUpdating();
			return;
		}

		// Copy all kids to a new temporary list as changing children will cause
		// the kid keys to remove.insert themself
		//
		for ( itrChildKey =  m_setChildKey.begin();
					itrChildKey != m_setChildKey.end();
					itrChildKey++)
		{
			pChildKey = (InstanceKeyPtr) *itrChildKey;


			// stop if the first key is the same as the changed parent
			// (if this is true for the first child it will be true for all children)
			//
			if (*pChildKey == *m_pParentKey)
			{
				UnMarkUpdating();
				return;
			}

			// Update all children
			//
			pChildKey->On_BeforeUpdate();
			*pChildKey = *pKey;
			pChildKey->On_AfterUpdate();
		}

		UnMarkUpdating();
	}




	void Relation::On_BeforeUpdateChildKey(InstanceKeyPtr pKey)
	{
		if (IsUpdating())
			return;

		// If the parent key part is null, we can't remove the key. The problem is that by removing
		// the key we break the current relationship which in the case of a child key with a null
		// parent key part can't be restored due to potential ambiguities.
		// A classic example of this problem would arise if you created two new parent objects of a
		// meta entity with a single column autonum PK. If you created two dependent objects linked
		// to each parents respective PK, theparent key part of the children will be NULL.If we removed
		// one of the two children from the realation, we'd have no way of guaranteeing that after the
		// key changed we could reinsert it correctly since more than one matching parent exists.
		//
		if (pKey->IsKeyPartNull(m_pMetaRelation->GetParentMetaKey()))
			return;

		RemoveChild(pKey->GetEntity());
	}



	// Under normal circumstances, the On_BeforeUpdateRelation() notification received prior
	// to this notification will have removed the child and subsequent changes to the former
	// childs key would have resulted in the child having been inserted into the correct relation.
	// However, consider that the foreign key of entity y is a reference to an autonum column in
	// entity x. If you create a new instance of entity x and y and relate y with x, x has a NULL
	// foreign reference key. Therefore, On_BeforeUpdateRelation will not remove NULL keys.
	//
	void Relation::On_AfterUpdateChildKey(InstanceKeyPtr pKey)
	{
		if (IsUpdating())
			return;

		// This block is useful when the parent portion of a child key converts
		// from NULL to non-NULL or remains NULL while other portions change.
		// For these cases this method is expected to be efficient since keys
		// with a NULL parent portion will be at the begining of the set.
		//
		InstanceKeyPtrSetItr itrChildKey = FindChild(pKey, true);

		if (itrChildKey != m_setChildKey.end())
		{
			m_setChildKey.erase(itrChildKey);
			m_setChildKey.insert(pKey);
		}
	}



	bool Relation::IsValidChild(EntityPtr pEntity)
	{
		// If this is a switched relation, ensure the key meets the relation criteria
		//
		if (m_pMetaRelation->GetSwitchColumn())
		{
			if (m_pMetaRelation->IsParentSwitch())
			{
				if (*(m_pMetaRelation->GetSwitchColumn()) != *(m_pParentKey->GetEntity()->GetColumn(m_pMetaRelation->GetSwitchColumn()->GetMetaColumn())))
					return false;
			}
			else if (m_pMetaRelation->IsChildSwitch())
			{
				if (*(m_pMetaRelation->GetSwitchColumn()) != *(pEntity->GetColumn(m_pMetaRelation->GetSwitchColumn()->GetMetaColumn())))
					return false;
			}
		}

		return true;
	}



	std::ostream & Relation::AsJSON(RoleUserPtr pRoleUser, std::ostream & ostrm, bool* pFirstSibling)
	{
		RolePtr										pRole = pRoleUser ? pRoleUser->GetRole() : NULL;

		if (!pRole || pRole->CanReadParent(m_pMetaRelation) || pRole->CanReadChildren(m_pMetaRelation))
		{
			KeyPtr						pKey;

			if (pFirstSibling)
			{
				if (*pFirstSibling)
					*pFirstSibling = false;
				else
					ostrm << ',';
			}

			ostrm << "{";

			// Only bother about parent key
			if (m_pParentKey)
			{
				ostrm << "\"ParentKey\":";
				m_pParentKey->AsJSON(NULL, ostrm);
				ostrm << ",";
				ostrm << "\"ParentConceptualKey\":\"";
				pKey = GetParent()->GetConceptualKey();

				if (!pKey)
					pKey = GetParent()->GetPrimaryKey();

				ostrm << APALUtil::base64_encode(pKey->AsString()) << '"';
			}
			else
			{
				ostrm << "\"ParentKey\":null,";
				ostrm << "\"ParentConceptualKey\":null";
			}

			ostrm << "}";
		}

		return ostrm;
	}





	void Relation::GetSnapshot(EntityPtrListPtr pEL)
	{
		if (pEL)
		{
			InstanceKeyPtrSetItr itrChildKey;

			for (itrChildKey  = m_setChildKey.begin();
					 itrChildKey != m_setChildKey.end();
					 itrChildKey++)
			{
					pEL->push_back((*itrChildKey)->GetEntity());
			}
		}
	}





} // end namespace D3
