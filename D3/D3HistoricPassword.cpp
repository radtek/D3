// MODULE: HistoricPassword source

#include "D3Types.h"

#include "D3HistoricPassword.h"
#include "D3User.h"

namespace D3
{
	D3_CLASS_IMPL(D3HistoricPassword, D3HistoricPasswordBase);



	void D3HistoricPassword::ExportToJSON(std::ostream& ostrm)
	{
		ostrm << std::endl;
		ostrm << "\t\t\t{\n";
		ostrm << "\t\t\t\t\"UserName\":\"" << JSONEncode(GetD3User()->GetName()) << "\",\n";
		ostrm << "\t\t\t\t\"Password\":\"" << GetPassword().base64_encode()      << "\"\n";
		ostrm << "\t\t\t}";
	}



	/* static */
	D3HistoricPasswordPtr D3HistoricPassword::ImportFromJSON(DatabasePtr pDB, Json::Value & jsnValue)
	{
		D3UserPtr											pUser = D3User::FindD3User(pDB, jsnValue["UserName"].asString());
		D3HistoricPasswordPtr					pD3HistoricPassword = NULL;
		D3::Data											dataPWD;

		// No work without pUser
		if (pUser)
		{
			dataPWD.base64_decode(jsnValue["Password"].asString());

			pD3HistoricPassword = FindD3HistoricPassword(pDB, pUser, dataPWD);

			if (pD3HistoricPassword)
			{
				pD3HistoricPassword->UnMarkMarked();
			}
			else
			{
				pD3HistoricPassword = CreateD3HistoricPassword(pDB);
				pD3HistoricPassword->SetUserID(pUser->GetID());
				pD3HistoricPassword->SetPassword(dataPWD);
			}
		}

		return pD3HistoricPassword;
	}



	// Find resident D3HistoricPassword by name
	/* static */
	D3HistoricPasswordPtr D3HistoricPassword::FindD3HistoricPassword(DatabasePtr pDatabase, D3UserPtr pUser, Data& dataPWD)
	{
		// Won't do a thing unless we have a pUser
		if (pUser)
		{
			D3User::D3HistoricPasswords::iterator		itrHistoricPassword;
			D3HistoricPasswordPtr										pHistoricPassword;

			for ( itrHistoricPassword =  pUser->GetD3HistoricPasswords()->begin();
						itrHistoricPassword != pUser->GetD3HistoricPasswords()->end();
						itrHistoricPassword++)
			{
				pHistoricPassword = *itrHistoricPassword;

				if (pHistoricPassword->GetPassword() == dataPWD)
					return pHistoricPassword;
			}
		}

		return NULL;
	}





}
