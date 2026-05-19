#include "sourcefile.h"
#include "CommonFunc.h"
#include "lexer.h"
#include <string.h>
#include <windows.h>
#define       MAX_LINE_SIZE         (4096)
using namespace std;

// Check if a byte buffer is valid UTF-8
static bool IsValidUTF8(const unsigned char* data, int len)
{
	int i = 0;
	while (i < len)
	{
		if (data[i] <= 0x7F) { i++; continue; }
		int seqLen = 0;
		if ((data[i] & 0xE0) == 0xC0) seqLen = 2;
		else if ((data[i] & 0xF0) == 0xE0) seqLen = 3;
		else if ((data[i] & 0xF8) == 0xF0) seqLen = 4;
		else return false; // Invalid lead byte
		if (i + seqLen > len) return false;
		for (int j = 1; j < seqLen; j++)
		{
			if ((data[i + j] & 0xC0) != 0x80) return false;
		}
		i += seqLen;
	}
	return true;
}

// Convert GBK string to UTF-8 string
static std::string GBKToUTF8(const char* gbkStr, int gbkLen)
{
	// GBK -> UTF-16
	int wLen = MultiByteToWideChar(CP_ACP, 0, gbkStr, gbkLen, NULL, 0);
	if (wLen == 0) return std::string(gbkStr, gbkLen);
	wchar_t* wBuf = new wchar_t[wLen];
	MultiByteToWideChar(CP_ACP, 0, gbkStr, gbkLen, wBuf, wLen);
	// UTF-16 -> UTF-8
	int u8Len = WideCharToMultiByte(CP_UTF8, 0, wBuf, wLen, NULL, 0, NULL, NULL);
	if (u8Len == 0) { delete[] wBuf; return std::string(gbkStr, gbkLen); }
	char* u8Buf = new char[u8Len];
	WideCharToMultiByte(CP_UTF8, 0, wBuf, wLen, u8Buf, u8Len, NULL, NULL);
	std::string result(u8Buf, u8Len);
	delete[] wBuf;
	delete[] u8Buf;
	return result;
}

CSourceFile::CSourceFile(void)
{
	Reset();
}

CSourceFile::~CSourceFile(void)
{
	for (int i = 0; i < (int)mSourceFileDataList.size(); i++)
	{
		if(mSourceFileDataList[i].pSourceCode != NULL)
			delete mSourceFileDataList[i].pSourceCode;
	}
}
void CSourceFile::Reset()
{
	m_iCurChar = 0;
	m_iCurLine = 0;
}
bool   CSourceFile::LoadSourceFile(const char* pFileName)
{
	m_iMaxLine = 0;
	bool bSucc = LinkSourceFile(pFileName);
	return bSucc;
}

std::vector<std::string> Split(const std::string& src, char key)
{
	std::vector<std::string> result;

	std::string temp(src);
	while (true)
	{
		size_t pos = temp.find(key);
		if (pos == std::string::npos)
			break;

		result.push_back(temp.substr(0, pos));
		temp.erase(0, pos + 1);
	}

	if (!temp.empty())
		result.push_back(temp);

	return result;
}

void  CSourceFile::LoadFromString(const std::string& str)
{
	m_iMaxLine = 0;
	std::vector<std::string> lineVec = Split(str, '\n');
	for (int i = 0; i < (int)lineVec.size(); i++)
	{
		LineSourceCode lineCode;
		int iLen = (int)lineVec[i].length();
		lineCode.pSourceCode = new char[iLen + 1];
		strncpy_s(lineCode.pSourceCode, iLen + 1, lineVec[i].c_str(), iLen + 1);
		lineCode.iLength = iLen;
		mSourceFileDataList.push_back(lineCode);
		m_iMaxLine++;
	}
}

