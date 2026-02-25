#include "native/components/tag/tag.h"
#include <string>

using namespace std;

namespace ix::samsung::homecomponents
{
    void Tag::Setup(string& tag)
    {
        Set(tag);
    }

    void Tag::Set(string& tag)
    {
        this_tag = tag;
    }

    string& Tag::Get()
    {
        return this_tag;
    }

    bool Tag::Compare(string& other)
    {
        return this_tag == other;
    }

}  // namespace ix::samsung::homecomponents
