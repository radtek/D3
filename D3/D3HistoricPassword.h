#ifndef INC_D3HistoricPassword_H
#define INC_D3HistoricPassword_H

#include "D3HistoricPasswordBase.h"

namespace D3
{
	class D3_API D3HistoricPassword : public D3HistoricPasswordBase
  {
    D3_CLASS_DECL(D3HistoricPassword);

    protected:
      D3HistoricPassword() {};

    public:
      ~D3HistoricPassword() {};

			//! Export this to JSON
			void														ExportToJSON(std::ostream& ostrm);

			//! Import this from JSON (returns NULL if nothing was imported)
			static D3HistoricPasswordPtr		ImportFromJSON(DatabasePtr pDB, Json::Value & jsnValue);

			//! Find D3HistoricPassword
			/*! This method looks up all resident D3HistoricPassword objects and returns the one with
					with a matching name or NULL if non can be found
			*/
			static D3HistoricPasswordPtr		FindD3HistoricPassword(DatabasePtr pDatabase, D3UserPtr pUser, Data& data);
  };
}

#endif /* INC_D3HistoricPassword_H */
