#ifndef INC_HSResource_H
#define INC_HSResource_H

#include "HSResourceBase.h"

namespace D3
{
	//! HSResource objects represent instances of HSResource records from the D3 Meta Dictionary database
	/*! This class is based on the generated #HSResourceBase class.
	*/
	class D3_API HSResource : public HSResourceBase
	{
		D3_CLASS_DECL(HSResource);

		protected:
			HSResource() {}

		public:
			~HSResource() {}
	};

} // end namespace D3

#endif /* INC_HSResource_H */
