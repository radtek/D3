#ifndef INC_HSTopic_H
#define INC_HSTopic_H

#include "HSTopicBase.h"

namespace D3
{
	enum TargetType
	{
		UNKNOWN		=					0,
		AGI				=         1,
		CONAGRA		=         2,
		IBM				=         4,
		IBMKRAFT	=         8,
		KRAFT			=        16,
		NESTLE		=        32,
		PGSAP			=        64,
		RTCIS			=       128,
		SAP				=       256,
		SCHENCK		=       512,
		SMUCKERS	=      1024,
		UNILEVER	=      2048,
		PEPSI			=      4096,
		DELMONTE	=      8192,
		DREYERS   =     16384,
		AUTOVLB		=     32768,
		PRIME		  =     65536,
		EWM		    =    131072
	};

	//! HSTopic objects represent instances of HSTopic records from the D3 Meta Dictionary database
	/*! This class is based on the generated #HSTopicBase class.
	*/
	class D3_API HSTopic : public HSTopicBase
	{
		D3_CLASS_DECL(HSTopic);

		protected:
			HSTopic() {}

		public:
			~HSTopic() {}

			static TargetType GetCurrentTargetMask()
			{
#if defined(APP3_AIG)
				return AIG;
#elif defined(APP3_AUTOVLB)
				return AUTOVLB;
#elif defined(APP3_CONAGRA)
				return CONAGRA;
#elif defined(APP3_IBM)
				return IBM;
#elif defined(APP3_IBMKRAFT)
				return IBMKRAFT;
#elif defined(APP3_KRAFT)
				return KRAFT;
#elif defined(APP3_EWM)
				return EWM;
#elif defined(APP3_NESTLE)
				return NESTLE;
#elif defined(APP3_PRIME)
				return PRIME;
#elif defined(APP3_PGSAP)
				return PGSAP;
#elif defined(APP3_RTCIS)
				return RTCIS;
#elif defined(APP3_SAP)
				return SAP;
#elif defined(APP3_SCHENCK)
				return SCHENCK;
#elif defined(APP3_SMUCKERS)
				return SMUCKERS;
#elif defined(APP3_UNILEVER)
				return UNILEVER;
#elif defined(APP3_PEPSI)
				return PEPSI;
#elif defined(APP3_DELMONTE)
				return DELMONTE;
#elif defined(APP3_DREYERS)
				return DREYERS;
#else
				return UNKNOWN;
#endif
			}
	};

} // end namespace D3

#endif /* INC_HSTopic_H */
