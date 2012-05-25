/**
 * This file is part of Pandion.
 *
 * Pandion is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Pandion is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Pandion.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Filename:    XMPPXMLParser.cpp
 * Author(s):   Dries Staelens
 * Copyright:   Copyright (c) 2009 Dries Staelens
 * Description: This file declares a parser for parsing XMPP XML data and 
 *              generating appropriate events.
 */
#include "stdafx.h"
#include "UTF.h"
#include "XMPPXMLParser.h"

/*
 * Constructor for the XMPPXMLParser. The parameter should be a reference
 * to the object that handles all the events generated by the parser.
 */
XMPPXMLParser::XMPPXMLParser(XMPPHandlers& handlers, XMPPLogger& logger) :
	m_Handlers(handlers), m_Logger(logger)
{
	RestartParser();
}

/*
 * Destructor for the XMPPXMLParser. No custom destruction work required...
 * for now.
 */
XMPPXMLParser::~XMPPXMLParser()
{
	m_ElementAttributes.clear();
}

/*
 * This method restarts the parser by setting the parser for expecting an XML
 * prolog and it clears the parse buffer.
 */
void XMPPXMLParser::RestartParser()
{
	m_ParsingState = ParsingProlog;
	m_ParsedData.clear();
}

/*
 * This method is used by the owner to notify the parser that it can be
 * expecting data.
 */
void XMPPXMLParser::SetConnected()
{
	RestartParser();
}

/*
 * This method is used by the owner to notify the parser that it should no 
 * longer expect data.
 */
void XMPPXMLParser::SetDisconnected()
{
	RestartParser();
}

/*
 * This method parses a chunk of data on a per-character basis. The parameter
 * should be a UTF-32 character value.
 */
bool XMPPXMLParser::ParseChar(unsigned c)
{
	bool continueParsing = true;

	m_ParsedData += c;
	if(m_ParsingState == ParsingProlog)
	{
		continueParsing = ParseProlog(c);
	}
	else if(m_ParsingState == ParsingRootElement)
	{
		continueParsing = ParseRootElement(c);
	}
	else if(m_ParsingState == ParsingRootElementEnd)
	{
		continueParsing = ParseRootElementEnd(c);
	}
	else if(m_ParsingState == ParsingXMPPStanzaBegin)
	{
		continueParsing = ParseXMPPStanzaBegin(c);
	}
	else if(m_ParsingState == ParsingXMPPStanza)
	{
		continueParsing = ParseXMPPStanza(c);
	}
	else if(m_ParsingState == ParsingXMPPStanzaEnd)
	{
		continueParsing = ParseXMPPStanzaEnd(c);
	}
	else
	{
		continueParsing = false;
	}

	return continueParsing;
}

/*
 * Private helper that returns true if the character is XML whitespace.
 */
bool inline XMPPXMLParser::IsXMLWhitespace(unsigned xmlCharacter)
{
	return xmlCharacter == L' ' ||
		xmlCharacter == L'\t' ||
		xmlCharacter == L'\n' ||
		xmlCharacter == L'\r';
}

void XMPPXMLParser::BuildNamespaceCache()
{
	const unsigned xmlnsLiteral[] = { L'x', L'm', L'l', L'n', L's', 0 };
	const unsigned xmlnsLiteral2[] = { L'x', L'm', L'l', L'n', L's', L':', 0 };

	for(std::map<UTF32String, UTF32String>::iterator it =
		m_ElementAttributes.begin();it != m_ElementAttributes.end();it++)
	{
		if( it->first.compare(xmlnsLiteral) == 0 ||
			it->first.find(xmlnsLiteral2) == 0)
		{
			m_NamespaceCache[it->first] = it->second;
		}
	}
}

/*
 * Private method that is called when the parser is expecting prolog data. When
 * this method sees there is no prolog it automatically puts the parser in the
 * state for expecting the root element. When the prolog ends it also starts
 * expecting the root element.
 */
bool XMPPXMLParser::ParseProlog(unsigned xmlCharacter)
{
	if(xmlCharacter != L'?' && m_ParsedData.size() == 2)
	{
		m_ParsingState = ParsingRootElement;
		m_ElementParsingState = ParsingBeforeElement;
		ParseRootElement(m_ParsedData[0]);
		ParseRootElement(xmlCharacter);
	}
	else if(xmlCharacter == L'>' && 
		m_ParsedData.size() >= 2 &&
		*(m_ParsedData.end()-2) == L'?')
	{
		m_ParsingState = ParsingRootElement;
		m_ElementParsingState = ParsingBeforeElement;
	}
	return true;
}

