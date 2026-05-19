#include "regex.h"


RegData::~RegData()
{
	if (subRegData != NULL)
	{
		delete subRegData;
	}
}

Regex::Regex(char* regexExpr)
{
	m_groupIncID = 0;
	m_rootRegexFactor = ParseExpr(regexExpr, false);
}

Regex::~Regex()
{
	if (m_rootRegexFactor != NULL)
		ClearRegFactor(m_rootRegexFactor);
}

void	Regex::ClearRegFactor(RegexFactor* factor)
{
	for (int i = 0; i < factor->subFactorSize; i++)
	{
		ClearRegFactor(factor->subFactor[i]);
	}

	delete factor;
}
void  Regex::GetNumMatchChar(int& numChar, RegexFactor* factor)
{
	if (factor->refGroupIndex >= 0)	
	{	
		numChar = m_matchGroupIDMap[factor->refGroupIndex].endPos - m_matchGroupIDMap[factor->refGroupIndex].startPos;	
	}	
	else if (factor->refGroupName[0] != 0)	
	{	
		numChar = m_matchGroupNameMap[factor->refGroupName].endPos - m_matchGroupNameMap[factor->refGroupName].startPos;	
	}
	else
	{
		numChar = factor->mContentSize;
		if (numChar < 1)
		{
			if (factor->mCharRangeVecSize > 0 && factor->m_posType == PMT_Normal)
				numChar = 1;
		}
	}
		
}

RegexFactor*	Regex::AddRegFactor(RegexFactor* lastExpr)
{
	RegexFactor* factor = new RegexFactor();
	factor->type = RFT_Normal;
	factor->lastFactor = lastExpr;
	factor->minRepeat = factor->maxRepeat = 1;

	AddSubFactor(lastExpr, factor);

	return factor;
}

void Regex::AddSubFactor(RegexFactor* lastExpr, RegexFactor* factor)
{
	lastExpr->subFactorSize++;
	RegexFactor** subFactor = new RegexFactor*[lastExpr->subFactorSize];
	if (lastExpr->subFactor != NULL)
		memcpy(subFactor, lastExpr->subFactor, sizeof(RegexFactor*) * (lastExpr->subFactorSize - 1));

	lastExpr->subFactor = subFactor;
	lastExpr->subFactor[lastExpr->subFactorSize - 1] = factor;
}

bool	Regex::ParseRange(char* &expr, int& start, int& end)
{
	char* saved = expr;
	expr++;
	start = 0;
	while (true)
	{
		if (*expr >= '0' && *expr <= '9')
		{
			start = start * 10 + (*expr - '0');
		}
		else if (*expr == ',')
		{
			expr++;
			break;
		}
		else if (*expr == '}')
		{
			end = start;
			return true;
		}
		else
		{
			expr = saved;
			return false;
		}

		expr++;
	}


	if (*expr == '}')
	{
		end = -1;
		return true;
	}
	else
	{
		end = 0;
		while (true)
		{
			if (*expr >= '0' && *expr <= '9')
			{
				end = end * 10 + (*expr - '0');
			}
			else if (*expr == '}')
			{
				return true;
			}
			else
			{
				expr = saved;
				return false;
			}

			expr++;
		}
	}

}

void	Regex::InsertAssertEnd(RegexFactor* regFactor)
{
	for (int i = 0; i < regFactor->subFactorSize; i++)
	{
		RegexFactor* startFactor = regFactor->subFactor[i];
		while (startFactor != NULL)
		{
			if (startFactor->subFactorSize > 0)
			{
				startFactor = startFactor->subFactor[0];
			}
			else
			{
				RegexFactor* endExpr = AddRegFactor(startFactor);
				endExpr->m_posType = PMT_EndAssert;
				break;
			}
		}

	
	}
}

