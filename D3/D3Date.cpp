// MODULE: D3Date source
//;
// ===========================================================
// File Info:
//
// $Author: lalit $
// $Date: 2014/12/16 12:54:23 $
// $Revision: 1.27 $
//
// D3Date: The class used to represent dates from datastores
//
// ===========================================================
// Change History:
// ===========================================================
//
// 19/02/04 - R1 - Hugp
//
// Created class
//
// -----------------------------------------------------------

#include "D3Types.h"

#include <time.h>
#include <sstream>
#include <stdexcept>
#include "D3Date.h"

#include <boost/date_time/local_time/local_time.hpp>
#include <boost/date_time/local_time_adjustor.hpp>
#include <boost/date_time/c_local_time_adjustor.hpp>
#include <boost/date_time/posix_time/conversion.hpp>

#include <iomanip>

typedef boost::date_time::c_local_adjustor<boost::posix_time::ptime> local_adj;


// Ugly helper that:
//     looks for the last '.' in the strIn and if exists
//				if fractionalPrecision is 0
//          removes everything following the '.' as well as the '.'
//        else
//          makes sure there are no more than fractionalPrecision chars following the '.'
//          removes any trailing '0', and the '.' if the fraction is zero
//
std::string& uglyTruncateDateFractions(std::string& strIn, unsigned short fractionalPrecision)
{
	std::string::size_type	pos = strIn.find_last_of('.');

	if (pos != std::string::npos)
	{
		if (fractionalPrecision == 0)
		{
			if (pos > 0)
				strIn = strIn.substr(0, pos);
		}
		else
		{
 			strIn = strIn.substr(0, pos + fractionalPrecision + 1);

			// strip trailing '0'
			std::string::size_type	pos2 = strIn.find_last_not_of('0');

			if (pos2 != std::string::npos)
			{
				if (pos2 == pos)
					pos2--;		// Also strip the '.'
				if (pos2 != strIn.size() - 1)
					strIn.erase(pos2 + 1);
			}
		}
	}

	return strIn;
}






// ==========================================================================
// D3TimeZone implementation
//

//! Initially, this will be UTC until set explicitly
D3TimeZone		D3TimeZone::M_defaultServerTimeZone;
D3TimeZone		D3TimeZone::M_explicitServerTimeZone;
D3TimeZone		D3TimeZone::M_utcTimeZone("UTC");




//! Returns the local servers timezone
/*! If M_explicitServerTimeZone does not specify a TimeZone, the system will create
		a default time zone called UKN and set its time offset from UTC according to the
		local systems time zone configuration. This value will be cached and returned
		on subsequent calls.
*/
/* static */
D3TimeZone	D3TimeZone::GetDefaultServerTimeZone()
{
	if (M_explicitServerTimeZone)
		return M_explicitServerTimeZone;

	if (!M_defaultServerTimeZone)
	{
		std::ostringstream		strm;
		boost::posix_time::ptime	tmUTC = boost::posix_time::second_clock::universal_time();
		boost::posix_time::ptime	tmLocal = local_adj::utc_to_local(tmUTC);
		boost::posix_time::time_duration	td = tmLocal > tmUTC ? tmLocal - tmUTC : tmUTC - tmLocal;

		if (td.hours() || td.minutes() || td.seconds())
		{
			strm << "UKN";

			if (tmLocal > tmUTC)
				strm << '+';
			else
				strm << '-';

			if (td.hours() < 10) strm << '0';
			strm << td.hours();

			if (td.minutes() || td.seconds())
			{
				strm << ':';
				if (td.minutes() < 10) strm << '0';
				strm << td.minutes();
			}

			if (td.seconds())
			{
				strm << ':';
				if (td.seconds() < 10) strm << '0';
				strm << td.seconds();
			}
		}
		else
		{
			strm << "UTC";
		}

		M_defaultServerTimeZone = D3TimeZone(strm.str());
	}

	return M_defaultServerTimeZone;
}


std::string formatDuration(const boost::posix_time::time_duration & d)
{
	using namespace std;

	ostringstream			ostrm;

	if (d.hours() < 0)
		ostrm << '-';

	ostrm << setfill('0') << setw(2) << abs(d.hours()) << ':';
	ostrm << setfill('0') << setw(2) << d.minutes() << ':';
	ostrm << setfill('0') << setw(2) << d.seconds();

	return ostrm.str();
}

