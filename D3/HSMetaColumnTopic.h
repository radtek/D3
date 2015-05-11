#ifndef INC_HSMetaColumnTopic_H
#define INC_HSMetaColumnTopic_H

#include "HSMetaColumnTopicBase.h"

namespace D3
{
	//! HSMetaColumnTopic objects represent instances of HSMetaColumnTopic records from the D3 Meta Dictionary database
	/*! This class is based on the generated #HSMetaColumnTopicBase class.
	*/
	class D3_API HSMetaColumnTopic : public HSMetaColumnTopicBase
	{
		D3_CLASS_DECL(HSMetaColumnTopic);

		protected:
			HSMetaColumnTopic() {}

		public:
			~HSMetaColumnTopic() {}
	};

} // end namespace D3

#endif /* INC_HSMetaColumnTopic_H */
