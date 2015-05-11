#ifndef INC_HSTopicAssociation_H
#define INC_HSTopicAssociation_H

#include "HSTopicAssociationBase.h"

namespace D3
{
	//! HSTopicAssociation objects represent instances of HSTopicAssociation records from the D3 Meta Dictionary database
	/*! This class is based on the generated #HSTopicAssociationBase class.
	*/
	class D3_API HSTopicAssociation : public HSTopicAssociationBase
	{
		D3_CLASS_DECL(HSTopicAssociation);

		protected:
			HSTopicAssociation() {}

		public:
			~HSTopicAssociation() {}
	};

} // end namespace D3

#endif /* INC_HSTopicAssociation_H */