void D3TimeZone::Dump(int indent)
{
	using namespace std;

	cout << setw(indent) << "Timezone string......................: " << (*this)->to_posix_string() << endl;
	cout << setw(indent) << "Standard abbreviation abbreviation...: " << (*this)->std_zone_abbrev() << endl;
	cout << setw(indent) << "Daylight savings abbreviation........: " << (*this)->dst_zone_abbrev() << endl;
	cout << setw(indent) << "Standard zone name...................: " << (*this)->std_zone_name() << endl;
	cout << setw(indent) << "Standard offset from UTC.............: " << formatDuration((*this)->base_utc_offset()) << endl;
	cout << setw(indent) << "Has daylight savings time............: " << ((*this)->has_dst() ? "true" : "false") << endl;

	if ((*this)->has_dst())
	{
		cout << setw(indent) << "The daylight savings time begins.....: " << boost::posix_time::to_simple_string((*this)->dst_local_start_time(2013)) << endl;
		cout << setw(indent) << "The daylight savings time ends.......: " << boost::posix_time::to_simple_string((*this)->dst_local_end_time(2013)) << endl;
		cout << setw(indent) << "Shift during daylight savings........: " << formatDuration((*this)->dst_offset()) << endl;
	}
}



// ==========================================================================
// D3Date implementation
//
const char * D3Date::MaxISO8601DateTime = "2038-01-01T00:00:00Z";


//! The default constructor will set this date to Now()
D3Date::D3Date(D3TimeZone tz)
: boost::local_time::local_date_time(boost::posix_time::ptime(boost::date_time::neg_infin),	tz)
{
	*this = Now(tz);
}





// Constructs a date from a string (see operator=() for more details)
D3Date::D3Date(const std::string & strDateTime, D3TimeZone tz)
: boost::local_time::local_date_time(boost::posix_time::ptime(boost::date_time::neg_infin),	tz)
{
	*this = strDateTime;
}





D3Date::D3Date(const D3Date & dt, D3TimeZone tz)
: boost::local_time::local_date_time(boost::posix_time::ptime(boost::date_time::neg_infin),	tz)
{
	*this = D3Date(dt.utc_time(), tz);
}





// Returns the current system date and time
/* static */
D3Date D3Date::Now(D3TimeZone tz)
{
	return boost::local_time::local_microsec_clock::local_time(tz);
}






