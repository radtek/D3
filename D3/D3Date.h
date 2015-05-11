#ifndef _D3Date_H_
#define _D3Date_H_

// ===========================================================
// File Info:
//
// $Author: pete $
// $Date: 2014/12/03 23:25:32 $
// $Revision: 1.18 $
//
// ===========================================================
// Change History:
// ===========================================================
//
// 13/03/03 - R1 - Hugp
//
// Created header declaring D3Date class
// -----------------------------------------------------------

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/date_time/local_time/local_time.hpp>
#include <boost/regex.hpp>

#ifdef APAL_SUPPORT_ODBC
	#include <odbc++/types.h>
#endif

#ifdef APAL_SUPPORT_OTL
	#include "OTLParams.h"
#endif

//! A D3TimeZone is a boost smart pointer holding a pointer to a time zone
/*! The purpose of this class is to provided additional mechanisms required
		to conver native data store dates to a date type common to all other
		components of D3.
*/
class D3TimeZone : public boost::local_time::time_zone_ptr
{
	protected:
		static D3TimeZone							M_explicitServerTimeZone;
		static D3TimeZone							M_defaultServerTimeZone;
		static D3TimeZone							M_utcTimeZone;

	public:
		//! Essentially, same as UTC
		D3_API	D3TimeZone() {}
		//! Creates a time boost posix time zone from the strTZInfo string.
		/*! Please refer to the boost documentation for syntactical details of strTZInfo (Posix time zone string) here:

				http://www.boost.org/doc/libs/1_42_0/doc/html/date_time/local_time.html#date_time.local_time.posix_time_zone

				A POSIX time zone string takes the form of:

							"std offset dst [offset],start[/time],end[/time]" (w/no spaces).

				'std' specifies the abbrev of the time zone. 'offset' is the offset from UTC. 'dst' specifies the abbrev of the time zone during daylight savings time. The second offset is how many hours changed during DST. 'start' and 'end' are the dates when DST goes into (and out of) effect. 'offset' takes the form of:

							[+|-]hh[:mm[:ss]] {h=0-23, m/s=0-59}

				'time' and 'offset' take the same form. 'start' and 'end' can be one of three forms:

							Mm.w.d {month=1-12, week=1-5 (5 is always last), day=0-6}
							Jn {n=1-365 Feb29 is never counted}
							n {n=0-365 Feb29 is counted in leap years}

				Exceptions will be thrown under the following conditions:

				An exception will be thrown for an invalid date spec (see date class).
				A boost::local_time::bad_offset exception will be thrown for:
				A DST start or end offset that is negative or more than 24 hours.
				A UTC zone that is greater than +14 or less than -12 hours.
				A boost::local_time::bad_adjustment exception will be thrown for a DST adjustment that is 24 hours or more (positive or negative)
				As stated above, the 'offset' and '/time' portions of the string are not required. If they are not given they default to 01:00 for 'offset', and 02:00 for both occurrences of '/time'.

				Some examples are:

							"PST-8PDT01:00:00,M4.1.0/02:00:00,M10.1.0/02:00:00"
							"PST-8PDT,M4.1.0,M10.1.0"

				These two are actually the same specification (defaults were used in the second string). This zone lies eight hours west of GMT and makes a one hour shift forward during daylight savings time. Daylight savings for this zone starts on the first Sunday of April at 2am, and ends on the first Sunday of October at 2am.

							"MST-7"

				This zone is as simple as it gets. This zone lies seven hours west of GMT and has no daylight savings.

							"EST10EDT,M10.5.0,M3.5.0/03"

				This string describes the time zone for Sydney Australia. It lies ten hours east of GMT and makes a one hour shift forward during daylight savings. Being located in the southern hemisphere, daylight savings begins on the last Sunday in October at 2am and ends on the last Sunday in March at 3am.

							"FST+3FDT02:00,J60/00,J304/02"

				This specification describes a fictitious zone that lies three hours east of GMT. It makes a two hour shift forward for daylight savings which begins on March 1st at midnight, and ends on October 31st at 2am. The 'J' designation in the start/end specs signifies that counting starts at one and February 29th is never counted.

							"FST+3FDT,59,304"

				This specification is significant because of the '59'. The lack of 'J' for the start and end dates, indicates that the Julian day-count begins at zero and ends at 365. If you do the math, you'll see that allows for a total of 366 days. This is fine in leap years, but in non-leap years '59' (Feb-29) does not exist. This will construct a valid posix_time_zone object but an exception will be thrown if the date of '59' is accessed in a non-leap year. Ex:

							"UTC"
		*/
		D3_API	D3TimeZone(const std::string& strTZInfo) : boost::local_time::time_zone_ptr(new boost::local_time::posix_time_zone(strTZInfo)) {}
		D3_API	D3TimeZone(boost::local_time::time_zone_ptr p) : boost::local_time::time_zone_ptr(p) {}
		D3_API	virtual ~D3TimeZone() {}