bool  CSourceFile::LinkSourceFile(const char* pFileName)
{
	FILE *fp = NULL;
	fopen_s(&fp, pFileName, "rb");
	if (fp == NULL)
		return false;

	// Read entire file into memory to detect encoding
	fseek(fp, 0, SEEK_END);
	long fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	char* fileBuf = new char[fileSize + 1];
	size_t bytesRead = fread(fileBuf, 1, fileSize, fp);
	fileBuf[bytesRead] = '\0';
	fclose(fp);

	// Skip UTF-8 BOM if present
	char* pContent = fileBuf;
	int contentLen = (int)bytesRead;
	if (contentLen >= 3 && (unsigned char)pContent[0] == 0xEF
		&& (unsigned char)pContent[1] == 0xBB
		&& (unsigned char)pContent[2] == 0xBF)
	{
		pContent += 3;
		contentLen -= 3;
	}

	// Detect encoding: check if content is valid UTF-8
	// If it contains high bytes but is not valid UTF-8, assume GBK and convert
	bool hasHighByte = false;
	for (int i = 0; i < contentLen; i++)
	{
		if ((unsigned char)pContent[i] >= 0x80) { hasHighByte = true; break; }
	}

	std::string utf8Content;
	if (hasHighByte && !IsValidUTF8((const unsigned char*)pContent, contentLen))
	{
		// GBK detected, convert to UTF-8
		utf8Content = GBKToUTF8(pContent, contentLen);
	}
	else
	{
		// Already UTF-8 (or pure ASCII)
		utf8Content.assign(pContent, contentLen);
	}
	delete[] fileBuf;

	// Split into lines and store
	const char* pData = utf8Content.c_str();
	int totalLen = (int)utf8Content.length();
	int lineStart = 0;
	for (int i = 0; i <= totalLen; i++)
	{
		if (i == totalLen || pData[i] == '\n')
		{
			int lineLen = i - lineStart;
			int extraLength = pData[i] == '\n' ? 1 : 0;
			// Remove trailing \r if present
			if (lineLen > 0 && pData[lineStart + lineLen - 1] == '\r')
			{ 
				lineLen--;
				extraLength++;
			}
			// Include the \n character for compatibility with existing lexer logic
			// The lexer expects lines to end with \n
			if (lineLen + extraLength > 0 || i < totalLen)
			{
				// Allocate line with trailing \n
				char* pLine = new char[lineLen + 2]; // +1 for \n, +1 for \0
				memcpy(pLine, pData + lineStart, lineLen);
				pLine[lineLen] = '\n';
				pLine[lineLen + 1] = '\0';

				LineSourceCode lineCode;
				lineCode.pSourceCode = pLine;
				lineCode.iLength = lineLen + 1; // includes \n
				lineCode.iRealLength = lineLen + extraLength;
				mSourceFileDataList.push_back(lineCode);
				m_iMaxLine++;
			}
			lineStart = i + 1;
		}
	}

	return true;
}


char  CSourceFile::GetNextChar()
{
	if (m_iCurLine < (int)mSourceFileDataList.size())
	{
		LineSourceCode line = mSourceFileDataList[m_iCurLine];
		if (m_iCurChar >= line.iLength)
		{
			if (m_iCurLine == m_iMaxLine - 1)
			{
				return '\0';
			}
			else
			{
				m_iCurChar = 0;
				m_iCurLine++;
			}
		}
		return mSourceFileDataList[m_iCurLine].pSourceCode[m_iCurChar++];
	}
	else
	{
		return '\0';
	}
}

char  CSourceFile::LookNextChar()
{
	int curCharIndex =  m_iCurChar;
	int curLineIndex = m_iCurLine;
	LineSourceCode line = mSourceFileDataList[m_iCurLine];
	if (curCharIndex >= line.iLength)
	{
		if(curLineIndex == m_iMaxLine - 1)
		{
			return '\0';
		}
		else
		{
			curCharIndex = 0;
			curLineIndex ++;
		}
	}
	return mSourceFileDataList[curLineIndex].pSourceCode[curCharIndex];
}

void  CSourceFile::ReWindChar()
{
	if (m_iCurChar == 0)
	{
		m_iCurLine --;
		m_iCurChar = mSourceFileDataList[m_iCurLine].iLength -1;
	}
	else
		m_iCurChar--;
}
void  CSourceFile::SetCurChar(char c)
{
	mSourceFileDataList[m_iCurLine].pSourceCode[m_iCurChar] = c;
}


inline const char * CSourceFile::GetNextLine()
{
	if (m_iCurLine >= m_iMaxLine)
		return NULL;
	return mSourceFileDataList[m_iCurLine++].pSourceCode;
}

int	 CSourceFile::GetCurReadedLen()
{
	int totalLength = 0;
	for (int i = 0; i < m_iCurLine; i++)
		totalLength += mSourceFileDataList[i].iRealLength;
	totalLength += m_iCurChar;
	return totalLength;
}

