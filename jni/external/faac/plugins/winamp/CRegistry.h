/*
FAAC - codec plugin for Cooledit
Copyright (C) 2002-2004 Antonio Foranna

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation.
	
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
		
You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
			
The author can be contacted at:
ntnfrn_email-temp@yahoo.it
*/

#ifndef registry_h
#define registry_h

class CRegistry 
{
public:
			CRegistry();
			~CRegistry();

	BOOL	openReg(HKEY hKey, char *SubKey);
	BOOL	openCreateReg(HKEY hKey, char *SubKey);
	void	closeReg();
	void	DeleteRegVal(char *SubKey);
	void	DeleteRegKey(char *SubKey);

	void	setRegBool(char *keyStr , BOOL val);
	void	setRegBool(char *keyStr , bool val);
	void	setRegByte(char *keyStr , BYTE val);
	void	setRegWord(char *keyStr , WORD val);
	void	setRegDword(char *keyStr , DWORD val);
	void	setRegFloat(char *keyStr , float val);
	void	setRegStr(char *keyStr , char *valStr);
	void	setRegValN(char *keyStr , BYTE *addr,  DWORD size);

	BOOL	getSetRegBool(char *keyStr, BOOL var);
	bool	getSetRegBool(char *keyStr, bool var);
	BYTE	getSetRegByte(char *keyStr, BYTE var);
	WORD	getSetRegWord(char *keyStr, WORD var);
	DWORD	getSetRegDword(char *keyStr, DWORD var);
	float	getSetRegFloat(char *keyStr, float var);
	int		getSetRegStr(char *keyStr, char *tempString, char *dest, int maxLen);
	int		getSetRegValN(char *keyStr, BYTE *tempAddr, BYTE *addr, DWORD size);

	HKEY	regKey;
	char	*path;
};
#endif