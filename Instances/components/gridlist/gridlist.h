#ifndef COMPONENTS_GRIDLIST_H
#define COMPONENTS_GRIDLIST_H

#include "imp.h"
#include "proto/components/gridlist_state.proto.imp.h"

namespace ix::samsung::homecomponents
{
    class GridList : public imp::Component
    {
    public:
        void Setup();
        void Setup(GridListDimension dimension, int dimensionSize, float cellWidth, float cellHeight);
        void AddItem(imp::NodeHandle node, int index);
        void AddItem(imp::NodeHandle node);
        void ChangeItemIndex(int itemIndex, int newIndex);
        void Rearrange();
        std::vector<imp::NodeHandle> GetChildren() const;
        float GetCellWidth() const;

    private:
        GridListState state_;
        GridListDimension dimension_;
        std::vector<imp::NodeHandle> children_;

    public:
        using IsfInfo = imp::IsfInfo<&GridList::state_>;
#if IMP_RUNTIME(DEV)
        void DrawEditorUi();
#endif
    };
} // namespace ix::samsung::homecomponents

#endif // COMPONENTS_GRIDLIST_H