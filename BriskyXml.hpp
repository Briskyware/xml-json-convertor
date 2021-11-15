#ifndef BRISKYXML_HXX
#define BRISKYXML_HXX

#include "rapidjson/reader.h"
#include "rapidjson/document.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/filereadstream.h"
#include "rapidjson/filewritestream.h"
#include "rapidjson/error/en.h"
#include "rapidjson/schema.h"
#include "rapidjson/writer.h"
#include <cstdio>
#include <iostream>
#include <pugixml.hpp>
#include <stack>


#define PREFIX_ATTRIBUTE '@'

#define WRITE_ELEMENT(APPEND_FUNC)             \
    std::string key = keys.top(); \
        if(!arrayKeys.empty())\
        {\
            std::string a = arrayKeys.top();\
            if(a==key)\
            {\
                if(!endElementFlg) xml+=">";\
                xml+="<";\
                xml+=arrayElementName;\
                xml+=">";\
                APPEND_FUNC(buffer,len);\
                xml+="</";\
                xml+=arrayElementName;\
                xml+=">";\
                endElementFlg = true;\
                return true;\
            }\
        }\
        keys.pop();\
        if(key[0]==PREFIX_ATTRIBUTE)\
        {\
             xml+=" ";\
             xml+=key.substr(1);\
             xml+="=\"";\
             APPEND_FUNC(buffer,len);\
             xml+="\" ";\
             return true;\
        }\
        if(!endElementFlg) xml+=">";\
        xml+="<";\
        xml+=key;\
        xml+=">";\
        APPEND_FUNC(buffer,len);\
        xml+="</";\
        xml+=key;\
        xml+=">";\
        endElementFlg = true;\
        return true; 



namespace Brisky::Xml
{


//======================Xml to Json

void processDoc(pugi::xml_node &node, rapidjson::Document &obj)
{
    if (node.type() != 2)
        return;

      auto attrs = node.attributes();
      size_t attrs_count = std::distance(attrs.begin(), attrs.end());

      if (attrs_count > 0) {
        for (pugi::xml_attribute a : node.attributes()) {
            std::string k(1,PREFIX_ATTRIBUTE);
            k+=a.name();
            rapidjson::Value key(k.c_str(), obj.GetAllocator()); 
            rapidjson::Value val(std::string(a.value()).c_str(), obj.GetAllocator());                   
            obj.AddMember(key, val, obj.GetAllocator());
        }
      }

    auto children = node.children();

    for (pugi::xml_node child : children)
    {
        if(child.type()!=2) continue;
        rapidjson::Document c(&obj.GetAllocator());
        c.SetObject();
        auto childchildren = child.children();
        size_t children_count = std::distance(childchildren.begin(), childchildren.end());
        std::string k = std::string(child.name()); 
        if(children_count == 1 && childchildren.begin()->type()==3)
        {     
            rapidjson::Value val = rapidjson::Value(std::string(childchildren.begin()->value()).c_str(), obj.GetAllocator());  
            c.Swap(val);
        }
        else
        {
            processDoc(child, c);
        }

        if(!obj.HasMember(k.c_str()))
        {
            rapidjson::Value key(k.c_str(), obj.GetAllocator());
            obj.AddMember(key, c, obj.GetAllocator());
        }
        else if(!obj[k.c_str()].IsArray())
        {
            rapidjson::Value v;
            v.CopyFrom(obj[k.c_str()],obj.GetAllocator());
            obj.RemoveMember(k.c_str());
            rapidjson::Value a(rapidjson::kArrayType);
            a.PushBack(v,obj.GetAllocator());
            a.PushBack(c,obj.GetAllocator());
            rapidjson::Value key(k.c_str(), obj.GetAllocator());
            obj.AddMember(key, a, obj.GetAllocator());
        }
        else
        {
            obj[k.c_str()].PushBack(c,obj.GetAllocator());
        }           
    }
};


//========================Json to Xml

template <typename OutputStream>
class JsonxWriter
{
public:
    JsonxWriter(OutputStream &os, std::string &rootTag,std::string &arrayElementName_) : xml(os),endElementFlg(false), rootName(rootTag),arrayElementName(arrayElementName_)
    {
    }

    bool Null()
    {
        if(!endElementFlg) xml+=">";
        xml+="<";
        xml+=keys.top();
        xml+="/>";
        endElementFlg=true;
        keys.pop();
        return true;       
    }

    bool Bool(bool b)
    {
         std::string tmp=(b?"TRUE":"FALSE");
         const char * buffer = tmp.c_str();
         auto len = tmp.length();
         WRITE_ELEMENT(xml.append)    
         return true;
    }

    bool Int(int i)
    {
        char buffer[12];
        auto len = sprintf(buffer, "%d", i);
        WRITE_ELEMENT(xml.append) 
    }

