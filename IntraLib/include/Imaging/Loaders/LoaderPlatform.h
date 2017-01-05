#pragma once

#include "Platform/PlatformInfo.h"

//! ������������ ����������\API ������� ��� �������� �����������
#define INTRA_LIBRARY_IMAGE_LOADING_None 0
#define INTRA_LIBRARY_IMAGE_LOADING_STB 1
#define INTRA_LIBRARY_IMAGE_LOADING_DevIL 2
#define INTRA_LIBRARY_IMAGE_LOADING_Gdiplus 3
#define INTRA_LIBRARY_IMAGE_LOADING_Qt 4
#define INTRA_LIBRARY_IMAGE_LOADING_SDL 5
#define INTRA_LIBRARY_IMAGE_LOADING_Android 6

//�������� ������������� ���������� ��������� ��������� ���������� ��� �������� �����������
#ifndef INTRA_LIBRARY_IMAGE_LOADING

#if(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Windows)
#define INTRA_LIBRARY_IMAGE_LOADING INTRA_LIBRARY_IMAGE_LOADING_Gdiplus
#elif(INTRA_PLATFORM_OS==INTRA_PLATFORM_OS_Android)
//TODO: ������� �������� ����������� ����� JNI
#define INTRA_LIBRARY_IMAGE_LOADING INTRA_LIBRARY_IMAGE_LOADING_None
#else
#define INTRA_LIBRARY_IMAGE_LOADING INTRA_LIBRARY_IMAGE_LOADING_None
#endif

#endif

#if(INTRA_LIBRARY_IMAGE_LOADING != INTRA_LIBRARY_IMAGE_LOADING_None)

#include "Imaging/Image.h"

namespace Intra { namespace Imaging {

//! ��������� ����������� ���������� ��������� ������������ �������
//! ��� ��������� ���������, �������������� ����� ��������� ��������:
//! GDI+ (Windows)
//! Java API ����� JNI (Android)
//! Qt (�������� ����������, ������ �� ������ ������������ Linux)
//! DevIL (��������� ����������)
//! STB image (��������� ����������)
//! SDL image (��������� ����������)
Image LoadWithPlatform(IO::IInputStream& stream, size_t bytes);

}}

#endif
