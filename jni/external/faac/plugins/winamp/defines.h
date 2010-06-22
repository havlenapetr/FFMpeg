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

#define APP_NAME "MPEG4-AAC encoder"
#define APP_VER "v1.3"
#define REGISTRY_PROGRAM_NAME "SOFTWARE\\4N\\Winamp\\Out_AAC"

// -----------------------------------------------------------------------------------------------

#define FREE_ARRAY(ptr) \
{ \
	if(ptr) \
		free(ptr); \
	ptr=0; \
}

// -----------------------------------------------------------------------------------------------

#define GLOBALLOCK(ptr,handle,type,ret) \
{ \
	if(!(ptr=(type *)GlobalLock(handle))) \
	{ \
		MessageBox(0, "GlobalLock", APP_NAME " plugin", MB_OK|MB_ICONSTOP); \
		ret; \
	} \
}