void  Regex::ComputeCharactorRange(RegexFactor* regFactor , int& iMax, int& iMin)
{
	switch (regFactor->type)
	{
	case RFT_Start:
	{
		for (int i = 0; i < regFactor->subFactorSize; i++)
		{
			int subMinTotal = 0;
			int subMaxTotal = 0;
			RegexFactor* startFactor = regFactor->subFactor[i];
			while (startFactor != NULL)
			{
				int subMin = 0;
				int subMax = 0;
				ComputeCharactorRange(startFactor, subMax, subMin);
				subMinTotal += subMin;
				if (subMaxTotal >= 0)
				{
					if (subMax >= 0)
						subMaxTotal += subMax;
					else
						subMaxTotal = -1;
				}

				if (startFactor->subFactorSize > 0)
				{
					startFactor = startFactor->subFactor[0];
				}
				else
				{
					startFactor = NULL;
				}
			}

			if (i == 0)
			{
				iMin = subMinTotal;
				iMax = subMaxTotal;
			}
			else
			{
				if (subMinTotal < iMin)
					iMin = subMinTotal;
				if (subMaxTotal < 0 || iMax < 0)
				{
					iMax = -1;
				}
				else if (subMaxTotal > iMax)
				{
					iMax = subMaxTotal;
				}
			}

		}
	}
	break;
	case RFT_Normal:
	{
		if (regFactor->refGroupIndex > 0)
		{
			if (m_groupIDExprMap.find(regFactor->refGroupIndex) != m_groupIDExprMap.end())
			{
				iMin = m_groupIDExprMap[regFactor->refGroupIndex]->mMinCharactor * regFactor->minRepeat;

				if (regFactor->maxRepeat < MaxRegRepeat)
				{
					if (m_groupIDExprMap[regFactor->refGroupIndex]->mMaxCharactor < 0)
					{
						iMax = -1;
					}
					else
					{
						iMax = regFactor->maxRepeat * m_groupIDExprMap[regFactor->refGroupIndex]->mMaxCharactor;
					}
					
				}
				else
					iMax = -1;
			}
		}
		else if (regFactor->refGroupName[0] != '\0')
		{
			if (m_groupNameExprMap.find(regFactor->refGroupName) != m_groupNameExprMap.end())
			{
				iMin = m_groupNameExprMap[regFactor->refGroupName]->mMinCharactor * regFactor->minRepeat;

				if (regFactor->maxRepeat < MaxRegRepeat)
				{
					if (m_groupNameExprMap[regFactor->refGroupName]->mMaxCharactor < 0)
					{
						iMax = -1;
					}
					else
					{
						iMax = regFactor->maxRepeat * m_groupNameExprMap[regFactor->refGroupName]->mMaxCharactor;
					}

				}
				else
					iMax = -1;
			}
		}
		else
		{
			if (regFactor->mContentSize > 0)
			{
				iMin = regFactor->minRepeat * regFactor->mContentSize;
				if (regFactor->maxRepeat < MaxRegRepeat)
					iMax = regFactor->maxRepeat * regFactor->mContentSize;
				else
					iMax = -1;
			}
			else if (regFactor->mCharRangeVecSize > 0)
			{
				iMin = regFactor->minRepeat;
				if (regFactor->maxRepeat < MaxRegRepeat)
					iMax = regFactor->maxRepeat;
				else
					iMax = -1;
			}
			else
			{
				iMin = 0;
				iMax = 0;
			}
		}
	}
	break;
	case RFT_Group:
	{
		if (regFactor->mAssertType == AT_Normal)
		{
			iMin = regFactor->mMinCharactor * regFactor->minRepeat;

			if (regFactor->maxRepeat < MaxRegRepeat)
			{
				if (regFactor->mMaxCharactor < 0)
				{
					iMax = -1;
				}
				else
				{
					iMax = regFactor->maxRepeat * regFactor->mMaxCharactor;
				}

			}
			else
				iMax = -1;
		}
		else
		{
			iMin = 0;
			iMax = 0;
		}
	}
	break;
	default:
		break;
	}
}


