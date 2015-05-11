// MODULE: D3MetaColumnChoice source

#include "D3Types.h"

#include "D3MetaColumnChoice.h"

namespace D3
{
	D3_CLASS_IMPL(D3MetaColumnChoice, D3MetaColumnChoiceBase);



	// Return a new instance populated with the details from the MetaColumn passed in
	D3MetaColumnChoice::D3MetaColumnChoice(DatabasePtr pDB, D3MetaColumnPtr pD3MC, const MetaColumn::ColumnChoice & choice)
	{
		MarkConstructing();
		m_pMetaEntity = MetaDatabase::GetMetaDictionary()->GetMetaEntity(D3MDDB_D3MetaColumnChoice);
		m_pDatabase = pDB;
		On_PostCreate();
		UnMarkConstructing();

		// Set the details from the meta type
		SetD3MetaColumn(pD3MC);

		SetSequenceNo(choice.SeqNo);
		SetChoiceVal(choice.Val);
		SetChoiceDesc(choice.Desc);
	}



	/* static */
	D3MetaColumnChoicePtr D3MetaColumnChoice::Create(	D3MetaColumnPtr pMC,
																										const std::string & strChoiceVal,	
																										const std::string & strChoiceDesc)
	{
		D3MetaColumnChoicePtr		pMCC;

		try
		{
			pMCC = D3MetaColumnChoice::CreateD3MetaColumnChoice(pMC->GetDatabase());

			pMCC->SetChoiceVal	(strChoiceVal);
			pMCC->SetChoiceDesc	(strChoiceDesc);

			pMCC->SetD3MetaColumn(pMC);
		}
		catch(Exception & e)
		{
			e.AddMessage("D3MetaColumnChoice::Create(): Error creating D3MetaColumnChoice for column [%s][%s]", pMC->GetD3MetaEntity()->GetName().c_str(), pMC->GetName().c_str());
			e.LogError();
			return NULL;
		}

		return pMCC;
	}





	void D3MetaColumnChoice::ExportAsSQLScript(std::ofstream& sqlout)
	{
		MetaColumnPtr																	pMC;
		ColumnIndex																		idx;
		bool																					bFirst;


		sqlout << "INSERT INTO D3MetaColumnChoice (";

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

			if (pMC->GetName() == "MetaColumnID")
				sqlout << "@iMCID";
			else
				sqlout << GetColumn(pMC)->AsSQLString();
		}

		sqlout << ");" << std::endl;
	}

}