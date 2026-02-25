#ifndef COMPONENTS_TAG_H
#define COMPONENTS_TAG_H

#include "core/ncsb/component.h"
#include "imp.h"
#include "absl/status/status.h"
#include <string>

using namespace imp;
using namespace std;

namespace ix::samsung::homecomponents
{
// Displays the texture of the lighting environment in the background.
    class Tag : public Component
    {
    private:
        string this_tag;

    public:
        void Setup(string& tag);
        void Set(string& tag);
        string& Get();
        bool Compare(string& other);
    };

}  // namespace xr::component

#endif // COMPONENTS_TAG_H