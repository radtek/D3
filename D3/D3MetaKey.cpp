// MODULE: D3MetaKey source

#include "D3Types.h"

#include "D3MetaKey.h"

namespace D3
{
	D3_CLASS_IMPL(D3MetaKey, D3MetaKeyBase);




	// Return a new instance populated with the details from the MetaKey passed in
	D3MetaKey::D3MetaKey(DatabasePtr pDB, D3MetaEntityPtr pD3ME, MetaKeyPtr pMK, float fSequenceNo)
	{
		MarkConstructing();
		m_pMetaEntity = MetaDatabase::GetMetaDictionary()->GetMetaEntity(D3MDDB_D3MetaKey);
		m_pDatabase = pDB;
		On_PostCreate();
		UnMarkConstructing();

		// Set the details from the meta type
		SetD3MetaEntity(pD3ME);

		SetSequenceNo(fSequenceNo);
		SetName(pMK->GetName());
		SetProsaName(pMK->GetProsaName());
		SetDescription(pMK->GetDescription());
		SetMetaClass(pMK->IAm().Name());
		SetInstanceClass(pMK->GetInstanceClassName());
		SetJSMetaClass(pMK->GetJSMetaClassName());
		SetJSViewClass(pMK->GetJSViewClassName());
		SetInstanceInterface(pMK->GetInstanceInterface());
		SetFlags(pMK->GetFlags().RawValue());
	}



	/* static */
	D3MetaKeyPtr D3MetaKey::Create(	D3MetaEntityPtr pME,
																	const std::string & strName,
																	const std::string & strInstanceClass,
																	const std::string & strInstanceInterface,
																	const MetaKey::Flags & flags)
	{
		D3MetaKeyPtr			pMK;


		try
		{
			// Note: The cast of pME to a D3::Entity* is necessary because D3MetaEntityBase
			// also implements a GetDatabase() method which returns the D3::D3MetaDatabase
			// of which the D3::D3MetaEntity object is a member, whereas what we're after here
			// is the D3::Database object
			//
			pMK = D3MetaKeyBase::CreateD3MetaKey(((EntityPtr) pME)->GetDatabase());

			pMK->SetSequenceNo					((short) pME->GetD3MetaKeys()->size());
			pMK->SetName								(strName);
			pMK->SetInstanceClass				(strInstanceClass);
			pMK->SetInstanceInterface		(strInstanceInterface);
			pMK->SetFlags								(flags.m_bitset.m_value);

			pMK->SetD3MetaEntity				(pME);
		}
		catch(Exception & e)
		{
			e.AddMessage("D3MetaKey::Create(): Error creating D3MetaKey instance %s", strName.c_str());
			e.LogError();
			return NULL;
		}

		return pMK;
	}




	std::string D3MetaKey::IDAsSoftlink()
	{
		std::ostringstream					ostrm;

		ostrm << "(SELECT ID FROM D3MetaKey WHERE ";
		ostrm << "Name='" << GetName() << "'  AND ";
		ostrm << "MetaEntityID=" << GetD3MetaEntity()->IDAsSoftlink() << ')';

		return ostrm.str();
	}




	void D3MetaKey::ExportAsSQLScript(std::ofstream& sqlout)
	{
		MetaColumnPtr															pMC;
		ColumnIndex																idx;
		bool																			bFirst;
		D3MetaKey::D3MetaKeyColumns::iterator			itrKeyColumns;
		D3MetaKeyColumnPtr												pD3KeyColumn;
		int																				i;
		std::string																strVals;


		sqlout << "INSERT INTO D3MetaKey (";

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
				for (strVals="", i=0; i<sizeof(MetaKey::Flags::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
				{
					if ((GetFlags() & (1u << i)) && D3::MetaKey::Flags::BitMaskT::M_primitiveMasks[i])
					{
						if (!strVals.empty())	strVals += " | ";
						strVals += "@iMKF";
						strVals += D3::MetaKey::Flags::BitMaskT::M_primitiveMasks[i];
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

		sqlout << "SELECT @iMKID=ID FROM D3MetaKey WHERE Name='" << GetName() << "' AND MetaEntityID=@iMEID;" << std::endl;

		for ( itrKeyColumns =  GetD3MetaKeyColumns()->begin();
					itrKeyColumns != GetD3MetaKeyColumns()->end();
					itrKeyColumns++)
		{
			pD3KeyColumn = *itrKeyColumns;
			pD3KeyColumn->ExportAsSQLScript(sqlout);
		}
	}









	//===========================================================================
	//
	// CK_D3MetaKey implementation
	//

	D3_CLASS_IMPL(CK_D3MetaKey, InstanceKey);


	std::string CK_D3MetaKey::AsString()
	{
		std::string				strKey;
		D3MetaKeyPtr			pD3MK = (D3MetaKeyPtr) GetEntity();

		strKey += pD3MK->GetD3MetaEntity()->GetConceptualKey()->AsString();
		strKey += "[";
		strKey += pD3MK->GetName();
		strKey += "]";

		return strKey;
	}


}