// Assign a string representation of a date in the format 'YYYY-MM-DD hh:mm:ss.nnnn'.
//
D3Date & D3Date::operator=(const std::string & strDateIn)
{
	bool	bSuccess = false;

	static const boost::regex		rgxAPALDateLong				("\\A(\\d{4})[-]?(\\d{2})[-]?(\\d{2})[ ]?(\\d{2})[:]?(\\d{2})[:]?(\\d{2})\\z");
	static const boost::regex		rgxAPALDateShort			("\\A(\\d{4})[-]?(\\d{2})[-]?(\\d{2})\\z");
	static const boost::regex		rgxISO8601						("\\A(\\d{4})(-)?(\\d{1,2})(-)?(\\d{1,2})(T)?((\\d{1,2})(:)?(\\d{2})(:)?(\\d{2})(\\.\\d+)?)?(Z|([ \\+-])(\\d{2})(:)?(\\d{2}))?\\z");
	static const boost::regex		rgxSimpleDate					("\\A(\\d{4})[-/ ](jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)[-/ ](\\d{1,2})\\z", boost::regex::icase);
	static const boost::regex		rgxSimpleDateTime			("\\A(\\d{4})[-/ ](jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)[-/ ](\\d{1,2})\\s+(\\d{1,2})[:](\\d{2})[:](\\d{2})\\z", boost::regex::icase);
	static const boost::regex		rgxSimpleDateTimeMicro("\\A(\\d{4})[-/ ](jan|feb|mar|apr|may|jun|jul|aug|sep|oct|nov|dec)[-/ ](\\d{1,2})\\s+(\\d{1,2})[:](\\d{2})[:](\\d{2})[\\.,](\\d{1,9})\\z", boost::regex::icase);
	static const boost::regex		rgxDate								("\\A(\\d{4})[-/ ](\\d{1,2})[-/ ](\\d{1,2})\\z");
	static const boost::regex		rgxDateTime						("\\A(\\d{4})[-/ ](\\d{1,2})[-/ ](\\d{1,2})\\s+(\\d{1,2})[:](\\d{2})[:](\\d{2})\\z");
	static const boost::regex		rgxDateTimeMicro			("\\A(\\d{4})[-/ ](\\d{1,2})[-/ ](\\d{1,2})\\s+(\\d{1,2})[:](\\d{2})[:](\\d{2})[\\.,](\\d{1,9})\\z");
	boost::smatch								matchResult;
	std::string									strDate(strDateIn);
	std::string::size_type			pos;
	std::string									strParts[7];		// [0]=year,[1]=month,[2]=day,[3]=hour,[4]=min,[5]=secs,[6]=micros

	// remove leading and trailing blanks
	pos = strDate.find_first_not_of(' ');
	if (pos != std::string::npos)
		strDate.erase(0, pos);
	pos = strDate.find_last_not_of(' ');
	if (pos != strDate.size() - 1)
		strDate.erase(pos+1);

	if (boost::regex_match(strDate, matchResult, rgxAPALDateLong))
		bSuccess = true;
	else if (boost::regex_match(strDate, matchResult, rgxAPALDateShort))
		bSuccess = true;
	else if (boost::regex_match(strDate, matchResult, rgxISO8601))
	{
		setFromISO8601(matchResult);
		return *this;
	}
	else if (boost::regex_match(strDate, matchResult, rgxSimpleDate))
		bSuccess = true;
	else if (boost::regex_match(strDate, matchResult, rgxSimpleDateTime))
		bSuccess = true;
	else if (boost::regex_match(strDate, matchResult, rgxSimpleDateTimeMicro))
		bSuccess = true;
	else if (boost::regex_match(strDate, matchResult, rgxDate))
		bSuccess = true;
	else if (boost::regex_match(strDate, matchResult, rgxDateTime))
		bSuccess = true;
	else if (boost::regex_match(strDate, matchResult, rgxDateTimeMicro))
		bSuccess = true;

	if (bSuccess)
	{
		for (unsigned idx=1; idx < matchResult.size(); idx++)
			strParts[idx-1] = matchResult[idx];

		setFromParts(strParts[0],strParts[1],strParts[2],strParts[3],strParts[4],strParts[5],strParts[6]);
	}
	else
	{
		std::ostringstream ostrm;
		ostrm << "D3Date::operator=(): Failed to convert the string '" << strDateIn << "' into a D3Date.";
		throw std::runtime_error(ostrm.str());
	}

	return *this;
}





#ifdef APAL_SUPPORT_ODBC
	// Assign an odbc::Timestamp
	D3Date::D3Date(const odbc::Timestamp & oTS, D3TimeZone tz)
	:	boost::local_time::local_date_time(	boost::gregorian::date(oTS.getYear(), oTS.getMonth(),  oTS.getDay()),
																				boost::posix_time::time_duration(oTS.getHour(), oTS.getMinute(), oTS.getSecond(), oTS.getNanos() / 1000),
																				tz,
																				NOT_DATE_TIME_ON_ERROR)
	{
	}





	// Assign an odbc::Timestamp (they are expected to be UTC)
	D3Date & D3Date::operator=(const odbc::Timestamp & oTS)
	{
		*this = D3Date(oTS, zone());
		return *this;
	}





	// convert this into an odbc::Timestamp (as UTC)
	D3Date::operator odbc::Timestamp() const
	{
		boost::posix_time::ptime	tm = local_time();

		return odbc::Timestamp(	tm.date().year(),
														tm.date().month(),
														tm.date().day(),
														tm.time_of_day().hours(),
														tm.time_of_day().minutes(),
														tm.time_of_day().seconds(),
														(int) tm.time_of_day().fractional_seconds() / 1000);
	}
#endif





