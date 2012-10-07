/*
    Copyright © 2010, 2011, 2012 Vladimír Vondruš <mosra@centrum.cz>

    This file is part of Magnum.

    Magnum is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License version 3
    only, as published by the Free Software Foundation.

    Magnum is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU Lesser General Public License version 3 for more details.
*/

#include "TgaImporter.h"

#include <algorithm>
#include <Utility/Endianness.h>

#include "Math/Vector2.h"
#include <Swizzle.h>
#include "Trade/ImageData.h"

using namespace std;
using namespace Corrade::Utility;

namespace Magnum { namespace Trade { namespace TgaImporter {

#ifndef DOXYGEN_GENERATING_OUTPUT
static_assert(sizeof(TgaImporter::Header) == 18, "TgaImporter: header size is not 18 bytes");
#endif

bool TgaImporter::TgaImporter::open(const string& filename, const string& name) {
    ifstream in(filename.c_str());
    if(!in.good()) {
        Error() << "TgaImporter: cannot open file" << filename;
        return false;
    }
    bool status = open(in, name);
    in.close();
    return status;
}

bool TgaImporter::open(istream& in, const string& name) {
    if(_image) close();
    if(!in.good()) {
        Error() << "TgaImporter: cannot read input stream";
        return false;
    }

    /* Check if the file is long enough */
    in.seekg(0, istream::end);
    streampos filesize = in.tellg();
    in.seekg(0, istream::beg);
    if(filesize < streampos(sizeof(Header))) {
        Error() << "TgaImporter: the file is too short:" << filesize << "bytes";
        return false;
    }

    Header header;
    in.read(reinterpret_cast<char*>(&header), sizeof(Header));

    /* Convert to machine endian */
    header.width = Endianness::littleEndian<unsigned short>(header.width);
    header.height = Endianness::littleEndian<unsigned short>(header.height);

    if(header.colorMapType != 0) {
        Error() << "TgaImporter: paletted files are not supported";
        return false;
    }

    if(header.imageType != 2) {
        Error() << "TgaImporter: non-RGB files are not supported";
        return false;
    }

    AbstractImage::Components components;
    switch(header.bpp) {
        case 24:
            #ifndef MAGNUM_TARGET_GLES
            components = AbstractImage::Components::BGR;
            #else
            components = AbstractImage::Components::RGB;
            #endif
            break;
        case 32:
            #ifndef MAGNUM_TARGET_GLES
            components = AbstractImage::Components::BGRA;
            #else
            components = AbstractImage::Components::RGBA;
            #endif
            break;
        default:
            Error() << "TgaImporter: unsupported bits-per-pixel:" << int(header.bpp);
            return false;
    }

    size_t size = header.width*header.height*header.bpp/8;
    GLubyte* buffer = new GLubyte[size];
    in.read(reinterpret_cast<char*>(buffer), size);

    Math::Vector2<GLsizei> dimensions(header.width, header.height);

    #ifdef MAGNUM_TARGET_GLES
    if(components == AbstractImage::Components::RGB) {
        Math::Vector3<GLubyte>* data = reinterpret_cast<Math::Vector3<GLubyte>*>(buffer);
        std::transform(data, data + dimensions.product(), data,
            [](Math::Vector3<GLubyte> pixel) { return swizzle<'b', 'g', 'r'>(pixel); });
    } else /* RGBA */ {
        Math::Vector4<GLubyte>* data = reinterpret_cast<Math::Vector4<GLubyte>*>(buffer);
        std::transform(data, data + dimensions.product(), data,
            [](Math::Vector4<GLubyte> pixel) { return swizzle<'b', 'g', 'r', 'a'>(pixel); });
    }
    #endif

    _image = new ImageData2D(name, dimensions, components, buffer);
    return true;
}

void TgaImporter::close() {
    /** @todo fixme: delete it only if it wasn't retrieved by user */
    //delete _image;
    _image = nullptr;
}

ImageData2D* TgaImporter::image2D(std::uint32_t) {
    return _image;
}

}}}
