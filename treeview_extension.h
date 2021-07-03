#include <dwmapi.h>
#include <stdbool.h>

extern HWND treeview;

#define TreeView_IsRootItem(hItem) !TreeView_GetParent(treeview, hItem)

HTREEITEM TreeView_PerformHitTest(DWORD screenPosition, UINT* outFlags);

void TreeView_ClearAllSelectedItems();

bool HandleMultiSelectionClick(HTREEITEM hItem);
bool HandleMultiSelectionChanging(NMTREEVIEW* selectionInfo);
void HandleMultiSelectionChanged(NMTREEVIEW* selectionInfo);

LRESULT CALLBACK TreeviewWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
