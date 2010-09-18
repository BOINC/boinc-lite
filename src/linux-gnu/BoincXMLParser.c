#include "BoincXMLParser.h"
#include <expat.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#define XMLBUFFSIZE 512

typedef struct _BoincConfigurationXML {
  int index;
  int data_offset;
  char data_buff[XMLBUFFSIZE];
  boincXMLElementBeginCallback beginCallback;
  boincXMLElementEndCallback endCallback;
  void* userData;
  BoincError error;
  char fullName[XMLBUFFSIZE];
  int level;
  int withFullNames;
} BoincXMLParser ;


static void XMLCALL boincXMLParserStart(void *data, const char *el, const char **attr)
{
  BoincXMLParser * xml = (BoincXMLParser*) data;
  xml->data_buff[0] = 0;
  xml->data_offset = 0;
  xml->level++;
  if (xml->level > 1 && xml->withFullNames) {
    strcat(xml->fullName, ">");
    strcat(xml->fullName, el);
  }else{
    strcpy(xml->fullName, el);
  }
  if (xml->error)
    xml->index = -1;
  else
    xml->index = xml->beginCallback(xml->fullName, attr, xml->userData);
}

void boincXMLParserData(void *data, const char *s, int len) 
{
  if (len < 1)
    return ;
  BoincXMLParser * xml = (BoincXMLParser*) data;
  if (xml->index < 0)
    return ;

  if (XMLBUFFSIZE - xml->data_offset < len + 1) {
    len = XMLBUFFSIZE - xml->data_offset - 1;
  }
  char b[len+1];
  memcpy(b, s, len);
  b[len] = 0 ;

  strcat(xml->data_buff, b);
  xml->data_offset += len;
  return ;
}

static void XMLCALL boincXMLParserEnd(void *data, const char * name)
{
  BoincXMLParser * xml = (BoincXMLParser*) data;
  
  if (xml->data_offset && xml->index >= 0)
    xml->error = xml->endCallback(xml->index, xml->data_buff, xml->userData);
  xml->data_buff[0] = 0;
  xml->data_offset = 0;
  xml->index = -1;
  if(xml->level > 0) {
    char * child = strrchr(xml->fullName, '>');
    if (child)
      child[0] = 0;
  }else{
    xml->fullName[0] = 0;
  }
  xml->level--;
  return;
}


BoincError boincXMLParse(const char * xmlFile, boincXMLElementBeginCallback begin, boincXMLElementEndCallback end, void* userData)
{
  FILE * fh = fopen(xmlFile, "r");
  char Buff[XMLBUFFSIZE];
  if (fh == NULL) {
    struct stat bStat;
    if (!stat(xmlFile, &bStat))
      return boincErrorCreatef(kBoincError, -1, "Cannot access file %d", xmlFile);
    return boincErrorCreatef(kBoincError, 1, "Cannot open file %s", xmlFile);
  }

  XML_Parser p = XML_ParserCreate(NULL);
  if (!p) {
    return boincErrorCreate(kBoincFatal, -2, "Cannot create xml parser");
  }

  XML_SetElementHandler(p, boincXMLParserStart, boincXMLParserEnd);
  XML_SetCharacterDataHandler(p, boincXMLParserData);
  BoincXMLParser xml;
  xml.userData = userData;
  xml.beginCallback = begin;
  xml.endCallback = end;
  xml.error = NULL;
  xml.level = -1;
  xml.withFullNames = 1;
  XML_SetUserData(p, &xml);
  xml.fullName[0] = 0;

  for (;;) {
    int done;
    int len;

    len = (int)fread(Buff, 1, XMLBUFFSIZE, fh);
    if (ferror(fh)) {
      XML_ParserFree(p);
      return boincErrorCreatef(kBoincError, -3, "Cannot read file %s", xmlFile);
    }
    done = feof(fh);

    if (XML_Parse(p, Buff, len, done) == XML_STATUS_ERROR) {
      XML_ParserFree(p);
      return boincErrorCreate(kBoincError, -4, "Error in XML parsing");
    }

    if (done)
      break;
  }
  XML_ParserFree(p);
  fclose(fh);
  return xml.error;
}

const char * boincXMLGetAttributeValue(const char** attributes, const char* attributeName) 
{
  for (int i = 0 ; attributes[i] ; i+=2 ) {
    if (!strcmp(attributes[i], attributeName))
      return attributes[i+1];
  }
  return NULL;
}

int boincXMLAttributeCompare(const char** attributes, const char* attributeName, const char* attributeValue)
{
  const char * res = boincXMLGetAttributeValue(attributes, attributeName);
  if (!res)
    return -1;
  if (! strcmp(res, attributeValue))
    return 0;
  return 1;
}
