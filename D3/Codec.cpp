#include "Codec.h"
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdexcept>

using namespace std;

//                                          1         2         3         4         5         6
//                                0123456789012345678901234567890123456789012345678901234567890123
static const char base64_set[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


namespace APALUtil
{
	// base64

	void base64_encode(const std::string& strFileName, const std::string& strPath, std::string & encoded)
	{
		string str;
		unsigned char *pBuffer = NULL;
		int iPos = 0;
		string strFullFileName = strPath + strFileName;

		struct stat filestat;
		if(::stat(strFullFileName.c_str(), &filestat) == 0)
		{
			pBuffer = new unsigned char[filestat.st_size];
		}

		if(!pBuffer)return;

		FILE *fp = ::fopen(strFullFileName.c_str(), "rb");

		if(fp)
		{
			unsigned char buffer[1025];

			int nRead = 0;
			while(nRead = ::fread(buffer, 1, 1024, fp))
			{
				::memcpy(pBuffer + iPos, &buffer[0], nRead);
				iPos += nRead;
			}
			::fclose(fp);
		}

		base64_encode(encoded, pBuffer, iPos);
		if(pBuffer)delete[] pBuffer;

	}





	void base64_encode(std::string & encoded, unsigned const char* buffer, unsigned int length)
	{
		const int MaxLine = 76;

		if (length < 1)
		{
			encoded = "";
			return;
		}

		try
		{
			// allocate - using 6 bits per byte to be encoded => 8/6 + padding
			unsigned int EncLength = 0;
			encoded.resize(length * 8 / 6 + 6);

			unsigned int x, y = 0, i, flag = 0, m = 0, n = 3;
			unsigned char triple[3], quad[4];

			for(x = 0; x < length; x = x + 3)
			{
				if((length - x) / 3 == 0) n = (length - x) % 3;

				for(i=0; i < 3; i++) triple[i] = 0;
				for(i=0; i < n; i++) triple[i] = buffer[x + i];
				quad[0] = base64_set[(triple[0] & 0xfc) >> 2]; // FC = 11111100
				quad[1] = base64_set[((triple[0] & 0x03) << 4) | ((triple[1] & 0xf0) >> 4)]; // 03 = 11
				quad[2] = base64_set[((triple[1] & 0x0f) << 2) | ((triple[2] & 0xc0) >> 6)]; // 0F = 1111, C0=11110
				quad[3] = base64_set[triple[2] & 0x3f]; // 3F = 111111
				if(n < 3) quad[3] = '=';
				if(n < 2) quad[2] = '=';
				for(i=0; i < 4; i++)
				{
					encoded[y + i] = quad[i];
					EncLength = y + i;
				}
				y = y + 4;
				m = m + 4;
			}

			encoded = encoded.substr(0, EncLength + 1);

		}
		catch(...)
		{
			encoded = "";
		}

	}

	inline unsigned char Decode(char c)
	{
		if (c >= 'A' && c <= 'Z')return c - 'A';
		if (c >= 'a' && c <= 'z')return c - 'a' + 26;
		if (c >= '0' && c <= '9')return c - '0' + 52;
		if (c == '+')return 62;
		return 63;
	};

/*
	unsigned char * base64_decode(string & source, int & length)
	{
		int len = source.length();
		int iPos = 0;
		unsigned char *pBuffer = NULL;

		try
		{
			pBuffer = new unsigned char[len];

			for (int i = 0; i < len; i += 4)
			{
				char c1='A', c2='A', c3='A', c4='A';

				c1 = source[i];
				if (i+1 < len)c2 = source[i + 1];
				if (i+2 < len)c3 = source[i + 2];
				if (i+3 < len)c4 = source[i + 3];

				unsigned char byte1=0, byte2=0, byte3=0, byte4=0;
				byte1 = Decode(c1);
				byte2 = Decode(c2);
				byte3 = Decode(c3);
				byte4 = Decode(c4);

				pBuffer[iPos++] = (byte1 << 2) | (byte2 >> 4);

				if (c3 != '=')
				{
					pBuffer[iPos++] =  ((byte2 & 0xf) << 4)|(byte3 >> 2);
				}

				if (c4 != '=')
				{
					pBuffer[iPos++] = ((byte3 & 0x3) << 6)| byte4 ;
				}
			}

			length = iPos;
			pBuffer[length] = 0;

			return pBuffer;

		}
		catch(...)
		{
			length = 0;
			return NULL;
		}

	}
*/




	int base64_decode(const unsigned char * pEncodedData, int lngEncoded, unsigned char * pDecodedData, int lngDecodedMax)
	{
		char *pszOverflowErr = "APALUtil::base64_decode(): output buffer is too small)";
		int iPos = 0;

		for (int i = 0; i < lngEncoded; i += 4)
		{
			char c1='A', c2='A', c3='A', c4='A';

			c1 = pEncodedData[i];
			if (i+1 < lngEncoded)c2 = pEncodedData[i + 1];
			if (i+2 < lngEncoded)c3 = pEncodedData[i + 2];
			if (i+3 < lngEncoded)c4 = pEncodedData[i + 3];

			unsigned char byte1=0, byte2=0, byte3=0, byte4=0;
			byte1 = Decode(c1);
			byte2 = Decode(c2);
			byte3 = Decode(c3);
			byte4 = Decode(c4);

			if (iPos >= lngDecodedMax) throw std::runtime_error(pszOverflowErr);
			pDecodedData[iPos++] = (byte1 << 2) | (byte2 >> 4);

			if (c3 != '=')
			{
				if (iPos >= lngDecodedMax) throw std::runtime_error(pszOverflowErr);
				pDecodedData[iPos++] =  ((byte2 & 0xf) << 4)|(byte3 >> 2);
			}

			if (c4 != '=')
			{
				if (iPos >= lngDecodedMax) throw std::runtime_error(pszOverflowErr);
				pDecodedData[iPos++] = ((byte3 & 0x3) << 6)| byte4 ;
			}
		}

		if (iPos < lngDecodedMax)
			pDecodedData[iPos] = 0;

		return iPos;
	}



	int base64_decode(const unsigned char * pEncodedData, int lngEncoded, std::stringstream & sstream)
	{
		char *pszOverflowErr = "APALUtil::base64_decode(): output buffer is too small)";
		int iLen = 0;

		for (int i = 0; i < lngEncoded; i += 4)
		{
			char c1='A', c2='A', c3='A', c4='A';

			c1 = pEncodedData[i];
			if (i+1 < lngEncoded)c2 = pEncodedData[i + 1];
			if (i+2 < lngEncoded)c3 = pEncodedData[i + 2];
			if (i+3 < lngEncoded)c4 = pEncodedData[i + 3];

			unsigned char byte1=0, byte2=0, byte3=0, byte4=0;
			byte1 = Decode(c1);
			byte2 = Decode(c2);
			byte3 = Decode(c3);
			byte4 = Decode(c4);

			sstream << char((byte1 << 2) | (byte2 >> 4));
			iLen++;

			if (c3 != '=')
			{
				sstream << char(((byte2 & 0xf) << 4)|(byte3 >> 2));
				iLen++;
			}

			if (c4 != '=')
			{
				sstream << char(((byte3 & 0x3) << 6)| byte4);
				iLen++;
			}
		}

		return iLen;
	}



	unsigned char * base64_decode(const unsigned char * pEncodedData, int lngEncoded, int & lngDecoded)
	{
		int iPos = 0;
		unsigned char *pBuffer = NULL;

		try
		{
			pBuffer = new unsigned char[lngEncoded];

			for (int i = 0; i < lngEncoded; i += 4)
			{
				char c1='A', c2='A', c3='A', c4='A';

				c1 = pEncodedData[i];
				if (i+1 < lngEncoded)c2 = pEncodedData[i + 1];
				if (i+2 < lngEncoded)c3 = pEncodedData[i + 2];
				if (i+3 < lngEncoded)c4 = pEncodedData[i + 3];

				unsigned char byte1=0, byte2=0, byte3=0, byte4=0;
				byte1 = Decode(c1);
				byte2 = Decode(c2);
				byte3 = Decode(c3);
				byte4 = Decode(c4);

				pBuffer[iPos++] = (byte1 << 2) | (byte2 >> 4);

				if (c3 != '=')
				{
					pBuffer[iPos++] =  ((byte2 & 0xf) << 4)|(byte3 >> 2);
				}

				if (c4 != '=')
				{
					pBuffer[iPos++] = ((byte3 & 0x3) << 6)| byte4 ;
				}
			}

			lngDecoded = iPos;
			pBuffer[lngDecoded] = 0;

			return pBuffer;

		}
		catch(...)
		{
		}

		delete pBuffer;
		lngDecoded = 0;

		return NULL;
	}





	// ####################################################################### //
	// # base64_encode()                                                    # //
	// ####################################################################### //
	/** Encodes the specified string to base64

			@param  sString  String to encode
			@return Base64 encoded string
	*/
	std::string base64_encode(const std::string &sString) {
		static const std::string sBase64Table(
		// 0000000000111111111122222222223333333333444444444455555555556666
		// 0123456789012345678901234567890123456789012345678901234567890123
			"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
		);
		static const char cFillChar = '=';
		std::string::size_type   nLength = sString.length();
		std::string              sResult;

		// Allocate memory for the converted string
		sResult.reserve(nLength * 8 / 6 + 1);

		for(std::string::size_type nPos = 0; nPos < nLength; nPos++) {
			char cCode;

			// Encode the first 6 bits
			cCode = (sString[nPos] >> 2) & 0x3f;
			sResult.append(1, sBase64Table[cCode]);

			// Encode the remaining 2 bits with the next 4 bits (if present)
			cCode = (sString[nPos] << 4) & 0x3f;
			if(++nPos < nLength)
				cCode |= (sString[nPos] >> 4) & 0x0f;
			sResult.append(1, sBase64Table[cCode]);

			if(nPos < nLength) {
				cCode = (sString[nPos] << 2) & 0x3f;
				if(++nPos < nLength)
					cCode |= (sString[nPos] >> 6) & 0x03;

				sResult.append(1, sBase64Table[cCode]);
			} else {
				++nPos;
				sResult.append(1, cFillChar);
			}

			if(nPos < nLength) {
				cCode = sString[nPos] & 0x3f;
				sResult.append(1, sBase64Table[cCode]);
			} else {
				sResult.append(1, cFillChar);
			}
		}

		return sResult;
	}



	// ####################################################################### //
	// # base64_decode()                                                    # //
	// ####################################################################### //
	/** Decodes the specified base64 string

			@param  sString  Base64 string to decode
			@return Decoded string
	*/
	std::string base64_decode(const std::string &sString) {
		static const std::string::size_type np = std::string::npos;
		static const std::string::size_type DecodeTable[] = {
		// 0   1   2   3   4   5   6   7   8   9
			np, np, np, np, np, np, np, np, np, np,  //   0 -   9
			np, np, np, np, np, np, np, np, np, np,  //  10 -  19
			np, np, np, np, np, np, np, np, np, np,  //  20 -  29
			np, np, np, np, np, np, np, np, np, np,  //  30 -  39
			np, np, np, 62, np, np, np, 63, 52, 53,  //  40 -  49
			54, 55, 56, 57, 58, 59, 60, 61, np, np,  //  50 -  59
			np, np, np, np, np,  0,  1,  2,  3,  4,  //  60 -  69
			 5,  6,  7,  8,  9, 10, 11, 12, 13, 14,  //  70 -  79
			15, 16, 17, 18, 19, 20, 21, 22, 23, 24,  //  80 -  89
			25, np, np, np, np, np, np, 26, 27, 28,  //  90 -  99
			29, 30, 31, 32, 33, 34, 35, 36, 37, 38,  // 100 - 109
			39, 40, 41, 42, 43, 44, 45, 46, 47, 48,  // 110 - 119
			49, 50, 51, np, np, np, np, np, np, np,  // 120 - 129
			np, np, np, np, np, np, np, np, np, np,  // 130 - 139
			np, np, np, np, np, np, np, np, np, np,  // 140 - 149
			np, np, np, np, np, np, np, np, np, np,  // 150 - 159
			np, np, np, np, np, np, np, np, np, np,  // 160 - 169
			np, np, np, np, np, np, np, np, np, np,  // 170 - 179
			np, np, np, np, np, np, np, np, np, np,  // 180 - 189
			np, np, np, np, np, np, np, np, np, np,  // 190 - 199
			np, np, np, np, np, np, np, np, np, np,  // 200 - 209
			np, np, np, np, np, np, np, np, np, np,  // 210 - 219
			np, np, np, np, np, np, np, np, np, np,  // 220 - 229
			np, np, np, np, np, np, np, np, np, np,  // 230 - 239
			np, np, np, np, np, np, np, np, np, np,  // 240 - 249
			np, np, np, np, np, np                   // 250 - 256
		};
		static const char cFillChar = '=';

		std::string::size_type nLength = sString.length();
		std::string            sResult;

		sResult.reserve(nLength);

		for(std::string::size_type nPos = 0; nPos < nLength; nPos++) {
			unsigned char c, c1;

			c = (char) DecodeTable[(unsigned char)sString[nPos]];
			nPos++;
			c1 = (char) DecodeTable[(unsigned char)sString[nPos]];
			c = (c << 2) | ((c1 >> 4) & 0x3);
			sResult.append(1, c);

			if(++nPos < nLength) {
				c = sString[nPos];
				if(cFillChar == c)
					break;

				c = (char) DecodeTable[(unsigned char)sString[nPos]];
				c1 = ((c1 << 4) & 0xf0) | ((c >> 2) & 0xf);
				sResult.append(1, c1);
			}

			if(++nPos < nLength) {
				c1 = sString[nPos];
				if(cFillChar == c1)
					break;

				c1 = (char) DecodeTable[(unsigned char)sString[nPos]];
				c = ((c << 6) & 0xc0) | c1;
				sResult.append(1, c);
			}
		}

		return sResult;
	}



	// shared key used by encryptPassword and decryptPassword
	char*			G_pszKey = "$%oR$-K12l&K";

	// XOR encrypt a the plain text password passed in and return it base64_encoded
	std::string encryptPassword(const std::string & strPlainPwd, const std::string & strKey)
	{
    string				strEncrypt = strPlainPwd;
		const char *	pszKey = strKey.empty() ? G_pszKey : strKey.c_str();
		unsigned int	iSize = strlen(pszKey);

    for (unsigned int i = 0; i < strEncrypt.size(); i++)
			strEncrypt[i] = strEncrypt[i] ^ pszKey[i % iSize];
    
    return base64_encode(strEncrypt);
	}


	// base64_decode the encrypted password passed in and return it XOR decrypted
	std::string decryptPassword(const std::string & strEncryptedPwd, const std::string & strKey)
	{
    string				strPlain = base64_decode(strEncryptedPwd);
		const char *	pszKey = strKey.empty() ? G_pszKey : strKey.c_str();
		unsigned int	iSize = strlen(pszKey);

    for (unsigned int i = 0; i < strPlain.size(); i++)
			strPlain[i] = strPlain[i] ^ pszKey[i % iSize];
    
    return strPlain;
	}
}
