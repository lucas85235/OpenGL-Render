#include "gridlist.h"

#if IMP_RUNTIME(DEV)
#include "dear_imgui/imgui.h"
#endif

namespace ix::samsung::homecomponents {
    void GridList::Setup()
    {
        Rearrange();
    }

    void GridList::Setup(GridListDimension dimension, int dimensionSize, float cellWidth, float cellHeight)
    {
        state_.dimension = dimension;
        state_.dimension_size = dimensionSize;
        state_.cell_width = cellWidth;
        state_.cell_height = cellHeight;

        Rearrange();
    }

    void GridList::AddItem(imp::NodeHandle node)
    {
        // At this moment just set the node parent to the current node
        // More operations could be done in the future, e.g. scaling
        node->SetParent(GetNode());
        children_.push_back(node);
    }
    void GridList::AddItem(imp::NodeHandle node, int index)
    {
        node->SetParent(GetNode());
        auto pos = children_.begin() + index;
        children_.insert(pos, children_.begin(), children_.end());
    }

    void GridList::ChangeItemIndex(int itemIndex, int newIndex)
    {
        if(newIndex >= children_.size()) return;

        auto itemToChangeWith = children_[newIndex];
        children_[newIndex] = children_[itemIndex];
        children_[itemIndex] = itemToChangeWith;
    }

    void GridList::Rearrange()
    {
        int cellPosX, cellPosY;
        for(int i = 0; i < children_.size() ; i++)
        {
            switch(state_.dimension){
                case GridListDimension::GRIDLIST_DIMENSION_ROWS:
                    cellPosX = i % state_.dimension_size;
                    cellPosY = i / state_.dimension_size;
                    break;
                case GridListDimension::GRIDLIST_DIMENSION_COLUMNS:
                    cellPosX = i / state_.dimension_size;
                    cellPosY = i % state_.dimension_size;
                    break;
                default:
                    imp::output::Error("Invalid grid dimension!");
                    break;
            }

            children_[i]->SetLocalPosition(
                {cellPosX * state_.cell_width,
                -cellPosY * state_.cell_height, 0}
            );
        }
    }

    std::vector<imp::NodeHandle> GridList::GetChildren() const
    {
        return children_;
    }

    float GridList::GetCellWidth() const
    {
        return state_.cell_width;
    }

#if IMP_RUNTIME(DEV)
    // Customize Editor UI for this component
    void GridList::DrawEditorUi()
    {
        // Calls Rearrange() on the grid list if the parameters are valid
        if (ImGui::Button("Rearrange items")) {
            if(state_.dimension_size != 0 && state_.cell_width > 0.0 && state_.cell_height > 0.0)
            {
                Rearrange();
            }
            else
            {
                imp::output::Warning("Cannot rearrange GridList with invalid values!");
            }
        }
    }
#endif // IMP_RUNTIME(DEV)
} // namespace ix::samsung::homecomponents