#ifdef APAL_SUPPORT_OTL
	// Assign an const otl_datetime & otlDT
	D3Date::D3Date(const otl_datetime & otlDT, D3TimeZone tz)
	:	boost::local_time::local_date_time(	boost::gregorian::date(otlDT.year, otlDT.month,  otlDT.day),
																				boost::posix_time::time_duration(otlDT.hour, otlDT.minute, otlDT.second, otlDT.fraction),
																				tz,
																				NOT_DATE_TIME_ON_ERROR)
	{
	}





	// Assign a string represemntation of a date in the format 'YYYY-MM-DD hh:mm:ss.nnnn'.
	D3Date & D3Date::operator=(const otl_datetime & otlDT)
	{
		*this = D3Date(otlDT, zone());
		return *this;
	}





	D3Date::operator otl_datetime() const
	{
		otl_datetime							otlDT;
		boost::posix_time::ptime	tm = local_time();

		otlDT.year		= tm.date().year();
		otlDT.month		= tm.date().month();
		otlDT.day			= tm.date().day();

		otlDT.hour		= tm.time_of_day().hours();
		otlDT.minute	= tm.time_of_day().minutes();
		otlDT.second	= tm.time_of_day().seconds();

		if (tm.time_of_day().fractional_seconds())
		{
			otlDT.fraction				= tm.time_of_day().fractional_seconds();
      otlDT.frac_precision	= 4;
		}

		return otlDT;
	}
#endif





bool D3Date::HasFractional()
{
	return utc_time().time_of_day().fractional_seconds() != 0;
}




// AsUTCString
std::string D3Date::AsString(unsigned short fractionalPrecision) const
{
	std::string strBuffer(boost::posix_time::to_simple_string(local_time()));
	uglyTruncateDateFractions(strBuffer, fractionalPrecision);
	return  strBuffer;
}




// AsUTCString
std::string D3Date::AsUTCString(unsigned short fractionalPrecision) const
{
	std::string strBuffer(boost::posix_time::to_simple_string(utc_time()));
	uglyTruncateDateFractions(strBuffer, fractionalPrecision);
	return  strBuffer;
}


std::string D3Date::As12HrLocalString(unsigned short fractionalPrecision, bool bIncludeTZAbbrev) const
{
	using namespace std;

	int	iHrs = getHours() % 12;

	ostringstream ostr;

	ostr << localDateAsString();
	ostr << ' ';

	ostr << setfill('0') << setw(2) << (iHrs == 0 ? 12 : iHrs) << ':';
	ostr << setfill('0') << setw(2) << getMinutes() << ':';
	ostr << setfill('0') << setw(2) << getSeconds();

	if (fractionalPrecision > 0)
	{
		fractionalPrecision = fractionalPrecision > 6 ? 6 : fractionalPrecision;

		ostr << '.' << setfill('0') << setw(fractionalPrecision) << (getMicroseconds() / (int) pow((float) 10, 6 - fractionalPrecision));
	}

	ostr << (getHours() < 12 ? "AM" : "PM");

	if (bIncludeTZAbbrev)
		ostr << ' ' << getTZAbbrv();

	return ostr.str();
}



std::string D3Date::As24HrLocalString(unsigned short fractionalPrecision, bool bIncludeTZAbbrev) const
{
	using namespace std;

	ostringstream ostr;

	ostr << localDateAsString();
	ostr << ' ';

	ostr << setfill('0') << setw(2) << getHours() << ':';
	ostr << setfill('0') << setw(2) << getMinutes() << ':';
	ostr << setfill('0') << setw(2) << getSeconds();

	if (fractionalPrecision > 0)
	{
		fractionalPrecision = fractionalPrecision > 6 ? 6 : fractionalPrecision;

		ostr << '.' << setfill('0') << setw(fractionalPrecision) << (getMicroseconds() / (int) pow((float) 10, 6 - fractionalPrecision));
	}

	if (bIncludeTZAbbrev)
		ostr << ' ' << getTZAbbrv();

	return ostr.str();
}



// AsISOString
std::string D3Date::AsISOString(unsigned short fractionalPrecision) const
{
	std::string													strISO8601;
	boost::posix_time::ptime						tm(local_time());
	boost::posix_time::time_duration		td = utc_time() - tm;

	strISO8601 = asBaseISO8601String(local_time(), fractionalPrecision);

	strISO8601 += td.is_negative() ? '+' : '-';
	strISO8601 += '0' + (char) (abs(td.hours()) / 10);
	strISO8601 += '0' + (char) (abs(td.hours()) % 10);
	strISO8601 += ':';
	strISO8601 += '0' + (char) (abs(td.minutes()) / 10);
	strISO8601 += '0' + (char) (abs(td.minutes()) % 10);

	return strISO8601;
}





