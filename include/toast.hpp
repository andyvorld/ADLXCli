#pragma once
#include <string>

void ShowToast(std::wstring title, std::wstring content);

inline void ToastSuccess() { ShowToast(L"Update Success", L"New GPU tune applied"); }
inline void ToastNoop() { ShowToast(L"NO-OP", L"Current GPU state matches target"); }
inline void ToastFail() { ShowToast(L"Update Failed", L"Failed to update GPU state"); }