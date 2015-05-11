#ifndef INC_HSMetaEntityTopic_H
#define INC_HSMetaEntityTopic_H

#include "HSMetaEntityTopicBase.h"

namespace D3
{
	//! HSMetaEntityTopic objects represent instances of HSMetaEntityTopic records from the D3 Meta Dictionary database
	/*! This class is based on the generated #HSMetaEntityTopicBase class.
	*/
	class D3_API HSMetaEntityTopic : public HSMetaEntityTopicBase
	{
		D3_CLASS_DECL(HSMetaEntityTopic);

		protected:
			HSMetaEntityTopic() {}

		public:
			~HSMetaEntityTopic() {}
	};

} // end namespace D3

#endif /* INC_HSMetaEntityTopic_H */