RegexFactor*	Regex::ParseExpr(char* &p, bool hasParent)
{
	RegexFactor* startExpr = new RegexFactor();
	startExpr->type = RFT_Start;

	RegexFactor* lastExpr = startExpr;

	while (true)
	{
		switch (*p)
		{
		case '\\':
		{
			if (*(p + 1) != 0)
			{
				switch (*(p + 1))
				{
				case 'b':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->m_posType = PMT_Word;
					break;
				case 'B':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->m_posType = PMT_Not_Word;
					break;
				case 'd':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Number, MCT_Number));
					break;
				case 'D':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Not_Number, MCT_Not_Number));
					break;
				case 's':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Space, MCT_Space));
					break;
				case 'S':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Not_Space, MCT_Not_Space));
					break;
				case 'w':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Char, MCT_Char));
					break;
				case 'W':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Not_Char, MCT_Not_Char));
					break;
				case 'n':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange('\n', '\n'));
					break;
				case 'r':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange('\r', '\r'));
					break;
				case 't':
					lastExpr = AddRegFactor(lastExpr);
					lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange('\t', '\t'));
					break;
				default:
					{
						lastExpr = AddRegFactor(lastExpr);
						if (*(p + 1) >= '0' && *(p + 1) <= '9')
						{
							lastExpr->refGroupIndex = (*(p + 1) - '0');
							if (lastExpr->refGroupIndex >= m_groupIncID 
								|| m_groupIDExprMap.find(lastExpr->refGroupIndex) == m_groupIDExprMap.end())
							{
								goto failure;
							}
						}
						else if (*(p + 1) == 'k' && *(p + 2) == '<')
						{
							p += 3;

							std::string groupName;
							while (1)
							{
								if (*p == '>')
								{
									p++;
									break;
								}
								else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '-' || *p == '_')
								{
									groupName.push_back(*p);
								}
								else
									goto failure;

								p++;
							}

							if (groupName.empty())
							{
								goto failure;
							}

							if (m_groupNameExprMap.find(groupName) == m_groupNameExprMap.end())
							{
								goto failure;
							}

							p -= 2;

							if (groupName.size() < MaxGroupNameSize)
							{
								strncpy(lastExpr->refGroupName, groupName.c_str(), MaxGroupNameSize);
							}
							else
							{
								goto failure;
							}
						
						}
						else
						{
							lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(*(p + 1), *(p + 1)));
						}
					}
				break;
					
					
				}

				p += 2;
			}
			else
			{
				goto failure;
			}
		}
		break;
		case '.':
		{
			lastExpr = AddRegFactor(lastExpr);
			lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(MCT_Point, MCT_Point));
			p++;
		}
		break;
		case '$':
		{
			lastExpr = AddRegFactor(lastExpr);
			lastExpr->m_posType = PMT_End;
			p++;
		}
		break;
		case '^':
		{
			lastExpr = AddRegFactor(lastExpr);
			lastExpr->m_posType = PMT_Start;
			p++;
		}
		break;
		case '{':
		{
			int start, end;
			bool isRange = ParseRange(p, start, end);
			if (isRange)
			{
				if (lastExpr == startExpr || lastExpr->mAssertType != AT_Normal)
				{
					goto failure;
				}
				else
				{
					lastExpr->maxRepeat = end;
					lastExpr->minRepeat = start;

					if (*(p + 1) == '?')
					{
						lastExpr->isGreedy = false;
						p++;
					}
				}
			}
			else
			{
				lastExpr = AddRegFactor(lastExpr);
				lastExpr->mCharRangeVec[lastExpr->mCharRangeVecSize++] = (CharRange(*p, *p));
			}

			p++;
		}
		break;
		case '[':
		{
			p++;
			if (*p == 0)
				goto failure;

			RegexFactor* factor = AddRegFactor(lastExpr);
			bool isNot = false;
			if (*p == '^')
			{
				factor->isNot = true;
				p++;
			}

			while (true)
			{
				if (*p == 0)
				{
					goto failure;
				}
				else if (*p == ']')
				{
					if (factor->mCharRangeVecSize == 0)
					{
						goto failure;
					}
					else
					{
						p++;
						break;
					}
				}
				else
				{
					if (factor->mCharRangeVecSize >= MaxCharRangeSize)
					{
						goto failure;
					}

					if (*p == '\\')
					{
						if (*(p + 1) == '\0')
							goto failure;

						switch (*(p + 1))
						{
						case 'd':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(MCT_Number, MCT_Number));
							break;
						case 'D':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(MCT_Not_Number, MCT_Not_Number));
							break;
						case 's':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(MCT_Space, MCT_Space));
							break;
						case 'S':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(MCT_Not_Space, MCT_Not_Space));
							break;
						case 'w':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(MCT_Char, MCT_Char));
							break;
						case 'W':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(MCT_Not_Char, MCT_Not_Char));
							break;
						case 'n':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange('\n', '\n'));
							break;
						case 'r':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange('\r', '\r'));
							break;
						case 't':
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange('\t', '\t'));
							break;
						default:
							factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(*(p + 1), *(p + 1)));
							break;
						}

						p += 2;
					}
					else  if (*(p + 1) == '-' && *(p + 2) != ']' && *(p + 2) != 0)
					{
						factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(*p, *(p + 2)));
						p += 3;
					}
					else
					{
						factor->mCharRangeVec[factor->mCharRangeVecSize++] = (CharRange(*p, *p));
						p++;
					}
				}
			}

			lastExpr = factor;
		}
		break;
		case '|':
		{
			if (lastExpr == startExpr)
			{
				goto failure;
			}
			else
			{
				lastExpr = startExpr;
			}
			p++;
		}
		break;
		case '(':
		{
			p++;

			std::string groupName;
			if (*p == '?' && *(p + 1) == '<' && *(p + 2) != '=' && *(p + 2) != '!')
			{
				p += 2;
				while (1)
				{
					if (*p == '>')
					{
						p++;
						break;
					}
					else if ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') || *p == '-' || *p == '_')
					{
						groupName.push_back(*p);
					}
					else
						goto failure;

					p++;
				}

				if (groupName.size() == 0)
				{
					goto failure;
				}
					
			}

			int groupID = -1;
			if (groupName.empty())
			{
				groupID = m_groupIncID++;
			}

			
			RegexFactor*  subFactor = ParseExpr(p, true);
			if (subFactor == NULL)
				goto failure;

			if (subFactor->mAssertType == AT_NegativeLookBehind || subFactor->mAssertType == AT_PositiveLookBehind)
			{
				InsertAssertEnd(subFactor);
			}

			RegexFactor* groupFactor = new RegexFactor();
			groupFactor->type = RFT_Group;
			groupFactor->startFactor = subFactor;
			groupFactor->groupName = groupName;
			groupFactor->mAssertType = subFactor->mAssertType;
			if (groupName.empty())
			{
				groupFactor->groupIndex = groupID;
				m_groupIDExprMap[groupID] = groupFactor;
			}
			else
			{
				m_groupNameExprMap[groupName] = groupFactor;
			}

			int iMax = 0, iMin = 0;


			ComputeCharactorRange(subFactor, iMax, iMin);
			if (iMax < 0 && (subFactor->mAssertType == AT_NegativeLookBehind || subFactor->mAssertType == AT_PositiveLookBehind))
			{
				goto failure;
			}
			
			groupFactor->mMaxCharactor = iMax;
			groupFactor->mMinCharactor = iMin;

			AddSubFactor(lastExpr, groupFactor);
			lastExpr = groupFactor;
		}
		break;
		case ')':
		{
			if (hasParent)
			{
				p++;
				return startExpr;
			}
			else
			{
				goto failure;
			}
		}
		break;
		case '+':
		{
			if (lastExpr == startExpr || lastExpr->mAssertType != AT_Normal)
				goto failure;
			else
			{
				lastExpr->minRepeat = 1;
				lastExpr->maxRepeat = MaxRegRepeat;
				if (*(p + 1) == '?')
				{
					lastExpr->isGreedy = false;
					p++;
				}
			}

			p++;
		}
		break;
		case '*':
		{
			if (lastExpr == startExpr || lastExpr->mAssertType != AT_Normal)
				goto failure;
			else
			{
				lastExpr->minRepeat = 0;
				lastExpr->maxRepeat = MaxRegRepeat;
				if (*(p + 1) == '?')
				{
					lastExpr->isGreedy = false;
					p++;
				}
			}

			p++;
		}
		break;
		case '?':
		{
			if (lastExpr == startExpr)
			{
				if (*(p + 1) == '=')
				{
					lastExpr->mAssertType = AT_PositiveLookAhead;
					p++;
				}
				else if (*(p + 1) == '!')
				{
					lastExpr->mAssertType = AT_NegativeLookAhead;
					p++;
				}
				else if (*(p + 1) == '<' && *(p + 2) == '=')
				{
					lastExpr->mAssertType = AT_PositiveLookBehind;
					p += 2;
				}
				else if (*(p + 1) == '<' && *(p + 2) == '!')
				{
					lastExpr->mAssertType = AT_NegativeLookBehind;
					p += 2;
				}
				else
					goto failure;
			}
			else if (lastExpr->mAssertType != AT_Normal)
			{
				goto failure;
			}
			else
			{
				lastExpr->minRepeat = 0;
				lastExpr->maxRepeat = 1;
				if (*(p + 1) == '?')
				{
					lastExpr->isGreedy = false;
					p++;
				}
			}

			p++;
		}
		break;
		case '\0':
		{
			if (hasParent)
			{
				goto failure;
			}
			else
			{
				return startExpr;
			}
		}
		default:

			bool hasLimit = *(p + 1) == '{' || *(p + 1) == '?' || *(p + 1) == '+' || *(p + 1) == '*';

			if (lastExpr->mContentSize > 0 && lastExpr->mContentSize < MaxContentSize -1 && !hasLimit && (lastExpr->minRepeat == lastExpr->maxRepeat) && lastExpr->minRepeat == 1)
			{
				lastExpr->mContent[lastExpr->mContentSize] = *p;
				lastExpr->mContentSize++;
			}
			else
			{
				lastExpr = AddRegFactor(lastExpr);
				lastExpr->mContent[0] = (*p);
				lastExpr->mContentSize = 1;
			}

			p++;
			break;
		}
	}


