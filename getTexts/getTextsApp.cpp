/***************************************************************
 * Name:      getTextsApp.cpp
 * Purpose:   Code for Application Class
 * Author:    ZenJu (zhnmju123@gmx.de)
 * Created:   2008-08-06
 * Copyright: ZenJu (http://sourceforge.net/projects/freefilesync)
 * License:
 **************************************************************/

#include <iostream>
#include <fstream>
#include <set>
#include <wx/wx.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

using namespace std;

//appends all texts in _(" ... ") into file localize.txt for translation (and preserves content including already translated lines)

struct TranslationLine
{
    string original;
    string translation;

    bool operator>(const TranslationLine& b ) const
    {
        return (original > b.original);
    }
    bool operator<(const TranslationLine& b) const
    {
        return (original < b.original);
    }
    bool operator==(const TranslationLine& b) const
    {
        return (original == b.original);
    }
};



int main(int args, char** argv)
{
    set<TranslationLine> textBuffer;
    TranslationLine currentLine;

    char temp[100000];

    assert (args >= 1);

    if (!wxSetWorkingDirectory(wxFileName(argv[0]).GetPath()))
    {
        cout<<"Error when setting working directory!"<<endl;
        system("pause");
    }

    string inOutFile = "localize.txt";


//read localize.txt into buffer, if available
    ifstream localize(inOutFile.c_str());
    if (localize)
    {
        int rowNumber = 0;
        while (localize.getline(temp, 100000))
        {
            if (rowNumber%2 == 0)  //  -> original text and translation alternate
                currentLine.original = temp;
            else
            {
                currentLine.translation = temp;
                textBuffer.insert(currentLine);
            }
            rowNumber++;
        }
        localize.close();
    }


    for (int i = 1; i < args; ++i)
    {
        ifstream input(argv[i]);
        if (input)
        {
            while (input.getline(temp, 100000))
            {
                string tmpString(temp);
                unsigned indexStart = 0;
                while ((indexStart = tmpString.find("_(\"", indexStart)) != string::npos)
                {
                    unsigned int indexEnd = 0;
                    if ((indexEnd = tmpString.find("\")", indexStart + 3)) != string::npos)
                    {
                        currentLine.original = tmpString.substr(indexStart + 3, indexEnd - indexStart - 3);
                        currentLine.translation = "";
                        textBuffer.insert(currentLine);

                        indexStart = indexEnd + 2;
                    }
                    else break;
                }
            }
            input.close();
        }
        else
        {
            cout<<"Unable to read "<<argv[i]<<"!"<<endl;
            system("pause");
            return -1;
        }
    }

    ofstream writeLocalize(inOutFile.c_str());
    for (set<TranslationLine>::iterator i = textBuffer.begin(); i != textBuffer.end(); ++i)
        writeLocalize<<i->original.c_str()<<endl<<i->translation.c_str()<<endl;
    writeLocalize.close();

    return 0;
}
