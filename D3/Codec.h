/**
 * \class Codec
 *
 * \ingroup APALUtil
 *
 * \brief
 *
 * Coder/Decoder
 *
 * \note
 *
 * \author (last to touch it) $Author: pete $
 *
 * \version $Revision: 1.8 $
 *
 * \date $Date: 2014/10/04 01:02:09 $
 *
 * Created on:
 *
 * $Id: Codec.h,v 1.8 2014/10/04 01:02:09 pete Exp $
 *
 */

#ifndef __APALUtil_Codec_H__
#define __APALUtil_Codec_H__

#include <vector>
#include <list>
#include <string>
#include <sstream>

using namespace std;

namespace APALUtil
{
	// encoders --------------------------

	// Takes decoded data as input and populates a string with the encoded version.
	void							base64_encode(std::string & strEncoded, unsigned char const* pDecodedData, unsigned int lngDecoded);

	// Nice and simple, the std::string encoder
	std::string				base64_encode(const std::string &sString);



	// decoders --------------------------

	// Decodes the input buffer up to lngEncoded bytes and stores the output up to lngDecodedMax in pDecodedData and returns the length of the data stored in pDecodedData
	int base64_decode(const unsigned char * pEncodedData, int lngEncoded, unsigned char * pDecodedData, int lngDecodedMax);

	// Decodes the input buffer stores the output in the stringstream (should be binary if data is expected to be binary)
	int base64_decode(const unsigned char * pEncodedData, int lngEncoded, std::stringstream& sstream);

	// Translator for the above
	inline int base64_decode(const std::string& strEncodedData, std::stringstream& sstream)
	{
		return base64_decode((const unsigned char *) strEncodedData.c_str(), (int) strEncodedData.size(), sstream);
	}

	// Returns a buffer containing the decoded data. The length of the buffer is returned in lngDecoded.
	// The buffer must be deleted by the client when done.
	unsigned char *		base64_decode(const unsigned char * pEncodedData, int lngEncoded, int & lngDecoded);

	// Returns a buffer containing the decoded data. The length of the buffer is returned in lngDecoded.
	// The buffer must be deleted by the client when done.
	inline unsigned char *		base64_decode(const std::string & strEncodedData, int & lngDecoded)
	{
		return base64_decode((const unsigned char*) strEncodedData.data(), strEncodedData.size(), lngDecoded);
	}

	// Nice and simple, the std::string decoder
	std::string				base64_decode(const std::string &sString);



	// Files -----------------------------
	// and something using files
	void							base64_encode(const std::string & FileName, const std::string & Path, std::string & strEncodedData);

	// XOR encrypt a the plain text password passed in using the encolsed key (or the default key if the enclosed key is empty) and return it base64_encoded
	std::string				encryptPassword(const std::string & strPlainPwd, const std::string & strKey = "");

	// base64_decode the encrypted password passed in and return it XOR decrypted it using the encolsed key (or the default key if the enclosed key is empty) and return the plain text
	std::string				decryptPassword(const std::string & strEncryptedPwd, const std::string & strKey = "");
}


#endif // #ifndef __APALUtil_Codec_H__
