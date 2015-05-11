// MODULE: D3MetaRelation source

#include "D3Types.h"

#include "D3MetaRelation.h"

namespace D3
{
	D3_CLASS_IMPL(D3MetaRelation, D3MetaRelationBase);





	// Return a new instance populated with the details from the MetaRelation passed in
	D3MetaRelation::D3MetaRelation(DatabasePtr pDB, D3MetaKeyPtr pParentD3MK, D3MetaKeyPtr pChildD3MK, D3MetaColumnPtr pSwitchD3MC, MetaRelationPtr pMR, float fSequenceNo)
	{
		MarkConstructing();
		m_pMetaEntity = MetaDatabase::GetMetaDictionary()->GetMetaEntity(D3MDDB_D3MetaRelation);
		m_pDatabase = pDB;
		On_PostCreate();
		UnMarkConstructing();

		// Set the details from the meta type
		SetParentD3MetaKey(pParentD3MK);
		SetChildD3MetaKey(pChildD3MK);
		SetParentD3MetaEntity(pParentD3MK->GetD3MetaEntity());

		SetSequenceNo(fSequenceNo);
		SetName(pMR->GetName());
		SetProsaName(pMR->GetProsaName());
		SetDescription(pMR->GetDescription());
		SetReverseName(pMR->GetReverseName());
		SetReverseProsaName(pMR->GetReverseProsaName());
		SetReverseDescription(pMR->GetReverseDescription());
		SetMetaClass(pMR->IAm().Name());
		SetInstanceClass(pMR->GetInstanceClassName());
		SetInstanceInterface(pMR->GetInstanceInterface());
		SetJSMetaClass(pMR->GetJSMetaClassName());
		SetJSParentViewClass(pMR->GetJSParentViewClassName());
		SetJSChildViewClass(pMR->GetJSChildViewClassName());

		if (pSwitchD3MC)
		{
			SetSwitchColumnID(pSwitchD3MC->GetID());
			SetSwitchColumnValue(pSwitchD3MC->AsString());
		}
	}



	/* static */
	D3MetaRelationPtr D3MetaRelation::Create(	const std::string & strName,
																						const std::string & strReverseName,
																						const std::string & strInstanceClass,
																						const std::string & strInstanceInterface,
																						D3MetaKeyPtr	pMKParent,
																						D3MetaKeyPtr	pMKChild,
																						const MetaRelation::Flags & flags,
																						D3MetaColumnPtr	pMCSwitch,
																						const std::string & strSwitchColumnValue)
	{
		D3MetaRelationPtr			pMR;

		try
		{
			pMR = D3MetaRelationBase::CreateD3MetaRelation(pMKParent->GetDatabase());

			pMR->SetName							(strName);
			pMR->SetReverseName				(strReverseName);
			pMR->SetInstanceClass			(strInstanceClass);
			pMR->SetInstanceInterface	(strInstanceInterface);
			pMR->SetFlags							(flags.m_bitset.m_value);

			pMR->SetParentD3MetaKey		(pMKParent);
			pMR->SetChildD3MetaKey		(pMKChild);

			if (pMCSwitch || (flags & MetaRelation::Flags::ParentSwitch) || (flags & MetaRelation::Flags::ChildSwitch))
			{
				if (pMCSwitch &&
						(	((flags & MetaRelation::Flags::ParentSwitch) && pMCSwitch->GetD3MetaEntity() == pMKParent->GetD3MetaEntity()) ||
							((flags & MetaRelation::Flags::ChildSwitch)  && pMCSwitch->GetD3MetaEntity() == pMKChild->GetD3MetaEntity())))
				{
					pMR->SetSwitchD3MetaColumn(pMCSwitch);
					pMR->SetSwitchColumnValue(strSwitchColumnValue);
				}
				else
				{
					throw Exception(__FILE__, __LINE__, Exception_error, "D3MetaRelation::Create(): D3MetaRelation [%s %s] specifies incorrect relation switch details", pMKParent->GetD3MetaEntity()->GetName().c_str(), strName.c_str());
				}
			}
		}
		catch(Exception & e)
		{
			e.AddMessage("D3MetaRelation::Create(): Error creating D3MetaRelation [%s %s]", pMKParent->GetD3MetaEntity()->GetName().c_str(), strName.c_str());
			e.LogError();
			return NULL;
		}

		return pMR;
	}