/*
 * Private method that is called when the parser is expecting the root element.
 * When this method sees the end of the root element, the parsed data is sent
 * to the event handler and the parser is put in the state for expecting XMPP
 * stanzas.
 */
bool XMPPXMLParser::ParseRootElement(unsigned xmlCharacter)
{
	bool continueParsing = true;
	if(m_ElementParsingState == ParsingBeforeElement)
	{
		continueParsing = ParseElementBegin(xmlCharacter);
	}
	else if(m_ElementParsingState == ParsingName)
	{
		continueParsing = ParseElementName(xmlCharacter);
	}
	else if(m_ElementParsingState == ParsingBetweenAttributes)
	{
		continueParsing = ParseBetweenAttributes(xmlCharacter);
	}
	else if(m_ElementParsingState == ParsingAttributeName)
	{
		continueParsing = ParseAttributeName(xmlCharacter);
	}
	else if(m_ElementParsingState == ParsingAttributeValue)
	{
		continueParsing = ParseAttributeValue(xmlCharacter);
	}
	return continueParsing;
}

/*
 * Private method that is called when the parser is expecting the end of the
 * root element (e.g. </stream:stream>).
 */
bool XMPPXMLParser::ParseRootElementEnd(unsigned xmlCharacter)
{
	bool continueParsing = true;
	if(xmlCharacter == L'>')
	{
		m_Handlers.OnDocumentEnd(UTF::utf32to16(m_ParsedData).c_str());
		continueParsing = false;
	}
	return continueParsing;
}

/*
 * Private method that is called when the parser is expecting the root element
 * of an XMPP stanza. The state transitions when ">" is encountered.
 * If "/>", the XML element is self-closing and is sent to the event handler
 * by calling ParseXMPPStanzaEnd.
 */
bool XMPPXMLParser::ParseXMPPStanzaBegin(unsigned xmlCharacter)
{
	bool continueParsing = true;
	m_ElementLevel = 0;

	if(xmlCharacter == L'>' && 
		m_ParsedData.size() >= 2 &&
		*(m_ParsedData.end()-2) == L'/')
	{
		ParseXMPPStanzaEnd(xmlCharacter);
	}
	else if(xmlCharacter == L'/' &&
		m_ParsedData.size() == 2 &&
		*(m_ParsedData.begin()) == L'<')
	{
		m_ParsingState = ParsingRootElementEnd;
	}
	else if(xmlCharacter == L'>')
	{
		m_ParsingState = ParsingXMPPStanza;
	}
	else if(m_ElementParsingState == ParsingBeforeElement)
	{
		continueParsing = ParseElementBegin(xmlCharacter);
	}
	else if(m_ElementParsingState == ParsingName)
	{
		continueParsing = ParseElementName(xmlCharacter);
	}
	else if(m_ElementParsingState == ParsingBetweenAttributes)
	{
		continueParsing = ParseBetweenAttributes(xmlCharacter);
	}
	else if(m_ElementParsingState == ParsingAttributeName)
	{
		continueParsing = ParseAttributeName(xmlCharacter);
	}
	else if(m_ElementParsingState == ParsingAttributeValue)
	{
		continueParsing = ParseAttributeValue(xmlCharacter);
	}

	return continueParsing;
}

/*
 * Private method that is called when the parser is expecting the contents of
 * the body of the root element of the XMPP stanza. It maintains the nesting
 * level of the XML element for determining the end of the body.
 */