		//! Dump the details for this timezone to std::cout
		/*! If indent is specified, each line will have indent number of leading blanks
		*/
		D3_API	void																Dump(int indent = 0);

		//! Returns the UTC time zone
		D3_API	static D3TimeZone										GetUTCTimeZone()		{ return M_utcTimeZone; }

		//! Returns a default time zone based on the systems tz configuration
		/*! Note that the time zone created doesn't cater for Daylight Savings adjustments
				and the name given is simply UKN (Unknown).
		*/
		D3_API	static D3TimeZone										GetDefaultServerTimeZone();

		//! Returns the local servers timezone
		/*! If M_explicitServerTimeZone does not specify a TimeZone, the system will create
				a default time zone called UKN and set its time offset from UTC according to the
				local systems time zone configuration. This value will be cached and returned
				on subsequent calls.
		*/
		D3_API	static D3TimeZone										GetLocalServerTimeZone()
		{
			if (!M_explicitServerTimeZone)
				M_explicitServerTimeZone = GetDefaultServerTimeZone();

			return M_explicitServerTimeZone;
		}

		//! Returns the local servers timezone
		D3_API	static void													SetLocalServerTimeZone(D3TimeZone tz)		{ M_explicitServerTimeZone = tz; }
};






//! The Date class is a class based on boost::local_time::local_date_time
/*! The purpose of this class is to provided additional mechanisms required
		to convert native data store dates to a date type common to all other
		components of D3.
*/
class D3_API D3Date : public boost::local_time::local_date_time
{
	public:
		static const char*	MaxISO8601DateTime;		//!< Contains "20371231T23:59:59Z" which can be passed in a D3Date ctor like D3Date(D3Date::MaxISO8601DateTime) to create a date tim value that works reliably in C++, ruby and javascript

		//! The passed time information understood to be in the passed tz. The DST flag is calculated according to the specified rule. May throw a ambiguous_result or time_label_invalid exception.
		D3Date(	boost::gregorian::date d,
						time_duration_type t,
						D3TimeZone tz = D3TimeZone::GetLocalServerTimeZone())
			:	boost::local_time::local_date_time(d, t, tz, NOT_DATE_TIME_ON_ERROR)
		{}

		//! Uses d as a local date and assume a local time of 00:00:00.000 (mid night). May throw a ambiguous_result or time_label_invalid exception.
		explicit
		D3Date(	boost::gregorian::date d,
						D3TimeZone tz = D3TimeZone::GetLocalServerTimeZone())
			:	boost::local_time::local_date_time(d, boost::posix_time::time_duration(0,0,0,0), tz, NOT_DATE_TIME_ON_ERROR)
		{}

		D3Date(	const boost::posix_time::ptime& utcTime,
						D3TimeZone tz = D3TimeZone::GetLocalServerTimeZone())
			: boost::local_time::local_date_time(utcTime,	tz)
		{}

