#include "Image/Loaders/LoaderPlatform.h"

#if(INTRA_LIBRARY_IMAGE_LOADING == INTRA_LIBRARY_IMAGE_LOADING_STB)
#error "INTRA_LIBRARY_IMAGE_LOADING_STB is not implemented!"
#elif(INTRA_LIBRARY_IMAGE_LOADING == INTRA_LIBRARY_IMAGE_LOADING_DevIL)
#include "detail/LoaderDevIL.hxx"
#elif(INTRA_LIBRARY_IMAGE_LOADING == INTRA_LIBRARY_IMAGE_LOADING_Gdiplus)
#include "detail/LoaderGdiplus.hxx"
#elif(INTRA_LIBRARY_IMAGE_LOADING == INTRA_LIBRARY_IMAGE_LOADING_Qt)
#include "detail/LoaderQt.hxx"
#elif(INTRA_LIBRARY_IMAGE_LOADING == INTRA_LIBRARY_IMAGE_LOADING_SDL)
#error Not implemented!
#endif
