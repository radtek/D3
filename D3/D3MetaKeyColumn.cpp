// MODULE: D3MetaKeyColumn source

#include "D3Types.h"

#include "D3MetaKeyColumn.h"

namespace D3
{
	D3_CLASS_IMPL(D3MetaKeyColumn, D3MetaKeyColumnBase);





	// Return a new populated instance
	D3MetaKeyColumn::D3MetaKeyColumn(DatabasePtr pDB, D3MetaKeyPtr pD3MK, D3MetaColumnPtr pD3MC, float fSequenceNo)
	{
		MarkConstructing();
		m_pMetaEntity = MetaDatabase::GetMetaDictionary()->GetMetaEntity(D3MDDB_D3MetaKeyColumn);
		m_pDatabase = pDB;
		On_PostCreate();
		UnMarkConstructing();

		// Set the details from the meta type
		SetD3MetaKey(pD3MK);
		SetD3MetaColumn(pD3MC);

		SetSequenceNo(fSequenceNo);
	}



	/* static */
	D3MetaKeyColumnPtr D3MetaKeyColumn::Create( D3MetaKeyPtr pMK,
																							const std::string & strColumnName)

	{
		D3MetaColumnPtr								pMC;
		D3MetaKeyColumnPtr						pMKC;

		pMC = pMK->GetD3MetaEntity()->GetMetaColumn(strColumnName);

		if (!pMC)
		{
			ReportError("D3MetaKeyColumn::Create(): Error creating D3MetaKeyColumn [%s][%s][%s]", pMK->GetD3MetaEntity()->GetName().c_str(), pMK->GetName().c_str(), strColumnName.c_str());
			return NULL;
		}

		pMKC = D3MetaKeyColumnBase::CreateD3MetaKeyColumn(pMK->GetDatabase());
		pMKC->SetSequenceNo((short) pMK->GetD3MetaKeyColumns()->size());

		pMKC->SetD3MetaKey(pMK);
		pMKC->SetD3MetaColumn(pMC);

		return pMKC;
	}






	void D3MetaKeyColumn::ExportAsSQLScript(std::ofstream& sqlout)
	{
		MetaColumnPtr																	pMC;
		ColumnIndex																		idx;
		bool																					bFirst;


		sqlout << "SELECT @iMCID=" << this->GetD3MetaColumn()->IDAsSoftlink() << ';' << std::endl;
		sqlout << "INSERT INTO D3MetaKeyColumn (";

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

			if (pMC->GetName() == "MetaKeyID")
				sqlout << "@iMKID";
			else if (pMC->GetName() == "MetaColumnID")
				sqlout << "@iMCID";
			else
				sqlout << GetColumn(pMC)->AsSQLString();
		}

		sqlout << ");" << std::endl << std::endl;
	}

}