	void D3MetaRelation::ExportAsSQLScript(std::ofstream& sqlout)
	{
		MetaColumnPtr															pMC;
		ColumnIndex																idx;
		bool																			bFirst;
		int																				i;
		std::string																strVals;


		sqlout << "SET @iParentMKID = " << GetParentD3MetaKey()->IDAsSoftlink() << ';' << std::endl;
		sqlout << "SET @iChildMKID = " << GetChildD3MetaKey()->IDAsSoftlink() << ';' << std::endl;

		if (GetSwitchD3MetaColumn())
			sqlout << "SET @iSwitchMCID = " << GetSwitchD3MetaColumn()->IDAsSoftlink() << ';' << std::endl;
		else
			if (!GetColumn(D3MetaRelation_SwitchColumnID)->IsNull())
				std::cout	<< "CRAP!\n";

		sqlout << "INSERT INTO D3MetaRelation (";

		for (bFirst = true, idx = 0; idx < GetMetaEntity()->GetMetaColumnCount(); idx++)
		{
			pMC = GetMetaEntity()->GetMetaColumn(idx);

			if (pMC->IsAutoNum())
				continue;

			if (bFirst)
				bFirst = false;
			else
				sqlout << ", ";	

			sqlout << pMC->GetName();
		}

		sqlout << ") VALUES (" << std::endl;
		sqlout << "\t\t/*" << std::setw(30) << (GetParentD3MetaEntity()->GetName() + '.' + GetName()) << " */ " << std::endl;

		for (bFirst = true, idx = 0; idx < GetMetaEntity()->GetMetaColumnCount(); idx++)
		{
			pMC = GetMetaEntity()->GetMetaColumn(idx);

			if (pMC->IsAutoNum())
				continue;

			if (bFirst)
				bFirst = false;
			else
				sqlout << ", " << std::endl;	

			sqlout << "\t\t/*" << std::setw(30) << pMC->GetName() << " */ ";

			if (pMC->GetName() == "Flags")
			{
				for (strVals="", i=0; i<sizeof(MetaRelation::Flags::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
				{
					if ((GetFlags() & (1u << i)) && D3::MetaRelation::Flags::BitMaskT::M_primitiveMasks[i])
					{
						if (!strVals.empty())	strVals += " | ";
						strVals += "@iMRF";
						strVals += D3::MetaRelation::Flags::BitMaskT::M_primitiveMasks[i];
					}
				}

				if (strVals.empty())
					sqlout << "(0)";
				else
					sqlout << '(' + strVals + ')';
			}
			else if (pMC->GetName() == "ParentKeyID")
				sqlout << "@iParentMKID";
			else if (pMC->GetName() == "ChildKeyID")
				sqlout << "@iChildMKID";
			else if (pMC->GetName() == "ParentEntityID")
				sqlout << "@iMEID";
			else if (pMC->GetName() == "SwitchColumnID")
				sqlout << (GetSwitchD3MetaColumn() ? "@iSwitchMCID" : "NULL");
			else
				sqlout << GetColumn(pMC)->AsSQLString();
		}

		sqlout << ");" << std::endl;
	}






	//===========================================================================
	//
	// CK_D3MetaRelation implementation
	//

	D3_CLASS_IMPL(CK_D3MetaRelation, InstanceKey);


	std::string CK_D3MetaRelation::AsString()
	{
		std::string				strKey;
		D3MetaRelationPtr	pD3MR = (D3MetaRelationPtr) GetEntity();

		strKey += pD3MR->GetParentD3MetaEntity()->GetConceptualKey()->AsString();
		strKey += "[";
		strKey += pD3MR->GetName();
		strKey += "]";

		return strKey;
	}


}