// AsUTCISOString
std::string D3Date::AsUTCISOString(unsigned short fractionalPrecision) const
{
	std::string		strISO8601 = asBaseISO8601String(utc_time(), fractionalPrecision);

	strISO8601 += 'Z';

	return strISO8601;
}





// Format
std::string	D3Date::Format(const std::string& strMask) const
{
	std::string								mask(strMask);
	char											buffer[256];
	std::string::size_type		tzPos;

	// deal with milliseconds
	tzPos = 0;

	while (true)
	{
		tzPos = mask.find("%N", tzPos);

		if (tzPos == std::string::npos)
			break;

		if ((tzPos - 1) < 0 || mask[tzPos - 1] != '%')
		{
			snprintf(buffer, 256, "%03i", ((time_of_day().fractional_seconds() / 100) + 5) / 10);
			mask.replace(tzPos, 2, buffer);
		}

		tzPos += 2;
	}

	// deal with nanoseconds
	tzPos = 0;

	while (true)
	{
		tzPos = mask.find("%n", tzPos);

		if (tzPos == std::string::npos)
			break;

		if ((tzPos - 1) < 0 || mask[tzPos - 1] != '%')
		{
			snprintf(buffer, 256, "%06i", time_of_day().fractional_seconds());
			mask.replace(tzPos, 2, buffer);
		}

		tzPos += 2;
	}

	// deal with timezone
	tzPos = 0;

	while (true)
	{
		tzPos = mask.find("%Z", tzPos);

		if (tzPos == std::string::npos)
			break;

		if ((tzPos - 1) < 0 || mask[tzPos - 1] != '%')
			mask.replace(tzPos, 2, getTZAbbrv());

		tzPos += 2;
	}

	// let the std c library do the rest
	strftime(buffer, 256, mask.c_str(), &(to_tm(*this)));

	return buffer;
};





//! Debug aid (prints AsISOString() with TimeZone info)
std::string D3Date::Dump() const
{
	std::string		strDump = AsISOString();

	strDump += ' ';
	strDump += zone()->to_posix_string();


	return strDump;
}





std::string	D3Date::AsAPAL3DateString() const
{
	std::string strVal;

	if (!is_special())
	{
		char											tmpbuf[32];
		boost::posix_time::ptime	tm(local_time());

		snprintf(tmpbuf, 32, "%04u%02u%02u%02u%02u%02u",	(unsigned short)tm.date().year(),
																											(unsigned short)tm.date().month(),
																											(unsigned short)tm.date().day(),
																											tm.time_of_day().hours(),
																											tm.time_of_day().minutes(),
																											tm.time_of_day().seconds());

		strVal = tmpbuf;
	}

	return strVal;
}





std::string	D3Date::AsUTCAPAL3DateString() const
{
	std::string strVal;

	if (!is_special())
	{
		char											tmpbuf[32];
		boost::posix_time::ptime	tm(utc_time());

		snprintf(tmpbuf, 32, "%04u%02u%02u%02u%02u%02u",	(unsigned short)tm.date().year(),
																											(unsigned short)tm.date().month(),
																											(unsigned short)tm.date().day(),
																											tm.time_of_day().hours(),
																											tm.time_of_day().minutes(),
																											tm.time_of_day().seconds());

		strVal = tmpbuf;
	}

	return strVal;
}





std::string	D3Date::AsAPAL3DateOnlyString() const
{
	std::string strVal;

	if (!is_special())
	{
		char											tmpbuf[32];
		boost::posix_time::ptime	tm(local_time());

		snprintf(tmpbuf, 32, "%04u%02u%02u",	(unsigned short)tm.date().year(),
																					(unsigned short)tm.date().month(),
																					(unsigned short)tm.date().day());

		strVal = tmpbuf;
	}

	return strVal;
}





std::string	D3Date::AsAPAL3DateMMDDYY(std::string strSepearator) const
{
	std::string strVal;

	if (!is_special())
	{
		char											tmpbuf[32];
		boost::posix_time::ptime	tm(local_time());

		// Get YY from CCYY
		snprintf(tmpbuf, 32, "%04u",							(unsigned short)tm.date().year());
		std::string strYear = tmpbuf;
		strYear = strYear.substr(2,2);

		snprintf(tmpbuf, 32, "%02u%s%02u%s%s",		(unsigned short)tm.date().month(),
																							strSepearator.c_str(),
																							(unsigned short)tm.date().day(),
																							strSepearator.c_str(),
																							strYear.c_str());

		strVal = tmpbuf;
	}

	return strVal;
}





