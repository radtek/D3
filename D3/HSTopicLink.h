#ifndef INC_HSTopicLink_H
#define INC_HSTopicLink_H

#include "HSTopicLinkBase.h"

namespace D3
{
	//! HSTopicLink objects represent instances of HSTopicLink records from the D3 Meta Dictionary database
	/*! This class is based on the generated #HSTopicLinkBase class.
	*/
	class D3_API HSTopicLink : public HSTopicLinkBase
	{
		D3_CLASS_DECL(HSTopicLink);

		protected:
			HSTopicLink() {}

		public:
			~HSTopicLink() {}
	};

} // end namespace D3

#endif /* INC_HSTopicLink_H */