		D3Date(const boost::local_time::local_date_time& tm)		: boost::local_time::local_date_time(tm) {}
		D3Date(const D3Date& dt)																: boost::local_time::local_date_time(dt) {}
		D3Date(const D3Date& dt, D3TimeZone tz);

		//! The default constructor will set this date to Now()
		D3Date(D3TimeZone tz = D3TimeZone::GetLocalServerTimeZone());

		//! Converts a date string to a D3Date (see operator=(const std::string&) for details)
		/*! Please refer to operator=() for a detailed explanations on how the string is interpreted.
		*/
		explicit
		D3Date(const std::string & strDateTime, D3TimeZone tz = D3TimeZone::GetLocalServerTimeZone());

		//! Return a D3Date representing the current system date and time
		static D3Date		Now(D3TimeZone tz = D3TimeZone::GetLocalServerTimeZone());

		//! Convert the string passed in to a date/time value and assign it to this
		/*! This method understands ISO8601 date strings in addition to some common variations
				of textual date representation.

				Technically, the following input complies with ISO8601. Essentially, these strings
				are what we used to call APAL dates or date-time strings. According to the ISO8601
				standard these strings are actually considered to contain UTC dates. In order to
				maintain backwards compatibility with earlier versions of APAL, I have decided to
				deviate from the standard and interpret such strings as containing local dates or
				date times:

				Special Handling of APAL date/time                            UTC Date Time                          NZDT (+13)
				----------------------------------       ----------------------------------  ----------------------------------
																	20071203   ==>               2007-Dec-02 11:00:00                2007-Dec-03 00:00:00
														20071203111213   ==>               2007-Dec-02 22:12:13                2007-Dec-03 11:12:13

				Here are some typical examples of acceptable ISO8601 compliant input:

																	 ISO8601                            UTC Date Time                          NZDT (+13)
				----------------------------------       ----------------------------------  ----------------------------------
																 2007-12-3   ==>               2007-Dec-03 00:00:00                2007-Dec-03 13:00:00
													 20071203T111213   ==>               2007-Dec-03 11:12:13                2007-Dec-04 00:12:13
													20071203T111213Z   ==>               2007-Dec-03 11:12:13                2007-Dec-04 00:12:13
											2007-12-03T11:12:13Z   ==>               2007-Dec-03 11:12:13                2007-Dec-04 00:12:13
								 2007-12-03T11:12:13-05:30   ==>               2007-Dec-03 16:42:13                2007-Dec-04 05:42:13
											20071203T111213+1300   ==>               2007-Dec-02 22:12:13                2007-Dec-03 11:12:13
										 20071203T111213+11:00   ==>               2007-Dec-03 00:12:13                2007-Dec-03 13:12:13
										 20071203111213.123456   ==>        2007-Dec-03 11:12:13.123456         2007-Dec-04 00:12:13.123456
									20071203T111213.12345678   ==>        2007-Dec-03 11:12:13.123456         2007-Dec-04 00:12:13.123456
												20071203T111213.1Z   ==>        2007-Dec-03 11:12:13.100000         2007-Dec-04 00:12:13.100000
									2007-12-03T11:12:13.123Z   ==>        2007-Dec-03 11:12:13.123000         2007-Dec-04 00:12:13.123000
						 2007-12-03T11:12:13.123-05:30   ==>        2007-Dec-03 16:42:13.123000         2007-Dec-04 05:42:13.123000
									20071203T111213.123+1300   ==>        2007-Dec-02 22:12:13.123000         2007-Dec-03 11:12:13.123000

											 Simple date strings                            UTC Date Time                          NZDT (+13)
				----------------------------------       ----------------------------------  ----------------------------------
									1970/06/24 06:10:24.0123   ==>        1970-Jun-23 18:10:24.012300         1970-Jun-24 06:10:24.012300
															 2002/Apr/06   ==>               2002-Apr-05 11:00:00                2002-Apr-06 00:00:00
										 2008 DEC 25  19:59:21   ==>               2008-Dec-25 06:59:21                2008-Dec-25 19:59:21
								 2008 sep 25  19:59:21.123   ==>        2008-Sep-25 07:59:21.123000         2008-Sep-25 19:59:21.123000
										 2002-7-2  9:59:21.123   ==>        2002-Jul-01 21:59:21.123000         2002-Jul-02 09:59:21.123000
							 2008 MAR 2  19:59:21.891872   ==>        2008-Mar-02 06:59:21.891872         2008-Mar-02 19:59:21.891872

																		Manual                            UTC Date Time                          NZDT (+13)
				----------------------------------       ----------------------------------  ----------------------------------
																	D3Date()   ==>               2010-Mar-04 00:39:03                2010-Mar-04 13:39:03
										 SetDate(1998, 12, 27)   ==>               1998-Dec-26 11:00:00                1998-Dec-27 00:00:00
									SetUTCDate(1998, 12, 27)   ==>               1998-Dec-27 00:00:00                1998-Dec-27 13:00:00
					D3Date('19981227').ToLocalDate()   ==>               1998-Dec-27 00:00:00                1998-Dec-27 13:00:00
						D3Date('19981227').ToUTCDate()   ==>               1998-Dec-25 22:00:00                1998-Dec-26 11:00:00

			 Note: The system does not support dates prior to Jan 1, 1970 (aka epoch).
		*/
		D3Date &				operator=		(const std::string & strDateTime);

#ifdef APAL_SUPPORT_ODBC
		//! Construct a date from an odbc::Timestamp
		D3Date(const odbc::Timestamp & oTS, D3TimeZone tz = D3TimeZone::GetLocalServerTimeZone());