failure:
	ClearRegFactor(startExpr);
	return NULL;
}


#define  IsWordBound(c) !((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_')

inline bool	Regex::CanMatch(const RegexFactor* factor, char* c, int index, int& num, int endIndex)
{
	if (factor->refGroupIndex >= 0)
	{
		std::map<int, MatchGroupData>::iterator it = m_matchGroupIDMap.find(factor->refGroupIndex);
		if (it != m_matchGroupIDMap.end())
		{
			num = it->second.endPos - it->second.startPos;
			if (num == 0)
				return true;
			if (index == endIndex)
				return false;
			return strncmp(c + it->second.startPos, c + index, num) == 0;
		}
		else
		{
			num = 0;
			return true;
		}
	}
	else if (factor->refGroupName[0] != 0)
	{
		std::map<std::string, MatchGroupData>::iterator it = m_matchGroupNameMap.find(factor->refGroupName);
		if (it != m_matchGroupNameMap.end())
		{
			num = it->second.endPos - it->second.startPos;
			if (num == 0)
				return true;

			if (index == endIndex)
				return false;
			return strncmp(c + it->second.startPos, c + index, num) == 0;
		}
		else
		{
			num = 0;
			return true;
		}
	}
	if (factor->mContent[0] != 0)
	{
		if (index == endIndex)
			return false;

		char* p = c + index;
		num = factor->mContentSize;
		bool isEqual = true;
		for (int i = 0; i < num; i++)
		{
			if (p[i] != factor->mContent[i] || p[i] == '\0')
			{
				isEqual = false;
				break;
			}
		}
		return isEqual;
	}
	else
	{
		num = 1;
		if (factor->m_posType != PMT_Normal)
		{
			num = 0;
			switch (factor->m_posType)
			{
			case PMT_EndAssert:
				return index == endIndex;
			case PMT_End:
				return (*(c + index) == 0 || *(c + index) == '\r' || *(c + index) == '\n');
			case PMT_Start:
				return (index == 0 || *(c + index - 1) == '\r' || *(c + index - 1) == '\n');
			case PMT_Word:
				return index == 0 || *(c + index) == 0 ||  IsWordBound(*(c + index)) || IsWordBound(*(c + index - 1));
			case PMT_Not_Word:
				return !(index == 0 || *(c + index) == 0 || IsWordBound(*(c + index)) || IsWordBound(*(c + index - 1)));
			default:
				return false;
			}
		}
		else if (factor->isNot)
		{
			if (index == endIndex)
				return false;

			for (int i = 0; i < factor->mCharRangeVecSize; i++)
			{
				if (factor->mCharRangeVec[i].start < 0)
				{
					bool ret = false;
					IsCharMatched(ret, *(c + index), factor->mCharRangeVec[i].start)
					if (ret)
						return false;
				}
				else
				{
					if (*(c + index) >= factor->mCharRangeVec[i].start && *(c + index) <= factor->mCharRangeVec[i].end)
						return	false;
				}
				
			}

			return true;
		}
		else
		{
			if (index == endIndex)
				return false;

			for (int i = 0; i < factor->mCharRangeVecSize; i++)
			{
				if (factor->mCharRangeVec[i].start < 0)
				{
					bool ret = false;
					IsCharMatched(ret, *(c + index), factor->mCharRangeVec[i].start)
					if (ret)
						return true;
				}
				else if (*(c + index) >= factor->mCharRangeVec[i].start && *(c + index) <= factor->mCharRangeVec[i].end)
					return true;
			}

			return false;
		}

	}




}


