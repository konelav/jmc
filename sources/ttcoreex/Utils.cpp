/* Autoconf patching by David Hedbor, neotron@lysator.liu.se */
/*********************************************************************/
/* file: utils.c - some utility-functions                            */
/*                             TINTIN III                            */
/*          (T)he K(I)cki(N) (T)ickin D(I)kumud Clie(N)t             */
/*                     coded by peter unold 1992                     */
/*********************************************************************/
#include "stdafx.h"
#include "tintin.h"
#include <vector>
#include <string>
#include <sstream>

using namespace std;

void syserr(char* msg);

int is_all_digits(const char *number)
{
	int i, ok = 1;
    for (i = 0; number[i]; i++) {
        if (number[i] < '0' || number[i] > '9') {
            ok = 0;
            break;
        }
    }
	return ok;
}

/*********************************************/
/* return: TRUE if s1 is an abrevation of s2 */
/*********************************************/
int is_abrev(const char *s1, const char *s2)
{
//vls-begin// bugfix
//    return(s1[0]==s2[0] && !strncmp(s2, s1, strlen(s1)));
    return(tolower(s1[0])==tolower(s2[0]) && !_strnicmp(s2, s1, strlen(s1)));
//vls-end//
}


/*************************************************/
/* print system call error message and terminate */
/*************************************************/
void syserr(char* msg)
{
  extern int errno;
  char ErrMsg[256];

  sprintf(ErrMsg,rs::rs(1211),msg, errno);
  ShowError(ErrMsg);
  // EndApplication();
}

string StrPrintfV(char* pszFormat, va_list marker)
{
	/*
	string data;
	data.reserve(256);
	while( _vsnprintf(&data[0], data.capacity(), pszFormat, marker) == -1 )
           data.reserve(data.capacity() + 256);
    return data;
	*/
	
    vector<char> data;
        data.reserve(256);
    while( _vsnprintf(data.begin(), data.capacity(), pszFormat, marker) == -1 )
           data.reserve(data.capacity() + 256);
    return data.begin();
	
}

string strprintf(char* pszFormat, ...)
{
    va_list marker;
    va_start(marker, pszFormat);
    string str = StrPrintfV(pszFormat, marker);
    va_end(marker);
    return str;
}


std::vector<int> split(const string &s, char delim) {
    vector<int> elems;
    stringstream ss(s);
    string item;

    while(std::getline(ss, item, delim) && (item.length() > 0)) {
         elems.push_back(atoi(item.c_str()));
    }
    return elems;
}