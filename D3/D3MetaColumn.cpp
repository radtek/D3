// MODULE: D3MetaColumn source

#include "D3Types.h"

#include "D3MetaColumn.h"

namespace D3
{
	D3_CLASS_IMPL(D3MetaColumn, D3MetaColumnBase);




	// Return a new instance populated with the details from the MetaColumn passed in
	D3MetaColumn::D3MetaColumn(DatabasePtr pDB, D3MetaEntityPtr pD3ME, MetaColumnPtr pMC, float fSequenceNo)
	{
		MarkConstructing();
		m_pMetaEntity = MetaDatabase::GetMetaDictionary()->GetMetaEntity(D3MDDB_D3MetaColumn);
		m_pDatabase = pDB;
		On_PostCreate();
		UnMarkConstructing();

		// Set the details from the meta type
		SetD3MetaEntity(pD3ME);

		SetSequenceNo(fSequenceNo);
		SetName(pMC->GetName());
		SetProsaName(pMC->GetProsaName());
		SetDescription(pMC->GetDescription());
		SetType((char) pMC->GetType());
		SetUnit((char) pMC->GetUnit());
		SetFloatPrecision(pMC->GetFloatPrecision());
		SetMaxLength(pMC->GetMaxLength());
		SetFlags(pMC->GetFlags().RawValue());
		SetAccessRights(pMC->GetPermissions().RawValue());
		SetMetaClass(pMC->IAm().Name());
		SetInstanceClass(pMC->GetInstanceClassName());
		SetInstanceInterface(pMC->GetInstanceInterface());
		SetJSMetaClass(pMC->GetJSMetaClassName());
		SetJSInstanceClass(pMC->GetJSInstanceClassName());
		SetJSViewClass(pMC->GetJSViewClassName());
		SetDefaultValue(pMC->GetDefaultValue());
	}



	/* static */
	D3MetaColumnPtr	D3MetaColumn::Create(	D3MetaEntityPtr pME,
																				const std::string & strName,	
																				const std::string & strInstanceClass,	
																				const std::string & strInstanceInterface,	
																				const std::string & strProsaName,	
																				const std::string & strDescription,
																				MetaColumn::Type eType,
																				int  iMaxLength,
																				const MetaColumn::Flags & flags,
																				const char*	szDefaultValue)
	{
		D3MetaColumnPtr			pMC;

		try
		{
			// Note: The cast of pME to a D3::Entity* is necessary because D3MetaEntityBase
			// also implements a GetDatabase() method which returns the D3::D3MetaDatabase
			// of which the D3::D3MetaEntity object is a member, whereas what we're after here
			// is the D3::Database object
			//
			pMC = D3MetaColumnBase::CreateD3MetaColumn(((EntityPtr) pME)->GetDatabase());

			pMC->SetSequenceNo				((short) pME->GetD3MetaColumns()->size());
			pMC->SetName							(strName);
			pMC->SetInstanceClass			(strInstanceClass);
			pMC->SetInstanceInterface	(strInstanceInterface);
			pMC->SetProsaName					(strProsaName);
			pMC->SetDescription				(strDescription);
			pMC->SetType							(eType);
			pMC->SetMaxLength					(iMaxLength);
			pMC->SetFlags							((long) flags.m_bitset.m_value);

			if (szDefaultValue)
				pMC->SetDefaultValue		(std::string(szDefaultValue));
			else
				pMC->GetColumn(D3MDDB_D3MetaColumn_DefaultValue)->SetNull();

			pMC->SetD3MetaEntity			(pME);
		}
		catch(Exception & e)
		{
			e.AddMessage("Error creating D3MetaColumn instance %s", strName.c_str());
			e.LogError();
			return NULL;
		}

		return pMC;
	}




	std::string D3MetaColumn::IDAsSoftlink()
	{
		std::ostringstream					ostrm;

		ostrm << "(SELECT ID FROM D3MetaColumn WHERE ";
		ostrm << "Name='" << GetName() << "'  AND ";
		ostrm << "MetaEntityID=" << GetD3MetaEntity()->IDAsSoftlink() << ")";

		return ostrm.str();
	}




	void D3MetaColumn::ExportAsSQLScript(std::ofstream& sqlout)
	{
		MetaColumnPtr																	pMC;
		ColumnIndex																		idx;
		bool																					bFirst;
		D3MetaColumn::D3MetaColumnChoices::iterator		itrColumnChoices;
		D3MetaColumnChoicePtr													pD3ColumnChoice;
		int																						i;
		std::string																		strVals;


		sqlout << "INSERT INTO D3MetaColumn (";

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
		sqlout << "\t\t/*" << std::setw(30) << (GetD3MetaEntity()->GetName() + '.' + GetName()) << " */ " << std::endl;

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
				for (strVals="", i=0; i<sizeof(MetaColumn::Flags::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
				{
					if ((GetFlags() & (1u << i)) && D3::MetaColumn::Flags::BitMaskT::M_primitiveMasks[i])
					{
						if (!strVals.empty())	strVals += " | ";
						strVals += "@iMCF";
						strVals += D3::MetaColumn::Flags::BitMaskT::M_primitiveMasks[i];
					}
				}

				if (strVals.empty())
					sqlout << "(0)";
				else
					sqlout << '(' + strVals + ')';
			}
			else if (pMC->GetName() == "AccessRights")
			{
				for (strVals="", i=0; i<sizeof(MetaColumn::Permissions::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
				{
					if ((GetAccessRights() & (1u << i)) && D3::MetaColumn::Permissions::BitMaskT::M_primitiveMasks[i])
					{
						if (!strVals.empty())	strVals += " | ";
						strVals += "@iMCP";
						strVals += D3::MetaColumn::Permissions::BitMaskT::M_primitiveMasks[i];
					}
				}

				if (strVals.empty())
					sqlout << "(0)";
				else
					sqlout << '(' + strVals + ')';
			}
			else if (pMC->GetName() == "MetaEntityID")
				sqlout << "@iMEID";
			else
				sqlout << GetColumn(pMC)->AsSQLString();
		}

		sqlout << ");" << std::endl;

		if (!GetD3MetaColumnChoices()->empty())
		{
			sqlout << "SELECT @iMCID=ID FROM D3MetaColumn WHERE Name='" << GetName() << "' AND MetaEntityID=@iMEID;" << std::endl;

			for ( itrColumnChoices =  GetD3MetaColumnChoices()->begin();
						itrColumnChoices != GetD3MetaColumnChoices()->end();
						itrColumnChoices++)
			{
				pD3ColumnChoice = *itrColumnChoices;
				pD3ColumnChoice->ExportAsSQLScript(sqlout);
			}
		}
	}









	//===========================================================================
	//
	// CK_D3MetaColumn implementation
	//

	D3_CLASS_IMPL(CK_D3MetaColumn, InstanceKey);


	std::string CK_D3MetaColumn::AsString()
	{
		std::string				strKey;
		D3MetaColumnPtr		pD3MC = (D3MetaColumnPtr) GetEntity();

		strKey += pD3MC->GetD3MetaEntity()->GetConceptualKey()->AsString();
		strKey += "[";
		strKey += pD3MC->GetName();
		strKey += "]";

		return strKey;
	}


}