bool XMPPXMLParser::ParseXMPPStanza(unsigned xmlCharacter)
{
	static bool parsingCDATA = false;
	bool continueParsing = true;
	if(m_ParsedData.size() >= 9 && 
		*(m_ParsedData.end() - 1) == L'[' &&
		*(m_ParsedData.end() - 2) == L'A' &&
		*(m_ParsedData.end() - 3) == L'T' &&
		*(m_ParsedData.end() - 4) == L'A' &&
		*(m_ParsedData.end() - 5) == L'D' &&
		*(m_ParsedData.end() - 6) == L'C' &&
		*(m_ParsedData.end() - 7) == L'[' &&
		*(m_ParsedData.end() - 8) == L'!' &&
		*(m_ParsedData.end() - 9) == L'<')
	{
		m_ElementLevel--;
		parsingCDATA = true;
	}
	else if(parsingCDATA)
	{
		if(m_ParsedData.size() >= 3 && 
		*(m_ParsedData.end() - 1) == L'>' &&
		*(m_ParsedData.end() - 2) == L']' &&
		*(m_ParsedData.end() - 3) == L']')
		{
			parsingCDATA = false;
		}
	}
	else if(m_ParsedData.size() >= 2 &&
		*(m_ParsedData.end() - 2) == L'<')
	{
		if(xmlCharacter == L'/')
		{
			m_ElementLevel--;
			if(m_ElementLevel == -1)
			{
				m_ParsingState = ParsingXMPPStanzaEnd;
			}
		}
		else
		{
			m_ElementLevel++;
		}
	}
	else if(xmlCharacter == L'>' && 
		m_ParsedData.size() >= 2 &&
		*(m_ParsedData.end() - 2) == L'/')
	{
		m_ElementLevel--;
	}
	return continueParsing;
}

/*
 * Private method that is called when we are expecting the end of the closing
 * tag of the root element of the current stanza.
 */
bool XMPPXMLParser::ParseXMPPStanzaEnd(unsigned xmlCharacter)
{
	bool continueParsing = true;
	if(xmlCharacter == L'>')
	{
		m_ParsingState = ParsingXMPPStanzaBegin;
		continueParsing = HandleXMPPStanza();
		m_ElementParsingState = ParsingBeforeElement;
		m_ParsedData.clear();
		m_ElementAttributes.clear();
		m_ElementName.clear();
		m_AttributeName.clear();
		m_AttributeValue.clear();
	}
	return continueParsing;
}

/*
 * Private method that is called when the parser is expecting a new element.
 */
bool XMPPXMLParser::ParseElementBegin(unsigned xmlCharacter)
{
	bool continueParsing = true;
	if(IsXMLWhitespace(xmlCharacter))
	{
	}
	else if(xmlCharacter == L'<')
	{
		m_ElementName.clear();
		m_ElementParsingState = ParsingName;
	}
	else
	{
		m_ElementName += xmlCharacter;
	}
	return continueParsing;
}

/*
 * Private method that is called when the parser is excpecing an element name.
 */
bool XMPPXMLParser::ParseElementName(unsigned xmlCharacter)
{
	bool continueParsing = true;;
	
	if(xmlCharacter == L'>' || xmlCharacter == L'\\')
	{
		m_ElementParsingState = ParsingBetweenAttributes;
		ParseBetweenAttributes(xmlCharacter);
	}
	else if(!IsXMLWhitespace(xmlCharacter))
	{
		m_ElementName += xmlCharacter;
	}
	else
	{
		m_ElementParsingState = ParsingBetweenAttributes;
		ParseBetweenAttributes(xmlCharacter);
	}

	return continueParsing;
}

/*
 * Private method that is called when the parser assumes to be between
 * elements.
 */
bool XMPPXMLParser::ParseBetweenAttributes(unsigned xmlCharacter)
{
	bool continueParsing = true;

	if(xmlCharacter == L'>')
	{
		BuildNamespaceCache();
		m_Handlers.OnDocumentStart(_bstr_t(UTF::utf32to16(m_ParsedData).c_str()));
		m_ParsedData.clear();
		m_ElementAttributes.clear();
		m_ElementName.clear();
		m_AttributeName.clear();
		m_AttributeValue.clear();
		m_ElementParsingState = ParsingBeforeElement;
		m_ParsingState = ParsingXMPPStanzaBegin;
	}
	else if(xmlCharacter == L'/')
	{
	}
	else if(!IsXMLWhitespace(xmlCharacter))
	{
		m_AttributeName.clear();
		m_AttributeValue.clear();
		m_ElementParsingState = ParsingAttributeName;
		ParseAttributeName(xmlCharacter);
	}
	else
	{
	}
	return continueParsing;	
}

/*
 * Private method that is called when the parser expects the name of an
 * attribute.
 */
bool XMPPXMLParser::ParseAttributeName(unsigned xmlCharacter)
{
	bool continueParsing = true;

	if((xmlCharacter == L'"'||xmlCharacter == L'\'') &&
		m_ParsedData.size() >= 2 &&
		*(m_ParsedData.end() - 2) == L'=')
	{
		m_AttributeDelimiter = xmlCharacter;
		m_ElementParsingState = ParsingAttributeValue;
	}
	else if(IsXMLWhitespace(xmlCharacter))
	{
		continueParsing = false;
	}
	else if(xmlCharacter != L'=')
	{
		m_AttributeName += xmlCharacter;
	}
	else
	{
	}
	return continueParsing;	
}

