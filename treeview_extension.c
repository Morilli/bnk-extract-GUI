#include <windows.h>
#include <commctrl.h>
#include <stdbool.h>

HWND treeview;

HTREEITEM TreeView_PerformHitTest(DWORD screenPosition, UINT* outFlags)
{
    POINTS mousePosition = MAKEPOINTS(screenPosition);
    POINT point = {.x = mousePosition.x, .y = mousePosition.y};
    ScreenToClient(treeview, &point);
    TVHITTESTINFO hitTestInfo = {.pt = point};
    HTREEITEM hItem = TreeView_HitTest(treeview, &hitTestInfo);
    if (outFlags) *outFlags = hitTestInfo.flags;
    return hItem;
}

void TreeView_ClearAllSelectedItems()
{
    void ClearChildren(HTREEITEM currentRoot) {
        HTREEITEM child = TreeView_GetChild(treeview, currentRoot);
        if (child)
        do {
            TreeView_SetItemState(treeview, child, 0, TVIS_SELECTED);
            ClearChildren(child);
        } while ( (child = TreeView_GetNextSibling(treeview, child)));
    }

    HTREEITEM root = TreeView_GetRoot(treeview);
    do {
        TreeView_SetItemState(treeview, root, 0, TVIS_SELECTED);
        ClearChildren(root);
    } while ( (root = TreeView_GetNextSibling(treeview, root)));
}

static bool previousItemSelected[2];

bool HandleMultiSelectionClick(HTREEITEM hItem)
{
    static HTREEITEM previousItem;
    bool controlDown = GetKeyState(VK_CONTROL) & 0x8000;
    bool shiftDown = GetKeyState(VK_SHIFT) & 0x8000;
    bool itemIsSelected = true;
    if (controlDown && !shiftDown) {
        UINT itemState = TreeView_GetItemState(treeview, hItem, TVIS_SELECTED);
        itemState ^= TVIS_SELECTED;
        TreeView_SetItemState(treeview, hItem, itemState, TVIS_SELECTED);
        itemIsSelected = (itemState & TVIS_SELECTED) == TVIS_SELECTED;
        if (itemIsSelected || (!itemIsSelected && previousItem == hItem)) {
            previousItemSelected[0] = previousItemSelected[1];
            previousItemSelected[1] = itemIsSelected;
            previousItem = hItem;
        }
    } else if (!controlDown && !shiftDown) {
        TreeView_ClearAllSelectedItems();
        TreeView_SetItemState(treeview, hItem, TVIS_SELECTED, TVIS_SELECTED); // select new item manually
        previousItemSelected[1] = true;
        previousItem = hItem;
    }

    return !itemIsSelected;
}

void HandleMultiSelectionChanged(NMTREEVIEW* selectionInfo)
{
    static HTREEITEM initialItem;
    bool controlDown = GetKeyState(VK_CONTROL) & 0x8000;
    bool shiftDown = GetKeyState(VK_SHIFT) & 0x8000;
    if (selectionInfo->action == TVC_BYMOUSE && controlDown && previousItemSelected[0])
        TreeView_SetItemState(treeview, selectionInfo->itemOld.hItem, TVIS_SELECTED, TVIS_SELECTED); // old item should *not* get deselected
    if (shiftDown && selectionInfo->action == TVC_BYKEYBOARD) {
        if (!initialItem) initialItem = selectionInfo->itemOld.hItem;
        if (initialItem == selectionInfo->itemOld.hItem)
            TreeView_SetItemState(treeview, initialItem, TVIS_SELECTED, TVIS_SELECTED); // the initial item should never get deselected
        HTREEITEM currentItem = TreeView_GetRoot(treeview);
        bool selectItem = false;
        do {
            selectItem ^= currentItem == initialItem || currentItem == selectionInfo->itemNew.hItem;
            if (selectItem) TreeView_SetItemState(treeview, currentItem, TVIS_SELECTED, TVIS_SELECTED);
            if (initialItem == selectionInfo->itemNew.hItem) selectItem = false;
        } while ( (currentItem = TreeView_GetNextVisible(treeview, currentItem)) );
    } else {
        initialItem = NULL;
    }
}

bool HandleMultiSelectionChanging(NMTREEVIEW* selectionInfo)
{
    bool return_value = false;
    bool controlDown = GetKeyState(VK_CONTROL) & 0x8000;
    bool shiftDown = GetKeyState(VK_SHIFT) & 0x8000;
    if (shiftDown && selectionInfo->action == TVC_BYMOUSE) {
        if (!controlDown) TreeView_ClearAllSelectedItems();
        TreeView_SetItemState(treeview, selectionInfo->itemOld.hItem, TVIS_SELECTED, TVIS_SELECTED); // old item should *not* get deselected
        HTREEITEM currentItem = TreeView_GetRoot(treeview);
        bool selectItem = false;
        do {
            selectItem ^= currentItem == selectionInfo->itemOld.hItem || currentItem == selectionInfo->itemNew.hItem;
            if (selectItem) TreeView_SetItemState(treeview, currentItem, TVIS_SELECTED, TVIS_SELECTED);
        } while ( (currentItem = TreeView_GetNextVisible(treeview, currentItem)) );
        return_value = true;
    }
    if (!controlDown && !shiftDown) {
        TreeView_ClearAllSelectedItems();
    }
    TreeView_SetItemState(treeview, selectionInfo->itemNew.hItem, TVIS_SELECTED, TVIS_SELECTED); // select new item manually

    return return_value;
}

LRESULT CALLBACK TreeviewWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, __attribute__((unused)) UINT_PTR uIdSubclass, __attribute__((unused)) DWORD_PTR dwRefData)
{
    // this is a retarded construct which attempt to remove the default behavior of a right click selecting the underlying item of a treeview
    // for some reason there is no proper way to do this (?), so it's completely hacked in here
    switch (msg)
    {
        case WM_RBUTTONDOWN: {
            RECT aSelectedItemPosition;
            TreeView_GetItemRect(treeview, TreeView_GetSelection(treeview), &aSelectedItemPosition, TVGIPR_BUTTON);
            lParam = MAKELPARAM(aSelectedItemPosition.right, aSelectedItemPosition.top);
            break;
        }
        case WM_RBUTTONUP:
            return SendNotifyMessage(GetParent(hWnd), WM_NOTIFY, 0, (LPARAM) &(NMHDR) {.hwndFrom = hWnd, .code = NM_RCLICK});
        case WM_KILLFOCUS:
            return 0; // if this gets processed normally, the last selected item gets... "greyed" somehow, which doesn't make much sense for multiselection or in general
    }

    return DefSubclassProc(hWnd, msg, wParam, lParam);
}