    bool Uint(unsigned i)
    {
        char buffer[11];
        auto len = sprintf(buffer, "%u", i);
        WRITE_ELEMENT(xml.append) 
    }

    bool Int64(int64_t i)
    {
        char buffer[21];
        auto len = sprintf(buffer, "%" PRId64, i);
        WRITE_ELEMENT(xml.append) 
    }

    bool Uint64(uint64_t i)
    {
        char buffer[21];
        auto len = sprintf(buffer, "%" PRIu64, i);
        WRITE_ELEMENT(xml.append)         
    }

    bool Double(double d)
    {
        char buffer[30];
        auto len = sprintf(buffer, "%.17g", d);
        WRITE_ELEMENT(xml.append)         
    }

    bool RawNumber(const char *buffer, rapidjson::SizeType len, bool)
    {
        WRITE_ELEMENT(WriteEscapedText)
    }

    bool String(const char *buffer, rapidjson::SizeType len, bool)
    {
        WRITE_ELEMENT(WriteEscapedText)
    }

    bool StartObject()
    {
        
        if (keys.empty())
        {
            xml+="<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
            keys.push(rootName);
        }
        else if(!endElementFlg)
        {
            xml+=">";
        }
        endElementFlg = false;
        std::string key = keys.top();
        if(!arrayKeys.empty() && key==arrayKeys.top())
        {
            keys.push(arrayElementName);
        }
        xml+="<";
        xml+=keys.top();
        return true;
    }

    bool Key(const char *buffer, rapidjson::SizeType len, bool)
    {
        std::string name="";
        name.append(buffer,len);
        keys.push(name);
        return true;
    }

    bool EndObject(rapidjson::SizeType)
    {
        if(!endElementFlg)
        {
             xml+=">";
        }
        xml+="</";
        xml+=keys.top();
        xml+=">";
        endElementFlg = true;
        keys.pop();
        return true;
    }

    bool StartArray()
    {        
        if (keys.empty())
        {
            xml+="<?xml version=\"1.0\" encoding=\"UTF-8\"?>";
            keys.push(rootName);
        }
        else if(!endElementFlg)
        {
            xml+=">";
        }

        endElementFlg = false;
        std::string key = keys.top();
        arrayKeys.push(key);
        xml+="<";
        xml+=key;
        return true;
    }

    bool EndArray(rapidjson::SizeType)
    { 
        if(!endElementFlg)
        {
             xml+=">";
        }
        xml+="</";
        xml+=keys.top();
        xml+=">";
        endElementFlg = true;
        keys.pop();
        arrayKeys.pop();
        return true;
    }

private:

    void  WriteEscapedText(const char *s, size_t length)
    {
        for (size_t i = 0; i < length; i++)
        {
            switch (s[i])
            {
            case '&':
                xml+="&amp;";
                break;
            case '<':
                xml+="&lt;";
                break;
            case '"':
                xml+="&quot;";
                break;
            default:
                xml.push_back(s[i]);
                break;
            }
        }
    }
   
    OutputStream &xml;    
    bool endElementFlg;
    std::stack<std::string> keys;
    std::stack<std::string> arrayKeys;
    std::string rootName;
    std::string arrayElementName;
};


static void xml_to_json(std::string &xml, rapidjson::Document &json)
{
    json.SetObject();
    pugi::xml_document doc;
    if (doc.load_string(xml.c_str()))
    {
        for (pugi::xml_node n : doc)
        {
            processDoc(n, json);
        }
    }
};

static void json_to_xml(std::string &xml, rapidjson::Document &json,std::string &rootName,std::string &arrayElementName)
{
    xml.clear();
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> w(buffer);
    json.Accept(w);
    rapidjson::StringStream ss(buffer.GetString());
    rapidjson::Reader reader;
    JsonxWriter<std::string> writer(xml, rootName,arrayElementName);
    if (!reader.Parse(ss, writer))
    {
        fprintf(stderr, "\nError(%u): %s\n", static_cast<unsigned>(reader.GetErrorOffset()), GetParseError_En(reader.GetParseErrorCode()));
        return ;
    }

};

static void json_to_xml(std::string &xml, std::string &json,std::string &rootName,std::string &arrayElementName)
{
   
    xml.clear();
    rapidjson::StringStream ss(json.c_str());
    rapidjson::Reader reader;
    JsonxWriter<std::string> writer(xml, rootName,arrayElementName);
    if (!reader.Parse(ss, writer))
    {
        fprintf(stderr, "\nError(%u): %s\n", static_cast<unsigned>(reader.GetErrorOffset()), GetParseError_En(reader.GetParseErrorCode()));
        return ;
    }
};

}

#endif // BRISKYXML_HXX
