#ifndef INC_HSMetaRelationTopic_H
#define INC_HSMetaRelationTopic_H

#include "HSMetaRelationTopicBase.h"

namespace D3
{
	//! HSMetaRelationTopic objects represent instances of HSMetaRelationTopic records from the D3 Meta Dictionary database
	/*! This class is based on the generated #HSMetaRelationTopicBase class.
	*/
	class D3_API HSMetaRelationTopic : public HSMetaRelationTopicBase
	{
		D3_CLASS_DECL(HSMetaRelationTopic);

		protected:
			HSMetaRelationTopic() {}

		public:
			~HSMetaRelationTopic() {}
	};

} // end namespace D3

#endif /* INC_HSMetaRelationTopic_H */
