#ifndef _CALCU_EXP_
#define _CALCU_EXP_

void DelSpaceAndNotes(TCHAR * str);
UINT ConvertValue(TCHAR *pCh, UINT uiLine);
UINT GetOperateResult(TCHAR * pCh, UINT uiLine, BOOL * pIsAuto);
#endif