#pragma once
#include <napi.h>

// 独立函数：输入文件夹，返回第一张文件的Tag信息和缩略图数据
Napi::Value LoadDicomFolderInfo(const Napi::CallbackInfo& info);

// 初始化 DICOM 模块
void InitDicomModule(Napi::Env env, Napi::Object& exports);