		//! Assing an odbc::Timestamp
		D3Date &				operator=		(const odbc::Timestamp & oTS);

		//! Convert to an otl_datetime
										operator odbc::Timestamp() const;
#endif

#ifdef APAL_SUPPORT_OTL
		//! Construct a date from an odbc::Timestamp
		D3Date(const otl_datetime & otlDT, D3TimeZone tz = D3TimeZone::GetLocalServerTimeZone());

		//! Assign an otl_datetime struct
		D3Date &				operator=		(const otl_datetime & otlDT);

		//! Convert to an otl_datetime
										operator otl_datetime() const;
#endif

		//! Convert to Local and return a stringified date in the format YYYY-mmm-DD HH:MM:SS[.ffffff] where mmm 3 char month name. Fractional seconds only included if non-zero and if fractionalPrecision > 0
		std::string			AsString(unsigned short fractionalPrecision = 0) const;
		//! Convert to UTC  and return a stringified date in the format YYYY-mmm-DD HH:MM:SS[.ffffff] where mmm 3 char month name. Fractional seconds only included if non-zero and if fractionalPrecision > 0
		std::string			AsUTCString(unsigned short fractionalPrecision = 0) const;
		//! As local time string US formatlike "Jun 27, 2012 03:45:21PM EDT" (EDT only appears if bIncludeTZAbbrev is true)
		std::string			As12HrLocalString(unsigned short fractionalPrecision = 0, bool bIncludeTZAbbrev = false) const;
		//! As local time string US formatlike "Jun 27, 2012 15:45:21 EDT" (EDT only appears if bIncludeTZAbbrev is true)
		std::string			As24HrLocalString(unsigned short fractionalPrecision = 0, bool bIncludeTZAbbrev = false) const;

		//! Convert to Local YYYYMMDDTHHMMSS[.ffffff][+|-]hh:mm Fractional seconds only included if non-zero and fractionalPrecision > 0.
		std::string			AsISOString(unsigned short fractionalPrecision = 6) const;
		//! Convert to UTC YYYYMMDDTHHMMSS[.ffffff]Z Fractional seconds only included if non-zero and fractionalPrecision > 0.
		std::string			AsUTCISOString(unsigned short fractionalPrecision = 6) const;