bool Regex::MatchGroupRegExprGreedy(RegData& regData, char* content, int& index, int endIndex)
{
	if (regData.m_state == RS_RollBack)
	{
		//组回滚逻辑:
		//先尝试组的迭代次数不变，若无法继续回滚，则再往下减小迭代次数， 若迭代次数小于最小迭代次数则试图往上增加迭代次数
		bool hasIncrease = false;
		bool hasFound = false;
		while (!hasFound)
		{

			//如果上次尝试的是迭代次数已经为0了， 那么只能再次试图往上尝试更多的迭代次数
			if (regData.numLastTryTimes == 0)
			{
				//如果还有往上尝试的空间则往上尝试，如果没有的话则回滚失败了
				if (regData.numLastTryTimesSave >= 0 && regData.numLastTryTimesSave < regData.factor->maxRepeat)
				{
					regData.numLastTryTimes = regData.numLastTryTimesSave + 1;

					//设置为-1表示正在往上尝试过了
					regData.numLastTryTimesSave = -1;

					RegData t(regData.factor->startFactor);
					RegDataVec& vec = *regData.subRegData;
					PushRegData(vec, t)
				}
				else
				{
					return false;
				}
			}

			if (regData.numTimes > 0)
			{
				regData.numTimes--;
				Back(*regData.subRegData).m_state = RS_RollBack;
			}

			while (regData.numTimes < regData.numLastTryTimes)
			{
				int oldIndex = index;
				bool isRollBack = Back(*regData.subRegData).m_state == RS_RollBack;

				if (MatchRegExpr(*regData.subRegData, content, index, endIndex))
				{
					//如果匹配到的组内容，设置组内容
					if (index > oldIndex)
					{
						if (isRollBack)
							RefreshMatchGroupData(regData, -1, index);
						else
							RefreshMatchGroupData(regData, oldIndex, index);
					}
					else if (regData.numLastTryTimesSave < 0)
					{
						//如果组内容为空，并且在往上尝试则回滚失败，因为组内容为空再怎么尝试也不会有什么变化，只会死循环
						return false;
					}

					regData.numTimes++;

					if (regData.numTimes < regData.numLastTryTimes)
					{
						RegData t(regData.factor->startFactor);
						RegDataVec& vec = *regData.subRegData;
						PushRegData(vec, t)
					}
					else
					{
						regData.m_state = RS_Matched;
						return true;
					}
				}
				else
				{
					if (regData.numTimes == 0)
					{
						if (regData.numLastTryTimesSave >= 0)
						{
							if (regData.numLastTryTimes > regData.factor->minRepeat)
							{
								regData.numLastTryTimes--;

								if (regData.numLastTryTimes == 0)
								{
									regData.m_state = RS_Matched;
									return true;
								}

								RegData t(regData.factor->startFactor);
								RegDataVec& vec = *regData.subRegData;
								PushRegData(vec, t)
								break;
							}
							else
							{
								//试图往上迭代
								if (regData.numLastTryTimesSave < regData.factor->maxRepeat)
								{
									regData.numLastTryTimes = regData.numLastTryTimesSave + 1;
									regData.numLastTryTimesSave = -1;

									RegData t(regData.factor->startFactor);
									RegDataVec& vec = *regData.subRegData;
									PushRegData(vec, t)
								}
								else
								{
									regData.m_state = RS_Match_Failed;
									return false;
								}

							}
						}
						else
						{
							//往上迭代次数一次回溯中只需要增加一次即可， 如果增加一次无法满足条件再增加没有任何作用，只能死循环
							if (regData.numLastTryTimes < regData.factor->maxRepeat && !hasIncrease)
							{
								hasIncrease = true;
								regData.numLastTryTimes++;
								RegData t(regData.factor->startFactor);
								RegDataVec& vec = *regData.subRegData;
								PushRegData(vec, t)
									break;
							}
							else
							{
								regData.m_state = RS_Match_Failed;
								return false;
							}
						}

					}
					else
					{
						regData.numTimes--;
						Back(*regData.subRegData).m_state = RS_RollBack;
					}
				}
			}


		}
	}
	else
	{
		regData.startIndex = index;
		while (regData.numTimes < regData.factor->maxRepeat)
		{
			int oldIndex = index;
			if (MatchRegExpr(*regData.subRegData, content, index, endIndex))
			{
				if (index > oldIndex)
					RefreshMatchGroupData(regData, oldIndex, index);

				regData.numTimes++;

				//匹配内容为空则达到最低要求后停止匹配
				if (index == oldIndex && regData.numTimes >= regData.factor->minRepeat)
				{
					break;
				}
				else if (regData.numTimes < regData.factor->maxRepeat)
				{
					RegData t(regData.factor->startFactor);
					RegDataVec& vec = *regData.subRegData;
					PushRegData(vec, t)
				}
			}
			else
			{
				if (regData.numTimes >= regData.factor->minRepeat)
				{
					break;
				}
				else
				{
					//匹配失败后，回滚，继续匹配
					if (regData.numTimes == 0)
					{
						return false;
					}
					else
					{
						regData.numTimes--;
						Back(*regData.subRegData).m_state = RS_RollBack;
					}
				}

			}
		}

		regData.numLastTryTimesSave = regData.numTimes;
		regData.numLastTryTimes = regData.numTimes;
		return true;
	}
	return false;
}

