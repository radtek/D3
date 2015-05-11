#ifndef INC_D3MetaColumnChoice_H
#define INC_D3MetaColumnChoice_H

#include "D3MetaColumnChoiceBase.h"

namespace D3
{
	//! D3MetaColumnChoice objects represent instances of D3MetaColumnChoice records from the D3 Meta Dictionary database
	/*! This class is based on the generated #D3MetaColumnChoiceBase class.
	*/
	class D3_API D3MetaColumnChoice : public D3MetaColumnChoiceBase
	{
		friend class D3::MetaDatabase;

		D3_CLASS_DECL(D3MetaColumnChoice);

		protected:
			D3MetaColumnChoice() {}
			D3MetaColumnChoice(DatabasePtr pDB, D3MetaColumnPtr pD3MC, const MetaColumn::ColumnChoice & choice);

		public:
			~D3MetaColumnChoice() {}

			//! This static method simplifies creating fully populated and linked D3MetaColumnChoice objects
			/*! This method allows you to create a new D3MetaColumnChoice object and in one call pass all it's
					attributes to the method.

					The newly create D3MetaColumnChoice object will be created in the same database workspace
					which owns the D3MetaColumn you pass in. The newly created D3MetaColumnChoice will become
					a choice of the D3MetaColumn object you pass in.

					The purpose of D3MetaColumnChoice objects is to persist D3::MetaColumnChoice objects. In other words,
					at some stage these objects will be converted into D3::MetaColumn objects.

					@param  D3MetaColumnPtr						The D3MetaColumn which can assume this choice value

					@param  strChoiceVal							The value that can be assigned to instances of the D3MetaColumn passed in

					@param  strChoiceDesc							The meaning of the value (something more meaningful to the user than the value)

					@return D3MetaColumnChoicePtr			The method returns the newly created D3MetaColumnChoice which is a member
																						of the passed in D3MetaColumn
			*/
			static D3MetaColumnChoicePtr Create(D3MetaColumnPtr pMC,
																					const std::string & strChoiceVal,	
																					const std::string & strChoiceDesc);

			//! Helper called by D3MetaColumn::ExportAsSQLScript
			void													ExportAsSQLScript(std::ofstream& sqlout);
	};
} // end namespace D3

#endif /* INC_D3MetaColumnChoice_H */
