#ifndef _CONFIGSCRIPT_H__
#define _CONFIGSCRIPT_H__
 
#include <OgreScriptLoader.h>
#include <OgreStringConverter.h>
#include <hash_map>
#include <vector>
 
class ConfigNode;
 
class ConfigScriptLoader: public Ogre::ScriptLoader
{
public:
    ConfigScriptLoader();
    ~ConfigScriptLoader();
 
    inline static ConfigScriptLoader &getSingleton() { return *singletonPtr; }
    inline static ConfigScriptLoader *getSingletonPtr() { return singletonPtr; }
 
    Ogre::Real getLoadingOrder() const;
    const Ogre::StringVector &getScriptPatterns() const;
 
    ConfigNode *getConfigScript(const Ogre::String &type, const Ogre::String &name);
 
    void parseScript(Ogre::DataStreamPtr &stream, const Ogre::String &groupName);
 
private:
    static ConfigScriptLoader *singletonPtr;
 
    Ogre::Real mLoadOrder;
    Ogre::StringVector mScriptPatterns;
 
    stdext::hash_map<Ogre::String, ConfigNode*> scriptList;
 
    //Parsing
    char *parseBuff, *parseBuffEnd, *buffPtr;
    size_t parseBuffLen;
 
    enum Token
    {
        TOKEN_Text,
        TOKEN_NewLine,
        TOKEN_OpenBrace,
        TOKEN_CloseBrace,
        TOKEN_EOF,
    };
 
    Token tok, lastTok;
    Ogre::String tokVal, lastTokVal;
    char *lastTokPos;
 
    void _parseNodes(ConfigNode *parent);
    void _nextToken();
    void _prevToken();
};
 
class ConfigNode
{
public:
    ConfigNode(ConfigNode *parent, const Ogre::String &name = "untitled");
    ~ConfigNode();
 
    inline void setName(const Ogre::String &name)
    {
        this->name = name;
    }
 
    inline Ogre::String &getName()
    {
        return name;
    }
 
    inline void addValue(const Ogre::String &value)
    {
        values.push_back(value);
    }
 
    inline void clearValues()
    {
        values.clear();
    }
 
    inline std::vector<Ogre::String> &getValues()
    {
        return values;
    }
 
    inline const Ogre::String &getValue(unsigned int index = 0)
    {
        assert(index < values.size());
        return values[index];
    }
 
    inline float getValueF(unsigned int index = 0)
    {
        assert(index < values.size());
        return Ogre::StringConverter::parseReal(values[index]);
    }
 
    inline double getValueD(unsigned int index = 0)
    {
        assert(index < values.size());
 
        std::istringstream str(values[index]);
        double ret = 0;
        str >> ret;
        return ret;
    }
 
    inline int getValueI(unsigned int index = 0)
    {
        assert(index < values.size());
        return Ogre::StringConverter::parseInt(values[index]);
    }

	inline bool getValueB(unsigned int index = 0)
    {
		assert(index < values.size());
		return Ogre::StringConverter::parseBool(values[index]);
    }

    ConfigNode *addChild(const Ogre::String &name = "untitled", bool replaceExisting = false);
    ConfigNode *findChild(const Ogre::String &name, bool recursive = false);
 
    inline std::vector<ConfigNode*> &getChildren()
    {
        return children;
    }
 
    inline ConfigNode *getChild(unsigned int index = 0)
    {
        assert(index < children.size());
        return children[index];
    }
 
    void setParent(ConfigNode *newParent);
 
    inline ConfigNode *getParent()
    {
        return parent;
    }
 
private:
    Ogre::String name;
    std::vector<Ogre::String> values;
    std::vector<ConfigNode*> children;
    ConfigNode *parent;
 
    int lastChildFound;  //The last child node's index found with a call to findChild()
 
    std::vector<ConfigNode*>::iterator _iter;
    bool _removeSelf;
};
 
#endif