bool	Regex::MatchGroupRegExprNoGreedy(RegData& regData, char* content, int& index, int endIndex)
{

	if (regData.m_state == RS_RollBack)
	{
		bool hasFound = false;
		bool hasIncrease = false;
		while (!hasFound)
		{
			if (regData.numLastTryTimes == 0)
				regData.numLastTryTimes = 1;

			if (regData.numTimes > 0)
			{
				regData.numTimes--;
				Back(*regData.subRegData).m_state = RS_RollBack;
			}

			while (regData.numTimes < regData.numLastTryTimes)
			{
				int oldIndex = index;
				if (MatchRegExpr(*regData.subRegData, content, index, endIndex))
				{
					if (index > oldIndex)
						RefreshMatchGroupData(regData, oldIndex, index);
					regData.numTimes++;
					if (regData.numTimes < regData.numLastTryTimes)
					{
						RegData t(regData.factor->startFactor);
						RegDataVec& vec = *regData.subRegData;
						PushRegData(vec, t)
					}
					else
					{
						hasFound = true;
						break;
					}
				}
				else
				{
					if (regData.numTimes == 0)
					{
						if (regData.numLastTryTimes < regData.factor->maxRepeat && !hasIncrease)
						{
							hasIncrease = true;
							regData.numLastTryTimes++;
							RegData t(regData.factor->startFactor);
							RegDataVec& vec = *regData.subRegData;
							PushRegData(vec, t)
								break;
						}
						else
						{
							regData.m_state = RS_Match_Failed;
							return false;
						}
					}
					else
					{
						regData.numTimes--;
						Back(*regData.subRegData).m_state = RS_RollBack;
					}
				}
			}


		}

		regData.m_state = RS_Matched;
		return true;
	}
	else
	{
		regData.startIndex = index;
		while (regData.numTimes < regData.factor->minRepeat)
		{
			int oldIndex = index;
			if (MatchRegExpr(*regData.subRegData, content, index, endIndex))
			{
				if (index > oldIndex)
					RefreshMatchGroupData(regData, oldIndex, index);
				regData.numTimes++;
				if (regData.numTimes < regData.factor->minRepeat)
				{
					RegData t(regData.factor->startFactor);
					RegDataVec& vec = *regData.subRegData;
					PushRegData(vec, t)
				}
			}
			else
			{
				if (regData.numTimes == 0)
				{
					return false;
				}
				else
				{
					regData.numTimes--;
					Back(*regData.subRegData).m_state = RS_RollBack;
				}
			}
		}

		regData.numLastTryTimes = regData.numTimes;
	}

	return true;

}

