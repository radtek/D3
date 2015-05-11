#ifndef INC_D3Session_H
#define INC_D3Session_H

#include "D3SessionBase.h"

namespace D3
{
	class D3_API D3Session : public D3SessionBase
  {
    D3_CLASS_DECL(D3Session);

    protected:
      D3Session() {};

    public:
      ~D3Session() {};
  };
}

#endif /* INC_D3Session_H */