		//! Return a string that reflects this in compliance with with the strMask passed in.
		/*!
				The strMask parameter is interpreted as follows [samples in brackets]:

				%a	Abbreviated weekday name *	[Thu]
				%A	Full weekday name *	[Thursday]
				%b	Abbreviated month name *	[Aug]
				%B	Full month name *	[August]
				%d	Day of the month (01-31)	[23]
				%H	Hour in 24h format (00-23)	[14]
				%I	Hour in 12h format (01-12)	[02]
				%j	Day of the year (001-366)	[235]
				%m	Month as a decimal number (01-12)	[08]
				%M	Minute (00-59)	[55]
				%p	AM or PM designation	[PM]
				%S	Second (00-61)	[02]
				%N	Milliseconds (000-999) [023]
				%n	Nanoseconds (000000-999999) [023000]
				%U	Week number with the first Sunday as the first day of week one (00-53)	[33]
				%w	Weekday as a decimal number with Sunday as 0 (0-6)	[4]
				%W	Week number with the first Monday as the first day of week one (00-53)	[34]
				%x	Date representation *	[08/23/01]
				%X	Time representation *	[14:55:02]
				%y	Year, last two digits (00-99)	[01]
				%Y	Year	[2001]
				%Z	Timezone name or abbreviation	[CDT]
				%%	A % sign	[%]

				* The specifiers whose description is marked with an asterisk (*) are locale-dependent.
		*/
		std::string			Format(const std::string& strMask) const;

		//! Same as Format but prints the D3Date in a specific timezone
		std::string			Format(const std::string& strMask, D3TimeZone tz) const		{ return 	D3Date(*this, tz).Format(strMask); }

		//! Debug aid (prints AsISOString() with TimeZone info)
		std::string			Dump() const;

		//! Returns APAL Local DateTime string
		std::string			AsAPAL3DateString() const;
		//! Returns APAL UTC DateTime string
		std::string			AsUTCAPAL3DateString() const;
		//! Returns APAL Local Date string
		std::string			AsAPAL3DateOnlyString() const;
		//! Returns APAL MMDDYY with or without seperator
		std::string			AsAPAL3DateMMDDYY(std::string strSepearator="") const;
		//! Returns APAL UTC Date string
		std::string			AsUTCAPAL3DateOnlyString() const;
		//! Returns ODBC Canonical
		std::string			AsODBCCanonical(std::string strSepearator="-") const;
		//! Returns the date as a constant that can be used in SQL (i.e. "WHERE DateColum=" << d3Date.AsOracleConstant(tz)")
		/*! tz is the D3 timezone of the database server
		*/
		std::string			AsOracleConstant(D3TimeZone tz) const;
		//! Returns the date as a constant that can be used in SQLServer (i.e. "WHERE DateColum=" << d3Date.AsSQLServerConstant(tz)")
		/*! tz is the D3 timezone of the database server
		*/
		std::string			AsSQLServerConstant(D3TimeZone tz) const;

		//! Returns true if this has fractional seconds (milli and nano seconds)
		bool						HasFractional();

		//@{ Date component accessors
		unsigned short	getYear() const					{ return local_time().date().year(); }
		unsigned short	getMonth() const				{ return local_time().date().month(); }
		unsigned short	getDay() const					{ return local_time().date().day(); }
		unsigned long		getHours() const				{ return local_time().time_of_day().hours(); }
		unsigned long		getMinutes() const			{ return local_time().time_of_day().minutes(); }
		unsigned long		getSeconds() const			{ return local_time().time_of_day().seconds(); }
		unsigned long		getMicroseconds() const	{ return (long) local_time().time_of_day().fractional_seconds(); }
		unsigned short	getDayofWeek() const		{ return local_time().date().day_of_week().as_number(); }

