#pragma once
#include <vector>
#include <string>
class CSourceFile
{
public:
	struct LineSourceCode
	{
		char *pSourceCode;
		int  iLength;
		int  iRealLength;
	};
private:
	int  m_iCurChar;
	int  m_iCurLine;
	int  m_iMaxLine;
	std::vector<LineSourceCode>   mSourceFileDataList; 

public:
	void  Reset();
	void  LoadFromString(const std::string& str);
	bool  LoadSourceFile(const char* pFileName);
	bool  LinkSourceFile( const char* pFileName);
	const char* GetNextLine();
	void  ReWindChar();
	char  GetNextChar();
	char  LookNextChar();
	void  SetCurChar(char c);
	inline int   GetCurCharIndex() {return m_iCurChar;}
	inline int   GetCurLineIndex(){return m_iCurLine;}
	inline void   SetCurLineIndex(int index) {m_iCurLine = index;}
	inline void   SetCurCharIndex(int index) {m_iCurChar = index;}

	int	 GetCurReadedLen();

public:
	CSourceFile(void);
public:
	~CSourceFile(void);
};
