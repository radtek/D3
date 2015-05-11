// MODULE: D3MetaEntity source

#include "D3Types.h"

#include "D3MetaEntity.h"

namespace D3
{
	D3_CLASS_IMPL(D3MetaEntity, D3MetaEntityBase);





	// Return a new instance populated with the details from the MetaEntity passed in
	D3MetaEntity::D3MetaEntity(DatabasePtr pDB, D3MetaDatabasePtr pD3MD, MetaEntityPtr pME, float fSequenceNo)
	{
		MarkConstructing();
		m_pMetaEntity = MetaDatabase::GetMetaDictionary()->GetMetaEntity(D3MDDB_D3MetaEntity);
		m_pDatabase = pDB;
		On_PostCreate();
		UnMarkConstructing();

		// Set the details from the meta type
		SetD3MetaDatabase(pD3MD);

		SetSequenceNo(fSequenceNo);
		SetName(pME->GetName());
		SetMetaClass(pME->GetMetaClassName());
		SetInstanceClass(pME->GetInstanceClassName());
		SetInstanceInterface(pME->GetInstanceInterface());
		SetJSMetaClass(pME->GetJSMetaClassName());
		SetJSInstanceClass(pME->GetJSInstanceClassName());
		SetJSViewClass(pME->GetJSViewClassName());
		SetJSDetailViewClass(pME->GetJSDetailViewClassName());
		SetProsaName(pME->GetProsaName());
		SetDescription(pME->GetDescription());
		SetAlias(pME->GetAlias());
		SetFlags(pME->GetFlags().RawValue());
		SetAccessRights(pME->GetPermissions().RawValue());
	}



	// Return the related column with the given name
	D3MetaColumnPtr D3MetaEntity::GetMetaColumn(const std::string & strColumnName)
	{
		D3MetaColumnPtr												pMC;
		D3MetaEntity::D3MetaColumns::iterator	itr;


		for ( itr =  GetD3MetaColumns()->begin();
					itr != GetD3MetaColumns()->end();
					itr++)
		{
			pMC = *itr;

			if (pMC->GetName() == strColumnName)
				return pMC;
		}

		throw Exception(__FILE__, __LINE__, Exception_error, "D3MetaEntity::GetMetaColumn(): D3MetaEntity %s has no column %s.", GetName().c_str(), strColumnName.c_str());

		return NULL;
	}




	// Return the related key with the given name
	D3MetaKeyPtr D3MetaEntity::GetMetaKey(const std::string & strKeyName)
	{
		D3MetaKeyPtr												pMC;
		D3MetaEntity::D3MetaKeys::iterator	itr;


		for ( itr =  GetD3MetaKeys()->begin();
					itr != GetD3MetaKeys()->end();
					itr++)
		{
			pMC = *itr;

			if (pMC->GetName() == strKeyName)
				return pMC;
		}

		throw Exception(__FILE__, __LINE__, Exception_error, "D3MetaEntity::GetMetaKey(): D3MetaEntity %s has no key %s.", GetName().c_str(), strKeyName.c_str());

		return NULL;
	}





	D3MetaColumnPtr D3MetaEntity::FindD3MetaColumn(const std::string & strName)
	{
		D3MetaColumns::iterator			itrD3MC;
		D3MetaColumnPtr							pD3MC;

		// Iterate over all resident related D3MetaColumns
		for ( itrD3MC =  this->GetD3MetaColumns()->begin();
					itrD3MC != this->GetD3MetaColumns()->end();
					itrD3MC++)
		{
			pD3MC = *itrD3MC;

			if (pD3MC->GetColumn(D3MDDB_D3MetaColumn_Name)->GetString() == strName)
				return pD3MC;
		}

		return NULL;
	}




	/* static */
	D3MetaEntityPtr D3MetaEntity::Create(	D3MetaDatabasePtr pMD,
															const std::string & strName,
															const std::string & strAlias,
															const std::string & strInstanceClass,
															const std::string & strInstanceInterface,
															const std::string & strProsaName,
															const std::string & strDescription,
															const MetaEntity::Flags & flags)
	{
		D3MetaEntityPtr			pME;


		try
		{
			pME = D3MetaEntityBase::CreateD3MetaEntity(pMD->GetDatabase());

			pME->SetSequenceNo				((short) pMD->GetD3MetaEntities()->size());
			pME->SetName							(strName);
			pME->SetAlias							(strAlias);
			pME->SetInstanceClass			(strInstanceClass);
			pME->SetInstanceInterface	(strInstanceInterface);
			pME->SetProsaName					(strProsaName);
			pME->SetDescription				(strDescription);
			pME->SetFlags							(flags.m_bitset.m_value);

			pME->SetD3MetaDatabase(pMD);
		}
		catch(Exception & e)
		{
			e.AddMessage("D3MetaEntity::Create(): Error creating D3MetaEntity instance %s", strName.c_str());
			e.LogError();
			return NULL;
		}

		return pME;
	}