bool	Regex::MatchGroupRegExpr(RegData& regData, char* content, int& index, int endIndex)
{
	if (regData.subRegData == NULL)
	{
		regData.subRegData = new RegDataVec();
		InitRegData(*(regData.subRegData), 4);
		RegData re(regData.factor->startFactor);
		RegDataVec& t = *(regData.subRegData);
		PushRegData(t, re);
	}
	bool succ = false;
	if (regData.factor->mAssertType == AT_Normal)
	{
		if (regData.factor->isGreedy)
			succ = MatchGroupRegExprGreedy(regData, content, index, endIndex);
		else
			succ = MatchGroupRegExprNoGreedy(regData, content, index, endIndex);

	}
	else
	{

		if (regData.factor->mAssertType == AT_PositiveLookAhead
			|| regData.factor->mAssertType == AT_NegativeLookAhead)
		{
			int newIndex = index;
			if (regData.factor->isGreedy)
				succ = MatchGroupRegExprGreedy(regData, content, newIndex, endIndex);
			else
				succ = MatchGroupRegExprNoGreedy(regData, content, newIndex, endIndex);
			if (regData.factor->mAssertType == AT_NegativeLookAhead)
			{
				succ = !succ;
			}
		}
		else if (regData.factor->mAssertType == AT_PositiveLookBehind
			|| regData.factor->mAssertType == AT_NegativeLookBehind)
		{
			int startNewIndex = index - regData.factor->mMaxCharactor;
			if (startNewIndex < 0)
				startNewIndex = 0;

			int endNewIndex = index - regData.factor->mMinCharactor;
			if (regData.factor->mAssertType == AT_NegativeLookBehind)
				succ = true;

			for (int i = startNewIndex; i <= endNewIndex; i++)
			{
				if (i > startNewIndex)
				{
					RegData re(regData.factor->startFactor);
					RegDataVec& t = *(regData.subRegData);
					PushRegData(t, re);
				}

				bool ret = false;
				if (regData.factor->isGreedy)
					ret = MatchGroupRegExprGreedy(regData, content, i, index);
				else
					ret = MatchGroupRegExprNoGreedy(regData, content, i, index);

				if (ret)
				{
					succ = regData.factor->mAssertType == AT_NegativeLookBehind ? false : true;
					break;
				}
			}
		}
			
		
		
	}
	

	return succ;
}

void Regex::RefreshMatchGroupData(RegData &regData, int startIndex, int endIndex)
{
	if (!regData.factor->groupName.empty())
	{
		if (startIndex >= 0)
			m_matchGroupNameMap[regData.factor->groupName].startPos = startIndex;
		m_matchGroupNameMap[regData.factor->groupName].endPos = endIndex;
	}
	else
	{
		if (startIndex >= 0)
			m_matchGroupIDMap[regData.factor->groupIndex].startPos = startIndex;
		m_matchGroupIDMap[regData.factor->groupIndex].endPos = endIndex;
	}
}

