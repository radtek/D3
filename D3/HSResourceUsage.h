#ifndef INC_HSResourceUsage_H
#define INC_HSResourceUsage_H

#include "HSResourceUsageBase.h"

namespace D3
{
	//! HSResourceUsage objects represent instances of HSResourceUsage records from the D3 Meta Dictionary database
	/*! This class is based on the generated #HSResourceUsageBase class.
	*/
	class D3_API HSResourceUsage : public HSResourceUsageBase
	{
		D3_CLASS_DECL(HSResourceUsage);

		protected:
			HSResourceUsage() {}

		public:
			~HSResourceUsage() {}
	};

} // end namespace D3

#endif /* INC_HSResourceUsage_H */
