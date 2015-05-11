#ifndef _D3BitMask_H_
#define _D3BitMask_H_

#include <string>
#include <assert.h>

namespace D3
{
	// Use these macros to ensure consistency
	#define BITS_DECL(containerclass, bitsetname) \
		typedef Bits<sz##containerclass##bitsetname> Ancestor; \
		typedef BitMask< Ancestor >	BitMaskT; \
		bitsetname() {} \
		bitsetname(const Ancestor::Mask& m) : Ancestor(m) {}

	#define BITS_IMPL(containerclass, bitsetname) \
		template <> const char * BitMask< Bits<sz##containerclass##bitsetname> >::M_primitiveMasks[32] = { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 };

	#define PRIMITIVEMASK_IMPL(containerclass, bitsetname, bitmaskname, maskvalue) \
		const containerclass::bitsetname::Mask containerclass::bitsetname::bitmaskname(#bitmaskname, maskvalue)

	#define COMBOMASK_IMPL(containerclass, bitsetname, bitmaskname, maskvalue) \
		const containerclass::bitsetname::Mask containerclass::bitsetname::bitmaskname(maskvalue)


	//! The purpose of the Mask template class is to define masks which only apply to one specific type of Bits
	/*! Mask objects are essentially unsigned longs who's value represents 32 individual bits as turnd on/off.

			All masks belonging to a Bits class must be defined inside the class like:

			\code
			class Container
			{
				//...

				public:
					class XYZ : public Bits<...>
					{
						//...

						// Primitive Masks
						static const Mask	A;			// 0x0001
						static const Mask	B;			// 0x0002

						// Combo Masks
						static const Mask	C;			// A | B
						static const Mask	CLEAR;	// Nothing is set

						//...
					};

					//...

			};
			\endcode

			While the above declares static constant XYZ::Mask objects belonging to XYS, you'll also need to implement
			these. For convenience, you should use the macros to implement these Masks in the corresponding implementation
			file.

			A primitive mask should only have a single bit set. The macro will assert at runtime if you create a
			primitive mask that has more than one of its bits turned on. For the above example, we'd write:

			  PRIMITIVEMASK_IMPL(Container, XYZ, A, 0x0001);
			  PRIMITIVEMASK_IMPL(Container, XYZ, B, 0x0002);

			Note that if the mask value you pass in does not specify a single bit, the code will cause a
			run-time assert. Also, if you specify the same value a second time, this will also lead to a
			runtime assertion.

			A combo mask can be anything from having no bits set at all to having all bits set.  For the above example,
			we'd write:

			  COMBOMASK_IMPL(Container, XYZ, Combo, Container::XYZ::A | Container::XYZ::B);
			  COMBOMASK_IMPL(Container, XYZ, CLEAR, 0x0000);

			Masks support the typical bitwise operators ~, &, |. The class also overloads the
			operators bitwise assignment operators |= and &=.

			Automatic conversion from an unsigned or signed short, int, long, etc is not supported,
			though you can create a mask passing such a type as the sole argument to the ctor().

			This module also provides support to convert masks to stl string objects. The
			way this works is that when you use the PRIMITIVEMASK_IMPL macro to implement a
			primitive mask, the constructor remembers the name and the bit. When a mask is converted
			to a string object, it simply checks each known, primitive bit if it is set and if
			it is, prints the bits name enclosed in square brackets, e.g.: "" "[A]", "[B]" or "[A][B]".
			The order is not relevant, regardless of the sequence of static	implementaions.
	*/
	template <class TBits>
	struct BitMask
	{
		static const char*		M_primitiveMasks[32];		//!< This static array contains the name of known primitive bits oir NULL if not known
		unsigned long					m_value;								//!< The value of this mask

		//!< The basic constructor (you should use macros PRIMITIVEMASK_IMPL or COMBOMASK_IMPL instead)
		explicit BitMask(const unsigned long l = 0) : m_value(l) {}
		//!< Copy constructor
		BitMask(const BitMask& m) : m_value(m.m_value) {}
		//!< Constructor used by PRIMITIVEMASK_IMPL (use the PRIMITIVEMASK_IMPL instead of using this constructor directly)
		BitMask(const char* pszName, const unsigned long m_value) : m_value(m_value)
		{
			unsigned long i;

			// only allow single bits in mask
			for (i=0; i<sizeof(M_primitiveMasks)/sizeof(const char*); i++)
			{
				if (m_value == 1u << i)
					break;
			}

			assert(i < sizeof(M_primitiveMasks)/sizeof(const char*)); // must be a single bit mask!
			assert(!M_primitiveMasks[i]);
			M_primitiveMasks[i] = pszName;
		}

		//! Equality operator returns true if this has the same bits set as the mask passed in
		int operator==(const BitMask& m) const			{ return m_value == m.m_value; }

		//! The sole purpose of the operator const BitMask<TBits>* is to allow boolean tests
		/*! So you ask why not use operator bool()?
				The short answer is that the bool operator would make this comparison valid:

					BitMask<ABC>() == BitMask<XYZ>()

				instead of causing a compile error.

				The trick is that pointers can be converted implicitly to a bool (NULL vs.
				NOT NULL), but it is NOT a bool itself. C++ only allows one implicit conversion,
				so when it comes to doing a calculation that compares two different types (e.g.
				BitMask<ABC>() == BitMask<XYZ>()), they both return pointers that it implictly converts
				to bool. However it cannot execute the int operator==(bool, bool) because C++ only
				allows one implicit conversion and this is two implicit conversions (BitMask<XYZ> to *
				and * to bool).

				The same rule means that if you code:

				if (BitMask<XYZ>())

				this operator will be invoked and it's result will be tested against NULL or NOT NULL
				to determine the the boolean tests result.
		*/
		operator const BitMask*() const						{ return ((m_value > 0)  ? this : NULL); }

		//! Complement operator returns a new mask that is the complement of this' mask
		BitMask	operator~	() const								{ BitMask lhs(*this); lhs.m_value = ~lhs.m_value; return lhs; }

		//! Bitwise OR operator that returns a new mask with is the OR result of this masks value and the passed in masks value
		BitMask	operator|(const BitMask& rhs) const	{ BitMask lhs(*this); lhs |= rhs; return lhs; }
		//! Bitwise AND operator that returns a new mask with is the AND result of this masks value and the passed in masks value
		BitMask	operator&(const BitMask& rhs) const	{ BitMask lhs(*this); lhs &= rhs; return lhs; }

		//! Assignement operator copys the value from another mask
		BitMask& operator=	(const BitMask& rhs)				{ m_value = rhs.m_value; return *this; }

		//! Bitwise OR assignemnt operator that sets its own value to the reslt of bitwise OR'ing its own value with the passed in masks value
		BitMask&	operator|=(const BitMask& rhs)				{ m_value |= rhs.m_value; return *this; }
		//! Bitwise AND assignemnt operator that sets its own value to the reslt of bitwise AND'ing its own value with the passed in masks value
		BitMask&	operator&=(const BitMask& rhs)				{ m_value &= rhs.m_value; return *this; }

		//! Returns a string that includes all the names (enclosed in square brackets) of all the primitive	bits the current mask has set
		std::string AsString() const
		{
			unsigned long i;
			std::string strRslt;

			for (i=0; i<sizeof(M_primitiveMasks)/sizeof(const char*); i++)
			{
				if (M_primitiveMasks[i])
				{
					if (m_value & (1 << i))
					{
						strRslt += "[";
						strRslt += M_primitiveMasks[i];
						strRslt += "]";
					}
				}
			}

			return strRslt;
		}

	};



	//! The Bits class provides standard support for bitmasks consisting of upto 32 individual flags
	/*! The main drive behind this class is to ensure type safety with minimal impact on efficiency.

			The class has been designed with the expectation that such instances are typically assigned
			as instance members of other classes (the container class). Each bitmask based on this class
			has its own unique type such that comparisons, assignements, bitwise operations, etc. are only
			legal between objects of the same type/ The class also allows you to define multiple bitmaks
			of different type for the same container.

			Usage:

			\code
			#include "D3BitMask.h"

			// Create a types that can be used to construct each bitset as unique class (no implementation needed)
			extern const char szColumnPermissions[];
			extern const char szColumnFeatures[];

			namespace D3
			{
				class Column //...
				{
					//...
					public:
						class Permissions : public Bits<szColumnPermissions>
						{
							BITS_DECL(Column, Permissions);

							static const Mask		Readable;
							static const Mask		Writable;

							//...

						};

						class Features : public Bits<szColumnFeatures>
						{
							BITS_DECL(Column, Features);

							static const Mask		Mandatory;
							static const Mask		Autonum;
							static const Mask		Binary;

							//...

						};

					protected:
						Permissions			m_Permissions;
						Features				m_Features;

						//...

					};

				//...

			}
			/endcode

			In your implementation file, you do:

			/code
			namespace D3
			{
				BITS_IMPL(MetaColumn, Permissions);

				// Primitive mask values
				PRIMITIVEMASK_IMPL(MetaColumn, Permissions, Readable,		0x00000001);
				PRIMITIVEMASK_IMPL(MetaColumn, Permissions, Writable,		0x00000002);

				BITS_IMPL(MetaColumn, Features);

				// Primitive mask values
				PRIMITIVEMASK_IMPL(MetaColumn, Features, Mandatory,			0x00000001);
				PRIMITIVEMASK_IMPL(MetaColumn, Features, Autonum,				0x00000002);
				PRIMITIVEMASK_IMPL(MetaColumn, Features, Binary,				0x00000004);

				//...
			}
			/endcode

			(Also refor to D3::Mask for more details on specifying masks).

			Here are some examples of how these can/can't be used:

			/code
			void Column::foo()
			{
				if (m_Permissions & Permissions::Readable);												// OK
				m_Permissions &= ~Permissions::Readable;													// OK
				m_Permissions = Permissions::Readable | Permissions::Writable;		// OK
				m_Permissions = m_Features;																				// Compile error
				if (m_Features & Permissions::Readable);													// Compile error
				m_Features = Permissions::Readable;																// Compile error
			}
			/endcode

			As you can see, only operations between Bits<T> and Mask< Bits<T> > are legal.

			However, there are two ways to cheat:

			# you can create a Mask< Bits<T> > instance passing an unsigned long as the only argument to the constructor
			# you can access the m_value member of a Mask< Bits<T> > or Bits<T>::m_bitset directly

			This means that the following (somewhat nonsensical) code is valied:

			/code
			m_Features = Features::Mask(Permission::Readable.m_value);
			/endcode
	*/
	template <const char []>
	struct Bits
	{
		typedef BitMask<Bits>						Mask;				//!< Allows you to use typename Bits<T>::Mask nominate the Mask type compatible with this class

		Mask														m_bitset;		//!< The mask that specifies the currently set bits

		//!< The basic constructor creats a bitset with all bits turned off
		Bits() {}

		//!< The constructor creats a bitset with the same bits set as the passed in mask
		Bits(const Mask& m) : m_bitset(m) {}

		//! Equality operator returns true if this has the same bits set as the mask passed in
		int operator==(const Bits& m) const		{ return m_bitset == m.m_bitset; }

		//! The sole purpose of the operator const Mask<TBits>* is to allow boolean tests
		/*! So you ask why not use operator bool()?
				The short answer is that the bool operator would make this comparison valid:

					Mask<ABC>() == Mask<XYZ>()

				instead of causing a compile error.

				The trick is that pointers can be converted implicitly to a bool (NULL vs.
				NOT NULL), but it is NOT a bool itself. C++ only allows one implicit conversion,
				so when it comes to doing a calculation that compares two different types (e.g.
				Mask<ABC>() == Mask<XYZ>()), they both return pointers that it implictly converts
				to bool. However it cannot execute the int operator==(bool, bool) because C++ only
				allows one implicit conversion and this is two implicit conversions (Mask<XYZ> to *
				and * to bool).

				The same rule means that if you code:

				if (Mask<XYZ>())

				this operator will be invoked and it's result will be tested against NULL or NOT NULL
				to determine the the boolean tests result.
		*/
		operator const Bits*() const					{ return m_bitset.m_value ? this : NULL; }

		//! Complement operator returns a new mask that is the complement of this' mask
		Mask	operator~	() const							{ return ~m_bitset; }

		//! Bitwise OR operator that returns a new mask with is the OR result of this bitset value and the passed in masks value
		Mask	operator|(const Mask& m) const	{ return m_bitset | m; }
		//! Bitwise AND operator that returns a new mask with is the AND result of this bitset value and the passed in masks value
		Mask	operator&(const Mask& m) const	{ return m_bitset & m; }

		//! Assignement operator copys the value from another bitset
		Bits& operator=	(const Mask& m)				{ m_bitset = m;	return *this; }

		//! Bitwise OR assignemnt operator that sets its own value to the reslt of bitwise OR'ing its own bitset value with the passed in masks value
		Bits& operator|=(const Mask& m)				{ m_bitset |= m; return *this; }
		//! Bitwise AND assignemnt operator that sets its own value to the reslt of bitwise AND'ing its own bitset value with the passed in masks value
		Bits& operator&=(const Mask& m)				{ m_bitset &= m; return *this; }

		//! Allows using this as though it was a Mask of a compatible type
		operator Mask() const									{ return Mask(m_bitset); }

		//! Allows using this as though it was a Mask of a compatible type
		unsigned long RawValue() const				{ return m_bitset.m_value; }

		//! Returns a string that includes all the names (enclosed in square brackets) of all the primitive	bits thye current mask has set
		std::string AsString() const					{ return m_bitset.AsString(); }
	};

} // end namespace D3



#endif /* _D3BitMask_H_ */
