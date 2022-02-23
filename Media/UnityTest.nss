//::///////////////////////////////////////////
//:: File: UnityTest.nss
//:: Unity test file for NWScript Lexer Plugin
//:: 
//:: Copyright 2022 - Leonard The Wise
//::///////////////////////////////////////////

/*
	Includes all customizable tokens:
		- Pre-processor directives;
		- Default Text
		- Built-in Instruction SET;
		- Standard variable types;
		- Engine-defined variable types;
		- Object (*special*) Type;
		- Engine Constants
		- User-Defined Constants (optional - must configure)
		- Internal Engine Functions;
		- User-Defined Functions (optional - must configure)
		- Numbers, Strings, Operators, Single Comment, Multiline Comments, 
*/

#include "somefile"

void UndefinedUserFunction(object oObject)
{
	effect eTemp = OBJECT_INVALID;
	int nColor = COLOR_WHITE;
	string sString = "Lorem Ipsum";
	vector vTranslation = [0.0198275f, -0.6277636f, 1.0f];
	location iLoc = 0x0H;

	if (GetIsPC(oObject)==OBJECT_INVALID || sTring=="" && iLoc >= 0x0H)
		string sName = GetNamePCColor(oCreature);
	
	/* Press ENTER after the brackets bellow to test auto-identation */ 
	{}
	
	int bReturne = FALSE;
	
	return;
}