/*
 * Private method that is called when the parser expects the value of an
 * attribute.
 */
bool XMPPXMLParser::ParseAttributeValue(unsigned xmlCharacter)
{
	bool continueParsing = true;

	if(xmlCharacter == m_AttributeDelimiter)
	{
		m_ElementAttributes[m_AttributeName] = m_AttributeValue;
		m_ElementParsingState = ParsingBetweenAttributes;
	}
	else
	{
		m_AttributeValue += xmlCharacter;
	}
	return continueParsing;
}

/*
 * This method calls NotifyHandler for sending the XMPP stanza to the Handler
 * and puts the parser in a new state for parsing the next stanza or for
 * expecting a prolog/root element when it encounters the proceed or success
 * stanza.
 */
bool XMPPXMLParser::HandleXMPPStanza()
{
	bool continueParsing = true;

	const unsigned proceedLiteral[] =
		{ L'p', L'r', L'o', L'c', L'e', L'e', L'd', 0 };
	const unsigned successLiteral[] =
		{ L's', L'u', L'c', L'c', L'e', L's', 0 };
	const unsigned compressedLiteral[] =
		{ L'c', L'o', L'm', L'p', L'r', L'e', L's', L's', L'e', L'd', 0 };

	continueParsing = NotifyHandler(m_ElementName);

	if(m_ElementName.find(proceedLiteral) != UTF32String::npos ||
		m_ElementName.find(successLiteral) != UTF32String::npos ||
		m_ElementName.find(compressedLiteral) != UTF32String::npos)
	{
		RestartParser();
	}
	else
	{
		m_ParsingState = ParsingXMPPStanzaBegin;
		m_ParsedData.clear();
	}
	return continueParsing;
}

/*
 * Private method that ensures definition of the XML namespace for every stanza
 * so that MSXML can load the XML contained in the buffer.
 */
void XMPPXMLParser::FixXMLNS()
{
	UTF32String::size_type stanzaEnd = GetStanzaEnd();
	UTF32String xmlns;

	for(std::map<UTF32String, UTF32String>::iterator it = 
		m_NamespaceCache.begin(); it != m_NamespaceCache.end(); it++)
	{
		if(m_ElementAttributes.find(it->first) == m_ElementAttributes.end())
		{
			xmlns += (unsigned) L' ';
			xmlns += it->first;
			xmlns += (unsigned) L'=';
			xmlns += (unsigned) L'\"';
			xmlns += it->second;
			xmlns += (unsigned) L'\"';
		}
	}

	m_ParsedData.insert(stanzaEnd, xmlns);
}

/*
 * Private method that loads XML into an MSXML DomDocument object and passes
 * this object to the event handler.
 */
bool XMPPXMLParser::NotifyHandler(const UTF32String& stanzaName)
{
	bool continueParsing;

	FixXMLNS();

	MSXML2::IXMLDOMDocumentPtr xmlDoc;
	xmlDoc.CreateInstance(CLSID_DOMDocument);
	BOOL bSuccess = xmlDoc->loadXML(
		_bstr_t(UTF::utf32to16(m_ParsedData).c_str()));

	if(bSuccess)
	{
		m_Handlers.OnStanza(
			xmlDoc, _bstr_t(UTF::utf32to16(stanzaName).c_str()));
		continueParsing = true;
	}
	else
	{
		m_Logger.LogLoadXMLError(xmlDoc, UTF::utf32to16(m_ParsedData));
		continueParsing = false;
	}

	return continueParsing;
}

/*
 * Private helper that determines the position of last character of the name
 * of the root element in the buffer.
 */
UTF32String::size_type XMPPXMLParser::GetStanzaEnd()
{
	const unsigned stanzaBeginChars[] = 
		{ L'<', 0 };
	const unsigned stanzaEndChars[] =
		{ L'/', L'>', L'\t', L'\n', L'\r', L' ', 0 };

	UTF32String::size_type stanzaBegin = 
		m_ParsedData.find_first_of(stanzaBeginChars);
	UTF32String::size_type stanzaEnd =
		m_ParsedData.find_first_of(stanzaEndChars, stanzaBegin);

	return stanzaEnd;
}

