#include <pangolin/platform.h>

#include <fstream>
#include <pangolin/image/typed_image.h>
#include <pangolin/utils/file_extension.h>
// #include <ImfIO.h>

// #ifdef HAVE_OPENEXR
#include <ImfChannelList.h>
#include <ImfInputFile.h>
#include <ImfOutputFile.h>
#include <ImfIO.h>
// #endif // HAVE_OPENEXR

namespace pangolin {

Imf::PixelType OpenEXRPixelType(int channel_bits)
{
    if( channel_bits == 16 ) {
        return Imf::PixelType::HALF;
    }else if( channel_bits == 32 ) {
        return Imf::PixelType::FLOAT;
    }else{
        throw std::runtime_error("Unsupported OpenEXR Pixel Type.");
    }
}

void SetOpenEXRChannels(Imf::ChannelList& ch, const pangolin::PixelFormat& fmt)
{
    const char* CHANNEL_NAMES[] = {"R","G","B","A"};
    for(size_t c=0; c < fmt.channels; ++c) {
        ch.insert( CHANNEL_NAMES[c], Imf::Channel(OpenEXRPixelType(fmt.channel_bits[c])) );
    }
}

void SaveExr(const Image<unsigned char>& image_in, const pangolin::PixelFormat& fmt, const std::string& filename, bool top_line_first)
{
#ifdef HAVE_OPENEXR
    ManagedImage<unsigned char> flip_image;
    Image<unsigned char> image;

    if(top_line_first) {
        image = image_in;
    }else{
        flip_image.Reinitialise(image_in.pitch,image_in.h);
        for(size_t y=0; y<image_in.h; ++y) {
            std::memcpy(flip_image.RowPtr(y), image_in.RowPtr(y), image_in.pitch);
        }
        image = flip_image;
    }


    Imf::Header header (image.w, image.h);
    SetOpenEXRChannels(header.channels(), fmt);

    Imf::OutputFile file (filename.c_str(), header);
    Imf::FrameBuffer frameBuffer;

    size_t ch_bits = 0;
    const char* CHANNEL_NAMES[] = {"R","G","B","A"};
    for(unsigned int i=0; i<fmt.channels; i++)
    {
        const Imf::Channel *channel = header.channels().findChannel(CHANNEL_NAMES[i]);
        frameBuffer.insert(
            CHANNEL_NAMES[i],
            Imf::Slice(
                channel->type,
                (char*)image.ptr + (ch_bits/8),
                fmt.bpp/8,  // xstride
                image.pitch // ystride
            )
        );

        ch_bits += fmt.channel_bits[i];
    }

    file.setFrameBuffer(frameBuffer);
    file.writePixels(image.h);

#else
    PANGOLIN_UNUSED(image_in);
    PANGOLIN_UNUSED(fmt);
    PANGOLIN_UNUSED(filename);
    PANGOLIN_UNUSED(top_line_first);
    throw std::runtime_error("EXR Support not enabled. Please rebuild Pangolin.");
#endif // HAVE_OPENEXR
}
}