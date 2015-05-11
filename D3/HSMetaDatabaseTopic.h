#ifndef INC_HSMetaDatabaseTopic_H
#define INC_HSMetaDatabaseTopic_H

#include "HSMetaDatabaseTopicBase.h"

namespace D3
{
	//! HSMetaDatabaseTopic objects represent instances of HSMetaDatabaseTopic records from the D3 Meta Dictionary database
	/*! This class is based on the generated #HSMetaDatabaseTopicBase class.
	*/
	class D3_API HSMetaDatabaseTopic : public HSMetaDatabaseTopicBase
	{
		D3_CLASS_DECL(HSMetaDatabaseTopic);

		protected:
			HSMetaDatabaseTopic() {}

		public:
			~HSMetaDatabaseTopic() {}
	};

} // end namespace D3

#endif /* INC_HSMetaDatabaseTopic_H */