std::string	D3Date::AsUTCAPAL3DateOnlyString() const
{
	std::string strVal;

	if (!is_special())
	{
		char											tmpbuf[32];
		boost::posix_time::ptime	tm(utc_time());

		snprintf(tmpbuf, 32, "%04u%02u%02u",	(unsigned short)tm.date().year(),
																					(unsigned short)tm.date().month(),
																					(unsigned short)tm.date().day());

		strVal = tmpbuf;
	}

	return strVal;
}




std::string	D3Date::AsODBCCanonical(std::string strSepearator) const
{
	std::string strVal;

	if (!is_special())
	{
		char											tmpbuf[32];
		boost::posix_time::ptime	tm(local_time());

		snprintf(tmpbuf, 32, "%04u%s%02u%s%02u %02u:%02u:%02u",	(unsigned short)tm.date().year(),
																														strSepearator.c_str(),
																														(unsigned short)tm.date().month(),
																														strSepearator.c_str(),
																														(unsigned short)tm.date().day(),
																														tm.time_of_day().hours(),
																														tm.time_of_day().minutes(),
																														tm.time_of_day().seconds());
		strVal = tmpbuf;
	}

	return strVal;
}




std::string D3Date::AsOracleConstant(D3TimeZone tz) const
{
	D3Date							dtRDBMS(utc_time(), tz);
	std::ostringstream	ostrm;

	ostrm << "TO_DATE('" << dtRDBMS.AsODBCCanonical() << "', 'YYYY-MM-DD HH24:MI:SS')";

	return ostrm.str();
}




std::string D3Date::AsSQLServerConstant(D3TimeZone tz) const
{
	D3Date							dtRDBMS(utc_time(), tz);
	std::ostringstream	ostrm;

	ostrm << "CONVERT(datetime, '" << dtRDBMS.AsODBCCanonical() << "', 121)";

	return ostrm.str();
}




bool D3Date::SetDate(	unsigned short year,
											unsigned short month,
											unsigned short day,
											unsigned short hour,
											unsigned short minute,
											unsigned short second,
											unsigned int micro)
{
	// some sanity checks
	if (year < 1970 || month > 12 || day > 31 || hour > 23 || minute > 59 || second > 59 || micro > 999999)
		return false;

	*this = boost::local_time::local_date_time(	boost::gregorian::date(year, month, day),
																							boost::posix_time::time_duration(hour, minute, second, micro),
																							zone(),
																							NOT_DATE_TIME_ON_ERROR);

	return true;
}





bool D3Date::SetUTCDate(unsigned short year,
												unsigned short month,
												unsigned short day,
												unsigned short hour,
												unsigned short minute,
												unsigned short second,
												unsigned int micro)
{
	// some sanity checks
	if (year < 1970 || month > 12 || day > 31 || hour > 23 || minute > 59 || second > 59 || micro > 999999)
		return false;

	*this = boost::local_time::local_date_time(	boost::posix_time::ptime(	boost::gregorian::date(year, month, day),
																																				boost::posix_time::time_duration(hour, minute, second, micro)),
																							zone());

	return true;
}





