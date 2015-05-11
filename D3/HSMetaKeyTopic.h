#ifndef INC_HSMetaKeyTopic_H
#define INC_HSMetaKeyTopic_H

#include "HSMetaKeyTopicBase.h"

namespace D3
{
	//! HSMetaKeyTopic objects represent instances of HSMetaKeyTopic records from the D3 Meta Dictionary database
	/*! This class is based on the generated #HSMetaKeyTopicBase class.
	*/
	class D3_API HSMetaKeyTopic : public HSMetaKeyTopicBase
	{
		D3_CLASS_DECL(HSMetaKeyTopic);

		protected:
			HSMetaKeyTopic() {}

		public:
			~HSMetaKeyTopic() {}
	};

} // end namespace D3

#endif /* INC_HSMetaKeyTopic_H */