bool	Regex::MatchRegExpr(RegDataVec& regDataVec, char* content, int& index, int endIndex)
{
	int savedIndex = index;
	while (true)
	{
		switch (Back(regDataVec).m_state)
		{
		case RS_RollBack:
		{
			switch (Back(regDataVec).factor->type)
			{
			case RFT_Start:
			{
				if (Back(regDataVec).numTimes < Back(regDataVec).factor->subFactorSize - 1)
				{
					Back(regDataVec).numTimes++;
					Back(regDataVec).m_state = RS_Matching;
				}
				else
				{
					PopRegData(regDataVec)
					return false;
				}
			}
			break;
			case RFT_Normal:
			{
				int numChar = 1;
				GetNumMatchChar(numChar, Back(regDataVec).factor);
				if (Back(regDataVec).factor->isGreedy)
				{
					if (numChar > 0 && Back(regDataVec).numTimes > Back(regDataVec).factor->minRepeat)
					{
						Back(regDataVec).numTimes--;
						index -= numChar;
						Back(regDataVec).m_state = RS_Matched;
					}
					else
					{
						index -= Back(regDataVec).numTimes * numChar;

						Back(regDataVec).m_state = RS_Match_Failed;
					}
				}
				else
				{
					int maxRepeat = Back(regDataVec).factor->maxRepeat;
					int num = 0;
					if ((Back(regDataVec).numTimes < maxRepeat)
						&& CanMatch(Back(regDataVec).factor, content, index, num, endIndex) && num > 0)
					{
						Back(regDataVec).numTimes++;
						Back(regDataVec).m_state = RS_Matched;
						index += num;
					}
					else
					{
						index -= Back(regDataVec).numTimes * numChar;

						Back(regDataVec).m_state = RS_Match_Failed;
					}
				}
			}
			break;
			case RFT_Group:
			{
				if (Back(regDataVec).factor->mAssertType == AT_Normal &&
					MatchGroupRegExpr(Back(regDataVec), content, index, endIndex))
				{
					Back(regDataVec).m_state = RS_Matched;
				}
				else
				{
					Back(regDataVec).m_state = RS_Match_Failed;
				}
			}
			break;
			}
		}
		break;
		case RS_Matched:
		{
			if (Back(regDataVec).factor->subFactorSize > 0)
			{
				RegexFactor* nextFactor = Back(regDataVec).factor->subFactor[0];
				RegData t(nextFactor);
				PushRegData(regDataVec, t);
			}
			else
			{
				return true;
			}
		}
		break;
		case RS_Match_Failed:
		{
			PopRegData(regDataVec);
			Back(regDataVec).m_state = RS_RollBack;
		}
		break;
		case RS_Matching:
		{
			switch (Back(regDataVec).factor->type)
			{
			case RFT_Start:
			{
				if (Back(regDataVec).numTimes < Back(regDataVec).factor->subFactorSize)
				{
					RegexFactor* nextFactor = Back(regDataVec).factor->subFactor[Back(regDataVec).numTimes];
					RegData t(nextFactor);
					PushRegData(regDataVec, t);
				}
				else
				{
					PopRegData(regDataVec)
					return false;
				}
			}
			break;
			case RFT_Normal:
			{
				if (!Back(regDataVec).factor->isGreedy)
				{
					if (Back(regDataVec).numTimes < Back(regDataVec).factor->minRepeat)
					{
						int num = 1;
						if (CanMatch(Back(regDataVec).factor, content, index, num, endIndex))
						{
							Back(regDataVec).numTimes++;
							index += num;
						}
						else
						{
							int numChar = 1;
							GetNumMatchChar(numChar, Back(regDataVec).factor);
							index -= Back(regDataVec).numTimes * numChar;

							Back(regDataVec).m_state = RS_Match_Failed;
						}
					}
					else
					{
						Back(regDataVec).m_state = RS_Matched;
					}
				}
				else
				{
					while (Back(regDataVec).numTimes < Back(regDataVec).factor->maxRepeat)
					{
						int num = 1;
						if (CanMatch(Back(regDataVec).factor, content, index, num, endIndex))
						{
							Back(regDataVec).numTimes++;
							index += num;
						}
						else
						{
							break;
						}
					}

					if (Back(regDataVec).numTimes >= Back(regDataVec).factor->minRepeat)
					{
						Back(regDataVec).m_state = RS_Matched;
					}
					else
					{
						int numChar = 1;
						GetNumMatchChar(numChar, Back(regDataVec).factor);
						index -= Back(regDataVec).numTimes * numChar;

						Back(regDataVec).m_state = RS_Match_Failed;
					}

				}
			}
			break;

			case RFT_Group:
			{
				if (MatchGroupRegExpr(Back(regDataVec), content, index, endIndex))
				{
					Back(regDataVec).m_state = RS_Matched;
				}
				else
				{
					Back(regDataVec).m_state = RS_Match_Failed;
				}
			}
			break;
			}
		}
		break;
		}
	}

}


bool	Regex::Match(char* content)
{
	if (m_rootRegexFactor != NULL)
	{
		m_matchGroupIDMap.clear();
		m_matchGroupNameMap.clear();

		RegDataVec	regDataVec;
		InitRegData(regDataVec, 4);
		RegData re(m_rootRegexFactor);
		PushRegData(regDataVec, re);
		int	index = 0;
		int endIndex = (int)strlen(content);
		bool ret = MatchRegExpr(regDataVec, content, index, endIndex);
		return ret && content[index] == 0;
	}
	return false;
}

const std::vector<MatchData>&	Regex::Search(char* content)
{
	m_matchResultVec.clear();
	if (m_rootRegexFactor != NULL)
	{

		int	index = 0;
		int save = 0;
		RegDataVec	regDataVec;
		InitRegData(regDataVec, 4);
		RegData re(m_rootRegexFactor);
		int endIndex = (int)strlen(content);
		while (content[index] != '\0')
		{
			int startIndex = index;
			PushRegData(regDataVec, re);
			if (MatchRegExpr(regDataVec, content, index, endIndex) && index > startIndex)
			{
				MatchData matchData;
				matchData.matchedValue.assign(content + startIndex, content + index);

				std::map<int, MatchGroupData>::iterator it = m_matchGroupIDMap.begin();
				for (; it != m_matchGroupIDMap.end(); it++)
				{
					std::string& groupValue = matchData.groupIDMap[it->first];
					groupValue.assign(content + it->second.startPos, content + it->second.endPos);
				}

				std::map<std::string, MatchGroupData>::iterator NameIt = m_matchGroupNameMap.begin();
				for (; NameIt != m_matchGroupNameMap.end(); NameIt++)
				{
					std::string& groupValue = matchData.groupNameMap[NameIt->first];
					groupValue.assign(content + NameIt->second.startPos, content + NameIt->second.endPos);
				}

				m_matchResultVec.push_back(matchData);

				m_matchGroupIDMap.clear();
				m_matchGroupNameMap.clear();

				for (int i = 0; i <= regDataVec.mLastIndex; i++)
					regDataVec.mRegData[i].~RegData();

				regDataVec.mLastIndex = -1;
			}
			else
			{
				index++;
			}
		}
		
	}
	return m_matchResultVec;
}