void	D3Date::setFromParts (const std::string& strYear,
														const std::string& strMonth,
														const std::string& strDay,
														const std::string& strHour,
														const std::string& strMinute,
														const std::string& strSecond,
														const std::string& strMicro)
{
	int short year, month, day, hour, minute, second;
	int				micro;

	const int	iMaxYear = 2037;
	const int	iMaxMonth = 12;
	const int	iMaxDay = 31;

	// the month can be a 3 char month name
	if (strMonth.length() == 3)
	{
		std::string	strMMM(strMonth);
		std::string	strColl("janfebmaraprmayjunjulaugsepoctnovdec");

		// convert to lowercase and do a lookup
		std::transform(strMMM.begin(), strMMM.end(), strMMM.begin(), tolower);
		std::string::size_type pos = strColl.find(strMMM);

		if (pos == std::string::npos)
			throw std::runtime_error("Unknown month name");

		month = (pos / 3) + 1;
	}
	else
	{
		month		= atoi(strMonth.c_str());
	}

	if (!strMicro.empty() && strMicro.length() != 6)
	{
		// truncate extra digits if the microseconds is too long or pad it with 0 if it is to short
		std::string	strMS(strMicro.substr(0,6));
		while (strMS.length() < 6) strMS += '0';
		micro		= atoi(strMS.c_str());
	}
	else
	{
		micro		= atoi(strMicro.c_str());
	}

	year		= atoi(strYear.c_str());
	day			= atoi(strDay.c_str());
	hour		= atoi(strHour.c_str());
	minute	= atoi(strMinute.c_str());
	second	= atoi(strSecond.c_str());

	if (year < 100)
		year += 2000;

	// Todo : We don;t need this if YUI can handle it.
	// Reset the date if beyond year 2037 to 12/31/2037
	if((year-1) >= iMaxYear)
	{
		D3::ReportWarning("D3Date::setFromParts(): Resetting date to 12/31/2037 as it is beyond that.");
		year = iMaxYear;
		month = iMaxMonth;
		day = iMaxDay;
	}

	// some sanity checks
	if (year < 1970 || month > 12 || day > 31 || hour > 23 || minute > 59 || second > 59 || micro > 999999)
		throw std::runtime_error("Incorrect date string");

	*this = boost::local_time::local_date_time(	boost::gregorian::date(year, month, day),
																							boost::posix_time::time_duration(hour, minute, second, micro),
																							zone(),
																							NOT_DATE_TIME_ON_ERROR);
}



void D3Date::setFromISO8601(const boost::smatch& matches)
{
	std::string								aryMatch[20];
	int short									year, month, day, hour, minute, second;
	int												micro;
	boost::posix_time::ptime	tm;


	for (unsigned idx=0; idx < matches.size() && idx < 20; idx++)
		aryMatch[idx] = matches[idx];

	year		= atoi(aryMatch[1].c_str());
	month		= atoi(aryMatch[3].c_str());
	day			= atoi(aryMatch[5].c_str());

	boost::gregorian::date							dt(year, month, day);

	if (aryMatch[7].empty())
	{
		tm = boost::posix_time::ptime(dt);
	}
	else
	{
		hour		= atoi(aryMatch[8].c_str());
		minute	= atoi(aryMatch[10].c_str());
		second	= atoi(aryMatch[12].c_str());

		// Add micro seconds
		if (!aryMatch[13].empty())
		{
			std::string		strMicro(aryMatch[13].substr(1));

			if (strMicro.length() != 6)
			{
				// truncate extra digits if the microseconds is too long or pad it with 0 if it is to short
				std::string	strMS(strMicro.substr(0,6));
				while (strMS.length() < 6) strMS += '0';
				micro		= atoi(strMS.c_str());
			}
			else
			{
				micro		= atoi(strMicro.c_str());
			}
		}
		else
		{
			micro = 0;
		}

		assert(month > 12 || day > 31 || hour > 23 || minute > 59 || second > 59 || micro > 999999 == false);

		boost::posix_time::time_duration		t(hour, minute, second, micro);

		tm = boost::posix_time::ptime(dt, t);

		// Nothing or a 'Z' indicates UTC, otherwise we expect something like [+|-]hh[:]mm
		if (!aryMatch[14].empty() && aryMatch[14] != "Z")
		{
			int short														opr, ofsHrs, ofsMins;

			opr = ((aryMatch[15] == "-") ? -1 : 1);
			ofsHrs = opr * atoi(aryMatch[16].c_str());
			ofsMins = opr * atoi(aryMatch[18].c_str());

			tm = tm - boost::posix_time::time_duration(ofsHrs, ofsMins, 0);
		}
	}

	*this = D3Date(tm, zone());
}