	std::string D3MetaEntity::IDAsSoftlink()
	{
		std::ostringstream					ostrm;

		ostrm << "(SELECT ID FROM D3MetaEntity WHERE ";
		ostrm << "Name='" << GetName() << "'  AND ";
		ostrm << "MetaDatabaseID=@iMDID)";

		return ostrm.str();
	}




	void D3MetaEntity::ExportAsSQLScript(std::ofstream& sqlout)
	{
		MetaColumnPtr															pMC;
		ColumnIndex																idx;
		bool																			bFirst;
		D3MetaEntity::D3MetaColumns::iterator			itrColumns;
		D3MetaColumnPtr														pD3Column;
		D3MetaEntity::D3MetaKeys::iterator				itrKeys;
		D3MetaKeyPtr															pD3Key;
		int																				i;
		std::string																strVals;


		sqlout << std::endl;
		sqlout << "-- INSERT D3MetaEntity " << GetD3MetaDatabase()->GetAlias() << '.' << GetName() << std::endl;
		sqlout << "INSERT INTO D3MetaEntity (";

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
				for (strVals="", i=0; i<sizeof(MetaEntity::Flags::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
				{
					if ((GetFlags() & (1u << i)) && D3::MetaEntity::Flags::BitMaskT::M_primitiveMasks[i])
					{
						if (!strVals.empty())	strVals += " | ";
						strVals += "@iMEF";
						strVals += D3::MetaEntity::Flags::BitMaskT::M_primitiveMasks[i];
					}
				}

				if (strVals.empty())
					sqlout << "(0)";
				else
					sqlout << '(' + strVals + ')';
			}
			else if (pMC->GetName() == "AccessRights")
			{
				for (strVals="", i=0; i<sizeof(MetaEntity::Permissions::BitMaskT::M_primitiveMasks)/sizeof(const char*); i++)
				{
					if ((GetAccessRights() & (1u << i)) && D3::MetaEntity::Permissions::BitMaskT::M_primitiveMasks[i])
					{
						if (!strVals.empty())	strVals += " | ";
						strVals += "@iMEP";
						strVals += D3::MetaEntity::Permissions::BitMaskT::M_primitiveMasks[i];
					}
				}

				if (strVals.empty())
					sqlout << "(0)";
				else
					sqlout << '(' + strVals + ')';
			}
			else if (pMC->GetName() == "MetaDatabaseID")
				sqlout << "@iMDID";
			else
				sqlout << GetColumn(pMC)->AsSQLString();
		}

		sqlout << ");" << std::endl;

		sqlout << "SELECT @iMEID=ID FROM D3MetaEntity WHERE Name='" << GetName() << "' AND MetaDatabaseID=@iMDID;" << std::endl;

		for ( itrColumns =  GetD3MetaColumns()->begin();
					itrColumns != GetD3MetaColumns()->end();
					itrColumns++)
		{
			pD3Column = *itrColumns;
			pD3Column->ExportAsSQLScript(sqlout);
		}

		for ( itrKeys =  GetD3MetaKeys()->begin();
					itrKeys != GetD3MetaKeys()->end();
					itrKeys++)
		{
			pD3Key = *itrKeys;
			pD3Key->ExportAsSQLScript(sqlout);
		}
	}




	void D3MetaEntity::ExportRelationsAsSQLScript(std::ofstream& sqlout)
	{
		D3MetaEntity::ChildD3MetaRelations::iterator		itrChildRels;
		D3MetaRelationPtr																pD3ChildRel;

		sqlout << std::endl;
		sqlout << "-- INSERT child D3MetaRelation for " << GetD3MetaDatabase()->GetAlias() << '.' << GetName() << std::endl;
		sqlout << "SELECT @iMEID=ID FROM D3MetaEntity WHERE Name='" << GetName() << "' AND MetaDatabaseID=@iMDID;" << std::endl;

		for ( itrChildRels =  GetChildD3MetaRelations()->begin();
					itrChildRels != GetChildD3MetaRelations()->end();
					itrChildRels++)
		{
			pD3ChildRel = *itrChildRels;
			pD3ChildRel->ExportAsSQLScript(sqlout);
		}
	}










	//===========================================================================
	//
	// CK_D3MetaEntity implementation
	//

	D3_CLASS_IMPL(CK_D3MetaEntity, InstanceKey);


	std::string CK_D3MetaEntity::AsString()
	{
		std::string				strKey;
		D3MetaEntityPtr		pD3ME = (D3MetaEntityPtr) GetEntity();

		strKey += pD3ME->GetD3MetaDatabase()->GetConceptualKey()->AsString();
		strKey += "[";
		strKey += pD3ME->GetName();
		strKey += "]";

		return strKey;
	}

}