		unsigned short	getUTCYear() const							{ return utc_time().date().year(); }
		unsigned short	getUTCMonth() const							{ return utc_time().date().month(); }
		unsigned short	getUTCDay() const								{ return utc_time().date().day(); }
		unsigned long		getUTCHours() const							{ return utc_time().time_of_day().hours(); }
		unsigned long		getUTCMinutes() const						{ return utc_time().time_of_day().minutes(); }
		unsigned long		getUTCSeconds() const						{ return utc_time().time_of_day().seconds(); }
		unsigned long		getUTCMicroseconds() const			{ return (long) utc_time().time_of_day().fractional_seconds(); }
		unsigned short	getUTCDayofWeek() const					{ return utc_time().date().day_of_week().as_number(); }
		//@}

		//@{ setters
		//! Set the date to a local value (use 24hr clock for time)
		bool						SetDate(		unsigned short uYear,
																unsigned short uMonth,
																unsigned short uDay,
																unsigned short uHour = 0,
																unsigned short uMinute = 0,
																unsigned short uSecond = 0,
																unsigned int uMicro = 0);

		//! Set the date to a UTC value (use 24hr clock for time)
		bool						SetUTCDate(	unsigned short uYear,
																unsigned short uMonth,
																unsigned short uDay,
																unsigned short uHour = 0,
																unsigned short uMinute = 0,
																unsigned short uSecond = 0,
																unsigned int uMicro = 0);
		//@}

		//! Make local date
		/*! Here the following applies:

				aD3Date.AsString() == a3Date.ToLocalDate().AsUTCString()

		*/
		D3Date					ToLocalDate();

		//! Make a UTC date
		/*! Here the following applies:

				aD3Date.AsUTCString() == a3Date.ToUTCDate().AsString()

		*/
		D3Date					ToUTCDate();

		//! Return true if this is a UTC date time
		bool						IsUTCDate();

		//! Allows you to convert this into another timezone
		void						AdjustToTimeZone(D3TimeZone tz);

		//! Compares this with another date
		/*! Returns
					-1: this is older than RHS
					0:	this is the same as RHS
					1:	this is newer than RHS

				\note The comparison will be done in local times (internally, dtRHS will be translated to the same timezone as this before comparing).
		*/
		int							Compare(D3Date & dtRHS) const;

		//! Compares this with another date ignoring the time portion
		/*! Returns
					-1: this is older than RHS
					0:	this is the same as RHS
					1:	this is newer than RHS

				\note The comparison will be done in local times (internally, dtRHS will be translated to the same timezone as this before comparing).
		*/
		int							CompareDateOnly(D3Date & dtRHS) const;

	protected:
		//! Helper that attempts to set this from the parts passed in. Note that the entire date will be reset and not just parts.
		/*! Returns false if something goes wrong and in this case, the original date is retained.
		*/
		void																setFromParts(	const std::string& strYear,
																											const std::string& strMonth,
																											const std::string& strDay,
																											const std::string& strHour = "0",
																											const std::string& strMinute = "0",
																											const std::string& strSecond = "0",
																											const std::string& strMicro = "0");

		//! Helper that attempts to set this from the IS8601 parts passed in.
		void																setFromISO8601(const boost::smatch& matches);

		//! returns this as a ptime object converted to the local date/time
		static boost::posix_time::ptime			asLocalDate(const boost::posix_time::ptime& utcTime);

		//! returns this as a ptime object converted to UTC date/time
		static boost::posix_time::ptime			asUTCDate(const boost::posix_time::ptime& localTime);

		//! Convert tm to YYYY-MM-DDTHH:MM:SS[.ffffff] Fractional seconds only included if non-zero and fractionalPrecision > 0
		static std::string									asBaseISO8601String(const boost::posix_time::ptime& localTime, unsigned short fractionalPrecision);

		//! Returns the time zone abbreviation for the current time and timezone
		std::string													getTZAbbrv() const;

		//! returns the date portion of this as a string in the form "Jun 24, 2012"
		std::string													localDateAsString() const;
};

//! This operator enables streaming of D3Date objects to std::ostream types.
inline std::ostream& operator<<(std::ostream& out, const D3Date& aDate)
{
	out << aDate.AsString().c_str();
	return out;
}

#endif /* _D3Date_H_ */