std::string D3Date::asBaseISO8601String(const boost::posix_time::ptime& tm, unsigned short fractionalPrecision)
{
	char						tmpbuf[36];
	std::string			strDT;
	unsigned short	uYear		= tm.date().year();
	unsigned short	uMonth	= tm.date().month();
	unsigned short	uDay		= tm.date().day();
	unsigned long		uHour		= tm.time_of_day().hours();
	unsigned long		uMinute	= tm.time_of_day().minutes();
	unsigned long		uSecond =	tm.time_of_day().seconds();

	snprintf(tmpbuf, 36, "%04u-%02u-%02uT%02u:%02u:%02u", uYear, uMonth, uDay, uHour, uMinute, uSecond);

	strDT = tmpbuf;

	if (fractionalPrecision > 0 && tm.time_of_day().fractional_seconds() > 0)
	{
		snprintf(tmpbuf, 36, "%f", (double) tm.time_of_day().fractional_seconds() / 1000000.0);
		std::string strBuffer(&(tmpbuf[1]));
		strDT += uglyTruncateDateFractions(strBuffer, fractionalPrecision);
	}

	return strDT;
}




D3Date D3Date::ToLocalDate()
{ 
	return D3Date(*this, D3TimeZone::GetLocalServerTimeZone());
}




D3Date D3Date::ToUTCDate()
{ 
	if (!IsUTCDate())
		return D3Date(utc_time(), D3TimeZone::GetUTCTimeZone());

	return *this;
}




bool D3Date::IsUTCDate()
{ 
	boost::posix_time::time_duration	base, dst, tdZero(0,0,0);
	
	if (zone().get())
	{
		base = zone()->base_utc_offset();
		dst = zone()->dst_offset();
	}

	return base == tdZero && dst == tdZero;
}




void D3Date::AdjustToTimeZone(D3TimeZone tz)
{
	*this = D3Date(utc_time(), tz);
}



int D3Date::Compare(D3Date & dtRHS) const
{
	D3Date																					dtRHSNormalized(dtRHS.utc_time(), (D3TimeZone) zone());
	boost::posix_time::ptime::date_type							dLHS(local_time().date()), dRHS(dtRHSNormalized.local_time().date());
	boost::posix_time::time_duration::duration_type	tmLHS(local_time().time_of_day()), tmRHS(dtRHSNormalized.local_time().time_of_day());

	if (dLHS.year() < dRHS.year())
		return -1;
	if (dLHS.year() > dRHS.year())
		return 1;
	if (dLHS.month() < dRHS.month())
		return -1;
	if (dLHS.month() > dRHS.month())
		return 1;
	if (dLHS.day() < dRHS.day())
		return -1;
	if (dLHS.day() > dRHS.day())
		return 1;
	if (tmLHS.hours() < tmRHS.hours())
		return -1;
	if (tmLHS.hours() > tmRHS.hours())
		return 1;
	if (tmLHS.minutes() < tmRHS.minutes())
		return -1;
	if (tmLHS.minutes() > tmRHS.minutes())
		return 1;
	if (tmLHS.seconds() < tmRHS.seconds())
		return -1;
	if (tmLHS.seconds() > tmRHS.seconds())
		return 1;

	return 0;
}



int D3Date::CompareDateOnly(D3Date & dtRHS) const
{
	D3Date																					dtRHSNormalized(dtRHS.utc_time(), (D3TimeZone) zone());
	boost::posix_time::ptime::date_type							dLHS(local_time().date()), dRHS(dtRHSNormalized.local_time().date());

	if (dLHS.year() < dRHS.year())
		return -1;
	if (dLHS.year() > dRHS.year())
		return 1;
	if (dLHS.month() < dRHS.month())
		return -1;
	if (dLHS.month() > dRHS.month())
		return 1;
	if (dLHS.day() < dRHS.day())
		return -1;
	if (dLHS.day() > dRHS.day())
		return 1;

	return 0;
}



std::string D3Date::getTZAbbrv() const
{
	bool bDST = false;

	if (this->zone()->has_dst())
	{
		if (this->zone()->dst_local_start_time(this->getYear()) < this->zone()->dst_local_end_time(this->getYear()))
			bDST = local_time() >= this->zone()->dst_local_start_time(this->getYear()) && local_time() <= this->zone()->dst_local_end_time(this->getYear());
		else
			bDST = local_time() <= this->zone()->dst_local_end_time(this->getYear()) || local_time() >= this->zone()->dst_local_start_time(this->getYear());
	}

	return bDST ? this->zone()->dst_zone_abbrev() : this->zone()->std_zone_abbrev();
}



std::string D3Date::localDateAsString() const
{
	std::ostringstream ostr;
	char *	sMN[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

	ostr << sMN[getMonth() - 1] << ' ';
	ostr << getDay() << ", ";
	ostr << getYear();

	return  ostr.str();
}
